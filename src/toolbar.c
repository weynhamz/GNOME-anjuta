/*
    toolbar.c
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

#include "toolbar_callbacks.h"

#include "resources.h"
#include "toolbar.h"
#include "anjuta.h"
#include "pixmaps.h"

GtkWidget *
create_main_toolbar (GtkWidget * anjuta_gui, MainToolbar * toolbar)
{
	GtkWidget *toolbar1;
	GtkWidget *toolbar_led;
	GtkTooltips *tooltips;
	GtkWidget *tmp_toolbar_icon;
	gchar *filename;
	GdkPixbufAnimation *led_anim;
	GError *gerror = NULL;

	tooltips = gtk_tooltips_new ();

	toolbar1 = gtk_toolbar_new ();
	gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar1),
	                             GTK_ORIENTATION_HORIZONTAL);
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar1), GTK_TOOLBAR_ICONS);
	gtk_widget_ref (toolbar1);
	gtk_widget_show (toolbar1);

#warning "G2: Add LED animation image file path here"
	filename = anjuta_res_get_pixmap_file (ANJUTA_PIXMAP_GREEN_LED);
	led_anim = gdk_pixbuf_animation_new_from_file (filename, &gerror);
	toolbar_led = gtk_image_new ();
	gtk_image_set_from_animation (GTK_IMAGE (toolbar_led), led_anim);
	if (gerror)
		g_error_free (gerror);
	if (filename)
		g_free (filename);

	gtk_widget_ref (toolbar_led);
	gtk_widget_show (toolbar_led);
	// FIXME: Led animation.
	// gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar1), toolbar_led,
	//			   NULL, NULL);
	//gtk_toolbar_append_space (GTK_TOOLBAR (toolbar1));
	
	toolbar->novus = 
		anjuta_util_toolbar_append_stock (toolbar1, GTK_STOCK_NEW, _("New"),
							  _("New file"),
							  G_CALLBACK (on_toolbar_new_clicked), NULL);
	toolbar->open = 
		anjuta_util_toolbar_append_stock (toolbar1, GTK_STOCK_OPEN, _("Open"),
							  _("Open file"),
							  G_CALLBACK (on_toolbar_open_clicked), NULL);
	toolbar->save = 
		anjuta_util_toolbar_append_stock (toolbar1, GTK_STOCK_SAVE, _("Save"),
							  _("Save current file"),
							  G_CALLBACK (on_toolbar_save_clicked), NULL);
	toolbar->reload =
		anjuta_util_toolbar_append_stock (toolbar1, GTK_STOCK_REVERT_TO_SAVED, _("Reload"),
						   _("Reload current file"),
						   G_CALLBACK (on_toolbar_reload_clicked), NULL);
	toolbar->close =
		anjuta_util_toolbar_append_stock (toolbar1, GTK_STOCK_CLOSE, _("Close"),
						   _("Close current file"),
						   G_CALLBACK (on_toolbar_close_clicked), NULL);
	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar1));
	toolbar->undo =
		anjuta_util_toolbar_append_stock (toolbar1, GTK_STOCK_UNDO, _("Undo"),
						   _("Undo the last action"),
						   G_CALLBACK (on_toolbar_undo_clicked), NULL);
	toolbar->redo =
		anjuta_util_toolbar_append_stock (toolbar1, GTK_STOCK_REDO, _("Redo"),
						   _("Redo the last undone action"),
						   G_CALLBACK (on_toolbar_redo_clicked), NULL);
	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar1));
	toolbar->detach =
		anjuta_util_toolbar_append_button (toolbar1, ANJUTA_PIXMAP_UNDOCK, _("Detach"),
						   _("Detach the current page"),
						   G_CALLBACK (on_toolbar_detach_clicked), NULL);
	toolbar->print =
		anjuta_util_toolbar_append_stock (toolbar1, GTK_STOCK_PRINT, _("Print"),
						   _("Print the current file"),
						   G_CALLBACK (on_toolbar_print_clicked), NULL);
	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar1));
	toolbar->find =
		anjuta_util_toolbar_append_stock (toolbar1, GTK_STOCK_FIND, _("Find"),
						   _("Search for the given string in the current file"),
						   G_CALLBACK (on_toolbar_find_clicked), NULL);

	toolbar->find_combo = gtk_combo_new ();
	gtk_widget_ref (toolbar->find_combo);
	gtk_combo_disable_activate (GTK_COMBO (toolbar->find_combo));
	gtk_combo_set_case_sensitive (GTK_COMBO (toolbar->find_combo), TRUE);
	gtk_widget_show (toolbar->find_combo);
	gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar1), toolbar->find_combo,
				   NULL, NULL);

	toolbar->find_entry = GTK_COMBO (toolbar->find_combo)->entry;
	gtk_widget_ref (toolbar->find_entry);
	gtk_widget_show (toolbar->find_entry);
	gtk_tooltips_set_tip (tooltips, toolbar->find_entry,
			      _("Enter the string to search for"),
			      NULL);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar1));
	toolbar->go_to =
		anjuta_util_toolbar_append_stock (toolbar1, GTK_STOCK_JUMP_TO, _("Go To"),
						   _("Go to the given line number in the current file"),
						   G_CALLBACK (on_toolbar_goto_clicked), NULL);

	toolbar->line_entry = gtk_entry_new ();
	gtk_widget_ref (toolbar->line_entry);
	gtk_widget_show (toolbar->line_entry);
	gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar1), toolbar->line_entry,
				   _("Enter the line number to go to"),
				   NULL);
	gtk_widget_set_usize (toolbar->line_entry, 53, -2);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar1));
	toolbar->project =
		anjuta_util_toolbar_append_button (toolbar1, ANJUTA_PIXMAP_PROJECT, _("Project"),
						   _("Show/Hide the Project window"),
						   G_CALLBACK (on_toolbar_project_clicked), NULL);
	toolbar->messages =
		anjuta_util_toolbar_append_button (toolbar1, ANJUTA_PIXMAP_MESSAGES, _("Messages"),
						   _("Show/Hide the Message window"),
						   G_CALLBACK (on_toolbar_messages_clicked), NULL);
	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar1));
	toolbar->help =
		anjuta_util_toolbar_append_stock (toolbar1, GTK_STOCK_HELP,
						   _("Help"),
						   _("Context sensitive help"),
						   G_CALLBACK (on_toolbar_help_clicked), NULL);
	
	gtk_signal_connect (GTK_OBJECT (toolbar->find_entry),
			    "activate",
			    GTK_SIGNAL_FUNC (on_toolbar_find_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar->find_entry),
			    "changed",
			    GTK_SIGNAL_FUNC (on_toolbar_find_incremental), NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar->find_entry),
			    "focus_in_event",
			    GTK_SIGNAL_FUNC (on_toolbar_find_incremental_start), NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar->find_entry),
			    "focus_out_event",
			    GTK_SIGNAL_FUNC (on_toolbar_find_incremental_end), NULL);

	gtk_signal_connect (GTK_OBJECT (toolbar->line_entry), "activate",
			    GTK_SIGNAL_FUNC (on_toolbar_goto_clicked), NULL);

	toolbar->toolbar = toolbar1;
	return toolbar1;
}

GtkWidget *
create_extended_toolbar (GtkWidget * anjuta_gui, ExtendedToolbar * toolbar)
{
	GtkWidget *toolbar2;

	GtkWidget *toolbar_open_project;
	GtkWidget *toolbar_save_project;
	GtkWidget *toolbar_close_project;

	GtkWidget *toolbar_compile;
	GtkWidget *toolbar_configure;
	GtkWidget *toolbar_build;
	GtkWidget *toolbar_build_all;
	GtkWidget *toolbar_exec;
	GtkWidget *toolbar_debug;
	GtkWidget *toolbar_stop;
	GtkWidget *tmp_toolbar_icon;

	toolbar2 = gtk_toolbar_new ();
	gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar2),
	                             GTK_ORIENTATION_HORIZONTAL);
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar2), GTK_TOOLBAR_ICONS);
	gtk_widget_ref (toolbar2);
	gtk_widget_show (toolbar2);

	tmp_toolbar_icon =
		anjuta_res_get_image (ANJUTA_PIXMAP_OPEN_PROJECT);
	toolbar_open_project =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Open Project"),
					    _("Open a Project"), NULL,
					    tmp_toolbar_icon, NULL, NULL);
	gtk_widget_ref (toolbar_open_project);
	gtk_widget_show (toolbar_open_project);

	tmp_toolbar_icon =
		anjuta_res_get_image (ANJUTA_PIXMAP_SAVE_PROJECT);
	toolbar_save_project =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Save Project"),
					    _("Save the current Project"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_save_project);
	gtk_widget_show (toolbar_save_project);

	tmp_toolbar_icon =
		anjuta_res_get_image (ANJUTA_PIXMAP_CLOSE_PROJECT);
	toolbar_close_project =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Close Project"),
					    _("Close the current Project"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_close_project);
	gtk_widget_show (toolbar_close_project);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar2));

	tmp_toolbar_icon =
		anjuta_res_get_image (ANJUTA_PIXMAP_COMPILE);
	toolbar_compile =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Compile"),
					    _("Compile the current file"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_compile);
	gtk_widget_show (toolbar_compile);

	tmp_toolbar_icon =
		anjuta_res_get_image (ANJUTA_PIXMAP_CONFIGURE);
	toolbar_configure =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Configure"),
					    _("Run configure"), NULL,
					    tmp_toolbar_icon, NULL, NULL);
	gtk_widget_ref (toolbar_configure);
	gtk_widget_show (toolbar_configure);

	tmp_toolbar_icon =
		anjuta_res_get_image (ANJUTA_PIXMAP_BUILD);
	toolbar_build =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Build"),
					    _
					    ("Build current file, or build the source directory of the Project"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_build);
	gtk_widget_show (toolbar_build);

	tmp_toolbar_icon =
		anjuta_res_get_image (ANJUTA_PIXMAP_BUILD_ALL);
	toolbar_build_all =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Build_all"),
					    _
					    ("Build from the top directory of the Project"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_build_all);
	gtk_widget_show (toolbar_build_all);

	toolbar_exec =
		anjuta_util_toolbar_append_stock (toolbar2, GTK_STOCK_EXECUTE,
										  _("Execute"),
										  _("Execute the program"),
										  NULL, NULL);
	gtk_widget_ref (toolbar_exec);
	gtk_widget_show (toolbar_exec);

	tmp_toolbar_icon =
		anjuta_res_get_image (ANJUTA_PIXMAP_DEBUG);
	toolbar_debug =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Debug"),
					    _("Start the Debugger"), NULL,
					    tmp_toolbar_icon, NULL, NULL);
	gtk_widget_ref (toolbar_debug);
	gtk_widget_show (toolbar_debug);

	toolbar_stop =
		anjuta_util_toolbar_append_stock (toolbar2, GTK_STOCK_STOP,
										  _("Stop"),
										  _("Stop/interrupt compile or build"),
										  NULL, NULL);
	gtk_widget_ref (toolbar_stop);
	gtk_widget_show (toolbar_stop);

	gtk_signal_connect (GTK_OBJECT (toolbar_open_project), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_open_project_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_save_project), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_save_project_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_close_project), "clicked",
			    GTK_SIGNAL_FUNC
			    (on_toolbar_close_project_clicked), NULL);

	gtk_signal_connect (GTK_OBJECT (toolbar_compile), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_compile_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_configure), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_configure_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_build), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_build_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_build_all), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_build_all_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_exec), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_exec_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_debug), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_debug_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_stop), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_stop_clicked), NULL);

	toolbar->toolbar = toolbar2;
	toolbar->open_project = toolbar_open_project;
	toolbar->save_project = toolbar_save_project;
	toolbar->close_project = toolbar_close_project;

	toolbar->compile = toolbar_compile;
	toolbar->configure = toolbar_configure;
	toolbar->build = toolbar_build;
	toolbar->build_all = toolbar_build_all;
	toolbar->exec = toolbar_exec;
	toolbar->debug = toolbar_debug;
	toolbar->stop = toolbar_stop;
	return toolbar2;
}

GtkWidget *
create_browser_toolbar (GtkWidget * anjuta_gui, BrowserToolbar * toolbar)
{
	GtkWidget *window1;
	GtkWidget *toolbar2;
	GtkTooltips *tooltips;

	tooltips = gtk_tooltips_new ();

	window1 = anjuta_gui;

	toolbar2 = gtk_toolbar_new ();
	gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar2),
	                             GTK_ORIENTATION_HORIZONTAL);
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar2), GTK_TOOLBAR_ICONS);
	gtk_widget_ref (toolbar2);
	gtk_widget_show (toolbar2);

	toolbar->toggle_bookmark =
		anjuta_util_toolbar_append_button (toolbar2,
						   ANJUTA_PIXMAP_BOOKMARK_TOGGLE,
						   _("Toggle"),
					       _("Toggle bookmark at current location"),
						   G_CALLBACK (on_browser_toggle_bookmark_clicked),
						   NULL);

	toolbar->first_bookmark =
		anjuta_util_toolbar_append_stock (toolbar2, GTK_STOCK_GOTO_FIRST,
						   _("First"),
						   _("Goto first bookmark in this document"),
						   G_CALLBACK (on_browser_first_bookmark_clicked),
						   NULL);
	
	toolbar->prev_bookmark =
		anjuta_util_toolbar_append_stock (toolbar2, GTK_STOCK_GO_BACK,
						   _("Prev"),
						   _("Goto previous bookmark in this document"),
						   G_CALLBACK (on_browser_prev_bookmark_clicked),
						   NULL);
	toolbar->next_bookmark =
		anjuta_util_toolbar_append_stock (toolbar2, GTK_STOCK_GO_FORWARD,
						   _("Next"),
						   _("Goto next bookmark in this document"),
						   G_CALLBACK (on_browser_next_bookmark_clicked),
						   NULL);
	toolbar->last_bookmark =
		anjuta_util_toolbar_append_stock (toolbar2, GTK_STOCK_GOTO_LAST,
						   _("Last"),
						   _("Goto last bookmark in this document"),
						   G_CALLBACK (on_browser_last_bookmark_clicked),
						   NULL);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar2));

	toolbar->prev_error =
		anjuta_util_toolbar_append_button (toolbar2,
						   ANJUTA_PIXMAP_ERROR_PREV,
						   _("Prev"),
						   _("Goto previous message in this document"),
						   G_CALLBACK (on_browser_prev_mesg_clicked),
						   NULL);
	toolbar->next_error =
		anjuta_util_toolbar_append_button (toolbar2,
						   ANJUTA_PIXMAP_ERROR_NEXT,
						   _("Next"),
						   _("Goto next message in this document"),
						   G_CALLBACK (on_browser_next_mesg_clicked),
						   NULL);
	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar2));

	toolbar->block_start =
		anjuta_util_toolbar_append_button (toolbar2,
						   ANJUTA_PIXMAP_BLOCK_START,
						   _("Start"),
						   _("Goto start of current code block"),
						   G_CALLBACK (on_browser_block_start_clicked),
						   NULL);
	toolbar->block_end =
		anjuta_util_toolbar_append_button (toolbar2,
						   ANJUTA_PIXMAP_BLOCK_END,
						   _("End"),
						   _("Goto end of current code block"),
						   G_CALLBACK (on_browser_block_end_clicked),
						   NULL);
	
	/* Tag combo entry - added by Biswa */
	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar2));
	
	toolbar->tag_label = gtk_label_new(_("Tags:"));
	// gtk_misc_set_padding (GTK_MISC(toolbar->tag_label), 5, 5);
	gtk_widget_show (toolbar->tag_label);
	gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar2), toolbar->tag_label,
				   NULL, NULL);
	
	toolbar->tag =
		anjuta_util_toolbar_append_stock (toolbar2,
						   GTK_STOCK_JUMP_TO,
						   _("Goto Tag"),
						   _("Search for the given tag in the current file"),
						   G_CALLBACK (on_toolbar_tag_clicked), NULL);

	toolbar->tag_combo = gtk_combo_new ();
	gtk_combo_set_case_sensitive (GTK_COMBO (toolbar->tag_combo), TRUE);
	gtk_widget_set_usize(toolbar->tag_combo, 256, -1);
	gtk_widget_show (toolbar->tag_combo);
	gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar2), toolbar->tag_combo,
							   NULL, NULL);

	toolbar->tag_entry = GTK_COMBO (toolbar->tag_combo)->entry;
	gtk_editable_set_editable(GTK_EDITABLE(toolbar->tag_entry), FALSE);
	gtk_widget_ref (toolbar->tag_entry);
	gtk_widget_show (toolbar->tag_entry);
	gtk_tooltips_set_tip (tooltips, toolbar->tag_entry,
						  _("Enter the tag to jump to"),
						  NULL);

	/* Goto Tag signal handlers */
	g_signal_connect (G_OBJECT (GTK_COMBO(toolbar->tag_combo)->entry),
					  "changed", G_CALLBACK (on_toolbar_tag_clicked), NULL);

	toolbar->toolbar = toolbar2;
	return toolbar2;
}

