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

#include "git-branch-list-command.h"

/* Detects the currently active branch in the output */
#define ACTIVE_BRANCH_REGEX "^\\* (.*)"

/* Other branches. Just strips off the space and newlines for us :) */
#define REGULAR_BRANCH_REGEX "^(?:\\s) (.*)"

struct _GitBranchListCommandPriv
{
	GitBranchType type;
	GRegex *active_branch_regex;
	GRegex *regular_branch_regex;
	GList *output;
	GFileMonitor *ref_monitor;
	GFileMonitor *head_monitor;
};

G_DEFINE_TYPE (GitBranchListCommand, git_branch_list_command, GIT_TYPE_COMMAND);

static void
git_branch_list_command_init (GitBranchListCommand *self)
{
	self->priv = g_new0 (GitBranchListCommandPriv, 1);
	self->priv->active_branch_regex = g_regex_new (ACTIVE_BRANCH_REGEX, 0, 0,
												   NULL);
	self->priv->regular_branch_regex = g_regex_new (REGULAR_BRANCH_REGEX, 0, 0,
													NULL);
}

static void
git_branch_list_command_clear_output (GitBranchListCommand *self)
{
	g_list_foreach (self->priv->output, (GFunc) g_object_unref, NULL);
	g_list_free (self->priv->output);
	self->priv->output = NULL;
}

static void
git_branch_list_command_monitor_changed (GFileMonitor *file_monitor, 
                                         GFile *file, GFile *other_file, 
                                         GFileMonitorEvent event_type,
                                         GitBranchListCommand *self)
{
	/* Git must overwrite this file during swtiches instead of just modifying 
	 * it... */
	if (event_type == G_FILE_MONITOR_EVENT_CREATED)
	{
		/* Always refresh the branch model to at least reflect that the 
		 * active branch has probably changed */
		anjuta_command_start (ANJUTA_COMMAND (self));
	}
}

static gboolean
git_branch_list_command_start_automatic_monitor (AnjutaCommand *command)
{
	GitBranchListCommand *self;
	gchar *working_directory;
	gchar *git_ref_path;
	gchar *git_head_path;
	GFile *git_head_file;
	GFile *git_ref_file;

	self = GIT_BRANCH_LIST_COMMAND (command);

	g_object_get (self, "working-directory", &working_directory, NULL);

	/* Watch for changes to two key files in the git repository: .git/refs/heads
	 * for indicating creation/deletion of branches, and .git/HEAD for active
	 * branch changes. */
	git_ref_path = g_strjoin (G_DIR_SEPARATOR_S,
	                          working_directory,
	                          ".git",
	                          "refs",
	                          "heads",
	                          NULL);
	git_head_path = g_strjoin (G_DIR_SEPARATOR_S,
	                           working_directory,
	                           ".git",
	                           "HEAD",
	                           NULL);
	git_ref_file = g_file_new_for_path (git_ref_path);
	git_head_file = g_file_new_for_path (git_head_path);
	self->priv->ref_monitor = g_file_monitor_directory (git_ref_file, 0, NULL, 
	                                            		NULL);
	self->priv->head_monitor = g_file_monitor_file (git_head_file, 0, NULL,
	                                                NULL);

	g_signal_connect (G_OBJECT (self->priv->ref_monitor), "changed",
	                  G_CALLBACK (git_branch_list_command_monitor_changed),
	                  self);

	g_signal_connect (G_OBJECT (self->priv->head_monitor), "changed",
	                  G_CALLBACK (git_branch_list_command_monitor_changed),
	                  self);

	g_free (git_ref_path);
	g_free (git_head_path);
	g_free (working_directory);
	g_object_unref (git_ref_file);
	g_object_unref (git_head_file);

	return TRUE;
}

static void
git_branch_list_command_stop_automatic_monitor (AnjutaCommand *command)
{
	GitBranchListCommand *self;

	self = GIT_BRANCH_LIST_COMMAND (command);

	if (self->priv->ref_monitor)
	{
		g_file_monitor_cancel (self->priv->ref_monitor);
		g_object_unref (self->priv->ref_monitor);
		self->priv->ref_monitor = NULL;
	}

	if (self->priv->head_monitor)
	{
		g_file_monitor_cancel (self->priv->head_monitor);
		g_object_unref (self->priv->head_monitor);
		self->priv->head_monitor = NULL;
	}
}

