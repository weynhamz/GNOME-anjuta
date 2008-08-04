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

#include "git-fetch-dialog.h"

/* FIXME: We don't really have a dialog yet. We would probably need one to 
 * support some advanced options of fetch, though. Make the dialog when we 
 * implment those. */

static void
on_fetch_command_finished (AnjutaCommand *command, guint return_code, 
						   Git *plugin)
{
	AnjutaStatus *status;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell,
									  NULL);
	
	anjuta_status (status, _("Git: Fetch complete."), 5);
	
	git_report_errors (command, return_code);
	
	g_object_unref (command);
				   
}

static void
git_fetch (Git *plugin)
{
	GitProgressData *data;
	GitFetchCommand *fetch_command;
	
	data = git_progress_data_new (plugin, _("Git: Fetching..."));
	fetch_command = git_fetch_command_new (plugin->project_root_directory);
	
	g_signal_connect (G_OBJECT (fetch_command), "data-arrived",
					  G_CALLBACK (on_git_command_info_arrived),
					  plugin);
	
	g_signal_connect (G_OBJECT (fetch_command), "command-finished",
					  G_CALLBACK (on_fetch_command_finished),
					  plugin);
	
	g_signal_connect_swapped (G_OBJECT (fetch_command), "command-finished",
							  G_CALLBACK (git_progress_data_free),
							  data);
	
	g_signal_connect (G_OBJECT (fetch_command), "progress",
					  G_CALLBACK (on_git_command_progress),
					  data);
	
	git_create_message_view (plugin);
	
	anjuta_command_start (ANJUTA_COMMAND (fetch_command));
	
}

void
on_menu_git_fetch (GtkAction *action, Git *plugin)
{
	git_fetch (plugin);
}

