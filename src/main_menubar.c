/*
 * main_menubar.c
 * Copyright (C) 2000  Kh. Naba Kumar Singh
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>

#include "anjuta.h"
#include "mainmenu_callbacks.h"
#include "main_menubar_def.h"
#include "fileselection.h"
#include "scroll-menu.h"

static void on_file_menu_realize (GtkWidget * widget, gpointer data);
static void on_project_menu_realize (GtkWidget * widget, gpointer data);
static void on_plugins_menu_realize (GtkWidget * widget, gpointer data);
static void on_plugins_menu_item_activate (GtkMenuItem * item, AnjutaAddInPtr pPlug );

void
create_main_menubar (GtkWidget * ap, MainMenuBar * mb)
{
	int i;
	gnome_app_create_menus (GNOME_APP (ap), menubar1_uiinfo);

	/* File submenu */
	mb->file.new_file = file1_menu_uiinfo[0].widget;
	mb->file.open_file = file1_menu_uiinfo[1].widget;
	mb->file.save_file = file1_menu_uiinfo[2].widget;
	mb->file.save_as_file = file1_menu_uiinfo[3].widget;
	mb->file.save_all_file = file1_menu_uiinfo[4].widget;
	mb->file.close_file = file1_menu_uiinfo[5].widget;
	mb->file.close_all_file = file1_menu_uiinfo[6].widget;
	mb->file.reload_file = file1_menu_uiinfo[7].widget;
	mb->file.new_project = file1_menu_uiinfo[9].widget;
	mb->file.import_project = file1_menu_uiinfo[10].widget;
	mb->file.open_project = file1_menu_uiinfo[11].widget;
	mb->file.save_project = file1_menu_uiinfo[12].widget;
	mb->file.close_project = file1_menu_uiinfo[13].widget;
	mb->file.rename = file1_menu_uiinfo[15].widget;
	mb->file.page_setup = file1_menu_uiinfo[17].widget;
	mb->file.print = file1_menu_uiinfo[18].widget;
	mb->file.recent_files = file1_menu_uiinfo[20].widget;
	mb->file.recent_projects = file1_menu_uiinfo[21].widget;
	mb->file.exit = file1_menu_uiinfo[23].widget;

	/* Edit submenu */
	mb->edit.uppercase = transform1_submenu_uiinfo[0].widget;
	mb->edit.lowercase = transform1_submenu_uiinfo[1].widget;
	mb->edit.convert_crlf = transform1_submenu_uiinfo[3].widget;
	mb->edit.convert_lf = transform1_submenu_uiinfo[4].widget;
	mb->edit.convert_cr = transform1_submenu_uiinfo[5].widget;
	mb->edit.convert_auto = transform1_submenu_uiinfo[6].widget;
	mb->edit.insert_c_gpl = inserttext1_submenu_uiinfo[0].widget;
	mb->edit.insert_cpp_gpl = inserttext1_submenu_uiinfo[1].widget;
	mb->edit.insert_py_gpl = inserttext1_submenu_uiinfo[2].widget;
	mb->edit.insert_username = inserttext1_submenu_uiinfo[3].widget;
	mb->edit.insert_datetime = inserttext1_submenu_uiinfo[4].widget;
	mb->edit.insert_header_template = inserttext1_submenu_uiinfo[5].widget;
	mb->edit.undo = edit1_menu_uiinfo[0].widget;
	mb->edit.redo = edit1_menu_uiinfo[1].widget;
	mb->edit.cut = edit1_menu_uiinfo[3].widget;
	mb->edit.copy = edit1_menu_uiinfo[4].widget;
	mb->edit.paste = edit1_menu_uiinfo[5].widget;
	mb->edit.clear = edit1_menu_uiinfo[6].widget;
	mb->edit.select_all = select1_submenu_uiinfo[0].widget;
	mb->edit.select_brace = select1_submenu_uiinfo[1].widget;
	mb->edit.select_block = select1_submenu_uiinfo[2].widget;
	mb->edit.autocomplete = edit1_menu_uiinfo[12].widget;
	mb->edit.calltip = edit1_menu_uiinfo[13].widget;
	mb->edit.find = edit1_menu_uiinfo[15].widget;
	mb->edit.find_in_files = edit1_menu_uiinfo[16].widget;
	mb->edit.find_replace = edit1_menu_uiinfo[17].widget;
	mb->edit.goto_line = goto1_submenu_uiinfo[0].widget;
	mb->edit.goto_brace = goto1_submenu_uiinfo[1].widget;
	mb->edit.goto_block_start = goto1_submenu_uiinfo[2].widget;
	mb->edit.goto_block_end = goto1_submenu_uiinfo[3].widget;
	mb->edit.goto_prev_mesg = goto1_submenu_uiinfo[4].widget;
	mb->edit.goto_next_mesg = goto1_submenu_uiinfo[5].widget;
	mb->edit.repeat_find = edit1_menu_uiinfo[18].widget;
	mb->edit.edit_app_gui = edit1_menu_uiinfo[21].widget;

	/* View Submenu */
	mb->view.main_toolbar = toolbar1_submenu_uiinfo[0].widget;
	mb->view.extended_toolbar = toolbar1_submenu_uiinfo[1].widget;
	mb->view.debug_toolbar = toolbar1_submenu_uiinfo[2].widget;
	mb->view.browser_toolbar = toolbar1_submenu_uiinfo[3].widget;
	mb->view.format_toolbar = toolbar1_submenu_uiinfo[4].widget;
	mb->view.editor_linenos = editor1_submenu_uiinfo[0].widget;
	mb->view.editor_markers = editor1_submenu_uiinfo[1].widget;
	mb->view.editor_folds = editor1_submenu_uiinfo[2].widget;
	mb->view.editor_indentguides = editor1_submenu_uiinfo[3].widget;
	mb->view.editor_whitespaces = editor1_submenu_uiinfo[4].widget;
	mb->view.editor_eolchars = editor1_submenu_uiinfo[5].widget;
	mb->view.messages = view1_menu_uiinfo[0].widget;
	mb->view.project_listing = view1_menu_uiinfo[1].widget;
	mb->view.bookmarks = view1_menu_uiinfo[2].widget;
	mb->view.breakpoints = view1_menu_uiinfo[8].widget;
	mb->view.variable_watch = view1_menu_uiinfo[9].widget;
	mb->view.registers = view1_menu_uiinfo[10].widget;
	mb->view.program_stack = view1_menu_uiinfo[11].widget;
	mb->view.shared_lib = view1_menu_uiinfo[12].widget;
	mb->view.signals = view1_menu_uiinfo[13].widget;
	mb->view.dump_window = view1_menu_uiinfo[14].widget;
	mb->view.console = view1_menu_uiinfo[16].widget;
	mb->view.show_hide_locals = view1_menu_uiinfo[17].widget;
	
	/* Project submenu */
	mb->project.add_inc = import_file1_menu_uiinfo[0].widget;
	mb->project.add_src = import_file1_menu_uiinfo[1].widget;
	mb->project.add_hlp = import_file1_menu_uiinfo[2].widget;
	mb->project.add_data = import_file1_menu_uiinfo[3].widget;
	mb->project.add_pix = import_file1_menu_uiinfo[4].widget;
	mb->project.add_po = import_file1_menu_uiinfo[5].widget;
	mb->project.add_doc = import_file1_menu_uiinfo[6].widget;
	mb->project.new_file = project1_menu_uiinfo[0].widget;
	mb->project.remove = project1_menu_uiinfo[2].widget;
	mb->project.configure = project1_menu_uiinfo[4].widget;
	mb->project.info = project1_menu_uiinfo[5].widget;
	
	/* Format submenu */
	mb->format.indent = format1_menu_uiinfo[0].widget;
	mb->format.indent_inc = format1_menu_uiinfo[1].widget;
	mb->format.indent_dcr = format1_menu_uiinfo[2].widget;
	mb->format.update_tags = format1_menu_uiinfo[4].widget;
	mb->format.force_hilite = format1_menu_uiinfo[6].widget;
	mb->format.close_folds = format1_menu_uiinfo[8].widget;
	mb->format.open_folds = format1_menu_uiinfo[9].widget;
	mb->format.toggle_fold = format1_menu_uiinfo[10].widget;
	mb->format.detach = format1_menu_uiinfo[12].widget;
	
	/* Build submenu */
	mb->build.compile = build1_menu_uiinfo[0].widget;
	mb->build.make = build1_menu_uiinfo[1].widget;
	mb->build.build = build1_menu_uiinfo[2].widget;
	mb->build.build_all = build1_menu_uiinfo[3].widget;
	mb->build.save_build_messages = build1_menu_uiinfo[5].widget;
	mb->build.install = build1_menu_uiinfo[7].widget;
	mb->build.build_dist = build1_menu_uiinfo[8].widget;
	mb->build.configure = build1_menu_uiinfo[10].widget;
	mb->build.autogen = build1_menu_uiinfo[11].widget;
	mb->build.clean = build1_menu_uiinfo[13].widget;
	mb->build.clean_all = build1_menu_uiinfo[14].widget;
	mb->build.stop_build = build1_menu_uiinfo[16].widget;
	mb->build.execute = build1_menu_uiinfo[18].widget;
	mb->build.execute_params = build1_menu_uiinfo[19].widget;
	
	/* Bookmark submenu */
	mb->bookmark.toggle = bookmark1_menu_uiinfo[0].widget;
	mb->bookmark.first = bookmark1_menu_uiinfo[2].widget;
	mb->bookmark.prev = bookmark1_menu_uiinfo[3].widget;
	mb->bookmark.next = bookmark1_menu_uiinfo[4].widget;
	mb->bookmark.last = bookmark1_menu_uiinfo[5].widget;
	mb->bookmark.clear = bookmark1_menu_uiinfo[7].widget;

	/* Debug Submenu */
	mb->debug.cont = execution1_submenu_uiinfo[0].widget;
	mb->debug.step_in = execution1_submenu_uiinfo[1].widget;
	mb->debug.step_over = execution1_submenu_uiinfo[2].widget;
	mb->debug.step_out = execution1_submenu_uiinfo[3].widget;
	mb->debug.run_to_cursor = execution1_submenu_uiinfo[4].widget;
	mb->debug.tog_break = breakpoints1_submenu_uiinfo[0].widget;
	mb->debug.set_break = breakpoints1_submenu_uiinfo[1].widget;
	mb->debug.show_breakpoints = breakpoints1_submenu_uiinfo[3].widget;
	mb->debug.disable_all_breakpoints = breakpoints1_submenu_uiinfo[4].widget;
	mb->debug.clear_all_breakpoints = breakpoints1_submenu_uiinfo[5].widget;
	mb->debug.info_targets = info1_submenu_uiinfo[0].widget;
	mb->debug.info_program = info1_submenu_uiinfo[1].widget;
	mb->debug.info_udot = info1_submenu_uiinfo[2].widget;
	mb->debug.info_threads = info1_submenu_uiinfo[3].widget;
	mb->debug.info_variables = info1_submenu_uiinfo[4].widget;
	mb->debug.info_locals = info1_submenu_uiinfo[5].widget;
	mb->debug.info_frame = info1_submenu_uiinfo[6].widget;
	mb->debug.info_args = info1_submenu_uiinfo[7].widget;
	mb->debug.start_debug = debug1_menu_uiinfo[0].widget;
	mb->debug.open_exec = debug1_menu_uiinfo[2].widget;
	mb->debug.load_core = debug1_menu_uiinfo[3].widget;
	mb->debug.attach = debug1_menu_uiinfo[4].widget;
	mb->debug.restart = debug1_menu_uiinfo[6].widget;
	mb->debug.stop_prog = debug1_menu_uiinfo[7].widget;
	mb->debug.detach = debug1_menu_uiinfo[8].widget;
	mb->debug.interrupt = debug1_menu_uiinfo[10].widget;
	mb->debug.send_signal = debug1_menu_uiinfo[11].widget;
	mb->debug.inspect = debug1_menu_uiinfo[18].widget;
	mb->debug.add_watch = debug1_menu_uiinfo[19].widget;
	mb->debug.stop = debug1_menu_uiinfo[21].widget;
	
	mb->cvs.update_file = cvs_menu_uiinfo[0].widget;
	mb->cvs.commit_file = cvs_menu_uiinfo[1].widget;
	mb->cvs.status_file = cvs_menu_uiinfo[2].widget;
	mb->cvs.add_file = cvs_menu_uiinfo[3].widget;
	mb->cvs.remove_file = cvs_menu_uiinfo[4].widget;
	mb->cvs.diff_file = cvs_menu_uiinfo[5].widget;
	mb->cvs.update_project = cvs_menu_uiinfo[7].widget;
	mb->cvs.commit_project = cvs_menu_uiinfo[8].widget;
	mb->cvs.import_project = cvs_menu_uiinfo[9].widget;
	mb->cvs.status_project = cvs_menu_uiinfo[10].widget;
	mb->cvs.diff_project = cvs_menu_uiinfo[11].widget;
	mb->cvs.login = cvs_menu_uiinfo[13].widget;
	mb->cvs.settings = cvs_menu_uiinfo[15].widget;
	
	/* not implemented yet */
	for (i = 7; i < 12; i++)
		gtk_widget_hide(cvs_menu_uiinfo[i].widget);
	
	/* Settings submenu */
	mb->settings.compiler = settings1_menu_uiinfo[0].widget;
	mb->settings.src_paths = settings1_menu_uiinfo[1].widget;
	mb->settings.commands = settings1_menu_uiinfo[2].widget;
	mb->settings.preferences = settings1_menu_uiinfo[4].widget;
	mb->settings.default_preferences = settings1_menu_uiinfo[5].widget;
	
	/* Help submenu */
	mb->help.gnome = help1_menu_uiinfo[2].widget;
	mb->help.man = help1_menu_uiinfo[3].widget;
	mb->help.info = help1_menu_uiinfo[4].widget;
	mb->help.context_help = help1_menu_uiinfo[6].widget;
	mb->help.search = help1_menu_uiinfo[7].widget;
	mb->help.about = help1_menu_uiinfo[17].widget;
	
	/* Unimplemented */
	gtk_widget_hide (file1_menu_uiinfo[15].widget);
	gtk_widget_hide (file1_menu_uiinfo[16].widget);
	gtk_widget_hide (view1_menu_uiinfo[2].widget);
	gtk_widget_hide (view1_menu_uiinfo[14].widget);
	gtk_widget_hide (view1_menu_uiinfo[16].widget);
	gtk_widget_hide (import_file1_menu_uiinfo[0].widget);
	gtk_widget_hide (project1_menu_uiinfo[0].widget);

	/* Note: this is because we don't know yet what's the
	 *contents of these menus
	 */
	/* Recent files and project submenu */
	gtk_signal_connect (GTK_OBJECT (mb->file.recent_files), "realize",
			    GTK_SIGNAL_FUNC (on_file_menu_realize), NULL);
	gtk_signal_connect (GTK_OBJECT (mb->file.recent_projects), "realize",
			    GTK_SIGNAL_FUNC (on_project_menu_realize), NULL);
	
	/* Plugin Menu */
	gtk_signal_connect (GTK_OBJECT (menubar1_uiinfo[8].widget), "realize",
			    GTK_SIGNAL_FUNC (on_plugins_menu_realize), NULL);
}

