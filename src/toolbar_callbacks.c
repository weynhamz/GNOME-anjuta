/*
    toolbar_callbacks.c
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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gnome.h>

#include "anjuta.h"
#include "text_editor.h"
#include "mainmenu_callbacks.h"
#include "toolbar_callbacks.h"
#include "messagebox.h"
#include "debugger.h"


static gint on_member_combo_entry_sel_idle (gpointer data);

void
on_toolbar_new_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.file.new_file),
				 "activate");
}


void
on_toolbar_open_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.file.open_file),
				 "activate");
}


void
on_toolbar_save_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.file.save_file),
				 "activate");
}

void
on_toolbar_save_all_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.file.save_all_file),
				 "activate");
}

void
on_toolbar_close_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.file.close_file),
				 "activate");
}


void
on_toolbar_reload_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.file.reload_file),
				 "activate");
}


void
on_toolbar_undo_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.edit.undo),
				 "activate");
}


void
on_toolbar_redo_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.edit.redo),
				 "activate");
}

void
on_toolbar_print_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.file.print),
				 "activate");
}

void
on_toolbar_detach_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.format.detach),
				 "activate");
}

void
on_toolbar_find_clicked (GtkButton * button, gpointer user_data)
{
	TextEditor *te;
	gchar *string, *string1;
	gchar buff[512];
	gint ret;

	te = anjuta_get_current_text_editor ();
	if (!te)
		return;
	string1 =
		gtk_entry_get_text (GTK_ENTRY
				    (app->widgets.toolbar.main_toolbar.
				     find_entry));
	if (!string1 || strlen (string1) == 0)
		return;
	string = g_strdup (string1);
	app->find_replace->find_text->find_history =
		update_string_list (app->find_replace->find_text->
				    find_history, string, COMBO_LIST_LENGTH);
	gtk_combo_set_popdown_strings (GTK_COMBO
				       (app->widgets.toolbar.main_toolbar.
					find_combo),
				       app->find_replace->find_text->
				       find_history);

	ret = text_editor_find (te, string,
				TEXT_EDITOR_FIND_SCOPE_CURRENT,
				app->find_replace->find_text->forward,
				app->find_replace->find_text->regexp,
				app->find_replace->find_text->ignore_case,
				app->find_replace->find_text->whole_word);

	sprintf (buff, _("The match \"%s\" was not found from the current location"), string);
	if (ret < 0)
		anjuta_error (buff);
	g_free (string);
}

void
on_toolbar_goto_clicked (GtkButton * button, gpointer user_data)
{
	TextEditor *te;
	guint line;
	gchar *line_ascii;

	te = anjuta_get_current_text_editor ();
	if (te)
	{
		line_ascii =
			gtk_entry_get_text (GTK_ENTRY
					    (app->widgets.toolbar.
					     main_toolbar.line_entry));
		if (strlen (line_ascii) == 0)
			return;
		line = atoi (line_ascii);
		if (text_editor_goto_line (te, line, TRUE) == FALSE)
		{
			anjuta_error (_("There is line no. %d in \"%s\"."),
				 line, te->filename);
		}
	}
}

void
on_toolbar_project_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.view.project_listing),
				 "activate");
}

void
on_toolbar_messages_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.view.messages),
				 "activate");
}


void
on_toolbar_help_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.help.contents),
				 "activate");
}

void
on_toolbar_open_project_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.file.open_project),
				 "activate");
}

void
on_toolbar_save_project_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.file.save_project),
				 "activate");
}

void
on_toolbar_close_project_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.file.close_project),
				 "activate");
}

void
on_toolbar_compile_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.build.compile),
				 "activate");
}

void
on_toolbar_configure_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.build.configure),
				 "activate");
}

void
on_toolbar_build_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.build.build),
				 "activate");
}

void
on_toolbar_build_all_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.build.build_all),
				 "activate");
}

void
on_toolbar_exec_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.build.execute),
				 "activate");
}

void
on_toolbar_debug_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.debug.start_debug),
				 "activate");
}

void
on_toolbar_stop_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.build.stop_build),
				 "activate");
}

void
on_toolbar_go_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.debug.cont),
				 "activate");
}

void
on_toolbar_run_to_cursor_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.debug.run_to_cursor),
				 "activate");
}

void
on_toolbar_step_in_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.debug.step_in),
				 "activate");
}


void
on_toolbar_step_out_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.debug.step_out),
				 "activate");
}

void
on_toolbar_step_over_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.debug.step_over),
				 "activate");
}

void
on_toolbar_toggle_bp_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.debug.tog_break),
				 "activate");
}

void
on_toolbar_watch_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.view.variable_watch),
				 "activate");
}

void
on_toolbar_stack_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.view.program_stack),
				 "activate");
}

void
on_toolbar_registers_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.view.registers),
				 "activate");
}

void
on_toolbar_frame_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.debug.info_frame),
				 "activate");
}

void
on_toolbar_inspect_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.debug.inspect),
				 "activate");
}

void
on_toolbar_interrupt_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.debug.interrupt),
				 "activate");
}

void
on_toolbar_debug_stop_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.debug.stop),
				 "activate");
}

/*******************************************************************************/

