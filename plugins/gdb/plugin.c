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
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>

#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-debugger.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-message-manager.h>
#include <libanjuta/interfaces/ianjuta-message-view.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>

#include "plugin.h"
#include "debugger.h"
#include "actions.h"
#include "utilities.h"

#define ICON_FILE "anjuta-gdb.plugin.png"
#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-gdb-plugin.ui"

static gpointer parent_class;

/* GDB actions */

static GtkActionEntry actions_gdb[] =
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
		"ActionMenuGdbBreakpoints",               /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("_Breakpoints"),                       /* Display label */
		NULL,                                     /* short-cut */
		NULL,                                     /* Tooltip */
		NULL                                      /* action callback */
	},
	{
		"ActionGdbToggleBreakpoint",              /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("Toggle breakpoint"),                  /* Display label */
		NULL,                                     /* short-cut */
		N_("Toggle breakpoint at the current location"), /* Tooltip */
		G_CALLBACK (on_toggle_breakpoint_activate) /* action callback */
	},
	{
		"ActionGdbSetBreakpoint",                 /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("Set Breakpoint ..."),                 /* Display label */
		NULL,                                     /* short-cut */
		N_("Set a breakpoint"),                   /* Tooltip */
		G_CALLBACK (on_set_breakpoint_activate)   /* action callback */
	},
	{
		"ActionGdbBreakpoints",                   /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("_Breakpoints ..."),                   /* Display label */
		NULL,                                     /* short-cut */
		N_("Edit breakpoints"),                   /* Tooltip */
		G_CALLBACK (on_show_breakpoints_activate) /* action callback */
	},
	{
		"ActionGdbDisableAllBreakpoints",         /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("Disable all Breakpoints"),            /* Display label */
		NULL,                                     /* short-cut */
		N_("Deactivate all breakpoints"),         /* Tooltip */
		G_CALLBACK (on_disable_all_breakpoints_activate)/* action callback */
	},
	{
		"ActionGdbClearAllBreakpoints",           /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("C_lear all Breakpoints"),             /* Display label */
		NULL,                                     /* short-cut */
		N_("Delete all breakpoints"),             /* Tooltip */
		G_CALLBACK (on_clear_breakpoints_activate)/* action callback */
	},
	{
		"ActionMenuGdbInformation",
		NULL,
		N_("_Info"),
		NULL,
		NULL,
		NULL
	},
	{
		"ActionGdbInfoTargetFiles",
		NULL,
		N_("Info _Target Files"),
		NULL,
		N_("Display information on the files the debugger is active with"),
		G_CALLBACK (on_info_targets_activate)
	},
	{
		"ActionGdbInfoProgram",
		NULL,
		N_("Info _Program"),
		NULL,
		N_("Display information on the execution status of the program"),
		G_CALLBACK (on_info_program_activate)
	},
	{
		"ActionGdbInfoKernelUserStruct",
		NULL,
		N_("Info _Kernel User Struct"),
		NULL,
		N_("Display the contents of kernel 'struct user' for current child"),
		G_CALLBACK (on_info_udot_activate)
	},
	{
		"ActionGdbInfoThreads",
		NULL,
		N_("Info _Threads"),
		NULL,
		N_("Display the IDs of currently known threads"),
		G_CALLBACK (on_info_threads_activate)
	},
	{
		"ActionGdbInfoGlobalVariables",
		NULL,
		N_("Info _Global variables"),
		NULL,
		N_("Display all global and static variables of the program"),
		G_CALLBACK (on_info_variables_activate)
	},
	{
		"ActionGdbInfoLocalVariables",
		NULL,
		N_("Info _Local variables"),
		NULL,
		N_("Display local variables of the current frame"),
		G_CALLBACK (on_info_locals_activate)
	},
	{
		"ActionGdbInfoCurrentFrame",
		NULL,
		N_("Info _Current Frame"),
		NULL,
		N_("Display information about the current frame of execution"),
		G_CALLBACK (on_info_frame_activate)
	},
	{
		"ActionGdbInfoFunctionArgs",
		NULL,
		N_("Info Function _Arguments"),
		NULL,
		N_("Display function arguments of the current frame"),
		G_CALLBACK (on_info_args_activate)
	},
	{
		"ActionGdbExamineMemory",
		NULL,
		N_("Examine _Memory"),
		NULL,
		N_("Display accessible memory"),
		G_CALLBACK (on_info_memory_activate)
	},
	{
		"ActionGdbRestartProgram",
		GTK_STOCK_REFRESH,
		N_("_Restart Program"),
		NULL,
		N_("Stop and restart the program"),
		G_CALLBACK (on_debugger_restart_prog_activate)
	},
	{
		"ActionGdbStopProgram",
		GTK_STOCK_STOP,
		N_("S_top Program"),
		NULL,
		N_("Stop the program being debugged"),
		G_CALLBACK (on_debugger_stop_prog_activate)
	},
	{
		"ActionGdbDetachProgram",
		NULL,
		N_("_Detach Debugger"),
		NULL,
		N_("Detach from an attached program"),
		G_CALLBACK (on_debugger_detach_activate)
	},
	{
		"ActionGdbPauseProgram",
		NULL,
		N_("Pa_use Program"),
		NULL,
		N_("Pauses the execution of the program"),
		G_CALLBACK (on_debugger_interrupt_activate)
	},
	{
		"ActionGdbSignalProgram",
		NULL,
		N_("Si_gnal to Process"),
		NULL,
		N_("Send a kernel signal to the process being debugged"),
		G_CALLBACK (on_debugger_signal_activate)
	},
	{
		"ActionGdbInspect",
		GTK_STOCK_DIALOG_INFO,
		N_("Ins_pect/Evaluate ..."),
		NULL,
		N_("Inspect or evaluate an expression or variable"),
		G_CALLBACK (on_debugger_inspect_activate)
	},
	{
		"ActionGdbAddWatch",
		NULL,
		N_("Add Expression in _Watch ..."),
		NULL,
		N_("Add expression or variable to the watch"),
		G_CALLBACK (on_debugger_add_watch_activate)
	},
	{
		"ActionGdbCommand",
		NULL,
		N_("Debugger command ..."),
		NULL,
		N_("Custom debugger command"),
		G_CALLBACK (on_debugger_custom_command_activate)
	},
	{
		"ActionGdbViewRegisters",
		NULL,
		N_("Registers ..."),
		NULL,
		N_("Show CPU register contents"),
		G_CALLBACK (on_debugger_registers_activate)
	},
	{
		"ActionGdbViewSharedlibs",
		NULL,
		N_("Shared Libraries ..."),
		NULL,
		N_("Show shared libraries mappings"),
		G_CALLBACK (on_debugger_sharedlibs_activate)
	},
	{
		"ActionGdbViewSignals",
		NULL,
		N_("Kernel Signals ..."),
		NULL,
		N_("Show kernel signals"),
		G_CALLBACK (on_debugger_signals_activate)
	}
};

