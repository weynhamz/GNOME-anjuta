/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * plugin.c
 * Copyright (C) James Liggett 2006 <jrliggett@cox.net>
 * 
 * plugin.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2, or (at your option) any later version.
 * 
 * plugin.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with plugin.c.  See the file "COPYING".  If not,
 * write to:  The Free Software Foundation, Inc.,
 *            59 Temple Place - Suite 330,
 *            Boston,  MA  02111-1307, USA.
 */


#include "plugin.h"


#define UI_FILE PACKAGE_DATA_DIR"/ui/profiler.ui"
#define GLADE_FILE PACKAGE_DATA_DIR"/glade/profiler.glade"
#define ICON_FILE PACKAGE_PIXMAPS_DIR"/profiler.png"

static gpointer parent_class;

static void
add_options_strings (GPtrArray *options, const gchar *prefix, const gchar *args)
{
	gchar **split_args;  /* List of arguements split by lines */
	gchar **current_string;
	gchar *full_arg;  /* Argument with prefix */
	
	if (strlen (args) > 0)
	{
		split_args = g_strsplit (args, "\n", -1);
		
		for (current_string = split_args; 
			 *current_string; 
			 current_string++)
		{
			
			if (strlen (*current_string) > 0)
			{
				full_arg = g_strconcat (prefix, *current_string, NULL);
				g_ptr_array_add (options, full_arg);
			}
		}
		
		g_strfreev (split_args);
	}
}

static GPtrArray *
setup_options (Profiler *profiler)
{
	GPtrArray *options;
	gchar *symbols;
	
	options = g_ptr_array_new ();
	
	/* First handle the easy ones: -a, -c, and -z */
	if (gprof_options_get_int (profiler->options, "no_show_static"))
	{
		g_ptr_array_add (options, g_strdup ("-a"));
	}
	
	if (gprof_options_get_int (profiler->options, "show_possible_called"))
	{
		g_ptr_array_add (options, g_strdup ("-c"));
	}
	
	if (gprof_options_get_int (profiler->options, "show_uncalled"))
	{
		g_ptr_array_add (options, g_strdup ("-z"));
	}
	
	/* If the user wants to modify the call graph, put in -p so we have a flat
	 * profile. */
	if (!gprof_options_get_int (profiler->options, "show_all_symbols"))
	{
		g_ptr_array_add (options, g_strdup ("-p"));
		
		symbols = gprof_options_get_string (profiler->options, "symbols");
	
		if (gprof_options_get_int (profiler->options, "include_symbols"))
		{
			add_options_strings (options, "-q", symbols);
		}
		
		if (gprof_options_get_int (profiler->options, "exclude_symbols"))
		{
			add_options_strings (options, "-Q", symbols);
		}
		
		g_free (symbols);
	}
	
	/* Time propagation options */
	if (!gprof_options_get_int (profiler->options, "propagate_all_symbols"))
	{
	
		symbols = gprof_options_get_string (profiler->options,
										  	 "propagation_symbols");
		
		if (gprof_options_get_int (profiler->options,
								   "propagate_include_symbols"))
		{
			add_options_strings (options, "-n", symbols);
		}
		
		if (gprof_options_get_int (profiler->options,
								   "propagate_exclude_symbols"))
		{
			add_options_strings (options, "-N", symbols);
		}
		
		g_free (symbols);
	}
	
	/* NULL terminate the array for compatibility with g_strfreev */
	
	g_ptr_array_add (options, NULL);
	
	/* Other options not directly passed to gprof */
	
	/* If there is an existing profile data monitor and automatic refresh
	 * is disabled, cancel the monitor */
	 
	 if (profiler->profile_data_monitor)
	 {
	 	if (!gprof_options_get_int (profiler->options, "automatic_refresh"))
	 	{
	 		gnome_vfs_monitor_cancel (profiler->profile_data_monitor);
	 		profiler->profile_data_monitor = NULL;
	 	}
	 }
	
	return options;	
}

