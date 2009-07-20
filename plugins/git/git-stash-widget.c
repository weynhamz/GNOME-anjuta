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
						  GtkListStore *stash_list_model)
{
	/* Allow refreshes to continue */
	g_object_set_data (G_OBJECT (stash_list_model), "being-refreshed", 
					   GINT_TO_POINTER (FALSE));

	git_report_errors (command, return_code);
	g_object_unref (command);
}

void
git_stash_widget_create (Git *plugin, GtkWidget **stash_widget, 
						 GtkWidget **stash_widget_grip)
{
	gchar *objects[] = {"stash_widget_scrolled_window", 
						"stash_widget_grip_hbox",
						"stash_list_model"};
	GtkBuilder *bxml;
	GError *error;
	GitUIData *data;
	GtkWidget *stash_widget_scrolled_window;
	GtkWidget *stash_widget_grip_hbox;

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
	stash_widget_grip_hbox = GTK_WIDGET (gtk_builder_get_object (bxml,
																 "stash_widget_grip_hbox"));

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

		gtk_list_store_clear (stash_list_model);

		g_signal_connect (G_OBJECT (list_command), "data-arrived",
						  G_CALLBACK (on_git_list_stash_command_data_arrived),
						  stash_list_model);

		g_signal_connect (G_OBJECT (list_command), "command-finished",
						  G_CALLBACK (on_list_command_finished),
						  stash_list_model);

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

	data = g_object_get_data (G_OBJECT (plugin->stash_widget), "ui-data");
	stash_list_model = GTK_LIST_STORE (gtk_builder_get_object (data->bxml,
															   "stash_list_model"));

	gtk_list_store_clear (stash_list_model);
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