#define REGISTER_ICON(icon, stock_id) \
	pixbuf = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"icon, NULL); \
	icon_set = gtk_icon_set_new_from_pixbuf (pixbuf); \
	gtk_icon_factory_add (icon_factory, stock_id, icon_set); \
	g_object_unref (pixbuf);
#if 0
static void
register_stock_icons (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	GtkIconFactory *icon_factory;
	GtkIconSet *icon_set;
	GdkPixbuf *pixbuf;
	static gboolean registered = FALSE;

	if (registered)
		return;
	registered = TRUE;

	/* Register stock icons */
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	icon_factory = anjuta_ui_get_icon_factory (ui);
	REGISTER_ICON (GDB_PIXMAP_STACK_ICON, "gdb-stack-icon");
	REGISTER_ICON (GDB_PIXMAP_WATCH_ICON, "gdb-locals-icon");
	REGISTER_ICON (GDB_PIXMAP_LOCALS_ICON, "gdb-watch-icon");
}
#endif

static void
value_added_project_root_uri (AnjutaPlugin *plugin, const gchar *name,
							  const GValue *value, gpointer user_data)
{
	GdbPlugin *gdb_plugin;
	const gchar *root_uri;

	gdb_plugin = (GdbPlugin *) plugin;
	
	if (gdb_plugin->project_root_uri)
		g_free (gdb_plugin->project_root_uri);
	gdb_plugin->project_root_uri = NULL;
	
	root_uri = g_value_get_string (value);
	if (root_uri)
	{
		gdb_plugin->project_root_uri = g_strdup (root_uri);
	}
}

