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
#include "debugger.h"
#include "search_incremental.h"

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
on_toolbar_goto_clicked (GtkButton * button, gpointer user_data)
{
	TextEditor *te;
	guint line;
	const gchar *line_ascii;

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
		if (text_editor_goto_line (te, line, TRUE, TRUE) == FALSE)
		{
			anjuta_error (_("There is line number %d in \"%s\"."),
				 line, te->filename);
		}
	}
}

void
on_toolbar_tag_clicked (GtkButton * button, gpointer user_data)
{
	TextEditor *te;
	const gchar *string;

	te = anjuta_get_current_text_editor ();
	if (!te)
		return;
	string =
		gtk_entry_get_text (GTK_ENTRY
		  (app->widgets.toolbar.browser_toolbar.tag_entry));
	if (!string || strlen (string) == 0)
		return;
	anjuta_goto_local_tag(te, string);
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
	on_context_help_activate(NULL, NULL);
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

/*
void
on_toolbar_watch_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.view.variable_watch),
				 "activate");
}
*/
void
on_toolbar_signals_clicked (GtkButton * button, gpointer user_data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.view.signals),
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
on_browser_wizard_clicked (GtkButton * button, gpointer user_data)
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
/*	gtk_notebook_set_page (GTK_NOTEBOOK
			       (app->preferences->widgets.notebook), 5);
*/	gtk_signal_emit_by_name (GTK_OBJECT
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
