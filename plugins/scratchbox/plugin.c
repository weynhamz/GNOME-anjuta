/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) 2008 Sébastien Granjoux

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

/*
 * Plugins functions
 *
 *---------------------------------------------------------------------------*/

#include <config.h>

#include "plugin.h"

#include <libanjuta/anjuta-launcher.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-environment.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>

/* Constantes
 *---------------------------------------------------------------------------*/

#define ICON_FILE "anjuta-scratchbox-48.png"
#define GLADE_FILE PACKAGE_DATA_DIR"/glade/anjuta-scratchbox.glade"

#define SB_ENTRY "preferences_folder:text:/scratchbox:0:build.scratchbox.path"
#define SB_TARGET_ENTRY "combo_target"
#define SB_SBOX_ENTRY "preferences_combo:text:Sbox1,Sbox2:0:scratchbox.version"

#define PREF_SB_PATH "build.scratchbox.path"
#define PREF_SB_VERSION "scratchbox.version"

/* Type defintions
 *---------------------------------------------------------------------------*/

struct _ScratchboxPluginClass
{
	AnjutaPluginClass parent_class;
};

struct _ScratchboxPlugin
{
	AnjutaPlugin parent;
	AnjutaLauncher *launcher;

	/* Plugin Data */
	gchar *user_dir;
	gchar **target_list;
	gchar *sb_dir;
	gchar *target;
	gint id;
	gint combo_element;
	GString *buffer;
};

GladeXML *gxml;

#define EXECUTE_CMD	0
#define TARGET_LIST	1

static gchar *
sbox2_commands_args[][2] = {
	{ "bin/sb2", "-t" },	/* execute command */
	{ "bin/sb2-config", "-l"},	/* target list */
	{ NULL, NULL },
};

static gchar *
sbox1_commands_args[][2] = {
	{ "bin/login", "-d" },
	{ "bin/sb-conf", "--list" },	/* target list */
	{ NULL, NULL },
};

/* Callback for saving session
 *---------------------------------------------------------------------------*/

static void
on_session_save (AnjutaShell *shell, AnjutaSessionPhase phase, AnjutaSession *session, ScratchboxPlugin *self)
{
	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;
}

static void on_session_load (AnjutaShell *shell, AnjutaSessionPhase phase, AnjutaSession *session, ScratchboxPlugin *self)
{
	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;
}

/* Callbacks
 *---------------------------------------------------------------------------*/

static void on_list_terminated (AnjutaLauncher *launcher, gint child_pid,
                                gint status, gulong time_taken, gpointer data)
{
	g_return_if_fail (launcher != NULL);
	
	ScratchboxPlugin* plugin = ANJUTA_PLUGIN_SCRATCHBOX (data);

	if (!(status != 0 || !plugin->buffer)) {
		/* Program terminate normaly */
		gint str_splitted_length;
		gint i;
		plugin->target_list = g_strsplit (plugin->buffer->str, "\n", 0);
		str_splitted_length = g_strv_length (plugin->target_list) - 1;
		
		GtkWidget* combo_target_entry;
		combo_target_entry = glade_xml_get_widget(gxml,
							  SB_TARGET_ENTRY);

		for (i = 1; i < plugin->combo_element; i++)
			gtk_combo_box_remove_text(GTK_COMBO_BOX(combo_target_entry), 1);
		plugin->combo_element = 1;

		for (i = 0; i < str_splitted_length; i++) {
			gtk_combo_box_append_text(GTK_COMBO_BOX(combo_target_entry), plugin->target_list[i]);
			plugin->combo_element++;
		}
                
		/* enable target combo box */
		gtk_combo_box_set_active (GTK_COMBO_BOX(combo_target_entry),
					  plugin->id);
                gtk_widget_set_sensitive(combo_target_entry, TRUE);
		g_strfreev (plugin->target_list);
	}

	plugin->target_list = NULL;
}

static void on_target (AnjutaLauncher *launcher, AnjutaLauncherOutputType out,
		       const gchar* line, gpointer data)
{
	ScratchboxPlugin* plugin = ANJUTA_PLUGIN_SCRATCHBOX (data);
	g_return_if_fail (line != NULL);
	g_return_if_fail (plugin != NULL);

	g_string_append (plugin->buffer, line);
	
}

