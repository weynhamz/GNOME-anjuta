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
#include <libanjuta/interfaces/ianjuta-debugger.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-message-manager.h>
#include <libanjuta/interfaces/ianjuta-message-view.h>

#include "plugin.h"
#include "debugger.h"

#define ICON_FILE "anjuta-gdb.plugin.png"

static gpointer parent_class;

static const gchar * MESSAGE_VIEW_TITLE = N_("Debug");

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
	/* TODO */
}


static gboolean
activate_plugin (AnjutaPlugin* plugin)
{
	GObject *obj;
	IAnjutaMessageManager *message_manager;
	IAnjutaMessageView *message_view;
	GdbPlugin *gdb_plugin = (GdbPlugin *) plugin;

	DEBUG_PRINT ("GdbPlugin: Activating Gdb plugin ...");

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
	debugger_init (gdb_plugin);

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
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}


static void
gdb_plugin_instance_init (GObject* obj)
{
	GdbPlugin *plugin = (GdbPlugin *) obj;
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


/* Implementation of IAnjutaDebugger interface */
static gboolean
idebugger_is_active (IAnjutaDebugger *plugin, GError **err)
{
	return debugger_is_active ();
}

static void
idebugger_start (IAnjutaDebugger *plugin, const gchar *prog, GError **err)
{
	debugger_start (prog);
}

static void
idebugger_load_executable (IAnjutaDebugger *plugin, const gchar *prog, GError **err)
{
	debugger_load_executable (prog);
}

static void
idebugger_load_core (IAnjutaDebugger *plugin, const gchar *core, GError **err)
{
	debugger_load_core (core);
}

static void
idebugger_run_continue (IAnjutaDebugger *plugin, GError **err)
{
	debugger_run ();
}

static void
idebugger_step_in (IAnjutaDebugger *plugin, GError **err)
{
	debugger_step_in ();
}

static void
idebugger_step_over (IAnjutaDebugger *plugin, GError **err)
{
	debugger_step_over ();
}

static void
idebugger_step_out (IAnjutaDebugger *plugin, GError **err)
{
	debugger_step_out ();
}

static void
idebugger_toggle_breakpoint (IAnjutaDebugger *plugin, GError **err)
{
	debugger_toggle_breakpoint (0); // 0 means current line
}

static void
idebugger_toggle_breakpoint1 (IAnjutaDebugger *plugin, gint line, GError **err)
{
	debugger_toggle_breakpoint (line);
}

static void
idebugger_iface_init (IAnjutaDebuggerIface *iface)
{
	iface->is_active = idebugger_is_active;
	iface->start = idebugger_start;
	iface->load_executable = idebugger_load_executable;
	iface->load_core = idebugger_load_core;
	iface->run_continue = idebugger_run_continue;
	iface->step_in = idebugger_step_in;
	iface->step_over = idebugger_step_over;
	iface->step_out = idebugger_step_out;
	iface->toggle_breakpoint = idebugger_toggle_breakpoint;
	iface->toggle_breakpoint1 = idebugger_toggle_breakpoint1;
}

ANJUTA_PLUGIN_BEGIN (GdbPlugin, gdb_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE(idebugger, IANJUTA_TYPE_DEBUGGER);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (GdbPlugin, gdb_plugin);
