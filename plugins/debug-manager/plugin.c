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
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-file.h>
//#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-debugger-manager.h>
#include <libanjuta/interfaces/ianjuta-debugger.h>

#include "plugin.h"

#include "attach_process.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-debug-manager.ui"

static gpointer parent_class;


static void on_load_file_response_ok (GtkDialog* dialog, gint id, DebugManagerPlugin* plugin);


static void
load_file (DebugManagerPlugin* plugin, gboolean executable)
{
	const gchar *title_executable = _("Load Executable File");
	const gchar *title_core = _("Load Core File");

	plugin->is_executable = executable;

	GtkWindow *parent = GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell);
	GtkWidget *dialog = gtk_file_chooser_dialog_new (
			executable ? title_executable : title_core,
			parent,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			NULL);
	gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER(dialog), FALSE);
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (dialog), TRUE);

	GtkFileFilter *filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("All files"));
	gtk_file_filter_add_pattern (filter, "*");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

	g_signal_connect (G_OBJECT (dialog), "response",
			G_CALLBACK (on_load_file_response_ok), plugin);
	g_signal_connect_swapped (GTK_OBJECT (dialog), "response",
			G_CALLBACK (gtk_widget_destroy), GTK_OBJECT (dialog));

	gtk_widget_show (dialog);
}

static IAnjutaDebugger *
get_anjuta_debugger_iface (DebugManagerPlugin* plugin)
{
	GObject *obj;
	IAnjutaDebugger *debugger;

	/* Query for object implementing IAnjutaDebugger interface */
	obj = anjuta_shell_get_object (ANJUTA_PLUGIN (plugin)->shell,
			"IAnjutaDebugger", NULL);
	debugger = IANJUTA_DEBUGGER (obj);

	return debugger;
}

static void
on_load_file_response_ok (GtkDialog* dialog, gint id, DebugManagerPlugin* plugin)
{
	gchar *filename;
	IAnjutaDebugger *debugger;

	if (plugin->uri != NULL)
	{
		/* TODO - somehow handle situation when an executable has already been loaded */
		return;
	}

	plugin->uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));

	gtk_widget_destroy (GTK_WIDGET (dialog));

	debugger = get_anjuta_debugger_iface (plugin);

	filename = g_filename_from_uri (plugin->uri, NULL, NULL);
	if (plugin->is_executable)
	{
		ianjuta_debugger_load_executable (debugger, filename, NULL /* TODO */);
	}
	else
	{
		ianjuta_debugger_load_core (debugger, filename, NULL /* TODO */);
	}
	g_free (filename);
}

static void
on_start_debug_activate (GtkAction* action, DebugManagerPlugin* plugin)
{
	IAnjutaDebugger *debugger = get_anjuta_debugger_iface (plugin);

	g_return_if_fail (debugger != NULL);

	ianjuta_debugger_start (debugger, "" /* TODO */, NULL /* TODO */);
}

static void
on_load_exec_action_activate (GtkAction* action, DebugManagerPlugin* plugin)
{
	load_file (plugin, TRUE);
}

static void
on_load_core_action_activate (GtkAction* action, DebugManagerPlugin* plugin)
{
	load_file (plugin, FALSE);
}

static void
on_attach_to_project_action_activate (GtkAction* action, DebugManagerPlugin* plugin)
{
	/* TODO: fix the below */
	attach_process_show (attach_process_new());
}

static void
on_run_continue_action_activate (GtkAction* action, DebugManagerPlugin* plugin)
{
	IAnjutaDebugger *debugger = get_anjuta_debugger_iface (plugin);

	g_return_if_fail (debugger != NULL);

	ianjuta_debugger_run_continue (debugger, NULL /* TODO */);
}

static void
on_step_in_action_activate (GtkAction* action, DebugManagerPlugin* plugin)
{
	IAnjutaDebugger *debugger = get_anjuta_debugger_iface (plugin);

	g_return_if_fail (debugger != NULL);

	ianjuta_debugger_step_in (debugger, NULL /* TODO */);
}

static void
on_step_over_action_activate (GtkAction* action, DebugManagerPlugin* plugin)
{
	IAnjutaDebugger *debugger = get_anjuta_debugger_iface (plugin);

	g_return_if_fail (debugger != NULL);

	ianjuta_debugger_step_over (debugger, NULL /* TODO */);
}