static gboolean 
profiler_get_data (Profiler *profiler)
{
	GPtrArray *options;
	gchar **option_strings;
	gboolean ret = FALSE;
	
	if (profiler->profile_target_path)
	{

		options = setup_options (profiler);
		if (!gprof_profile_data_init_profile (profiler->profile_data, 
										 	 profiler->profile_target_path,
										 	 options))
		{
			anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (profiler)->shell),
									  _("Could not get profiling data."
										"\n\n"
										"Please check the path to "
										"this target's profiling data file"));
		}
											 
		option_strings = (gchar **) g_ptr_array_free (options, FALSE);
		g_strfreev (option_strings);
		
		ret = TRUE;
	}
	
	return ret;
}

static void
on_profile_data_changed (GnomeVFSMonitorHandle *handle, 
						 const gchar *monitor_uri, const gchar *info_uri,
						 GnomeVFSMonitorEventType event,
						 gpointer user_data)
{
	Profiler *profiler;
	
	profiler = PROFILER (user_data);
	
	switch (event)
	{
		case GNOME_VFS_MONITOR_EVENT_CHANGED:
			if (profiler_get_data (profiler))
				gprof_view_manager_refresh_views (profiler->view_manager);
			break;
		case GNOME_VFS_MONITOR_EVENT_DELETED:
			gnome_vfs_monitor_cancel (handle);
			profiler->profile_data_monitor = NULL;
			break;
		default:
			break;			
	}
}

static void
profiler_set_target (Profiler *profiler, const gchar *profile_target_uri)
{
	gchar *profile_target_path;
	gchar *profile_target_dir;
	gchar *gmon_out_path;
	gchar *gmon_out_uri;
	
	if (profiler->profile_target_path)
	{
		g_free (profiler->profile_target_path);
		profiler->profile_target_path = NULL;
	}
	
	if (profile_target_uri)
	{
		profile_target_path = gnome_vfs_get_local_path_from_uri (profile_target_uri);
		profile_target_dir = g_path_get_dirname (profile_target_path);
		gmon_out_path = g_build_filename (profile_target_dir, "gmon.out", 
										  NULL);
		gmon_out_uri = gnome_vfs_get_uri_from_local_path (gmon_out_path);
		
		if (g_file_test (gmon_out_path, G_FILE_TEST_EXISTS))
		{
			profiler->profile_target_path = profile_target_path;
		
		
			/* Set up a file change monitor for automatic refresh if enabled */
			if (gprof_options_get_int (profiler->options, 
									   "automatic_refresh"))
			{
				/* Cancel any existing monitor */
				if (profiler->profile_data_monitor)
					gnome_vfs_monitor_cancel (profiler->profile_data_monitor);
				
				gnome_vfs_monitor_add (&profiler->profile_data_monitor,
									   gmon_out_uri, GNOME_VFS_MONITOR_FILE,  
									   on_profile_data_changed,
									   (gpointer) profiler);
			}
			
			/* Show user the profiler views if they aren't visible so they
			 * know what happened */
			anjuta_shell_present_widget (ANJUTA_PLUGIN (profiler)->shell,
										 gprof_view_manager_get_notebook (profiler->view_manager),
										 NULL);
		}
		else
		{
			anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (profiler)->shell),
									   _("This target does not have any "
									     "profiling data.\n\n"
									     "Please ensure that the target is "
									     "complied with profiling support "
										 "and that it is run at least "
										 "once."));
		}
		
		g_free (profile_target_dir);
		g_free (gmon_out_path);
		g_free (gmon_out_uri);
	}
}

static GProfOptions *
register_options ()
{
	GProfOptions *options;
	
	options = gprof_options_new ();
	
	gprof_options_register_key (options, "automatic_refresh", "0",
								"automatic_refresh_check", 
								OPTION_TYPE_TOGGLE);
	
	gprof_options_register_key (options, "no_show_static", "0",
								"no_show_static_check", 
								OPTION_TYPE_TOGGLE);
											  
	gprof_options_register_key (options, "show_possible_called", "0",
								"show_possible_called_check",
								OPTION_TYPE_TOGGLE);
																		  
	gprof_options_register_key (options, "show_uncalled", "0",
								"show_uncalled_check", 
								OPTION_TYPE_TOGGLE);
	
	gprof_options_register_key (options, "show_all_symbols", "1",
								"show_all_symbols_radio",
								OPTION_TYPE_TOGGLE);
	
	gprof_options_register_key (options, "include_symbols", "0",
								"include_symbols_radio",
								OPTION_TYPE_TOGGLE);
	
	gprof_options_register_key (options, "exclude_symbols", "0",
								"exclude_symbols_radio",
								OPTION_TYPE_TOGGLE);
	
	gprof_options_register_key (options, "symbols", "", "symbols_text_view",
								OPTION_TYPE_TEXT_ENTRY);
											  
	gprof_options_register_key (options, "propagate_all_symbols", "1",
								"propagate_all_symbols_radio",
								OPTION_TYPE_TOGGLE);
	
	gprof_options_register_key (options, "propagate_include_symbols", "0",
								"propagate_include_symbols_radio",
								OPTION_TYPE_TOGGLE);
											  
	gprof_options_register_key (options, "propagate_exclude_symbols", "0",
								"propagate_exclude_symbols_radio", 
								OPTION_TYPE_TOGGLE);
											  
	gprof_options_register_key (options, "propagation_symbols", "",
								"propagation_text_view",
								OPTION_TYPE_TEXT_ENTRY);										  
	
	return options;
}

