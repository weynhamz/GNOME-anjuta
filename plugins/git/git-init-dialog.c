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

#include "git-init-dialog.h"

/* TODO: Implement a working dialog when the need arises. */

static void
git_init (Git *plugin)
{
	GitInitCommand *init_command;
	
	init_command = git_init_command_new (plugin->project_root_directory);

	git_create_message_view (plugin);
	
	g_signal_connect (G_OBJECT (init_command), "data-arrived",
					  G_CALLBACK (on_git_command_info_arrived),
					  plugin);
	
	g_signal_connect (G_OBJECT (init_command), "command_finished",
					  G_CALLBACK (on_git_command_finished),
					  NULL);
	
	anjuta_command_start (ANJUTA_COMMAND (init_command));
	
}

void
on_menu_git_init (GtkAction *action, Git *plugin)
{
	git_init (plugin);
}
