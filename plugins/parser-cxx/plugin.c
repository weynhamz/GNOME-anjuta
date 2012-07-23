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
#include <ctype.h>
#include <stdlib.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-editor-assist.h>
#include <libanjuta/interfaces/ianjuta-document.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>
#include <libanjuta/interfaces/ianjuta-language.h>

#include "plugin.h"

#define PREFS_BUILDER PACKAGE_DATA_DIR"/glade/anjuta-parser-cxx.ui"
#define ICON_FILE "anjuta-parser-cxx-plugin.png"

/* Preferences keys */
#define ANJUTA_PREF_SCHEMA_PREFIX "org.gnome.anjuta."
#define PREF_SCHEMA "org.gnome.anjuta.plugins.parser-cxx"

static gpointer parent_class;

/* Enable/Disable language-support */
static void
install_support (ParserCxxPlugin *parser_plugin)
{
    IAnjutaLanguage* lang_manager =
        anjuta_shell_get_interface (ANJUTA_PLUGIN (parser_plugin)->shell,
                                    IAnjutaLanguage, NULL);
	
    if (!lang_manager)
        return;

    if (parser_plugin->support_installed)
        return;

    parser_plugin->current_language =
        ianjuta_language_get_name_from_editor (lang_manager,
                                               IANJUTA_EDITOR_LANGUAGE (parser_plugin->current_editor), NULL);

    DEBUG_PRINT("Parser support installed for: %s",
                parser_plugin->current_language);
	
	if (parser_plugin->current_language &&
	    (g_str_equal (parser_plugin->current_language, "C" )
	     || g_str_equal (parser_plugin->current_language, "C++")))
	{
		ParserCxxAssist *assist;
	
		g_assert (parser_plugin->assist == NULL);
	
		assist = parser_cxx_assist_new (IANJUTA_EDITOR (parser_plugin->current_editor),
					anjuta_shell_get_interface (
							anjuta_plugin_get_shell (ANJUTA_PLUGIN (parser_plugin)),
				    		IAnjutaSymbolManager, NULL),
		            parser_plugin->settings);
		
		if (!assist)
			return;
	
		parser_plugin->assist = assist;
	}
	else
		return;

	parser_plugin->support_installed = TRUE;
}

static void
uninstall_support (ParserCxxPlugin *parser_plugin)
{
    if (!parser_plugin->support_installed)
        return;

    if (parser_plugin->assist)
    {
        g_object_unref (parser_plugin->assist);
        parser_plugin->assist = NULL;
    }
	
    parser_plugin->support_installed = FALSE;
}

static void
on_editor_language_changed (IAnjutaEditor *editor,
                            const gchar *new_language,
                            ParserCxxPlugin *plugin)
{
    uninstall_support (plugin);
    install_support (plugin);
}

static void
on_value_added_current_editor (AnjutaPlugin *plugin, const gchar *name,
                               const GValue *value, gpointer data)
{
    ParserCxxPlugin *parser_plugin;
    IAnjutaDocument* doc = IANJUTA_DOCUMENT(g_value_get_object (value));
    parser_plugin = ANJUTA_PLUGIN_PARSER_CXX (plugin);
	
    if (IANJUTA_IS_EDITOR(doc))
        parser_plugin->current_editor = G_OBJECT(doc);
    else
    {
        parser_plugin->current_editor = NULL;
        return;
    }
	
    if (IANJUTA_IS_EDITOR(parser_plugin->current_editor))
        install_support (parser_plugin);
	
    g_signal_connect (parser_plugin->current_editor, "language-changed",
                      G_CALLBACK (on_editor_language_changed),
                      plugin);
}

static void
on_value_removed_current_editor (AnjutaPlugin *plugin, const gchar *name,
                                 gpointer data)
{
    ParserCxxPlugin *parser_plugin;
    parser_plugin = ANJUTA_PLUGIN_PARSER_CXX (plugin);
	
    if (parser_plugin->current_editor)
        g_signal_handlers_disconnect_by_func (parser_plugin->current_editor,
                                          G_CALLBACK (on_editor_language_changed),
                                          plugin);
	
    if (IANJUTA_IS_EDITOR(parser_plugin->current_editor))
        uninstall_support (parser_plugin);
	
    parser_plugin->current_editor = NULL;
}

