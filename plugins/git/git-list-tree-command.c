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

#include "git-list-tree-command.h"

#define BLOB_REGEX "blob ([[:xdigit:]]{40})(?:\\t)(.*)"

struct _GitListTreeCommandPriv
{
	gchar *commit_sha;
	GRegex *blob_regex;
	GHashTable *blobs;
};

G_DEFINE_TYPE (GitListTreeCommand, git_list_tree_command, GIT_TYPE_COMMAND);

static void
git_list_tree_command_init (GitListTreeCommand *self)
{
	self->priv = g_new0 (GitListTreeCommandPriv, 1);
	
	self->priv->blob_regex = g_regex_new (BLOB_REGEX, 0, 0, NULL);
	self->priv->blobs = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, 
											   g_free);
}

static void
git_list_tree_command_finalize (GObject *object)
{
	GitListTreeCommand *self;
	
	self = GIT_LIST_TREE_COMMAND (object);
	
	g_free (self->priv->commit_sha);
	g_regex_unref (self->priv->blob_regex);
	g_hash_table_unref (self->priv->blobs);
	
	g_free (self->priv);

	G_OBJECT_CLASS (git_list_tree_command_parent_class)->finalize (object);
}

static guint
git_list_tree_command_run (AnjutaCommand *command)
{
	GitListTreeCommand *self;
	
	self = GIT_LIST_TREE_COMMAND (command);
	
	git_command_add_arg (GIT_COMMAND (command), "ls-tree");
	git_command_add_arg (GIT_COMMAND (command), self->priv->commit_sha);
	
	return 0;
}

static void
git_list_tree_command_handle_output (GitCommand *git_command, 
									 const gchar *output)
{
	GitListTreeCommand *self;
	GMatchInfo *match_info;
	gchar *blob_sha;
	gchar *filename;
	
	self = GIT_LIST_TREE_COMMAND (git_command);
	match_info = NULL;
	
	if (g_regex_match (self->priv->blob_regex, output, 0, &match_info))
	{
		blob_sha = g_match_info_fetch (match_info, 1);
		filename = g_match_info_fetch (match_info, 2);
		
		g_hash_table_insert (self->priv->blobs, g_strdup (filename),
							 g_strdup (blob_sha));
		
		g_free (blob_sha);
		g_free (filename);
	}
	
	if (match_info)
		g_match_info_free (match_info);
}

static void
git_list_tree_command_class_init (GitListTreeCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GitCommandClass* parent_class = GIT_COMMAND_CLASS (klass);
	AnjutaCommandClass *command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = git_list_tree_command_finalize;
	parent_class->output_handler = git_list_tree_command_handle_output;
	command_class->run = git_list_tree_command_run;
}


GitListTreeCommand *
git_list_tree_command_new (const gchar *working_directory, 
						   const gchar *commit_sha)
{
	GitListTreeCommand *self;
	
	self = g_object_new (GIT_TYPE_LIST_TREE_COMMAND, 
						 "working-directory", working_directory,
						 "single-line-output", TRUE,
						 NULL);
	
	self->priv->commit_sha = g_strdup (commit_sha);
	
	return self;
}

GHashTable *
git_list_tree_command_get_blobs (GitListTreeCommand *self)
{
	return g_hash_table_ref (self->priv->blobs);
}
