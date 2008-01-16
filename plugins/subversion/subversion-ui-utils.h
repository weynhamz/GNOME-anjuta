/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2007 <jrliggett@cox.net>
 *
 * Portions based on the original Subversion plugin 
 * Copyright (C) Johannes Schmid 2005 
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

#ifndef SUBVERSION_UI_UTILS_H
#define SUBVERSION_UI_UTILS_H

#include "plugin.h"
#include "svn-status-command.h"
#include "svn-diff-command.h"
#include <libanjuta/anjuta-vcs-status-tree-view.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-editor.h>

typedef struct
{
	GladeXML* gxml;
	Subversion* plugin;
} SubversionData;

SubversionData* subversion_data_new (Subversion* plugin, GladeXML* gxml);
void subversion_data_free (SubversionData* data);
void create_message_view (Subversion* plugin);
gboolean check_input (GtkWidget *parent, GtkWidget *entry, 
					  const gchar *error_message);
gchar *get_log_from_textview (GtkWidget* textview);
guint status_bar_progress_pulse (Subversion *plugin, gchar *text);
void clear_status_bar_progress_pulse (guint timer_id);
void pulse_progress_bar (GtkProgressBar *progress_bar);
void report_errors (AnjutaCommand *command, guint return_code);
gchar *get_filename_from_full_path (gchar *path);

/* Stock signal handlers */
void on_status_command_finished (AnjutaCommand *command, guint return_code, 
								 gpointer user_data);
void on_status_command_data_arrived (AnjutaCommand *command, 
									 AnjutaVcsStatusTreeView *tree_view);
void on_command_info_arrived (AnjutaCommand *command, Subversion *plugin);
void select_all_status_items (GtkButton *select_all_button, 
							  AnjutaVcsStatusTreeView *tree_view);
void clear_all_status_selections (GtkButton *clear_button,
								  AnjutaVcsStatusTreeView *tree_view);

void init_whole_project (Subversion *plugin, GtkWidget* project,
						 gboolean active);
void on_whole_project_toggled (GtkToggleButton* project, Subversion *plugin);
void on_diff_command_finished (AnjutaCommand *command, guint return_code, 
							   Subversion *plugin);
void send_diff_command_output_to_editor (AnjutaCommand *command, 
										 IAnjutaEditor *editor);
void stop_status_bar_progress_pulse (AnjutaCommand *command, guint return_code,
									 gpointer timer_id);
void hide_pulse_progress_bar (AnjutaCommand *command, guint return_code,
							  GtkProgressBar *progress_bar);
void disconnect_data_arrived_signals (AnjutaCommand *command, GObject *object);
void cancel_data_arrived_signal_disconnect (AnjutaCommand *command, 
											guint return_code,
											GObject *signal_target);
#endif
