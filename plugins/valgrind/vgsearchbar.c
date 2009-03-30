/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Jeffrey Stedfast <fejj@ximian.com>
 *
 *  Copyright 2003 Ximian, Inc. (www.ximian.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>

#include "vgsearchbar.h"
#include "vgmarshal.h"

enum {
	SEARCH,
	CLEAR,
	LAST_SIGNAL
};

enum {
  COLUMN_STRING,
  COLUMN_INT,
  N_COLUMNS
};

static int search_bar_signals[LAST_SIGNAL] = { 0, };


static void vg_search_bar_class_init (VgSearchBarClass *klass);
static void vg_search_bar_init (VgSearchBar *bar);
static void vg_search_bar_destroy (GtkObject *obj);
static void vg_search_bar_finalize (GObject *obj);

static void search_bar_set_menu_items (VgSearchBar *bar, VgSearchBarItem *items);


static GtkHBoxClass *parent_class = NULL;


GType
vg_search_bar_get_type (void)
{
	static GType type = 0;
	
	if (!type) {
		static const GTypeInfo info = {
			sizeof (VgSearchBarClass),
			NULL, /* base_class_init */
			NULL, /* base_class_finalize */
			(GClassInitFunc) vg_search_bar_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (VgSearchBar),
			0,    /* n_preallocs */
			(GInstanceInitFunc) vg_search_bar_init,
		};
		
		type = g_type_register_static (GTK_TYPE_HBOX, "VgSearchBar", &info, 0);
	}
	
	return type;
}

static void
vg_search_bar_class_init (VgSearchBarClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkObjectClass *gtk_object_class = GTK_OBJECT_CLASS (klass);
	
	parent_class = g_type_class_ref (GTK_TYPE_VBOX);
	
	object_class->finalize = vg_search_bar_finalize;
	gtk_object_class->destroy = vg_search_bar_destroy;
	
	/* virtual methods */
	klass->set_menu_items = search_bar_set_menu_items;
	
	/* signals */
	search_bar_signals[SEARCH] =
		g_signal_new ("search",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (VgSearchBarClass, search),
			      NULL, NULL,
			      vg_marshal_NONE__INT,
			      G_TYPE_NONE, 1, G_TYPE_INT);
	
	search_bar_signals[CLEAR] =
		g_signal_new ("clear",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (VgSearchBarClass, clear),
			      NULL, NULL,
			      vg_marshal_NONE__NONE,
			      G_TYPE_NONE, 0);
}


static void
entry_activate (GtkEntry *entry, VgSearchBar *bar)
{
	g_signal_emit (bar, search_bar_signals[SEARCH], 0, bar->item_id);
}

static void
clear_clicked (GtkWidget *widget, VgSearchBar *bar)
{
	gtk_entry_set_text (bar->entry, "");
	gtk_combo_box_set_active (bar->menu, 0);
	g_signal_emit (bar, search_bar_signals[CLEAR], 0);
}

static void
vg_search_bar_init (VgSearchBar *bar)
{
	GtkWidget *widget;
	
	gtk_box_set_spacing (GTK_BOX(bar), 6);
	
	bar->item_id = -1;
	
	bar->menu = GTK_COMBO_BOX (widget = gtk_combo_box_new ());
	gtk_widget_show (widget);
	gtk_box_pack_start (GTK_BOX (bar), widget, FALSE, FALSE, 0);
	
	bar->entry = GTK_ENTRY (widget = gtk_entry_new ());
	gtk_widget_show (widget);
	gtk_box_pack_start (GTK_BOX (bar), widget, TRUE, TRUE, 0);
	g_signal_connect (bar->entry, "activate", G_CALLBACK (entry_activate), bar);
	
	bar->clear = widget = gtk_button_new_from_stock (GTK_STOCK_CLEAR);
	gtk_widget_show (widget);
	gtk_box_pack_start (GTK_BOX (bar), widget, FALSE, FALSE, 0);
	g_signal_connect (bar->clear, "clicked", G_CALLBACK (clear_clicked), bar);
}

static void
vg_search_bar_finalize (GObject *obj)
{
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
vg_search_bar_destroy (GtkObject *obj)
{
	GTK_OBJECT_CLASS (parent_class)->destroy (obj);
}


GtkWidget *
vg_search_bar_new (void)
{
	return g_object_new (VG_TYPE_SEARCH_BAR, NULL);
}


static void
item_activate (GtkComboBox *widget, VgSearchBar *bar)
{
	GtkTreeIter iter;

	gtk_combo_box_get_active_iter (widget, &iter);
	gtk_tree_model_get (gtk_combo_box_get_model (widget), &iter, 1, &bar->item_id, -1);
}

static void
search_bar_set_menu_items (VgSearchBar *bar, VgSearchBarItem *items)
{
	GtkTreeIter iter;
	GtkCellRenderer *cell;
	GtkListStore *list_store;
	int i;

	g_return_if_fail (VG_IS_SEARCH_BAR (bar));
	g_return_if_fail (items != NULL);

	list_store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_INT);

	bar->item_id = items->id;

	for (i = 0; items[i].label != NULL; i++) {
		gtk_list_store_append (list_store, &iter);
		gtk_list_store_set (list_store, &iter, COLUMN_STRING, _(items[i].label), COLUMN_INT, items[i].id, -1);
	}
	
	gtk_combo_box_set_model (bar->menu, GTK_TREE_MODEL (list_store));

	cell = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(bar->menu), cell, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(bar->menu), cell, "text", 0, NULL);

	g_signal_connect (bar->menu, "changed", G_CALLBACK (item_activate), bar);
	gtk_combo_box_set_active (bar->menu, 0);
}


void
vg_search_bar_set_menu_items (VgSearchBar *bar, VgSearchBarItem *items)
{
	g_return_if_fail (VG_IS_SEARCH_BAR (bar));
	g_return_if_fail (items != NULL);
	
	VG_SEARCH_BAR_GET_CLASS (bar)->set_menu_items (bar, items);
}

const char *
vg_search_bar_get_text (VgSearchBar *bar)
{
	g_return_val_if_fail (VG_IS_SEARCH_BAR (bar), NULL);
	
	return gtk_entry_get_text (bar->entry);
}
