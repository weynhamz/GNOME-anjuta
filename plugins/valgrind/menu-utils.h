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


#ifndef __MENU_UTILS_H__
#define __MENU_UTILS_H__

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

struct _MenuItem {
	char *label;
	char *stock_id;
	guint is_stock : 1;
	guint is_toggle : 1;
	guint is_radio : 1;
	guint is_active : 1;
	
	GCallback callback;
	guint32 disable_mask;
};

#define MENU_ITEM_SEPARATOR  { "",   NULL, FALSE, FALSE, FALSE, FALSE, NULL, 0 }
#define MENU_ITEM_TERMINATOR { NULL, NULL, FALSE, FALSE, FALSE, FALSE, NULL, 0 }

void menu_utils_construct_menu (GtkWidget *menu, struct _MenuItem *items, guint32 disable_mask, void *user_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MENU_UTILS_H__ */