static void
value_removed_project_root_uri (AnjutaPlugin *plugin, const gchar *name,
								gpointer user_data)
{
	GdbPlugin *gdb_plugin;

	gdb_plugin = (GdbPlugin *) plugin;
	
	if (gdb_plugin->project_root_uri)
		g_free (gdb_plugin->project_root_uri);
	gdb_plugin->project_root_uri = NULL;
}

static void
value_added_current_editor (AnjutaPlugin *plugin, const char *name,
							const GValue *value, gpointer data)
{
	GObject *editor;
	GdbPlugin *gdb_plugin;
	
	editor = g_value_get_object (value);
	gdb_plugin = (GdbPlugin*)plugin;
	gdb_plugin->current_editor = editor;
	
	/* Restore breakpoints */
	if (gdb_plugin->breakpoints)
		breakpoints_dbase_set_all_in_editor (gdb_plugin->breakpoints,
											 IANJUTA_EDITOR (editor));
}

static void
value_removed_current_editor (AnjutaPlugin *plugin,
							  const char *name, gpointer data)
{
	GdbPlugin *gdb_plugin = (GdbPlugin*)plugin;
	if (gdb_plugin->breakpoints && gdb_plugin->current_editor)
		breakpoints_dbase_clear_all_in_editor (gdb_plugin->breakpoints,
											   gdb_plugin->current_editor);
	gdb_plugin->current_editor = NULL;
}

void
gdb_plugin_update_ui (GdbPlugin *plugin)
{
	AnjutaUI *ui;
	GtkAction *action;
	gboolean DR, Pr, PrA;
	
	DR = TRUE;
	Pr = PrA = FALSE;
	
	if (plugin->debugger != NULL)
	{
		DR = debugger_is_ready (plugin->debugger);
		Pr = debugger_program_is_running (plugin->debugger);
		PrA = debugger_program_is_attached (plugin->debugger);
	}
	
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (plugin)->shell, NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupGdb",
								   "ActionGdbRestartProgram");
	g_object_set (G_OBJECT (action), "sensitive", Pr && !PrA, NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupGdb",
								   "ActionGdbStopProgram");
	g_object_set (G_OBJECT (action), "sensitive", Pr && !PrA, NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupGdb",
								   "ActionGdbDetachProgram");
	g_object_set (G_OBJECT (action), "sensitive", DR && Pr && PrA, NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupGdb",
								   "ActionGdbBreakpoints");
	g_object_set (G_OBJECT (action), "sensitive", DR, NULL);

	action = anjuta_ui_get_action (ui, "ActionGroupGdb",
								   "ActionGdbSetBreakpoint");
	g_object_set (G_OBJECT (action), "sensitive", DR, NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupGdb",
								   "ActionGdbClearAllBreakpoints");
	g_object_set (G_OBJECT (action), "sensitive", DR, NULL);

	action = anjuta_ui_get_action (ui, "ActionGroupGdb",
								   "ActionGdbDisableAllBreakpoints");
	g_object_set (G_OBJECT (action), "sensitive", DR, NULL);

	action = anjuta_ui_get_action (ui, "ActionGroupGdb",
								   "ActionGdbAddWatch");
	g_object_set (G_OBJECT (action), "sensitive", DR, NULL);

	action = anjuta_ui_get_action (ui, "ActionGroupGdb",
								   "ActionGdbInspect");
	g_object_set (G_OBJECT (action), "sensitive", DR, NULL);

	action = anjuta_ui_get_action (ui, "ActionGroupGdb",
								   "ActionGdbInspect");
	g_object_set (G_OBJECT (action), "sensitive", DR, NULL);

	action = anjuta_ui_get_action (ui, "ActionGroupGdb",
								   "ActionGdbInfoTargetFiles");
	g_object_set (G_OBJECT (action), "sensitive", DR, NULL);

	action = anjuta_ui_get_action (ui, "ActionGroupGdb",
								   "ActionGdbInfoProgram");
	g_object_set (G_OBJECT (action), "sensitive", DR, NULL);

	action = anjuta_ui_get_action (ui, "ActionGroupGdb",
								   "ActionGdbInfoKernelUserStruct");
	g_object_set (G_OBJECT (action), "sensitive", DR, NULL);

	action = anjuta_ui_get_action (ui, "ActionGroupGdb",
								   "ActionGdbInfoGlobalVariables");
	g_object_set (G_OBJECT (action), "sensitive", DR, NULL);

	action = anjuta_ui_get_action (ui, "ActionGroupGdb",
								   "ActionGdbInfoLocalVariables");
	g_object_set (G_OBJECT (action), "sensitive", DR, NULL);

	action = anjuta_ui_get_action (ui, "ActionGroupGdb",
								   "ActionGdbInfoCurrentFrame");
	g_object_set (G_OBJECT (action), "sensitive", DR, NULL);

	action = anjuta_ui_get_action (ui, "ActionGroupGdb",
								   "ActionGdbExamineMemory");
	g_object_set (G_OBJECT (action), "sensitive", DR, NULL);

	action = anjuta_ui_get_action (ui, "ActionGroupGdb",
								   "ActionGdbInfoFunctionArgs");
	g_object_set (G_OBJECT (action), "sensitive", DR, NULL);

	action = anjuta_ui_get_action (ui, "ActionGroupGdb",
								   "ActionGdbInfoThreads");
	g_object_set (G_OBJECT (action), "sensitive", DR, NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupGdb",
								   "ActionGdbSignalProgram");
	g_object_set (G_OBJECT (action), "sensitive", Pr, NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupGdb",
								   "ActionGdbPauseProgram");
	g_object_set (G_OBJECT (action), "sensitive", !DR && Pr, NULL);