static void
on_step_out_action_activate (GtkAction* action, DebugManagerPlugin* plugin)
{
	IAnjutaDebugger *debugger = get_anjuta_debugger_iface (plugin);

	g_return_if_fail (debugger != NULL);

	ianjuta_debugger_step_out (debugger, NULL /* TODO */);
}


static void
on_toggle_breakpoint_action_activate (GtkAction* action, DebugManagerPlugin* plugin)
{
	IAnjutaDebugger *debugger = get_anjuta_debugger_iface (plugin);

	g_return_if_fail (debugger != NULL);

	ianjuta_debugger_toggle_breakpoint (debugger, NULL /* TODO */);
}


static GtkActionEntry actions_debug[] =
{
	{
		"ActionMenuDebug",                        /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("_Debug"),                             /* Display label */
		NULL,                                     /* short-cut */
		NULL,                                     /* Tooltip */
		NULL                                      /* action callback */
	},
	{
		"ActionStartDebugger",                    /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("_Start Debugger"),                    /* Display label */
		"<shift>F12",                             /* short-cut */
		N_("Start the debugging session"),        /* Tooltip */
		G_CALLBACK (on_start_debug_activate)      /* action callback */
	},
	{
		"ActionLoadExecutable",                   /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("Load E_xecutable ..."),               /* Display label */
		NULL,                                     /* short-cut */
		N_("Open the executable for debugging"),  /* Tooltip */
		G_CALLBACK (on_load_exec_action_activate) /* action callback */
	},
	{
		"ActionLoadCore",                         /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("Load _Core file ..."),                /* Display label */
		NULL,                                     /* short-cut */
		N_("Load a core file to dissect"),        /* Tooltip */
		G_CALLBACK (on_load_core_action_activate) /* action callback */
	},
	{
		"ActionAttachToProcess",                  /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("_Attach to Process ..."),             /* Display label */
		NULL,                                     /* short-cut */
		N_("Attach to a running program"),        /* Tooltip */
		G_CALLBACK (on_attach_to_project_action_activate) /* action callback */
	},
	{
		"ActionMenuExecution",                    /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("_Execution"),                         /* Display label */
		NULL,                                     /* short-cut */
		NULL,                                     /* Tooltip */
		NULL                                      /* action callback */
	},
	{
		"ActionRunContinue",                      /* Action name */
		GTK_STOCK_EXECUTE,                        /* Stock icon, if any */
		N_("Run/_Continue"),                      /* Display label */
		"F4",                                     /* short-cut */
		N_("Continue the execution of the program"), /* Tooltip */
		G_CALLBACK (on_run_continue_action_activate) /* action callback */
	},
	{
		"ActionStepIn",                           /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("Step _In"),                           /* Display label */
		"F5",                                     /* short-cut */
		N_("Single step into function"),          /* Tooltip */
		G_CALLBACK (on_step_in_action_activate)   /* action callback */
	},
	{
		"ActionStepOver",                         /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("Step O_ver"),                         /* Display label */
		"F6",                                     /* short-cut */
		N_("Single step over function"),          /* Tooltip */
		G_CALLBACK (on_step_over_action_activate) /* action callback */
	},
	{
		"ActionStepOut",                          /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("Step _Out"),                          /* Display label */
		"F7",                                     /* short-cut */
		N_("Single step out of the function"),    /* Tooltip */
		G_CALLBACK (on_step_out_action_activate)  /* action callback */
	},
	{
		"ActionMenuBreakpoints",                  /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("_Breakpoints"),                       /* Display label */
		NULL,                                     /* short-cut */
		NULL,                                     /* Tooltip */
		NULL                                      /* action callback */
	},
	{
		"ActionToggleBreakpoint",                 /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("Toggle breakpoint"),                  /* Display label */
		NULL,                                     /* short-cut */
		N_("Toggle breakpoint at the current location"), /* Tooltip */
		G_CALLBACK (on_toggle_breakpoint_action_activate) /* action callback */
	}
};

