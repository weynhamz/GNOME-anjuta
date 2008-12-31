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
#include <libanjuta/anjuta-encodings.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-savable.h>
#include <libanjuta/interfaces/ianjuta-editor-factory.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>


#include "aneditor.h"
#include "lexer.h"
#include "plugin.h"
#include "style-editor.h"
#include "text_editor.h"

#define PREFS_GLADE PACKAGE_DATA_DIR"/glade/anjuta-editor-scintilla.glade"
#define ICON_FILE "anjuta-editor-scintilla-plugin-48.png"

gpointer parent_class;

static void 
on_style_button_clicked(GtkWidget* button, AnjutaPreferences* prefs)
{
	StyleEditor* se = style_editor_new(prefs);
	style_editor_show(se);
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	
	return TRUE;
}

static void
dispose (GObject *obj)
{
	/* EditorPlugin *eplugin = ANJUTA_PLUGIN_EDITOR (obj); */

	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
finalize (GObject *obj)
{
	/* Finalization codes here */
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
editor_plugin_instance_init (GObject *obj)
{
	/* EditorPlugin *plugin = ANJUTA_PLUGIN_EDITOR (obj); */
}

static void
editor_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
	klass->finalize = finalize;
}

static IAnjutaEditor*
itext_editor_factory_new_editor(IAnjutaEditorFactory* factory, 
								GFile* file,
								const gchar* filename, 
								GError** error)
{
	AnjutaShell *shell = ANJUTA_PLUGIN (factory)->shell;
	AnjutaPreferences *prefs = anjuta_shell_get_preferences (shell, NULL);
	AnjutaStatus *status = anjuta_shell_get_status (shell, NULL);
	/* file can be NULL, if we open a buffer, not a file */
	gchar* uri = file ? g_file_get_uri (file) : NULL;
	IAnjutaEditor* editor = IANJUTA_EDITOR(text_editor_new(status, prefs, shell,
														   uri, filename));
	g_free(uri);
	return editor;
}

static void
itext_editor_factory_iface_init (IAnjutaEditorFactoryIface *iface)
{
	iface->new_editor = itext_editor_factory_new_editor;
}

static void
ipreferences_merge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	GladeXML* gxml;
	EditorPlugin* plugin = ANJUTA_PLUGIN_EDITOR (ipref);
	gxml = glade_xml_new (PREFS_GLADE, "preferences_dialog", NULL);
	plugin->style_button = glade_xml_get_widget(gxml, "style_button");
	g_signal_connect(G_OBJECT(plugin->style_button), "clicked", 
		G_CALLBACK(on_style_button_clicked), prefs);
	anjuta_preferences_add_page (prefs,
								 gxml, "Editor", _("Scintilla Editor"),  ICON_FILE);
	g_object_unref(gxml);
}

static void
ipreferences_unmerge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	EditorPlugin* plugin = ANJUTA_PLUGIN_EDITOR (ipref);
	g_signal_handlers_disconnect_by_func(G_OBJECT(plugin->style_button), 
		G_CALLBACK(on_style_button_clicked), 
		anjuta_shell_get_preferences(ANJUTA_PLUGIN(plugin)->shell, NULL));
	
	anjuta_preferences_remove_page(prefs, _("Scintilla Editor"));
}

static void
ipreferences_iface_init(IAnjutaPreferencesIface* iface)
{
	iface->merge = ipreferences_merge;
	iface->unmerge = ipreferences_unmerge;	
}

ANJUTA_PLUGIN_BEGIN (EditorPlugin, editor_plugin);
ANJUTA_TYPE_ADD_INTERFACE(itext_editor_factory, IANJUTA_TYPE_EDITOR_FACTORY);
ANJUTA_TYPE_ADD_INTERFACE(ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (EditorPlugin, editor_plugin);
