/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2009 <jrliggett@cox.net>
 * 
 * anjuta is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "git-stash-widget.h"

static void 
on_stash_refresh_monitor_changed (GFileMonitor *file_monitor, GFile *file,
								  GFile *other_file, 
								  GFileMonitorEvent event_type, Git *plugin)
{
	
	 /* Git seems to make several changes to this file at once, so just respond
	  * to the changes done hint so we don't refresh more than once at a 
	  * time. */
	if (event_type == G_FILE_MONITOR_EVENT_CREATED ||
	    event_type == G_FILE_MONITOR_EVENT_DELETED ||
	    event_type == G_FILE_MONITOR_EVENT_CHANGED)
	{
		git_stash_widget_refresh (plugin);
	}
}

static void
on_list_command_finished (AnjutaCommand *command, guint return_code,
						  GitUIData *data)
{
	GtkTreeModel *stash_list_model;
	GtkWidget *stash_widget_clear_button;
	GtkTreeIter iter;

	stash_list_model = GTK_TREE_MODEL (gtk_builder_get_object (data->bxml,
	                                                       	   "stash_list_model"));
	stash_widget_clear_button = GTK_WIDGET (gtk_builder_get_object (data->bxml,
	                                                            	"stash_widget_clear_button"));

	/* Allow refreshes to continue */
	g_object_set_data (G_OBJECT (stash_list_model), "being-refreshed", 
					   GINT_TO_POINTER (FALSE));

	gtk_widget_set_sensitive (stash_widget_clear_button,
	                          gtk_tree_model_get_iter_first (stash_list_model, 
	                                                         &iter));
	    

	git_report_errors (command, return_code);
	g_object_unref (command);
}

static gboolean
on_stash_widget_view_row_selected (GtkTreeSelection *selection,
								   GtkTreeModel *model,
						  		   GtkTreePath *path, 
							  	   gboolean path_currently_selected,
								   GitUIData *data)
{
	GtkWidget *stash_widget_apply_button;
	GtkWidget *stash_widget_show_button;
	GtkWidget *stash_widget_drop_button;

	stash_widget_apply_button = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																	"stash_widget_apply_button"));
	stash_widget_show_button = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																   "stash_widget_show_button"));
	stash_widget_drop_button = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																   "stash_widget_drop_button"));

	gtk_widget_set_sensitive (stash_widget_apply_button, 
							  !path_currently_selected);
	gtk_widget_set_sensitive (stash_widget_show_button,
							  !path_currently_selected);
	gtk_widget_set_sensitive (stash_widget_drop_button,
							  !path_currently_selected);

	return TRUE;
}

static void
on_stash_widget_save_button_clicked (GtkButton *button, GitUIData *data)
{
	GitStashSaveCommand *save_command;

	save_command = git_stash_save_command_new (data->plugin->project_root_directory,
											   FALSE, NULL);

	git_create_message_view (data->plugin);

	g_signal_connect (G_OBJECT (save_command), "command-finished",
					  G_CALLBACK (on_git_stash_save_command_finished),
					  data->plugin);

	g_signal_connect (G_OBJECT (save_command), "data-arrived",
					  G_CALLBACK (on_git_command_info_arrived),
					  data->plugin);

	anjuta_command_start (ANJUTA_COMMAND (save_command));
	
}

static void
on_stash_widget_apply_button_clicked (GtkButton *button, GitUIData *data)
{
	GtkWidget *stash_widget_view;
	GtkListStore *stash_list_model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	gchar *stash;
	GitStashApplyCommand *apply_command;

	stash_widget_view = GTK_WIDGET (gtk_builder_get_object (data->bxml,
															"stash_widget_view"));
	stash_list_model = GTK_LIST_STORE (gtk_builder_get_object (data->bxml,
															   "stash_list_model"));
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (stash_widget_view));

	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
	{
		gtk_tree_model_get (GTK_TREE_MODEL (stash_list_model), &iter, 0, 
							&stash, -1);

		apply_command = git_stash_apply_command_new (data->plugin->project_root_directory,
													 FALSE, stash);

		g_free (stash);

		git_create_message_view (data->plugin);

		g_signal_connect (G_OBJECT (apply_command), "data-arrived",
						  G_CALLBACK (on_git_command_info_arrived),
						  data->plugin);

		g_signal_connect (G_CALLBACK (apply_command), "command-finished",
						  G_CALLBACK (on_git_stash_apply_command_finished),
						  data->plugin);

		anjuta_command_start (ANJUTA_COMMAND (apply_command));
	}
	
}

