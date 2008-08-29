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

#include "git-ignore-command.h"

struct _GitIgnoreCommandPriv
{
	GList *paths;
};

G_DEFINE_TYPE (GitIgnoreCommand, git_ignore_command, GIT_TYPE_FILE_COMMAND);

static void
git_ignore_command_init (GitIgnoreCommand *self)
{
	self->priv = g_new0 (GitIgnoreCommandPriv, 1);
}

static void
git_ignore_command_finalize (GObject *object)
{
	GitIgnoreCommand *self;
	
	self = GIT_IGNORE_COMMAND (object);
	
	git_command_free_path_list (self->priv->paths);
	g_free (self->priv);
	
	G_OBJECT_CLASS (git_ignore_command_parent_class)->finalize (object);
}

static guint
git_ignore_command_run (AnjutaCommand *command)
{
	GitIgnoreCommand *self;
	gchar *working_directory;
	GList *current_path;
	gchar *ignore_file_path;
	GFile *ignore_file;
	GFile *ignore_file_parent;
	GFile *gitignore_file;
	gchar *ignore_path_basename;
	GFileOutputStream *gitignore_stream;
	
	self = GIT_IGNORE_COMMAND (command);
	g_object_get (self, "working-directory", &working_directory, NULL);
	current_path = self->priv->paths;
	
	while (current_path)
	{
		ignore_file_path = g_build_filename (working_directory, 
											 current_path->data, NULL);
		ignore_file = g_file_new_for_path (ignore_file_path);
		ignore_file_parent = g_file_get_parent (ignore_file);
		gitignore_file = g_file_get_child (ignore_file_parent, ".gitignore");
		ignore_path_basename = g_file_get_basename (ignore_file);
		gitignore_stream = g_file_append_to (gitignore_file, 0, NULL, NULL);
		
		g_output_stream_write (G_OUTPUT_STREAM (gitignore_stream), 
							   ignore_path_basename,
							   strlen (ignore_path_basename),
							   NULL, NULL);
		g_output_stream_write (G_OUTPUT_STREAM (gitignore_stream),
							   "\n", 1, NULL, NULL);
		
		g_free (ignore_file_path);
		g_free (ignore_path_basename);
		g_object_unref (ignore_file);
		g_object_unref (ignore_file_parent);
		g_object_unref (gitignore_file);
		g_object_unref (gitignore_stream);
		
		current_path = g_list_next (current_path);
	}
	
	g_free (working_directory);
	
	return 0;
}

static void
git_ignore_command_class_init (GitIgnoreCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaCommandClass* command_class = ANJUTA_COMMAND_CLASS (klass);
	
	object_class->finalize = git_ignore_command_finalize;
	command_class->run = git_ignore_command_run;
}


GitIgnoreCommand *
git_ignore_command_new_path (const gchar *working_directory, const gchar *path)
{
	GitIgnoreCommand *self;
	
	self = g_object_new (GIT_TYPE_IGNORE_COMMAND,
						 "working-directory", working_directory,
						 NULL);
	
	self->priv->paths = g_list_append (self->priv->paths, g_strdup (path));
	
	return self;
}


GitIgnoreCommand *
git_ignore_command_new_list (const gchar *working_directory, GList *path_list)
{
	GitIgnoreCommand *self;
	
	self = g_object_new (GIT_TYPE_IGNORE_COMMAND,
						 "working-directory", working_directory,
						 NULL);
	
	self->priv->paths = git_command_copy_path_list (path_list);
	
	return self;
}
