/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2008 <jrliggett@cox.net>
 * 
 * anjuta is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * anjuta is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with anjuta.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include "git-bisect-dialog.h"

static void
on_bisect_start_dialog_response (GtkDialog *dialog, gint response_id, 
								 GitUIData *data)
{
	GtkWidget *bisect_start_revision_radio;
	GtkWidget *bisect_start_bad_revision_entry;
	GtkWidget *bisect_start_good_revision_entry;
	const gchar *bad_revision;
	const gchar *good_revision;
	GitBisectStartCommand *bisect_command;
	
	if (response_id == GTK_RESPONSE_OK)
	{	
		bisect_start_revision_radio = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																		  "bisect_start_revision_radio"));
		bisect_start_bad_revision_entry = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																			  "bisect_start_bad_revision_entry"));
		bisect_start_good_revision_entry = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																			 "bisect_start_good_revision_entry"));
		bad_revision = "";
		
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (bisect_start_revision_radio)))
		{
			bad_revision = gtk_entry_get_text (GTK_ENTRY (bisect_start_bad_revision_entry));
			
			if (!git_check_input (GTK_WIDGET (dialog), 
								  bisect_start_bad_revision_entry, 
								  bad_revision, _("Please enter a revision.")))
			{
				return;
			}
		}
		
		good_revision = gtk_entry_get_text (GTK_ENTRY (bisect_start_good_revision_entry));
		
		/* Empty revisions means don't give it to bisect. Use gtk_entry_get_text
		 * so we can re-assign the pointers to NULL without worrying about 
		 * leaks. */
		if (strlen (bad_revision) == 0)
			bad_revision = NULL;
		
		if (strlen (good_revision) == 0)
			good_revision = NULL;
		
		bisect_command = git_bisect_start_command_new (data->plugin->project_root_directory,
													   bad_revision, 
													   good_revision);
		
		git_create_message_view (data->plugin);
		
		g_signal_connect (G_OBJECT (bisect_command), "command-finished",
						  G_CALLBACK (on_git_command_finished),
						  data->plugin);
		
		g_signal_connect (G_OBJECT (bisect_command), "data-arrived",
						  G_CALLBACK (on_git_command_info_arrived),
						  data->plugin);
		
		anjuta_command_start (ANJUTA_COMMAND (bisect_command));
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	git_ui_data_free (data);
}

static void
on_bisect_start_revision_radio_toggled (GtkToggleButton *toggle_button, 
										GitUIData *data)
{
	GtkWidget *bisect_start_dialog;
	GtkWidget *bisect_start_revision_entry;
	gboolean active;
	
	bisect_start_dialog = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
															  "bisect_start_dialog"));
	bisect_start_revision_entry = GTK_WIDGET (gtk_builder_get_object (data->bxml,
														              "bisect_start_bad_revision_entry"));
	
	active = gtk_toggle_button_get_active (toggle_button);
	gtk_widget_set_sensitive (bisect_start_revision_entry, active);
	
	if (active)
	{
		gtk_window_set_focus (GTK_WINDOW (bisect_start_dialog),
							  bisect_start_revision_entry);
	}
	
}

