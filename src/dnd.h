/*
 * dnd.h - Header file for dnd.c.
 *
 * Copyright (C) 2000 José Antonio Caminero Granja
 *
 * Author(s): 
 * 	José Antonio Caminero Granja <JCamGra@alumnos.uva.es>
 *  Archit Baweja <bighead@crosswinds.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#include <gnome.h>

#ifndef _DND_H_
#define _DND_H_

BEGIN_GNOME_DECLS

/*
 * Maximum number of mime types that the widget can handle droppings for.
 */
#define DND_MAX_MIME_TYPES 20

/*
 * dnd_drop_init () - Initialize a widget to accept "droppings".
 *	
 * - GtkWidget *widget
 * 	Widget which will respond to the drags a.k.a "droppings".
 * - void (* data_dropped) (gchar *file_name, gpointer data)
 *	A pointer to the user supplied function which will handle the
 *      "droppings" (file_name).
 * - gpointer data
 *      Any user supplied data. This will be passed on to the callback. 
 * - ... [char *mime_type]
 *	NULL terminated list of accepted mime types.
 */
void
dnd_drop_init (GtkWidget *widget,
	       void (* data_dropped) (gchar *filename, gpointer data),
	       gpointer data, ...);

END_GNOME_DECLS

#endif	/* _DND_H_ */
