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
#include "pixmaps.h"
#include "mainmenu_callbacks.h"
#include "main_menubar_def.h"
#include "fileselection.h"
#include "widget-registry.h"

static void on_file_menu_realize (GtkWidget * widget, gpointer data);
static void on_project_menu_realize (GtkWidget * widget, gpointer data);

void
create_main_menubar (GtkWidget * ap, MainMenuBar * mb)
{
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
	mb->file.print_preview = file1_menu_uiinfo[19].widget;
	mb->file.recent_files = file1_menu_uiinfo[21].widget;
	mb->file.recent_projects = file1_menu_uiinfo[22].widget;
	mb->file.exit = file1_menu_uiinfo[24].widget;

	/* Edit submenu */
	mb->edit.uppercase = transform1_submenu_uiinfo[0].widget;
	mb->edit.lowercase = transform1_submenu_uiinfo[1].widget;
	mb->edit.convert_crlf = transform1_submenu_uiinfo[3].widget;
	mb->edit.convert_lf = transform1_submenu_uiinfo[4].widget;
	mb->edit.convert_cr = transform1_submenu_uiinfo[5].widget;
	mb->edit.convert_auto = transform1_submenu_uiinfo[6].widget;
	
	mb->edit.insert_header = insert_submenu_uiinfo[0].widget;
	mb->edit.insert_c_switch = insert_template_c_uiinfo[0].widget;
	mb->edit.insert_c_for = insert_template_c_uiinfo[1].widget;
	mb->edit.insert_c_while = insert_template_c_uiinfo[2].widget;
	mb->edit.insert_c_ifelse = insert_template_c_uiinfo[3].widget;
	mb->edit.insert_cvs_author = insert_cvskeyword_submenu_uiinfo[0].widget;
	mb->edit.insert_cvs_date = insert_cvskeyword_submenu_uiinfo[1].widget;
	mb->edit.insert_cvs_header = insert_cvskeyword_submenu_uiinfo[2].widget;
	mb->edit.insert_cvs_id = insert_cvskeyword_submenu_uiinfo[3].widget;
	mb->edit.insert_cvs_log = insert_cvskeyword_submenu_uiinfo[4].widget;
	mb->edit.insert_cvs_name = insert_cvskeyword_submenu_uiinfo[5].widget;
	mb->edit.insert_cvs_revision = insert_cvskeyword_submenu_uiinfo[6].widget;
	mb->edit.insert_cvs_source = insert_cvskeyword_submenu_uiinfo[7].widget;
	mb->edit.insert_c_gpl = inserttext1_submenu_uiinfo[0].widget;
	mb->edit.insert_cpp_gpl = inserttext1_submenu_uiinfo[1].widget;
	mb->edit.insert_py_gpl = inserttext1_submenu_uiinfo[2].widget;
	mb->edit.insert_username = inserttext1_submenu_uiinfo[3].widget;
	mb->edit.insert_datetime = inserttext1_submenu_uiinfo[4].widget;
	mb->edit.insert_header_template = inserttext1_submenu_uiinfo[5].widget;
	mb->edit.insert_changelog_entry = inserttext1_submenu_uiinfo[6].widget;
	
	mb->edit.select_all = select1_submenu_uiinfo[0].widget;
	mb->edit.select_brace = select1_submenu_uiinfo[1].widget;
	mb->edit.select_block = select1_submenu_uiinfo[2].widget;
	
	mb->edit.comment_block = comment_submenu_uiinfo[0].widget;
	mb->edit.comment_box = comment_submenu_uiinfo[1].widget;
	mb->edit.comment_stream = comment_submenu_uiinfo[1].widget;
	
	mb->edit.find = find_submenu_uiinfo[0].widget;
	mb->edit.find_next = find_submenu_uiinfo[1].widget;
	mb->edit.find_replace = find_submenu_uiinfo[2].widget;
	mb->edit.find_in_files = find_submenu_uiinfo[3].widget;
	mb->edit.enter_selection = find_submenu_uiinfo[4].widget;
	
	mb->edit.goto_line = goto1_submenu_uiinfo[0].widget;
	mb->edit.goto_brace = goto1_submenu_uiinfo[1].widget;
	mb->edit.goto_block_start = goto1_submenu_uiinfo[2].widget;
	mb->edit.goto_block_end = goto1_submenu_uiinfo[3].widget;
	mb->edit.goto_prev_mesg = goto1_submenu_uiinfo[4].widget;
	mb->edit.goto_next_mesg = goto1_submenu_uiinfo[5].widget;
	mb->edit.go_back = goto1_submenu_uiinfo[6].widget;
	mb->edit.go_forward = goto1_submenu_uiinfo[7].widget;
	mb->edit.goto_tag_def = goto1_submenu_uiinfo[8].widget;
	mb->edit.goto_tag_decl = goto1_submenu_uiinfo[9].widget;
	
	mb->edit.undo = edit1_menu_uiinfo[0].widget;
	mb->edit.redo = edit1_menu_uiinfo[1].widget;
	mb->edit.cut = edit1_menu_uiinfo[3].widget;
	mb->edit.copy = edit1_menu_uiinfo[4].widget;
	mb->edit.paste = edit1_menu_uiinfo[5].widget;
	mb->edit.clear = edit1_menu_uiinfo[6].widget;
	mb->edit.autocomplete = edit1_menu_uiinfo[15].widget;
	mb->edit.calltip = edit1_menu_uiinfo[16].widget;
	
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
	mb->view.editor_linewrap = editor1_submenu_uiinfo[6].widget;
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
	mb->project.add_file = project1_menu_uiinfo[0].widget;
	mb->project.view_file = project1_menu_uiinfo[1].widget;
	mb->project.edit_file = project1_menu_uiinfo[2].widget;
	mb->project.remove_file = project1_menu_uiinfo[3].widget;
	mb->project.configure = project1_menu_uiinfo[5].widget;
	mb->project.project_info = project1_menu_uiinfo[6].widget;
	mb->project.dock_undock = project1_menu_uiinfo[8].widget;
	mb->project.update_tags = project1_menu_uiinfo[10].widget;
	mb->project.rebuild_tags = project1_menu_uiinfo[11].widget;
	mb->project.edit_app_gui = project1_menu_uiinfo[12].widget;
	mb->project.project_help = project1_menu_uiinfo[14].widget;

	/* Format submenu */
	mb->format.indent = format1_menu_uiinfo[0].widget;
	mb->format.indent_inc = format1_menu_uiinfo[1].widget;
	mb->format.indent_dcr = format1_menu_uiinfo[2].widget;
	mb->format.force_hilite = format1_menu_uiinfo[4].widget;
	mb->format.close_folds = format1_menu_uiinfo[6].widget;
	mb->format.open_folds = format1_menu_uiinfo[7].widget;
	mb->format.toggle_fold = format1_menu_uiinfo[8].widget;
	mb->format.detach = format1_menu_uiinfo[10].widget;

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
	mb->debug.info_memory = info1_submenu_uiinfo[8].widget;

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

	/* CVS submenu */
	mb->cvs.update_file = cvs_menu_uiinfo[0].widget;
	mb->cvs.commit_file = cvs_menu_uiinfo[1].widget;
	mb->cvs.status_file = cvs_menu_uiinfo[2].widget;
	mb->cvs.log_file = cvs_menu_uiinfo[3].widget;
	mb->cvs.add_file = cvs_menu_uiinfo[4].widget;
	mb->cvs.remove_file = cvs_menu_uiinfo[5].widget;
	mb->cvs.diff_file = cvs_menu_uiinfo[6].widget;
	mb->cvs.update_project = cvs_menu_uiinfo[8].widget;
	mb->cvs.commit_project = cvs_menu_uiinfo[9].widget;
	mb->cvs.import_project = cvs_menu_uiinfo[10].widget;
	mb->cvs.status_project = cvs_menu_uiinfo[11].widget;
	mb->cvs.log_project = cvs_menu_uiinfo[12].widget;
	mb->cvs.diff_project = cvs_menu_uiinfo[13].widget;
	mb->cvs.login = cvs_menu_uiinfo[15].widget;

	/* Settings submenu */
	mb->settings.compiler = settings1_menu_uiinfo[0].widget;
	mb->settings.src_paths = settings1_menu_uiinfo[1].widget;
	mb->settings.commands = settings1_menu_uiinfo[2].widget;
	mb->settings.preferences = settings1_menu_uiinfo[4].widget;
	mb->settings.style_editor = settings1_menu_uiinfo[5].widget;
	mb->settings.user_properties = settings1_menu_uiinfo[6].widget;
	mb->settings.default_preferences = settings1_menu_uiinfo[7].widget;
	mb->settings.shortcuts = settings1_menu_uiinfo[8].widget;

	/* Help submenu */
	/* Read the comment in main_menubar.h.
	mb->help.gnome = help1_menu_uiinfo[2].widget;
	mb->help.man = help1_menu_uiinfo[3].widget;
	mb->help.info = help1_menu_uiinfo[4].widget;
	mb->help.context_help = help1_menu_uiinfo[6].widget;
	mb->help.search = help1_menu_uiinfo[7].widget;
	mb->help.about = help1_menu_uiinfo[17].widget;
	*/
	/* Unimplemented */
	gtk_widget_hide (file1_menu_uiinfo[15].widget);
	gtk_widget_hide (file1_menu_uiinfo[16].widget);
	gtk_widget_hide (edit1_menu_uiinfo[16].widget);
	gtk_widget_hide (view1_menu_uiinfo[2].widget);
	gtk_widget_hide (view1_menu_uiinfo[14].widget);
	gtk_widget_hide (view1_menu_uiinfo[16].widget);
	gtk_widget_hide (project1_menu_uiinfo[13].widget);
	gtk_widget_hide (project1_menu_uiinfo[14].widget);

	/* Recent files and project submenu */
	gtk_signal_connect (GTK_OBJECT (mb->file.recent_files), "realize",
			    GTK_SIGNAL_FUNC (on_file_menu_realize), NULL);
	gtk_signal_connect (GTK_OBJECT (mb->file.recent_projects), "realize",
			    GTK_SIGNAL_FUNC (on_project_menu_realize), NULL);

	/* Populate the menu map */
	an_register_submenu("file", GTK_MENU_ITEM(menubar1_uiinfo[0].widget)->submenu);
	an_register_submenu(NULL /*"edit"*/,  GTK_MENU_ITEM(menubar1_uiinfo[1].widget)->submenu);
	an_register_submenu("view", GTK_MENU_ITEM(menubar1_uiinfo[2].widget)->submenu);
	an_register_submenu("project", GTK_MENU_ITEM(menubar1_uiinfo[3].widget)->submenu);
	an_register_submenu("format", GTK_MENU_ITEM(menubar1_uiinfo[4].widget)->submenu);
	an_register_submenu("build", GTK_MENU_ITEM(menubar1_uiinfo[5].widget)->submenu);
	an_register_submenu("bookmark", GTK_MENU_ITEM(menubar1_uiinfo[6].widget)->submenu);
	an_register_submenu("debug", GTK_MENU_ITEM(menubar1_uiinfo[7].widget)->submenu);
	an_register_submenu("cvs", GTK_MENU_ITEM(menubar1_uiinfo[8].widget)->submenu);
	an_register_submenu("settings", GTK_MENU_ITEM(menubar1_uiinfo[9].widget)->submenu);
}