#if 0
	gtk_widget_set_sensitive (dm->start_debug, !L);
	gtk_widget_set_sensitive (dm->open_exec, A && R);
	gtk_widget_set_sensitive (dm->load_core, A && R);
	gtk_widget_set_sensitive (dm->attach, A && R);
	
	gtk_widget_set_sensitive (dm->cont, A && R && Ld);
	gtk_widget_set_sensitive (dm->run_to_cursor, A && R && Ld);
	gtk_widget_set_sensitive (dm->step_in, A && R && Ld);
	gtk_widget_set_sensitive (dm->step_over, A && R && Pr);
	gtk_widget_set_sensitive (dm->step_out, A && R && Pr);
	gtk_widget_set_sensitive (dm->tog_break, A && R && Ld);
#endif
}

static void
on_output_arrived (Debugger *debugger, DebuggerOutputType type,
				   const gchar *msg, gpointer user_data)
{
	gdb_util_append_message (ANJUTA_PLUGIN (user_data), msg);
}

static gboolean
gdb_debugger_is_active (GdbPlugin *plugin)
{
	return (plugin->debugger != NULL);
}

static void
on_location_changed (Debugger* debugger, const gchar *file, gint line,
					 const gchar *address, AnjutaPlugin *plugin)
{
	IAnjutaDocumentManager *docman = NULL;
	gchar *msg, *file_uri;
	
	msg = g_strdup_printf (_("Location: %s, line %d\n"), file, line);
	gdb_util_append_message (plugin, msg);
	g_free (msg);
	
	docman = anjuta_shell_get_interface (plugin->shell,
										 IAnjutaDocumentManager, NULL);
	file_uri = g_strconcat ("file://", file, NULL);
	if (docman)
		ianjuta_document_manager_goto_file_line (docman, file_uri,
												 line, NULL);
	g_free (file_uri);
}

static void
on_program_running (Debugger* debugger, GdbPlugin *plugin)
{
	g_signal_emit_by_name (plugin, "busy", TRUE);
	gdb_plugin_update_ui (plugin);
}