static void
on_change_target(GtkComboBox *combo, ScratchboxPlugin *plugin)
{
	AnjutaShell* shell = ANJUTA_PLUGIN (plugin)->shell;
	gint id;
	
	g_return_if_fail (plugin != NULL);
	id = gtk_combo_box_get_active (combo);
	if (plugin->target) {
		g_free(plugin->target);
		plugin->target = NULL;
	}
	plugin->target = gtk_combo_box_get_active_text (combo);
	plugin->id = id > 0 ? id :0;

	anjuta_preferences_set_int (anjuta_shell_get_preferences (shell, NULL),
                                SB_TARGET_ENTRY,
                                plugin->id);
}

static void
on_update_target(GtkComboBox *combo, ScratchboxPlugin *plugin)
{
	AnjutaPreferences* prefs;
	GString* command = g_string_new (NULL);
	gchar* sbox_commands;
	gchar* sbox_args;
	gchar* sb_dir;
	gchar* sb_ver;

	g_return_if_fail (plugin != NULL);

	prefs = anjuta_shell_get_preferences (ANJUTA_PLUGIN (plugin)->shell,
						NULL);
        sb_ver = anjuta_preferences_get(prefs, PREF_SB_VERSION);

	sb_dir = anjuta_preferences_get(prefs, PREF_SB_PATH);

        if (!sb_dir)
                return;

        g_string_printf (command, "%s%s", sb_dir, G_DIR_SEPARATOR_S);

        if (!strcmp(sb_ver, "Sbox1")) {
		sbox_commands = sbox1_commands_args[TARGET_LIST][0];
		sbox_args = sbox1_commands_args[TARGET_LIST][1];
        } else {
		sbox_commands = sbox2_commands_args[TARGET_LIST][0];
		sbox_args = sbox2_commands_args[TARGET_LIST][1];
	}

	g_string_append (command, sbox_commands);

	if (g_file_test (command->str, G_FILE_TEST_EXISTS) == FALSE)
        {
		anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
					_("Program '%s' does not exists"), command->str);
		return;
        }

	g_string_append_printf(command, " %s", sbox_args);
	
	if (!anjuta_launcher_is_busy (plugin->launcher))
	{
		GtkWidget* combo_target_entry;
		
		if (plugin->buffer != NULL) {
			g_string_free (plugin->buffer, TRUE);
			plugin->buffer = NULL;
		}
		
		plugin->buffer = g_string_new(NULL);

		combo_target_entry = glade_xml_get_widget(gxml,
							  SB_TARGET_ENTRY);
		/* disable target combo box */
		gtk_widget_set_sensitive(combo_target_entry, FALSE);

		anjuta_launcher_execute (plugin->launcher, command->str,
					(AnjutaLauncherOutputCallback)on_target,
					plugin);
	}

	g_string_free(command, TRUE);

}

static void
on_change_directory(GtkFileChooserButton *FileChooser, gpointer user_data)
{
	ScratchboxPlugin *plugin = (ScratchboxPlugin *) user_data;
	GtkWidget* combo_sbox_entry;
	gchar *old_dir;

	combo_sbox_entry = glade_xml_get_widget(gxml,
						  SB_SBOX_ENTRY);
	old_dir = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER(FileChooser));

	if (!plugin->user_dir || strcmp(old_dir, plugin->user_dir) != 0) {
		if (plugin->user_dir)
			g_free(plugin->user_dir);
		plugin->user_dir = g_strdup(old_dir);
	} else
		return;

	g_free(old_dir);

	on_update_target(GTK_COMBO_BOX(combo_sbox_entry), plugin);
}

/* Actions table
 *---------------------------------------------------------------------------*/

/* AnjutaPlugin functions
 *---------------------------------------------------------------------------*/

static gboolean
scratchbox_plugin_activate (AnjutaPlugin *plugin)
{
	ScratchboxPlugin *self = ANJUTA_PLUGIN_SCRATCHBOX (plugin);
	
	DEBUG_PRINT ("%s", "Scratchbox 1 and 2 Plugin: Activating plugin...");

	self->launcher = anjuta_launcher_new ();

	/* Connect to session signal */
	g_signal_connect (plugin->shell, "save-session",
					  G_CALLBACK (on_session_save), self);
	g_signal_connect (plugin->shell, "load-session",
					  G_CALLBACK (on_session_load), self);

	/* Connect launcher signal */
	g_signal_connect (self->launcher, "child-exited",
                          G_CALLBACK (on_list_terminated), self);
	
	return TRUE;
}

static gboolean
scratchbox_plugin_deactivate (AnjutaPlugin *plugin)
{
	ScratchboxPlugin *self = ANJUTA_PLUGIN_SCRATCHBOX (plugin);

	DEBUG_PRINT ("%s", "Scratchbox 1 Plugin: Deactivating plugin...");
	
	g_signal_handlers_disconnect_by_func (plugin->shell, G_CALLBACK (on_session_save), self);
	g_signal_handlers_disconnect_by_func (plugin->shell, G_CALLBACK (on_session_load), self);
	
	return TRUE;
}