static void
on_profiling_options_button_clicked (GtkButton *button, gpointer *user_data)
{													 
	Profiler *profiler;
	GladeXML *gxml;
	GtkWidget *profiling_options_dialog;
	GtkWidget *closebutton;

	profiler = PROFILER (user_data);
	gxml = glade_xml_new (GLADE_FILE, "profiling_options_dialog",
						  NULL);
	profiling_options_dialog = glade_xml_get_widget (gxml, "profiling_options_dialog");
	closebutton = glade_xml_get_widget (gxml, "closebutton");
	
	g_signal_connect (profiling_options_dialog, "response", G_CALLBACK (gtk_widget_hide),
					  profiling_options_dialog);
	
	gprof_options_create_window (profiler->options, gxml);
	
	gtk_window_set_transient_for (GTK_WINDOW (profiling_options_dialog),
								  GTK_WINDOW (ANJUTA_PLUGIN(profiler)->shell));
	
	gtk_dialog_run (GTK_DIALOG (profiling_options_dialog));
	
	g_object_unref (gxml);
}

static gboolean
on_target_selected (GtkTreeSelection *selection, GtkTreeModel *model, 
					GtkTreePath *path, gboolean path_currently_selected,
					Profiler *profiler)
{
	GtkTreeIter list_iter;
	gchar *target_uri;
	
	gtk_tree_model_get_iter (model, &list_iter, path);
	gtk_tree_model_get (model, &list_iter, 1, &target_uri, -1);
	
	if (target_uri)
	{
		gprof_options_set_target (profiler->options, target_uri);
		g_free (target_uri);
	}
	
	return TRUE;
}

