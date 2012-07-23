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
#include <libanjuta/anjuta-launcher.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/interfaces/ianjuta-iterable.h>
#include <libanjuta/interfaces/ianjuta-document.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-editor-cell.h>
#include <libanjuta/interfaces/ianjuta-editor-language.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>
#include <libanjuta/interfaces/ianjuta-editor-assist.h>
#include <libanjuta/interfaces/ianjuta-editor-glade-signal.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>
#include <libanjuta/interfaces/ianjuta-symbol.h>
#include <libanjuta/interfaces/ianjuta-language.h>
#include <libanjuta/interfaces/ianjuta-indenter.h>

#include "plugin.h"
#include "python-assist.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-language-support-python.xml"
#define PROPERTIES_FILE_UI PACKAGE_DATA_DIR"/glade/anjuta-language-support-python.ui"
#define ICON_FILE "anjuta-language-support-python-plugin.png"

/* Preferences keys */

#define ANJUTA_PREF_SCHEMA_PREFIX "org.gnome.anjuta."
#define PREF_SCHEMA "org.gnome.anjuta.plugins.python"


#define PREF_NO_ROPE_WARNING "no-rope-warning"
#define PREF_INTERPRETER_PATH "interpreter-path"

static gpointer parent_class;

static void
on_check_finished (AnjutaLauncher* launcher,
                   int child_pid, int exit_status,
                   gulong time, gpointer user_data)
{
	PythonPlugin* plugin = ANJUTA_PLUGIN_PYTHON (user_data);
	if (exit_status != 0)
	{
		GtkWidget* dialog = gtk_dialog_new_with_buttons (_("Python support warning"),
		                                                 NULL,
		                                                 GTK_DIALOG_MODAL,
		                                                 GTK_STOCK_OK,
		                                                 GTK_RESPONSE_ACCEPT,
		                                                 NULL);
		GtkWidget* area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

		GtkWidget* label = gtk_label_new (_("Either python path is wrong or python-rope (http://rope.sf.net) libraries\n"
		                                    "aren't installed. Both are required for autocompletion in python files.\n"
		                                    "Please install them and check the python path in the preferences."));
		GtkWidget* check_button = gtk_check_button_new_with_label (_("Do not show that warning again"));

		gtk_box_pack_start (GTK_BOX (area), label,
		                    TRUE, TRUE, 5);
		gtk_box_pack_start (GTK_BOX (area), check_button,
		                    TRUE, TRUE, 5);
		gtk_widget_show_all (dialog);

		gtk_dialog_run (GTK_DIALOG(dialog));

		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_button)))
		{
			g_settings_get_boolean (plugin->settings,
			                        PREF_NO_ROPE_WARNING);
		}
		gtk_widget_destroy (dialog);
	}
	g_object_unref (launcher);
}

static void
check_support (PythonPlugin *python_plugin)
{
	if (!g_settings_get_boolean (python_plugin->settings,
	                             PREF_NO_ROPE_WARNING))
	{
		AnjutaLauncher* launcher = anjuta_launcher_new ();
		gchar* python_path = g_settings_get_string (python_plugin->settings,
		                                            PREF_INTERPRETER_PATH);
		gchar* command = g_strdup_printf ("%s -c \"import rope\"", python_path);

		g_signal_connect (launcher, "child-exited",
	                  G_CALLBACK(on_check_finished), python_plugin);
		anjuta_launcher_execute (launcher, command, NULL, NULL);

		g_free (python_path);
		g_free (command);
	}
}

/* Glade support */
static gchar*
language_support_check_param_name (const gchar* name,
                                   GList** names)
{
	gint index = 0;
	GString* real_name = g_string_new (name);
	while (g_list_find_custom (*names, real_name->str, (GCompareFunc) strcmp))
	{
		g_string_free (real_name, TRUE);
		real_name = g_string_new (name);
		g_string_append_printf (real_name, "%d", ++index);
	}
	*names = g_list_append (*names, real_name->str);
	return g_string_free (real_name, FALSE);
}

static const gchar*
language_support_get_signal_parameter (const gchar* type_name, GList** names)
{
	const gchar* c;
	const gchar* param_name = NULL;
	GString* param_string;
	gchar* real_name;
	/* Search for the second upper character */
	for (c = type_name + 1; *c != '\0'; c++)
	{
		if (g_ascii_isupper (*c))
		{
			param_name = c;
			break;
		}
	}
	if (param_name && strlen (param_name))
	{
		param_string = g_string_new (param_name);
		g_string_down (param_string);
	}
	else
	{
		param_string = g_string_new ("arg");
	}
	real_name = language_support_check_param_name (g_string_free (param_string, FALSE), names);

	return real_name;
}

