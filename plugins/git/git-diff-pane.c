/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * git-shell-test
 * Copyright (C) James Liggett 2010 <jrliggett@cox.net>
 * 
 * git-shell-test is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * git-shell-test is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "git-diff-pane.h"

void
on_diff_button_clicked (GtkAction *action, Git *plugin)
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
					  G_CALLBACK (git_pane_send_raw_output_to_editor),
					  editor);

	g_signal_connect (G_OBJECT (diff_command), "command_finished",
					  G_CALLBACK (git_pane_report_errors),
					  plugin);

	g_signal_connect (G_OBJECT (diff_command), "command_finished",
					  G_CALLBACK (g_object_unref),
					  NULL);

	anjuta_command_start (ANJUTA_COMMAND (diff_command));
}
 