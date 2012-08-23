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

#include "git-status-command.h"

#define STATUS_REGEX "((M|A|D|U|\\?|\\s){2}) (.*)"

struct _GitStatusCommandPriv
{
	GQueue *status_queue;
	GitStatusSections sections;
	GHashTable *status_codes;
	GHashTable *conflict_codes;
	GRegex *status_regex;
	GFileMonitor *head_monitor;
	GFileMonitor *index_monitor;
};

G_DEFINE_TYPE (GitStatusCommand, git_status_command, GIT_TYPE_COMMAND);

static guint
git_status_command_run (AnjutaCommand *command)
{
	git_command_add_arg (GIT_COMMAND (command), "status");
	git_command_add_arg (GIT_COMMAND (command), "--porcelain");
	
	return 0;
}

static void
git_status_command_handle_output (GitCommand *git_command, const gchar *output)
{
	GitStatusCommand *self;
	GMatchInfo *match_info;
	GitStatus *status_object;
	gchar *status;
	gchar *path;
	
	self = GIT_STATUS_COMMAND (git_command);
	status_object = NULL;

	if (g_regex_match (self->priv->status_regex, output, 0, &match_info))
	{
		/* Determine which section this entry goes in */
		status = g_match_info_fetch (match_info, 1);
		path = g_match_info_fetch (match_info, 3);

		if (status[0] == ' ')
		{
			/* Changed but not updated */
			if (self->priv->sections & GIT_STATUS_SECTION_NOT_UPDATED)
			{
				status_object = git_status_new(path, 
				                               GPOINTER_TO_INT (g_hash_table_lookup (self->priv->status_codes, 
				                                                					 GINT_TO_POINTER (status[1]))));
			}
		}
		else if (status[1] == ' ')
		{
			/* Added to commit */
			if (self->priv->sections & GIT_STATUS_SECTION_COMMIT)
			{
				status_object = git_status_new(path, 
				                               GPOINTER_TO_INT (g_hash_table_lookup (self->priv->status_codes, 
				                                                    				 GINT_TO_POINTER (status[0]))));
			}
		}
		else
		{
			/* File may have been added to the index and then changed again in
			 * the working tree, or it could be a conflict */

			/* Unversioned files */
			if (status[0] == '?')
			{
				if (self->priv->sections & GIT_STATUS_SECTION_UNTRACKED)
				{
					status_object = git_status_new(path, 
				                           		   ANJUTA_VCS_STATUS_UNVERSIONED);
				}
			}
			else if (g_hash_table_lookup_extended (self->priv->conflict_codes, status,
			                                       NULL, NULL))
			{
				/* Conflicts are put in the changed but not updated section */
				if (self->priv->sections & GIT_STATUS_SECTION_NOT_UPDATED)
				{
					status_object = git_status_new (path, 
					                                ANJUTA_VCS_STATUS_CONFLICTED);
				}
			}
			else
			{
				status_object = git_status_new(path, 
				                               GPOINTER_TO_INT(g_hash_table_lookup (self->priv->status_codes, 
				                                                                    GINT_TO_POINTER (status[0]))));
			}
			    
		}

		

		g_free (status);
		g_free (path);

		if (status_object)
		{
			g_queue_push_tail (self->priv->status_queue, status_object);
			anjuta_command_notify_data_arrived (ANJUTA_COMMAND (self));
		}
		
	}

	g_match_info_free (match_info);
	
}

static void
git_status_command_init (GitStatusCommand *self)
{
	self->priv = g_new0 (GitStatusCommandPriv, 1);
	self->priv->status_queue = g_queue_new ();
	
	self->priv->status_regex = g_regex_new (STATUS_REGEX, 0, 0, NULL);
	self->priv->status_codes = g_hash_table_new (g_direct_hash, g_direct_equal);
	self->priv->conflict_codes = g_hash_table_new (g_str_hash, g_str_equal);

	/* Initialize status code hash tables */
	g_hash_table_insert (self->priv->status_codes, 
	                     GINT_TO_POINTER ('M'),
	                     GINT_TO_POINTER (ANJUTA_VCS_STATUS_MODIFIED));

	g_hash_table_insert (self->priv->status_codes, 
	                     GINT_TO_POINTER ('A'),
	                     GINT_TO_POINTER (ANJUTA_VCS_STATUS_ADDED));

	g_hash_table_insert (self->priv->status_codes, 
	                     GINT_TO_POINTER ('D'),
	                     GINT_TO_POINTER (ANJUTA_VCS_STATUS_MODIFIED));

	/* TODO: Handle each conflict case individually so that we can eventually
	 * give the user more information about the conflict */
	g_hash_table_insert (self->priv->conflict_codes, "DD", NULL);
	g_hash_table_insert (self->priv->conflict_codes, "AU", NULL);
	g_hash_table_insert (self->priv->conflict_codes, "UD", NULL);
	g_hash_table_insert (self->priv->conflict_codes, "UA", NULL);
	g_hash_table_insert (self->priv->conflict_codes, "DU", NULL);
	g_hash_table_insert (self->priv->conflict_codes, "AA", NULL);
	g_hash_table_insert (self->priv->conflict_codes, "UU", NULL);
}

