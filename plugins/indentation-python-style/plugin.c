/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * plugin.c
 * Copyright (C) Ishan Chattopadhyaya 2009 <ichattopadhyaya@gmail.com>
 *
 * plugin.c is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * plugin.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with plugin.c.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include <config.h>
#include <ctype.h>
#include <stdlib.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-iterable.h>
#include <libanjuta/interfaces/ianjuta-document.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-editor-cell.h>
#include <libanjuta/interfaces/ianjuta-editor-language.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>
#include <libanjuta/interfaces/ianjuta-language.h>
#include <libanjuta/interfaces/ianjuta-indenter.h>

#include "plugin.h"
#include "python-indentation.h"

/* Pixmaps */
#define ANJUTA_STOCK_AUTOINDENT           "anjuta-indent"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-indentation-python-style.xml"
#define PROPERTIES_FILE_UI PACKAGE_DATA_DIR"/glade/anjuta-indentation-python-style.ui"
#define ICON_FILE "anjuta-indentation-python-style-plugin.png"

/* Preferences keys */
#define ANJUTA_PREF_SCHEMA_PREFIX "org.gnome.anjuta."
#define PREF_SCHEMA "org.gnome.anjuta.plugins.indent-python"

static gpointer parent_class;

static void
on_editor_char_inserted_python (IAnjutaEditor *editor,
                                IAnjutaIterable *insert_pos,
                                gchar ch,
                                IndentPythonPlugin *plugin)
{
	python_indent (plugin, editor, insert_pos, ch);
}

static void
install_support (IndentPythonPlugin *lang_plugin)
{
	IAnjutaLanguage* lang_manager =
		anjuta_shell_get_interface (ANJUTA_PLUGIN (lang_plugin)->shell,
									IAnjutaLanguage, NULL);

	if (!lang_manager)
		return;

	if (lang_plugin->support_installed)
		return;

	lang_plugin->current_language =
		ianjuta_language_get_name_from_editor (lang_manager,
											   IANJUTA_EDITOR_LANGUAGE (lang_plugin->current_editor), NULL);

	if (lang_plugin->current_language &&
		(g_str_equal (lang_plugin->current_language, "Python")))
	{
		g_signal_connect (lang_plugin->current_editor,
							"char-added",
							G_CALLBACK (on_editor_char_inserted_python),
							lang_plugin);
	}
	else
	{
		return;
	}

	python_indent_init (lang_plugin);
	/* Disable editor intern auto-indent */
	ianjuta_editor_set_auto_indent (IANJUTA_EDITOR(lang_plugin->current_editor),
								    FALSE, NULL);

	lang_plugin->support_installed = TRUE;
}

static void
uninstall_support (IndentPythonPlugin *lang_plugin)
{
	if (!lang_plugin->support_installed)
		return;

	if (lang_plugin->current_language &&
		(g_str_equal (lang_plugin->current_language, "Python")))
	{
		g_signal_handlers_disconnect_by_func (lang_plugin->current_editor,
									G_CALLBACK (on_editor_char_inserted_python),
									lang_plugin);
	}

	lang_plugin->support_installed = FALSE;
}

static void
on_editor_language_changed (IAnjutaEditor *editor,
							const gchar *new_language,
							IndentPythonPlugin *plugin)
{
	uninstall_support (plugin);
	install_support (plugin);
}

static void
on_editor_added (AnjutaPlugin *plugin, const gchar *name,
                 const GValue *value, gpointer data)
{
	IndentPythonPlugin *lang_plugin;
	IAnjutaDocument* doc = IANJUTA_DOCUMENT(g_value_get_object (value));
	lang_plugin = ANJUTA_PLUGIN_INDENT_PYTHON(plugin);


	if (IANJUTA_IS_EDITOR(doc))
	{
		lang_plugin->current_editor = G_OBJECT(doc);
	}
	else
	{
		lang_plugin->current_editor = NULL;
		return;
	}
	if (lang_plugin->current_editor)
	{
		IAnjutaEditor* editor = IANJUTA_EDITOR (lang_plugin->current_editor);
		GFile* current_editor_file = ianjuta_file_get_file (IANJUTA_FILE (editor),
		                                                    NULL);

		if (current_editor_file)
		{
			lang_plugin->current_editor_filename = g_file_get_path (current_editor_file);
			g_object_unref (current_editor_file);
		}

		install_support (lang_plugin);
		g_signal_connect (lang_plugin->current_editor, "language-changed",
		                  G_CALLBACK (on_editor_language_changed),
		                  plugin);
	}
}

static void
on_editor_removed (AnjutaPlugin *plugin, const gchar *name,
                 gpointer data)
{
	IndentPythonPlugin *lang_plugin;
	lang_plugin = ANJUTA_PLUGIN_INDENT_PYTHON (plugin);

	if (lang_plugin->current_editor)
		g_signal_handlers_disconnect_by_func (lang_plugin->current_editor,
										  G_CALLBACK (on_editor_language_changed),
										  plugin);

	uninstall_support (lang_plugin);


	g_free (lang_plugin->current_editor_filename);
	lang_plugin->current_editor_filename = NULL;
	lang_plugin->current_editor = NULL;
	lang_plugin->current_language = NULL;
}

static void
on_auto_indent (GtkAction *action, gpointer data)
{
	IndentPythonPlugin *lang_plugin = ANJUTA_PLUGIN_INDENT_PYTHON (data);

	python_indent_auto (lang_plugin, NULL, NULL);
}