void
on_tag_combo_entry_changed (GtkEditable * editable, gpointer user_data)
{
	gchar *label;

	/* Dynamic allocation is necessary, because text in the editable is going to
	 * change very soon
	 */
	label = g_strdup (gtk_entry_get_text (GTK_ENTRY (editable)));
	if (label == NULL)
		return;
	switch (app->tags_manager->menu_type)
	{
	case function_t:
		if (app->tags_manager->cur_file)
			if (strcmp (app->tags_manager->cur_file, label) == 0)
				break;
		gtk_signal_disconnect_by_func (GTK_OBJECT
					       (app->widgets.toolbar.
						tags_toolbar.member_entry),
					       GTK_SIGNAL_FUNC
					       (on_member_combo_entry_changed),
					       NULL);
		gtk_combo_set_popdown_strings (GTK_COMBO
					       (app->widgets.toolbar.
						tags_toolbar.member_combo),
					       tags_manager_get_function_list
					       (app->tags_manager, label));
		if (app->tags_manager->cur_file)
			g_free (app->tags_manager->cur_file);
		app->tags_manager->cur_file = g_strdup (label);
		gtk_signal_connect (GTK_OBJECT
				    (app->widgets.toolbar.tags_toolbar.
				     member_entry), "changed",
				    GTK_SIGNAL_FUNC
				    (on_member_combo_entry_changed), NULL);
		g_free (label);
		break;
	case class_t:
		gtk_signal_disconnect_by_func (GTK_OBJECT
					       (app->widgets.toolbar.
						tags_toolbar.member_entry),
					       GTK_SIGNAL_FUNC
					       (on_member_combo_entry_changed),
					       NULL);
		gtk_combo_set_popdown_strings (GTK_COMBO
					       (app->widgets.toolbar.
						tags_toolbar.member_combo),
					       tags_manager_get_mem_func_list
					       (app->tags_manager, label));
		g_free (label);
		gtk_signal_connect (GTK_OBJECT
				    (app->widgets.toolbar.tags_toolbar.
				     member_entry), "changed",
				    GTK_SIGNAL_FUNC
				    (on_member_combo_entry_changed), NULL);
		break;
	default:
		gtk_timeout_add (10, on_member_combo_entry_sel_idle, label);
/*             on_member_combo_entry_sel_idle(label); */
	}
}

void
on_tag_combo_list_select_child (GtkList * list,
				GtkWidget * widget, gpointer user_data)
{
}

void
on_member_combo_entry_changed (GtkEditable * editable, gpointer user_data)
{
	gchar *label;

	switch (app->tags_manager->menu_type)
	{
	case function_t:
		label = g_strdup (gtk_entry_get_text (GTK_ENTRY (editable)));
		break;
	case class_t:
		label =
			g_strconcat (gtk_entry_get_text
				     (GTK_ENTRY
				      (app->widgets.toolbar.tags_toolbar.
				       tag_entry)), "::",
				     gtk_entry_get_text (GTK_ENTRY
							 (editable)), NULL);
		break;
	default:
		label = NULL;
		break;
	}
	if (label)
	{
		gtk_timeout_add (10, on_member_combo_entry_sel_idle, label);
/*        on_member_combo_entry_sel_idle(label); */
	}
}