static void
on_file_monitor_changed (GFileMonitor *monitor, GFile *file, GFile *other_file,
                         GFileMonitorEvent event, AnjutaCommand *command)
{
	/* Handle created and modified events just to cover all possible cases. 
	 * Sometimes git does some odd things... */
	if (event == G_FILE_MONITOR_EVENT_CHANGED ||
	    event == G_FILE_MONITOR_EVENT_CREATED)
	{
		anjuta_command_start (command);
	}
}

static gboolean
git_status_command_start_automatic_monitor (AnjutaCommand *command)
{
	GitStatusCommand *self;
	gchar *working_directory;
	gchar *git_head_path;
	gchar *git_index_path;
	GFile *git_head_file;
	GFile *git_index_file;

	self = GIT_STATUS_COMMAND (command);

	g_object_get (self, "working-directory", &working_directory, NULL);

	/* Watch for changes to the HEAD file and the index file, so that we can
	 * at least detect commits and index changes. */
	git_head_path = g_strjoin (G_DIR_SEPARATOR_S,
	                           working_directory,
	                           ".git",
	                           "HEAD",
	                           NULL);
	git_index_path = g_strjoin (G_DIR_SEPARATOR_S,
	                            working_directory,
	                            ".git",
	                            "index",
	                            NULL);
	git_head_file = g_file_new_for_path (git_head_path);
	git_index_file = g_file_new_for_path (git_index_path);
	self->priv->head_monitor = g_file_monitor_file (git_head_file, 0, NULL, 
	                                                NULL);
	self->priv->index_monitor = g_file_monitor_file (git_index_file, 0, NULL,
	                                                 NULL);

	g_signal_connect (G_OBJECT (self->priv->head_monitor), "changed",
	                  G_CALLBACK (on_file_monitor_changed),
	                  command);

	g_signal_connect (G_OBJECT (self->priv->index_monitor), "changed",
	                  G_CALLBACK (on_file_monitor_changed),
	                  command);

	g_free (git_head_path);
	g_free (git_index_path);
	g_object_unref (git_head_file);
	g_object_unref (git_index_file);

	return TRUE;
}

static void
git_status_command_stop_automatic_monitor (AnjutaCommand *command)
{
	GitStatusCommand *self;

	self = GIT_STATUS_COMMAND (command); 

	if (self->priv->head_monitor)
	{
		g_file_monitor_cancel (self->priv->head_monitor);
		g_object_unref (self->priv->head_monitor);
		self->priv->head_monitor = NULL;
	}

	if (self->priv->index_monitor)
	{
		g_file_monitor_cancel (self->priv->index_monitor);
		g_object_unref (self->priv->index_monitor);
		self->priv->index_monitor = NULL;
	}
}

static void
git_status_command_clear_output (GitStatusCommand *self)
{
	GList *current_output;

	current_output = self->priv->status_queue->head;

	while (current_output)
	{
		g_object_unref (current_output->data);
		current_output = g_list_next (current_output);
	}

	g_queue_clear (self->priv->status_queue);
}

static void
git_status_command_data_arrived (AnjutaCommand *command)
{
	git_status_command_clear_output (GIT_STATUS_COMMAND (command));
}


static void
git_status_command_finalize (GObject *object)
{
	GitStatusCommand *self;
	
	self = GIT_STATUS_COMMAND (object);
	
	git_status_command_clear_output (self);
	git_status_command_stop_automatic_monitor (ANJUTA_COMMAND (self));
	
	g_queue_free (self->priv->status_queue);
	g_regex_unref (self->priv->status_regex);
	
	g_free (self->priv);

	G_OBJECT_CLASS (git_status_command_parent_class)->finalize (object);
}

static void
git_status_command_class_init (GitStatusCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GitCommandClass* parent_class = GIT_COMMAND_CLASS (klass);
	AnjutaCommandClass* command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = git_status_command_finalize;
	parent_class->output_handler = git_status_command_handle_output;
	command_class->run = git_status_command_run;
	command_class->data_arrived = git_status_command_data_arrived;
	command_class->start_automatic_monitor = git_status_command_start_automatic_monitor;
	command_class->stop_automatic_monitor = git_status_command_stop_automatic_monitor;
}


GitStatusCommand *
git_status_command_new (const gchar *working_directory, 
						GitStatusSections sections)
{
	GitStatusCommand *self;
	
	self = g_object_new (GIT_TYPE_STATUS_COMMAND, 
						 "working-directory", working_directory,
						 "single-line-output", TRUE,
						 NULL);
	
	self->priv->sections = sections;
	
	return self;
}

GQueue *
git_status_command_get_status_queue (GitStatusCommand *self)
{
	return self->priv->status_queue;
}
