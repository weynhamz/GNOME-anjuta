/*
    anjuta1.c
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
#include "resources.h"
#include "main_menubar.h"
#include "notebook.h"
#include "dnd.h"

void
create_anjuta_gui (AnjutaApp * appl)
{
	GtkWidget *anjuta_gui;
	GtkWidget *dock1;
	GtkWidget *eventbox1;
	GtkWidget *toolbar1;
	GtkWidget *toolbar2;
	GtkWidget *toolbar3;
	GtkWidget *toolbar4;
	GtkWidget *toolbar5;
	GtkWidget *toolbar6;
	GtkWidget *vpaned1;
	GtkWidget *hpaned1;
	GtkWidget *vbox1;
	GtkWidget *hbox3;
	GtkWidget *button1;
	GtkWidget *frame1;
	GtkWidget *vbox3;
	GtkWidget *pixmap;
	GtkWidget *hseparator1;
	GtkWidget *hseparator2;
	GtkWidget *hseparator3;
	GtkWidget *button3;
	GtkWidget *hbox2;
	GtkWidget *vbox2;
	GtkWidget *button4;
	GtkWidget *frame2;
	GtkWidget *hbox4;
	GtkWidget *vseparator3;
	GtkWidget *vseparator2;
	GtkWidget *vseparator1;
	GtkWidget *button2;
	GtkWidget *anjuta_notebook;
	GtkWidget *appbar1;
	GtkTooltips *tooltips;

	tooltips = gtk_tooltips_new ();

	anjuta_gui = gnome_app_new ("Anjuta", "Anjuta");
	gtk_widget_set_uposition (anjuta_gui, 0, 0);
	gtk_widget_set_usize (anjuta_gui, 500, 116);
	gtk_window_set_default_size (GTK_WINDOW (anjuta_gui), 700, 400);
	gtk_window_set_policy (GTK_WINDOW (anjuta_gui), FALSE, TRUE, FALSE);
	gtk_window_set_wmclass (GTK_WINDOW (anjuta_gui), "mainide", "Anjuta");
	
	/* Add file drag and drop support */
	dnd_drop_init(anjuta_gui, on_anjuta_dnd_drop, NULL,
		"text/plain", "text/html", "text/source", NULL);

	dock1 = GNOME_APP (anjuta_gui)->dock;
	gtk_widget_show (dock1);

	create_main_menubar (anjuta_gui, &(app->widgets.menubar));

	/* Toolbars are created and added */

	toolbar1 =
		create_main_toolbar (anjuta_gui,
				     &(appl->widgets.toolbar.main_toolbar));
	gnome_app_add_toolbar (GNOME_APP (anjuta_gui), GTK_TOOLBAR (toolbar1),
			       ANJUTA_MAIN_TOOLBAR,
			       GNOME_DOCK_ITEM_BEH_EXCLUSIVE |
			       GNOME_DOCK_ITEM_BEH_NEVER_VERTICAL,
			       GNOME_DOCK_TOP, 1, 0, 0);
	gtk_toolbar_set_space_size (GTK_TOOLBAR (toolbar1), 5);
	gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar1),
				     GTK_TOOLBAR_SPACE_LINE);
	gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar1),
				       GTK_RELIEF_NONE);

	toolbar2 =
		create_extended_toolbar (anjuta_gui,
					 &(appl->widgets.toolbar.
					   extended_toolbar));
	gnome_app_add_toolbar (GNOME_APP (anjuta_gui), GTK_TOOLBAR (toolbar2),
			       ANJUTA_EXTENDED_TOOLBAR,
			       GNOME_DOCK_ITEM_BEH_NORMAL,
			       GNOME_DOCK_TOP, 3, 0, 0);
	gtk_toolbar_set_space_size (GTK_TOOLBAR (toolbar2), 5);
	gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar2),
				     GTK_TOOLBAR_SPACE_LINE);
	gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar2),
				       GTK_RELIEF_NONE);

	toolbar3 =
		create_tags_toolbar (anjuta_gui,
				     &(appl->widgets.toolbar.tags_toolbar));
	gnome_app_add_toolbar (GNOME_APP (anjuta_gui), GTK_TOOLBAR (toolbar3),
			       ANJUTA_TAGS_TOOLBAR,
			       GNOME_DOCK_ITEM_BEH_EXCLUSIVE |
			       GNOME_DOCK_ITEM_BEH_NEVER_VERTICAL,
			       GNOME_DOCK_TOP, 2, 0, 0);
	gtk_toolbar_set_space_size (GTK_TOOLBAR (toolbar3), 5);
	gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar3),
				     GTK_TOOLBAR_SPACE_LINE);
	gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar3),
				       GTK_RELIEF_NONE);

	toolbar4 =
		create_debug_toolbar (anjuta_gui,
				      &(appl->widgets.toolbar.debug_toolbar));
	gnome_app_add_toolbar (GNOME_APP (anjuta_gui), GTK_TOOLBAR (toolbar4),
			       ANJUTA_DEBUG_TOOLBAR,
			       GNOME_DOCK_ITEM_BEH_NORMAL,
			       GNOME_DOCK_TOP, 3, 1, 0);
	gtk_toolbar_set_space_size (GTK_TOOLBAR (toolbar4), 5);
	gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar4),
				     GTK_TOOLBAR_SPACE_LINE);
	gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar4),
				       GTK_RELIEF_NONE);

	toolbar5 =
		create_browser_toolbar (anjuta_gui,
					&(appl->widgets.toolbar.
					  browser_toolbar));
	gnome_app_add_toolbar (GNOME_APP (anjuta_gui), GTK_TOOLBAR (toolbar5),
			       ANJUTA_BROWSER_TOOLBAR,
			       GNOME_DOCK_ITEM_BEH_NORMAL,
			       GNOME_DOCK_TOP, 4, 0, 0);
	gtk_toolbar_set_space_size (GTK_TOOLBAR (toolbar5), 5);
	gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar5),
				     GTK_TOOLBAR_SPACE_LINE);
	gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar5),
				       GTK_RELIEF_NONE);

	toolbar6 =
		create_format_toolbar (anjuta_gui,
				       &(appl->widgets.toolbar.
					 format_toolbar));
	gnome_app_add_toolbar (GNOME_APP (anjuta_gui), GTK_TOOLBAR (toolbar6),
			       ANJUTA_FORMAT_TOOLBAR,
			       GNOME_DOCK_ITEM_BEH_NORMAL,
			       GNOME_DOCK_TOP, 4, 1, 0);
	gtk_toolbar_set_space_size (GTK_TOOLBAR (toolbar6), 5);
	gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar6),
				     GTK_TOOLBAR_SPACE_LINE);
	gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar6),
				       GTK_RELIEF_NONE);

	eventbox1 = gtk_event_box_new ();
	gtk_widget_ref (eventbox1);
	gtk_widget_show (eventbox1);
	gnome_app_set_contents (GNOME_APP (anjuta_gui), eventbox1);

	vpaned1 = gtk_vpaned_new ();
	gtk_widget_ref (vpaned1);
	gtk_widget_show (vpaned1);
	gtk_container_add (GTK_CONTAINER (eventbox1), vpaned1);
	gtk_paned_set_gutter_size (GTK_PANED (vpaned1), 8);
	gtk_paned_set_handle_size (GTK_PANED (vpaned1), 8);

	hpaned1 = gtk_hpaned_new ();
	gtk_widget_ref (hpaned1);
	gtk_widget_show (hpaned1);
	gtk_container_add (GTK_CONTAINER (vpaned1), hpaned1);
	gtk_paned_set_gutter_size (GTK_PANED (hpaned1), 8);
	gtk_paned_set_handle_size (GTK_PANED (hpaned1), 8);

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_ref (vbox1);
	gtk_widget_show (vbox1);
	gtk_container_add (GTK_CONTAINER (hpaned1), vbox1);

	hbox3 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox3);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox3, FALSE, FALSE, 0);

	button1 = gtk_button_new ();
	gtk_widget_show (button1);
	gtk_box_pack_start (GTK_BOX (hbox3), button1, FALSE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (button1), 1);
	gtk_tooltips_set_tip (tooltips, button1,
			      _("Click here to undock this window"), NULL);

	pixmap =
		anjuta_res_get_pixmap_widget (anjuta_gui, "handle_undock.xpm", FALSE);
	gtk_widget_show (pixmap);
	gtk_container_add (GTK_CONTAINER (button1), pixmap);

	frame1 = gtk_frame_new (NULL);
	gtk_widget_show (frame1);
	gtk_box_pack_start (GTK_BOX (hbox3), frame1, TRUE, TRUE, 1);
	gtk_container_set_border_width (GTK_CONTAINER (frame1), 1);
	gtk_frame_set_shadow_type (GTK_FRAME (frame1), GTK_SHADOW_NONE);

	vbox3 = gtk_vbox_new (TRUE, 1);
	gtk_widget_show (vbox3);
	gtk_container_add (GTK_CONTAINER (frame1), vbox3);

	hseparator1 = gtk_hseparator_new ();
	gtk_widget_show (hseparator1);
	gtk_box_pack_start (GTK_BOX (vbox3), hseparator1, TRUE, TRUE, 0);

	hseparator2 = gtk_hseparator_new ();
	gtk_widget_show (hseparator2);
	gtk_box_pack_start (GTK_BOX (vbox3), hseparator2, TRUE, TRUE, 0);

	hseparator3 = gtk_hseparator_new ();
	gtk_widget_show (hseparator3);
	gtk_box_pack_start (GTK_BOX (vbox3), hseparator3, TRUE, TRUE, 0);

	button3 = gtk_button_new ();
	gtk_widget_show (button3);
	gtk_box_pack_start (GTK_BOX (hbox3), button3, FALSE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (button3), 1);
	gtk_tooltips_set_tip (tooltips, button3,
			      _("Click here to hide this window"), NULL);


	pixmap = anjuta_res_get_pixmap_widget (anjuta_gui, "handle_hide.xpm", FALSE);
	gtk_widget_show (pixmap);
	gtk_container_add (GTK_CONTAINER (button3), pixmap);

	hbox2 = gtk_hbox_new (FALSE, 0);
	gtk_widget_ref (hbox2);
	gtk_widget_show (hbox2);
	gtk_container_add (GTK_CONTAINER (vpaned1), hbox2);

	vbox2 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox2);
	gtk_box_pack_start (GTK_BOX (hbox2), vbox2, FALSE, FALSE, 0);

	button4 = gtk_button_new ();
	gtk_widget_show (button4);

	gtk_box_pack_start (GTK_BOX (vbox2), button4, FALSE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (button4), 1);
	gtk_tooltips_set_tip (tooltips, button4,
			      _("Click here to hide this window"), NULL);

	pixmap = anjuta_res_get_pixmap_widget (anjuta_gui, "handle_hide.xpm", FALSE);
	gtk_widget_show (pixmap);
	gtk_container_add (GTK_CONTAINER (button4), pixmap);

	frame2 = gtk_frame_new (NULL);
	gtk_widget_show (frame2);
	gtk_box_pack_start (GTK_BOX (vbox2), frame2, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame2), 1);
	gtk_frame_set_shadow_type (GTK_FRAME (frame2), GTK_SHADOW_NONE);

	hbox4 = gtk_hbox_new (TRUE, 1);
	gtk_widget_show (hbox4);
	gtk_container_add (GTK_CONTAINER (frame2), hbox4);

	vseparator3 = gtk_vseparator_new ();
	gtk_widget_show (vseparator3);
	gtk_box_pack_start (GTK_BOX (hbox4), vseparator3, TRUE, TRUE, 0);

	vseparator2 = gtk_vseparator_new ();
	gtk_widget_show (vseparator2);
	gtk_box_pack_start (GTK_BOX (hbox4), vseparator2, TRUE, TRUE, 0);

	vseparator1 = gtk_vseparator_new ();
	gtk_widget_show (vseparator1);
	gtk_box_pack_start (GTK_BOX (hbox4), vseparator1, TRUE, TRUE, 0);

	button2 = gtk_button_new ();
	gtk_widget_show (button2);
	gtk_box_pack_start (GTK_BOX (vbox2), button2, FALSE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (button2), 1);
	gtk_tooltips_set_tip (tooltips, button2,
			      _("Click here to undock this window"), NULL);

	pixmap =
		anjuta_res_get_pixmap_widget (anjuta_gui, "handle_undock.xpm", FALSE);
	gtk_widget_show (pixmap);
	gtk_container_add (GTK_CONTAINER (button2), pixmap);

	anjuta_notebook = notebook_new ();
	gtk_widget_ref (anjuta_notebook);
	gtk_widget_show (anjuta_notebook);
	gtk_notebook_set_show_border (GTK_NOTEBOOK (anjuta_notebook), TRUE);
	gtk_notebook_set_scrollable (GTK_NOTEBOOK (anjuta_notebook), TRUE);

	/* Notebook is not added now. It will be added later */
	appbar1 = gnome_appbar_new (TRUE, TRUE, GNOME_PREFERENCES_USER);
	gtk_widget_ref (appbar1);
	gtk_widget_show (appbar1);
	gnome_app_set_statusbar (GNOME_APP (anjuta_gui), appbar1);

	gtk_signal_connect (GTK_OBJECT (anjuta_gui), "focus_in_event",
			    GTK_SIGNAL_FUNC (on_anjuta_window_focus_in_event),
			    NULL);

	gtk_signal_connect (GTK_OBJECT (anjuta_gui), "delete_event",
			    GTK_SIGNAL_FUNC (on_anjuta_delete), NULL);
	gtk_signal_connect (GTK_OBJECT (anjuta_gui), "destroy",
			    GTK_SIGNAL_FUNC (on_anjuta_destroy), NULL);
	gtk_signal_connect (GTK_OBJECT (anjuta_notebook), "switch_page",
			    GTK_SIGNAL_FUNC (on_anjuta_notebook_switch_page),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (button1), "clicked",
			    GTK_SIGNAL_FUNC
			    (on_prj_list_undock_button_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (button3), "clicked",
			    GTK_SIGNAL_FUNC (on_prj_list_hide_button_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (button4), "clicked",
			    GTK_SIGNAL_FUNC (on_mesg_win_hide_button_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (button2), "clicked",
			    GTK_SIGNAL_FUNC
			    (on_mesg_win_undock_button_clicked), NULL);

	app->accel_group = GNOME_APP (anjuta_gui)->accel_group;
	app->widgets.window = anjuta_gui;
	app->widgets.client_area = eventbox1;
	app->widgets.notebook = anjuta_notebook;
	app->widgets.appbar = appbar1;
	app->widgets.hpaned = hpaned1;
	app->widgets.vpaned = vpaned1;
	app->widgets.mesg_win_container = hbox2;
	app->widgets.project_dbase_win_container = vbox1;

	main_menu_install_hints ();
}