static void
on_glade_drop (IAnjutaEditor* editor,
               IAnjutaIterable* iterator,
               const gchar* signal_data,
               PythonPlugin* lang_plugin)
{
	GSignalQuery query;
	GType type;
	guint id;

	const gchar* widget;
	const gchar* signal;
	const gchar* handler;
	GList* names = NULL;
	GString* str = g_string_new (NULL);
	int i;
	IAnjutaIterable* start, * end;

	GStrv data = g_strsplit(signal_data, ":", 5);

	widget = data[0];
	signal = data[1];
	handler = data[2];

	type = g_type_from_name (widget);
	id = g_signal_lookup (signal, type);

	g_signal_query (id, &query);

	g_string_append_printf (str, "\ndef %s (self, %s", handler,
	                        language_support_get_signal_parameter (widget,
	                                                               &names));
	for (i = 0; i < query.n_params; i++)
	{
				const gchar* type_name = g_type_name (query.param_types[i]);
		const gchar* param_name = language_support_get_signal_parameter (type_name,
		                                                                 &names);

		g_string_append_printf (str, ", %s", param_name);
	}
	g_string_append (str, "):\n");

	ianjuta_editor_insert (editor, iterator,
	                       str->str, -1, NULL);

	/* Indent code correctly */
	start = iterator;
	end = ianjuta_iterable_clone (iterator, NULL);
	ianjuta_iterable_set_position (end,
	                               ianjuta_iterable_get_position (iterator, NULL)
	                           		+ g_utf8_strlen (str->str, -1),
	                               NULL);
	ianjuta_indenter_indent (IANJUTA_INDENTER (lang_plugin),
	                         start, end, NULL);
	g_object_unref (end);

	g_string_free (str, TRUE);
	anjuta_util_glist_strings_free (names);

	g_strfreev (data);
}

static void
install_support (PythonPlugin *lang_plugin)
{
	IAnjutaLanguage* lang_manager =
		anjuta_shell_get_interface (ANJUTA_PLUGIN (lang_plugin)->shell,
									IAnjutaLanguage, NULL);
	IAnjutaSymbolManager* sym_manager =
		anjuta_shell_get_interface (ANJUTA_PLUGIN (lang_plugin)->shell,
		                            IAnjutaSymbolManager,
		                            NULL);

	if (!lang_manager || !sym_manager)
		return;

	if (lang_plugin->support_installed)
		return;

	lang_plugin->current_language =
		ianjuta_language_get_name_from_editor (lang_manager,
											   IANJUTA_EDITOR_LANGUAGE (lang_plugin->current_editor), NULL);

	if (!(lang_plugin->current_language &&
		(g_str_equal (lang_plugin->current_language, "Python"))))
		return;

	/* Disable editor intern auto-indent */
	ianjuta_editor_set_auto_indent (IANJUTA_EDITOR(lang_plugin->current_editor),
								    FALSE, NULL);

	if (IANJUTA_IS_EDITOR_ASSIST (lang_plugin->current_editor) )
	{
		AnjutaPlugin *plugin;
		IAnjutaEditor* ieditor;

		const gchar *project_root;

		check_support (lang_plugin);

		plugin = ANJUTA_PLUGIN (lang_plugin);
		ieditor = IANJUTA_EDITOR (lang_plugin->current_editor);

		g_assert (lang_plugin->assist == NULL);

		project_root = ANJUTA_PLUGIN_PYTHON(plugin)->project_root_directory;

		lang_plugin->assist = python_assist_new (ieditor,
		                                         sym_manager,
		                                         lang_plugin->settings,
		                                         plugin,
		                                         project_root);
	}

	if (IANJUTA_IS_EDITOR_GLADE_SIGNAL (lang_plugin->current_editor))
	{
		g_signal_connect (lang_plugin->current_editor,
		                  "drop-possible", G_CALLBACK (gtk_true), NULL);
		g_signal_connect (lang_plugin->current_editor,
		                  "drop", G_CALLBACK (on_glade_drop),
		                  lang_plugin);
	}

	lang_plugin->support_installed = TRUE;
}

