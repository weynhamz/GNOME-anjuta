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
 
#include "git-apply-stash-dialog.h"

static void
on_apply_stash_dialog_response (GtkDialog *dialog, gint response, 
								GitUIData *data)
{
	GtkWidget *apply_stash_view;
	GtkWidget *apply_stash_restore_index_check;
	GtkTreeSelection *selection;
	gchar *stash;
	GitStashApplyCommand *apply_command;

	if (response == GTK_RESPONSE_OK)
	{
		apply_stash_view = GTK_WIDGET (gtk_builder_get_object (data->bxml,
															   "apply_stash_view"));
		apply_stash_restore_index_check = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																			  "apply_stash_restore_index_check"));

		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (apply_stash_view));
		stash = NULL;

		if (!git_get_selected_stash (selection, &stash))
			return;

		apply_command = git_stash_apply_command_new (data->plugin->project_root_directory,
		                                             gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (apply_stash_restore_index_check)),
		                                             stash);

		git_create_message_view (data->plugin);

		g_signal_connect (G_OBJECT (apply_command), "command-finished",
		                  G_CALLBACK (on_git_stash_apply_command_finished),
		                  data->plugin);

		g_signal_connect (G_OBJECT (apply_command), "data-arrived",
		                  G_CALLBACK (on_git_command_info_arrived),
		                  data->plugin);

		anjuta_command_start (ANJUTA_COMMAND (apply_command));
	}

	git_ui_data_free (data);
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
apply_stash_dialog (Git *plugin)
{
	GtkBuilder *bxml;
	gchar *objects[] = {"apply_stash_dialog", "stash_list_model", NULL};
	GError *error;
	GtkWidget *dialog;
	GtkWidget *apply_stash_view;
	GtkListStore *stash_widget_model;
	GitUIData *data;

	bxml = gtk_builder_new ();
	error = NULL;

	if (!gtk_builder_add_objects_from_file (bxml, BUILDER_FILE, objects, 
	                                        &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}

	dialog = GTK_WIDGET (gtk_builder_get_object (bxml, "apply_stash_dialog"));
	apply_stash_view = GTK_WIDGET (gtk_builder_get_object (bxml, 
														"apply_stash_view"));
	stash_widget_model = git_stash_widget_get_model (plugin);

	gtk_tree_view_set_model (GTK_TREE_VIEW (apply_stash_view),
							 GTK_TREE_MODEL (stash_widget_model));

	data = git_ui_data_new (plugin, bxml);

	g_signal_connect (G_OBJECT (dialog), "response",
					  G_CALLBACK (on_apply_stash_dialog_response),
					  data);

	gtk_widget_show_all (dialog);
}

void
on_menu_git_apply_stash (GtkAction *action, Git *plugin)
{
	apply_stash_dialog (plugin);
}