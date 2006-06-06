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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <config.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-editor-language.h>

#include "plugin.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-language-cpp-java-plugin.ui"

static gpointer parent_class;

static void
on_editor_char_inserted_cpp (IAnjutaEditor *editor,
							 const gchar ch,
							 CppJavaPlugin *plugin)
{
	if (ch != '\n' || ch != '\t')
		return;
	DEBUG_PRINT ("Indenting cpp code");
}

static void
on_editor_char_inserted_java (IAnjutaEditor *editor,
							  const gchar ch,
							  CppJavaPlugin *plugin)
{
	if (ch != '\n' || ch != '\t')
		return;
	DEBUG_PRINT ("Indenting java code");
}

static void
install_support (CppJavaPlugin *lang_plugin)
{
	if (lang_plugin->support_installed)
		return;
	
	lang_plugin->current_language
		= ianjuta_editor_language_get_language
					(IANJUTA_EDITOR_LANGUAGE (lang_plugin->current_editor),
											  NULL);
	
	if (lang_plugin->current_language &&
		strcmp (lang_plugin->current_language, "cpp") == 0)
	{
		g_signal_connect (lang_plugin->current_editor,
						  "char-added",
						  G_CALLBACK (on_editor_char_inserted_cpp),
						  lang_plugin);
	}
	else if (lang_plugin->current_language &&
		strcmp (lang_plugin->current_language, "java") == 0)
	{
		g_signal_connect (lang_plugin->current_editor,
						  "char-added",
						  G_CALLBACK (on_editor_char_inserted_java),
						  lang_plugin);
	}
	else
	{
		return;
	}
	lang_plugin->support_installed = TRUE;
}

static void
uninstall_support (CppJavaPlugin *lang_plugin)
{
	if (!lang_plugin->support_installed)
		return;
	
	if (lang_plugin->current_language &&
		strcmp (lang_plugin->current_language, "cpp") == 0)
	{
		g_signal_handlers_disconnect_by_func (lang_plugin->current_editor,
									G_CALLBACK (on_editor_char_inserted_cpp),
									lang_plugin);
	}
	else if (lang_plugin->current_language &&
		strcmp (lang_plugin->current_language, "java") == 0)
	{
		g_signal_handlers_disconnect_by_func (lang_plugin->current_editor,
									G_CALLBACK (on_editor_char_inserted_java),
									lang_plugin);
	}
	lang_plugin->support_installed = FALSE;
}

static void
on_editor_language_changed (IAnjutaEditor *editor,
							const gchar *new_language,
							CppJavaPlugin *plugin)
{
	uninstall_support (plugin);
	install_support (plugin);
}

static void
on_value_added_current_editor (AnjutaPlugin *plugin, const gchar *name,
							   const GValue *value, gpointer data)
{
	CppJavaPlugin *lang_plugin;
	lang_plugin = (CppJavaPlugin*) plugin;
	lang_plugin->current_editor = g_value_get_object (value);
	install_support (lang_plugin);
	g_signal_connect (lang_plugin->current_editor, "language-changed",
					  G_CALLBACK (on_editor_language_changed),
					  plugin);
}

static void
on_value_removed_current_editor (AnjutaPlugin *plugin, const gchar *name,
								 gpointer data)
{
	CppJavaPlugin *lang_plugin;
	lang_plugin = (CppJavaPlugin*) plugin;
	g_signal_handlers_disconnect_by_func (lang_plugin->current_editor,
										  G_CALLBACK (on_editor_language_changed),
										  plugin);
	uninstall_support (lang_plugin);
	lang_plugin->current_editor = NULL;
}

static gboolean
cpp_java_plugin_activate_plugin (AnjutaPlugin *plugin)
{
	CppJavaPlugin *lang_plugin;
	lang_plugin = (CppJavaPlugin*) plugin;
	
	DEBUG_PRINT ("AnjutaLanguageCppJavaPlugin: Activating plugin ...");
	
	lang_plugin->editor_watch_id = 
		anjuta_plugin_add_watch (plugin,
								 "document_manager_current_editor",
								 on_value_added_current_editor,
								 on_value_removed_current_editor,
								 plugin);
	return TRUE;
}

static gboolean
cpp_java_plugin_deactivate_plugin (AnjutaPlugin *plugin)
{
	CppJavaPlugin *lang_plugin;
	lang_plugin = (CppJavaPlugin*) plugin;
	anjuta_plugin_remove_watch (plugin,
								lang_plugin->editor_watch_id,
								TRUE);
	return TRUE;
}

static void
cpp_java_plugin_finalize (GObject *obj)
{
	/* Finalization codes here */
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

static void
cpp_java_plugin_dispose (GObject *obj)
{
	/* Disposition codes */
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
cpp_java_plugin_instance_init (GObject *obj)
{
	CppJavaPlugin *plugin = (CppJavaPlugin*)obj;
}

static void
cpp_java_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = cpp_java_plugin_activate_plugin;
	plugin_class->deactivate = cpp_java_plugin_deactivate_plugin;
	klass->finalize = cpp_java_plugin_finalize;
	klass->dispose = cpp_java_plugin_dispose;
}

ANJUTA_PLUGIN_BOILERPLATE (CppJavaPlugin, cpp_java_plugin);
ANJUTA_SIMPLE_PLUGIN (CppJavaPlugin, cpp_java_plugin);