static void
sbox2_environment_override (IAnjutaEnvironment* environment, gchar **dir, gchar ***argvp, gchar ***envp, GError** err)
{
	ScratchboxPlugin *plugin = ANJUTA_PLUGIN_SCRATCHBOX (environment);
	AnjutaPreferences* prefs;
	gchar **new_argv;
	gchar* sb_dir;
	int i;

	if (plugin->target == NULL || !strcmp(plugin->target, "host"))
		return;

	prefs = anjuta_shell_get_preferences (ANJUTA_PLUGIN (plugin)->shell, NULL);
	sb_dir = anjuta_preferences_get(prefs, PREF_SB_PATH);

	if (plugin->user_dir) g_free (plugin->user_dir);
	plugin->user_dir = g_strconcat (sb_dir, G_DIR_SEPARATOR_S, NULL);

	/* Build in scratchbox environment */
	gsize len_argv = g_strv_length (*argvp);

	/* Add scratchbox login */
	new_argv = g_new (gchar*, len_argv + 4);
	new_argv[0] = g_strconcat (sb_dir, G_DIR_SEPARATOR_S,
				   sbox2_commands_args[EXECUTE_CMD][0],
				   NULL);
	new_argv[1] = g_strconcat (sbox2_commands_args[EXECUTE_CMD][1], NULL);
	new_argv[2] = g_strconcat (plugin->target, NULL);

	for (i = 0; i < len_argv; i++)
		new_argv[3 + i] = g_strconcat("\"", *(*argvp + i), "\"", NULL);
	
	g_free (*argvp);
	*argvp = new_argv;
	g_free(sb_dir);
}
static void
sbox1_environment_override (IAnjutaEnvironment* environment, gchar **dir, gchar ***argvp, gchar ***envp, GError** err)
{
	ScratchboxPlugin *plugin = ANJUTA_PLUGIN_SCRATCHBOX (environment);
	AnjutaPreferences* prefs;
	gchar* sb_dir;
	gsize len;

	prefs = anjuta_shell_get_preferences (ANJUTA_PLUGIN (plugin)->shell, NULL);

	sb_dir = anjuta_preferences_get(prefs, PREF_SB_PATH);
	
	if (plugin->user_dir) g_free (plugin->user_dir);
	plugin->user_dir = g_strconcat (sb_dir, G_DIR_SEPARATOR_S,
						   "users", G_DIR_SEPARATOR_S,
						   g_get_user_name(), NULL);
	
	len = strlen (plugin->user_dir);

	if (strncmp (*dir, plugin->user_dir, len) == 0)
	{
		/* Build in scratchbox environment */
		gchar **new_argv;
		gsize len_argv = g_strv_length (*argvp);

		/* Add scratchbox login */
		new_argv = g_new (gchar*, len_argv + 3);
		memcpy (new_argv + 2, *argvp, sizeof(gchar *) * (len_argv + 1));
		new_argv[0] = g_strconcat (sb_dir, G_DIR_SEPARATOR_S,
					   sbox1_commands_args[EXECUTE_CMD][0],
					   NULL);
		new_argv[1] = g_strconcat (sbox1_commands_args[EXECUTE_CMD][1],
					   (*dir) + len, NULL);
		
		g_free (*argvp);
		*argvp = new_argv;
	}

	g_free (sb_dir);
}

/* IAnjutaEnvironment implementation
 *---------------------------------------------------------------------------*/

static gboolean
ienvironment_override (IAnjutaEnvironment* environment, gchar **dir, gchar ***argvp, gchar ***envp, GError** err)
{
	ScratchboxPlugin *plugin = ANJUTA_PLUGIN_SCRATCHBOX (environment);
	AnjutaPreferences* prefs;
	gchar* sb_dir;
	gchar* sb_ver;

	prefs = anjuta_shell_get_preferences (ANJUTA_PLUGIN (plugin)->shell, NULL);
	sb_dir = anjuta_preferences_get(prefs, PREF_SB_PATH);

	if (!sb_dir)
		return FALSE;

	sb_ver = anjuta_preferences_get(prefs, PREF_SB_VERSION);
	if (!strcmp(sb_ver, "Sbox1"))
		sbox1_environment_override(environment, dir, argvp, envp, err);
	else
		sbox2_environment_override(environment, dir, argvp, envp, err);

	return TRUE;
}

