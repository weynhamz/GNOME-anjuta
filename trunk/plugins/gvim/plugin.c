/*
 *  Copyright (c) Naba Kumar <naba@gnome.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <config.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/interfaces/ianjuta-editor-factory.h>

#include "plugin.h"
#include "anjuta-vim.h"

static gpointer parent_class;

static gboolean anjuta_vim_plugin_activate (AnjutaPlugin *plugin);
static gboolean anjuta_vim_plugin_deactivate (AnjutaPlugin *plugin);
static void anjuta_vim_plugin_instance_init (GObject *obj);
static void anjuta_vim_plugin_class_init (GObjectClass *klass);

static gboolean
anjuta_vim_plugin_activate (AnjutaPlugin *plugin)
{
	DEBUG_PRINT ("AnjutaVimPlugin: Activating Vim plugin...");
	return TRUE;
}


static gboolean
anjuta_vim_plugin_deactivate (AnjutaPlugin *plugin)
{
	DEBUG_PRINT ("AnjutaVimPlugin: Dectivating Vim plugin ...");
	return TRUE;
}

static void
anjuta_vim_plugin_finalize (GObject *obj)
{
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}


static void
anjuta_vim_plugin_dispose (GObject *obj)
{
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
anjuta_vim_plugin_instance_init (GObject *obj)
{
}

static void
anjuta_vim_plugin_class_init (GObjectClass *klass)
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = anjuta_vim_plugin_activate;
	plugin_class->deactivate = anjuta_vim_plugin_deactivate;
	klass->dispose = anjuta_vim_plugin_dispose;
	klass->finalize = anjuta_vim_plugin_finalize;
}

static IAnjutaEditor*
ieditor_factory_new_editor(IAnjutaEditorFactory* factory, 
						   const gchar* uri,
						   const gchar* filename, 
						   GError** error)
{
	AnjutaPlugin* plugin = ANJUTA_PLUGIN(factory);
	IAnjutaEditor* editor = IANJUTA_EDITOR(anjuta_vim_new (uri, filename,
														   plugin));
	return editor;
}

static void
ieditor_factory_iface_init (IAnjutaEditorFactoryIface *iface)
{
	iface->new_editor = ieditor_factory_new_editor;
}

ANJUTA_PLUGIN_BEGIN (AnjutaVimPlugin, anjuta_vim_plugin);
ANJUTA_TYPE_ADD_INTERFACE(ieditor_factory, IANJUTA_TYPE_EDITOR_FACTORY);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (AnjutaVimPlugin, anjuta_vim_plugin);
