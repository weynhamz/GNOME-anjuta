/*
    registers_gui.c
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
#include "registers.h"
#include "registers_cbs.h"
#include "resources.h"

GtkWidget* create_register_menu (void);

static GnomeUIInfo register_menu_uiinfo[] =
{
  {
    GNOME_APP_UI_ITEM, N_("Modify"),
    NULL,
    on_register_modify_activate, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  {
    GNOME_APP_UI_ITEM, N_("Update"),
    NULL,
    on_register_update_activate, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  GNOMEUIINFO_SEPARATOR,
  {
    GNOME_APP_UI_ITEM, N_("Dock/Undock"),
    NULL,
    on_register_dock_undock_activate, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  {
    GNOME_APP_UI_ITEM, N_("Help"),
    NULL,
    on_register_help_activate, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  GNOMEUIINFO_END
};

GtkWidget*
create_register_menu ()
{
  GtkWidget *register_menu;

  register_menu = gtk_menu_new ();
  gnome_app_fill_menu (GTK_MENU_SHELL (register_menu), register_menu_uiinfo,
                       NULL, FALSE, 0);
  return register_menu;
}

void
create_cpu_registers_gui(CpuRegisters *cr)
{
  GtkWidget *window3;
  GtkWidget *scrolledwindow4;
  GtkWidget *clist4;
  GtkWidget *label6;
  GtkWidget *label7;
  GtkWidget *label8;

  window3 = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_usize (window3, 170, -2);
  gtk_window_set_title (GTK_WINDOW (window3), _("CPU Registers"));
  gtk_window_set_default_size (GTK_WINDOW (window3), 240, 230);

  scrolledwindow4 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scrolledwindow4);
  gtk_container_add (GTK_CONTAINER (window3), scrolledwindow4);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow4), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  clist4 = gtk_clist_new (3);
  gtk_widget_show (clist4);
  gtk_container_add (GTK_CONTAINER (scrolledwindow4), clist4);
  gtk_clist_set_column_width (GTK_CLIST (clist4), 0, 80);
  gtk_clist_set_column_width (GTK_CLIST (clist4), 1, 80);
  gtk_clist_set_column_width (GTK_CLIST (clist4), 2, 80);
  gtk_clist_set_selection_mode (GTK_CLIST (clist4), GTK_SELECTION_BROWSE);
  gtk_clist_column_titles_show (GTK_CLIST (clist4));

  label6 = gtk_label_new (_("Register"));
  gtk_widget_show (label6);
  gtk_clist_set_column_widget (GTK_CLIST (clist4), 0, label6);

  label7 = gtk_label_new (_("Hex"));
  gtk_widget_show (label7);
  gtk_clist_set_column_widget (GTK_CLIST (clist4), 1, label7);

  label8 = gtk_label_new (_("Decimal"));
  gtk_widget_show (label8);
  gtk_clist_set_column_widget (GTK_CLIST (clist4), 2, label8);

  gtk_accel_group_attach( app->accel_group, GTK_OBJECT(window3));

  gtk_signal_connect (GTK_OBJECT (clist4), "select_row",
                      GTK_SIGNAL_FUNC (on_registers_clist_select_row),
                      cr);
  gtk_signal_connect (GTK_OBJECT (window3), "delete_event",
                             GTK_SIGNAL_FUNC (on_registers_delete_event), cr);

  gtk_signal_connect (GTK_OBJECT (clist4), "event",
                      GTK_SIGNAL_FUNC (on_register_event),
                      cr);

  cr->widgets.window = window3;
  cr->widgets.clist = clist4;
  cr->widgets.menu = create_register_menu();
  cr->widgets.menu_modify = register_menu_uiinfo[0].widget;
  cr->widgets.menu_update = register_menu_uiinfo[1].widget;
  
  gtk_widget_ref(cr->widgets.window);
  gtk_widget_ref(cr->widgets.clist);
  gtk_widget_ref(cr->widgets.menu);
  gtk_widget_ref(cr->widgets.menu_modify);
  gtk_widget_ref(cr->widgets.menu_update);

}
