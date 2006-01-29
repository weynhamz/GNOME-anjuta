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
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgnome/gnome-i18n.h>

#include "vgsearchbar.h"
#include "vgmarshal.h"

enum {
	SEARCH,
	CLEAR,
	LAST_SIGNAL
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
	gtk_option_menu_set_history (bar->menu, 0);
	g_signal_emit (bar, search_bar_signals[CLEAR], 0);
}

static void
vg_search_bar_init (VgSearchBar *bar)
{
	GtkWidget *widget;
	
	gtk_box_set_spacing ((GtkBox *) bar, 6);
	
	bar->item_id = -1;
	
	bar->menu = (GtkOptionMenu *) (widget = gtk_option_menu_new ());
	gtk_widget_show (widget);
	gtk_box_pack_start ((GtkBox *) bar, widget, FALSE, FALSE, 0);
	
	bar->entry = (GtkEntry *) (widget = gtk_entry_new ());
	gtk_widget_show (widget);
	gtk_box_pack_start ((GtkBox *) bar, widget, TRUE, TRUE, 0);
	g_signal_connect (bar->entry, "activate", G_CALLBACK (entry_activate), bar);
	
	bar->clear = widget = gtk_button_new_from_stock (GTK_STOCK_CLEAR);
	gtk_widget_show (widget);
	gtk_box_pack_start ((GtkBox *) bar, widget, FALSE, FALSE, 0);
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
item_activate (GtkMenuItem *item, VgSearchBar *bar)
{
	bar->item_id = GPOINTER_TO_INT (g_object_get_data ((GObject *) item, "item_id"));
}

static void
search_bar_set_menu_items (VgSearchBar *bar, VgSearchBarItem *items)
{
	GtkWidget *menu, *item;
	int i;
	
	g_return_if_fail (VG_IS_SEARCH_BAR (bar));
	g_return_if_fail (items != NULL);
	
	menu = gtk_menu_new ();
	bar->item_id = items->id;
	
	for (i = 0; items[i].label != NULL; i++) {
		item = gtk_menu_item_new_with_label (_(items[i].label));
		g_object_set_data ((GObject *) item, "item_id", GINT_TO_POINTER (items[i].id));
		g_signal_connect (item, "activate", G_CALLBACK (item_activate), bar);
		gtk_widget_show (item);
		
		gtk_menu_shell_append ((GtkMenuShell *) menu, item);
	}
	
	gtk_option_menu_set_menu (bar->menu, menu);
	gtk_option_menu_set_history (bar->menu, 0);
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
