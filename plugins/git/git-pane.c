/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * git-shell-test
 * Copyright (C) James Liggett 2010 <jrliggett@cox.net>
 * 
 * git-shell-test is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * git-shell-test is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "git-pane.h"



G_DEFINE_ABSTRACT_TYPE (GitPane, git_pane, ANJUTA_TYPE_DOCK_PANE);

static void
git_pane_init (GitPane *object)
{
	/* TODO: Add initialization code here */
}

static void
git_pane_finalize (GObject *object)
{
	/* TODO: Add deinitalization code here */

	G_OBJECT_CLASS (git_pane_parent_class)->finalize (object);
}

static void
git_pane_class_init (GitPaneClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
#if 0
	AnjutaDockPaneClass* parent_class = ANJUTA_DOCK_PANE_CLASS (klass);
#endif

	object_class->finalize = git_pane_finalize;
}

static void
on_message_view_destroyed (Git* plugin, gpointer destroyed_view)
{
	plugin->message_view = NULL;
}

void
git_pane_create_message_view (Git *plugin)
{
	IAnjutaMessageManager *message_manager; 
		
		
	message_manager = anjuta_shell_get_interface  (ANJUTA_PLUGIN (plugin)->shell,	
												   IAnjutaMessageManager, NULL);
	plugin->message_view = ianjuta_message_manager_get_view_by_name (message_manager, 
																	 _("Git"), 
																	 NULL);
	if (!plugin->message_view)
	{
		plugin->message_view = ianjuta_message_manager_add_view (message_manager, 
																 _("Git"), 
																 ICON_FILE, 
																 NULL);
		g_object_weak_ref (G_OBJECT (plugin->message_view), 
						   (GWeakNotify) on_message_view_destroyed, plugin);
	}
	
	ianjuta_message_view_clear (plugin->message_view, NULL);
	ianjuta_message_manager_set_current_view (message_manager, 
											  plugin->message_view, 
											  NULL);
}

void 
git_pane_on_command_info_arrived (AnjutaCommand *command, Git *plugin)
{
	GQueue *info;
	gchar *message;
	
	info = git_command_get_info_queue (GIT_COMMAND (command));
	
	while (g_queue_peek_head (info))
	{
		message = g_queue_pop_head (info);
		ianjuta_message_view_append (plugin->message_view, 
								     IANJUTA_MESSAGE_VIEW_TYPE_NORMAL,
									 message, "", NULL);
		g_free (message);
	}
}

void
git_pane_set_log_view_column_label (GtkTextBuffer *buffer, 
                                    GtkTextIter *location,
                                    GtkTextMark *mark,
                                    GtkLabel *column_label)
{
	gint column;
	gchar *text;

	column = gtk_text_iter_get_line_offset (location) + 1;
	text = g_strdup_printf (_("Column %i"), column);

	gtk_label_set_text (column_label, text);

	g_free (text);
}

gchar *
git_pane_get_log_from_text_view (GtkTextView *text_view)
{
	GtkTextBuffer *log_buffer;
	GtkTextIter start_iter;
	GtkTextIter end_iter;
	gchar *log;

	log_buffer = gtk_text_view_get_buffer(text_view);
	
	gtk_text_buffer_get_start_iter(log_buffer, &start_iter);
	gtk_text_buffer_get_end_iter(log_buffer, &end_iter) ;

	log = gtk_text_buffer_get_text(log_buffer, &start_iter, &end_iter, FALSE);
	
	return log;
}

gboolean
git_pane_check_input (GtkWidget *parent, GtkWidget *widget, const gchar *input,
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
		                                 "%s",
										 error_message);

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		gtk_window_set_focus (GTK_WINDOW (parent), widget);
	}


	return ret;
}

static void
message_dialog (GtkMessageType message_type, const gchar *message, Git *plugin)
{
	const gchar *dialog_title;
	GtkWidget *image;
	GtkWidget *dialog;
	GtkWidget *close_button;
	GtkWidget *content_area;
	GtkWidget *hbox;
	GtkWidget *scrolled_window;
	GtkWidget *text_view;
	GtkTextBuffer *text_buffer;

	dialog_title = NULL;
	image = gtk_image_new ();

	switch (message_type)
	{
		case GTK_MESSAGE_ERROR:
			gtk_image_set_from_icon_name (GTK_IMAGE (image), 
			                              GTK_STOCK_DIALOG_ERROR, 
			                              GTK_ICON_SIZE_DIALOG);
			dialog_title = _("Git Error");
			break;
		case GTK_MESSAGE_WARNING:
			gtk_image_set_from_icon_name (GTK_IMAGE (image), 
			                              GTK_STOCK_DIALOG_WARNING,
			                              GTK_ICON_SIZE_DIALOG);
			dialog_title = _("Git Warning");
			break;
		default:
			break;
	}
		

	dialog = gtk_dialog_new_with_buttons (dialog_title,
	                                      GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
	                                      GTK_DIALOG_DESTROY_WITH_PARENT,
	                                      NULL);

	close_button = gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CLOSE,
	                                      GTK_RESPONSE_CLOSE);
	content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
	hbox = gtk_hbox_new (FALSE, 2);
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	text_view = gtk_text_view_new ();
	text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));

	gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
	gtk_widget_set_size_request (text_view, 500, 150);
	
	gtk_container_add (GTK_CONTAINER (scrolled_window), text_view);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
	                                     GTK_SHADOW_IN);

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
	                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_text_view_set_editable (GTK_TEXT_VIEW (text_view), FALSE);
	gtk_text_buffer_set_text (text_buffer, message, strlen (message));

	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), scrolled_window, TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (content_area), hbox, TRUE, TRUE, 0);

	gtk_widget_grab_default (close_button);
	gtk_widget_grab_focus (close_button);

	g_signal_connect (G_OBJECT (dialog), "response",
	                  G_CALLBACK (gtk_widget_destroy),
	                  NULL);

	gtk_widget_show_all (dialog);
	
}

void
git_pane_report_errors (AnjutaCommand *command, guint return_code, Git *plugin)
{
	gchar *message;
	
	/* In some cases, git might report errors yet still indicate success.
	 * When this happens, use a warning dialog instead of an error, so the user
	 * knows that something actually happened. */
	message = anjuta_command_get_error_message (command);
	
	if (message)
	{
		if (return_code != 0)
			message_dialog (GTK_MESSAGE_ERROR, message, plugin);
		else
			message_dialog (GTK_MESSAGE_WARNING, message, plugin);
		
		g_free (message);
	}
}

