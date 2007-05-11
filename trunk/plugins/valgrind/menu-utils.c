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

#include <gtk/gtk.h>
#include <string.h>

#include "menu-utils.h"


#if 0
static void
make_item (GtkMenu *menu, GtkMenuItem *item, const char *name, GtkWidget *pixmap)
{
	GtkWidget *label;
	
	if (*name == '\0')
		return;
	
	/*
	 * Ugh.  This needs to go into Gtk+
	 */
	label = gtk_label_new_with_mnemonic (name);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_widget_show (label);
	
	gtk_container_add (GTK_CONTAINER (item), label);
	
	if (pixmap && GTK_IS_IMAGE_MENU_ITEM (item)){
		gtk_widget_show (pixmap);
		gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), pixmap);
	}
}
#endif

void
menu_utils_construct_menu (GtkWidget *menu, struct _MenuItem *items, guint32 disable_mask, void *user_data)
{
	GtkWidget *item, *image;
	GSList *group = NULL;
	int i;
	
	for (i = 0; items[i].label; i++) {
		const char *label, *stock_id;
		
		label = items[i].label;
		stock_id = items[i].stock_id;
		
		if (items[i].is_stock) {
			item = gtk_image_menu_item_new_with_mnemonic (label);
			image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_MENU);
			gtk_widget_show (image);
			
			gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
			/*item = gtk_image_menu_item_new_from_stock (stock_id, NULL);*/
		} else if (strcmp (label, "") != 0) {
			if (items[i].is_toggle)
				item = gtk_check_menu_item_new_with_mnemonic (label);
			else if (items[i].is_radio)
				item = gtk_radio_menu_item_new_with_mnemonic (group, label);
			else if (items[i].stock_id)
				item = gtk_image_menu_item_new_with_mnemonic (label);
			else
				item = gtk_menu_item_new_with_mnemonic (label);
			
			if (items[i].is_toggle || items[i].is_radio)
				gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), items[i].is_active);
			if (items[i].is_radio)
				group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));
			if (items[i].stock_id) {
				if (stock_id[0] != '/')
					image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_MENU);
				else
					image = gtk_image_new_from_file (stock_id);
				
				gtk_widget_show (image);
				gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
			}
		} else {
			item = gtk_separator_menu_item_new ();
		}
		
		if (items[i].callback)
			g_signal_connect (item, "activate", items[i].callback, user_data);
		else if (strcmp (label, "") != 0) {
			/* unimplemented menu item? */
			gtk_widget_set_sensitive (item, FALSE);
		}
		
		if (items[i].disable_mask & disable_mask)
			gtk_widget_set_sensitive (item, FALSE);
		
		gtk_widget_show (item);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	}
}
