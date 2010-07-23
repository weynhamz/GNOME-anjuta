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

#include "git-tag-list-command.h"

struct _GitTagListCommandPriv
{
	GFileMonitor *file_monitor;
};

G_DEFINE_TYPE (GitTagListCommand, git_tag_list_command, GIT_TYPE_RAW_OUTPUT_COMMAND);

static void
git_tag_list_command_init (GitTagListCommand *self)
{
	self->priv = g_new0 (GitTagListCommandPriv, 1);
}

static void
git_tag_list_command_finalize (GObject *object)
{
	GitTagListCommand *self;

	self = GIT_TAG_LIST_COMMAND (object);

	anjuta_command_stop_automatic_monitor (ANJUTA_COMMAND (object));
	g_free (self->priv);

	G_OBJECT_CLASS (git_tag_list_command_parent_class)->finalize (object);
}

static guint
git_tag_list_command_run (AnjutaCommand *command)
{	
	git_command_add_arg (GIT_COMMAND (command), "tag");
	
	return 0;
}

static void
on_file_monitor_changed (GFileMonitor *monitor, GFile *file, GFile *other_file,
                         GFileMonitorEvent event, AnjutaCommand *command)
{
	if (event == G_FILE_MONITOR_EVENT_CREATED ||
	    event == G_FILE_MONITOR_EVENT_DELETED)
	{
		anjuta_command_start (command);
	}
}

static gboolean
git_tag_list_command_start_automatic_monitor (AnjutaCommand *command)
{
	GitTagListCommand *self;
	gchar *working_directory;
	gchar *git_tags_path;
	GFile *git_tags_file;

	self = GIT_TAG_LIST_COMMAND (command);

	g_object_get (G_OBJECT (self), "working-directory", &working_directory,
	              NULL);

	git_tags_path = g_strjoin (G_DIR_SEPARATOR_S,
	                           working_directory,
	                           ".git",
	                           "refs",
	                           "tags",
	                           NULL);
	
	git_tags_file = g_file_new_for_path (git_tags_path);

	self->priv->file_monitor = g_file_monitor_directory (git_tags_file, 0, NULL, 
	                                                     NULL);

	g_signal_connect (G_OBJECT (self->priv->file_monitor), "changed",
	                  G_CALLBACK (on_file_monitor_changed),
	                  command);

	g_free (working_directory);
	g_free (git_tags_path);
	g_object_unref (git_tags_file);
	                    
	return TRUE;
}

static void
git_tag_list_command_stop_automatic_monitor (AnjutaCommand *command)
{
	GitTagListCommand *self;

	self = GIT_TAG_LIST_COMMAND (command);

	if (self->priv->file_monitor)
	{
		g_file_monitor_cancel (self->priv->file_monitor);
		g_object_unref (self->priv->file_monitor);
		self->priv->file_monitor = NULL;
	}
}

static void
git_tag_list_command_class_init (GitTagListCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaCommandClass *command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = git_tag_list_command_finalize;
	command_class->run = git_tag_list_command_run;
	command_class->start_automatic_monitor = git_tag_list_command_start_automatic_monitor;
	command_class->stop_automatic_monitor = git_tag_list_command_stop_automatic_monitor;
}


GitTagListCommand *
git_tag_list_command_new (const gchar *working_directory)
{
	return g_object_new (GIT_TYPE_TAG_LIST_COMMAND, 
						 "working-directory", working_directory,
						 "single-line-output", TRUE,
						 "strip-newlines", TRUE,
						 NULL);
}
