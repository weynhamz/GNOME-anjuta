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
#include <libanjuta/plugins.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-debugger-manager.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include <libanjuta/interfaces/ianjuta-debugger.h>

#include "plugin.h"

#include "attach_process.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-debug-manager.ui"
#define GLADE_FILE PACKAGE_DATA_DIR"/glade/anjuta-debug-manager.glade"

static gpointer parent_class;


static void on_load_file_response (GtkDialog* dialog, gint id,
								   DebugManagerPlugin* plugin);
static void debug_manager_plugin_update_ui (DebugManagerPlugin *plugin);

static GList*
get_search_directories (DebugManagerPlugin *plugin)
{
	gchar *cwd;
	GList *node, *search_dirs = NULL;
	GList *slibs_dirs = NULL;
	GList *libs_dirs = NULL;
	
	cwd = g_get_current_dir();
	
	/* Set source file search directories */
	if (plugin->project_root_uri)
	{
		IAnjutaProjectManager *pm;
		pm = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
										 IAnjutaProjectManager, NULL);
		if (pm)
		{
			slibs_dirs =
				ianjuta_project_manager_get_targets (pm,
					IANJUTA_PROJECT_MANAGER_TARGET_SHAREDLIB,
												  NULL);
			libs_dirs =
				ianjuta_project_manager_get_targets (pm,
					IANJUTA_PROJECT_MANAGER_TARGET_STATICLIB,
												  NULL);
		}
	}
	slibs_dirs = g_list_reverse (slibs_dirs);
	libs_dirs = g_list_reverse (libs_dirs);
	
	search_dirs = g_list_prepend (search_dirs, g_strconcat ("file://",
															cwd, NULL));
	g_free (cwd);
	
	node = slibs_dirs;
	while (node)
	{
		gchar *dir_uri;
		dir_uri = g_path_get_dirname (node->data);
		search_dirs = g_list_prepend (search_dirs, dir_uri);
		node = g_list_next (node);
	}
	
	node = libs_dirs;
	while (node)
	{
		gchar *dir_uri;
		dir_uri = g_path_get_dirname (node->data);
		search_dirs = g_list_prepend (search_dirs, dir_uri);
		node = g_list_next (node);
	}
	
	g_list_foreach (slibs_dirs, (GFunc)g_free, NULL);
	g_list_free (slibs_dirs);
	g_list_foreach (libs_dirs, (GFunc)g_free, NULL);
	g_list_free (libs_dirs);
	
	return g_list_reverse (search_dirs);
}

static void
load_file (DebugManagerPlugin* plugin)
{
	const gchar *title = _("Load Target to debug");
	GtkWindow *parent = GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell);
	GtkWidget *dialog =
		gtk_file_chooser_dialog_new (title, parent,
									 GTK_FILE_CHOOSER_ACTION_OPEN,
									 GTK_STOCK_OPEN, GTK_RESPONSE_OK,
									 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
									 NULL);
	gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER(dialog), FALSE);
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (dialog), TRUE);

	GtkFileFilter *filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("All files"));
	gtk_file_filter_add_pattern (filter, "*");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

	g_signal_connect (G_OBJECT (dialog), "response",
					  G_CALLBACK (on_load_file_response), plugin);
	g_signal_connect_swapped (GTK_OBJECT (dialog), "response",
							  G_CALLBACK (gtk_widget_destroy),
							  GTK_OBJECT (dialog));
	gtk_widget_show (dialog);
}

static IAnjutaDebugger *
get_anjuta_debugger_iface (DebugManagerPlugin* plugin)
{
	if (!plugin->debugger)
	{
		/* Query for object implementing IAnjutaDebugger interface */
		plugin->debugger =
			anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
										IAnjutaDebugger, NULL);
		g_signal_connect_swapped (G_OBJECT (plugin->debugger), "busy",
						  G_CALLBACK (debug_manager_plugin_update_ui),
						  plugin);
	}
	return plugin->debugger;
}

