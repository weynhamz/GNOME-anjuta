/*
    text_editor_gui.c
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
#include "text_editor_cbs.h"
#include "text_editor_gui.h"
#include "mainmenu_callbacks.h"
#include "pixmaps.h"
#include "resources.h"
#include "print.h"

#define GTK
#undef PLAT_GTK
#define PLAT_GTK 1
#include "Scintilla.h"
#include "ScintillaWidget.h"
#include "properties.h"
#include "aneditor.h"

#include <libgnomeui/gnome-window-icon.h>

gint
on_text_editor_key_pressed (GtkWidget * widget, GdkEventKey * event,
			    gpointer data);

void
create_text_editor_gui (TextEditor * te)
{
	GtkWidget *window1;
	GtkWidget *dock1;
	GtkWidget *dock_item1;
	GtkWidget *toolbar1;
	GtkWidget *frame1;
	GtkWidget *hbox1;
	GtkWidget *label1;
	GtkWidget *label2;
	GtkWidget *event_box1;
	GtkWidget *client_frame;
	GtkWidget *editor1;
	static guint current_id;

	window1 = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_transient_for(GTK_WINDOW(window1), GTK_WINDOW(app->widgets.window));
	gnome_window_icon_set_from_default((GtkWindow *) window1);
	gtk_window_set_wmclass (GTK_WINDOW (window1), "editor", "Anjuta");
	gtk_widget_set_size_request (window1, 200, 200);
	gtk_window_move (GTK_WINDOW (window1), te->geom.x, te->geom.y);
	gtk_window_set_default_size (GTK_WINDOW (window1),
				     te->geom.width, te->geom.height);

	dock1 = bonobo_dock_new ();
	gtk_widget_show (dock1);
	gtk_container_add (GTK_CONTAINER (window1), dock1);

	dock_item1 = bonobo_dock_item_new ("text_toolbar",
					  BONOBO_DOCK_ITEM_BEH_EXCLUSIVE |
					  BONOBO_DOCK_ITEM_BEH_NEVER_FLOATING);
	gtk_widget_show (dock_item1);
	bonobo_dock_add_item (BONOBO_DOCK (dock1), BONOBO_DOCK_ITEM (dock_item1),
			     BONOBO_DOCK_TOP, 1, 1, 0, 0);
	bonobo_dock_item_set_shadow_type (BONOBO_DOCK_ITEM (dock_item1),
					 GTK_SHADOW_OUT);

	toolbar1 = gtk_toolbar_new ();
	gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar1),
	                             GTK_ORIENTATION_HORIZONTAL);
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar1), GTK_TOOLBAR_ICONS);
	gtk_widget_show (toolbar1);
	gtk_container_add (GTK_CONTAINER (dock_item1), toolbar1);
	
	te->buttons.novus = 
		anjuta_util_toolbar_append_stock (toolbar1, GTK_STOCK_NEW, 
							  _("New file"),
							  G_CALLBACK (on_new_file1_activate), te);
	te->buttons.open = 
		anjuta_util_toolbar_append_stock (toolbar1, GTK_STOCK_OPEN, 
							  _("Open file"),
							  G_CALLBACK (on_open1_activate), te);
	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar1));

	te->buttons.save = 
		anjuta_util_toolbar_append_stock (toolbar1, GTK_STOCK_SAVE, 
							  _("Save current file"),
							  G_CALLBACK (on_save1_activate), te);
	te->buttons.reload =
		anjuta_util_toolbar_append_stock (toolbar1, GTK_STOCK_REVERT_TO_SAVED,
						   _("Reload current file"),
						   G_CALLBACK (on_reload_file1_activate), te);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar1));

	te->buttons.cut =
		anjuta_util_toolbar_append_stock (toolbar1, GTK_STOCK_CUT, 
						   _("Cut to clipboard"),
						   G_CALLBACK (on_editor_command_activate),
						   (gpointer) ANE_CUT);
	te->buttons.copy =
		anjuta_util_toolbar_append_stock (toolbar1, GTK_STOCK_COPY, 
						   _("Copy to clipboard"),
						   G_CALLBACK (on_editor_command_activate),
						   (gpointer) ANE_COPY);
	te->buttons.paste =
		anjuta_util_toolbar_append_stock (toolbar1, GTK_STOCK_PASTE, 
						   _("Paste from clipboard"),
						   G_CALLBACK (on_editor_command_activate),
						   (gpointer) ANE_PASTE);
	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar1));

	te->buttons.find =
		anjuta_util_toolbar_append_stock (toolbar1, GTK_STOCK_FIND, 
						   _("Search string"),
						   G_CALLBACK (on_find1_activate), te);
	te->buttons.replace =
		anjuta_util_toolbar_append_stock (toolbar1, GTK_STOCK_FIND_AND_REPLACE,
						   _("Search and replace string"),
						   G_CALLBACK (on_find_and_replace1_activate), te);
	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar1));

	frame1 = gtk_frame_new (NULL);
	gtk_widget_show (frame1);
	gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar1), frame1, NULL,
				   NULL);
	gtk_widget_set_size_request (frame1, 117, -1);
	gtk_frame_set_shadow_type (GTK_FRAME (frame1), GTK_SHADOW_IN);
	gtk_widget_hide (frame1);

	hbox1 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox1);
	gtk_container_add (GTK_CONTAINER (frame1), hbox1);

	label1 = gtk_label_new (_("line: "));
	gtk_widget_show (label1);
	gtk_box_pack_start (GTK_BOX (hbox1), label1, FALSE, FALSE, 0);

	label2 = gtk_label_new (_("0"));
	gtk_widget_show (label2);
	gtk_box_pack_start (GTK_BOX (hbox1), label2, FALSE, FALSE, 0);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar1));

	te->buttons.compile =
		anjuta_util_toolbar_append_stock (toolbar1, GTK_STOCK_CONVERT, 
						   _("Compile the current file"),
						   G_CALLBACK (on_compile1_activate), te);
	te->buttons.build =
		anjuta_util_toolbar_append_stock (toolbar1, GTK_STOCK_EXECUTE, 
						   _("Build current file or the source directory of the Project"),
						   G_CALLBACK (on_build_project1_activate), te);
	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar1));

	te->buttons.print =
		anjuta_util_toolbar_append_stock (toolbar1, GTK_STOCK_PRINT, 
						   _("Print the current file"),
						   G_CALLBACK (anjuta_print_cb), te);
	te->buttons.attach =
		anjuta_util_toolbar_append_button (toolbar1, ANJUTA_PIXMAP_DOCK, _("Attach"),
						   _("Attach the current page"),
						   G_CALLBACK (on_text_editor_dock_activate), te);
	
	frame1 = gtk_frame_new (NULL);
	gtk_widget_show (frame1);
	bonobo_dock_set_client_area (BONOBO_DOCK (dock1), frame1);
	gtk_frame_set_shadow_type (GTK_FRAME (frame1), GTK_SHADOW_NONE);

	event_box1 = gtk_event_box_new ();
	gtk_widget_show (event_box1);
	/* container is not added now, since we don't know
	if it is windowed or paged */

	client_frame = gtk_frame_new (NULL);
	gtk_widget_show (client_frame);
	gtk_container_add (GTK_CONTAINER (event_box1), client_frame);
	gtk_frame_set_shadow_type (GTK_FRAME (client_frame), GTK_SHADOW_IN);

	editor1 = aneditor_get_widget (te->editor_id);
	gtk_widget_show_all (editor1);
	scintilla_set_id (SCINTILLA (editor1), current_id++);
	gtk_container_add (GTK_CONTAINER (client_frame), editor1);

	gtk_window_add_accel_group (GTK_WINDOW (window1), app->accel_group);

	g_signal_connect (G_OBJECT (window1), "realize",
			    G_CALLBACK (on_text_editor_window_realize),
			    te);
	g_signal_connect (G_OBJECT (window1), "size_allocate",
			    G_CALLBACK
			    (on_text_editor_window_size_allocate), te);
	g_signal_connect (G_OBJECT (window1), "focus_in_event",
			    G_CALLBACK
			    (on_text_editor_window_focus_in_event), te);
	g_signal_connect (G_OBJECT (window1), "delete_event",
			    G_CALLBACK (on_text_editor_window_delete),
			    te);
	g_signal_connect (G_OBJECT (event_box1), "realize",
			    G_CALLBACK (on_text_editor_client_realize),
			    te);

	g_signal_connect (G_OBJECT (editor1), "event",
			    G_CALLBACK (on_text_editor_text_event), te);
	g_signal_connect (G_OBJECT (editor1), "button_press_event",
			    G_CALLBACK (on_text_editor_text_buttonpress_event), te);
	g_signal_connect_after (G_OBJECT (editor1), "size_allocate",
			    G_CALLBACK (on_text_editor_scintilla_size_allocate), te);
	g_signal_connect (G_OBJECT (editor1), "sci-notify",
			    G_CALLBACK (on_text_editor_scintilla_notify), te);

	te->widgets.window = window1;
	te->widgets.client_area = frame1;
	te->widgets.client = event_box1;
	te->widgets.editor = editor1;
	te->widgets.line_label = label2;
	te->buttons.close = NULL;	/* Created later */
	te->widgets.tab_label = NULL;	/* Created later */
	te->widgets.close_pixmap = NULL;	/* Created later */

	gtk_widget_ref (te->buttons.novus);
	gtk_widget_ref (te->buttons.open);
	gtk_widget_ref (te->buttons.save);
	gtk_widget_ref (te->buttons.reload);
	gtk_widget_ref (te->buttons.cut);
	gtk_widget_ref (te->buttons.copy);
	gtk_widget_ref (te->buttons.paste);
	gtk_widget_ref (te->buttons.find);
	gtk_widget_ref (te->buttons.replace);
	gtk_widget_ref (te->buttons.compile);
	gtk_widget_ref (te->buttons.build);
	gtk_widget_ref (te->buttons.print);
	gtk_widget_ref (te->buttons.attach);

	gtk_widget_ref (te->widgets.window);
	gtk_widget_ref (te->widgets.client_area);
	gtk_widget_ref (te->widgets.client);
	gtk_widget_ref (te->widgets.editor);
	gtk_widget_ref (te->widgets.line_label);
}
