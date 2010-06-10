/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2010 <jrliggett@cox.net>
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

#include "git-commit-pane.h"

struct _GitCommitPanePriv
{
	GtkBuilder *builder;
};

G_DEFINE_TYPE (GitCommitPane, git_commit_pane, GIT_TYPE_PANE);

static void
on_amend_check_toggled (GtkToggleButton *button, GitCommitPane *self)
{
	Git *plugin;
	GtkTextView *log_view;
	GtkTextBuffer *log_text_buffer;
	gchar *commit_message_path;
	GIOChannel *io_channel;
	gchar *line;
	GtkTextIter end_iter;

	plugin = ANJUTA_PLUGIN_GIT (anjuta_dock_pane_get_plugin (ANJUTA_DOCK_PANE (self)));
	log_view = GTK_TEXT_VIEW (gtk_builder_get_object (self->priv->builder,
	                                                  "log_view"));
	log_text_buffer = gtk_text_view_get_buffer (log_view);

	if (gtk_toggle_button_get_active (button))
	{
		commit_message_path = g_strjoin (G_DIR_SEPARATOR_S,
		                                 plugin->project_root_directory,
		                                 ".git",
		                                 "COMMIT_EDITMSG",
		                                 NULL);
		io_channel = g_io_channel_new_file (commit_message_path, "r", NULL);

		while (g_io_channel_read_line (io_channel, &line, NULL, NULL, 
		                               NULL) == G_IO_STATUS_NORMAL)
		{
			/* Filter out comment lines */
			if (line[0] != '#')
			{
				gtk_text_buffer_get_end_iter (log_text_buffer, &end_iter);
				gtk_text_buffer_insert (log_text_buffer, &end_iter, line, -1);

				g_free (line);
			}
			else
			{
				g_free (line);
				break;
			}
		}

		g_free (commit_message_path);
		g_io_channel_unref (io_channel);
	}
	else
		gtk_text_buffer_set_text (log_text_buffer, "", 0);
}

static void
on_use_custom_author_info_check_toggled (GtkToggleButton *button, 
                                         GitCommitPane *self)
{
	GtkWidget *author_info_alignment;

	author_info_alignment = GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                                            "author_info_alignment"));

	gtk_widget_set_sensitive (author_info_alignment, 
	                          gtk_toggle_button_get_active (button));
}

static void
on_ok_button_clicked (GtkButton *button, GitCommitPane *self)
{
	Git *plugin;
	GtkTextView *log_view;
	GtkToggleButton *amend_check;
	GtkToggleButton *failed_merge_check;
	GtkToggleButton *use_custom_author_info_check;
	gchar *log;
	gchar *author_name;
	gchar *author_email;
	GtkEditable *name_entry;
	GtkEditable *email_entry;
	GList *selected_paths;
	GitCommitCommand *commit_command;
	
	plugin = ANJUTA_PLUGIN_GIT (anjuta_dock_pane_get_plugin (ANJUTA_DOCK_PANE(self)));
	log_view = GTK_TEXT_VIEW (gtk_builder_get_object (self->priv->builder,
	                                                  "log_view"));
	amend_check = GTK_TOGGLE_BUTTON (gtk_builder_get_object (self->priv->builder,
	                                                         "amend_check"));
	failed_merge_check = GTK_TOGGLE_BUTTON (gtk_builder_get_object (self->priv->builder,
	                                                                "failed_merge_check"));
	use_custom_author_info_check = GTK_TOGGLE_BUTTON (gtk_builder_get_object (self->priv->builder,
	                                                                          "use_custom_author_info_check"));
	log = git_pane_get_log_from_text_view (log_view);
	author_name = NULL;
	author_email = NULL;

	if (gtk_toggle_button_get_active (use_custom_author_info_check))
	{
		name_entry = GTK_EDITABLE (gtk_builder_get_object (self->priv->builder,
		                                                   "name_entry"));
		email_entry = GTK_EDITABLE (gtk_builder_get_object (self->priv->builder,
		                                                    "email_entry"));

		author_name = gtk_editable_get_chars (name_entry, 0, -1);
		author_email = gtk_editable_get_chars (email_entry, 0, -1);
	}

	selected_paths = git_status_pane_get_all_selected_items (GIT_STATUS_PANE (plugin->status_pane));

	commit_command = git_commit_command_new (plugin->project_root_directory,
	                                         gtk_toggle_button_get_active (amend_check),
	                                         gtk_toggle_button_get_active (failed_merge_check),
	                                         log,
	                                         author_name,
	                                         author_email,
	                                         selected_paths);

	g_free (log);
	g_free (author_name);
	g_free (author_email);
	git_command_free_string_list (selected_paths);

	git_pane_create_message_view (plugin);

	g_signal_connect (G_OBJECT (commit_command), "data-arrived",
	                  G_CALLBACK (git_pane_on_command_info_arrived),
	                  plugin);

	g_signal_connect (G_OBJECT (commit_command), "command-finished",
	                  G_CALLBACK (g_object_unref),
	                  NULL);

	anjuta_command_start (ANJUTA_COMMAND (commit_command));

	anjuta_dock_remove_pane (ANJUTA_DOCK (plugin->dock), 
	                         ANJUTA_DOCK_PANE (self));
}

