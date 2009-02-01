/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2007 <jrliggett@cox.net>
 *
 * Portions based on the original Subversion plugin 
 * Copyright (C) Johannes Schmid 2005 
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

#include "subversion-ui-utils.h"

/* Private structure for pulse progress */
typedef struct
{
	AnjutaStatus *status;
	gchar *text;
} PulseProgressData;

SubversionData* 
subversion_data_new (Subversion* plugin, GladeXML* gxml)
{
	SubversionData* data = g_new0(SubversionData, 1);
	data->plugin = plugin;
	data->gxml = gxml;
	
	return data;
}

void subversion_data_free(SubversionData* data)
{
	g_free(data);
}

static void
on_mesg_view_destroy(Subversion* plugin, gpointer destroyed_view)
{
	plugin->mesg_view = NULL;
}

void
create_message_view(Subversion* plugin)
{
	IAnjutaMessageManager* mesg_manager = anjuta_shell_get_interface 
		(ANJUTA_PLUGIN (plugin)->shell,	IAnjutaMessageManager, NULL);
	plugin->mesg_view = 
	    ianjuta_message_manager_get_view_by_name(mesg_manager, _("Subversion"), 
												 NULL);
	if (!plugin->mesg_view)
	{
		plugin->mesg_view =
		     ianjuta_message_manager_add_view (mesg_manager, _("Subversion"), 
											   ICON_FILE, NULL);
		g_object_weak_ref (G_OBJECT (plugin->mesg_view), 
						  (GWeakNotify)on_mesg_view_destroy, plugin);
	}
	ianjuta_message_view_clear(plugin->mesg_view, NULL);
	ianjuta_message_manager_set_current_view(mesg_manager, plugin->mesg_view, 
											 NULL);
}

gboolean 
check_input (GtkWidget *parent, GtkWidget *entry, const gchar *error_message)
{
	gchar *input;
	gboolean ret;
	GtkWidget *dialog;
	
	input = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
	
	if (strlen (input) > 0)
		ret = TRUE;
	else
	{
		dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
										 GTK_DIALOG_DESTROY_WITH_PARENT,
										 GTK_MESSAGE_WARNING,
										 GTK_BUTTONS_OK,
										 "%s", error_message);
		
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		
		gtk_window_set_focus (GTK_WINDOW (parent), entry);
		
		ret = FALSE;
	}
	
	g_free (input);
	
	return ret;
}