static void
on_load_file_response (GtkDialog* dialog, gint response,
					   DebugManagerPlugin* plugin)
{
	gchar *uri;
	GList *search_dirs;
	
	if (response != GTK_RESPONSE_OK)
	{
		gtk_widget_destroy (GTK_WIDGET (dialog));
		return;
	}
	uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));
	if (!uri)
	{
		gtk_widget_destroy (GTK_WIDGET (dialog));
		return;
	}

	gtk_widget_destroy (GTK_WIDGET (dialog));
	
	search_dirs = get_search_directories (plugin);
	ianjuta_debugger_manager_start (IANJUTA_DEBUGGER_MANAGER (plugin),
									uri, search_dirs, NULL);
	g_free (uri);
	g_list_foreach (search_dirs, (GFunc)g_free, NULL);
	g_list_free (search_dirs);
}

static void
on_start_debug_activate (GtkAction* action, DebugManagerPlugin* plugin)
{
	if (!plugin->debugger)
	{
		GList *search_dirs = get_search_directories (plugin);
		if (!plugin->project_root_uri)
		{
			ianjuta_debugger_manager_start (IANJUTA_DEBUGGER_MANAGER (plugin),
											"", search_dirs, NULL);
		}
		else
		{
			IAnjutaProjectManager *pm;
			GList *exec_targets;
			
			pm = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
											 IAnjutaProjectManager, NULL);
			g_return_if_fail (pm != NULL);
			
			exec_targets =
				ianjuta_project_manager_get_targets (pm,
								 IANJUTA_PROJECT_MANAGER_TARGET_EXECUTABLE,
													 NULL);
			if (exec_targets == NULL)
			{
				ianjuta_debugger_manager_start (IANJUTA_DEBUGGER_MANAGER (plugin),
												"", search_dirs, NULL);
			}
			else if (g_list_length (exec_targets) == 1)
			{
				ianjuta_debugger_manager_start (IANJUTA_DEBUGGER_MANAGER (plugin),
												(gchar*)exec_targets->data,
												search_dirs, NULL);
				g_free (exec_targets->data);
				g_list_free (exec_targets);
			}
			else
			{
				GladeXML *gxml;
				GtkWidget *dlg, *treeview;
				GtkTreeViewColumn *column;
				GtkCellRenderer *renderer;
				GtkListStore *store;
				gint response;
				GList *node;
				GtkTreeIter iter;
				const gchar *sel_target = NULL;

				gxml = glade_xml_new (GLADE_FILE, "debugger_start_dialog",
									  NULL);
				dlg = glade_xml_get_widget (gxml, "debugger_start_dialog");
				treeview = glade_xml_get_widget (gxml, "programs_treeview");
				
				gtk_window_set_transient_for (GTK_WINDOW (dlg),
								  GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell));
			
				store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
				node = exec_targets;
				while (node)
				{
					const gchar *rel_path;
				
					rel_path =
					    (gchar*)node->data +
					     strlen (plugin->project_root_uri) + 1;
					gtk_list_store_append (store, &iter);
					gtk_list_store_set (store, &iter, 0, rel_path, 1,
										node->data, -1);
					g_free (node->data);
					node = g_list_next (node);
				}
				g_list_free (exec_targets);
				
				gtk_tree_view_set_model (GTK_TREE_VIEW (treeview),
										 GTK_TREE_MODEL (store));
				g_object_unref (store);
				
				column = gtk_tree_view_column_new ();
				gtk_tree_view_column_set_sizing (column,
												 GTK_TREE_VIEW_COLUMN_AUTOSIZE);
				gtk_tree_view_column_set_title (column,
												_("Select debugging target"));
				
				renderer = gtk_cell_renderer_text_new ();
				gtk_tree_view_column_pack_start (column, renderer, FALSE);
				gtk_tree_view_column_add_attribute (column, renderer, "text",
													0);
				gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
				gtk_tree_view_set_expander_column (GTK_TREE_VIEW (treeview),
												   column);
				
				/* Run dialog */
				response = gtk_dialog_run (GTK_DIALOG (dlg));
				if (response == GTK_RESPONSE_OK)
				{
					GtkTreeSelection *sel;
					sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
					if (gtk_tree_selection_get_selected (sel,
														 (GtkTreeModel**)(&store),
														 &iter))
					{
						gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, 1,
											&sel_target, -1);
					}
				}
				gtk_widget_destroy (dlg);
				if (sel_target)
				{
					ianjuta_debugger_manager_start (IANJUTA_DEBUGGER_MANAGER (plugin),
													sel_target, search_dirs,
													NULL);
				}
				g_object_unref (gxml);
			}
		}
		g_list_foreach (search_dirs, (GFunc)g_free, NULL);
		g_list_free (search_dirs);
	}
}

