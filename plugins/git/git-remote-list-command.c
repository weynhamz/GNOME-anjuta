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

#include "git-remote-list-command.h"

struct _GitRemoteListCommandPriv
{
	GFileMonitor *file_monitor;
};

G_DEFINE_TYPE (GitRemoteListCommand, git_remote_list_command, GIT_TYPE_RAW_OUTPUT_COMMAND);

static void
git_remote_list_command_init (GitRemoteListCommand *self)
{
	self->priv = g_new0 (GitRemoteListCommandPriv, 1);
}

static void
git_remote_list_command_finalize (GObject *object)
{
	GitRemoteListCommand *self;

	self = GIT_REMOTE_LIST_COMMAND (object);

	anjuta_command_stop_automatic_monitor (ANJUTA_COMMAND (self));

	g_free (self->priv);

	G_OBJECT_CLASS (git_remote_list_command_parent_class)->finalize (object);
}

static guint
git_remote_list_command_run (AnjutaCommand *command)
{	
	git_command_add_arg (GIT_COMMAND (command), "remote");
	
	return 0;
}

static void
on_file_monitor_changed (GFileMonitor *monitor, GFile *file, GFile *other_file,
                         GFileMonitorEvent event, AnjutaCommand *command)
{
	/* Git recreates the config file when it changes */
	if (event == G_FILE_MONITOR_EVENT_CREATED)
		anjuta_command_start (command);
}

static gboolean
git_remote_list_command_start_automatic_monitor (AnjutaCommand *command)
{
	GitRemoteListCommand *self;
	gchar *working_directory;
	gchar *git_config_path;
	GFile *git_config_file;

	self = GIT_REMOTE_LIST_COMMAND (command);
	g_object_get (G_OBJECT (self), "working-directory", &working_directory, 
	              NULL);
	git_config_path = g_strjoin (G_DIR_SEPARATOR_S,
	                             working_directory,
	                             ".git",
	                             "config",
	                             NULL);
	git_config_file = g_file_new_for_path (git_config_path);

	self->priv->file_monitor = g_file_monitor_file (git_config_file, 0, NULL, 
	                                                NULL);

	g_signal_connect (G_OBJECT (self->priv->file_monitor ), "changed",
	                  G_CALLBACK (on_file_monitor_changed),
	                  self);

	g_free (git_config_path);
	g_free (working_directory);
	g_object_unref (git_config_file);

	return TRUE;
}

static void
git_remote_list_command_stop_automatic_monitor (AnjutaCommand *command)
{
	GitRemoteListCommand *self;

	self = GIT_REMOTE_LIST_COMMAND (command);

	if (self->priv->file_monitor)
	{
		g_file_monitor_cancel (self->priv->file_monitor);
		g_object_unref (self->priv->file_monitor);
		self->priv->file_monitor = NULL;
	}
}

static void
git_remote_list_command_class_init (GitRemoteListCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaCommandClass *command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = git_remote_list_command_finalize;
	command_class->run = git_remote_list_command_run;
	command_class->start_automatic_monitor = git_remote_list_command_start_automatic_monitor;
	command_class->stop_automatic_monitor = git_remote_list_command_stop_automatic_monitor;
}


GitRemoteListCommand *
git_remote_list_command_new (const gchar *working_directory)
{
	return g_object_new (GIT_TYPE_REMOTE_LIST_COMMAND, 
						 "working-directory", working_directory,
						 "single-line-output", TRUE,
						 "strip-newlines", TRUE,
						 NULL);
}