static void
uninstall_support (PythonPlugin *lang_plugin)
{
	if (!lang_plugin->support_installed)
		return;

	if (lang_plugin->assist)
	{
		g_object_unref (lang_plugin->assist);
		lang_plugin->assist = NULL;
	}

	if (IANJUTA_IS_EDITOR_GLADE_SIGNAL (lang_plugin->current_editor))
	{
		g_signal_handlers_disconnect_by_func (lang_plugin->current_editor,
			                                  gtk_true, NULL);
		g_signal_handlers_disconnect_by_func (lang_plugin->current_editor,
			                                  on_glade_drop, lang_plugin);
	}

	lang_plugin->support_installed = FALSE;
}

static void
on_editor_language_changed (IAnjutaEditor *editor,
							const gchar *new_language,
							PythonPlugin *plugin)
{
	uninstall_support (plugin);
	install_support (plugin);
}

static void
on_editor_added (AnjutaPlugin *plugin, const gchar *name,
                 const GValue *value, gpointer data)
{
	PythonPlugin *lang_plugin;
	IAnjutaDocument* doc = IANJUTA_DOCUMENT(g_value_get_object (value));
	lang_plugin = ANJUTA_PLUGIN_PYTHON(plugin);


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
	PythonPlugin *lang_plugin;
	lang_plugin = ANJUTA_PLUGIN_PYTHON (plugin);

	if (lang_plugin->current_editor)
		g_signal_handlers_disconnect_by_func (lang_plugin->current_editor,
										  G_CALLBACK (on_editor_language_changed),
										  plugin);

	uninstall_support (lang_plugin);

	lang_plugin->current_editor = NULL;
	lang_plugin->current_language = NULL;
}

static GtkActionEntry actions[] = {
	{
		"ActionMenuEdit",
		NULL, N_("_Edit"),
		NULL, NULL, NULL
	}
};

static void
on_project_root_added (AnjutaPlugin *plugin, const gchar *name,
					   const GValue *value, gpointer user_data)
{
	PythonPlugin *python_plugin;
	gchar *project_root_uri;
	GFile *file;

	python_plugin = ANJUTA_PLUGIN_PYTHON (plugin);

	g_free (python_plugin->project_root_directory);
	project_root_uri = g_value_dup_string (value);
	file = g_file_new_for_uri (project_root_uri);
	python_plugin->project_root_directory = g_file_get_path (file);
	g_object_unref (file);
	g_free (project_root_uri);
}

static void
on_project_root_removed (AnjutaPlugin *plugin, const gchar *name,
						 gpointer user_data)
{
	PythonPlugin *python_plugin;

	python_plugin = ANJUTA_PLUGIN_PYTHON (plugin);

	g_free (python_plugin->project_root_directory);
	python_plugin->project_root_directory = NULL;
}

static gboolean
python_plugin_activate (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;

	PythonPlugin *python_plugin;

	python_plugin = (PythonPlugin*) plugin;

	python_plugin->prefs = anjuta_shell_get_preferences (plugin->shell, NULL);

	/* Add all UI actions and merge UI */
	ui = anjuta_shell_get_ui (plugin->shell, NULL);

	python_plugin->action_group =
		anjuta_ui_add_action_group_entries (ui, "ActionGroupPythonAssist",
											_("Python Assistance"),
											actions,
											G_N_ELEMENTS (actions),
											GETTEXT_PACKAGE, TRUE,
											plugin);
	python_plugin->uiid = anjuta_ui_merge (ui, UI_FILE);

	/* Add watches */
	python_plugin->project_root_watch_id = anjuta_plugin_add_watch (plugin,
									IANJUTA_PROJECT_MANAGER_PROJECT_ROOT_URI,
									on_project_root_added,
									on_project_root_removed,
									NULL);

	python_plugin->editor_watch_id = anjuta_plugin_add_watch (plugin,
									IANJUTA_DOCUMENT_MANAGER_CURRENT_DOCUMENT,
									on_editor_added,
									on_editor_removed,
									NULL);
	return TRUE;
}

static gboolean
python_plugin_deactivate (AnjutaPlugin *plugin)
{

	AnjutaUI *ui;
	PythonPlugin *lang_plugin;
	lang_plugin = (PythonPlugin*) (plugin);

	anjuta_plugin_remove_watch (plugin,
								lang_plugin->editor_watch_id,
								TRUE);
	anjuta_plugin_remove_watch (plugin,
								lang_plugin->project_root_watch_id,
								TRUE);


	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	anjuta_ui_remove_action_group (ui, ANJUTA_PLUGIN_PYTHON(plugin)->action_group);
	anjuta_ui_unmerge (ui, ANJUTA_PLUGIN_PYTHON(plugin)->uiid);

	return TRUE;
}

