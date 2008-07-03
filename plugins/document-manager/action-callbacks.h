/* 
 * action-callbacks.h Copyright (C) 2003 Naba Kumar  <naba@gnome.org>
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along with 
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#ifndef _ACTION_CALLBACKS_H_
#define _ACTION_CALLBACKS_H_

void on_new_file_activate (GtkAction *action, gpointer user_data);
void on_open_activate (GtkAction *action, gpointer user_data);
void on_save_activate (GtkAction *action, gpointer user_data);
void on_save_as_activate (GtkAction *action, gpointer user_data);
void on_save_all_activate (GtkAction *action, gpointer user_data);
void on_close_file_activate (GtkAction *action, gpointer user_data);
void on_reload_file_activate (GtkAction *action, gpointer user_data);
void on_close_all_file_activate (GtkAction *action, gpointer user_data);

void on_print_activate (GtkAction *action, gpointer user_data);
void on_print_preview_activate (GtkAction *action, gpointer user_data);

void on_editor_command_upper_case_activate (GtkAction *action, gpointer user_data);
void on_editor_command_lower_case_activate (GtkAction *action, gpointer user_data);
void on_editor_command_eol_crlf_activate (GtkAction *action, gpointer user_data);
void on_editor_command_eol_lf_activate (GtkAction *action, gpointer user_data);
void on_editor_command_eol_cr_activate (GtkAction *action, gpointer user_data);
void on_editor_command_select_all_activate (GtkAction *action, gpointer user_data);
void on_editor_command_select_to_brace_activate (GtkAction *action, gpointer user_data);
void on_editor_command_select_block_activate (GtkAction *action, gpointer user_data);
void on_editor_command_match_brace_activate (GtkAction *action, gpointer user_data);
void on_editor_command_undo_activate (GtkAction *action, gpointer user_data);
void on_editor_command_redo_activate (GtkAction *action, gpointer user_data);
void on_editor_command_cut_activate (GtkAction *action, gpointer user_data);
void on_editor_command_copy_activate (GtkAction *action, gpointer user_data);
void on_editor_command_paste_activate (GtkAction *action, gpointer user_data);
void on_editor_command_clear_activate (GtkAction *action, gpointer user_data);
void on_editor_command_complete_word_activate (GtkAction *action, gpointer user_data);
void on_editor_command_close_folds_all_activate (GtkAction *action, gpointer user_data);
void on_editor_command_open_folds_all_activate (GtkAction *action, gpointer user_data);
void on_editor_command_toggle_fold_activate (GtkAction *action, gpointer user_data);
void on_editor_command_bookmark_toggle_activate (GtkAction *action, gpointer user_data);
void on_editor_command_bookmark_first_activate (GtkAction *action, gpointer user_data);
void on_editor_command_bookmark_next_activate (GtkAction *action, gpointer user_data);
void on_editor_command_bookmark_prev_activate (GtkAction *action, gpointer user_data);
void on_editor_command_bookmark_last_activate (GtkAction *action, gpointer user_data);
void on_editor_command_bookmark_clear_activate (GtkAction *action, gpointer user_data);

void on_transform_eolchars1_activate (GtkAction *action, gpointer user_data);

void on_goto_activate (GtkAction *action, gpointer user_data);
void on_toolbar_goto_clicked (GtkAction *action, gpointer user_data);
void on_goto_line_no1_activate (GtkAction *action, gpointer user_data);

void on_next_history (GtkAction *action, gpointer user_data);
void on_prev_history (GtkAction *action, gpointer user_data);

//void on_autocomplete1_activate (GtkAction *action, gpointer user_data);
void on_calltip1_activate (GtkAction *action, gpointer user_data);

void on_comment_block (GtkAction *action, gpointer user_data);
void on_comment_box (GtkAction *action, gpointer user_data);
void on_comment_stream (GtkAction *action, gpointer user_data);
void on_insert_custom_indent (GtkAction *action, gpointer user_data);

void on_goto_block_start1_activate (GtkAction *action, gpointer user_data);
void on_goto_block_end1_activate (GtkAction *action, gpointer user_data);

void on_editor_linenos1_activate (GtkAction *action, gpointer user_data);
void on_editor_markers1_activate (GtkAction *action, gpointer user_data);
void on_editor_codefold1_activate (GtkAction *action, gpointer user_data);
void on_editor_indentguides1_activate (GtkAction *action,
									   gpointer user_data);
void on_editor_whitespaces1_activate (GtkAction *action,
									  gpointer user_data);
void on_editor_eolchars1_activate (GtkAction *action, gpointer user_data);
void on_editor_linewrap1_activate (GtkAction *action, gpointer user_data);
void on_zoom_in_text_activate (GtkAction *action, gpointer user_data);
void on_zoom_out_text_activate (GtkAction *action, gpointer user_data);

void on_force_hilite_activate (GtkWidget *menuitem, gpointer user_data);

void on_indent1_activate (GtkAction *action, gpointer user_data);

void on_format_indent_style_clicked (GtkAction *action, gpointer user_data);

void on_editor_add_view_activate (GtkAction *action, gpointer user_data);
void on_editor_remove_view_activate (GtkAction *action, gpointer user_data);

void on_show_search (GtkAction *action, gpointer user_data);
void on_repeat_quicksearch (GtkAction *action, gpointer user_data);

void on_next_document (GtkAction *action, gpointer user_data);
void on_previous_document (GtkAction *action, gpointer user_data);

#endif