void
main_menu_install_hints (GtkWidget* ap)
{
	gint i;
	
	for (i = 0; i < NUM_FILE_SUBMENUS; i++) {
		gtk_widget_ref (file1_menu_uiinfo[i].widget);
		gtk_accel_group_attach(GNOME_APP(ap)->accel_group,
			GTK_OBJECT(file1_menu_uiinfo[i].widget));
	}

	for (i = 0; i < NUM_TRANSFORM_SUBMENUS ; i++) {
		gtk_widget_ref (transform1_submenu_uiinfo[i].widget);
		gtk_accel_group_attach(GNOME_APP(ap)->accel_group,
			GTK_OBJECT(transform1_submenu_uiinfo[i].widget));
	}
	
	for (i = 0; i < NUM_SELECT_SUBMENUS; i++) {
		gtk_widget_ref (select1_submenu_uiinfo[i].widget);
		gtk_accel_group_attach(GNOME_APP(ap)->accel_group,
			GTK_OBJECT(select1_submenu_uiinfo[i].widget));
	}
	
	for (i = 0; i < NUM_GOTO_SUBMENUS; i++) {
		gtk_widget_ref (goto1_submenu_uiinfo[i].widget);
		gtk_accel_group_attach(GNOME_APP(ap)->accel_group,
			GTK_OBJECT(goto1_submenu_uiinfo[i].widget));
	}
	
	for (i = 0; i < NUM_INSERTTEXT_SUBMENUS; i++) {
		gtk_widget_ref (inserttext1_submenu_uiinfo[i].widget);
		gtk_accel_group_attach(GNOME_APP(ap)->accel_group,
			GTK_OBJECT(inserttext1_submenu_uiinfo[i].widget));
	}
	
	for (i = 0; i < NUM_EDIT_SUBMENUS ; i++) {
		gtk_widget_ref (edit1_menu_uiinfo[i].widget);
		gtk_accel_group_attach(GNOME_APP(ap)->accel_group,
			GTK_OBJECT(edit1_menu_uiinfo[i].widget));
	}
	
	for (i = 0; i < NUM_TOOLBAR_SUBMENUS; i++) {
		gtk_widget_ref (toolbar1_submenu_uiinfo[i].widget);
		gtk_accel_group_attach(GNOME_APP(ap)->accel_group,
			GTK_OBJECT(toolbar1_submenu_uiinfo[i].widget));
	}
	
	for (i = 0; i < NUM_EDITOR_SUBMENUS; i++) {
		gtk_widget_ref (editor1_submenu_uiinfo[i].widget);
		gtk_accel_group_attach(GNOME_APP(ap)->accel_group,
			GTK_OBJECT(editor1_submenu_uiinfo[i].widget));
	}
	
	for (i = 0; i < NUM_VIEW_SUBMENUS; i++) {
		gtk_widget_ref (view1_menu_uiinfo[i].widget);
		gtk_accel_group_attach(GNOME_APP(ap)->accel_group,
			GTK_OBJECT(view1_menu_uiinfo[i].widget));
	}
	
	for (i = 0; i < NUM_ZOOMTEXT_SUBMENUS; i++) {
		gtk_widget_ref (zoom_text1_submenu_uiinfo[i].widget);
		gtk_accel_group_attach(GNOME_APP(ap)->accel_group,
			GTK_OBJECT(zoom_text1_submenu_uiinfo[i].widget));
	}
	
	for (i = 0; i < NUM_IMPORTFILE_SUBMENUS; i++) {
		gtk_widget_ref (import_file1_menu_uiinfo[i].widget);
		gtk_accel_group_attach(GNOME_APP(ap)->accel_group,
			GTK_OBJECT(import_file1_menu_uiinfo[i].widget));
	}
	
	for (i = 0; i < NUM_PROJECT_SUBMENUS; i++) {
		gtk_widget_ref (project1_menu_uiinfo[i].widget);
		gtk_accel_group_attach(GNOME_APP(ap)->accel_group,
			GTK_OBJECT(project1_menu_uiinfo[i].widget));
	}
	
	for (i = 0; i < NUM_HILITE_SUBMENUS; i++) {
		gtk_widget_ref (hilitetype1_submenu_uiinfo[i].widget);
		gtk_accel_group_attach(GNOME_APP(ap)->accel_group,
			GTK_OBJECT(hilitetype1_submenu_uiinfo[i].widget));
	}
	
	for (i = 0; i < NUM_FORMAT_SUBMENUS; i++) {
		gtk_widget_ref (format1_menu_uiinfo[i].widget);
		gtk_accel_group_attach(GNOME_APP(ap)->accel_group,
			GTK_OBJECT(format1_menu_uiinfo[i].widget));
	}
	
	for (i = 0; i < NUM_BUILD_SUBMENUS; i++) {
		gtk_widget_ref (build1_menu_uiinfo[i].widget);
		gtk_accel_group_attach(GNOME_APP(ap)->accel_group,
			GTK_OBJECT(build1_menu_uiinfo[i].widget));
	}
	
	for (i = 0; i < NUM_BOOKMARK_SUBMENUS; i++) {
		gtk_widget_ref (bookmark1_menu_uiinfo[i].widget);
		gtk_accel_group_attach(GNOME_APP(ap)->accel_group,
			GTK_OBJECT(bookmark1_menu_uiinfo[i].widget));
	}
	
	for (i = 0; i < NUM_EXECUTION_SUBMENUS; i++) {
		gtk_widget_ref (execution1_submenu_uiinfo[i].widget);
		gtk_accel_group_attach(GNOME_APP(ap)->accel_group,
			GTK_OBJECT(execution1_submenu_uiinfo[i].widget));
	}
	
	for (i = 0; i < NUM_BREAKPOINTS_SUBMENUS; i++) {
		gtk_widget_ref (breakpoints1_submenu_uiinfo[i].widget);
		gtk_accel_group_attach(GNOME_APP(ap)->accel_group,
			GTK_OBJECT(breakpoints1_submenu_uiinfo[i].widget));
	}
	
	for (i = 0; i < NUM_INFO_SUBMENUS; i++) {
		gtk_widget_ref (info1_submenu_uiinfo[i].widget);
		gtk_accel_group_attach(GNOME_APP(ap)->accel_group,
			GTK_OBJECT(info1_submenu_uiinfo[i].widget));
	}
	
	for (i = 0; i < NUM_DEBUG_SUBMENUS; i++) {
		gtk_widget_ref (debug1_menu_uiinfo[i].widget);
		gtk_accel_group_attach(GNOME_APP(ap)->accel_group,
			GTK_OBJECT(debug1_menu_uiinfo[i].widget));
	}
	
	for (i = 0; i < NUM_SETTINGS_SUBMENUS; i++) {
		gtk_widget_ref (settings1_menu_uiinfo[i].widget);
		gtk_accel_group_attach(GNOME_APP(ap)->accel_group,
			GTK_OBJECT(settings1_menu_uiinfo[i].widget));
	}
	
	for (i = 1; i < NUM_HELP_SUBMENUS; i++) {
		gtk_widget_ref (help1_menu_uiinfo[i].widget);
		gtk_accel_group_attach(GNOME_APP(ap)->accel_group,
			GTK_OBJECT(help1_menu_uiinfo[i].widget));
	}
	
	gnome_app_install_appbar_menu_hints (GNOME_APPBAR
					     (app->widgets.appbar),
					     menubar1_uiinfo);
}