static gint
on_member_combo_entry_sel_idle (gpointer data)
{
	gchar *file;
	guint line;
	gchar *label = data;

	anjuta_set_busy ();

/*	update_gtk ();*/
	tags_manager_block_draw (app->tags_manager);
	tags_manager_get_tag_info (app->tags_manager, label, &file, &line);
	if (file)
	{
		anjuta_goto_file_line (file, line);
		anjuta_grab_text_focus ();
		g_free (file);
	}
	g_free (label);
	anjuta_set_active ();
	tags_manager_unblock_draw (app->tags_manager);
	return FALSE;
}

void
on_member_combo_list_select_child (GtkList * list,
				   GtkWidget * widget, gpointer user_data)
{
}

void
on_browser_widzard_clicked (GtkButton * button, gpointer user_data)
{

}

void
on_browser_toggle_bookmark_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.bookmark.toggle),
				 "activate");
}

void
on_browser_first_bookmark_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.bookmark.first),
				 "activate");
}

void
on_browser_prev_bookmark_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.bookmark.prev),
				 "activate");
}

void
on_browser_next_bookmark_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.bookmark.next),
				 "activate");
}

void
on_browser_last_bookmark_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.bookmark.last),
				 "activate");
}

void
on_browser_prev_mesg_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.edit.goto_prev_mesg),
				 "activate");
}

void
on_browser_next_mesg_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.edit.goto_next_mesg),
				 "activate");
}

void
on_browser_block_start_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.edit.goto_block_start),
				 "activate");
}

void
on_browser_block_end_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.edit.goto_block_end),
				 "activate");
}

void
on_format_fold_toggle_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.format.toggle_fold),
				 "activate");
}

void
on_format_fold_open_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.format.open_folds),
				 "activate");
}

void
on_format_fold_close_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.format.close_folds),
				 "activate");
}

void
on_format_indent_inc_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.format.indent_inc),
				 "activate");
}

void
on_format_indent_dcr_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.format.indent_dcr),
				 "activate");
}

void
on_format_indent_auto_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.format.indent),
				 "activate");
}

void
on_format_indent_style_clicked (GtkButton * button, gpointer user_data)
{
	gtk_notebook_set_page (GTK_NOTEBOOK
			       (app->preferences->widgets.notebook), 5);
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.settings.preferences),
				 "activate");
}

void
on_format_block_select_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.edit.select_block),
				 "activate");
}

void
on_format_calltip_clicked (GtkButton * button, gpointer user_data)
{
}

void
on_format_autocomplete_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.edit.autocomplete),
				 "activate");
}

void
on_tag_functions_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	if (app->tags_manager->menu_type == function_t)
		return;
	app->tags_manager->menu_type = function_t;
	app->tags_manager->update_required_tags = TRUE;
	app->tags_manager->update_required_mems = TRUE;
	tags_manager_update_menu (app->tags_manager);
}


void
on_tag_classes_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	if (app->tags_manager->menu_type == class_t)
		return;
	app->tags_manager->menu_type = class_t;
	app->tags_manager->update_required_tags = TRUE;
	app->tags_manager->update_required_mems = TRUE;
	tags_manager_update_menu (app->tags_manager);
}


void
on_tag_structs_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	if (app->tags_manager->menu_type == struct_t)
		return;
	app->tags_manager->menu_type = struct_t;
	app->tags_manager->update_required_tags = TRUE;
	tags_manager_update_menu (app->tags_manager);
}


void
on_tag_unions_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	if (app->tags_manager->menu_type == union_t)
		return;
	app->tags_manager->menu_type = union_t;
	app->tags_manager->update_required_tags = TRUE;
	tags_manager_update_menu (app->tags_manager);
}


void
on_tag_enums_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	if (app->tags_manager->menu_type == enum_t)
		return;
	app->tags_manager->menu_type = enum_t;
	app->tags_manager->update_required_tags = TRUE;
	tags_manager_update_menu (app->tags_manager);
}


void
on_tag_variables_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	if (app->tags_manager->menu_type == variable_t)
		return;
	app->tags_manager->menu_type = variable_t;
	app->tags_manager->update_required_tags = TRUE;
	tags_manager_update_menu (app->tags_manager);
}


void
on_tag_macros_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	if (app->tags_manager->menu_type == macro_t)
		return;
	app->tags_manager->menu_type = macro_t;
	app->tags_manager->update_required_tags = TRUE;
	tags_manager_update_menu (app->tags_manager);
}