static void
git_branch_list_command_finalize (GObject *object)
{
	GitBranchListCommand *self;
	
	self = GIT_BRANCH_LIST_COMMAND (object);
	
	g_regex_unref (self->priv->active_branch_regex);
	g_regex_unref (self->priv->regular_branch_regex);

	git_branch_list_command_clear_output (self);
	git_branch_list_command_stop_automatic_monitor (ANJUTA_COMMAND (self));
	
	g_free (self->priv);

	G_OBJECT_CLASS (git_branch_list_command_parent_class)->finalize (object);
}

static guint
git_branch_list_command_run (AnjutaCommand *command)
{
	GitBranchListCommand *self;
	
	self = GIT_BRANCH_LIST_COMMAND (command);
	
	git_command_add_arg (GIT_COMMAND (command), "branch");
						 
	switch (self->priv->type)
	{
		case GIT_BRANCH_TYPE_REMOTE:
			git_command_add_arg (GIT_COMMAND (command), "-r");
			break;
		case GIT_BRANCH_TYPE_ALL:
			git_command_add_arg (GIT_COMMAND (command), "-a");
			break;
		default:
			break;
	}
	
	return 0;
}

static void
git_branch_list_command_data_arrived (AnjutaCommand *command)
{
	git_branch_list_command_clear_output (GIT_BRANCH_LIST_COMMAND (command));
}

static void
git_branch_list_command_handle_output (GitCommand *git_command, 
									   const gchar *output)
{
	GitBranchListCommand *self;
	GMatchInfo *active_match_info;
	GMatchInfo *regular_match_info;
	gchar *branch_name;
	GitBranch *branch;
	gboolean active;

	self = GIT_BRANCH_LIST_COMMAND (git_command);
	
	active_match_info = NULL;
	regular_match_info = NULL;
	branch_name = NULL;
	branch = NULL;
	active = FALSE;
	
	if (g_regex_match (self->priv->active_branch_regex, output, 0, 
					   &active_match_info))
	{
		branch_name = g_match_info_fetch (active_match_info, 1);
		active = TRUE;
	}
	else if (g_regex_match (self->priv->regular_branch_regex, output, 0,
							&regular_match_info))
	{
		branch_name = g_match_info_fetch (regular_match_info, 1);
	}
	
	if (branch_name)
		branch = git_branch_new (branch_name, active);
	
	g_free (branch_name);


	if (active_match_info)
		g_match_info_free (active_match_info);

	if (regular_match_info)
		g_match_info_free (regular_match_info);
	
	self->priv->output = g_list_append (self->priv->output, branch);
	anjuta_command_notify_data_arrived (ANJUTA_COMMAND (git_command));
	
}

static void
git_branch_list_command_class_init (GitBranchListCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GitCommandClass* parent_class = GIT_COMMAND_CLASS (klass);
	AnjutaCommandClass *command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = git_branch_list_command_finalize;
	parent_class->output_handler = git_branch_list_command_handle_output;
	command_class->run = git_branch_list_command_run;
	command_class->data_arrived = git_branch_list_command_data_arrived;
	command_class->start_automatic_monitor = git_branch_list_command_start_automatic_monitor;
	command_class->stop_automatic_monitor = git_branch_list_command_stop_automatic_monitor;
}


GitBranchListCommand *
git_branch_list_command_new (const gchar *working_directory, 
							 GitBranchType type)
{
	GitBranchListCommand *self;
	
	self = g_object_new (GIT_TYPE_BRANCH_LIST_COMMAND,
						 "working-directory", working_directory,
						 "single-line-output", TRUE,
						 NULL);
	
	self->priv->type = type;
	
	return self;
}

GList *
git_branch_list_command_get_output (GitBranchListCommand *self)
{
	return self->priv->output;
}
