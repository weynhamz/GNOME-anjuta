/*
    stack_trace_gui.c
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
#include <libgnomeui/gnome-window-icon.h>

#include "anjuta.h"
#include "stack_trace.h"
#include "stack_trace_cbs.h"
#include "resources.h"

static GnomeUIInfo stack_menu_uiinfo[] =
{
  {
    GNOME_APP_UI_ITEM, N_("Frame set"),
    NULL,
    on_stack_frame_set_activate, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  {
    GNOME_APP_UI_ITEM, N_("Frame info"),
    NULL,
    on_stack_frame_info_activate, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  {
    GNOME_APP_UI_ITEM, N_("Update"),
    NULL,
    on_stack_update_activate, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  {
    GNOME_APP_UI_ITEM, N_("View Source"),
    NULL,
    on_stack_view_src_activate, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  GNOMEUIINFO_SEPARATOR,
  {
    GNOME_APP_UI_ITEM, N_("Help"),
    NULL,
    on_stack_help_activate, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  GNOMEUIINFO_END
};

GtkWidget* create_stack_menu (void);

GtkWidget*
create_stack_menu ()
{
  GtkWidget *menu4;

  menu4 = gtk_menu_new ();
  gnome_app_fill_menu (GTK_MENU_SHELL (menu4), stack_menu_uiinfo,
                       NULL, FALSE, 0);
  return menu4;
}

void
create_stack_trace_gui(StackTrace *st)
{
  GtkWidget *window1;
  GtkWidget *scrolledwindow1;
  GtkWidget *clist1;
  GtkWidget *label1;
  GtkWidget *label2;
  GtkWidget *label3;

  window1 = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  /* gtk_window_set_transient_for(GTK_WINDOW(self->dlgClass), GTK_WINDOW(self->m_pApp->widgets.window)); */
  gtk_window_set_title (GTK_WINDOW (window1), _("Stack Trace"));
  gtk_window_set_wmclass (GTK_WINDOW (window1), "stack_trace", "Anjuta");
  gnome_window_icon_set_from_default(GTK_WINDOW(window1));

  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scrolledwindow1);
  gtk_container_add (GTK_CONTAINER (window1), scrolledwindow1);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  clist1 = gtk_clist_new (3);
  gtk_widget_show (clist1);
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), clist1);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 0, 17);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 1, 50);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 2, 80);
  gtk_clist_set_column_auto_resize (GTK_CLIST (clist1), 2, TRUE);
  gtk_clist_column_titles_show (GTK_CLIST (clist1));

  label1 = gtk_label_new ("");
  gtk_widget_show (label1);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 0, label1);

  label2 = gtk_label_new (_("Frame"));
  gtk_widget_show (label2);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 1, label2);

  label3 = gtk_label_new (_("Stack"));
  gtk_widget_show (label3);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 2, label3);

  gtk_signal_connect (GTK_OBJECT (window1), "delete_event",
                      GTK_SIGNAL_FUNC (on_stack_trace_window_delete_event),
                      st);
  gtk_signal_connect (GTK_OBJECT (clist1), "select_row",
                      GTK_SIGNAL_FUNC (on_stack_trace_clist_select_row),
                      st);
  gtk_signal_connect (GTK_OBJECT (clist1), "unselect_row",
                      GTK_SIGNAL_FUNC (on_stack_trace_clist_unselect_row),
                      st);
  gtk_signal_connect (GTK_OBJECT (window1), "event",
                      GTK_SIGNAL_FUNC (on_stack_trace_event),
                      st);

  st->widgets.window = window1;
  st->widgets.clist = clist1;
  st->widgets.menu = create_stack_menu();
  st->widgets.menu_set = stack_menu_uiinfo[0].widget;
  st->widgets.menu_info = stack_menu_uiinfo[1].widget; 
  st->widgets.menu_update = stack_menu_uiinfo[2].widget;
  st->widgets.menu_view = stack_menu_uiinfo[3].widget;

  gtk_widget_ref(st->widgets.window);
  gtk_widget_ref(st->widgets.clist);
  gtk_widget_ref(st->widgets.menu);
  gtk_widget_ref(st->widgets.menu_set);
  gtk_widget_ref(st->widgets.menu_info);
  gtk_widget_ref(st->widgets.menu_update);
  gtk_widget_ref(st->widgets.menu_view);
}
