/*
    sharedlibs_gui.c
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
#include "sharedlib.h"
#include "sharedlib_cbs.h"
#include "resources.h"

enum {
	COLUMN_SHARED_LIB,
	COLUMN_FROM,
	COLUMN_TO,
	COLUMN_SYSM_READ,
	COLUMNS_NB
};

static const gchar *column_names[COLUMNS_NB] = {
	N_("Shared Object"), N_("From"), N_("To"), N_("Sysm Read")
};

GtkWidget* create_sharedlibs_menu (void);

static GnomeUIInfo sharedlibs_menu_uiinfo[] =
{
  {
    GNOME_APP_UI_ITEM, N_("Update"),
    NULL,
    on_sharedlibs_update_activate, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  GNOMEUIINFO_SEPARATOR,
  {
    GNOME_APP_UI_ITEM, N_("Help"),
    NULL,
    on_sharedlibs_help_activate, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  GNOMEUIINFO_END
};

GtkWidget*
create_sharedlibs_menu ()
{
  GtkWidget *sharedlibs_menu;

  sharedlibs_menu = gtk_menu_new ();
  gnome_app_fill_menu (GTK_MENU_SHELL (sharedlibs_menu), sharedlibs_menu_uiinfo,
                       NULL, FALSE, 0);
  return sharedlibs_menu;
}

void
create_sharedlibs_gui(Sharedlibs *sl)
{
	GladeXML *gxml;
	GtkWidget *topwindow;
	GtkTreeView *view;
	GtkListStore *store;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	guint i;

	gxml = glade_xml_new (GLADE_FILE_ANJUTA, "window.debugger.sharedlibs",
				NULL);
	topwindow = glade_xml_get_widget (gxml,
				"window.debugger.sharedlibs");
	view = GTK_TREE_VIEW (glade_xml_get_widget (gxml,
				"debugger.sharedlibs.tv"));
	g_object_unref (gxml);
	
	gtk_dialog_add_button (GTK_DIALOG (topwindow), GTK_STOCK_CLOSE,
						   GTK_RESPONSE_CLOSE);

	// top level window
	gtk_window_set_transient_for(GTK_WINDOW (topwindow),
				GTK_WINDOW (app->widgets.window));
	gtk_window_set_title (GTK_WINDOW (topwindow), _("Shared libraries"));
	gtk_window_set_role (GTK_WINDOW (topwindow), "sharedlibs");
	gnome_window_icon_set_from_default (GTK_WINDOW (topwindow));

	// treeview
	store = gtk_list_store_new (COLUMNS_NB,
								G_TYPE_STRING,
								G_TYPE_STRING,
								G_TYPE_STRING,
								G_TYPE_STRING);
	gtk_tree_view_set_model (view, GTK_TREE_MODEL (store));
	g_object_unref (G_OBJECT (store));
	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (view),
				GTK_SELECTION_BROWSE);
	gtk_tree_view_set_search_column (view, COLUMN_SHARED_LIB);

	renderer = gtk_cell_renderer_text_new ();
	for (i = 0; i < COLUMNS_NB; i++)
	{
		column = gtk_tree_view_column_new_with_attributes (column_names[i],
					renderer, "text", i, NULL);
		gtk_tree_view_column_set_sort_column_id (column, i);
		gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
		gtk_tree_view_append_column (view, column);
	}
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
				COLUMN_SHARED_LIB, GTK_SORT_ASCENDING);
	gtk_object_unref (GTK_OBJECT (renderer));

	// signals
	gtk_signal_connect (GTK_OBJECT (topwindow), "delete_event",
				GTK_SIGNAL_FUNC (on_sharedlibs_delete_event), sl);
	gtk_signal_connect (GTK_OBJECT (topwindow), "close",
				GTK_SIGNAL_FUNC (on_sharedlibs_close), sl);
	gtk_signal_connect (GTK_OBJECT (topwindow), "response",
				GTK_SIGNAL_FUNC (on_sharedlibs_response), sl);
	gtk_signal_connect (GTK_OBJECT (view), "event",
				GTK_SIGNAL_FUNC (on_sharedlibs_event), sl);

	// other stuff
	gtk_window_add_accel_group (GTK_WINDOW (topwindow), app->accel_group);

	sl->widgets.window = topwindow;
	sl->widgets.view = GTK_WIDGET (view);
	sl->widgets.menu = create_sharedlibs_menu ();
	sl->widgets.menu_update = sharedlibs_menu_uiinfo[0].widget;
	
	gtk_widget_ref (sl->widgets.window);
	gtk_widget_ref (sl->widgets.view);
	gtk_widget_ref (sl->widgets.menu);
	gtk_widget_ref (sl->widgets.menu_update);
}

/* this function has to be defined here in order to be able to access
   the COLUMNS... enum */
void
sharedlibs_update_cb(GList *lines, gpointer data)
{
	Sharedlibs *sl;
	GtkTreeModel *model;
	GtkListStore *store;
	GList *list, *node;
	gint count;
	gchar obj[512], from[32], to[32], read[32];
	GtkTreeIter iter;

    list = remove_blank_lines (lines);
    if (g_list_length (list) >= 2)
	{
		sl = (Sharedlibs *) data;
		model = gtk_tree_view_get_model (GTK_TREE_VIEW (sl->widgets.view));
		store = GTK_LIST_STORE (model);

		sharedlibs_clear (sl);
		node = list->next;
		while (node)
		{
			count = sscanf ((char *) node->data, "%s %s %s %s", from, to,
					read, obj);
			node = g_list_next (node);
			if (count != 4) continue;
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter,
						COLUMN_SHARED_LIB, extract_filename(obj),
						COLUMN_FROM, from,
						COLUMN_TO, to,
						COLUMN_SYSM_READ, read,
						-1);
		}
	}
    g_list_free (list);
}
