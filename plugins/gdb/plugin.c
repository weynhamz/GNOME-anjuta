/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) 2004 Naba Kumar

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
#include <libanjuta/interfaces/ianjuta-debugger-be.h>
#include <libanjuta/interfaces/ianjuta-message-manager.h>
#include <libanjuta/interfaces/ianjuta-message-view.h>

#include "plugin.h"
#include "debugger.h"

#define ICON_FILE "anjuta-gdb.plugin.png"

gpointer parent_class;

/* TODO: don't know ho to avoid having some global variables (we no more have
access to the global app object) */

static const gchar * MESSAGE_VIEW_TITLE = N_("Debug");
static GdbPlugin *gdb_plugin = NULL;


void
append_message (const gchar* message)
{
	GObject *obj;
	IAnjutaMessageManager *message_manager;
	IAnjutaMessageView *message_view;

	g_return_if_fail (message != NULL);

	obj = anjuta_shell_get_object (ANJUTA_PLUGIN (gdb_plugin)->shell,
			"IAnjutaMessageManager", NULL);
	message_manager = IANJUTA_MESSAGE_MANAGER (obj);

	/* TODO: error checking */

	message_view = ianjuta_message_manager_get_view_by_name (
			message_manager, MESSAGE_VIEW_TITLE, NULL);

	ianjuta_message_view_append (message_view, IANJUTA_MESSAGE_VIEW_TYPE_INFO,
			message, "", NULL);
}


void
show_messages (void)
{
	GObject *obj;
	IAnjutaMessageManager *message_manager;
	IAnjutaMessageView *message_view;

	obj = anjuta_shell_get_object (ANJUTA_PLUGIN (gdb_plugin)->shell,
			"IAnjutaMessageManager", NULL);
	message_manager = IANJUTA_MESSAGE_MANAGER (obj);

	/* TODO: error checking */

	message_view = ianjuta_message_manager_get_view_by_name (
			message_manager, MESSAGE_VIEW_TITLE, NULL);

	ianjuta_message_manager_set_current_view (message_manager, message_view, NULL);
}


void
clear_messages (void)
{
	GObject *obj;
	IAnjutaMessageManager *message_manager;
	IAnjutaMessageView *message_view;

	obj = anjuta_shell_get_object (ANJUTA_PLUGIN (gdb_plugin)->shell,
			"IAnjutaMessageManager", NULL);
	message_manager = IANJUTA_MESSAGE_MANAGER (obj);

	/* TODO: error checking */

	message_view = ianjuta_message_manager_get_view_by_name (
			message_manager, MESSAGE_VIEW_TITLE, NULL);

	ianjuta_message_view_clear (message_view, NULL);
}


AnjutaLauncher *
get_launcher (void)
{
	g_return_val_if_fail (gdb_plugin != NULL, NULL);

	return gdb_plugin->launcher;
}


static void
on_debug_buffer_flushed (IAnjutaMessageView *view, const gchar* line,
		AnjutaPlugin *plugin)
{
	g_return_if_fail (line != NULL);

	IAnjutaMessageViewType type = IANJUTA_MESSAGE_VIEW_TYPE_INFO;
	ianjuta_message_view_append (view, type, line, "", NULL);
}


static void
on_debug_mesg_clicked (IAnjutaMessageView* view, const gchar* line,
		AnjutaPlugin* plugin)
{
}


static gboolean
activate_plugin (AnjutaPlugin* plugin)
{
	GObject *obj;
	IAnjutaMessageManager *message_manager;
	IAnjutaMessageView *message_view;

	g_message ("GdbPlugin: Activating Gdb plugin ...");

	gdb_plugin = (GdbPlugin *) plugin;

	/* Query for object implementing IAnjutaMessageManager interface */
	obj = anjuta_shell_get_object (ANJUTA_PLUGIN (plugin)->shell,
			"IAnjutaMessageManager", NULL);
	message_manager = IANJUTA_MESSAGE_MANAGER (obj);

	/* TODO: error checking */
	message_view = ianjuta_message_manager_add_view (
			message_manager, MESSAGE_VIEW_TITLE, ICON_FILE, NULL);
	g_signal_connect (G_OBJECT (message_view), "buffer_flushed",
			G_CALLBACK (on_debug_buffer_flushed), plugin);
	g_signal_connect (G_OBJECT (message_view), "message_clicked",
			G_CALLBACK (on_debug_mesg_clicked), plugin);
	ianjuta_message_manager_set_current_view (message_manager, message_view, NULL);

	gdb_plugin->launcher = anjuta_launcher_new ();
	debugger_init ();

	return TRUE;
}


static gboolean
deactivate_plugin (AnjutaPlugin* plugin)
{
	g_message ("GdbPlugin: Deactivating Gdb plugin ...");

	/* TODO: remove view */

	debugger_shutdown ();

	return TRUE;
}


static void
dispose (GObject* obj)
{
	gdb_plugin = NULL; /* TODO: to be removed ? */
}


static void
gdb_plugin_instance_init (GObject* obj)
{
	GdbPlugin *plugin = (GdbPlugin *) obj;
	gdb_plugin = plugin; /* TODO: to be removed ? */
	plugin->uiid = 0;
	plugin->launcher = NULL;
}


static void
gdb_plugin_class_init (GObjectClass* klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
}


/* Implementation of IAnjutaDebuggerBE interface */
static void
idebugger_be_start (IAnjutaDebuggerBE *plugin, const gchar *prog, GError **err)
{
	debugger_start (prog);
}

static void
idebugger_be_iface_init (IAnjutaDebuggerBEIface *iface)
{
	iface->start = idebugger_be_start;
}

ANJUTA_PLUGIN_BEGIN (GdbPlugin, gdb_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE(idebugger_be, IANJUTA_TYPE_DEBUGGER_BE);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (GdbPlugin, gdb_plugin);
