/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * plugin.c
 * Copyright (C) Johannes Schmid 2005 <jhs@gnome.org>
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
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-status.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-editor-factory.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>
#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourcestyleschememanager.h>

#include "plugin.h"
#include "sourceview.h"
#include "sourceview-prefs.h"
#include "sourceview-private.h"

#define PREF_SCHEMA "org.gnome.anjuta.plugins.sourceview"
#define PREFS_GLADE PACKAGE_DATA_DIR"/glade/anjuta-editor-sourceview.ui"
#define ICON_FILE "anjuta-editor-sourceview-plugin-48.png"
#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-sourceview.xml"

#define COMBO_STYLES "combo_styles"
#define SOURCEVIEW_STYLE "style"

#define FONT_USE_THEME_BUTTON "preferences_toggle:bool:1:0:font-use-theme"
#define FONT_BUTTON "preferences_font:font:Monospace 12:0:font"

static gpointer parent_class;

static GtkBuilder* builder = NULL;

static void
on_editor_linenos_activate (GtkAction *action, gpointer user_data)
{
	gboolean state;
	SourceviewPlugin* plugin = ANJUTA_PLUGIN_SOURCEVIEW (user_data);

	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	g_settings_set_boolean (plugin->settings,
	                        VIEW_LINENUMBERS, state);
}

static void
on_editor_markers_activate (GtkAction *action, gpointer user_data)
{
	gboolean state;
	SourceviewPlugin* plugin = ANJUTA_PLUGIN_SOURCEVIEW (user_data);

	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	g_settings_set_boolean (plugin->settings,
	                        VIEW_MARKS, state);
}

static void
on_editor_whitespaces_activate (GtkAction *action, gpointer user_data)
{
	gboolean state;
	SourceviewPlugin* plugin = ANJUTA_PLUGIN_SOURCEVIEW (user_data);

	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	g_settings_set_boolean (plugin->settings,
	                        VIEW_WHITE_SPACES, state);
}

static void
on_editor_eolchars_activate (GtkAction *action, gpointer user_data)
{
	gboolean state;
	SourceviewPlugin* plugin = ANJUTA_PLUGIN_SOURCEVIEW (user_data);

	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	g_settings_set_boolean (plugin->settings,
	                        VIEW_EOL, state);
}

static void
on_editor_linewrap_activate (GtkAction *action, gpointer user_data)
{
	gboolean state;
	SourceviewPlugin* plugin = ANJUTA_PLUGIN_SOURCEVIEW (user_data);

	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	g_settings_set_boolean (plugin->settings,
	                        VIEW_LINE_WRAP, state);
}

static GtkToggleActionEntry actions_view[] = {
  { "ActionViewEditorLinenumbers", NULL, N_("_Line Number Margin"), NULL,
	N_("Show/Hide line numbers"),
    G_CALLBACK (on_editor_linenos_activate), FALSE},
  { "ActionViewEditorMarkers", NULL, N_("_Marker Margin"), NULL,
	N_("Show/Hide marker margin"),
    G_CALLBACK (on_editor_markers_activate), FALSE},
  { "ActionViewEditorSpaces", NULL, N_("_White Space"), NULL,
	N_("Show/Hide white spaces"),
    G_CALLBACK (on_editor_whitespaces_activate), FALSE},
  { "ActionViewEditorEOL", NULL, N_("_Line End Characters"), NULL,
	N_("Show/Hide line end characters"),
    G_CALLBACK (on_editor_eolchars_activate), FALSE},
  { "ActionViewEditorWrapping", NULL, N_("Line _Wrapping"), NULL,
	N_("Enable/disable line wrapping"),
    G_CALLBACK (on_editor_linewrap_activate), FALSE}
};

static void
ui_states_init (SourceviewPlugin* plugin, AnjutaUI *ui)
{
	static const gchar *prefs[] = {
		VIEW_LINENUMBERS,
		VIEW_MARKS,
		VIEW_WHITE_SPACES,
		VIEW_EOL,
		VIEW_LINE_WRAP
	};
	gint i;

	for (i = 0; i < sizeof (prefs)/sizeof(const gchar *); i++)
	{
		GtkAction *action;
		gboolean state;

		state = g_settings_get_boolean (plugin->settings, prefs[i]);
		action = anjuta_ui_get_action (ui, "ActionGroupEditorView",
		                               actions_view[i].name);
		g_object_set (G_OBJECT (action), "sensitive", TRUE, "visible", TRUE, NULL);
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), state);
	}
}


static void
on_font_check_toggled(GtkToggleButton* button, GtkBuilder* builder)
{
	GtkWidget* font_button;
	font_button = GTK_WIDGET (gtk_builder_get_object (builder, FONT_BUTTON));
	gtk_widget_set_sensitive(font_button, !gtk_toggle_button_get_active(button));
}