void
main_menu_install_hints (GtkWidget* ap)
{
	gint i, j;
	gint anjuta_menus_size = sizeof(anjuta_menus_uiinfo)/sizeof(GnomeUIInfo*);
	for (i = 0; i < anjuta_menus_size; i++) {
		int submenu_size = sizeof(*anjuta_menus_uiinfo[i])/sizeof(GnomeUIInfo);
		for (j = 0; j < submenu_size; j++) {
			gtk_widget_ref (anjuta_menus_uiinfo[i][j].widget);
			//gtk_accel_group_attach(GNOME_APP(ap)->accel_group,
			//	GTK_OBJECT(anjuta_menus_uiinfo[i][j].widget));
		}
	}
	/* Help menu is separate because we don't want to reference
	 * the first element
	 */
	for (i = 1; i < NUM_HELP_SUBMENUS; i++) {
		gtk_widget_ref (help1_menu_uiinfo[i].widget);
		//gtk_accel_group_attach(GNOME_APP(ap)->accel_group,
		//	GTK_OBJECT(help1_menu_uiinfo[i].widget));
	}
	
	gnome_app_install_appbar_menu_hints (GNOME_APPBAR
					     (app->widgets.appbar),
					     menubar1_uiinfo);
}

void
main_menu_unref ()
{
	gint i, j;
	gint anjuta_menus_size = sizeof(anjuta_menus_uiinfo)/sizeof(GnomeUIInfo*);
	for (i = 0; i < anjuta_menus_size; i++) {
		int submenu_size = sizeof(*anjuta_menus_uiinfo[i])/sizeof(GnomeUIInfo);
		for (j = 0; j < submenu_size; j++)
			gtk_widget_unref (anjuta_menus_uiinfo[i][j].widget);
	}
	/* Help menu is separate because we don't want to reference
	 * the first element
	 */
	for (i = 1; i < NUM_HELP_SUBMENUS; i++)
		gtk_widget_unref (help1_menu_uiinfo[i].widget);
}

GtkWidget *
create_submenu (gchar * title, GList * strings, GtkSignalFunc callback_func)
{
	GtkWidget *submenu;
	GtkWidget *item;
	gint i;
	submenu = gtk_menu_new ();

	/* FIXME: what about the user opinion on tearoff menuitems ?
	          perhaps (s)he has deactivated the option. */
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
