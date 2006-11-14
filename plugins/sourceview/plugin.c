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

#include "plugin.h"
#include "sourceview.h"

#define PREFS_GLADE PACKAGE_DATA_DIR"/glade/sourceview.glade"
#define ICON_FILE "sourceview.png"

#define COLOR_USE_THEME_BUTTON "preferences_toggle:bool:1:0:sourceview.color.use_theme"
#define COLOR_TEXT "preferences_color:color:#FFFFFF:0:sourceview.color.text"
#define COLOR_BACKGROUND "preferences_color:color:#000000:0:sourceview.color.background"
#define COLOR_SELECTED_TEXT "preferences_color:color:#000000:0:sourceview.color.selected_text"
#define COLOR_SELECTION "preferences_color:color:#0000FF:0:sourceview.color.selection"

#define FONT_USE_THEME_BUTTON "preferences_toggle:bool:1:0:sourceview.font.use_theme"
#define FONT_BUTTON "preferences_font:font:Sans:0:sourceview.font"

static gpointer parent_class;

static GladeXML* gxml = NULL;

static void
on_color_check_toggled(GtkToggleButton* button, GladeXML* gxml)
{
	GtkWidget* color_button;
	color_button = glade_xml_get_widget(gxml, COLOR_TEXT);
	gtk_widget_set_sensitive(color_button, !gtk_toggle_button_get_active(button));

	color_button = glade_xml_get_widget(gxml, COLOR_BACKGROUND);
	gtk_widget_set_sensitive(color_button, !gtk_toggle_button_get_active(button));

	color_button = glade_xml_get_widget(gxml, COLOR_SELECTED_TEXT);
	gtk_widget_set_sensitive(color_button, !gtk_toggle_button_get_active(button));

	color_button = glade_xml_get_widget(gxml, COLOR_SELECTION);
	gtk_widget_set_sensitive(color_button, !gtk_toggle_button_get_active(button));
}	

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
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

static void
sourceview_plugin_dispose (GObject *obj)
{
	/* Disposition codes */
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
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
	IAnjutaEditor* editor = IANJUTA_EDITOR(sourceview_new(uri, filename, plugin));
	return editor;
}

static void
ieditor_factory_iface_init (IAnjutaEditorFactoryIface *iface)
{
	iface->new_editor = ieditor_factory_new_editor;
}

static void
ipreferences_merge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	/* Add preferences */
	SourceviewPlugin* plugin = SOURCEVIEW_PLUGIN (ipref);
	gxml = glade_xml_new (PREFS_GLADE, "preferences_dialog", NULL);
	anjuta_preferences_add_page (prefs,
								 gxml, "Editor", _("GtkSourceView Editor"), ICON_FILE);
	
	plugin->check_color = glade_xml_get_widget(gxml, COLOR_USE_THEME_BUTTON);
	g_signal_connect(G_OBJECT(plugin->check_color), "toggled", G_CALLBACK(on_color_check_toggled), gxml);
	plugin->check_font = glade_xml_get_widget(gxml, FONT_USE_THEME_BUTTON);
	g_signal_connect(G_OBJECT(plugin->check_font), "toggled", G_CALLBACK(on_font_check_toggled), gxml);
}

static void
ipreferences_unmerge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	SourceviewPlugin* plugin = SOURCEVIEW_PLUGIN (ipref);
	g_signal_handlers_disconnect_by_func(G_OBJECT(plugin->check_color), 
		G_CALLBACK(on_color_check_toggled), gxml);
	g_signal_handlers_disconnect_by_func(G_OBJECT(plugin->check_font), 
		G_CALLBACK(on_font_check_toggled), gxml);
	
	anjuta_preferences_dialog_remove_page(ANJUTA_PREFERENCES_DIALOG(prefs), _("GtkSourceView Editor"));
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