static void
on_debugger_stop_activate (GtkAction* action, DebugManagerPlugin* plugin)
{
	if (plugin->debugger)
		ianjuta_debugger_manager_stop (IANJUTA_DEBUGGER_MANAGER (plugin),
									   NULL);
}

static void
on_load_target_action_activate (GtkAction* action, DebugManagerPlugin* plugin)
{
	load_file (plugin);
}

static void
on_attach_to_project_action_activate (GtkAction* action, DebugManagerPlugin* plugin)
{
	pid_t selected_pid;
	GtkWindow *parent;
	static AttachProcess *attach_process = NULL;
	
	parent = GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell);
	if (!attach_process)
		attach_process = attach_process_new();
	
	selected_pid = attach_process_show (attach_process, parent);
	if (selected_pid > 0)
	{
		gchar *buffer;
		long lpid = selected_pid;
		
		buffer = g_strdup_printf ("pid://%ld", lpid);
		/*
		if (!plugin->debugger)
			ianjuta_debugger_manager_start (IANJUTA_DEBUGGER_MANAGER (plugin),
											"", NULL);
		*/
		if (plugin->debugger)
		{
			GList *search_dirs;
			search_dirs = get_search_directories (plugin);
			ianjuta_debugger_load (plugin->debugger, buffer, search_dirs, NULL);
			g_list_foreach (search_dirs, (GFunc)g_free, NULL);
			g_list_free (search_dirs);
		}
		g_free (buffer);
	}
}

static void
on_run_continue_action_activate (GtkAction* action, DebugManagerPlugin* plugin)
{
	IAnjutaDebugger *debugger = get_anjuta_debugger_iface (plugin);
	/*
	if (!plugin->debugger)
		ianjuta_debugger_manager_start (IANJUTA_DEBUGGER_MANAGER (plugin),
										"", NULL);
	*/
	if (plugin->debugger)
		ianjuta_debugger_run_continue (debugger, NULL /* TODO */);
}

static void
on_step_in_action_activate (GtkAction* action, DebugManagerPlugin* plugin)
{
	IAnjutaDebugger *debugger = get_anjuta_debugger_iface (plugin);
	/*
	if (!plugin->debugger)
		ianjuta_debugger_manager_start (IANJUTA_DEBUGGER_MANAGER (plugin),
										"", NULL);
	*/
	if (plugin->debugger)
		ianjuta_debugger_step_in (debugger, NULL /* TODO */);
}

static void
on_step_over_action_activate (GtkAction* action, DebugManagerPlugin* plugin)
{
	/*
	if (!plugin->debugger)
		ianjuta_debugger_manager_start (IANJUTA_DEBUGGER_MANAGER (plugin),
										"", NULL);
	*/
	if (plugin->debugger)
		ianjuta_debugger_step_over (plugin->debugger, NULL /* TODO */);
}

static void
on_step_out_action_activate (GtkAction* action, DebugManagerPlugin* plugin)
{
	/*
	if (!plugin->debugger)
		ianjuta_debugger_manager_start (IANJUTA_DEBUGGER_MANAGER (plugin),
										"", NULL);
	*/
	if (plugin->debugger)
		ianjuta_debugger_step_out (plugin->debugger, NULL /* TODO */);
}