static void
on_stash_widget_show_button_clicked (GtkButton *button, GitUIData *data)
{
	GtkWidget *stash_widget_view;
	GtkListStore *stash_list_model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	gchar *stash;
	guint stash_number;
	gchar *editor_name;
	IAnjutaDocumentManager *document_manager;
	IAnjutaEditor *editor;
	GitStashShowCommand *show_command;

	stash_widget_view = GTK_WIDGET (gtk_builder_get_object (data->bxml,
															"stash_widget_view"));
	stash_list_model = GTK_LIST_STORE (gtk_builder_get_object (data->bxml,
															   "stash_list_model"));
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (stash_widget_view));

	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
	{
		gtk_tree_model_get (GTK_TREE_MODEL (stash_list_model), &iter,  
							0, &stash, 
		                    2, &stash_number, 
		                    -1);

		show_command = git_stash_show_command_new (data->plugin->project_root_directory,
												   stash);

		editor_name = g_strdup_printf ("Stash %i.diff", stash_number);

		document_manager = anjuta_shell_get_interface (ANJUTA_PLUGIN (data->plugin)->shell,
		                                               IAnjutaDocumentManager, 
		                                               NULL);
		editor = ianjuta_document_manager_add_buffer (document_manager, 
		                                              editor_name,
		                                              "", NULL);

		g_free (stash);
		g_free (editor_name);

		g_signal_connect (G_OBJECT (show_command), "data-arrived",
						  G_CALLBACK (git_send_raw_command_output_to_editor),
						  editor);

		g_signal_connect (G_CALLBACK (show_command), "command-finished",
						  G_CALLBACK (on_git_command_finished),
						  data->plugin);

		anjuta_command_start (ANJUTA_COMMAND (show_command));
	}
}

static void
on_stash_drop_command_finished (AnjutaCommand *command, guint return_code,
                            	Git *plugin)
{
	AnjutaStatus *status;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell,
									  NULL);
	
	anjuta_status (status, _("Git: Stash dropped."), 5);
	
	git_report_errors (command, return_code);
	
	g_object_unref (command);
}

static void
on_stash_widget_drop_button_clicked (GtkButton *button, GitUIData *data)
{
	GtkWidget *stash_widget_view;
	GtkListStore *stash_list_model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	gchar *stash;
	GitStashDropCommand *drop_command;

	stash_widget_view = GTK_WIDGET (gtk_builder_get_object (data->bxml,
															"stash_widget_view"));
	stash_list_model = GTK_LIST_STORE (gtk_builder_get_object (data->bxml,
															   "stash_list_model"));
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (stash_widget_view));

	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
	{
		gtk_tree_model_get (GTK_TREE_MODEL (stash_list_model), &iter, 0, 
							&stash, -1);

		drop_command = git_stash_drop_command_new (data->plugin->project_root_directory,
												   stash);

		g_free (stash);

		git_create_message_view (data->plugin);

		g_signal_connect (G_OBJECT (drop_command), "data-arrived",
						  G_CALLBACK (on_git_command_info_arrived),
						  data->plugin);

		g_signal_connect (G_CALLBACK (drop_command), "command-finished",
						  G_CALLBACK (on_stash_drop_command_finished),
						  data->plugin);

		anjuta_command_start (ANJUTA_COMMAND (drop_command));
	}
}