void
main_menu_unref ()
{
	gint i;
	
	for (i = 0; i < NUM_FILE_SUBMENUS; i++)
		gtk_widget_unref (file1_menu_uiinfo[i].widget);
	
	for (i = 0; i < NUM_TRANSFORM_SUBMENUS ; i++)
		gtk_widget_unref (transform1_submenu_uiinfo[i].widget);
	
	for (i = 0; i < NUM_SELECT_SUBMENUS; i++)
		gtk_widget_unref (select1_submenu_uiinfo[i].widget);
	
	for (i = 0; i < NUM_GOTO_SUBMENUS; i++)
		gtk_widget_unref (goto1_submenu_uiinfo[i].widget);
	
	for (i = 0; i < NUM_INSERTTEXT_SUBMENUS; i++)
		gtk_widget_unref (inserttext1_submenu_uiinfo[i].widget);
	
	for (i = 0; i < NUM_EDIT_SUBMENUS ; i++)
		gtk_widget_unref (edit1_menu_uiinfo[i].widget);
	
	for (i = 0; i < NUM_TOOLBAR_SUBMENUS; i++)
		gtk_widget_unref (toolbar1_submenu_uiinfo[i].widget);
	
	for (i = 0; i < NUM_EDITOR_SUBMENUS; i++)
		gtk_widget_unref (editor1_submenu_uiinfo[i].widget);
	
	for (i = 0; i < NUM_ZOOMTEXT_SUBMENUS; i++)
		gtk_widget_unref (editor1_submenu_uiinfo[i].widget);
	
	for (i = 0; i < NUM_VIEW_SUBMENUS; i++)
		gtk_widget_unref (view1_menu_uiinfo[i].widget);
	
	for (i = 0; i < NUM_IMPORTFILE_SUBMENUS; i++)
		gtk_widget_unref (project1_menu_uiinfo[i].widget);
	
	for (i = 0; i < NUM_PROJECT_SUBMENUS; i++)
		gtk_widget_unref (project1_menu_uiinfo[i].widget);
	
	for (i = 0; i < NUM_HILITE_SUBMENUS; i++)
		gtk_widget_unref (format1_menu_uiinfo[i].widget);
	
	for (i = 0; i < NUM_FORMAT_SUBMENUS; i++)
		gtk_widget_unref (format1_menu_uiinfo[i].widget);
	
	for (i = 0; i < NUM_BUILD_SUBMENUS; i++)
		gtk_widget_unref (build1_menu_uiinfo[i].widget);
	
	for (i = 0; i < NUM_BOOKMARK_SUBMENUS; i++)
		gtk_widget_unref (bookmark1_menu_uiinfo[i].widget);
	
	for (i = 0; i < NUM_EXECUTION_SUBMENUS; i++)
		gtk_widget_unref (execution1_submenu_uiinfo[i].widget);
	
	for (i = 0; i < NUM_BREAKPOINTS_SUBMENUS; i++)
		gtk_widget_unref (breakpoints1_submenu_uiinfo[i].widget);
	
	for (i = 0; i < NUM_INFO_SUBMENUS; i++)
		gtk_widget_unref (info1_submenu_uiinfo[i].widget);
	
	for (i = 0; i < NUM_DEBUG_SUBMENUS; i++)
		gtk_widget_unref (debug1_menu_uiinfo[i].widget);
		
	for (i = 0; i < NUM_CVS_SUBMENUS; i++)
		gtk_widget_unref(cvs_menu_uiinfo[i].widget);
	
	for (i = 0; i < NUM_SETTINGS_SUBMENUS; i++)
		gtk_widget_unref (settings1_menu_uiinfo[i].widget);
	
	for (i = 1; i < NUM_HELP_SUBMENUS; i++)
		gtk_widget_unref (help1_menu_uiinfo[i].widget);
}