static void
on_profiler_select_target (GtkAction *action, Profiler *profiler)
{
	GladeXML *gxml;
	GtkWidget *select_target_dialog;
	GtkWidget *profiling_options_button;
	GtkWidget *targets_list_view;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkListStore *targets_list_store;
	gint response;
	GList *current_target;
	GtkTreeIter iter;
	GList *exec_targets;
	IAnjutaProjectManager *project_manager;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	gchar *target = NULL;
	gchar *relative_path;
	guint project_root_uri_length;
	
	if (profiler->project_root_uri)
	{
		project_manager = anjuta_shell_get_interface (ANJUTA_PLUGIN (profiler)->shell,
													  IAnjutaProjectManager, NULL);
		
		exec_targets = ianjuta_project_manager_get_targets (project_manager, 
							 								IANJUTA_PROJECT_MANAGER_TARGET_EXECUTABLE,
											 				NULL);
											 				
		project_root_uri_length = strlen (profiler->project_root_uri) + 1;
	
		gxml = glade_xml_new (GLADE_FILE, "select_target_dialog", NULL);
		select_target_dialog = glade_xml_get_widget (gxml, 
													 "select_target_dialog");
		targets_list_view = glade_xml_get_widget (gxml, 
												  "targets_list_view");
		profiling_options_button = glade_xml_get_widget (gxml,
														 "profiling_options_button");
														 
		g_signal_connect (profiling_options_button, "clicked",
						  G_CALLBACK (on_profiling_options_button_clicked),
						  profiler);
		
		gtk_window_set_transient_for (GTK_WINDOW (select_target_dialog),
									  GTK_WINDOW (ANJUTA_PLUGIN(profiler)->shell));
			
		if (exec_targets)
		{
			/* Populate listview */
			selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (targets_list_view));
			gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
			gtk_tree_selection_set_select_function (selection, 
													(GtkTreeSelectionFunc) on_target_selected,
													profiler, NULL);
			targets_list_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
			current_target = exec_targets;
			
			while (current_target)
			{
				relative_path = (gchar *) current_target->data + project_root_uri_length;
				
				gtk_list_store_append (targets_list_store, &iter);
				gtk_list_store_set (targets_list_store, &iter, 0, relative_path, 1,
									current_target->data, -1);
									
				g_free (current_target->data);
				current_target = g_list_next (current_target);
			}
			g_list_free (exec_targets);
			
			gtk_tree_view_set_model (GTK_TREE_VIEW (targets_list_view),
									 GTK_TREE_MODEL (targets_list_store));
			g_object_unref (targets_list_store);
			
			column = gtk_tree_view_column_new ();
			gtk_tree_view_column_set_sizing (column,
											 GTK_TREE_VIEW_COLUMN_AUTOSIZE);

			renderer = gtk_cell_renderer_text_new ();
			gtk_tree_view_column_pack_start (column, renderer, FALSE);
			gtk_tree_view_column_add_attribute (column, renderer, "text",
												0);
			gtk_tree_view_append_column (GTK_TREE_VIEW (targets_list_view), column);
			gtk_tree_view_set_expander_column (GTK_TREE_VIEW (targets_list_view), column);
		}
		
		/* Run dialog */
		response = gtk_dialog_run (GTK_DIALOG (select_target_dialog));
		
		if (response == GTK_RESPONSE_OK)
		{		
			if (gtk_tree_selection_get_selected (selection, &model, &iter))
			{
				gtk_tree_model_get (model, &iter, 1, &target, -1);
				profiler_set_target (profiler, target);
				
				if (profiler_get_data (profiler))
					gprof_view_manager_refresh_views (profiler->view_manager);
			}
			else
				profiler_set_target (profiler, NULL);
		}
		
		gtk_widget_destroy (select_target_dialog);
		g_object_unref (gxml);
	}
}

static void
on_profiler_refresh (GtkAction *action, Profiler *profiler)
{
	if (profiler_get_data (profiler))
		gprof_view_manager_refresh_views (profiler->view_manager);
}

static void
on_profiler_delete_data (GtkAction *action, Profiler *profiler)
{
	gchar *profile_target_dir;
	gchar *gmon_out_path;
	
	if (profiler->profile_target_path)
	{
		profile_target_dir = g_path_get_dirname (profiler->profile_target_path);
		gmon_out_path = g_build_filename (profile_target_dir, "gmon.out", NULL);
		
		g_unlink (gmon_out_path);
		
		g_free (profile_target_dir);
		g_free (gmon_out_path);
	}
}

static void
project_root_added (AnjutaPlugin *plugin, const gchar *name,
					const GValue *value, gpointer user_data)
{
	Profiler *profiler;
	AnjutaUI *ui;
	const gchar *root_uri;
	GtkAction *action;
	
	profiler = PROFILER (plugin);
	root_uri = g_value_get_string (value);
	
	if (root_uri)
	{
		g_free (profiler->project_root_uri);
		profiler->project_root_uri = g_strdup (root_uri);
		
		ui = anjuta_shell_get_ui (plugin->shell, NULL);
		action = anjuta_ui_get_action (ui, "ActionGroupProfiler", "ActionMenuProfiler");
		g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
	}
	
	
}

static void
project_root_removed (AnjutaPlugin *plugin, const gchar *name,
					  gpointer user_data)
{
	Profiler *profiler;
	AnjutaUI *ui;
	GtkAction *action;
	
	profiler = PROFILER (plugin);
	
	g_free (profiler->project_root_uri);
	profiler->project_root_uri = NULL;
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupProfiler", "ActionMenuProfiler");
	g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
}

static void
on_session_load (AnjutaShell *shell, AnjutaSessionPhase phase,
				 AnjutaSession *session,
				 Profiler *plugin)
{
	const gchar *session_dir;
	gchar *settings_file_path;
	
	if (phase == ANJUTA_SESSION_PHASE_NORMAL)
	{
		session_dir = anjuta_session_get_session_directory (session);
		settings_file_path = g_build_filename (session_dir, 
											   "profiler-settings.xml", 
											   NULL);
	
		gprof_options_load (plugin->options, settings_file_path);
	
		g_free (settings_file_path);
	}
}

