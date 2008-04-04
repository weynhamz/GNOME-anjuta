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
#include "sourceview-private.h"

#define PREFS_GLADE PACKAGE_DATA_DIR"/glade/anjuta-editor-sourceview.glade"
#define ICON_FILE "anjuta-editor-sourceview-plugin-48.png"

#define COMBO_STYLES "combo_styles"
#define SOURCEVIEW_STYLE "sourceview.style"
#define SOURCEVIEW_DEFAULT_STYLE "classic"

#define FONT_USE_THEME_BUTTON "preferences_toggle:bool:1:0:sourceview.font.use_theme"
#define FONT_BUTTON "preferences_font:font:Monospace 12:0:sourceview.font"

static gpointer parent_class;

static GladeXML* gxml = NULL;

static void
on_font_check_toggled(GtkToggleButton* button, GladeXML* gxml)
{
	GtkWidget* font_button;
	font_button = glade_xml_get_widget(gxml, FONT_BUTTON);
	gtk_widget_set_sensitive(font_button, !gtk_toggle_button_get_active(button));
}

static gboolean
sourceview_plugin_activate (AnjutaPlugin *plugin)
{	
	DEBUG_PRINT ("SourceviewPlugin: Activating SourceviewPlugin plugin ...");

	return TRUE;
}

static gboolean
sourceview_plugin_deactivate (AnjutaPlugin *plugin)
{
	DEBUG_PRINT ("SourceviewPlugin: Dectivating SourceviewPlugin plugin ...");
	
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
	/* Disposition codes */
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
sourceview_plugin_instance_init (GObject *obj)
{

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
								const gchar* uri,
								const gchar* filename, 
								GError** error)
{
	AnjutaPlugin* plugin = ANJUTA_PLUGIN(factory);
	AnjutaPreferences* prefs = anjuta_shell_get_preferences (plugin->shell, NULL);
	gchar* current_style = anjuta_preferences_get (prefs, SOURCEVIEW_STYLE);
	GtkSourceStyleSchemeManager* manager = gtk_source_style_scheme_manager_get_default();
	Sourceview* sv;
	if (!current_style)
	{
		current_style = g_strdup (SOURCEVIEW_DEFAULT_STYLE);
	}
	sv = sourceview_new(uri, filename, plugin);
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
create_style_model (AnjutaPreferences* prefs, GtkTreeIter** current)
{
	GtkListStore* model = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING,
											  G_TYPE_STRING);
	GtkSourceStyleSchemeManager* manager = gtk_source_style_scheme_manager_get_default();
	const gchar* const *styles = gtk_source_style_scheme_manager_get_scheme_ids (manager);
	const gchar* const *style;
	gchar* current_style = anjuta_preferences_get (prefs, SOURCEVIEW_STYLE);
	if (!current_style)
	{
		current_style = g_strdup (SOURCEVIEW_DEFAULT_STYLE);
	}
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
		if (g_str_equal (id, current_style))
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

	anjuta_preferences_set (anjuta_shell_get_preferences (shell, NULL),
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
	gxml = glade_xml_new (PREFS_GLADE, "preferences_dialog", NULL);
	anjuta_preferences_add_page (prefs,
								 gxml, "Editor", _("GtkSourceView Editor"), ICON_FILE);
	
	plugin->check_font = glade_xml_get_widget(gxml, FONT_USE_THEME_BUTTON);
	g_signal_connect(G_OBJECT(plugin->check_font), "toggled", G_CALLBACK(on_font_check_toggled), gxml);
	on_font_check_toggled (GTK_TOGGLE_BUTTON (plugin->check_font), gxml);
	
	/* Init styles combo */
	plugin->combo_styles = glade_xml_get_widget (gxml, COMBO_STYLES);
	gtk_combo_box_set_model (GTK_COMBO_BOX (plugin->combo_styles),
							 create_style_model(prefs, &iter));
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
		G_CALLBACK(on_font_check_toggled), gxml);
	g_signal_handlers_disconnect_by_func(G_OBJECT(plugin->combo_styles), 
		G_CALLBACK(on_style_changed), gxml);
	
	anjuta_preferences_remove_page(prefs, _("GtkSourceView Editor"));
	g_object_unref(gxml);
	gxml = NULL;
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
