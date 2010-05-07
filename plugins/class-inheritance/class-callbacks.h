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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
 
#ifndef _CLASS_CALLBACKS_H
#define _CLASS_CALLBACKS_H

#include "plugin.h"
#include "class-inherit.h"

gint on_canvas_event (GnomeCanvasItem *item, GdkEvent *event, gpointer data);
gint on_canvas_event_proxy (GnomeCanvasItem *item, GdkEvent *event,
                            GnomeCanvasItem *proxy_item);

void on_update_menu_item_selected (GtkMenuItem *item, 
                                   AnjutaClassInheritance *plugin);
void on_style_set (GtkWidget *widget, GtkStyle  *previous_style,
                   AnjutaClassInheritance *plugin);


/* Callbacks for expanded class node */
gint on_expanded_class_title_event (GnomeCanvasItem *item, GdkEvent *event,
                                    ClsNode *cls_node);
gint on_expanded_class_item_event (GnomeCanvasItem *item, GdkEvent *event,
                                   gpointer data);
gint on_expanded_class_more_event (GnomeCanvasItem *item, GdkEvent *event,
                                   ClsNode *cls_node);

/* Callback for collapsed class node */
gint on_collapsed_class_event (GnomeCanvasItem *item, GdkEvent *event,
                                        gpointer data);

#endif /* _CLASS_CALLBACKS_H */