GtkWidget *
create_submenu (gchar * title, GList * strings, GtkSignalFunc callback_func)
{
	GtkWidget *submenu;
	GtkWidget *item;
	gint i;
	submenu = scroll_menu_new ();

	/* FIXME: what about the user opinion on tearoff menuitems ?
	          perhaps (s)he has desactivated the option. */
	item = gtk_tearoff_menu_item_new ();
	gtk_menu_append (GTK_MENU (submenu), item);
	gtk_widget_show (item);

	item = gtk_menu_item_new_with_label (title);
	gtk_widget_set_sensitive (item, FALSE);
	gtk_menu_append (GTK_MENU (submenu), item);
	gtk_widget_show (item);

	item = gtk_menu_item_new ();
	gtk_menu_append (GTK_MENU (submenu), item);
	gtk_widget_show (item);

	for (i = 0; i < g_list_length (strings); i++)
	{
		gchar *text;
		text = (gchar *) (g_list_nth (strings, i)->data);
		item = gtk_menu_item_new_with_label (text);
		gtk_menu_append (GTK_MENU (submenu), item);
		gtk_widget_show (item);
		gtk_signal_connect (GTK_OBJECT (item), "activate",
				    callback_func, text);
	}
	gtk_widget_show (submenu);
	return GTK_WIDGET (submenu);
}

