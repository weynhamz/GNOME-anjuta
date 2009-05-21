/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2007 <jrliggett@cox.net>
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

#include "git-ui-utils.h"

/* Private structure for pulse progress */
typedef struct
{
	AnjutaStatus *status;
	gchar *text;
} PulseProgressData;

GitUIData* 
git_ui_data_new (Git* plugin, GtkBuilder *bxml)
{
	GitUIData* data = g_new0 (GitUIData, 1);
	data->plugin = plugin;
	data->bxml = bxml;
	
	return data;
}

void 
git_ui_data_free (GitUIData* data)
{
	g_object_unref (data->bxml);
	g_free (data);
}

GitProgressData *
git_progress_data_new (Git *plugin, const gchar *text)
{
	GitProgressData *data;
	AnjutaStatus *status;
	
	data = g_new0 (GitProgressData, 1);
	data->plugin = plugin;
	data->text = g_strdup (text);
	data->last_progress = 100; /* Add the ticks when we get a progress signal */
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell, NULL);
	
	return data;
}

void
git_progress_data_free (GitProgressData *data)
{
	AnjutaStatus *status;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (data->plugin)->shell, 
									  NULL);
	
	g_free (data->text);
	g_free (data);
}

static void
on_message_view_destroy (Git* plugin, gpointer destroyed_view)
{
	plugin->message_view = NULL;
}

void
git_create_message_view (Git *plugin)
{
	IAnjutaMessageManager* message_manager; 
		
		
	message_manager = anjuta_shell_get_interface  (ANJUTA_PLUGIN (plugin)->shell,	
												   IAnjutaMessageManager, NULL);
	plugin->message_view =  ianjuta_message_manager_get_view_by_name (message_manager, 
																	  _("Git"), 
																	  NULL);
	if (!plugin->message_view)
	{
		plugin->message_view = ianjuta_message_manager_add_view (message_manager, 
																 _("Git"), 
																 ICON_FILE, 
																 NULL);
		g_object_weak_ref (G_OBJECT (plugin->message_view), 
						   (GWeakNotify) on_message_view_destroy, plugin);
	}
	
	ianjuta_message_view_clear(plugin->message_view, NULL);
	ianjuta_message_manager_set_current_view (message_manager, 
											  plugin->message_view, 
											  NULL);
}

gboolean 
git_check_input (GtkWidget *parent, GtkWidget *widget, const gchar *input, 
				 const gchar *error_message)
{
	gboolean ret;
	GtkWidget *dialog;
	
	ret = FALSE;
	
	if (input)
	{
		if (strlen (input) > 0)
			ret = TRUE;
	}
	
	if (!ret)
	{
		dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
										 GTK_DIALOG_DESTROY_WITH_PARENT,
										 GTK_MESSAGE_WARNING,
										 GTK_BUTTONS_OK,
										 error_message);
		
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		
		gtk_window_set_focus (GTK_WINDOW (parent), widget);
	}

	
	return ret;
}

gchar * 
git_get_log_from_textview (GtkWidget* textview)
{
	gchar* log;
	GtkTextBuffer* textbuf;
	GtkTextIter iterbegin, iterend;
	
	textbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
	gtk_text_buffer_get_start_iter(textbuf, &iterbegin);
	gtk_text_buffer_get_end_iter(textbuf, &iterend) ;
	log = gtk_text_buffer_get_text(textbuf, &iterbegin, &iterend, FALSE);
	return log;
}

static gboolean
status_pulse_timer (PulseProgressData *data)
{
	anjuta_status_progress_pulse (data->status, data->text);
	return TRUE;
}

static gboolean
pulse_timer (GtkProgressBar *progress_bar)
{
	gtk_progress_bar_pulse (progress_bar);
	return TRUE;
}

static void
on_pulse_timer_destroyed (PulseProgressData *data)
{
	anjuta_status_progress_reset (data->status);
	
	g_free (data->text);
	g_free (data);
}

guint
git_status_bar_progress_pulse (Git *plugin, gchar *text)
{
	PulseProgressData *data;
	
	data = g_new0 (PulseProgressData, 1);
	data->status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell, 
											NULL);
	data->text = g_strdup (text);
	
	return g_timeout_add_full (G_PRIORITY_DEFAULT, 100,  
							   (GSourceFunc) status_pulse_timer, data,
							   (GDestroyNotify) on_pulse_timer_destroyed);
}

void
git_clear_status_bar_progress_pulse (guint timer_id)
{
	g_source_remove (timer_id);
}