static void
python_plugin_finalize (GObject *obj)
{
	/* Finalization codes here */
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
python_plugin_dispose (GObject *obj)
{
	/* Disposition codes */
	PythonPlugin *plugin = (PythonPlugin*)obj;

	if (plugin->settings)
		g_object_unref (plugin->settings);
	plugin->settings = NULL;
	if (plugin->editor_settings)
		g_object_unref (plugin->editor_settings);
	plugin->editor_settings = NULL;

	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
python_plugin_instance_init (GObject *obj)
{
	PythonPlugin *plugin = (PythonPlugin*)obj;
	plugin->action_group = NULL;
	plugin->current_editor = NULL;
	plugin->current_language = NULL;
	plugin->editor_watch_id = 0;
	plugin->uiid = 0;
	plugin->assist = NULL;
	plugin->settings = g_settings_new (PREF_SCHEMA);
	plugin->editor_settings = g_settings_new (ANJUTA_PREF_SCHEMA_PREFIX IANJUTA_EDITOR_PREF_SCHEMA);
}

static void
python_plugin_class_init (GObjectClass *klass)
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = python_plugin_activate;
	plugin_class->deactivate = python_plugin_deactivate;
	klass->finalize = python_plugin_finalize;
	klass->dispose = python_plugin_dispose;
}

#define PREF_WIDGET_SPACE "preferences:completion-space-after-func"
#define PREF_WIDGET_BRACE "preferences:completion-brace-after-func"
#define PREF_WIDGET_CLOSEBRACE "preferences:completion-closebrace-after-func"
#define PREF_WIDGET_AUTO "preferences:completion-enable"

static void
on_autocompletion_toggled (GtkToggleButton* button,
                           PythonPlugin* plugin)
{
    GtkWidget* widget;
    gboolean sensitive = gtk_toggle_button_get_active (button);

    widget = GTK_WIDGET (gtk_builder_get_object (plugin->bxml, PREF_WIDGET_SPACE));
    gtk_widget_set_sensitive (widget, sensitive);
    widget = GTK_WIDGET (gtk_builder_get_object (plugin->bxml, PREF_WIDGET_BRACE));
    gtk_widget_set_sensitive (widget, sensitive);
    widget = GTK_WIDGET (gtk_builder_get_object (plugin->bxml, PREF_WIDGET_CLOSEBRACE));
    gtk_widget_set_sensitive (widget, sensitive);
}

static void
ipreferences_merge (IAnjutaPreferences* ipref, AnjutaPreferences* prefs,
					GError** e)
{
	/* Add preferences */
	GError* error = NULL;
	PythonPlugin* plugin = ANJUTA_PLUGIN_PYTHON (ipref);
	plugin->bxml = gtk_builder_new ();
    GtkWidget* toggle;
    
	if (!gtk_builder_add_from_file (plugin->bxml, PROPERTIES_FILE_UI, &error))
	{
        g_warning ("Couldn't load builder file: %s", error->message);
        g_error_free (error);
	}
	anjuta_preferences_add_from_builder (prefs,
	                                     plugin->bxml,
	                                     plugin->settings,
	                                     "preferences", _("Python"),
	                                     ICON_FILE);
    toggle = GTK_WIDGET (gtk_builder_get_object (plugin->bxml, PREF_WIDGET_AUTO));
    g_signal_connect (toggle, "toggled", G_CALLBACK (on_autocompletion_toggled),
                      plugin);
    on_autocompletion_toggled (GTK_TOGGLE_BUTTON (toggle), plugin);
}

static void
ipreferences_unmerge (IAnjutaPreferences* ipref, AnjutaPreferences* prefs,
					  GError** e)
{
	PythonPlugin* plugin = ANJUTA_PLUGIN_PYTHON (ipref);
	anjuta_preferences_remove_page(prefs, _("Python"));
	g_object_unref (plugin->bxml);
}

static void
ipreferences_iface_init (IAnjutaPreferencesIface* iface)
{
	iface->merge = ipreferences_merge;
	iface->unmerge = ipreferences_unmerge;
}

ANJUTA_PLUGIN_BEGIN (PythonPlugin, python_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE(ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (PythonPlugin, python_plugin);
