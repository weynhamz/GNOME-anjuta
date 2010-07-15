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

#include "git-resolve-conflicts-pane.h"

void
on_resolve_conflicts_button_clicked (GtkAction *action, Git *plugin)
{
	GList *paths;
	GitAddCommand *add_command;

	paths = git_status_pane_get_all_selected_items (GIT_STATUS_PANE (plugin->status_pane),
	                                                ANJUTA_VCS_STATUS_CONFLICTED);

	if (paths)
	{
		add_command = git_add_command_new_list (plugin->project_root_directory,
		                                        paths, FALSE);

		git_command_free_string_list (paths);

		g_signal_connect (G_OBJECT (add_command), "command-finished",
		                  G_CALLBACK (git_pane_report_errors),
		                  plugin);


		g_signal_connect (G_OBJECT (add_command), "command-finished",
		                  G_CALLBACK (g_object_unref),
		                  NULL);

		anjuta_command_start (ANJUTA_COMMAND (add_command));
	}
	else
		anjuta_util_dialog_error (NULL, _("No conflicted files selected."));
}
 