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
#include "print-doc.h"

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
	GtkWidget *tmp_toolbar_icon;
	GtkWidget *button1;
	GtkWidget *button2;
	GtkWidget *button3;
	GtkWidget *button5;
	GtkWidget *button6;
	GtkWidget *button7;
	GtkWidget *button8;
	GtkWidget *button9;
	GtkWidget *button10;
	GtkWidget *frame1;
	GtkWidget *hbox1;
	GtkWidget *label1;
	GtkWidget *label2;
	GtkWidget *button11;
	GtkWidget *button12;
	GtkWidget *button14;
	GtkWidget *button13;
	GtkWidget *event_box1;
	GtkWidget *client_frame;
	GtkWidget *editor1;
	static guint current_id;

	window1 = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_transient_for(GTK_WINDOW(window1), GTK_WINDOW(app->widgets.window));
	gnome_window_icon_set_from_default((GtkWindow *) window1);
	gtk_window_set_wmclass (GTK_WINDOW (window1), "editor", "Anjuta");
	gtk_widget_set_usize (window1, 200, 200);
	gtk_widget_set_uposition (window1, te->geom.x, te->geom.y);
	gtk_window_set_default_size (GTK_WINDOW (window1),
				     te->geom.width, te->geom.height);

#warning "G2: Add proper toolbar dock"
#if 0
	dock1 = bonobo_dock_new ();
	gtk_widget_show (dock1);
	gtk_container_add (GTK_CONTAINER (window1), dock1);

	dock_item1 = bonobo_dock_item_new ("text_toolbar",
					  BONOBO_DOCK_ITEM_BEH_EXCLUSIVE |
					  BONOBO_DOCK_ITEM_BEH_NEVER_FLOATING);
	gtk_widget_show (dock_item1);
	bonobo_dock_add_item (GNOME_DOCK (dock1), BONOBO_DOCK_ITEM (dock_item1),
			     BONOBO_DOCK_TOP, 1, 1, 0, 0);
	bonobo_dock_item_set_shadow_type (BONOBO_DOCK_ITEM (dock_item1),
					 GTK_SHADOW_OUT);
	gtk_container_set_border_width (GTK_CONTAINER (dock_item1), 2);

#else
	
	dock1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (dock1);
	gtk_container_add (GTK_CONTAINER (window1), dock1);

