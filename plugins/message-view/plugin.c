/*  plugin.c (c) Johannes Schmid 2004
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#include <config.h>
#include <libanjuta/anjuta-shell.h>

#include "plugin.h"

gpointer parent_class;

static void
shell_set (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	MessageViewPlugin *mv_plugin;
	
	mv_plugin = (MessageViewPlugin*) plugin;
	ui = plugin->ui;
	
	/* Which UI_FILE? */
	//mv_plugin->uiid = anjuta_ui_merge (ui, UI_FILE);
}
	
static void
dispose (GObject *obj)
{
	MessageViewPlugin *plugin = (MessageViewPlugin*)obj;
	anjuta_ui_unmerge (ANJUTA_PLUGIN (obj)->ui, plugin->uiid);
}

static void
message_view_plugin_instance_init (GObject *obj)
{
	MessageViewPlugin *plugin = (MessageViewPlugin*)obj;
	plugin->uiid = 0;
}

static void
message_view_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->shell_set = shell_set;
	klass->dispose = dispose;
}

ANJUTA_PLUGIN_BOILERPLATE (MessageViewPlugin, message_view_plugin);
ANJUTA_SIMPLE_PLUGIN (MessageViewPlugin, message_view_plugin);
