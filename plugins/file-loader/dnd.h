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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

#include <gtk/gtk.h>
#include <glib.h>
#include <gdk/gdk.h>

#ifndef _DND_H_
#define _DND_H_

void dnd_drop_init (GtkWidget *widget,
	 				void (* data_dropped) (GFile *uri, gpointer data),
					gpointer user_data);

void dnd_drop_finalize (GtkWidget *widget, gpointer user_data);

#endif	/* _DND_H_ */
