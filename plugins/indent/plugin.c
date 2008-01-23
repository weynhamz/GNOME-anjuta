/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) 2000 Naba Kumar

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
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>
#include <libanjuta/interfaces/ianjuta-document.h>
#include <libanjuta/anjuta-launcher.h>


#include "plugin.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-indent.ui"
#define PREFS_GLADE PACKAGE_DATA_DIR"/glade/indent.glade"
#define ICON_FILE "anjuta-indent-plugin.png"

static gpointer parent_class;

#define ANJUTA_PIXMAP_INDENT_AUTO         "indent_auto.xpm"
#define ANJUTA_PIXMAP_AUTOFORMAT_SETTING  "indent_set.xpm"

#define ANJUTA_STOCK_INDENT_AUTO              "anjuta-indent-auto"
#define ANJUTA_STOCK_AUTOFORMAT_SETTINGS      "anjuta-autoformat-settings"

#define AUTOFORMAT_DISABLE         "autoformat.disable"
#define AUTOFORMAT_STYLE           "autoformat.style"
#define AUTOFORMAT_LIST_STYLE      "autoformat.list.style"
#define AUTOFORMAT_OPTS            "autoformat.opts"

#define REGISTER_ICON(icon, stock_id) \
	pixbuf = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"icon, NULL); \
	icon_set = gtk_icon_set_new_from_pixbuf (pixbuf); \
	gtk_icon_factory_add (icon_factory, stock_id, icon_set); \
	g_object_unref (pixbuf);

static void
on_indent_finished (AnjutaLauncher *launcher, gint child_pid, gint status,
				   gulong time_taken, IndentPlugin *plugin)
{
	if (status)
	{
		anjuta_util_dialog_error (GTK_WINDOW(ANJUTA_PLUGIN(plugin)->shell), 
								  _("Indent command failed with error code: %d"), status);
	}
	else
	{
		gint line = ianjuta_editor_get_lineno (plugin->current_editor, NULL);
		
		ianjuta_document_begin_undo_action (IANJUTA_DOCUMENT (plugin->current_editor), NULL);
		
		/* Clear editor */
		ianjuta_editor_selection_select_all (IANJUTA_EDITOR_SELECTION (plugin->current_editor),
											 NULL);
		ianjuta_document_clear (IANJUTA_DOCUMENT (plugin->current_editor), NULL);
		
		/* Add new text */
		ianjuta_editor_append (plugin->current_editor, plugin->indent_output->str, 
							   plugin->indent_output->len, NULL);
		
		ianjuta_editor_goto_line (plugin->current_editor, line, NULL);
		
		ianjuta_document_end_undo_action (IANJUTA_DOCUMENT (plugin->current_editor), NULL);
	}
	
	g_string_free (plugin->indent_output, TRUE);
	plugin->indent_output = NULL;
}

static void
on_indent_output (AnjutaLauncher *launcher,
					   AnjutaLauncherOutputType output_type,
					   const gchar * mesg, IndentPlugin *plugin)
{
	if (output_type == ANJUTA_LAUNCHER_OUTPUT_STDOUT)
		g_string_append (plugin->indent_output, mesg);

	DEBUG_PRINT ("Indent Output: %s", mesg);
}