static gboolean
sourceview_plugin_activate (AnjutaPlugin *obj)
{
	SourceviewPlugin* plugin = ANJUTA_PLUGIN_SOURCEVIEW (obj);
	AnjutaUI *ui;

	DEBUG_PRINT ("%s", "SourceviewPlugin: Activating SourceviewPlugin plugin ...");

	/* Add menu entries */
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (plugin)->shell, NULL);
	plugin->group = anjuta_ui_add_toggle_action_group_entries (ui, "ActionGroupEditorView",
	                                                           _("Editor view settings"),
	                                                           actions_view,
	                                                           G_N_ELEMENTS (actions_view),
	                                                           GETTEXT_PACKAGE, TRUE, plugin);
	ui_states_init (plugin, ui);
	plugin->uiid = anjuta_ui_merge (ui, UI_FILE);

	return TRUE;
}

static gboolean
sourceview_plugin_deactivate (AnjutaPlugin *obj)
{
	SourceviewPlugin* plugin = ANJUTA_PLUGIN_SOURCEVIEW (obj);
	AnjutaUI *ui;

	DEBUG_PRINT ("%s", "SourceviewPlugin: Dectivating SourceviewPlugin plugin ...");

	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (plugin)->shell, NULL);
	anjuta_ui_unmerge (ui, plugin->uiid);
	if (plugin->group != NULL)
	{
		anjuta_ui_remove_action_group (ui, plugin->group);
		plugin->group = NULL;
	}

	return TRUE;
}

static void
sourceview_plugin_finalize (GObject *obj)
{
	/* Finalization codes here */
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
sourceview_plugin_dispose (GObject *obj)
{
	SourceviewPlugin* plugin = ANJUTA_PLUGIN_SOURCEVIEW (obj);

	g_object_unref (plugin->settings);

	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
sourceview_plugin_instance_init (GObject *obj)
{
	SourceviewPlugin* plugin = ANJUTA_PLUGIN_SOURCEVIEW (obj);
	plugin->settings = g_settings_new (PREF_SCHEMA);
	plugin->group = NULL;
	plugin->uiid = 0;
}

static void
sourceview_plugin_class_init (GObjectClass *klass)
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = sourceview_plugin_activate;
	plugin_class->deactivate = sourceview_plugin_deactivate;
	klass->finalize = sourceview_plugin_finalize;
	klass->dispose = sourceview_plugin_dispose;
}

static IAnjutaEditor*
ieditor_factory_new_editor(IAnjutaEditorFactory* factory,
								GFile* file,
								const gchar* filename,
								GError** error)
{
	AnjutaPlugin* plugin = ANJUTA_PLUGIN(factory);
	SourceviewPlugin* splugin = ANJUTA_PLUGIN_SOURCEVIEW (plugin);
	gchar* current_style = g_settings_get_string (splugin->settings, SOURCEVIEW_STYLE);
	GtkSourceStyleSchemeManager* manager = gtk_source_style_scheme_manager_get_default();
	Sourceview* sv;
	sv = sourceview_new(file, filename, plugin);
	if (current_style)
		gtk_source_buffer_set_style_scheme (GTK_SOURCE_BUFFER (sv->priv->document),
											gtk_source_style_scheme_manager_get_scheme (manager,
																						current_style));
	g_free (current_style);
	return IANJUTA_EDITOR (sv);
}

static void
ieditor_factory_iface_init (IAnjutaEditorFactoryIface *iface)
{
	iface->new_editor = ieditor_factory_new_editor;
}

enum
{
	COLUMN_NAME = 0,
	COLUMN_DESC,
	COLUMN_ID
};

static GtkTreeModel*
create_style_model (GSettings* settings, GtkTreeIter** current)
{
	GtkListStore* model = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING,
											  G_TYPE_STRING);
	GtkSourceStyleSchemeManager* manager = gtk_source_style_scheme_manager_get_default();
	const gchar* const *styles = gtk_source_style_scheme_manager_get_scheme_ids (manager);
	const gchar* const *style;
	gchar* current_style = g_settings_get_string (settings, SOURCEVIEW_STYLE);
	*current = NULL;
	for (style = styles; *style != NULL; style++)
	{
		GtkTreeIter iter;
		GtkSourceStyleScheme* scheme =
			gtk_source_style_scheme_manager_get_scheme (manager, *style);
		const gchar* id = gtk_source_style_scheme_get_id (scheme);
		gtk_list_store_append (model, &iter);
		gtk_list_store_set (model, &iter,
							COLUMN_NAME, gtk_source_style_scheme_get_name (scheme),
							COLUMN_DESC, gtk_source_style_scheme_get_description (scheme),
							COLUMN_ID, id, -1);
		if (current_style && g_str_equal (id, current_style))
		{
			*current = gtk_tree_iter_copy (&iter);
		}
	}
	g_free (current_style);
	return GTK_TREE_MODEL (model);
}

