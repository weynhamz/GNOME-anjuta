/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2007 <jrliggett@cox.net>
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

#ifndef GIT_UI_UTILS_H
#define GIT_UI_UTILS_H

#include <libanjuta/anjuta-vcs-status-tree-view.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-message-manager.h>
#include "plugin.h"
#include "git-status-command.h"
#include "git-diff-command.h"
#include "git-branch-list-command.h"
#include "git-branch-combo-model.h"

typedef struct
{
	GladeXML* gxml;
	Git* plugin;
} GitUIData;

typedef struct
{
	Git *plugin;
	gint last_progress;
	gchar *text;
} GitProgressData;

GitUIData* git_ui_data_new (Git* plugin, GladeXML* gxml);
void git_ui_data_free (GitUIData* data);
GitProgressData *git_progress_data_new (Git *plugin, const gchar *text);
void git_progress_data_free (GitProgressData *data);
void git_create_message_view (Git* plugin);
gboolean git_check_input (GtkWidget *parent, GtkWidget *widget,  
						  const gchar *input, const gchar *error_message);
gchar *git_get_log_from_textview (GtkWidget* textview);
guint git_status_bar_progress_pulse (Git *plugin, gchar *text);
void git_clear_status_bar_progress_pulse (guint timer_id);
void git_pulse_progress_bar (GtkProgressBar *progress_bar);
void git_report_errors (AnjutaCommand *command, guint return_code);
gchar *git_get_filename_from_full_path (gchar *path);
const gchar *git_get_relative_path (const gchar *path, 
									const gchar *working_directory);

/* Stock signal handlers */
void on_git_command_finished (AnjutaCommand *command, guint return_code, 
							  gpointer user_data);
void on_git_status_command_data_arrived (AnjutaCommand *command, 
										 AnjutaVcsStatusTreeView *tree_view);
void on_git_command_info_arrived (AnjutaCommand *command, Git *plugin);
void on_git_command_progress (AnjutaCommand *command, gfloat progress, 
							  GitProgressData *data);
void on_git_list_branch_command_data_arrived (AnjutaCommand *command, 
											  GitBranchComboData *data);
void on_git_list_branch_command_finished (AnjutaCommand *command, 
										  guint return_code, 
										  GitBranchComboData *data);
void git_select_all_status_items (GtkButton *select_all_button, 
								  AnjutaVcsStatusTreeView *tree_view);
void git_clear_all_status_selections (GtkButton *clear_button,
									  AnjutaVcsStatusTreeView *tree_view);

void git_init_whole_project (Git *plugin, GtkWidget* project,
							 gboolean active);
void on_git_whole_project_toggled (GtkToggleButton* project, Git *plugin);

void on_git_diff_command_finished (AnjutaCommand *command, guint return_code, 
								   Git *plugin);
void git_send_raw_command_output_to_editor (AnjutaCommand *command, 
											IAnjutaEditor *editor);
void git_stop_status_bar_progress_pulse (AnjutaCommand *command, 
										 guint return_code, gpointer timer_id);
void git_hide_pulse_progress_bar (AnjutaCommand *command, guint return_code,
								  GtkProgressBar *progress_bar);
void git_disconnect_data_arrived_signals (AnjutaCommand *command, GObject *object);
void git_cancel_data_arrived_signal_disconnect (AnjutaCommand *command, 
												guint return_code,
												GObject *signal_target);
#endif