static void
on_indent_action_activate (GtkAction *action, IndentPlugin *plugin)
{
	gchar *cmd;
	gchar *indent_style = NULL;
	gchar *fopts = NULL;
	gchar* text;
	AnjutaPreferences* prefs =
		anjuta_shell_get_preferences (ANJUTA_PLUGIN (plugin)->shell, NULL);
	
	AnjutaLauncher* launcher =
		anjuta_launcher_new ();

	if (anjuta_util_prog_is_installed ("indent", TRUE) == FALSE)
		return;
	
	if (!anjuta_preferences_get_pair (prefs, AUTOFORMAT_STYLE,
                                 GCONF_VALUE_STRING, GCONF_VALUE_STRING,
                                 &indent_style, &fopts))
		return;
	
	if (!fopts)
	{
		gchar *msg;
		msg = g_strdup_printf (_("Anjuta does not know %s!"), indent_style);
		anjuta_util_dialog_warning (NULL, msg);
		g_free(msg);
		return;
	}
	/* Read from stdin and write to stdout */
	cmd = g_strconcat ("indent ", fopts, " -st -", NULL);
	g_free (fopts);
	
	g_signal_connect (G_OBJECT (launcher), "child-exited", G_CALLBACK (on_indent_finished),
					  plugin);
	
	plugin->indent_output = g_string_new("");
	
	if (!anjuta_launcher_execute (launcher, cmd, 
								  (AnjutaLauncherOutputCallback) on_indent_output, plugin))
	{
		DEBUG_PRINT ("Could not execute: %s", cmd);
	}
	
	text = ianjuta_editor_get_text_all (plugin->current_editor, NULL);
	anjuta_launcher_set_terminal_echo (launcher, TRUE);
	anjuta_launcher_send_stdin (launcher, text);
	anjuta_launcher_send_stdin_eof (launcher);
	g_free (text);
	g_free (cmd);
}

static void
on_edit_editor_indent (GtkWidget *button, IndentPlugin *plugin)
{
	IndentData *idt = plugin->idt;

	if (idt->dialog == NULL)
		idt->dialog = create_dialog(idt);
	gtk_widget_show(idt->dialog);
}

static GtkActionEntry actions_indent[] = {
	{
		"ActionMenuTools",	/* Action name */
		NULL,			/* Stock icon, if any */
		N_("_Tools"),		/* Display label */
		NULL,			/* Short-cut */
		NULL,			/* Tooltip */
		NULL			/* Callback */
	},
	{
		"ActionFormatAutoformat",                       /* Action name */
		ANJUTA_STOCK_INDENT_AUTO,                            /* Stock icon, if any */
		N_("_Format Code with \"indent\""),                     /* Display label */
		NULL,                                     /* short-cut */
		N_("Format code with the \"indent\" command line utility"), /* Tooltip */
		G_CALLBACK (on_indent_action_activate)    /* action callback */
	}
};

static void
on_style_combo_changed (GtkComboBox *combo, IndentPlugin *plugin)
{
	IndentData *idt = plugin->idt;

	pref_style_combo_changed(combo, idt);
}

static void
value_added_current_editor (AnjutaPlugin *plugin, const char *name,
							const GValue *value, gpointer data)
{
	GObject *editor;
	
	editor = g_value_get_object (value);
	
	if (!IANJUTA_IS_EDITOR(editor))
		return;
	
	IndentPlugin* iplugin = ANJUTA_PLUGIN_INDENT (plugin);
	iplugin->current_editor = IANJUTA_EDITOR(editor);
	
	GtkAction* action = anjuta_ui_get_action (anjuta_shell_get_ui(plugin->shell, NULL), 
						  "ActionGroupIndent",
						  "ActionFormatAutoformat");
	
	g_object_set (G_OBJECT(action), "sensitive", TRUE, NULL);
}

static void
value_removed_current_editor (AnjutaPlugin *plugin,
							  const char *name, gpointer data)
{
	IndentPlugin* iplugin = ANJUTA_PLUGIN_INDENT (plugin);
	
	GtkAction* action = anjuta_ui_get_action (anjuta_shell_get_ui(plugin->shell, NULL), 
						  "ActionGroupIndent",
						  "ActionFormatAutoformat");
	
	g_object_set (G_OBJECT(action), "sensitive", FALSE, NULL);
	
	iplugin->current_editor = NULL;
}


