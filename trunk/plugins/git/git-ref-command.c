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

#include "git-ref-command.h"

#define BRANCH_REF_REGEX "([[:xdigit:]]{40}) refs/heads/(.*)"
#define TAG_REF_REGEX "([[:xdigit:]]{40}) refs/tags/(.*)"
#define REMOTE_REF_REGEX "([[:xdigit:]]{40}) refs/remotes/(.*)"

struct _GitRefCommandPriv
{
	GRegex *branch_ref_regex;
	GRegex *tag_ref_regex;
	GRegex *remote_ref_regex;
	GHashTable *refs;
};

G_DEFINE_TYPE (GitRefCommand, git_ref_command, GIT_TYPE_COMMAND);

static void
free_refs_list (GList *refs)
{
	GList *current_ref;
	
	current_ref = refs;
	
	while (current_ref)
	{
		g_object_unref (current_ref->data);
		current_ref = g_list_next (current_ref);
	}
	
	g_list_free (refs);
}

static void
git_ref_command_init (GitRefCommand *self)
{
	self->priv = g_new0 (GitRefCommandPriv, 1);
	
	self->priv->branch_ref_regex = g_regex_new (BRANCH_REF_REGEX, 0, 0, NULL);
	self->priv->tag_ref_regex = g_regex_new (TAG_REF_REGEX, 0, 0, NULL);
	self->priv->remote_ref_regex = g_regex_new (REMOTE_REF_REGEX, 0, 0, NULL);
	self->priv->refs = g_hash_table_new_full (g_str_hash, g_str_equal,
											  NULL, 
											  (GDestroyNotify) free_refs_list);
}

static void
git_ref_command_finalize (GObject *object)
{
	GitRefCommand *self;
	
	self = GIT_REF_COMMAND (object);
	
	g_regex_unref (self->priv->branch_ref_regex);
	g_regex_unref (self->priv->tag_ref_regex);
	g_regex_unref (self->priv->remote_ref_regex);
	g_hash_table_unref (self->priv->refs);
	
	g_free (self->priv);

	G_OBJECT_CLASS (git_ref_command_parent_class)->finalize (object);
}

static guint
git_ref_command_run (AnjutaCommand *command)
{
	git_command_add_arg (GIT_COMMAND (command), "show-ref");
	git_command_add_arg (GIT_COMMAND (command), "--dereference");
	
	return 0;
}

static void 
git_ref_command_insert_ref (GitRefCommand *self, const gchar *sha, GitRef *ref)
{
	GList *ref_list;
	gchar *name;
	gchar *old_sha;
	
	name = git_ref_get_name (ref);
	
	ref_list = g_hash_table_lookup (self->priv->refs, sha);
	
	ref_list = g_list_append (ref_list, ref);
	
	if (g_hash_table_lookup_extended (self->priv->refs, sha, 
									  (gpointer) &old_sha, NULL))
	{
		/* Change the list head for this SHA without destroying it */
		g_hash_table_steal (self->priv->refs, sha); 
		
		g_free (old_sha);
	}
	
	g_hash_table_insert (self->priv->refs, g_strdup (sha), ref_list);
	
	g_free (name);
}

static void
git_ref_command_handle_output (GitCommand *git_command, const gchar *output)
{
	GitRefCommand *self;
	GMatchInfo *match_info;
	gchar *sha;
	gchar *name;
	GitRef *ref;
	
	self = GIT_REF_COMMAND (git_command);
	match_info = NULL;
	
	if (g_regex_match (self->priv->branch_ref_regex, output, 0, &match_info))
	{
		sha = g_match_info_fetch (match_info, 1);
		name = g_match_info_fetch (match_info, 2);
		ref = git_ref_new (name, GIT_REF_TYPE_BRANCH);
		
		git_ref_command_insert_ref (self, sha, ref);
		
		g_free (sha);
		g_free (name);
	}
	else if (g_regex_match (self->priv->tag_ref_regex, output, 0, &match_info))
	{
		sha = g_match_info_fetch (match_info, 1);
		name = g_match_info_fetch (match_info, 2);
		
		if (g_str_has_suffix (name, "^{}"))
			(g_strrstr (name, "^{}")) [0] = '\0';
		
		ref = git_ref_new (name, GIT_REF_TYPE_TAG);
		
		
		git_ref_command_insert_ref (self, sha, ref);
		
		g_free (sha);
		g_free (name);
	}
	else if (g_regex_match (self->priv->remote_ref_regex, output, 0, 
							&match_info))
	{
		sha = g_match_info_fetch (match_info, 1);
		name = g_match_info_fetch (match_info, 2);
		ref = git_ref_new (name, GIT_REF_TYPE_REMOTE);
		
		git_ref_command_insert_ref (self, sha, ref);
		
		g_free (sha);
		g_free (name);
	}
	
	if (match_info)
		g_match_info_free (match_info);
}

static void
git_ref_command_class_init (GitRefCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GitCommandClass* parent_class = GIT_COMMAND_CLASS (klass);
	AnjutaCommandClass *command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = git_ref_command_finalize;
	parent_class->output_handler = git_ref_command_handle_output;
	command_class->run = git_ref_command_run;
}


GitRefCommand *
git_ref_command_new (const gchar *working_directory)
{
	return g_object_new (GIT_TYPE_REF_COMMAND, 
						 "working-directory", working_directory,
						 "single-line-output", TRUE,
						 NULL);
}

GHashTable *
git_ref_command_get_refs (GitRefCommand *self)
{
	return g_hash_table_ref (self->priv->refs);
}