GtkWidget *
create_debug_toolbar (GtkWidget * anjuta_gui, DebugToolbar * toolbar)
{
	GtkWidget *toolbar3;
	GtkWidget *toolbar_go;
	GtkWidget *toolbar_run_to_cursor;
	GtkWidget *toolbar_step_in;
	GtkWidget *toolbar_step_out;
	GtkWidget *toolbar_step_over;
	GtkWidget *toolbar_toggle_bp;
	GtkWidget *toolbar_interrupt;
	GtkWidget *toolbar_frame;
	/* GtkWidget *toolbar_watch; */
	GtkWidget *toolbar_inspect;
	GtkWidget *toolbar_stack;
	GtkWidget *toolbar_registers;
	GtkWidget *toolbar_stop;
	GtkWidget *tmp_toolbar_icon;

	toolbar3 = gtk_toolbar_new ();
	gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar3),
	                             GTK_ORIENTATION_HORIZONTAL);
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar3), GTK_TOOLBAR_ICONS);
	gtk_widget_ref (toolbar3);
	gtk_widget_show (toolbar3);

	tmp_toolbar_icon =
		anjuta_res_get_image (ANJUTA_PIXMAP_CONTINUE);
	toolbar_go =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar3),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Go"),
					    _("Go or continue execution"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_go);
	gtk_widget_show (toolbar_go);

	tmp_toolbar_icon =
		anjuta_res_get_image (ANJUTA_PIXMAP_RUN_TO_CURSOR);
	toolbar_run_to_cursor =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar3),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Run"),
					    _("Run to cursor"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_run_to_cursor);
	gtk_widget_show (toolbar_run_to_cursor);

	tmp_toolbar_icon =
		anjuta_res_get_image (ANJUTA_PIXMAP_STEP_IN);
	toolbar_step_in =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar3),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Step in"),
					    _("Single step in execution"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_step_in);
	gtk_widget_show (toolbar_step_in);

	tmp_toolbar_icon =
		anjuta_res_get_image (ANJUTA_PIXMAP_STEP_OVER);
	toolbar_step_over =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar3),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Step Over"),
					    _("Step over the function"), NULL,
					    tmp_toolbar_icon, NULL, NULL);
	gtk_widget_ref (toolbar_step_over);
	gtk_widget_show (toolbar_step_over);

	tmp_toolbar_icon =
		anjuta_res_get_image (ANJUTA_PIXMAP_STEP_OUT);
	toolbar_step_out =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar3),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Step Out"),
					    _("Step out of the function"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_step_out);
	gtk_widget_show (toolbar_step_out);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar3));

	tmp_toolbar_icon =
		anjuta_res_get_image (ANJUTA_PIXMAP_INTERRUPT);
	toolbar_interrupt =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar3),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Interrupt"),
					    _
					    ("Interrupt execution of the program"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_interrupt);
	gtk_widget_show (toolbar_interrupt);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar3));

	tmp_toolbar_icon =
		anjuta_res_get_image (ANJUTA_PIXMAP_BREAKPOINT);
	toolbar_toggle_bp =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar3),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Breakpoint"),
					    _("Toggle breakpoint at cursor"), NULL,
					    tmp_toolbar_icon, NULL, NULL);
	gtk_widget_ref (toolbar_toggle_bp);
	gtk_widget_show (toolbar_toggle_bp);

	tmp_toolbar_icon =
		anjuta_res_get_image (ANJUTA_PIXMAP_INSPECT);
	toolbar_inspect =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar3),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Inspect"),
					    _
					    ("Inspect or evaluate an expression"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_inspect);
	gtk_widget_show (toolbar_inspect);

	tmp_toolbar_icon =
		anjuta_res_get_image (ANJUTA_PIXMAP_FRAME);
	toolbar_frame =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar3),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Frame"),
					    _
					    ("Display the current frame information"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_frame);
	gtk_widget_show (toolbar_frame);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar3));
	/*
	tmp_toolbar_icon =
		anjuta_res_get_image (ANJUTA_PIXMAP_WATCH);
	toolbar_watch =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar3),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Watch"),
					    _
					    ("Watch expressions during execution"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_watch);
	gtk_widget_show (toolbar_watch);
	*/
	tmp_toolbar_icon =
		anjuta_res_get_image (ANJUTA_PIXMAP_STACK);
	toolbar_stack =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar3),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Stack"),
					    _("Stack trace of the program"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_stack);
	gtk_widget_show (toolbar_stack);

	tmp_toolbar_icon =
		anjuta_res_get_image (ANJUTA_PIXMAP_REGISTERS);
	toolbar_registers =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar3),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Registers"),
					    _
					    ("CPU registers and their contents"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_registers);
	gtk_widget_show (toolbar_registers);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar3));

	tmp_toolbar_icon =
		anjuta_res_get_image (ANJUTA_PIXMAP_DEBUG_STOP);
	toolbar_stop =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar3),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Stop"),
					    _("End the debugging session"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_stop);
	gtk_widget_show (toolbar_stop);

	gtk_signal_connect (GTK_OBJECT (toolbar_run_to_cursor), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_run_to_cursor_clicked), NULL);

	gtk_signal_connect (GTK_OBJECT (toolbar_step_in), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_step_in_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_step_out), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_step_out_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_step_over), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_step_over_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_toggle_bp), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_toggle_bp_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_go), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_go_clicked), NULL);

	/*
	gtk_signal_connect (GTK_OBJECT (toolbar_watch), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_watch_clicked), NULL);
	*/
	gtk_signal_connect (GTK_OBJECT (toolbar_stack), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_stack_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_frame), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_frame_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_registers), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_registers_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_inspect), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_inspect_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_interrupt), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_interrupt_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_stop), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_debug_stop_clicked), NULL);

	toolbar->toolbar = toolbar3;
	toolbar->go = toolbar_go;
	toolbar->run_to_cursor = toolbar_run_to_cursor;
	toolbar->step_in = toolbar_step_in;
	toolbar->step_out = toolbar_step_out;
	toolbar->step_over = toolbar_step_over;
	toolbar->toggle_bp = toolbar_toggle_bp;
	/* toolbar->watch = toolbar_watch; */
	toolbar->frame = toolbar_frame;
	toolbar->interrupt = toolbar_interrupt;
	toolbar->stack = toolbar_stack;
	toolbar->inspect = toolbar_inspect;
	toolbar->stop = toolbar_stop;
	return toolbar3;
}



