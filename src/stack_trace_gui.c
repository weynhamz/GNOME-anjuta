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
    GNOME_APP_UI_ITEM, N_("Frame args"),
    NULL,
    on_stack_frame_args_activate, NULL, NULL,
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

static GtkWidget* create_stack_menu (StackTrace* st);


void
create_stack_trace_gui(StackTrace *st)
{
  GtkTreeModel* model;
  GtkTreeSelection *selection;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;	
	
  model = GTK_TREE_MODEL(gtk_list_store_new (STACK_TRACE_N_COLUMNS,
											GDK_TYPE_PIXBUF,
											GTK_TYPE_STRING,
											GTK_TYPE_STRING));

  st->widgets.clist = gtk_tree_view_new_with_model (model);
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (st->widgets.clist));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_object_unref (G_OBJECT (model));

  /* Columns */
  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_title (column, _("Active"));
  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_add_attribute (column, renderer, "pixbuf", STACK_TRACE_ACTIVE_COLUMN);
  gtk_tree_view_append_column (GTK_TREE_VIEW (st->widgets.clist), column);
  gtk_tree_view_set_expander_column (GTK_TREE_VIEW (st->widgets.clist), column);

  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_title (column, _("Count"));
  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_add_attribute (column, renderer, "text", STACK_TRACE_COUNT_COLUMN);
  gtk_tree_view_append_column (GTK_TREE_VIEW (st->widgets.clist), column);
  gtk_tree_view_set_expander_column (GTK_TREE_VIEW (st->widgets.clist), column);

  column = gtk_tree_view_column_new ();
  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_add_attribute (column, renderer, "text", STACK_TRACE_FRAME_COLUMN);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_title (column, _("Frame"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (st->widgets.clist), column);

  g_signal_connect (st->widgets.clist, "event", G_CALLBACK (on_stack_trace_event), st);  

  st->widgets.menu = create_stack_menu(st);
  st->widgets.menu_set = stack_menu_uiinfo[0].widget;
  st->widgets.menu_info = stack_menu_uiinfo[1].widget; 
  st->widgets.menu_update = stack_menu_uiinfo[2].widget;
  st->widgets.menu_view = stack_menu_uiinfo[3].widget;

  gtk_widget_ref(st->widgets.clist);
  gtk_widget_ref(st->widgets.menu);
  gtk_widget_ref(st->widgets.menu_set);
  gtk_widget_ref(st->widgets.menu_info);
  gtk_widget_ref(st->widgets.menu_update);
  gtk_widget_ref(st->widgets.menu_view);
}


static GtkWidget*
create_stack_menu (StackTrace *st)
{
  GtkWidget *menu4;
  int i;
  int entries = sizeof (stack_menu_uiinfo) / sizeof (GnomeUIInfo);
	
	/* set all user data in the stack trace menu to the StackTrace struct parameter */
  for (i = 0; i < entries; i++)
  	stack_menu_uiinfo[i].user_data = st;

  menu4 = gtk_menu_new ();
  gnome_app_fill_menu (GTK_MENU_SHELL (menu4), stack_menu_uiinfo, NULL, FALSE, 0);
  return menu4;
}
