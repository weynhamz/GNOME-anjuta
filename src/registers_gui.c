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
#include <libgnomeui/gnome-window-icon.h>

#include "anjuta.h"
#include "registers.h"
#include "registers_cbs.h"
#include "resources.h"

enum {
	COLUMN_REGS,
	COLUMN_HEX,
	COLUMN_DEC,
	COLUMNS_NB
};

static const gchar *column_names[COLUMNS_NB] = {
	N_("Register"), N_("Hex"), N_("Decimal")
};

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
	GladeXML *gxml;
	GtkWidget *topwindow;
	GtkTreeView *view;
	GtkListStore *store;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	guint i;

	gxml = glade_xml_new (GLADE_FILE_ANJUTA, "window.debugger.registers",
				NULL);
	topwindow = glade_xml_get_widget (gxml, "window.debugger.registers");
	view = GTK_TREE_VIEW (glade_xml_get_widget (gxml,
				"debugger.registers.tv"));
	g_object_unref (gxml);

	// top level window
	gtk_window_set_transient_for(GTK_WINDOW (topwindow),
				GTK_WINDOW (app->widgets.window));
	gtk_window_set_title (GTK_WINDOW (topwindow), _("CPU Registers"));
	gtk_window_set_role (GTK_WINDOW (topwindow), "CPUregisters");
	gnome_window_icon_set_from_default (GTK_WINDOW (topwindow));

	// treeview
	store = gtk_list_store_new (COLUMNS_NB,
								G_TYPE_STRING,
								G_TYPE_STRING,
								G_TYPE_STRING);
	gtk_tree_view_set_model (view, GTK_TREE_MODEL (store));
	g_object_unref (G_OBJECT (store));
	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (view),
				GTK_SELECTION_BROWSE);
	gtk_tree_view_set_search_column (view, COLUMN_REGS);
	gtk_tree_view_set_headers_clickable (view, FALSE);

	renderer = gtk_cell_renderer_text_new ();
	for (i = 0; i < COLUMNS_NB; i++)
	{
		column = gtk_tree_view_column_new_with_attributes (column_names[i],
					renderer, "text", i, NULL);
//		gtk_tree_view_column_set_sort_column_id (column, i);
		gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
		gtk_tree_view_append_column (view, column);
	}
//	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
//				COLUMN_REGS, GTK_SORT_ASCENDING);
	gtk_object_unref (GTK_OBJECT (renderer));

	// signals	
	gtk_signal_connect (GTK_OBJECT (view), "select_row",
				GTK_SIGNAL_FUNC (on_registers_clist_select_row), cr);
	gtk_signal_connect (GTK_OBJECT (topwindow), "delete_event",
				GTK_SIGNAL_FUNC (on_registers_delete_event), cr);
	gtk_signal_connect (GTK_OBJECT (view), "event",
				GTK_SIGNAL_FUNC (on_register_event), cr);

	// other stuff
	gtk_window_add_accel_group (GTK_WINDOW (topwindow), app->accel_group);

	cr->widgets.window = topwindow;
	cr->widgets.view = GTK_WIDGET (view);
	cr->widgets.menu = create_register_menu ();
	cr->widgets.menu_modify = register_menu_uiinfo[0].widget;
	cr->widgets.menu_update = register_menu_uiinfo[1].widget;
	
	gtk_widget_ref (cr->widgets.window);
	gtk_widget_ref (cr->widgets.view);
	gtk_widget_ref (cr->widgets.menu);
	gtk_widget_ref (cr->widgets.menu_modify);
	gtk_widget_ref (cr->widgets.menu_update);
}

void
cpu_registers_update_cb (GList *lines, gpointer data)
{
    CpuRegisters *ew;
	GtkTreeModel *model;
	GtkListStore *store;
    gchar reg[10], hex[32], dec[32];
    gint count;
    GList *node, *list;
	GtkTreeIter iter;

    list = remove_blank_lines(lines);
    if (g_list_length (list) >= 2)
	{
		ew = (CpuRegisters *) data;
		model = gtk_tree_view_get_model (GTK_TREE_VIEW (ew->widgets.view));
		store = GTK_LIST_STORE (model);

		cpu_registers_clear (ew);
		node = list->next;
		while (node)
		{
			count = sscanf ((char *) node->data, "%s %s %s", reg, hex, dec);
			node = g_list_next (node);
			if (count != 3) continue;
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter,
						COLUMN_REGS, reg,
						COLUMN_HEX, hex,
						COLUMN_DEC, dec,
						-1);
		}
	}
	g_list_free(list);
}