static void
on_style_changed (GtkComboBox* combo, SourceviewPlugin* plugin)
{
	GtkTreeIter iter;
	gchar* id;
	GtkSourceStyleSchemeManager* manager = gtk_source_style_scheme_manager_get_default();
	GtkSourceStyleScheme* scheme;
	IAnjutaDocumentManager* docman;
	AnjutaShell* shell = ANJUTA_PLUGIN (plugin)->shell;
	gtk_combo_box_get_active_iter (combo, &iter);
	gtk_tree_model_get (gtk_combo_box_get_model(combo), &iter,
						COLUMN_ID, &id, -1);
	scheme = gtk_source_style_scheme_manager_get_scheme (manager, id);

	g_settings_set_string (plugin->settings,
	                       SOURCEVIEW_STYLE,
	                       id);
	g_free (id);


	docman = anjuta_shell_get_interface (shell,
										 IAnjutaDocumentManager, NULL);
	if (docman)
	{
		GList* editors = ianjuta_document_manager_get_doc_widgets (docman, NULL);
		GList* node;
		for (node = editors; node != NULL; node = g_list_next (node))
		{
			IAnjutaDocument* editor = IANJUTA_DOCUMENT (node->data);
			if (ANJUTA_IS_SOURCEVIEW (editor))
			{
				Sourceview* sv = ANJUTA_SOURCEVIEW (editor);
				gtk_source_buffer_set_style_scheme (GTK_SOURCE_BUFFER (sv->priv->document),
													scheme);
			}
		}
	}
}

static void
ipreferences_merge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	/* Add preferences */
	SourceviewPlugin* plugin = ANJUTA_PLUGIN_SOURCEVIEW (ipref);
	GtkCellRenderer* renderer_name = gtk_cell_renderer_text_new ();
	GtkCellRenderer* renderer_desc = gtk_cell_renderer_text_new ();
	GtkTreeIter* iter = NULL;
	GError* error = NULL;
	builder = gtk_builder_new ();
	if (!gtk_builder_add_from_file(builder, PREFS_GLADE, &error))
	{
		DEBUG_PRINT ("Could load sourceview preferences: %s", error->message);
		g_error_free (error);
		return;
	}
	anjuta_preferences_add_from_builder (prefs,
	                                     builder,
	                                     plugin->settings,
	                                     "Editor",
	                                     _("GtkSourceView Editor"),
	                                     ICON_FILE);

	plugin->check_font = GTK_WIDGET (gtk_builder_get_object (builder,
	                                                         FONT_USE_THEME_BUTTON));
	g_signal_connect(G_OBJECT(plugin->check_font), "toggled",
	                 G_CALLBACK(on_font_check_toggled), builder);
	on_font_check_toggled (GTK_TOGGLE_BUTTON (plugin->check_font), builder);

	/* Init styles combo */
	plugin->combo_styles = GTK_WIDGET (gtk_builder_get_object (builder, COMBO_STYLES));
	gtk_combo_box_set_model (GTK_COMBO_BOX (plugin->combo_styles),
							 create_style_model(plugin->settings, &iter));
	g_signal_connect (plugin->combo_styles, "changed", G_CALLBACK (on_style_changed), plugin);

	gtk_cell_layout_clear (GTK_CELL_LAYOUT(plugin->combo_styles));
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT(plugin->combo_styles), renderer_name, TRUE);
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT(plugin->combo_styles), renderer_desc, FALSE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT(plugin->combo_styles), renderer_name,
								   "text", COLUMN_NAME);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT(plugin->combo_styles), renderer_desc,
								   "text", COLUMN_DESC);
	g_object_set (renderer_desc,
				  "style", PANGO_STYLE_ITALIC, NULL);
	if (iter)
	{
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (plugin->combo_styles),
									   iter);
		gtk_tree_iter_free (iter);
	}
}

static void
ipreferences_unmerge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	SourceviewPlugin* plugin = ANJUTA_PLUGIN_SOURCEVIEW (ipref);
	g_signal_handlers_disconnect_by_func(G_OBJECT(plugin->check_font),
		G_CALLBACK(on_font_check_toggled), builder);
	g_signal_handlers_disconnect_by_func(G_OBJECT(plugin->combo_styles),
		G_CALLBACK(on_style_changed), builder);

	anjuta_preferences_remove_page(prefs, _("GtkSourceView Editor"));
	g_object_unref(builder);
	builder = NULL;
}

static void
ipreferences_iface_init(IAnjutaPreferencesIface* iface)
{
	iface->merge = ipreferences_merge;
	iface->unmerge = ipreferences_unmerge;
}

ANJUTA_PLUGIN_BEGIN (SourceviewPlugin, sourceview_plugin);
ANJUTA_TYPE_ADD_INTERFACE(ieditor_factory, IANJUTA_TYPE_EDITOR_FACTORY);
ANJUTA_TYPE_ADD_INTERFACE(ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (SourceviewPlugin, sourceview_plugin);
