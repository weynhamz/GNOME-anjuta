/* 
    toolbar_callbacks.h
    Copyright (C) 2000  Kh. Naba Kumar Singh

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef AN_TOOLBAR_CALLBACKS_H
#define AN_TOOLBAR_CALLBACKS_H

#include <gnome.h>
#include <libegg/menu/egg-action.h>

#ifdef __cplusplus
extern "C"
{
#endif

void
on_toolbar_new_clicked                 (EggAction       *action,
                                        gpointer         user_data);

void
on_toolbar_open_clicked                (EggAction       *action,
                                        gpointer         user_data);

void
on_toolbar_save_clicked                (EggAction       *action,
                                        gpointer         user_data);

void
on_toolbar_save_all_clicked                (EggAction       *action,
                                        gpointer         user_data);
void
on_toolbar_close_clicked               (EggAction       *action,
                                        gpointer         user_data);

void
on_toolbar_reload_clicked              (EggAction       *action,
                                        gpointer         user_data);

void
on_toolbar_undo_clicked                (EggAction       *action,
                                        gpointer         user_data);

void
on_toolbar_redo_clicked                (EggAction       *action,
                                        gpointer         user_data);

void
on_toolbar_print_clicked                (EggAction       *action,
                                        gpointer         user_data);

void
on_toolbar_detach_clicked                (EggAction       *action,
                                        gpointer         user_data);

gboolean on_toolbar_find_incremental_start (EggAction *action, gpointer user_data);
gboolean on_toolbar_find_incremental_end (EggAction *action, gpointer user_data);
void on_toolbar_find_incremental (EggAction *action, gpointer user_data);
void on_toolbar_find_clicked (EggAction *action, gpointer user_data);

void on_toolbar_goto_activate (EggAction *action, gpointer user_data);
void on_toolbar_goto_clicked (EggAction *action, gpointer user_data);

void
on_toolbar_tag_clicked                (EggAction       *action,
                                        gpointer         user_data);

void
on_toolbar_project_clicked                (EggAction       *action,
                                        gpointer         user_data);
void
on_toolbar_messages_clicked                (EggAction       *action,
                                        gpointer         user_data);

void
on_toolbar_help_clicked                (EggAction       *action,
                                        gpointer         user_data);


void
on_toolbar_class_entry_changed         (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_toolbar_function_entry_changed      (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_toolbar_open_project_clicked                (EggAction       *action,
                                        gpointer         user_data);
void
on_toolbar_save_project_clicked                (EggAction       *action,
                                        gpointer         user_data);
void
on_toolbar_close_project_clicked                (EggAction       *action,
                                        gpointer         user_data);

void
on_toolbar_compile_clicked             (EggAction       *action,
                                        gpointer         user_data);

void
on_toolbar_configure_clicked             (EggAction       *action,
                                        gpointer         user_data);

void
on_toolbar_build_clicked               (EggAction       *action,
                                        gpointer         user_data);
void
on_toolbar_build_all_clicked               (EggAction       *action,
                                        gpointer         user_data);

void
on_toolbar_exec_clicked               (EggAction       *action,
                                        gpointer         user_data);
void
on_toolbar_debug_clicked               (EggAction       *action,
                                        gpointer         user_data);

void
on_toolbar_stop_clicked                (EggAction       *action,
                                        gpointer         user_data);

void
on_toolbar_go_clicked                  (EggAction       *action,
                                        gpointer         user_data);
void
on_toolbar_run_to_cursor_clicked (EggAction * action, gpointer user_data);

void
on_toolbar_step_in_clicked                (EggAction       *action,
                                        gpointer         user_data);

void
on_toolbar_step_out_clicked        (EggAction       *action,
                                        gpointer         user_data);

void
on_toolbar_step_over_clicked        (EggAction       *action,
                                        gpointer         user_data);

void
on_toolbar_toggle_bp_clicked           (EggAction       *action,
                                        gpointer         user_data);
/*
void
on_toolbar_watch_clicked           (EggAction       *action,
                                        gpointer         user_data);
*/
void
on_toolbar_stack_clicked           (EggAction       *action,
                                        gpointer         user_data);
void
on_toolbar_registers_clicked           (EggAction       *action,
                                        gpointer         user_data);
void
on_toolbar_frame_clicked           (EggAction       *action,
                                        gpointer         user_data);
void
on_toolbar_inspect_clicked           (EggAction       *action,
                                        gpointer         user_data);
void
on_toolbar_interrupt_clicked           (EggAction       *action,
                                        gpointer         user_data);
void
on_toolbar_debug_stop_clicked           (EggAction       *action,
                                        gpointer         user_data);
void
on_browser_wizard_clicked  (EggAction       *action,
                                        gpointer         user_data);
void
on_browser_toggle_bookmark_clicked  (EggAction       *action,
                                        gpointer         user_data);
void
on_browser_first_bookmark_clicked  (EggAction       *action,
                                        gpointer         user_data);
void
on_browser_prev_bookmark_clicked  (EggAction       *action,
                                        gpointer         user_data);
void
on_browser_next_bookmark_clicked  (EggAction       *action,
                                        gpointer         user_data);
void
on_browser_last_bookmark_clicked  (EggAction       *action,
                                        gpointer         user_data);
void
on_browser_prev_mesg_clicked  (EggAction       *action,
                                        gpointer         user_data);
void
on_browser_next_mesg_clicked  (EggAction       *action,
                                        gpointer         user_data);
void
on_browser_block_start_clicked   (EggAction       *action,
                  gpointer         user_data);
void
on_browser_block_end_clicked   (EggAction       *action,
                  gpointer         user_data);

void
on_format_fold_toggle_clicked   (EggAction       *action,
                  gpointer         user_data);
void
on_format_fold_open_clicked   (EggAction       *action,
                  gpointer         user_data);
void
on_format_fold_close_clicked   (EggAction       *action,
                  gpointer         user_data);
void
on_format_indent_inc_clicked   (EggAction       *action,
                  gpointer         user_data);
void
on_format_indent_dcr_clicked   (EggAction       *action,
                  gpointer         user_data);
void
on_format_indent_auto_clicked   (EggAction       *action,
                  gpointer         user_data);
void
on_format_indent_style_clicked   (EggAction       *action,
                  gpointer         user_data);
void
on_format_block_select_clicked   (EggAction       *action,
                  gpointer         user_data);
void
on_format_calltip_clicked   (EggAction       *action,
                  gpointer         user_data);
void
on_format_autocomplete_clicked   (EggAction       *action,
                  gpointer         user_data);

#ifdef __cplusplus
}
#endif

#endif /* AN_TOOLBAR_CALLBACKS_H */
