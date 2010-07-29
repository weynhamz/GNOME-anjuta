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

#include "git-rebase-pane.h"

static void
start_rebase_command (Git *plugin, AnjutaCommand *command)
{
	git_pane_create_message_view (plugin);

	g_signal_connect (G_OBJECT (command), "command-finished",
	                  G_CALLBACK (git_pane_report_errors),
	                  plugin);


	g_signal_connect (G_OBJECT (command), "command-finished",
	                  G_CALLBACK (g_object_unref),
	                  NULL);

	g_signal_connect (G_OBJECT (command), "data-arrived",
	                  G_CALLBACK (git_pane_on_command_info_arrived),
	                  plugin);
	
	anjuta_command_start (command);
}

void
on_rebase_start_button_clicked (GtkAction *action, Git *plugin)
{
	gchar *remote;
	GitRebaseStartCommand *start_command;

	remote = git_remotes_pane_get_selected_remote (GIT_REMOTES_PANE (plugin->remotes_pane));

	if (remote)
	{
		start_command = git_rebase_start_command_new (plugin->project_root_directory,
		                                              remote);

		g_free (remote);

		start_rebase_command (plugin, ANJUTA_COMMAND (start_command));
	}
	else
		anjuta_util_dialog_error (NULL, _("No remote selected"));
}

void
on_rebase_continue_button_clicked (GtkAction *action, Git *plugin)
{
	GitRebaseContinueCommand *continue_command;

	continue_command = git_rebase_continue_command_new (plugin->project_root_directory,
	                                                    GIT_REBASE_CONTINUE_ACTION_CONTINUE);

	start_rebase_command (plugin, ANJUTA_COMMAND (continue_command));
}

void
on_rebase_skip_button_clicked (GtkAction *action, Git *plugin)
{
	GitRebaseContinueCommand *continue_command;

	continue_command = git_rebase_continue_command_new (plugin->project_root_directory,
	                                                    GIT_REBASE_CONTINUE_ACTION_SKIP);

	start_rebase_command (plugin, ANJUTA_COMMAND (continue_command));
}

void
on_rebase_abort_button_clicked (GtkAction *action, Git *plugin)
{
	GitRebaseContinueCommand *continue_command;

	continue_command = git_rebase_continue_command_new (plugin->project_root_directory,
	                                                    GIT_REBASE_CONTINUE_ACTION_ABORT);

	start_rebase_command (plugin, ANJUTA_COMMAND (continue_command));
}
 