static void
on_run_to_cursor_action_activate (GtkAction* action, DebugManagerPlugin* plugin)
{
	gchar *uri;
	gint line;
	/*
	if (!plugin->debugger)
		ianjuta_debugger_manager_start (IANJUTA_DEBUGGER_MANAGER (plugin),
										"", NULL);
	*/
	if (!plugin->debugger)
		return;
	
	g_return_if_fail (plugin->current_editor != NULL);
	
	uri = ianjuta_file_get_uri (IANJUTA_FILE (plugin->current_editor), NULL);
	if (!uri)
		return;
	
	line = ianjuta_editor_get_lineno (IANJUTA_EDITOR (plugin->current_editor),
									  NULL);
	ianjuta_debugger_run_to_position (plugin->debugger, uri, line,
									  NULL /* TODO */);
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
		"ActionDebuggerStart",                    /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("_Start Debugger"),                    /* Display label */
		"<shift>F12",                             /* short-cut */
		N_("Start the debugging session"),        /* Tooltip */
		G_CALLBACK (on_start_debug_activate)      /* action callback */
	},
	{
		"ActionDebuggerLoad",                      /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("Load debugging target ..."),          /* Display label */
		NULL,                                     /* short-cut */
		N_("Open the target for debugging"),      /* Tooltip */
		G_CALLBACK (on_load_target_action_activate) /* action callback */
	},
	{
		"ActionDebuggerAttachToProcess",                  /* Action name */
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
		"ActionDebuggerRunContinue",                      /* Action name */
		GTK_STOCK_EXECUTE,                        /* Stock icon, if any */
		N_("Run/_Continue"),                      /* Display label */
		"F4",                                     /* short-cut */
		N_("Continue the execution of the program"), /* Tooltip */
		G_CALLBACK (on_run_continue_action_activate) /* action callback */
	},
	{
		"ActionDebuggerStepIn",                           /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("Step _In"),                           /* Display label */
		"F5",                                     /* short-cut */
		N_("Single step into function"),          /* Tooltip */
		G_CALLBACK (on_step_in_action_activate)   /* action callback */
	},
	{
		"ActionDebuggerStepOver",                         /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("Step O_ver"),                         /* Display label */
		"F6",                                     /* short-cut */
		N_("Single step over function"),          /* Tooltip */
		G_CALLBACK (on_step_over_action_activate) /* action callback */
	},
	{
		"ActionDebuggerStepOut",                  /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("Step _Out"),                          /* Display label */
		"F7",                                     /* short-cut */
		N_("Single step out of the function"),    /* Tooltip */
		G_CALLBACK (on_step_out_action_activate)  /* action callback */
	},
	{
		"ActionDebuggerRunToPosition",            /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("_Run to cursor"),                     /* Display label */
		"F8",                                     /* short-cut */
		N_("Run to the cursor"),                  /* Tooltip */
		G_CALLBACK (on_run_to_cursor_action_activate)  /* action callback */
	},
	{
		"ActionDebuggerStop",
		GTK_STOCK_STOP,
		N_("St_op Debugger"),
		NULL,
		N_("Say goodbye to the debugger"),
		G_CALLBACK (on_debugger_stop_activate)
	}
};

