/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-git
 * Copyright (C) James Liggett 2008 <jrliggett@cox.net>
 * 
 * anjuta-git is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * anjuta-git is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with anjuta-git.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include "git-fetch-command.h"

G_DEFINE_TYPE (GitFetchCommand, git_fetch_command, GIT_TYPE_COMMAND);

static void
git_fetch_command_init (GitFetchCommand *self)
{
}

static void
git_fetch_command_finalize (GObject *object)
{	
	G_OBJECT_CLASS (git_fetch_command_parent_class)->finalize (object);
}

static guint
git_fetch_command_run (AnjutaCommand *command)
{
	git_command_add_arg (GIT_COMMAND (command), "fetch");
	git_command_add_arg (GIT_COMMAND (command), "-q");
	
	return 0;
}

static void
git_fetch_command_class_init (GitFetchCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GitCommandClass *parent_class = GIT_COMMAND_CLASS (klass);
	AnjutaCommandClass* command_class = ANJUTA_COMMAND_CLASS (klass);
	
	object_class->finalize = git_fetch_command_finalize;
	parent_class->output_handler = git_command_send_output_to_info;
	command_class->run = git_fetch_command_run;
}


GitFetchCommand *
git_fetch_command_new (const gchar *working_directory)
{	
	return g_object_new (GIT_TYPE_FETCH_COMMAND,
						 "working-directory", working_directory,
						 "single-line-output", TRUE,
						 NULL);
}