void
git_report_errors (AnjutaCommand *command, guint return_code)
{
	gchar *message;
	
	/* In some cases, git might report errors yet still indicate success.
	 * When this happens, use a warning dialog instead of an error, so the user
	 * knows that something actually happened. */
	message = anjuta_command_get_error_message (command);
	
	if (message)
	{
		if (return_code != 0)
			anjuta_util_dialog_error (NULL, message);
		else
			anjuta_util_dialog_warning (NULL, message);
		
		g_free (message);
	}
}

static void
stop_pulse_timer (gpointer timer_id, GtkProgressBar *progress_bar)
{
	g_source_remove (GPOINTER_TO_UINT (timer_id));
}

void
git_pulse_progress_bar (GtkProgressBar *progress_bar)
{
	guint timer_id;
	
	timer_id = g_timeout_add (100, (GSourceFunc) pulse_timer, 
							  progress_bar);
	g_object_set_data (G_OBJECT (progress_bar), "pulse-timer-id",
					   GUINT_TO_POINTER (timer_id));
	
	g_object_weak_ref (G_OBJECT (progress_bar),
					   (GWeakNotify) stop_pulse_timer,
					   GUINT_TO_POINTER (timer_id));
}

gchar *
git_get_filename_from_full_path (gchar *path)
{
	gchar *last_slash;
	
	last_slash = strrchr (path, '/');
	
	/* There might be a trailing slash in the string */
	if ((last_slash - path) < strlen (path))
		return g_strdup (last_slash + 1);
	else
		return g_strdup ("");
}

const gchar *
git_get_relative_path (const gchar *path, const gchar *working_directory)
{
	/* Path could already be a relative path */
	if (strstr (path, working_directory))
		return path + strlen (working_directory) + 1;
	else 
		return path;
}

void
on_git_command_finished (AnjutaCommand *command, guint return_code, 
						 gpointer user_data)
{
	git_report_errors (command, return_code);
	
	g_object_unref (command);
}

void
on_git_status_command_data_arrived (AnjutaCommand *command, 
									AnjutaVcsStatusTreeView *tree_view)
{
	GQueue *status_queue;
	GitStatus *status;
	gchar *path;
	
	status_queue = git_status_command_get_status_queue (GIT_STATUS_COMMAND (command));
	
	while (g_queue_peek_head (status_queue))
	{
		status = g_queue_pop_head (status_queue);
		path = git_status_get_path (status);
		
		anjuta_vcs_status_tree_view_add (tree_view, path, 
								  git_status_get_vcs_status (status),
								  FALSE);
		
		g_object_unref (status);
		g_free (path);
	}
}

void
on_git_command_info_arrived (AnjutaCommand *command, Git *plugin)
{
	GQueue *info;
	gchar *message;
	
	info = git_command_get_info_queue (GIT_COMMAND (command));
	
	while (g_queue_peek_head (info))
	{
		message = g_queue_pop_head (info);
		ianjuta_message_view_append (plugin->message_view, 
								     IANJUTA_MESSAGE_VIEW_TYPE_INFO,
									 message, "", NULL);
		g_free (message);
	}
}

void
on_git_command_progress (AnjutaCommand *command, gfloat progress, 
						 GitProgressData *data)
{
	AnjutaStatus *status;
	gint ticks;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (data->plugin)->shell, 
									  NULL);
	
	/* There are cases where there are multiple stages to a task, and each 
	 * has their own progress indicator. If one stage has completed and another
	 * is beginning, add another 100 ticks to reflect the progress of this new
	 * stage. */
	if (data->last_progress == 100)
	{
		anjuta_status_progress_add_ticks (status, 100);
		data->last_progress = 0;
	}

	ticks = progress * 100;
	anjuta_status_progress_increment_ticks (status, 
											(ticks - data->last_progress),
											data->text);
	data->last_progress = ticks;
}

void
on_git_list_branch_combo_command_data_arrived (AnjutaCommand *command,
                                               GtkListStore *branch_combo_model)
{
	GQueue *output_queue;
	GitBranch *branch;
	GtkTreeIter iter;
	gchar *name;
	
	output_queue = git_branch_list_command_get_output (GIT_BRANCH_LIST_COMMAND (command));

	while (g_queue_peek_head (output_queue))
	{
		branch = g_queue_pop_head (output_queue);
		name = git_branch_get_name (branch);

		if (!git_branch_is_active (branch))
		{
			gtk_list_store_append (branch_combo_model, &iter);
			gtk_list_store_set (branch_combo_model, &iter, 0, name, -1);
		}

		g_object_unref (branch);
		g_free (name);
	}
}