static void
on_program_exited (Debugger* debugger, GDBMIValue *value, GdbPlugin *plugin)
{
	g_signal_emit_by_name (plugin, "busy", FALSE);
	gdb_plugin_update_ui (plugin);
}

static void
on_program_stopped (Debugger* debugger, GDBMIValue *value, GdbPlugin *plugin)
{
	g_signal_emit_by_name (plugin, "busy", FALSE);
	gdb_plugin_update_ui (plugin);
}

static void
gdb_initialize_debugger (GdbPlugin *plugin, const gchar *prog,
						 gboolean is_libtool_target,
						 const GList *search_dir_uris)
{
	GtkWindow *parent;
	const GList *node;
	GList *search_dirs = NULL;
	
	g_return_if_fail (plugin->debugger == NULL);
	
	/* Convert search dirs to local paths, and reverse them */
	node = search_dir_uris;
	while (node)
	{
		gchar *local_path;
		local_path = gnome_vfs_get_local_path_from_uri (node->data);
		if (local_path)
			search_dirs = g_list_prepend (search_dirs, local_path);
		node = g_list_next (node);
	}
	search_dirs = g_list_reverse (search_dirs);
	
	parent = GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell);
	if (prog == NULL)
	{
		plugin->debugger = debugger_new (parent, search_dirs,
										 on_output_arrived, plugin);
	}
	else
	{
		plugin->debugger = debugger_new_with_program (parent, search_dirs,
													  on_output_arrived,
													  plugin, prog,
													  is_libtool_target);
	}
	
	g_list_foreach (search_dirs, (GFunc)g_free, NULL);
	g_list_free (search_dirs);
	
	g_signal_connect (plugin->debugger, "location-changed",
					  G_CALLBACK (on_location_changed), plugin);
	g_signal_connect (plugin->debugger, "program-running",
					  G_CALLBACK (on_program_running), plugin);
	g_signal_connect (plugin->debugger, "program-exited",
					  G_CALLBACK (on_program_exited), plugin);
	g_signal_connect (plugin->debugger, "program-stopped",
					  G_CALLBACK (on_program_stopped), plugin);
	
	plugin->watch = expr_watch_new (plugin->debugger);
	anjuta_shell_add_widget (ANJUTA_PLUGIN (plugin)->shell,
							 plugin->watch->widgets.scrolledwindow,
							 "AnjutaDebuggerWatch", _("Watches"),
							 "gdb-watch-icon", ANJUTA_SHELL_PLACEMENT_BOTTOM,
							 NULL);
	plugin->locals = locals_create (plugin->debugger);
	anjuta_shell_add_widget (ANJUTA_PLUGIN (plugin)->shell,
							 locals_get_main_widget (plugin->locals),
							 "AnjutaDebuggerLocals", _("Locals"),
							 "gdb-locals-icon", ANJUTA_SHELL_PLACEMENT_BOTTOM,
							 NULL);
	plugin->stack = stack_trace_new (plugin->debugger);
	anjuta_shell_add_widget (ANJUTA_PLUGIN (plugin)->shell,
							 stack_trace_get_main_widget (plugin->stack),
							 "AnjutaDebuggerStack", _("Stack"),
							 "gdb-stack-icon", ANJUTA_SHELL_PLACEMENT_BOTTOM,
							 NULL);
	plugin->registers = cpu_registers_new (plugin->debugger);
	plugin->sharedlibs = sharedlibs_new (plugin->debugger);
	plugin->signals = signals_new (plugin->debugger);
	plugin->breakpoints = breakpoints_dbase_new (ANJUTA_PLUGIN (plugin),
												 plugin->debugger);
	
	g_signal_emit_by_name (plugin, "busy", FALSE);
}