static void
on_stash_clear_command_finished (AnjutaCommand *command, guint return_code,
                                 Git *plugin)
{
	AnjutaStatus *status;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell,
									  NULL);
	
	anjuta_status (status, _("Git: All stashes cleared."), 5);
	
	git_report_errors (command, return_code);
	
	g_object_unref (command);
}

static void
on_stash_widget_clear_button_clicked (GtkButton *button, GitUIData *data)
{
	GitStashClearCommand *clear_command;

	clear_command = git_stash_clear_command_new (data->plugin->project_root_directory);

	g_signal_connect (G_OBJECT (clear_command), "command-finished",
	                  G_CALLBACK (on_stash_clear_command_finished),
	                  data->plugin);

	anjuta_command_start (ANJUTA_COMMAND (clear_command));
}

void
git_stash_widget_create (Git *plugin, GtkWidget **stash_widget, 
						 GtkWidget **stash_widget_grip)
{
	gchar *objects[] = {"stash_widget_scrolled_window", 
						"stash_widget_grip_hbox",
						"stash_list_model", NULL};
	GtkBuilder *bxml;
	GError *error;
	GitUIData *data;
	GtkWidget *stash_widget_scrolled_window;
	GtkWidget *stash_widget_view;
	GtkWidget *stash_widget_grip_hbox;
	GtkWidget *stash_widget_save_button;
	GtkWidget *stash_widget_apply_button;
	GtkWidget *stash_widget_show_button;
	GtkWidget *stash_widget_drop_button;
	GtkWidget *stash_widget_clear_button;
	GtkTreeSelection *selection;

	bxml = gtk_builder_new ();
	error = NULL;
	data = git_ui_data_new (plugin, bxml);

	if (!gtk_builder_add_objects_from_file (data->bxml, BUILDER_FILE, objects, 
	                                        &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}

	stash_widget_scrolled_window = GTK_WIDGET (gtk_builder_get_object (bxml, 
																	   "stash_widget_scrolled_window"));
	stash_widget_view = GTK_WIDGET (gtk_builder_get_object (bxml,
															"stash_widget_view"));
	stash_widget_grip_hbox = GTK_WIDGET (gtk_builder_get_object (bxml,
																 "stash_widget_grip_hbox"));
	stash_widget_save_button = GTK_WIDGET (gtk_builder_get_object (bxml,
																   "stash_widget_save_button"));
	stash_widget_apply_button = GTK_WIDGET (gtk_builder_get_object (bxml,
																	"stash_widget_apply_button"));
	stash_widget_show_button = GTK_WIDGET (gtk_builder_get_object (bxml,
	                                                           	   "stash_widget_show_button"));
	stash_widget_drop_button = GTK_WIDGET (gtk_builder_get_object (bxml,
	                                                           	   "stash_widget_drop_button"));
	stash_widget_clear_button = GTK_WIDGET (gtk_builder_get_object (bxml,
	                                                           	    "stash_widget_clear_button"));
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (stash_widget_view));

	gtk_tree_selection_set_select_function (selection, 
											(GtkTreeSelectionFunc) on_stash_widget_view_row_selected,
											data, NULL);

	g_signal_connect (G_OBJECT (stash_widget_save_button), "clicked",
					  G_CALLBACK (on_stash_widget_save_button_clicked),
					  data);

	g_signal_connect (G_OBJECT (stash_widget_apply_button), "clicked",
					  G_CALLBACK (on_stash_widget_apply_button_clicked),
					  data);

	g_signal_connect (G_OBJECT (stash_widget_show_button), "clicked",
					  G_CALLBACK (on_stash_widget_show_button_clicked),
					  data);

	g_signal_connect (G_OBJECT (stash_widget_drop_button), "clicked",
					  G_CALLBACK (on_stash_widget_drop_button_clicked),
					  data);

	g_signal_connect (G_OBJECT (stash_widget_clear_button), "clicked",
					  G_CALLBACK (on_stash_widget_clear_button_clicked),
					  data);

	g_object_set_data_full (G_OBJECT (stash_widget_scrolled_window), "ui-data",
							data, (GDestroyNotify) git_ui_data_free);

	*stash_widget = stash_widget_scrolled_window;
	*stash_widget_grip = stash_widget_grip_hbox;
}