static gchar*
ienvironment_get_real_directory (IAnjutaEnvironment* environment, gchar *dir, GError** err)
{
	ScratchboxPlugin *plugin = ANJUTA_PLUGIN_SCRATCHBOX (environment);

	if (plugin->user_dir)
	{
		gchar *real_dir;
		
		real_dir = g_strconcat(plugin->user_dir, dir, NULL);
		g_free (dir);
	
		return real_dir;
	}
	else
	{
		return dir;
	}
}

static void
ienvironment_iface_init(IAnjutaEnvironmentIface* iface)
{
	iface->override = ienvironment_override;
	iface->get_real_directory = ienvironment_get_real_directory;
}

/* IAnjutaPreferences implementation
 *---------------------------------------------------------------------------*/

static void
ipreferences_merge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	ScratchboxPlugin* plugin = ANJUTA_PLUGIN_SCRATCHBOX (ipref);
	GtkWidget* combo_target_entry;
	GtkWidget *combo_sbox_entry;
	GtkWidget *chooser_dir_entry;

	/* Create the preferences page */
	gxml = glade_xml_new (GLADE_FILE,
					"preferences_dialog_scratchbox", NULL);
	combo_target_entry = glade_xml_get_widget(gxml, SB_TARGET_ENTRY);
	combo_sbox_entry = glade_xml_get_widget(gxml, SB_SBOX_ENTRY);
	chooser_dir_entry = glade_xml_get_widget(gxml, SB_ENTRY);
	
	plugin->id = anjuta_preferences_get_int(prefs, SB_TARGET_ENTRY);

	anjuta_preferences_add_page (prefs, gxml, "Scratchbox", _("Scratchbox"),  ICON_FILE);
	g_signal_connect(chooser_dir_entry, "current-folder-changed",
			 G_CALLBACK(on_change_directory),
			 plugin);
	g_signal_connect(combo_sbox_entry, "changed",
			 G_CALLBACK(on_update_target), plugin);

	g_signal_connect(combo_target_entry,
			 "changed", G_CALLBACK(on_change_target),
			 plugin);

        plugin->target = gtk_combo_box_get_active_text (
				GTK_COMBO_BOX(combo_target_entry));

}

static void
ipreferences_unmerge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	GtkWidget *sb_entry;

	sb_entry = glade_xml_get_widget(gxml, SB_ENTRY);
		
	anjuta_preferences_remove_page(prefs, _("Scratchbox"));
	
	g_object_unref (gxml);
}

static void
ipreferences_iface_init(IAnjutaPreferencesIface* iface)
{
	iface->merge = ipreferences_merge;
	iface->unmerge = ipreferences_unmerge;	
}

/* GObject functions
 *---------------------------------------------------------------------------*/

/* Used in dispose and finalize */
static gpointer parent_class;

static void
scratchbox_plugin_instance_init (GObject *obj)
{
	ScratchboxPlugin *plugin = ANJUTA_PLUGIN_SCRATCHBOX (obj);
	
	plugin->user_dir = NULL;
	plugin->target_list = NULL;
	plugin->buffer = NULL;
	plugin->combo_element = 1;
	plugin->launcher = NULL;
	plugin->id = 0;
	plugin->target = NULL;
}

/* dispose is used to unref object created with instance_init */

static void
scratchbox_plugin_dispose (GObject *obj)
{
	ScratchboxPlugin *plugin = ANJUTA_PLUGIN_SCRATCHBOX (obj);
	
	/* Warning this function could be called several times */

	if (plugin->user_dir)
	{
		g_free (plugin->user_dir);
		plugin->user_dir = NULL;
	}
	
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
scratchbox_plugin_finalize (GObject *obj)
{
	/*ScratchboxPlugin *self = ANJUTA_PLUGIN_SCRATCHBOX (obj);*/
	
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

/* finalize used to free object created with instance init is not used */

static void
scratchbox_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = scratchbox_plugin_activate;
	plugin_class->deactivate = scratchbox_plugin_deactivate;
	klass->dispose = scratchbox_plugin_dispose;
	klass->finalize = scratchbox_plugin_finalize;
}

/* AnjutaPlugin declaration
 *---------------------------------------------------------------------------*/

ANJUTA_PLUGIN_BEGIN (ScratchboxPlugin, scratchbox_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE (ienvironment, IANJUTA_TYPE_ENVIRONMENT);
ANJUTA_PLUGIN_ADD_INTERFACE (ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_END;
			 
ANJUTA_SIMPLE_PLUGIN (ScratchboxPlugin, scratchbox_plugin);

/* Public functions
*---------------------------------------------------------------------------*/

