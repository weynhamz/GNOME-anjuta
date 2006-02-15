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
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-encodings.h>
#include <libegg/menu/egg-entry-action.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-savable.h>
#include <libanjuta/interfaces/ianjuta-editor-factory.h>


#include "aneditor.h"
#include "lexer.h"
#include "plugin.h"
#include "style-editor.h"
#include "text_editor.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-document-manager.ui"
#define PREFS_GLADE PACKAGE_DATA_DIR"/glade/anjuta-document-manager.glade"
#define ICON_FILE "anjuta-document-manager.png"

gpointer parent_class;

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
	/* EditorPlugin *eplugin = (EditorPlugin*)obj; */
	
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
editor_plugin_instance_init (GObject *obj)
{
	/* EditorPlugin *plugin = (EditorPlugin*)obj; */
}

static void
editor_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
}

static IAnjutaEditor*
itext_editor_factory_new_editor(IAnjutaEditorFactory* factory, 
								const gchar* uri,
								const gchar* filename, 
								GError** error)
{
	AnjutaShell *shell = ANJUTA_PLUGIN (factory)->shell;
	AnjutaPreferences *prefs = anjuta_shell_get_preferences (shell, NULL);
	AnjutaStatus *status = anjuta_shell_get_status (shell, NULL);
	IAnjutaEditor* editor = IANJUTA_EDITOR(text_editor_new(status, prefs,
														   uri, filename));
	return editor;
}

static gpointer
itext_editor_factory_new_style_editor(IAnjutaEditorFactory *factory, GError **e)
{
	AnjutaShell *shell = ANJUTA_PLUGIN (factory)->shell;
	AnjutaPreferences *prefs = anjuta_shell_get_preferences (shell, NULL);
	StyleEditor* se = style_editor_new(prefs);
	style_editor_show (se);
	return se;
}

static void
itext_editor_factory_iface_init (IAnjutaEditorFactoryIface *iface)
{
	iface->new_editor = itext_editor_factory_new_editor;
	iface->new_style_editor = itext_editor_factory_new_style_editor;
}

ANJUTA_PLUGIN_BEGIN (EditorPlugin, editor_plugin);
ANJUTA_TYPE_ADD_INTERFACE(itext_editor_factory, IANJUTA_TYPE_EDITOR_FACTORY);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (EditorPlugin, editor_plugin);
