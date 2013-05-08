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

#include "git-unstage-pane.h"

void
on_unstage_button_clicked (GtkAction *action, Git *plugin)
{
	GList *paths;
	GitResetFilesCommand *reset_command;

	paths = git_status_pane_get_checked_commit_items (GIT_STATUS_PANE (plugin->status_pane),
	                                                  ANJUTA_VCS_STATUS_ALL);

	if (paths)
	{
		reset_command = git_reset_files_command_new (plugin->project_root_directory,
		                                             GIT_RESET_FILES_HEAD,
		                                             paths);

		anjuta_util_glist_strings_free (paths);

		g_signal_connect (G_OBJECT (reset_command), "command-finished",
		                  G_CALLBACK (git_pane_report_errors),
		                  plugin);


		g_signal_connect (G_OBJECT (reset_command), "command-finished",
		                  G_CALLBACK (g_object_unref),
		                  NULL);

		anjuta_command_start (ANJUTA_COMMAND (reset_command));
	}
	else
		anjuta_util_dialog_error (NULL, _("No staged files selected."));
}

void
on_git_status_unstage_activated (GtkAction *action, Git *plugin)
{
	gchar *path;
	GList *paths;
	GitResetFilesCommand *reset_command;

	path = git_status_pane_get_selected_commit_path (GIT_STATUS_PANE (plugin->status_pane));

	if (path)
	{
		paths = g_list_append (NULL, path);

		reset_command = git_reset_files_command_new (plugin->project_root_directory,
		                                             GIT_RESET_FILES_HEAD,
		                                             paths);

		g_signal_connect (G_OBJECT (reset_command), "command-finished",
		                  G_CALLBACK (git_pane_report_errors),
		                  plugin);

		g_signal_connect (G_OBJECT (reset_command), "command-finished",
		                  G_CALLBACK (g_object_unref),
		                  NULL);

		anjuta_util_glist_strings_free (paths);

		anjuta_command_start (ANJUTA_COMMAND (reset_command));
	}
}
 