static gboolean
gdb_plugin_activate_plugin (AnjutaPlugin* plugin)
{
	AnjutaUI *ui;
	GdbPlugin *gdb_plugin = (GdbPlugin *) plugin;

	DEBUG_PRINT ("GdbPlugin: Activating Gdb plugin ...");

	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	/* Add action group */
	gdb_plugin->action_group = 
		anjuta_ui_add_action_group_entries (ui,
											"ActionGroupGdb",
											_("Gdb (debugger) commands"),
											actions_gdb,
											sizeof(actions_gdb)/sizeof(GtkActionEntry),
											plugin);
	/* Add UI */
	gdb_plugin->merge_id = anjuta_ui_merge (ui, UI_FILE);
	
	/* Add watches */
	gdb_plugin->project_watch_id = 
		anjuta_plugin_add_watch (plugin, "project_root_uri",
								 value_added_project_root_uri,
								 value_removed_project_root_uri, NULL);
	gdb_plugin->editor_watch_id = 
		anjuta_plugin_add_watch (plugin, "document_manager_current_editor",
								 value_added_current_editor,
								 value_removed_current_editor, NULL);
	gdb_plugin_update_ui (gdb_plugin);
	gdb_util_clear_messages (plugin);
	gdb_util_show_messages (plugin);
	return TRUE;
}

static gboolean
gdb_plugin_deactivate_plugin (AnjutaPlugin* plugin)
{
	GdbPlugin *gdb_plugin;
	AnjutaUI *ui;
	
	DEBUG_PRINT ("GdbPlugin: Deactivating Gdb plugin ...");

	gdb_plugin = (GdbPlugin*)plugin;
	
	/* Unmerge UI */
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	anjuta_ui_unmerge (ui, gdb_plugin->merge_id);
	
	/* Remove watches */
	anjuta_plugin_remove_watch (plugin, gdb_plugin->project_watch_id, TRUE);
	anjuta_plugin_remove_watch (plugin, gdb_plugin->editor_watch_id, TRUE);
	
	/* Views were removed from shell when they are destroyed */
	if (gdb_plugin->debugger)
	{
		g_object_unref (gdb_plugin->debugger);
		expr_watch_destroy (gdb_plugin->watch);
		locals_destroy (gdb_plugin->locals);
		stack_trace_destroy (gdb_plugin->stack);
		cpu_registers_destroy (gdb_plugin->registers);
		sharedlibs_destroy (gdb_plugin->sharedlibs);
		signals_destroy (gdb_plugin->signals);
		breakpoints_dbase_destroy (gdb_plugin->breakpoints);
		
		gdb_plugin->debugger = NULL;
		gdb_plugin->watch = NULL;
		gdb_plugin->locals = NULL;
		gdb_plugin->stack = NULL;
		gdb_plugin->registers = NULL;
		gdb_plugin->sharedlibs = NULL;
		gdb_plugin->signals = NULL;
		gdb_plugin->breakpoints = NULL;
	}
	
	/* Remvove action groups */
	anjuta_ui_remove_action_group (ui, gdb_plugin->action_group);

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
	plugin->debugger = NULL;
	plugin->uiid = 0;
	plugin->mesg_view = NULL;
	plugin->merge_id = 0;
	plugin->editor_watch_id = 0;
	plugin->project_watch_id = 0;
	
	plugin->current_editor = NULL;
	plugin->project_root_uri = NULL;
	plugin->watch = NULL;
	plugin->locals = NULL;
}

static void
gdb_plugin_class_init (GObjectClass* klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = gdb_plugin_activate_plugin;
	plugin_class->deactivate = gdb_plugin_deactivate_plugin;
	klass->dispose = dispose;
}

/* Implementation of IAnjutaDebugger interface */
static gboolean
idebugger_is_busy (IAnjutaDebugger *plugin, GError **err)
{
	GdbPlugin *gdb_plugin = (GdbPlugin*)G_OBJECT (plugin);
	if (gdb_plugin->debugger == NULL)
		return TRUE;
	return (!debugger_is_ready (gdb_plugin->debugger));
}