GtkWidget *
create_format_toolbar (GtkWidget * anjuta_gui, FormatToolbar * toolbar)
{
	GtkWidget *window1;
	GtkWidget *toolbar2;

	GtkWidget *button2;
	GtkWidget *button3;
	GtkWidget *button4;
	GtkWidget *button5;
	GtkWidget *button6;
	GtkWidget *button7;
	GtkWidget *button8;
	GtkWidget *button9;
	GtkWidget *button10;
	GtkWidget *button11;
	GtkWidget *tmp_toolbar_icon;

	window1 = anjuta_gui;

	toolbar2 = gtk_toolbar_new ();
	gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar2),
	                             GTK_ORIENTATION_HORIZONTAL);
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar2), GTK_TOOLBAR_ICONS);
	gtk_widget_ref (toolbar2);
	gtk_widget_show (toolbar2);
	//gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar2),
	//			     GTK_TOOLBAR_SPACE_LINE);
	//gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar2),
	//			       GTK_RELIEF_NONE);

	tmp_toolbar_icon =
		anjuta_res_get_image (ANJUTA_PIXMAP_FOLD_TOGGLE);
	button2 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Toggle"),
					    _
					    ("Toggle current code fold hide/show"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (button2);
	gtk_widget_show (button2);

	tmp_toolbar_icon =
		anjuta_res_get_image (ANJUTA_PIXMAP_FOLD_CLOSE);
	button4 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Close all"),
					    _
					    ("Close all code folds in this document"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (button4);
	gtk_widget_show (button4);

	tmp_toolbar_icon =
		anjuta_res_get_image (ANJUTA_PIXMAP_FOLD_OPEN);
	button3 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Open all"),
					    _
					    ("Open all code folds in this document"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (button3);
	gtk_widget_show (button3);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar2));

	tmp_toolbar_icon =
		anjuta_res_get_image (ANJUTA_PIXMAP_BLOCK_SELECT);
	button5 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Select"),
					    _("Select current code block"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (button5);
	gtk_widget_show (button5);

	tmp_toolbar_icon =
		anjuta_res_get_image (ANJUTA_PIXMAP_INDENT_INC);
	button6 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Indent inc"),
					    _
					    ("Increase indentation of block/line"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (button6);
	gtk_widget_show (button6);

	tmp_toolbar_icon =
		anjuta_res_get_image (ANJUTA_PIXMAP_INDENT_DCR);
	button7 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Indent dcr"),
					    _
					    ("Decrease indentation of block/line"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (button7);
	gtk_widget_show (button7);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar2));

	tmp_toolbar_icon =
		anjuta_res_get_image (ANJUTA_PIXMAP_INDENT_AUTO);
	button8 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Autoformat"),
					    _
					    ("Automatically format the code"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (button8);
	gtk_widget_show (button8);

	tmp_toolbar_icon =
		anjuta_res_get_image (ANJUTA_PIXMAP_AUTOFORMAT_SETTING);
	button9 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Format style"),
					    _("Configure style to use for auto format"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (button9);
	gtk_widget_show (button9);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar2));

	tmp_toolbar_icon =
		anjuta_res_get_image (ANJUTA_PIXMAP_CALLTIP);
	button10 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Calltip"), _("Show Calltip"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (button10);
	
	/* unimplemented */
	 gtk_widget_hide (button10);

	tmp_toolbar_icon =
		anjuta_res_get_image (ANJUTA_PIXMAP_AUTOCOMPLETE);
	button11 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Autocomplete"),
					    _("Autocomplete word"), NULL,
					    tmp_toolbar_icon, NULL, NULL);
	gtk_widget_ref (button11);
	gtk_widget_show (button11);

	gtk_signal_connect (GTK_OBJECT (button2), "clicked",
			    GTK_SIGNAL_FUNC (on_format_fold_toggle_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (button3), "clicked",
			    GTK_SIGNAL_FUNC (on_format_fold_open_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (button4), "clicked",
			    GTK_SIGNAL_FUNC (on_format_fold_close_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (button5), "clicked",
			    GTK_SIGNAL_FUNC (on_format_block_select_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (button6), "clicked",
			    GTK_SIGNAL_FUNC (on_format_indent_inc_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (button7), "clicked",
			    GTK_SIGNAL_FUNC (on_format_indent_dcr_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (button8), "clicked",
			    GTK_SIGNAL_FUNC (on_format_indent_auto_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (button9), "clicked",
			    GTK_SIGNAL_FUNC (on_format_indent_style_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (button10), "clicked",
			    GTK_SIGNAL_FUNC (on_format_calltip_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (button11), "clicked",
			    GTK_SIGNAL_FUNC (on_format_autocomplete_clicked),
			    NULL);

	toolbar->toolbar = toolbar2;
	toolbar->toggle_fold = button2;
	toolbar->open_all_folds = button3;
	toolbar->close_all_folds = button4;
	toolbar->block_select = button5;
	toolbar->indent_increase = button6;
	toolbar->indent_decrease = button7;
	toolbar->autoformat = button8;
	toolbar->set_autoformat_style = button9;
	toolbar->calltip = button10;
	toolbar->autocomplete = button11;

	return toolbar2;
}
