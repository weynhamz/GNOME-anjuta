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

static void on_next_message(GtkAction* menuitem, MessageViewPlugin *plugin)
{
	g_message("Next Message");
}

static void on_prev_message(GtkAction* menuitem, MessageViewPlugin *plugin)
{
	g_message("Prev Message");
}

static void on_show_messages(GtkAction* menuitem, MessageViewPlugin *plugin)
{
	g_message("Show Messages");
}


static GtkActionEntry actions_goto[] = {
  { "ActionMessageNext", GTK_STOCK_GO_FORWARD,
    N_("_Next message"), NULL,
	N_("Next message"),
    G_CALLBACK (on_next_message)},
  { "ActionMessagePrev", GTK_STOCK_GO_BACK,
    N_("_Previous message"), NULL,
	N_("Previous message"),
    G_CALLBACK (on_prev_message)}
};

static GtkActionEntry actions_view[] = {
  { "ActionViewMessageShow", NULL,
    N_("_Show messages"), NULL,
	N_("Show messages"),
    G_CALLBACK (on_show_messages)}
};

gpointer parent_class;

static void
activate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	MessageViewPlugin *mv_plugin;
	GtkWidget* msgman;
	
	g_message ("MessageViewPlugin: Activating MessageView plugin ...");
	mv_plugin = (MessageViewPlugin*) plugin;
	ui = plugin->ui;
	msgman = anjuta_msgman_new(plugin->prefs);
	mv_plugin->msgman = msgman;
	
	anjuta_ui_add_action_group_entries (ui, "ActionGroupGotoMessages",
										_("Next/Prev Message"),
										actions_goto,
										G_N_ELEMENTS (actions_goto), plugin);
	anjuta_ui_add_action_group_entries (ui, "ActionGroupViewMessages",
										_("View Messages"),
										actions_view,
										G_N_ELEMENTS (actions_view), plugin);
	
	mv_plugin->uiid = anjuta_ui_merge (plugin->ui, UI_FILE);
	anjuta_shell_add_widget (plugin->shell, msgman,
				  "AnjutaMessageView", _("Messages"), NULL);
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	g_message ("MessageViewPlugin: Dectivating message view plugin ...");
	anjuta_shell_remove_widget (plugin->shell,
								((MessageViewPlugin*)plugin)->msgman, NULL);
	anjuta_ui_unmerge (plugin->ui, ((MessageViewPlugin*)plugin)->uiid);
	return TRUE;
}

static void
dispose (GObject *obj)
{
	// MessageViewPlugin *plugin = (MessageViewPlugin*)obj;
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

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
}

ANJUTA_PLUGIN_BOILERPLATE (MessageViewPlugin, message_view_plugin);
ANJUTA_SIMPLE_PLUGIN (MessageViewPlugin, message_view_plugin);
