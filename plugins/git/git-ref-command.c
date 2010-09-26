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
	GHashTable *file_monitors;
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
	GMatchInfo *branch_match_info;
	GMatchInfo *tag_match_info;
	GMatchInfo *remote_match_info;
	gchar *sha;
	gchar *name;
	GitRef *ref;
	
	self = GIT_REF_COMMAND (git_command);

	branch_match_info = NULL;
	tag_match_info = NULL;
	remote_match_info = NULL;
	
	if (g_regex_match (self->priv->branch_ref_regex, output, 0, 
					   &branch_match_info))
	{
		sha = g_match_info_fetch (branch_match_info, 1);
		name = g_match_info_fetch (branch_match_info, 2);
		ref = git_ref_new (name, GIT_REF_TYPE_BRANCH);
		
		git_ref_command_insert_ref (self, sha, ref);
		
		g_free (sha);
		g_free (name);
	}
	else if (g_regex_match (self->priv->tag_ref_regex, output, 0, 
							&tag_match_info))
	{
		sha = g_match_info_fetch (tag_match_info, 1);
		name = g_match_info_fetch (tag_match_info, 2);
		
		if (g_str_has_suffix (name, "^{}"))
			(g_strrstr (name, "^{}")) [0] = '\0';
		
		ref = git_ref_new (name, GIT_REF_TYPE_TAG);
		
		
		git_ref_command_insert_ref (self, sha, ref);
		
		g_free (sha);
		g_free (name);
	}
	else if (g_regex_match (self->priv->remote_ref_regex, output, 0, 
							&remote_match_info))
	{
		sha = g_match_info_fetch (remote_match_info, 1);
		name = g_match_info_fetch (remote_match_info, 2);
		ref = git_ref_new (name, GIT_REF_TYPE_REMOTE);
		
		git_ref_command_insert_ref (self, sha, ref);
		
		g_free (sha);
		g_free (name);
	}
	
	if (branch_match_info)
		g_match_info_free (branch_match_info);

	if (tag_match_info)
		g_match_info_free (tag_match_info);

	if (remote_match_info)
		g_match_info_free (remote_match_info);
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

/* Helper method to add file monitors. The command takes ownership of the 
 * file */
static void
git_ref_command_add_file_monitor (GitRefCommand *self, GFile *file)
{
	GFileMonitor *file_monitor;

	file_monitor = g_file_monitor (file, 0, NULL, NULL);

	g_signal_connect (G_OBJECT (file_monitor), "changed",
	                  G_CALLBACK (on_file_monitor_changed),
	                  self);

	/* Have the hash table take ownership of both the file and the file
	 * monitor to keep memory management consistent. */
	g_hash_table_insert (self->priv->file_monitors, file, file_monitor);
}

static void
on_remote_file_monitor_changed (GFileMonitor *monitor, GFile *file, 
                                GFile *other_file, GFileMonitorEvent event, 
                                GitRefCommand *self)
{
	switch (event)
	{
		case G_FILE_MONITOR_EVENT_CREATED:
			/* A new remote was created */
			git_ref_command_add_file_monitor (self, g_object_ref (file));

			/* Start the command to reflect changes */
			anjuta_command_start (ANJUTA_COMMAND (self));

			break;
		case G_FILE_MONITOR_EVENT_DELETED:
			/* A remote was deleted--stop monitoring it */
			g_hash_table_remove (self->priv->file_monitors, file);

			anjuta_command_start (ANJUTA_COMMAND (self));

			break;
		default:
			break;
			
	};
}

