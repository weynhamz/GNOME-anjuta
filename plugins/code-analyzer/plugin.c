/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * plugin.c
 * Copyright (C) Johannes Schmid 2010 <jhs@gnome.org>
 * 
 * code-analyzer is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * code-analyzer is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-environment.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>
#include <glib/gi18n.h>

#include "plugin.h"

#define ICON_FILE "code-analyzer.png"

#define BUILDER_FILE PACKAGE_DATA_DIR"/glade/code-analyzer.ui"
#define CLANG_PREFS_ROOT "clang_preferences"
#define PREF_SCHEMA "org.gnome.anjuta.code-analyzer"

#define PREF_ENABLED "clang-enable"
#define PREF_CC_PATH "clang-cc-path"
#define PREF_CXX_PATH "clang-cxx-path"

static gpointer parent_class;

static gboolean
code_analyzer_activate (AnjutaPlugin *plugin)
{
	CodeAnalyzerPlugin *code_analyzer;
	
	DEBUG_PRINT ("%s", "CodeAnalyzerPlugin: Activating CodeAnalyzerPlugin plugin ...");
	code_analyzer = (CodeAnalyzerPlugin*) plugin;

	return TRUE;
}

static gboolean
code_analyzer_deactivate (AnjutaPlugin *plugin)
{

	DEBUG_PRINT ("%s", "CodeAnalyzerPlugin: Dectivating CodeAnalyzerPlugin plugin ...");
	
	return TRUE;
}

static void
code_analyzer_finalize (GObject *obj)
{
	/* Finalization codes here */
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
code_analyzer_dispose (GObject *obj)
{
	/* Disposition codes */
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
code_analyzer_instance_init (GObject *obj)
{
	CodeAnalyzerPlugin *plugin = (CodeAnalyzerPlugin*)obj;
	plugin->settings = g_settings_new (PREF_SCHEMA);
}

static void
code_analyzer_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = code_analyzer_activate;
	plugin_class->deactivate = code_analyzer_deactivate;
	klass->finalize = code_analyzer_finalize;
	klass->dispose = code_analyzer_dispose;
}

static gchar*
code_analyzer_get_cc_path (CodeAnalyzerPlugin* ca_plugin)
{
	gchar* path = g_settings_get_string (ca_plugin->settings,
	                                     PREF_CC_PATH);
	if (!g_file_test (path, G_FILE_TEST_IS_EXECUTABLE))
	{
		g_free (path);
		return NULL;
	}
	return path;
}

static gchar*
code_analyzer_get_cxx_path (CodeAnalyzerPlugin* ca_plugin)
{
	gchar* path = g_settings_get_string (ca_plugin->settings,
	                                     PREF_CXX_PATH);
	if (!g_file_test (path, G_FILE_TEST_IS_EXECUTABLE))
	{
		g_free (path);
		return NULL;
	}
	return path;
}


/* IAnjutaEnvironment implementation */
static gboolean 
ienvironment_override (IAnjutaEnvironment* env_iface,
                       gchar **dirp, gchar ***argvp, gchar ***envp,
                       GError** error)
{
	CodeAnalyzerPlugin* ca_plugin = (CodeAnalyzerPlugin*) (env_iface);
	gchar* command = *argvp[0];

	if (!g_settings_get_boolean (ca_plugin->settings,
	                             PREF_ENABLED))
		return TRUE;

	

	/* Check if this is a command we are interested in */	                     
	if (strcmp (command, "autogen.sh") ||
	    strcmp (command, "configure") ||
	    strcmp (command, "make"))
	{
		gchar** new_env = *envp;
		gchar** env;
		gchar* cc = code_analyzer_get_cc_path (ca_plugin);
		gchar* cxx = code_analyzer_get_cxx_path (ca_plugin);
		gboolean found_cc = FALSE;
		gboolean found_cxx = FALSE;
		/* NULL termination */
		gsize size = 1;

		/* Check if paths are correct */
		if (!cc || !cxx)
		{
			if (error)
				*error = g_error_new (ianjuta_environment_error_quark (), 
				                      IANJUTA_ENVIRONMENT_CONFIG, "%s",
				                      _("Couldn't find clang analyzer, please check "
				                        "if it is installed and if the paths are configured "
				                        "correctly in the preferences"));
			g_free (cc);
			g_free (cxx);
			return FALSE;
		}

		for (env = new_env; *env != NULL; env++)
		{
			if (g_str_has_prefix (*env, "CC="))
			{
				g_free (*env);
				*env = g_strdup_printf("CC=%s", cc);
				found_cc = TRUE;
			}
			else if (g_str_has_prefix (*env, "CXX="))
			{
				g_free (*env);
				*env = g_strdup_printf("CXX=%s", cxx);
				found_cxx = TRUE;
			}
			size++;
		}
		if (!found_cc)
		{
			new_env = g_realloc (new_env, sizeof (gchar**) * (size + 1));
			new_env[size - 1] = g_strdup_printf("CC=%s", cc);
			new_env[size] = NULL;
			size++;
		}
		if (!found_cxx)
		{
			new_env = g_realloc (new_env, sizeof (gchar**) * (size + 1));
			new_env[size - 1] = g_strdup_printf("CXX=%s", cxx);
			new_env[size] = NULL;
		}
		*envp = new_env;
	}
	return TRUE;
}

static gchar* 
ienvironment_get_real_directory (IAnjutaEnvironment* env_iface, gchar *dir,
                                 GError** error)
{
	return dir;
}

static void
ienvironment_iface_init (IAnjutaEnvironmentIface* iface)
{
	iface->override = ienvironment_override;
	iface->get_real_directory = ienvironment_get_real_directory;
}

/* IAnjutaPreferences implementation
 *---------------------------------------------------------------------------*/

static void
ipreferences_merge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	GtkBuilder *bxml;
	CodeAnalyzerPlugin* ca_plugin = (CodeAnalyzerPlugin*) (ipref);
	/* Create the preferences page */
	bxml =  anjuta_util_builder_new (BUILDER_FILE, NULL);
	if (!bxml) return;
		
	anjuta_preferences_add_from_builder (prefs, bxml, ca_plugin->settings, 
	                                     CLANG_PREFS_ROOT, _("CLang Analyzer"),  ICON_FILE);
	
	g_object_unref (bxml);
}

static void
ipreferences_unmerge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	anjuta_preferences_remove_page(prefs, _("CLang Analyzer"));
}

static void
ipreferences_iface_init(IAnjutaPreferencesIface* iface)
{
	iface->merge = ipreferences_merge;
	iface->unmerge = ipreferences_unmerge;	
}


ANJUTA_PLUGIN_BEGIN (CodeAnalyzerPlugin, code_analyzer);
ANJUTA_PLUGIN_ADD_INTERFACE(ienvironment, IANJUTA_TYPE_ENVIRONMENT);
ANJUTA_PLUGIN_ADD_INTERFACE(ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (CodeAnalyzerPlugin, code_analyzer);