#endif

	toolbar1 = gtk_toolbar_new ();
	gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar1),
	                             GTK_ORIENTATION_HORIZONTAL);
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar1), GTK_TOOLBAR_ICONS);
	gtk_widget_show (toolbar1);
	gtk_container_add (GTK_CONTAINER (dock1), toolbar1);
	gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar1),
				       GTK_RELIEF_NONE);
	gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar1),
				     GTK_TOOLBAR_SPACE_LINE);

	tmp_toolbar_icon = anjuta_res_get_pixmap_widget (window1,
													 ANJUTA_PIXMAP_NEW_FILE);
	button1 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar1),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("New"), _("New Text File"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_show (button1);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (window1, ANJUTA_PIXMAP_OPEN_FILE);
	button2 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar1),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Open"), _("Open Text File"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_show (button2);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar1));

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (window1, ANJUTA_PIXMAP_SAVE_FILE);
	button3 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar1),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Save"), _("Save Current File"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_show (button3);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (window1, ANJUTA_PIXMAP_RELOAD_FILE);
	button5 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar1),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Reload"),
					    _("Reload Current File"), NULL,
					    tmp_toolbar_icon, NULL, NULL);
	gtk_widget_show (button5);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar1));

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (window1, ANJUTA_PIXMAP_CUT);
	button6 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar1),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Cut"), _("Cut to clipboard"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_show (button6);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (window1, ANJUTA_PIXMAP_COPY);
	button7 = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar1),
					      GTK_TOOLBAR_CHILD_BUTTON,
					      NULL,
					      _("Copy"),
					      _("Copy to clipboard"), NULL,
					      tmp_toolbar_icon, NULL, NULL);
	gtk_widget_show (button7);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (window1, ANJUTA_PIXMAP_PASTE);
	button8 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar1),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Paste"),
					    _("Paste from clipboard"), NULL,
					    tmp_toolbar_icon, NULL, NULL);
	gtk_widget_show (button8);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar1));

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (window1, ANJUTA_PIXMAP_SEARCH);
	button9 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar1),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Find"),
					    _("Search the given string"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_show (button9);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (window1, ANJUTA_PIXMAP_FIND_REPLACE);
	button10 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar1),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Replace"),
					    _
					    ("Search and replace the given string"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_show (button10);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar1));

	frame1 = gtk_frame_new (NULL);
	gtk_widget_show (frame1);
	gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar1), frame1, NULL,
				   NULL);
	gtk_widget_set_usize (frame1, 117, -2);
	gtk_frame_set_shadow_type (GTK_FRAME (frame1), GTK_SHADOW_IN);
	gtk_container_set_border_width (GTK_CONTAINER (frame1), 2);
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

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (window1, ANJUTA_PIXMAP_COMPILE);
	button11 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar1),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Compile"),
					    _("Compile the current file"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_show (button11);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (window1, ANJUTA_PIXMAP_BUILD);
	button12 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar1),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Build"),
					    _
					    ("Build current file or the source directory of the Project"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_show (button12);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar1));

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (window1, ANJUTA_PIXMAP_PRINT);
	button14 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar1),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Print"),
					    _("Print the current File"), NULL,
					    tmp_toolbar_icon, NULL, NULL);
	gtk_widget_show (button14);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar1));

	tmp_toolbar_icon = anjuta_res_get_pixmap_widget (window1, ANJUTA_PIXMAP_DOCK);
	button13 = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar1),
					       GTK_TOOLBAR_CHILD_BUTTON,
					       NULL,
					       _("Attach"),
					       _
					       ("Attach the editor window as paged editor"),
					       NULL, tmp_toolbar_icon, NULL,
					       NULL);
	gtk_widget_show (button13);

	frame1 = gtk_frame_new (NULL);
	gtk_widget_show (frame1);
	//gnome_dock_set_client_area (GNOME_DOCK (dock1), frame1);
	gtk_container_add (GTK_CONTAINER (dock1), frame1);
	gtk_frame_set_shadow_type (GTK_FRAME (frame1), GTK_SHADOW_OUT);

	event_box1 = gtk_event_box_new ();
	gtk_widget_show (event_box1);
	/* container is not added now, since we don't know if it is windowed or paged */

	client_frame = gtk_frame_new (NULL);
	gtk_widget_show (client_frame);
	gtk_container_add (GTK_CONTAINER (event_box1), client_frame);
	gtk_frame_set_shadow_type (GTK_FRAME (client_frame), GTK_SHADOW_IN);

	editor1 = aneditor_get_widget (te->editor_id);
	gtk_widget_show_all (editor1);
	scintilla_set_id (SCINTILLA (editor1), current_id++);
	gtk_container_add (GTK_CONTAINER (client_frame), editor1);

	gtk_accel_group_attach (app->accel_group, GTK_OBJECT (window1));

	gtk_signal_connect (GTK_OBJECT (window1), "realize",
			    GTK_SIGNAL_FUNC (on_text_editor_window_realize),
			    te);
	gtk_signal_connect (GTK_OBJECT (window1), "size_allocate",
			    GTK_SIGNAL_FUNC
			    (on_text_editor_window_size_allocate), te);
	gtk_signal_connect (GTK_OBJECT (button1), "clicked",
			    GTK_SIGNAL_FUNC (on_new_file1_activate), te);
	gtk_signal_connect (GTK_OBJECT (button2), "clicked",
			    GTK_SIGNAL_FUNC (on_open1_activate), te);
	gtk_signal_connect (GTK_OBJECT (button3), "clicked",
			    GTK_SIGNAL_FUNC (on_save1_activate), te);
	gtk_signal_connect (GTK_OBJECT (button5), "clicked",
			    GTK_SIGNAL_FUNC (on_reload_file1_activate), te);
	gtk_signal_connect (GTK_OBJECT (button6), "clicked",
			    GTK_SIGNAL_FUNC (on_editor_command_activate)
				, (gpointer) ANE_CUT);
	gtk_signal_connect (GTK_OBJECT (button7), "clicked",
			    GTK_SIGNAL_FUNC (on_editor_command_activate)
				, (gpointer) ANE_COPY);
	gtk_signal_connect (GTK_OBJECT (button8), "clicked",
			    GTK_SIGNAL_FUNC (on_editor_command_activate)
				, (gpointer) ANE_PASTE);
	gtk_signal_connect (GTK_OBJECT (button9), "clicked",
			    GTK_SIGNAL_FUNC (on_find1_activate), te);
	gtk_signal_connect (GTK_OBJECT (button10), "clicked",
			    GTK_SIGNAL_FUNC (on_find_and_replace1_activate),
			    te);
	gtk_signal_connect (GTK_OBJECT (button11), "clicked",
			    GTK_SIGNAL_FUNC (on_compile1_activate), te);
	gtk_signal_connect (GTK_OBJECT (button12), "clicked",
			    GTK_SIGNAL_FUNC (on_build_project1_activate), te);
	gtk_signal_connect (GTK_OBJECT (button14), "clicked",
			    GTK_SIGNAL_FUNC (anjuta_print_cb), NULL);
	gtk_signal_connect (GTK_OBJECT (button13), "clicked",
			    GTK_SIGNAL_FUNC (on_text_editor_dock_activate),
			    te);