static GtkActionEntry actions[] = {
	{
		"ActionMenuEdit",
		NULL, N_("_Edit"),
		NULL, NULL, NULL
	},
	{
		"ActionEditAutoindent",
		GTK_STOCK_NEW, //ANJUTA_STOCK_AUTOINDENT,
		N_("Auto-Indent"), "<control>i",
		N_("Auto-indent current line or selection based on indentation settings"),
		G_CALLBACK (on_auto_indent)
	}
};

static gboolean
indent_python_plugin_activate (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;

	IndentPythonPlugin *python_plugin;
	python_plugin = (IndentPythonPlugin*) plugin;

	python_plugin->prefs = anjuta_shell_get_preferences (plugin->shell, NULL);

	/* Add all UI actions and merge UI */
	ui = anjuta_shell_get_ui (plugin->shell, NULL);

	python_plugin->action_group =
		anjuta_ui_add_action_group_entries (ui, "ActionGroupPythonIndent",
											_("Python Indentation"),
											actions,
											G_N_ELEMENTS (actions),
											GETTEXT_PACKAGE, TRUE,
											plugin);
	python_plugin->uiid = anjuta_ui_merge (ui, UI_FILE);

	/* Add watches */	
	python_plugin->editor_watch_id = anjuta_plugin_add_watch (plugin,
														IANJUTA_DOCUMENT_MANAGER_CURRENT_DOCUMENT,
														on_editor_added,
														on_editor_removed,
														NULL);
	return TRUE;
}

static gboolean
indent_python_plugin_deactivate (AnjutaPlugin *plugin)
{

	AnjutaUI *ui;
	IndentPythonPlugin *lang_plugin;
	lang_plugin = (IndentPythonPlugin*) (plugin);

	anjuta_plugin_remove_watch (plugin,
								lang_plugin->editor_watch_id,
								TRUE);

	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	anjuta_ui_remove_action_group (ui, ANJUTA_PLUGIN_INDENT_PYTHON(plugin)->action_group);
	anjuta_ui_unmerge (ui, ANJUTA_PLUGIN_INDENT_PYTHON(plugin)->uiid);

	return TRUE;
}

static void
indent_python_plugin_finalize (GObject *obj)
{
	/* Finalization codes here */
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
indent_python_plugin_dispose (GObject *obj)
{
	/* Disposition codes */
	IndentPythonPlugin *plugin = (IndentPythonPlugin*)obj;

	if (plugin->settings)
		g_object_unref (plugin->settings);
	plugin->settings = NULL;
	if (plugin->editor_settings)
		g_object_unref (plugin->editor_settings);
	plugin->editor_settings = NULL;

	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
indent_python_plugin_instance_init (GObject *obj)
{
	IndentPythonPlugin *plugin = (IndentPythonPlugin*)obj;
	plugin->action_group = NULL;
	plugin->current_editor = NULL;
	plugin->current_language = NULL;
	plugin->editor_watch_id = 0;
	plugin->uiid = 0;
	plugin->settings = g_settings_new (PREF_SCHEMA);
	plugin->editor_settings = g_settings_new (ANJUTA_PREF_SCHEMA_PREFIX IANJUTA_EDITOR_PREF_SCHEMA);
}

static void
indent_python_plugin_class_init (GObjectClass *klass)
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = indent_python_plugin_activate;
	plugin_class->deactivate = indent_python_plugin_deactivate;
	klass->finalize = indent_python_plugin_finalize;
	klass->dispose = indent_python_plugin_dispose;
}


static void
ipreferences_merge (IAnjutaPreferences* ipref, AnjutaPreferences* prefs,
					GError** e)
{
	GError* error = NULL;
	IndentPythonPlugin* plugin = ANJUTA_PLUGIN_INDENT_PYTHON (ipref);
	plugin->bxml = gtk_builder_new ();
	
	/* Add preferences */
    if (!gtk_builder_add_from_file (plugin->bxml, PROPERTIES_FILE_UI, &error))
    {
        g_warning ("Couldn't load builder file: %s", error->message);
        g_error_free (error);
    }
	anjuta_preferences_add_from_builder (prefs,
	                                     plugin->bxml, plugin->settings,
	                                     "preferences", _("Indentation"),
	                                     ICON_FILE);
}

static void
ipreferences_unmerge (IAnjutaPreferences* ipref, AnjutaPreferences* prefs,
					  GError** e)
{
	IndentPythonPlugin* plugin = ANJUTA_PLUGIN_INDENT_PYTHON (ipref);
	anjuta_preferences_remove_page(prefs, _("Indentation"));
	g_object_unref (plugin->bxml);
}

static void
ipreferences_iface_init (IAnjutaPreferencesIface* iface)
{
	iface->merge = ipreferences_merge;
	iface->unmerge = ipreferences_unmerge;
}

static void
iindenter_indent (IAnjutaIndenter* indenter,
                  IAnjutaIterable* start,
                  IAnjutaIterable* end,
                  GError** e)
{
	IndentPythonPlugin* plugin = ANJUTA_PLUGIN_INDENT_PYTHON (indenter);

	python_indent_auto (plugin,
	                    start, end);
}

static void
iindenter_iface_init (IAnjutaIndenterIface* iface)
{
	iface->indent = iindenter_indent;
}

ANJUTA_PLUGIN_BEGIN (IndentPythonPlugin, indent_python_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE(ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_ADD_INTERFACE(iindenter, IANJUTA_TYPE_INDENTER);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (IndentPythonPlugin, indent_python_plugin);
