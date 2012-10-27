/* -*- Mode: C; indent-spaces-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) 2012 Carl-Anton Ingmarsson

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <config.h>

#include "plugin.h"

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-environment.h>

#include <sys/wait.h>

struct _JHBuildPluginClass
{
    AnjutaPluginClass parent_class;
};

#define JHBUILD_PLUGIN_ERROR (jhbuild_plugin_error_quark())
static GQuark jhbuild_plugin_error_quark(void);

/* G_DEFINE_QUARK is defined in GLib 2.34 provide a fallback for Glib 2.32 */
#ifndef G_DEFINE_QUARK
#define G_DEFINE_QUARK(QN, q_n)                      \
GQuark                                               \
q_n##_quark (void)                                   \
{                                                    \
      return g_quark_from_string (#QN);              \
}
#endif

G_DEFINE_QUARK(JHBUILD_PLUGIN_ERROR, jhbuild_plugin_error);

static char *
jhbuild_plugin_environment_get_real_directory(IAnjutaEnvironment *obj,
                                              gchar *dir,
                                              GError **err)
{
    return dir;
}

static gboolean
jhbuild_plugin_environment_override(IAnjutaEnvironment* environment,
                                    char** dirp,
                                    char*** argvp,
                                    char*** envp,
                                    GError** error)
{
    JHBuildPlugin* self = ANJUTA_PLUGIN_JHBUILD(environment);
    
    gboolean add_prefix = FALSE;
    guint argvp_length;
    char** new_argv;

    if (g_str_has_suffix (*argvp[0], "configure") || g_str_has_suffix (*argvp[0], "autogen.sh"))
    {
        char** argv_iter;

        add_prefix = TRUE;
        for (argv_iter = *argvp; *argv_iter; argv_iter++)
        {
            if (g_str_has_prefix (*argv_iter, "--prefix") ||
                g_str_has_prefix (*argv_iter, "--libdir"))
            {
                add_prefix = FALSE;
                break;
            }
        }
    }

    argvp_length = g_strv_length (*argvp);
    new_argv = g_new(char*, argvp_length + (add_prefix ? 5 : 3));
    new_argv[0] = g_strdup("jhbuild");
    new_argv[1] = g_strdup("run");
    
    memcpy(new_argv + 2, *argvp, sizeof(char*) * argvp_length);
    
    if (add_prefix)
    {
        new_argv[2 + argvp_length] = g_strdup_printf("--prefix=%s", self->prefix);
        new_argv[2 + argvp_length + 1] = g_strdup_printf("--libdir=%s", self->libdir);
        new_argv[2 + argvp_length + 2] = NULL;
    }
    else
        new_argv[2 + argvp_length] = NULL;

    g_free(*argvp);
    *argvp = new_argv;
    
    return TRUE;
}

static char*
run_jhbuild_env (GError** error)
{
    char* argv[] = {"jhbuild", "run", "env", NULL};
    GError* err = NULL;
    char* standard_output;
    char* standard_error;
    int exit_status;

    if (!g_spawn_sync(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL,
                      &standard_output, &standard_error, &exit_status, &err))
    {
        g_propagate_prefixed_error (error, err, "Failed to run \"jhbuild run");
        return NULL;
    }

    if (WIFEXITED(exit_status) && WEXITSTATUS(exit_status) != 0)
    {
        g_set_error(error, JHBUILD_PLUGIN_ERROR, 0, "Failed to run \"jhbuild run\" (%s)", standard_error);
        g_free(standard_error);
        g_free(standard_output);
        return NULL;
    }

    g_free(standard_error);
    return standard_output;
}

static gboolean
jhbuild_plugin_get_jhbuild_info(JHBuildPlugin* self, GError** error)
{
    char* output;
    char** env_variables;
    char** variable;

    if (!(output = run_jhbuild_env(error)))
        return FALSE;

    env_variables = g_strsplit(output, "\n", 0);
    g_free(output);


    /* First scan for JHBUILD_PREFIX */
    for (variable = env_variables; *variable; ++variable)
    {
        if (g_str_has_prefix (*variable, "JHBUILD_PREFIX="))
        {
            const char* value;

            value = *variable + strlen("JHBUILD_PREFIX=");
            self->prefix = g_strdup(value);
        }
    }
    
    if (!self->prefix)
    {
        g_set_error_literal (error, ANJUTA_SHELL_ERROR, 0, "Could not find the JHBuild install prefix.");
        g_strfreev(env_variables);
        return FALSE;
    }

    /* Then we scan for LD_LIBRARY_PATH to extract libdir from there. */
    for (variable = env_variables; *variable; ++variable)
    {
        if (g_str_has_prefix (*variable, "LD_LIBRARY_PATH="))
        {
            const char* value;
            char **values, **iter;
            
            value = *variable + strlen("LD_LIBRARY_PATH=");
            if (!value)
                break;

            values = g_strsplit(value, ";", -1);
            for (iter = values; *iter; ++iter)
            {
                if (g_str_has_prefix (*iter, self->prefix))
                {
                    self->libdir = g_strdup(*iter);
                    break;
                }
            }
            g_strfreev(values);
            break;
        }
    }

    g_strfreev(env_variables);

    if (!self->libdir)
    {
        g_set_error_literal (error, JHBUILD_PLUGIN_ERROR, 0, "Could not find the JHBuild library directory.");
        return FALSE;
    }

    return TRUE;
}

static gboolean
jhbuild_plugin_activate(AnjutaPlugin* plugin)
{
    JHBuildPlugin *self = ANJUTA_PLUGIN_JHBUILD (plugin);

    GError *err = NULL;
    
    if (!jhbuild_plugin_get_jhbuild_info(self, &err))
    {
        anjuta_util_dialog_error(GTK_WINDOW(self->shell),
            "Failed to activate the JHBuild Plugin: %s", err->message);
        g_error_free (err);
        return FALSE;
    }
    return TRUE;
}

static gboolean
jhbuild_plugin_deactivate (AnjutaPlugin *plugin)
{
    return TRUE;
}

/* GObject functions
 *---------------------------------------------------------------------------*/

/* Used in dispose and finalize */
static gpointer parent_class;

static void
jhbuild_plugin_instance_init (GObject *obj)
{
    JHBuildPlugin *self = ANJUTA_PLUGIN_JHBUILD (obj);

    self->shell = anjuta_plugin_get_shell (ANJUTA_PLUGIN(self));
}

static void
jhbuild_plugin_finalize (GObject *obj)
{
    JHBuildPlugin *self = ANJUTA_PLUGIN_JHBUILD (obj);

    g_free(self->prefix);
    g_free(self->libdir);
    

    G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
jhbuild_plugin_class_init (GObjectClass *klass) 
{
    AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

    parent_class = g_type_class_peek_parent (klass);

    plugin_class->activate = jhbuild_plugin_activate;
    plugin_class->deactivate = jhbuild_plugin_deactivate;
    klass->finalize = jhbuild_plugin_finalize;
}

static void
ienvironment_iface_init(IAnjutaEnvironmentIface *iface)
{
    iface->override = jhbuild_plugin_environment_override;
    iface->get_real_directory = jhbuild_plugin_environment_get_real_directory;
}

ANJUTA_PLUGIN_BEGIN (JHBuildPlugin, jhbuild_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE (ienvironment, IANJUTA_TYPE_ENVIRONMENT);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (JHBuildPlugin, jhbuild_plugin);