static void
bisect_start_dialog (Git *plugin)
{
	GtkBuilder *bxml;
	gchar *objects[] = {"bisect_start_dialog", NULL};
	GError *error;
	GtkWidget *dialog;
	GtkWidget *bisect_start_revision_radio;
	GitUIData *data;
	
	bxml = gtk_builder_new ();
	error = NULL;

	if (!gtk_builder_add_objects_from_file (bxml, BUILDER_FILE, objects, 
	                                        &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}
	
	dialog = GTK_WIDGET (gtk_builder_get_object (bxml, "bisect_start_dialog"));
	bisect_start_revision_radio = GTK_WIDGET (gtk_builder_get_object (bxml, 
																	  "bisect_start_revision_radio"));
	data = git_ui_data_new (plugin, bxml);
	
	g_signal_connect (G_OBJECT (dialog), "response", 
					  G_CALLBACK (on_bisect_start_dialog_response), 
					  data);
	
	g_signal_connect (G_OBJECT (bisect_start_revision_radio), "toggled",
					  G_CALLBACK (on_bisect_start_revision_radio_toggled),
					  data);
	
	gtk_widget_show_all (dialog);
}

static void
bisect_reset (Git *plugin)
{
	GitBisectResetCommand *bisect_command;
	
	bisect_command = git_bisect_reset_command_new (plugin->project_root_directory);
	
	git_create_message_view (plugin);
	
	g_signal_connect (G_OBJECT (bisect_command), "command-finished",
					  G_CALLBACK (on_git_command_finished),
					  plugin);
		
	g_signal_connect (G_OBJECT (bisect_command), "data-arrived",
					  G_CALLBACK (on_git_command_info_arrived),
					  plugin);
	
	anjuta_command_start (ANJUTA_COMMAND (bisect_command));
}

static void
bisect_state (Git *plugin, GitBisectState state, const gchar *revision)
{
	GitBisectStateCommand *bisect_command;
	
	bisect_command = git_bisect_state_command_new (plugin->project_root_directory,
												   state, revision);
	
	git_create_message_view (plugin);
	
	g_signal_connect (G_OBJECT (bisect_command), "command-finished",
					  G_CALLBACK (on_git_command_finished),
					  plugin);
		
	g_signal_connect (G_OBJECT (bisect_command), "data-arrived",
					  G_CALLBACK (on_git_command_info_arrived),
					  plugin);
	
	anjuta_command_start (ANJUTA_COMMAND (bisect_command));
}

static void
update_bisect_menus (AnjutaUI *ui, gboolean in_bisect)
{
	GtkAction *bisect_start_action;
	GtkAction *bisect_reset_action;
	GtkAction *bisect_good_action;
	GtkAction *bisect_bad_action;
	GtkAction *bisect_log_menu_action;
	
	bisect_start_action = anjuta_ui_get_action (ui, "ActionGroupGit",
												"ActionGitBisectStart");
	bisect_reset_action = anjuta_ui_get_action (ui, "ActionGroupGit",
												"ActionGitBisectReset");
	bisect_good_action = anjuta_ui_get_action (ui, "ActionGroupGit",
											   "ActionGitBisectGood");
	bisect_bad_action = anjuta_ui_get_action (ui, "ActionGroupGit",
											  "ActionGitBisectBad");
	bisect_log_menu_action = anjuta_ui_get_action (ui, "ActionGroupGitLog",
												   "ActionMenuGitLogBisect");
	
	if (in_bisect)
	{
		gtk_action_set_sensitive (bisect_start_action, FALSE);
		gtk_action_set_sensitive (bisect_reset_action, TRUE);
		gtk_action_set_sensitive (bisect_good_action, TRUE);
		gtk_action_set_sensitive (bisect_bad_action, TRUE);
		gtk_action_set_sensitive (bisect_log_menu_action, TRUE);
	}
	else
	{
		gtk_action_set_sensitive (bisect_start_action, TRUE);
		gtk_action_set_sensitive (bisect_reset_action, FALSE);
		gtk_action_set_sensitive (bisect_good_action, FALSE);
		gtk_action_set_sensitive (bisect_bad_action, FALSE);
		gtk_action_set_sensitive (bisect_log_menu_action, FALSE);
	}
}

static void
on_bisect_file_monitor_changed (GFileMonitor *file_monitor, GFile *file,
								GFile *other_file, GFileMonitorEvent event_type,
								AnjutaUI *ui)
{
	switch (event_type)
	{
		case G_FILE_MONITOR_EVENT_CREATED:
			/* This indicates that a bisect has started */
			update_bisect_menus (ui, TRUE);
			break;
		case G_FILE_MONITOR_EVENT_DELETED:
			/* This indicates that a bisect has started */
			update_bisect_menus (ui, FALSE);
			break;
		default:
			break;
	}
	
}

void
on_menu_git_bisect_start (GtkAction *action, Git *plugin)
{
	bisect_start_dialog (plugin);
}

void
on_menu_git_bisect_reset (GtkAction *action, Git *plugin)
{
	bisect_reset (plugin);
}

void
on_menu_git_bisect_good (GtkAction *action, Git *plugin)
{
	bisect_state (plugin, GIT_BISECT_STATE_GOOD, NULL);
}

void
on_menu_git_bisect_bad (GtkAction *action, Git *plugin)
{
	bisect_state (plugin, GIT_BISECT_STATE_BAD, NULL);
}

void
on_log_menu_git_bisect_good (GtkAction *action, Git *plugin)
{
	GitRevision *revision;
	gchar *sha;
	
	revision = git_log_get_selected_revision (plugin);
	
	if (revision)
	{
		sha = git_revision_get_sha (revision);
		
		bisect_state (plugin, GIT_BISECT_STATE_GOOD, sha);
		g_free (sha);
	}
}

void
on_log_menu_git_bisect_bad (GtkAction *action, Git *plugin)
{
	GitRevision *revision;
	gchar *sha;
	
	revision = git_log_get_selected_revision (plugin);
	
	if (revision)
	{
		sha = git_revision_get_sha (revision);
		
		bisect_state (plugin, GIT_BISECT_STATE_BAD, sha);
		g_free (sha);
	}
}

GFileMonitor *
bisect_menus_init (Git *plugin)
{
	AnjutaUI *ui;
	gchar *bisect_log_path;
	GFile *bisect_log_file;
	GFileMonitor *bisect_file_monitor;
	
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (plugin)->shell, NULL);
	
	/* Git keeps a bisect log file in a project's top-level .git directory.
	 * The presence of this file indicates that a bisect is in progress.
	 * We'll monitor the creation/deletion of this file to detect the 
	 * occurrence of new bisect operations and the conclusions of existing ones
	 * and update the UI accordingly. */
	bisect_log_path = g_strjoin (G_DIR_SEPARATOR_S, 
								 plugin->project_root_directory,
								 ".git",
								 "BISECT_LOG", 
								 NULL);
	
	update_bisect_menus (ui, g_file_test (bisect_log_path, G_FILE_TEST_EXISTS));
	
	bisect_log_file = g_file_new_for_path (bisect_log_path);
	bisect_file_monitor = g_file_monitor_file (bisect_log_file, 0, NULL, NULL);
	
	g_signal_connect (G_OBJECT (bisect_file_monitor), "changed",
					  G_CALLBACK (on_bisect_file_monitor_changed),
					  ui);
	
	g_object_unref (bisect_log_file);
	g_free (bisect_log_path);
	
	return bisect_file_monitor;
}
