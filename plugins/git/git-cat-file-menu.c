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

#include "git-cat-file-menu.h"

static void
on_cat_command_finished (AnjutaCommand *command, guint return_code,
						 Git *plugin)
{
	AnjutaStatus *status;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell,
									  NULL);
	
	anjuta_status (status, _("Git: File retrieved."), 5);
	
	git_report_errors (command, return_code);
	
	g_object_unref (command);
}

static void
on_list_tree_command_finished (AnjutaCommand *command, guint return_code, 
							   Git *plugin)
{
	gchar *path;
	GFile *path_file;
	gchar *basename;
	gchar *short_commit_sha;
	gchar *editor_name;
	IAnjutaDocumentManager *document_manager;
	IAnjutaEditor *editor;
	GHashTable *blobs;
	gchar *blob_sha;
	GitCatBlobCommand *cat_command;
	
	if (return_code != 0)
	{
		git_report_errors (command, return_code);
		g_object_unref (command);
		
		return;
	}
	
	
	short_commit_sha = g_object_get_data (G_OBJECT (command), 
										  "short-commit-sha"); 
	path = g_object_get_data (G_OBJECT (command), "path");
	blobs = git_list_tree_command_get_blobs (GIT_LIST_TREE_COMMAND (command));
	path_file = g_file_new_for_path (path);
	basename = g_file_get_basename (path_file);
	editor_name = g_strdup_printf ("[Revision %s] %s", short_commit_sha, 
								   basename);
	document_manager = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
												   IAnjutaDocumentManager, 
												   NULL);
	editor = ianjuta_document_manager_add_buffer (document_manager, 
												  editor_name,
												  "", NULL);
	g_free (editor_name);
	
	blob_sha = g_hash_table_lookup (blobs, basename);
	cat_command = git_cat_blob_command_new (plugin->project_root_directory,
											blob_sha);
	
	g_signal_connect (G_OBJECT (cat_command), "data-arrived",
					  G_CALLBACK (git_send_raw_command_output_to_editor),
					  editor);
	
	g_signal_connect (G_OBJECT (cat_command), "command_finished",
					  G_CALLBACK (on_cat_command_finished),
					  plugin);
	
	anjuta_command_start (ANJUTA_COMMAND (cat_command));
	
	g_hash_table_unref (blobs);
	g_object_unref (path_file);
	g_free (basename);
}

static void
git_cat_file (Git *plugin, const gchar *path, GitRevision *revision)
{
	GFile *path_file;
	GFile *path_parent_file;
	gchar *parent_path;
	gchar *commit_sha;
	gchar *short_commit_sha;
	GitListTreeCommand *list_tree_command;
	
	/* Given paths are supposed to be full paths, not relative ones like 
	 * git would expect. */
	path_file = g_file_new_for_path (path);
	path_parent_file = g_file_get_parent (path_file);
	parent_path = g_file_get_path (path_parent_file);
	
	commit_sha = git_revision_get_sha (revision);
	short_commit_sha = git_revision_get_short_sha (revision);
	
	/* Unlike with all the other commands, use the folder of the given file,
	 * as git ls-tree works like regular ls in that it will ususally only give
	 * listings for the current directory. */
	list_tree_command = git_list_tree_command_new (parent_path,
												   commit_sha);
	
	g_signal_connect (G_OBJECT (list_tree_command), "command-finished",
					  G_CALLBACK (on_list_tree_command_finished),
					  plugin);
	
	g_object_set_data_full (G_OBJECT (list_tree_command), "path", 
							g_strdup (path), g_free);
	g_object_set_data_full (G_OBJECT (list_tree_command), "short-commit-sha", 
							g_strdup (short_commit_sha), g_free);
	
	anjuta_command_start (ANJUTA_COMMAND (list_tree_command));
	
	g_object_unref (path_file);
	g_object_unref (path_parent_file);
	g_free (parent_path);
	g_free (commit_sha);
	g_free (short_commit_sha);
}

void
on_log_menu_git_cat_file (GtkAction *action, Git *plugin)
{
	gchar *path;
	GitRevision *revision;
	
	path = git_log_get_path (plugin);
	revision = git_log_get_selected_revision (plugin);
	
	if (revision)
		git_cat_file (plugin, path, revision);
	
	g_free (path);
	g_object_unref (revision);
}

void
git_cat_file_menu_set_sensitive (Git *plugin, gboolean sensitive)
{
	AnjutaUI *ui;
	GtkAction *view_revision_action;
	
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (plugin)->shell, NULL);
	view_revision_action = anjuta_ui_get_action (ui, "ActionGroupGitLog",
												 "ActionGitLogViewRevision");
	
	gtk_action_set_sensitive (view_revision_action, sensitive);
}
