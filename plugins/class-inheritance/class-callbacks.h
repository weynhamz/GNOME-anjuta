/*
 *  Copyright (C) Massimo Cora' 2005 <maxcvs@email.it>
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#ifndef _CLASS_CALLBACKS_H
#define _CLASS_CALLBACKS_H

#include "plugin.h"
#include "class-inherit.h"

gint on_canvas_event (GnomeCanvasItem *item, GdkEvent *event, gpointer data);
	
void on_toggled_menuitem_clicked (GtkCheckMenuItem *checkmenuitem,
							 gpointer data);

void on_member_menuitem_clicked (GtkMenuItem *menuitem, gpointer data);
	
gint on_nodedata_expanded_event (GnomeCanvasItem *item, GdkEvent *event, 
							gpointer data);

gint on_nodedata_event (GnomeCanvasItem *item, GdkEvent *event, gpointer data);

void on_update_menu_item_selected (GtkMenuItem *item, 
							AnjutaClassInheritance *plugin);

void on_style_set (GtkWidget *widget, GtkStyle  *previous_style,
			  AnjutaClassInheritance *plugin);


#endif /* _CLASS_CALLBACKS_H */