static GtkWidget *
create_submenu_plugin (GList * pList, GtkSignalFunc callback_func)
{
	GtkWidget *submenu;
	GtkWidget *item;
	gint i;

	submenu = scroll_menu_new ();

	/* FIXME: what about the user opinion on tearoff menuitems ?
	          perhaps (s)he has desactivated the option. */
	item = gtk_tearoff_menu_item_new ();
	gtk_menu_append (GTK_MENU (submenu), item);
	gtk_widget_show (item);

	for (i = 0; i < g_list_length (pList); i++) {
		AnjutaAddInPtr pPlug;
		gchar *szTitle;

		pPlug = (AnjutaAddInPtr) (g_list_nth (pList, i)->data);
		szTitle = (*pPlug->GetMenuTitle)( pPlug->m_Handle, pPlug->m_UserData ) ;

		item = gtk_menu_item_new_with_label ( szTitle );

		g_free( szTitle );

		gtk_menu_append (GTK_MENU (submenu), item);
		gtk_widget_show (item);
		gtk_signal_connect (GTK_OBJECT (item), "activate",
				    callback_func, pPlug );
	}

	gtk_widget_show (submenu);

	return GTK_WIDGET (submenu);
}

static void
on_file_menu_realize (GtkWidget * widget, gpointer data)
{
	GtkWidget *submenu =
		create_submenu (_("Recent Files    "), app->recent_files,
				GTK_SIGNAL_FUNC
				(on_recent_files_menu_item_activate));
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (widget), submenu);
	app->widgets.menubar.file.recent_files = widget;
}