static void
on_cancel_button_clicked (GtkButton *button, GitCommitPane *self)
{
	Git *plugin;

	plugin = ANJUTA_PLUGIN_GIT (anjuta_dock_pane_get_plugin (ANJUTA_DOCK_PANE(self)));

	anjuta_dock_remove_pane (ANJUTA_DOCK (plugin->dock), 
	                         ANJUTA_DOCK_PANE (self));
}

static void
git_commit_pane_init (GitCommitPane *self)
{
	gchar *objects[] = {"commit_pane",
						NULL};
	GError *error = NULL;
	GtkTextView *log_view;
	GtkTextBuffer *log_text_buffer;
	GtkLabel *column_label;
	GtkWidget *amend_check;
	GtkWidget *use_custom_author_info_check;
	GtkWidget *ok_button;
	GtkWidget *cancel_button;

	self->priv = g_new0 (GitCommitPanePriv, 1);
	self->priv->builder = gtk_builder_new ();

	if (!gtk_builder_add_objects_from_file (self->priv->builder, BUILDER_FILE, 
	                                        objects, 
	                                        &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}

	log_view = GTK_TEXT_VIEW (gtk_builder_get_object (self->priv->builder,
	                                                  "log_view"));
	log_text_buffer = gtk_text_view_get_buffer (log_view);
	column_label = GTK_LABEL (gtk_builder_get_object (self->priv->builder,
	                                                  "column_label"));
	amend_check = GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                                  "amend_check"));
	use_custom_author_info_check = GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                                                   "use_custom_author_info_check"));
	ok_button = GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                                "ok_button"));
	cancel_button = GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                                    "cancel_button"));

	g_signal_connect (G_OBJECT (amend_check), "toggled",
	                  G_CALLBACK (on_amend_check_toggled),
	                  self);

	g_signal_connect (G_OBJECT (log_text_buffer), "mark-set",
	                  G_CALLBACK (git_pane_set_log_view_column_label),
	                  column_label);

	g_signal_connect (G_OBJECT (use_custom_author_info_check), "toggled",
	                  G_CALLBACK (on_use_custom_author_info_check_toggled),
	                  self);

	g_signal_connect (G_OBJECT (ok_button), "clicked",
	                  G_CALLBACK (on_ok_button_clicked),
	                  self);

	g_signal_connect (G_OBJECT (cancel_button), "clicked",
	                  G_CALLBACK (on_cancel_button_clicked),
	                  self);
	
}

static void
git_commit_pane_finalize (GObject *object)
{
	GitCommitPane *self;

	self = GIT_COMMIT_PANE (object);

	g_object_unref (self->priv->builder);
	g_free (self->priv);

	G_OBJECT_CLASS (git_commit_pane_parent_class)->finalize (object);
}

static GtkWidget *
git_commit_pane_get_widget (AnjutaDockPane *pane)
{
	GitCommitPane *self;

	self = GIT_COMMIT_PANE (pane);

	return GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                           "commit_pane"));
}

static void
git_commit_pane_class_init (GitCommitPaneClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaDockPaneClass* pane_class = ANJUTA_DOCK_PANE_CLASS (klass);

	object_class->finalize = git_commit_pane_finalize;
	pane_class->get_widget = git_commit_pane_get_widget;
	pane_class->refresh = NULL;
}


AnjutaDockPane *
git_commit_pane_new (Git *plugin)
{
	return g_object_new (GIT_TYPE_COMMIT_PANE, "plugin", plugin, NULL);
}

void
on_commit_button_clicked (GtkAction *action, Git *plugin)
{
	AnjutaDockPane *pane;

	pane = git_commit_pane_new (plugin);

	anjuta_dock_add_pane (ANJUTA_DOCK (plugin->dock), "Commit", _("Commit"),
	                      NULL, pane, GDL_DOCK_BOTTOM, NULL, 0, NULL);
}