void
on_git_list_branch_combo_command_finished (AnjutaCommand *command, 
                                           guint return_code,
                                           GtkComboBox *combo_box)
{
	gtk_combo_box_set_active (combo_box, 0);
	
	git_report_errors (command, return_code);
	g_object_unref (command);
}

void
git_select_all_status_items (GtkButton *select_all_button,
							 AnjutaVcsStatusTreeView *tree_view)
{
	anjuta_vcs_status_tree_view_select_all (tree_view);
}

void
git_clear_all_status_selections (GtkButton *clear_button,
								 AnjutaVcsStatusTreeView *tree_view)
{
	anjuta_vcs_status_tree_view_unselect_all (tree_view);
}

void
on_git_origin_check_toggled (GtkToggleButton *button, GtkWidget *widget)
{
	gtk_widget_set_sensitive (widget, !gtk_toggle_button_get_active (button));
}

void 
git_init_whole_project (Git *plugin, GtkWidget* project, gboolean active)
{
	gboolean project_loaded;
	
	project_loaded = (plugin->project_root_directory != NULL);
	
	gtk_widget_set_sensitive(project, project_loaded);
	
	if (project_loaded)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (project), active);
}	

void 
on_git_whole_project_toggled (GtkToggleButton* project, Git *plugin)
{
	GtkWidget *path_entry;
	
	path_entry = g_object_get_data (G_OBJECT (project), "file-entry");
	
	gtk_widget_set_sensitive (path_entry, 
							  !gtk_toggle_button_get_active (project));
}

void
git_send_raw_command_output_to_editor (AnjutaCommand *command, 
									   IAnjutaEditor *editor)
{
	GQueue *output;
	gchar *line;
	
	output = git_raw_output_command_get_output (GIT_RAW_OUTPUT_COMMAND (command));
	
	while (g_queue_peek_head (output))
	{
		line = g_queue_pop_head (output);
		ianjuta_editor_append (editor, line, strlen (line), NULL);
		g_free (line);
	}
}

void
on_git_diff_command_finished (AnjutaCommand *command, guint return_code, 
							  Git *plugin)
{
	AnjutaStatus *status;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell,
									  NULL);
	
	anjuta_status (status, _("Git: Diff complete."), 5);
	
	git_report_errors (command, return_code);
	
	g_object_unref (command);	
}

void
git_stop_status_bar_progress_pulse (AnjutaCommand *command, guint return_code,
									gpointer timer_id)
{
	git_clear_status_bar_progress_pulse (GPOINTER_TO_UINT (timer_id));
}

void
git_hide_pulse_progress_bar (AnjutaCommand *command, guint return_code,
							 GtkProgressBar *progress_bar)
{
	guint timer_id;
	
	/* If the progress bar has already been destroyed, the timer should be 
	 * stopped by stop_pulse_timer */
	if (GTK_IS_PROGRESS_BAR (progress_bar))
	{
		timer_id = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (progress_bar),
														"pulse-timer-id")); 
		
		g_source_remove (GPOINTER_TO_UINT (timer_id));
		gtk_widget_hide (GTK_WIDGET (progress_bar));
	}
}

/* This function is normally intended to disconnect stock data-arrived signal
 * handlers in this file. It is assumed that object is the user data for the 
 * callback. If you use any of the stock callbacks defined here, make sure 
 * to weak ref its target with this callback. Make sure to cancel this ref
 * by connecting git_cancel_data_arrived_signal_disconnect to the  
 * command-finished signal so we don't try to disconnect signals on a destroyed 
 * command. */
void
git_disconnect_data_arrived_signals (AnjutaCommand *command, GObject *object)
{
	guint data_arrived_signal;
	
	if (ANJUTA_IS_COMMAND (command))
	{
		data_arrived_signal = g_signal_lookup ("data-arrived",
											   ANJUTA_TYPE_COMMAND);
		
		g_signal_handlers_disconnect_matched (command,
											  G_SIGNAL_MATCH_DATA,
											  data_arrived_signal,
											  0,
											  NULL,
											  NULL,
											  object);
	}
										  
}

void 
git_cancel_data_arrived_signal_disconnect (AnjutaCommand *command, 
										   guint return_code,
										   GObject *signal_target)
{
	g_object_weak_unref (signal_target, 
						 (GWeakNotify) git_disconnect_data_arrived_signals,
						 command);
}