static gboolean 
git_ref_command_start_automatic_monitor (AnjutaCommand *command)
{
	GitRefCommand *self;
	gchar *working_directory;
	gchar *git_head_path;
	gchar *git_packed_refs_path;
	gchar *git_branches_path;
	gchar *git_tags_path;
	gchar *git_remotes_path;
	GFile *remotes_file;
	GFileMonitor *remotes_monitor;
	GFileEnumerator *remotes_enumerator;
	GFileInfo *remotes_info;
	GFile *current_remote_file;

	self = GIT_REF_COMMAND (command);

	g_object_get (self, "working-directory", &working_directory, NULL);

	/* The file monitors tabe keeps track of all the GFiles and GFileMonitors
	 * that we're tracking. The table takes ownership of the files and the 
	 * monitors so that cleanup can be done in one step. */
	self->priv->file_monitors = g_hash_table_new_full (g_file_hash,
	                                                   (GEqualFunc) g_file_equal,
	                                                   g_object_unref,
	                                                   g_object_unref);

	/* Watch for changes to any file that might tell us about changes to 
	 * branches, tags, remotes, or the active branch */
	git_head_path = g_strjoin (G_DIR_SEPARATOR_S,
	                           working_directory,
	                           ".git",
	                           "HEAD",
	                           NULL);
	git_packed_refs_path = g_strjoin (G_DIR_SEPARATOR_S,
	                                  working_directory,
	                                  ".git",
	                                  "packed-refs",
	                                  NULL);
	git_branches_path = g_strjoin (G_DIR_SEPARATOR_S,
	                               working_directory,
	                               ".git",
	                               "refs",
	                               "heads",
	                               NULL);
	git_tags_path = g_strjoin (G_DIR_SEPARATOR_S,
	                           working_directory,
	                           ".git",
	                           "refs",
	                           "tags",
	                           NULL);
	git_remotes_path = g_strjoin (G_DIR_SEPARATOR_S,
	                              working_directory,
	                              ".git",
	                              "refs",
	                              "remotes",
	                              NULL);

	git_ref_command_add_file_monitor (self, 
	                                  g_file_new_for_path (git_head_path));

	git_ref_command_add_file_monitor (self, 
	                                  g_file_new_for_path (git_packed_refs_path));

	git_ref_command_add_file_monitor (self,
	                                  g_file_new_for_path (git_branches_path));

	git_ref_command_add_file_monitor (self,
	                                  g_file_new_for_path (git_tags_path));

	/* Now handle remotes. Git stores the head of each remote branch in its own
	 * folder under .git/refs/remotes. We need to monitor for changes to each
	 * remote's folder. */
	remotes_file = g_file_new_for_path (git_remotes_path);

	/* First, monitor the remotes folder for creation/deletion of remotes */
	remotes_monitor = g_file_monitor (remotes_file, 0, NULL, NULL);

	g_signal_connect (G_OBJECT (remotes_monitor), "changed",
	                  G_CALLBACK (on_remote_file_monitor_changed),
	                  self);

	/* Add the monitor to the hash table to simplify cleanup */
	g_hash_table_insert (self->priv->file_monitors, remotes_file, 
						 remotes_monitor);
	
	remotes_enumerator = g_file_enumerate_children (remotes_file,
	                                                G_FILE_ATTRIBUTE_STANDARD_NAME ","
	                                                G_FILE_ATTRIBUTE_STANDARD_TYPE,
	                                                0, NULL, NULL);

	remotes_info = g_file_enumerator_next_file (remotes_enumerator, NULL, NULL);

	while (remotes_info)
	{
		/* Monitor each remote folder for changes */
		if (g_file_info_get_attribute_uint32 (remotes_info, 
		                                      G_FILE_ATTRIBUTE_STANDARD_TYPE) == G_FILE_TYPE_DIRECTORY)
		{
			current_remote_file = g_file_get_child (remotes_file,
			                                        g_file_info_get_name (remotes_info));

			git_ref_command_add_file_monitor (self, current_remote_file);
		}

		g_object_unref (remotes_info);
		
		remotes_info = g_file_enumerator_next_file (remotes_enumerator, NULL, 
		                                            NULL);
	}

	g_free (working_directory);
	g_free (git_head_path);
	g_free (git_packed_refs_path);
	g_free (git_branches_path);
	g_free (git_tags_path);
	g_free (git_remotes_path);
	g_object_unref (remotes_enumerator);
	
	return TRUE;
}

static void
git_ref_command_stop_automatic_monitor (AnjutaCommand *command)
{
	GitRefCommand *self;

	self = GIT_REF_COMMAND (command);

	if (self->priv->file_monitors)
	{
		g_hash_table_destroy (self->priv->file_monitors);
		self->priv->file_monitors = NULL;
	}
}

static void
git_ref_command_started (AnjutaCommand *command)
{
	GitRefCommand *self;

	self = GIT_REF_COMMAND (command);

	/* Clear out old data from previous runs */
	g_hash_table_remove_all (self->priv->refs);

	ANJUTA_COMMAND_CLASS (git_ref_command_parent_class)->command_started (command);
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
	command_class->command_started = git_ref_command_started;
	command_class->start_automatic_monitor = git_ref_command_start_automatic_monitor;
	command_class->stop_automatic_monitor = git_ref_command_stop_automatic_monitor;
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