static void
debug_manager_plugin_update_ui (DebugManagerPlugin *plugin)
{
	AnjutaUI *ui;
	GtkAction *action;
	
	/* We set busy FALSE even if debugger is not yet started because
	 * following actions will automatically start the debugger.
	 */
	gboolean is_busy = TRUE;
	
	if (plugin->debugger)
	{
		is_busy = ianjuta_debugger_is_busy (plugin->debugger, NULL);
	}
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (plugin)->shell, NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupDebug",
								   "ActionDebuggerStart");
	g_object_set (G_OBJECT (action), "sensitive",
				  (plugin->debugger == NULL), NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupDebug",
								   "ActionDebuggerStop");
	g_object_set (G_OBJECT (action), "sensitive",
				  (plugin->debugger != NULL), NULL);

	action = anjuta_ui_get_action (ui, "ActionGroupDebug",
								   "ActionDebuggerLoad");
	g_object_set (G_OBJECT (action), "sensitive",
				  (plugin->debugger == NULL) || !is_busy, NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupDebug",
								   "ActionDebuggerAttachToProcess");
	g_object_set (G_OBJECT (action), "sensitive", !is_busy, NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupDebug",
								   "ActionDebuggerRunContinue");
	g_object_set (G_OBJECT (action), "sensitive", !is_busy, NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupDebug",
								   "ActionDebuggerStepIn");
	g_object_set (G_OBJECT (action), "sensitive", !is_busy, NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupDebug",
								   "ActionDebuggerStepOver");
	g_object_set (G_OBJECT (action), "sensitive", !is_busy, NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupDebug",
								   "ActionDebuggerStepOut");
	g_object_set (G_OBJECT (action), "sensitive", !is_busy, NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupDebug",
								   "ActionDebuggerRunToPosition");
	g_object_set (G_OBJECT (action), "sensitive", !is_busy, NULL);
}

static void
value_added_project_root_uri (AnjutaPlugin *plugin, const gchar *name,
							  const GValue *value, gpointer user_data)
{
	DebugManagerPlugin *dm_plugin;
	const gchar *root_uri;

	dm_plugin = (DebugManagerPlugin *) plugin;
	
	if (dm_plugin->project_root_uri)
		g_free (dm_plugin->project_root_uri);
	dm_plugin->project_root_uri = NULL;
	
	root_uri = g_value_get_string (value);
	if (root_uri)
	{
		dm_plugin->project_root_uri = g_strdup (root_uri);
	}
}

static void
value_removed_project_root_uri (AnjutaPlugin *plugin, const gchar *name,
								gpointer user_data)
{
	DebugManagerPlugin *dm_plugin;

	dm_plugin = (DebugManagerPlugin *) plugin;
	
	if (dm_plugin->project_root_uri)
		g_free (dm_plugin->project_root_uri);
	dm_plugin->project_root_uri = NULL;
}

static void
value_added_current_editor (AnjutaPlugin *plugin, const char *name,
							const GValue *value, gpointer data)
{
	GObject *editor;
	DebugManagerPlugin *dm_plugin;
	
	editor = g_value_get_object (value);
	dm_plugin = (DebugManagerPlugin*)plugin;
	dm_plugin->current_editor = editor;
}

static void
value_removed_current_editor (AnjutaPlugin *plugin,
							  const char *name, gpointer data)
{
	DebugManagerPlugin *dm_plugin = (DebugManagerPlugin*)plugin;
	dm_plugin->current_editor = NULL;
}

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
	
	/* Add watches */
	debug_manager_plugin->project_watch_id = 
		anjuta_plugin_add_watch (plugin, "project_root_uri",
								 value_added_project_root_uri,
								 value_removed_project_root_uri, NULL);
	debug_manager_plugin->editor_watch_id = 
		anjuta_plugin_add_watch (plugin, "document_manager_current_editor",
								 value_added_current_editor,
								 value_removed_current_editor, NULL);
	debug_manager_plugin_update_ui (debug_manager_plugin);
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin* plugin)
{
	DebugManagerPlugin *dplugin;
	AnjutaUI *ui;

	DEBUG_PRINT ("DebugManagerPlugin: Deactivating Debug Manager plugin ...");
	
	dplugin = (DebugManagerPlugin *) plugin;
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	anjuta_ui_unmerge (ui, dplugin->uiid);
	anjuta_ui_remove_action_group (ui, dplugin->action_group);
	
	dplugin->uiid = 0;
	dplugin->action_group = NULL;
	
	return TRUE;
}

