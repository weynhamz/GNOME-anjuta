/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) 2004 Johannes Schmid

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
#include "plugin.h"
#include "anjuta-msgman.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-message-manager.ui"

static void on_next_message(GtkWidget* menuitem, gpointer data)
{
	g_message("Next Message");
}

static void on_prev_message(GtkWidget* menuitem, gpointer data)
{
	g_message("Prev Message");
}

static void on_show_messages(GtkWidget* menuitem, gpointer data)
{
	g_message("Show Messages");
}


static EggActionGroupEntry actions_goto[] = {
  { "ActionMessageNext", N_("_Next message"), GTK_STOCK_GO_FORWARD, NULL,
	N_("Next message"),
    G_CALLBACK (on_next_message), NULL },
  { "ActionMessagePrev", N_("_Previous message"), GTK_STOCK_GO_BACK, NULL,
	N_("Previous message"),
    G_CALLBACK (on_prev_message), NULL }
};

static EggActionGroupEntry actions_view[] = {
  { "ActionViewMessageShow", N_("_Show messages"), NULL, NULL,
	N_("Show messages"),
    G_CALLBACK (on_show_messages), NULL }
};

gpointer parent_class;

static void
ui_set (AnjutaPlugin *plugin)
{
	g_message ("SamplePlugin: UI set");
}

static void
prefs_set (AnjutaPlugin *plugin)
{
	g_message ("SamplePlugin: Prefs set");
}

static void
shell_set (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	MessageViewPlugin *mv_plugin;
	GtkWidget* msgman;
	
	g_message ("MessageViewPlugin: Shell set. Activating MessageView plugin ...");
	mv_plugin = (MessageViewPlugin*) plugin;
	ui = plugin->ui;
	msgman = anjuta_msgman_new(plugin->prefs);
	mv_plugin->msgman = msgman;
	
	anjuta_ui_add_action_group_entries (ui, "ActionGroupGotoMessages",
										_("Next/Prev Message"),
										actions_goto,
										G_N_ELEMENTS (actions_goto));
	anjuta_ui_add_action_group_entries (ui, "ActionGroupViewMessages",
										_("View Messages"),
										actions_view,
										G_N_ELEMENTS (actions_view));
	
	mv_plugin->uiid = anjuta_ui_merge (plugin->ui, UI_FILE);
	anjuta_shell_add_widget (plugin->shell, msgman,
				  "AnjutaMessageViewPlugin", _("MessageViewPlugin"), NULL);
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
	plugin_class->ui_set = ui_set;
	plugin_class->prefs_set = prefs_set;
	klass->dispose = dispose;
}

ANJUTA_PLUGIN_BOILERPLATE (MessageViewPlugin, message_view_plugin);
ANJUTA_SIMPLE_PLUGIN (MessageViewPlugin, message_view_plugin);