static void
on_session_save (AnjutaShell *shell, AnjutaSessionPhase phase,
				 AnjutaSession *session,
				 Profiler *plugin)
{
	const gchar *session_dir;
	gchar *settings_file_path;
	
	if (phase == ANJUTA_SESSION_PHASE_NORMAL)
	{
		session_dir = anjuta_session_get_session_directory (session);
		settings_file_path = g_build_filename (session_dir, 
											   "profiler-settings.xml", 
											   NULL);
	
		gprof_options_save (plugin->options, settings_file_path);
	
		g_free (settings_file_path);
	}
}


static GtkActionEntry actions_file[] = {
	{
		"ActionMenuDebug",               		/* Action name */
		NULL,                           		/* Stock icon, if any */
		N_("Debug"),                     		/* Display label */
		NULL,                            		/* short-cut */
		NULL,                      		 		/* Tooltip */
		NULL    						 		/* Action callback */
	},
	{
		"ActionMenuProfiler",            		/* Action name */
		"profiler-icon",                        /* Stock icon, if any */
		N_("Profiler"),                  		/* Display label */
		NULL,                            		/* short-cut */
		NULL,                      		 		/* Tooltip */
		NULL    						 		/* Action callback */
	},
	{
		"ActionProfilerSelectTarget",    		/* Action name */
		GTK_STOCK_EXECUTE,                      /* Stock icon, if any */
		N_("Select Target..."),          		/* Display label */
		NULL,                            		/* short-cut */
		NULL,                      		 		/* Tooltip */
		G_CALLBACK (on_profiler_select_target)  /* Action callback */
	},
	{
		"ActionProfilerRefresh",    			/* Action name */
		GTK_STOCK_REFRESH,                      /* Stock icon, if any */
		N_("Refresh"),          				/* Display label */
		NULL,                            		/* short-cut */
		NULL,                      		 		/* Tooltip */
		G_CALLBACK (on_profiler_refresh)  		/* Action callback */
	},
	{
		"ActionProfilerDeleteData",    			/* Action name */
		GTK_STOCK_DELETE,                       /* Stock icon, if any */
		N_("Delete Data"),          			/* Display label */
		NULL,                            		/* short-cut */
		NULL,                      		 		/* Tooltip */
		G_CALLBACK (on_profiler_delete_data)  	/* Action callback */
	}
};

#define REGISTER_ICON(icon, stock_id) \
	pixbuf = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"icon, NULL); \
	icon_set = gtk_icon_set_new_from_pixbuf (pixbuf); \
	gtk_icon_factory_add (icon_factory, stock_id, icon_set); \
	g_object_unref (pixbuf);

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
	REGISTER_ICON ("profiler.png", "profiler-icon");
}	


