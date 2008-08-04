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

#include "git-diff-dialog.h"

/* FIXME: We don't really have a dialog yet. Implement that when we have an FM
 * context menu (for diffing individual files.) A dialog isn't really needed 
 * for just getting project-wide uncommitted changes. */

static void
git_diff (Git *plugin)
{
	IAnjutaDocumentManager *document_manager;
	IAnjutaEditor *editor;
	GitDiffCommand *diff_command;
	
	document_manager = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
												   IAnjutaDocumentManager, 
												   NULL);
	editor = ianjuta_document_manager_add_buffer (document_manager, 
												  "Uncommitted Changes.diff",
												  "", NULL);
	
	diff_command = git_diff_command_new (plugin->project_root_directory);
	
	g_signal_connect (G_OBJECT (diff_command), "data-arrived",
					  G_CALLBACK (git_send_raw_command_output_to_editor),
					  editor);
	
	g_signal_connect (G_OBJECT (diff_command), "command_finished",
					  G_CALLBACK (on_git_diff_command_finished),
					  plugin);
	
	anjuta_command_start (ANJUTA_COMMAND (diff_command));
	
}

static void
git_commit_diff (Git *plugin)
{
	GitRevision *revision;
	gchar *sha;
	gchar *short_sha;
	gchar *editor_name;
	IAnjutaDocumentManager *document_manager;
	IAnjutaEditor *editor;
	GitDiffTreeCommand *diff_command;
	
	revision = git_log_get_selected_revision (plugin);
	
	if (revision)
	{
		sha = git_revision_get_sha (revision);
		short_sha = git_revision_get_short_sha (revision);
		editor_name = g_strdup_printf ("Commit %s.diff", short_sha);
		
		document_manager = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
													   IAnjutaDocumentManager, 
													   NULL);
		editor = ianjuta_document_manager_add_buffer (document_manager, 
													  editor_name,
													  "", NULL);
		
		g_free (short_sha);
		g_free (editor_name);
		
		
		diff_command = git_diff_tree_command_new (plugin->project_root_directory,
												  sha);
		
		g_signal_connect (G_OBJECT (diff_command), "data-arrived",
						  G_CALLBACK (git_send_raw_command_output_to_editor),
						  editor);
		
		g_signal_connect (G_OBJECT (diff_command), "command_finished",
						  G_CALLBACK (on_git_diff_command_finished),
						  plugin);
		
		anjuta_command_start (ANJUTA_COMMAND (diff_command));
	}
}

void
on_menu_git_diff (GtkAction *action, Git *plugin)
{
	git_diff (plugin);
}

void
on_log_menu_git_commit_diff (GtkAction *action, Git *plugin)
{
	git_commit_diff (plugin);
}
