/* 
 * mainmenu_callbacks.h Copyright (C) 2000 Kh. Naba Kumar Singh
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
#ifndef _MAINMENU_CALLBACKS_H_
#define _MAINMENU_CALLBACKS_H_

#include <gnome.h>

void on_new_file1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_open1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_save1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_save_as1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_save_all1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_close_file1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_reload_file1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_close_all_file1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_new_project1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_import_project_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_open_project1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_open_project1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_save_project1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_close_project1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_rename1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_page_setup1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_nonimplemented_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_exit1_activate (GtkMenuItem * menuitem, gpointer user_data);

/*****************************************************************************/
void on_editor_command_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_editor_select_function  (GtkMenuItem * menuitem, gpointer user_data);
void on_transform_eolchars1_activate (GtkMenuItem * menuitem,
									  gpointer user_data);
void on_find1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_autocomplete1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_calltip1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_find_in_files1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_find_and_replace1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_goto_line_no1_activate (GtkMenuItem * menuitem, gpointer user_data);

void on_next_occur (GtkMenuItem * menuitem, gpointer user_data);
void on_prev_occur (GtkMenuItem * menuitem, gpointer user_data);

void on_comment_block (GtkMenuItem * menuitem, gpointer user_data);
void on_comment_box (GtkMenuItem * menuitem, gpointer user_data);
void on_comment_stream (GtkMenuItem * menuitem, gpointer user_data);

void on_goto_block_start1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_goto_block_end1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_goto_prev_mesg1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_goto_next_mesg1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_edit_app_gui1_activate (GtkMenuItem * menuitem, gpointer user_data);

/*****************************************************************************/
void on_messages1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_project_listing1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_bookmarks1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_breakpoints1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_registers1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_program_stack1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_shared_lib1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_kernal_signals1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_dump_window1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_console1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_showhide_locals (GtkMenuItem * menuitem, gpointer user_data);
void on_anjuta_toolbar_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_editor_linenos1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_editor_markers1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_editor_codefold1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_editor_indentguides1_activate (GtkMenuItem * menuitem,
									   gpointer user_data);
void on_editor_whitespaces1_activate (GtkMenuItem * menuitem,
									  gpointer user_data);
void on_editor_eolchars1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_editor_linewrap1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_zoom_text_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_force_hilite1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_indent1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_update_tagmanager_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_detach1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_ordertab1_activate (GtkMenuItem * menuitem, gpointer user_data);

/*****************************************************************************/

void on_compile1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_make1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_build_project1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_build_dist_project1_activate (GtkMenuItem * menuitem,
									  gpointer user_data);
void on_build_all_project1_activate (GtkMenuItem * menuitem,
									 gpointer user_data);
void on_install_project1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_autogen_project1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_configure_project1_activate (GtkMenuItem * menuitem,
									 gpointer user_data);
void on_clean_project1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_clean_all_project1_activate (GtkMenuItem * menuitem,
									 gpointer user_data);
void on_stop_build_make1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_go_execute1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_go_execute2_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_toggle_breakpoint1_activate (GtkMenuItem * menuitem,
									 gpointer user_data);
void on_set_breakpoint1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_disable_all_breakpoints1_activate (GtkMenuItem * menuitem,
										   gpointer user_data);
void on_show_breakpoints1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_clear_breakpoints1_activate (GtkMenuItem * menuitem,
									 gpointer user_data);

/*****************************************************************************/

void on_execution_continue1_activate (GtkMenuItem * menuitem,
									  gpointer user_data);
void on_execution_step_in1_activate (GtkMenuItem * menuitem,
									 gpointer user_data);
void on_execution_step_out1_activate (GtkMenuItem * menuitem,
									  gpointer user_data);
void on_execution_step_over1_activate (GtkMenuItem * menuitem,
									   gpointer user_data);
void on_execution_run_to_cursor1_activate (GtkMenuItem * menuitem,
										   gpointer user_data);
/*****************************************************************************/

void on_info_targets_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_info_program_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_info_udot_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_info_threads_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_info_variables_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_info_locals_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_info_frame_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_info_args_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_info_memory_activate (GtkMenuItem * menuitem, gpointer user_data);

/*****************************************************************************/
void on_debugger_start_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_debugger_open_exec_activate (GtkMenuItem * menuitem,
									 gpointer user_data);
void on_debugger_attach_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_debugger_load_core_activate (GtkMenuItem * menuitem,
									 gpointer user_data);
void on_debugger_restart_prog_activate (GtkMenuItem * menuitem,
										gpointer user_data);