/***************************************************************************/

	gtk_signal_connect (GTK_OBJECT (window1), "focus_in_event",
			    GTK_SIGNAL_FUNC
			    (on_text_editor_window_focus_in_event), te);
	gtk_signal_connect (GTK_OBJECT (editor1), "button_press_event",
			    GTK_SIGNAL_FUNC
			    (on_text_editor_text_buttonpress_event), te);
	gtk_signal_connect_after (GTK_OBJECT (editor1), "size_allocate",
			    GTK_SIGNAL_FUNC
			    (on_text_editor_scintilla_size_allocate), te);

	gtk_signal_connect (GTK_OBJECT (editor1), "event",
			    GTK_SIGNAL_FUNC (on_text_editor_text_event), te);
	gtk_signal_connect (GTK_OBJECT (window1), "delete_event",
			    GTK_SIGNAL_FUNC (on_text_editor_window_delete),
			    te);
	gtk_signal_connect (GTK_OBJECT (event_box1), "realize",
			    GTK_SIGNAL_FUNC (on_text_editor_client_realize),
			    te);
	gtk_signal_connect (GTK_OBJECT (editor1), "notify",
			    GTK_SIGNAL_FUNC (on_text_editor_scintilla_notify),
			    te);

	te->widgets.window = window1;
	te->widgets.client_area = frame1;
	te->widgets.client = event_box1;
	te->widgets.editor = editor1;
	te->widgets.line_label = label2;
	te->buttons.close = NULL;	/* Created later */
	te->widgets.tab_label = NULL;	/* Created later */
	te->widgets.close_pixmap = NULL;	/* Created later */
	
	te->buttons.novus = button1;
	te->buttons.open = button2;
	te->buttons.save = button3;
	te->buttons.reload = button5;
	te->buttons.cut = button6;
	te->buttons.copy = button7;
	te->buttons.paste = button8;
	te->buttons.find = button9;
	te->buttons.replace = button10;
	te->buttons.compile = button11;
	te->buttons.build = button12;
	te->buttons.print = button14;
	te->buttons.attach = button13;

	gtk_widget_ref (te->buttons.novus);
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
