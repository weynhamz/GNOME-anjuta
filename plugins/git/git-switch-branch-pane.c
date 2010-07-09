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

#include "git-switch-branch-pane.h"

void
on_switch_branch_button_clicked (GtkAction *action, Git *plugin)
{
	gchar *selected_branch;
	GitBranchCheckoutCommand *checkout_command;

	selected_branch = git_branches_pane_get_selected_branch (GIT_BRANCHES_PANE (plugin->branches_pane));

	if (selected_branch)
	{
		checkout_command = git_branch_checkout_command_new (plugin->project_root_directory,
		                                                    selected_branch);

		g_free (selected_branch);

		g_signal_connect (G_OBJECT (checkout_command), "command-finished",
		                  G_CALLBACK (git_pane_report_errors),
		                  plugin);


		g_signal_connect (G_OBJECT (checkout_command), "command-finished",
		                  G_CALLBACK (g_object_unref),
		                  NULL);
		
		anjuta_command_start (ANJUTA_COMMAND (checkout_command));
	}
}
 