static gboolean
parser_cxx_plugin_activate_plugin (AnjutaPlugin *plugin)
{
    ParserCxxPlugin *parser_plugin;

    parser_plugin = ANJUTA_PLUGIN_PARSER_CXX (plugin);

    DEBUG_PRINT ("%s", "AnjutaParserCxxPlugin: Activating plugin ...");
	
    parser_plugin->editor_watch_id =
        anjuta_plugin_add_watch (plugin,
                                 IANJUTA_DOCUMENT_MANAGER_CURRENT_DOCUMENT,
                                 on_value_added_current_editor,
                                 on_value_removed_current_editor,
                                 plugin);

    return TRUE;
}

static gboolean
parser_cxx_plugin_deactivate_plugin (AnjutaPlugin *plugin)
{
    ParserCxxPlugin *parser_plugin;
    parser_plugin = ANJUTA_PLUGIN_PARSER_CXX (plugin);

    anjuta_plugin_remove_watch (plugin,
                                parser_plugin->editor_watch_id,
                                TRUE);

    DEBUG_PRINT ("%s", "AnjutaParserCxxPlugin: Deactivated plugin.");
    return TRUE;
}

static void
parser_cxx_plugin_finalize (GObject *obj)
{
    /* Finalization codes here */
    G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
parser_cxx_plugin_dispose (GObject *obj)
{
    ParserCxxPlugin* plugin = ANJUTA_PLUGIN_PARSER_CXX (obj);
    
    /* Disposition codes */
    g_object_unref (plugin->settings);
    g_object_unref (plugin->editor_settings);
	
    G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
parser_cxx_plugin_instance_init (GObject *obj)
{
    ParserCxxPlugin *plugin = ANJUTA_PLUGIN_PARSER_CXX (obj);
    plugin->current_editor = NULL;
    plugin->current_language = NULL;
    plugin->editor_watch_id = 0;
    plugin->assist = NULL;
    plugin->settings = g_settings_new (PREF_SCHEMA);
    plugin->editor_settings = g_settings_new (ANJUTA_PREF_SCHEMA_PREFIX IANJUTA_EDITOR_PREF_SCHEMA);
}

static void
parser_cxx_plugin_class_init (GObjectClass *klass)
{
    AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

    parent_class = g_type_class_peek_parent (klass);

    plugin_class->activate = parser_cxx_plugin_activate_plugin;
    plugin_class->deactivate = parser_cxx_plugin_deactivate_plugin;
    klass->finalize = parser_cxx_plugin_finalize;
    klass->dispose = parser_cxx_plugin_dispose;
}

#define PREF_WIDGET_SPACE "preferences:completion-space-after-func"
#define PREF_WIDGET_BRACE "preferences:completion-brace-after-func"
#define PREF_WIDGET_CLOSEBRACE "preferences:completion-closebrace-after-func"
#define PREF_WIDGET_AUTO "preferences:completion-enable"

static void
on_autocompletion_toggled (GtkToggleButton* button,
                           ParserCxxPlugin* plugin)
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
    GError* error = NULL;
    ParserCxxPlugin* plugin = ANJUTA_PLUGIN_PARSER_CXX (ipref);
    plugin->bxml = gtk_builder_new ();
    GtkWidget* toggle;

    /* Add preferences */
    if (!gtk_builder_add_from_file (plugin->bxml, PREFS_BUILDER, &error))
    {
        g_warning ("Couldn't load builder file: %s", error->message);
        g_error_free (error);
    }
    anjuta_preferences_add_from_builder (prefs,
                                         plugin->bxml, plugin->settings,
                                         "preferences", _("Auto-complete"),
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
    ParserCxxPlugin* plugin = ANJUTA_PLUGIN_PARSER_CXX (ipref);
    anjuta_preferences_remove_page(prefs, _("Auto-complete"));
    g_object_unref (plugin->bxml);
}

static void
ipreferences_iface_init (IAnjutaPreferencesIface* iface)
{
    iface->merge = ipreferences_merge;
    iface->unmerge = ipreferences_unmerge;
}

ANJUTA_PLUGIN_BEGIN (ParserCxxPlugin, parser_cxx_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE (ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (ParserCxxPlugin, parser_cxx_plugin);