static gboolean
profiler_activate (AnjutaPlugin *plugin)
{

	AnjutaUI *ui;
	Profiler *profiler;
	IAnjutaSymbolManager *symbol_manager;
	IAnjutaDocumentManager *document_manager;
	
	DEBUG_PRINT ("Profiler: Activating Profiler plugin ...");
	profiler = PROFILER (plugin);

	/* Add all UI actions and merge UI */
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	register_stock_icons (plugin);
	
	profiler->action_group = 
		anjuta_ui_add_action_group_entries (ui, "ActionGroupProfiler",
											_("Application Performance Profiler"),
											actions_file,
											G_N_ELEMENTS (actions_file),
											GETTEXT_PACKAGE, TRUE,
											plugin);
	profiler->uiid = anjuta_ui_merge (ui, UI_FILE);
	
	profiler->view_manager = gprof_view_manager_new ();
	profiler->profile_data = gprof_profile_data_new ();
	
	symbol_manager = anjuta_shell_get_interface (plugin->shell, 
												 IAnjutaSymbolManager,
												 NULL);
	
	document_manager = anjuta_shell_get_interface (plugin->shell, 
												   IAnjutaDocumentManager,
												   NULL);
	
	gprof_view_manager_add_view (profiler->view_manager,
								 GPROF_VIEW (gprof_flat_profile_view_new (profiler->profile_data,
																		  symbol_manager,
																		  document_manager)),
								 _("Flat Profile"));
	gprof_view_manager_add_view (profiler->view_manager,
								 GPROF_VIEW (gprof_call_graph_view_new (profiler->profile_data,
																		symbol_manager,
																		document_manager)),
								 _("Call Graph"));
	gprof_view_manager_add_view (profiler->view_manager,
								 GPROF_VIEW (gprof_function_call_tree_view_new (profiler->profile_data,
																				symbol_manager,
																				document_manager)),
								 _("Function Call Tree"));
	
#ifdef HAVE_GRAPHVIZ
	gprof_view_manager_add_view (profiler->view_manager,
								 GPROF_VIEW (gprof_function_call_chart_view_new (profiler->profile_data,
																				 symbol_manager,
																				 document_manager)),
								 _("Function Call Chart"));
#endif
								 
	anjuta_shell_add_widget (plugin->shell, 
							 gprof_view_manager_get_notebook (profiler->view_manager),
							 "Profiler",
							 _("Profiler"),
							 "profiler-icon",
							 ANJUTA_SHELL_PLACEMENT_CENTER,
							 NULL);
							 
	profiler->project_watch_id = anjuta_plugin_add_watch (plugin, "project_root_uri", 
										 				  project_root_added, 
										 				  project_root_removed, NULL);
										 				  
	profiler->options = register_options ();
	
	/* Set up session save/load */
	g_signal_connect (G_OBJECT (plugin->shell), "save_session", 
					  G_CALLBACK (on_session_save), plugin);
	
	g_signal_connect (G_OBJECT (plugin->shell), "load_session", 
					  G_CALLBACK (on_session_load), plugin);

	return TRUE;
}

static gboolean
profiler_deactivate (AnjutaPlugin *plugin)
{

	AnjutaUI *ui;
	Profiler *profiler;
	
	DEBUG_PRINT ("Profiler: Dectivating Profiler plugin ...");

	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	profiler = PROFILER (plugin);
	
	anjuta_plugin_remove_watch (plugin, profiler->project_watch_id, TRUE);
	
	anjuta_ui_unmerge (ui, PROFILER (plugin)->uiid);
	anjuta_ui_remove_action_group (ui, PROFILER (plugin)->action_group);
	
	anjuta_shell_remove_widget (plugin->shell,
								gprof_view_manager_get_notebook (profiler->view_manager),
								NULL);
								
	profiler_set_target (profiler, NULL);
	gprof_view_manager_free (profiler->view_manager);
	gprof_profile_data_free (profiler->profile_data);
	
	gprof_options_destroy (profiler->options);
	
	g_free (profiler->project_root_uri);
	
	if (profiler->profile_data_monitor)
		gnome_vfs_monitor_cancel (profiler->profile_data_monitor);
	
	return TRUE;
}

static void
profiler_finalize (GObject *obj)
{
	/* Finalization codes here */
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

static void
profiler_dispose (GObject *obj)
{
	/* Disposition codes */
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
profiler_instance_init (GObject *obj)
{
	Profiler *profiler = PROFILER (obj);

	profiler->uiid = 0;
	profiler->project_root_uri = NULL;
	profiler->profile_target_path = NULL;


}

static void
profiler_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = profiler_activate;
	plugin_class->deactivate = profiler_deactivate;
	klass->finalize = profiler_finalize;
	klass->dispose = profiler_dispose;
}

/* File open interface */
static void
ifile_open (IAnjutaFile *manager, const gchar *uri,
			GError **err)
{
	Profiler *profiler;
	
	profiler = PROFILER (manager);
	
	profiler_set_target (profiler, uri);
	
	if (profiler_get_data (profiler))
		gprof_view_manager_refresh_views (profiler->view_manager);
	
}

static gchar*
ifile_get_uri (IAnjutaFile *manager, GError **err)
{
	DEBUG_PRINT ("Unsupported operation");
	return NULL;
}

static void
ifile_iface_init (IAnjutaFileIface *iface)
{
	iface->open = ifile_open;
	iface->get_uri = ifile_get_uri;
}

ANJUTA_PLUGIN_BEGIN (Profiler, profiler);
ANJUTA_PLUGIN_ADD_INTERFACE (ifile, IANJUTA_TYPE_FILE);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (Profiler, profiler);