static void
idebugger_load (IAnjutaDebugger *plugin, const gchar *prog_uri,
				const GList *search_directories, GError **err)
{
	GdbPlugin *gdb_plugin;
	GnomeVFSURI *vfs_uri;

	gdb_plugin = (GdbPlugin*)G_OBJECT (plugin);
	
	if (gdb_debugger_is_active (gdb_plugin) &&
		strlen(prog_uri) > 6 &&
		strncmp (prog_uri, "pid://", 6) == 0)
	{
		long lpid;
		pid_t pid;
		
		sscanf (&prog_uri[6], "%ld", &lpid);
		pid = lpid;
		/* Attach to process */
		if (pid > 0)
			debugger_attach_process (gdb_plugin->debugger, pid);
		return;
	}
	
	if (gdb_debugger_is_active (gdb_plugin) == FALSE &&
		(prog_uri == NULL || strlen (prog_uri) <= 0))
	{
		gdb_initialize_debugger (gdb_plugin, NULL, FALSE,
								 search_directories);
	}
	else
	{
		vfs_uri = gnome_vfs_uri_new (prog_uri);
	
		g_return_if_fail (vfs_uri != NULL);
		
		if (gnome_vfs_uri_is_local (vfs_uri))
		{
			gchar *mime_type;
			const gchar *filename;
			
			mime_type = gnome_vfs_get_mime_type (prog_uri);
			filename = gnome_vfs_uri_get_path (vfs_uri);
			if (strcmp (mime_type, "application/x-executable") == 0)
			{
				if (gdb_debugger_is_active (gdb_plugin))
					debugger_load_executable (gdb_plugin->debugger, filename);
				else
					gdb_initialize_debugger (gdb_plugin, filename, FALSE,
											 search_directories);
			}
			else if (strcmp (mime_type, "application/x-shellscript") == 0)
			{
				/* Sounds like a libtool executable */
				if (gdb_debugger_is_active (gdb_plugin))
				{
					gchar *basename;
					gchar *dirname;
					gchar *proper_path;
					
					/* FIXME: By this time, debugger may have already been
					 * started without libtool support. But anyway try to load
					 * the proper executable found in .libs/ directory.
					 */
					basename = g_path_get_basename (filename);
					dirname = g_path_get_dirname (filename);
					proper_path = g_build_filename (dirname, ".libs",
													basename, NULL);
					if (g_file_test (proper_path, G_FILE_TEST_IS_EXECUTABLE))
					{
						debugger_load_executable (gdb_plugin->debugger,
												  proper_path);
					}
					else
					{
						debugger_load_executable (gdb_plugin->debugger,
												  filename);
					}
					g_free (proper_path);
					g_free (dirname);
					g_free (basename);
				}
				else
				{
					/* FIXME: We should really do more checks to confirm that
					 * this target is indeed libtool target
					 */
					gdb_initialize_debugger (gdb_plugin, filename,
											 TRUE, search_directories);
				}
			}
			else if (gdb_debugger_is_active (gdb_plugin) &&
					 strcmp (mime_type, "application/x-core") == 0)
			{
				debugger_load_core (gdb_plugin->debugger,filename);
			}
			else if (gdb_debugger_is_active (gdb_plugin))
			{
				anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
										  "Debugger can not load '%s' which is of mime type: '%s'",
										  filename, mime_type);
			}
			else if (gdb_debugger_is_active (gdb_plugin) == FALSE)
			{
				gdb_initialize_debugger (gdb_plugin, NULL,
										 FALSE, search_directories);
			}
			g_free (mime_type);
		}
		gnome_vfs_uri_unref (vfs_uri);
	}
}

static void
idebugger_run_continue (IAnjutaDebugger *plugin, GError **err)
{
	GdbPlugin *gdb_plugin = (GdbPlugin*)ANJUTA_PLUGIN (plugin);
	debugger_run (gdb_plugin->debugger);
}

static void
idebugger_step_in (IAnjutaDebugger *plugin, GError **err)
{
	GdbPlugin *gdb_plugin = (GdbPlugin*)ANJUTA_PLUGIN (plugin);
	debugger_step_in (gdb_plugin->debugger);
}

static void
idebugger_step_over (IAnjutaDebugger *plugin, GError **err)
{
	GdbPlugin *gdb_plugin = (GdbPlugin*)ANJUTA_PLUGIN (plugin);
	debugger_step_over (gdb_plugin->debugger);
}