gchar * 
get_log_from_textview (GtkWidget* textview)
{
	gchar* log;
	GtkTextBuffer* textbuf;
	GtkTextIter iterbegin, iterend;
	
	textbuf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
	gtk_text_buffer_get_start_iter (textbuf, &iterbegin);
	gtk_text_buffer_get_end_iter (textbuf, &iterend);
	log = gtk_text_buffer_get_text (textbuf, &iterbegin, &iterend, FALSE);
	
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
status_bar_progress_pulse (Subversion *plugin, gchar *text)
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
clear_status_bar_progress_pulse (guint timer_id)
{
	g_source_remove (timer_id);
}

void
report_errors (AnjutaCommand *command, guint return_code)
{
	gchar *message;
	
	if (return_code != 0)
	{
		message = anjuta_command_get_error_message (command);
		
		anjuta_util_dialog_error (NULL, message);
		g_free (message);
	}
}

static void
stop_pulse_timer (gpointer timer_id, GtkProgressBar *progress_bar)
{
	g_source_remove (GPOINTER_TO_UINT (timer_id));
}

void
pulse_progress_bar (GtkProgressBar *progress_bar)
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
get_filename_from_full_path (gchar *path)
{
	gchar *last_slash;
	
	last_slash = strrchr (path, '/');
	
	/* There might be a trailing slash in the string */
	if ((last_slash - path) < strlen (path))
		return g_strdup (last_slash + 1);
	else
		return g_strdup ("");
}

void
on_status_command_finished (AnjutaCommand *command, guint return_code, 
							gpointer user_data)
{
	report_errors (command, return_code);
	
	svn_status_command_destroy (SVN_STATUS_COMMAND (command));
}

void
on_status_command_data_arrived (AnjutaCommand *command, 
								AnjutaVcsStatusTreeView *tree_view)
{
	GQueue *status_queue;
	SvnStatus *status;
	gchar *path;
	
	status_queue = svn_status_command_get_status_queue (SVN_STATUS_COMMAND (command));
	
	while (g_queue_peek_head (status_queue))
	{
		status = g_queue_pop_head (status_queue);
		path = svn_status_get_path (status);
		
		switch (svn_status_get_vcs_status(status)) 	 
		{ 	 
			case ANJUTA_VCS_STATUS_MODIFIED: 	 
			case ANJUTA_VCS_STATUS_ADDED: 
			case ANJUTA_VCS_STATUS_DELETED: 	 
			case ANJUTA_VCS_STATUS_CONFLICTED: 	 
			case ANJUTA_VCS_STATUS_MISSING:
				anjuta_vcs_status_tree_view_add (tree_view, path, 
												 svn_status_get_vcs_status (status),
												 FALSE);
				break;
			default:
				break;
		}
		svn_status_destroy (status);
		g_free (path);
	}
}

void
on_command_info_arrived (AnjutaCommand *command, Subversion *plugin)
{
	GQueue *info;
	gchar *message;
	
	info = svn_command_get_info_queue (SVN_COMMAND (command));
	
	while (g_queue_peek_head (info))
	{
		message = g_queue_pop_head (info);
		ianjuta_message_view_append (plugin->mesg_view, 
								     IANJUTA_MESSAGE_VIEW_TYPE_INFO,
									 message, "", NULL);
		g_free (message);
	}
}

void
select_all_status_items (GtkButton *select_all_button,
						 AnjutaVcsStatusTreeView *tree_view)
{
	anjuta_vcs_status_tree_view_select_all (tree_view);
}

void
clear_all_status_selections (GtkButton *clear_button,
							 AnjutaVcsStatusTreeView *tree_view)
{
	anjuta_vcs_status_tree_view_unselect_all (tree_view);
}

void 
init_whole_project (Subversion *plugin, GtkWidget* project, gboolean active)
{
	gboolean project_loaded;
	
	project_loaded = (plugin->project_root_dir != NULL);
	
	gtk_widget_set_sensitive(project, project_loaded);
	
	if (project_loaded)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (project), active);
}	

void 
on_whole_project_toggled (GtkToggleButton* project, Subversion *plugin)
{
	GtkEntry* fileentry;
	
	fileentry = g_object_get_data (G_OBJECT (project), "fileentry");
	
	if (gtk_toggle_button_get_active(project) && plugin->project_root_dir)
	{
		gtk_entry_set_text (fileentry, plugin->project_root_dir);
		gtk_widget_set_sensitive(GTK_WIDGET(fileentry), FALSE);
	}
	else
		gtk_widget_set_sensitive(GTK_WIDGET(fileentry), TRUE);	
}

void
send_diff_command_output_to_editor (AnjutaCommand *command, 
									IAnjutaEditor *editor)
{
	GQueue *output;
	gchar *line;
	
	output = svn_diff_command_get_output (SVN_DIFF_COMMAND (command));
	
	while (g_queue_peek_head (output))
	{
		line = g_queue_pop_head (output);
		ianjuta_editor_append (editor, line, strlen (line), NULL);
		g_free (line);
	}
}

void
on_diff_command_finished (AnjutaCommand *command, guint return_code, 
						  Subversion *plugin)
{
	AnjutaStatus *status;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell,
									  NULL);
	
	anjuta_status (status, _("Subversion: Diff complete."), 5);
	
	report_errors (command, return_code);
	
	svn_diff_command_destroy (SVN_DIFF_COMMAND (command));	
}

void
stop_status_bar_progress_pulse (AnjutaCommand *command, guint return_code,
								gpointer timer_id)
{
	clear_status_bar_progress_pulse (GPOINTER_TO_UINT (timer_id));
}

void
hide_pulse_progress_bar (AnjutaCommand *command, guint return_code,
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
 * by connecting cancel_data_arrived_signal_disconnect to the command-finished 
 * signal so we don't try to disconnect signals on a destroyed command. */
void
disconnect_data_arrived_signals (AnjutaCommand *command, GObject *object)
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
cancel_data_arrived_signal_disconnect (AnjutaCommand *command, 
									   guint return_code,
									   GObject *signal_target)
{
	g_object_weak_unref (signal_target, 
						 (GWeakNotify) disconnect_data_arrived_signals,
						 command);
}