void
git_stash_widget_refresh (Git *plugin)
{
	GitUIData *data;
	GtkListStore *stash_list_model;
	gboolean being_refreshed;
	GitStashListCommand *list_command;

	data = g_object_get_data (G_OBJECT (plugin->stash_widget), "ui-data");
	stash_list_model = GTK_LIST_STORE (gtk_builder_get_object (data->bxml,
															   "stash_list_model"));
	being_refreshed = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (stash_list_model), 
														  "being-refreshed"));

	/* Use a crude locking hack similar to what the log viewer does to avoid 
	 * multiple concurrent refreshes. */
	if (!being_refreshed)
	{
		list_command = git_stash_list_command_new (plugin->project_root_directory);

		git_stash_widget_clear (plugin);

		g_signal_connect (G_OBJECT (list_command), "data-arrived",
						  G_CALLBACK (on_git_list_stash_command_data_arrived),
						  stash_list_model);

		g_signal_connect (G_OBJECT (list_command), "command-finished",
						  G_CALLBACK (on_list_command_finished),
						  data);

		g_object_set_data (G_OBJECT (stash_list_model), "being-refreshed", 
						   GINT_TO_POINTER (TRUE));

		anjuta_command_start (ANJUTA_COMMAND (list_command));
	}
	 
}

void
git_stash_widget_clear (Git *plugin)
{
	GitUIData *data;
	GtkListStore *stash_list_model;
	GtkWidget *stash_widget_apply_button;
	GtkWidget *stash_widget_show_button;
	GtkWidget *stash_widget_drop_button;

	data = g_object_get_data (G_OBJECT (plugin->stash_widget), "ui-data");
	stash_list_model = GTK_LIST_STORE (gtk_builder_get_object (data->bxml,
															   "stash_list_model"));
	stash_widget_apply_button = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																	"stash_widget_apply_button"));
	stash_widget_show_button = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																   "stash_widget_show_button"));
	stash_widget_drop_button = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																   "stash_widget_drop_button"));

	gtk_list_store_clear (stash_list_model);

	gtk_widget_set_sensitive (stash_widget_apply_button, FALSE);
	gtk_widget_set_sensitive (stash_widget_show_button, FALSE);
	gtk_widget_set_sensitive (stash_widget_drop_button, FALSE);
}

GFileMonitor *
git_stash_widget_setup_refresh_monitor (Git *plugin)
{
	gchar *git_stash_path;
	GFile *git_stash_file;
	GFileMonitor *git_stash_monitor;

	git_stash_path = g_strjoin (G_DIR_SEPARATOR_S,
	                            plugin->project_root_directory,
	                            ".git",
								"logs",
	                            "refs",
	                            "stash",
	                            NULL);

	git_stash_file = g_file_new_for_path (git_stash_path);
	git_stash_monitor = g_file_monitor_file (git_stash_file, 0, NULL, 
											 NULL);

	g_signal_connect (G_OBJECT (git_stash_monitor), "changed",
	                  G_CALLBACK (on_stash_refresh_monitor_changed),
	                  plugin);

	g_free (git_stash_path);
	g_object_unref (git_stash_file);

	return git_stash_monitor;
}

GtkListStore *
git_stash_widget_get_model (Git *plugin)
{
	GitUIData *data;
	GtkListStore *stash_list_model;

	data = g_object_get_data (G_OBJECT (plugin->stash_widget), "ui-data");
	stash_list_model = GTK_LIST_STORE (gtk_builder_get_object (data->bxml,
															   "stash_list_model"));
	return stash_list_model;
}