static void
idebugger_run_to_position (IAnjutaDebugger *plugin, const gchar *file_uri,
						   gint file_line, GError **err)
{
	gint line;
	GdbPlugin *gdb_plugin;
	GnomeVFSURI *vfs_uri;
	
	gdb_plugin = (GdbPlugin*)G_OBJECT (plugin);
	
	if (gdb_debugger_is_active (gdb_plugin) == FALSE) return;

	g_return_if_fail (IANJUTA_IS_EDITOR (gdb_plugin->current_editor));
	
	if (file_uri && strlen (file_uri) > 0)
	{
		vfs_uri = gnome_vfs_uri_new (file_uri);
	}
	else
	{
		gchar *uri;
		uri = ianjuta_file_get_uri (IANJUTA_FILE (gdb_plugin->current_editor),
									NULL);
		if (!uri)
			return;
		vfs_uri = gnome_vfs_uri_new (uri);
		g_free (uri);
	}
	g_return_if_fail (vfs_uri != NULL);
	
	if (gnome_vfs_uri_is_local (vfs_uri))
	{
		const gchar *filename;
		gchar *buff;
		
		filename = gnome_vfs_uri_get_path (vfs_uri);
		
		if (file_line <= 0)
			line = ianjuta_editor_get_lineno (IANJUTA_EDITOR (gdb_plugin->current_editor),
											  NULL);
		else
			line = file_line;
		
		buff = g_strdup_printf ("%s:%d", filename, line);
		debugger_run_to_location (gdb_plugin->debugger, buff);
		g_free (buff);
	}
	gnome_vfs_uri_unref (vfs_uri);
}

static void
idebugger_step_out (IAnjutaDebugger *plugin, GError **err)
{
	GdbPlugin *gdb_plugin = (GdbPlugin*)ANJUTA_PLUGIN (plugin);
	debugger_step_out (gdb_plugin->debugger);
}

static void
idebugger_breakpoint_toggle (IAnjutaDebugger *plugin, const gchar *file_uri,
							 gint file_line, GError **err)
{
	gint line;
	GdbPlugin *gdb_plugin;
	GnomeVFSURI *vfs_uri;
	
	gdb_plugin = (GdbPlugin*)G_OBJECT (plugin);
	
	if (file_uri && strlen (file_uri) > 0)
	{
		vfs_uri = gnome_vfs_uri_new (file_uri);
	}
	else
	{
		gchar *uri;
		if (!gdb_plugin->current_editor)
			return;
		
		uri = ianjuta_file_get_uri (IANJUTA_FILE (gdb_plugin->current_editor),
									NULL);
		if (!uri)
			return;
		vfs_uri = gnome_vfs_uri_new (uri);
		g_free (uri);
	}
	g_return_if_fail (vfs_uri != NULL);
	
	if (gnome_vfs_uri_is_local (vfs_uri))
	{
		const gchar *filename;
		filename = gnome_vfs_uri_get_path (vfs_uri);
		
		if (file_line <= 0)
			line = ianjuta_editor_get_lineno (IANJUTA_EDITOR (gdb_plugin->current_editor),
											  NULL);
		else
			line = file_line;
		
		/* FIXME:
		breakpoints_dbase_toggle_breakpoint (gdb_plugin->breakpoints,
											 filename, line);
		*/
		breakpoints_dbase_toggle_breakpoint (gdb_plugin->breakpoints, line);
	}
	gnome_vfs_uri_unref (vfs_uri);
}

static void
idebugger_iface_init (IAnjutaDebuggerIface *iface)
{
	iface->is_busy = idebugger_is_busy;
	iface->load = idebugger_load;
	iface->run_continue = idebugger_run_continue;
	iface->step_in = idebugger_step_in;
	iface->step_over = idebugger_step_over;
	iface->step_out = idebugger_step_out;
	iface->run_to_position = idebugger_run_to_position;
	iface->breakpoint_toggle = idebugger_breakpoint_toggle;
}

ANJUTA_PLUGIN_BEGIN (GdbPlugin, gdb_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE(idebugger, IANJUTA_TYPE_DEBUGGER);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (GdbPlugin, gdb_plugin);
