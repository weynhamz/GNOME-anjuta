/*
 *	Copyright  2008	Ignacio Casal Quinteiro <nacho.resa@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef STARTER_H
#define STARTER_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/anjuta-shell.h>

#define ANJUTA_TYPE_STARTER         (starter_get_type ())
#define ANJUTA_STARTER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_STARTER, Starter))
#define ANJUTA_STARTER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), ANJUTA_TYPE_STARTER, StarterClass))
#define ANJUTA_IS_STARTER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_STARTER))
#define ANJUTA_IS_STARTER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_STARTER))
#define ANJUTA_STARTER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_STARTER, StarterClass))

typedef struct StarterPrivate StarterPrivate;

typedef struct {
	GtkScrolledWindow parent;
	StarterPrivate *priv;
} Starter;

typedef struct {
	GtkScrolledWindowClass parent_class;
	void (*update_ui);
	
} StarterClass;

GType starter_get_type (void);
Starter *starter_new (AnjutaShell *shell);

#endif /* STARTER_H */
