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
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple 
 * Place, Suite 330, Boston, MA 02111-1307 USA */
#ifndef _ACTION_CALLBACKS_H_
#define _ACTION_CALLBACKS_H_


void on_new_file1_activate (EggAction * action, gpointer user_data);
void on_open1_activate (EggAction * action, gpointer user_data);
void on_save1_activate (EggAction * action, gpointer user_data);
void on_save_as1_activate (EggAction * action, gpointer user_data);
void on_save_all1_activate (EggAction * action, gpointer user_data);
void on_close_file1_activate (EggAction * action, gpointer user_data);
void on_reload_file1_activate (EggAction * action, gpointer user_data);
void on_close_all_file1_activate (EggAction * action, gpointer user_data);
void on_editor_command_activate (EggAction * action, gpointer user_data);
void on_editor_select_function  (EggAction * action, gpointer user_data);
void on_editor_select_word (EggAction *action, gpointer user_data);
void on_editor_select_line (EggAction *action, gpointer user_data);
void on_transform_eolchars1_activate (EggAction * action, gpointer user_data);

void on_search1_activate (EggAction * action, gpointer user_data);
void on_find1_activate (EggAction * action, gpointer user_data);
void on_find_and_replace1_activate (EggAction * action, gpointer user_data);
void on_find_in_files1_activate (EggAction * action, gpointer user_data);
void on_findnext1_activate (EggAction * action, gpointer user_data);
void on_findprevious1_activate (EggAction * action, gpointer user_data);
void on_enterselection (EggAction * action, gpointer user_data);

void on_goto_activate (EggAction *action, gpointer user_data);
void on_toolbar_goto_clicked (EggAction *action, gpointer user_data);
void on_goto_line_no1_activate (EggAction * action, gpointer user_data);

void on_next_occur (EggAction * action, gpointer user_data);
void on_prev_occur (EggAction * action, gpointer user_data);

void on_autocomplete1_activate (EggAction * action, gpointer user_data);
void on_calltip1_activate (EggAction * action, gpointer user_data);

void on_comment_block (EggAction * action, gpointer user_data);
void on_comment_box (EggAction * action, gpointer user_data);
void on_comment_stream (EggAction * action, gpointer user_data);
void on_insert_custom_indent (EggAction *action, gpointer user_data);

void on_goto_block_start1_activate (EggAction * action, gpointer user_data);
void on_goto_block_end1_activate (EggAction * action, gpointer user_data);

void on_editor_linenos1_activate (EggAction *action, gpointer user_data);
void on_editor_markers1_activate (EggAction *action, gpointer user_data);
void on_editor_codefold1_activate (EggAction *action, gpointer user_data);
void on_editor_indentguides1_activate (EggAction *action,
									   gpointer user_data);
void on_editor_whitespaces1_activate (EggAction *action,
									  gpointer user_data);
void on_editor_eolchars1_activate (EggAction *action, gpointer user_data);
void on_editor_linewrap1_activate (EggAction *action, gpointer user_data);
void on_zoom_text_activate (EggAction * action, gpointer user_data);
void on_force_hilite1_activate (EggAction * action, gpointer user_data);
void on_indent1_activate (EggAction * action, gpointer user_data);

void on_insert_header (EggAction * action, gpointer user_data);
void on_insert_cvs_author (EggAction * action, gpointer user_data);
void on_insert_cvs_date (EggAction * action, gpointer user_data);
void on_insert_cvs_header (EggAction * action, gpointer user_data);
void on_insert_cvs_id (EggAction * action, gpointer user_data);
void on_insert_cvs_log (EggAction * action, gpointer user_data);
void on_insert_cvs_name (EggAction * action, gpointer user_data);
void on_insert_cvs_revision (EggAction * action, gpointer user_data);
void on_insert_cvs_source (EggAction * action, gpointer user_data);
void on_insert_switch_template (EggAction * action, gpointer user_data);
void on_insert_for_template (EggAction * action, gpointer user_data);
void on_insert_while_template (EggAction * action, gpointer user_data);
void on_insert_ifelse_template (EggAction * action, gpointer user_data);
void on_insert_c_gpl_notice (EggAction * action, gpointer user_data);
void on_insert_cpp_gpl_notice (EggAction * action, gpointer user_data);
void on_insert_py_gpl_notice (EggAction * action, gpointer user_data);
void on_insert_date_time (EggAction * action, gpointer user_data);
void on_insert_changelog_entry (EggAction * action, gpointer user_data);
void on_insert_header_template (EggAction * action, gpointer user_data);
void on_insert_username (EggAction * action, gpointer user_data);

void on_format_indent_style_clicked (EggAction * action, gpointer user_data);

gboolean on_toolbar_find_incremental_start (EggAction *action, gpointer user_data);
gboolean on_toolbar_find_incremental_end (EggAction *action, gpointer user_data);
void on_toolbar_find_incremental (EggAction *action, gpointer user_data);
void on_toolbar_find_clicked (EggAction *action, gpointer user_data);

#endif