void on_debugger_stop_prog_activate (GtkMenuItem * menuitem,
									 gpointer user_data);
void on_debugger_detach_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_debugger_stop_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_debugger_confirm_stop_yes_clicked (GtkButton * button,
										   gpointer user_data);
void on_debugger_interrupt_activate (GtkMenuItem * menuitem,
									 gpointer user_data);
void on_debugger_signal_activate (GtkMenuItem * menuitem, gpointer user_data);

void on_debugger_inspect_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_debugger_add_watch_activate (GtkMenuItem * menuitem,
									 gpointer user_data);
void on_debugger_custom_command_activate (GtkMenuItem * menuitem,
										  gpointer user_data);
void on_windows1_new_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_windows1_close_activate (GtkMenuItem * menuitem, gpointer user_data);

/*****************************************************************************/

void on_cvs_update_file_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_cvs_commit_file_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_cvs_status_file_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_cvs_log_file_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_cvs_add_file_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_cvs_remove_file_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_cvs_diff_file_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_cvs_update_project_activate (GtkMenuItem * menuitem,
									 gpointer user_data);
void on_cvs_commit_project_activate (GtkMenuItem * menuitem,
									 gpointer user_data);
void on_cvs_import_project_activate (GtkMenuItem * menuitem,
									 gpointer user_data);
void on_cvs_project_status_activate (GtkMenuItem * menuitem,
									 gpointer user_data);
void on_cvs_project_log_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_cvs_project_diff_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_cvs_login_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_cvs_settings_activate (GtkMenuItem * menuitem, gpointer user_data);

/*****************************************************************************/

void on_set_compiler1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_set_src_paths1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_set_commands1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_edit_user_properties1_activate (GtkMenuItem * menuitem,
										gpointer user_data);
void on_set_preferences1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_set_style_editor_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_file_view_filters_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_set_default_preferences1_activate (GtkMenuItem * menuitem,
										   gpointer user_data);
void on_start_with_dialog_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_setup_wizard_activate (GtkMenuItem * menuitem, gpointer user_data);

/*****************************************************************************/
void on_contents1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_index1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_gnome_pages1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_man_pages1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_info_pages1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_context_help_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_goto_tag_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_lookup_symbol_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_go_back_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_go_forward_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_history_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_search_a_topic1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_url_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_about1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_insert_header (GtkMenuItem * menuitem, gpointer user_data);
void on_insert_cvs_author (GtkMenuItem * menuitem, gpointer user_data);
void on_insert_cvs_date (GtkMenuItem * menuitem, gpointer user_data);
void on_insert_cvs_header (GtkMenuItem * menuitem, gpointer user_data);
void on_insert_cvs_id (GtkMenuItem * menuitem, gpointer user_data);
void on_insert_cvs_log (GtkMenuItem * menuitem, gpointer user_data);
void on_insert_cvs_name (GtkMenuItem * menuitem, gpointer user_data);
void on_insert_cvs_revision (GtkMenuItem * menuitem, gpointer user_data);
void on_insert_cvs_source (GtkMenuItem * menuitem, gpointer user_data);
void on_insert_switch_template (GtkMenuItem * menuitem, gpointer user_data);
void on_insert_for_template (GtkMenuItem * menuitem, gpointer user_data);
void on_insert_while_template (GtkMenuItem * menuitem, gpointer user_data);
void on_insert_ifelse_template (GtkMenuItem * menuitem, gpointer user_data);
void on_insert_c_gpl_notice (GtkMenuItem * menuitem, gpointer user_data);
void on_insert_cpp_gpl_notice (GtkMenuItem * menuitem, gpointer user_data);
void on_insert_py_gpl_notice (GtkMenuItem * menuitem, gpointer user_data);
void on_insert_date_time (GtkMenuItem * menuitem, gpointer user_data);
void on_insert_changelog_entry (GtkMenuItem * menuitem, gpointer user_data);
void on_insert_header_template (GtkMenuItem * menuitem, gpointer user_data);
void on_insert_username (GtkMenuItem * menuitem, gpointer user_data);
void on_save_build_messages_activate (GtkMenuItem * menuitem,
									  gpointer user_data);

void on_findnext1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_enterselection (GtkMenuItem * menuitem, gpointer user_data);
void on_customize_shortcuts_activate (GtkMenuItem * menuitem,
									  gpointer user_data);
void on_tool_editor_activate (GtkMenuItem * menuitem, gpointer user_data);

#endif