static void
on_plugins_menu_realize (GtkWidget * widget, gpointer data)
{
	if (app->addIns_list) {
		GtkWidget *submenu =
			create_submenu_plugin (app->addIns_list,
					GTK_SIGNAL_FUNC
					(on_plugins_menu_item_activate));
		gtk_menu_item_set_submenu (GTK_MENU_ITEM (widget), submenu);
	} else
		/* Note: we only hide it to keep a proper ref counting */
		gtk_widget_hide (widget);
		
}

static void
on_plugins_menu_item_activate (GtkMenuItem * item, AnjutaAddInPtr pPlug )
{
	(*pPlug->Activate)( pPlug->m_Handle, pPlug->m_UserData, app ) ;
}

static void
on_project_menu_realize (GtkWidget * widget, gpointer data)
{
	GtkWidget *submenu =
		create_submenu (_("Recent Projects "), app->recent_projects,
				GTK_SIGNAL_FUNC
				(on_recent_projects_menu_item_activate));
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (widget), submenu);
	app->widgets.menubar.file.recent_projects = widget;
}

void
on_recent_files_menu_item_activate (GtkMenuItem * item, gchar * data)
{
	anjuta_append_text_editor (data);
}

void
on_recent_projects_menu_item_activate (GtkMenuItem * item, gchar * data)
{
	anjuta_load_this_project(data);
}