static gboolean
indent_plugin_activate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	IndentPlugin *indent_plugin;
	static gboolean init = FALSE;
	AnjutaPreferences* prefs;
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	if (!init)
	{
		GtkIconFactory *icon_factory = anjuta_ui_get_icon_factory (ui);
		GtkIconSet *icon_set;
		GdkPixbuf *pixbuf;
		
		REGISTER_ICON (ANJUTA_PIXMAP_INDENT_AUTO, ANJUTA_STOCK_INDENT_AUTO);
		REGISTER_ICON (ANJUTA_PIXMAP_AUTOFORMAT_SETTING, ANJUTA_STOCK_AUTOFORMAT_SETTINGS);
		init = TRUE;
	}
	
	DEBUG_PRINT ("IndentPlugin: Activating Indent plugin ...");
	indent_plugin = ANJUTA_PLUGIN_INDENT (plugin);
	
	/* Add all our editor actions */
	anjuta_ui_add_action_group_entries (ui, "ActionGroupIndent",
										_("Autoformat operations"),
										actions_indent,
										G_N_ELEMENTS (actions_indent),
										GETTEXT_PACKAGE, TRUE, plugin);
	indent_plugin->uiid = anjuta_ui_merge (ui, UI_FILE);
	prefs = anjuta_shell_get_preferences (plugin->shell, NULL);
	indent_plugin->idt = indent_init(prefs);
	
	indent_plugin->editor_watch_id = 
		anjuta_plugin_add_watch (plugin, "document_manager_current_editor",
								 value_added_current_editor,
								 value_removed_current_editor, NULL);
	indent_plugin->current_editor = NULL;
	
	return TRUE;
}

static gboolean
indent_plugin_deactivate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui = anjuta_shell_get_ui (plugin->shell, NULL);
	DEBUG_PRINT ("IndentPlugin: Dectivating Indent plugin ...");
	anjuta_ui_unmerge (ui, ANJUTA_PLUGIN_INDENT (plugin)->uiid);
	anjuta_plugin_remove_watch (plugin, ANJUTA_PLUGIN_INDENT(plugin)->editor_watch_id, TRUE);
	return TRUE;
}

static void
indent_plugin_finalize (GObject *obj)
{
	/* Finalization codes here */
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

static void
indent_plugin_dispose (GObject *obj)
{
	/* Disposition codes */
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
anjuta_indent_plugin_instance_init (GObject *obj)
{
	IndentPlugin *plugin = ANJUTA_PLUGIN_INDENT (obj);
	plugin->uiid = 0;
}

static void
anjuta_indent_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = indent_plugin_activate_plugin;
	plugin_class->deactivate = indent_plugin_deactivate_plugin;
	klass->finalize = indent_plugin_finalize;
	klass->dispose = indent_plugin_dispose;
}

static void
ipreferences_merge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	GtkWidget *indent_button;
	GtkWidget *indent_combo;
	GtkWidget *indent_entry;
	GladeXML* gxml;
		
	AnjutaPlugin* plugin = ANJUTA_PLUGIN (ipref);
	IndentPlugin* iplugin = ANJUTA_PLUGIN_INDENT (plugin);
	
	/* Add preferences */
	gxml = glade_xml_new (PREFS_GLADE, "indent_prefs_window", NULL);
	indent_button = glade_xml_get_widget (gxml, "set_indent_button");
	g_signal_connect (G_OBJECT (indent_button), "clicked",
						  G_CALLBACK (on_edit_editor_indent), plugin);
		
	anjuta_preferences_add_page (prefs,
									gxml, "indent_prefs", _("Indent Utility"),  ICON_FILE);
		
	indent_combo = glade_xml_get_widget (gxml, "pref_indent_style_combobox");
	iplugin->idt->pref_indent_combo = indent_combo;
	g_signal_connect (G_OBJECT (indent_combo), "changed",
						  G_CALLBACK (on_style_combo_changed), plugin);
	
	indent_entry = glade_xml_get_widget (gxml, "indent_style_entry");
	iplugin->idt->pref_indent_options = indent_entry;
	iplugin->idt->prefs = prefs;
	indent_init_load_style (iplugin->idt);
	
	pref_set_style_combo (iplugin->idt);
	g_object_unref (G_OBJECT (gxml));
}

static void
ipreferences_unmerge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	anjuta_preferences_remove_page (prefs, _("Indent utility"));
}

static void
ipreferences_iface_init(IAnjutaPreferencesIface* iface)
{
	iface->merge = ipreferences_merge;
	iface->unmerge = ipreferences_unmerge;	
}

ANJUTA_PLUGIN_BEGIN (IndentPlugin, anjuta_indent_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE (ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (IndentPlugin, anjuta_indent_plugin);