static void
dispose (GObject* obj)
{
/*	DebugManagerPlugin *plugin = (DebugManagerPlugin *) obj;
	if (plugin->uri)
	{
		g_free (plugin->uri);
		plugin->uri = NULL;
	}*/
}

static void
debug_manager_plugin_instance_init (GObject* obj)
{
	DebugManagerPlugin *plugin = (DebugManagerPlugin *) obj;
	plugin->uiid = 0;
	
	plugin->action_group = NULL;
	plugin->project_root_uri = NULL;
	plugin->debugger = NULL;
	plugin->current_editor = NULL;
	plugin->editor_watch_id = 0;
	plugin->project_watch_id = 0;
	
	/* plugin->uri = NULL; */
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
static void
idebugger_manager_start (IAnjutaDebuggerManager *debugman,
						 const gchar *prog_uri,
						 const GList *source_search_directories,
						 GError** e)
{
	DebugManagerPlugin *plugin = (DebugManagerPlugin *) debugman;
	IAnjutaDebugger *idebugger = get_anjuta_debugger_iface (plugin);
	ianjuta_debugger_load (idebugger, prog_uri, source_search_directories,
						   NULL);
}

static gboolean
idebugger_manager_stop (IAnjutaDebuggerManager *debugman, GError** e)
{
	DebugManagerPlugin *plugin = (DebugManagerPlugin *) debugman;
	if (plugin->debugger)
	{
		g_signal_handlers_disconnect_by_func (G_OBJECT (plugin->debugger),
											  debug_manager_plugin_update_ui,
											  plugin);
		if (anjuta_plugins_unload_plugin (ANJUTA_PLUGIN (plugin)->shell,
										  G_OBJECT (plugin->debugger)))
		{
			plugin->debugger = NULL;
			debug_manager_plugin_update_ui (plugin);
			return TRUE;
		}
		else
		{
			/* Debugger unload failed. Reconnect the signal */
			g_signal_connect_swapped (G_OBJECT (plugin->debugger), "busy",
							  G_CALLBACK (debug_manager_plugin_update_ui),
							  plugin);
		}
	}
	
	/* FIXME: Update ui */
	return FALSE;
}

static gboolean
idebugger_manager_is_debugger_active (IAnjutaDebuggerManager *debugman,
									  GError** e)
{
	DebugManagerPlugin *plugin = (DebugManagerPlugin *) debugman;
	return (plugin->debugger != NULL);
}

static gboolean
idebugger_manager_is_debugger_busy (IAnjutaDebuggerManager *debugman,
									  GError** e)
{
	DebugManagerPlugin *plugin = (DebugManagerPlugin *) debugman;
	
	if (plugin->debugger)
	{
		return ianjuta_debugger_is_busy (plugin->debugger, NULL /* TODO */);
	}
	else
		return TRUE;
}

static void
idebugger_manager_iface_init (IAnjutaDebuggerManagerIface *iface)
{
	iface->is_active = idebugger_manager_is_debugger_active;
	iface->is_busy = idebugger_manager_is_debugger_busy;
	
	iface->start = idebugger_manager_start;
	iface->stop = idebugger_manager_stop;
}

/* Implementation of IAnjutaFile interface */
static void
ifile_open (IAnjutaFile* plugin, const gchar* uri, GError** e)
{
	DebugManagerPlugin *dplugin = (DebugManagerPlugin *) ANJUTA_PLUGIN (plugin);
	GList *search_dirs = get_search_directories (dplugin);
	ianjuta_debugger_manager_start (IANJUTA_DEBUGGER_MANAGER (plugin),
									uri, search_dirs, NULL);
	g_list_foreach (search_dirs, (GFunc)g_free, NULL);
	g_list_free (search_dirs);
}

static gchar*
ifile_get_uri (IAnjutaFile* plugin, GError** e)
{
	return NULL; // ((DebugManagerPlugin *) plugin)->uri;
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
