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


#ifndef __VG_SEARCH_BAR_H__
#define __VG_SEARCH_BAR_H__

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

#define VG_TYPE_SEARCH_BAR            (vg_search_bar_get_type ())
#define VG_SEARCH_BAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), VG_TYPE_SEARCH_BAR, VgSearchBar))
#define VG_SEARCH_BAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), VG_TYPE_SEARCH_BAR, VgSearchBarClass))
#define VG_IS_SEARCH_BAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VG_TYPE_SEARCH_BAR))
#define VG_IS_SEARCH_BAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), VG_TYPE_SEARCH_BAR))
#define VG_SEARCH_BAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), VG_TYPE_SEARCH_BAR, VgSearchBarClass))

typedef struct _VgSearchBar VgSearchBar;
typedef struct _VgSearchBarClass VgSearchBarClass;

typedef struct _VgSearchBarItem {
	const char *label;
	int id;
} VgSearchBarItem;

struct _VgSearchBar {
	GtkHBox parent_object;
	
	GtkComboBox *menu;
	GtkEntry *entry;
	GtkWidget *clear;
	
	int item_id;
};

struct _VgSearchBarClass {
	GtkHBoxClass parent_class;
	
	/* virtual methods */
	void (* set_menu_items) (VgSearchBar *bar, VgSearchBarItem *items);
	
	/* signals */
	void (* search)         (VgSearchBar *bar, int item_id);
	void (* clear)          (VgSearchBar *bar);
};


GType vg_search_bar_get_type (void);

GtkWidget *vg_search_bar_new (void);

void vg_search_bar_set_menu_items (VgSearchBar *bar, VgSearchBarItem *items);

const char *vg_search_bar_get_text (VgSearchBar *bar);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VG_SEARCH_BAR_H__ */