static gboolean
activate_plugin (AnjutaPlugin* plugin)
{
	AnjutaUI *ui;
	DebugManagerPlugin *debug_manager_plugin;

	DEBUG_PRINT ("DebugManagerPlugin: Activating Debug Manager plugin ...");
	debug_manager_plugin = (DebugManagerPlugin*) plugin;

	ui = anjuta_shell_get_ui (plugin->shell, NULL);

	/* Add all our debug manager actions */
	debug_manager_plugin->action_group =
		anjuta_ui_add_action_group_entries (ui, "ActionGroupDebug",
											_("Debugger operations"),
											actions_debug,
											G_N_ELEMENTS (actions_debug),
											plugin);
	debug_manager_plugin->uiid = anjuta_ui_merge (ui, UI_FILE);
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin* plugin)
{
	DebugManagerPlugin *dplugin;
	AnjutaUI *ui;

	dplugin = (DebugManagerPlugin *) plugin;
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	DEBUG_PRINT ("DebugManagerPlugin: Deactivating Debug Manager plugin ...");
	anjuta_ui_unmerge (ui, dplugin->uiid);
	anjuta_ui_remove_action_group (ui, dplugin->action_group);
	dplugin->uiid = 0;
	dplugin->action_group = NULL;
	return TRUE;
}

static void
dispose (GObject* obj)
{
	DebugManagerPlugin *plugin = (DebugManagerPlugin *) obj;
	if (plugin->uri)
	{
		g_free (plugin->uri);
		plugin->uri = NULL;
	}
}

static void
debug_manager_plugin_instance_init (GObject* obj)
{
	DebugManagerPlugin *plugin = (DebugManagerPlugin *) obj;
	plugin->uiid = 0;
	plugin->uri = NULL;
	plugin->is_executable = FALSE;
}

static void
debug_manager_plugin_class_init (GObjectClass* klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
}


/* Implementation of IAnjutaDebuggerManager interface */
static gboolean
idebugger_manager_is_debugger_active (IAnjutaDebuggerManager *debugman,
		GError** e)
{
	DebugManagerPlugin *plugin = (DebugManagerPlugin *) debugman;
	IAnjutaDebugger *debugger = get_anjuta_debugger_iface (plugin);

	g_return_val_if_fail (debugger != NULL, FALSE);

	return ianjuta_debugger_is_active (debugger, NULL /* TODO */);
}

static void
idebugger_manager_toggle_breakpoint (IAnjutaDebuggerManager *debugman,
		gint line, GError** e)
{
	DebugManagerPlugin *plugin = (DebugManagerPlugin *) debugman;
	IAnjutaDebugger *debugger = get_anjuta_debugger_iface (plugin);

	g_return_if_fail (debugger != NULL);

	return ianjuta_debugger_toggle_breakpoint1 (debugger, line,
			NULL /* TODO */);
}

static void
idebugger_manager_iface_init (IAnjutaDebuggerManagerIface *iface)
{
	iface->is_debugger_active = idebugger_manager_is_debugger_active;
	iface->toggle_breakpoint = idebugger_manager_toggle_breakpoint;
}


/* Implementation of IAnjutaFile interface */
static void
ifile_open (IAnjutaFile* plugin, const gchar* uri, GError** e)
{
	DebugManagerPlugin *debug_manager = (DebugManagerPlugin *) plugin;
	if (debug_manager->uri == NULL)
	{
		debug_manager->uri = g_strdup (uri);
		/* TODO: this doesn't seem to work :-[ */
		anjuta_util_dialog_info (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
				"Opening executable %s\n", uri);
	}
	else
	{
		/* TODO: error message */
	}
}

static gchar*
ifile_get_uri (IAnjutaFile* plugin, GError** e)
{
	return ((DebugManagerPlugin *) plugin)->uri;
}

static void
ifile_iface_init (IAnjutaFileIface* iface)
{
	iface->open = ifile_open;
	iface->get_uri = ifile_get_uri;
}


ANJUTA_PLUGIN_BEGIN (DebugManagerPlugin, debug_manager_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE(idebugger_manager, IANJUTA_TYPE_DEBUGGER_MANAGER);
ANJUTA_PLUGIN_ADD_INTERFACE(ifile, IANJUTA_TYPE_FILE);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (DebugManagerPlugin, debug_manager_plugin);
