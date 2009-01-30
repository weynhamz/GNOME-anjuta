/***************************************************************************
 *            sourceview.h
 *
 *  Do Dez 29 00:50:15 2005
 *  Copyright  2005  Johannes Schmid
 *  jhs@gnome.org
 ***************************************************************************/

/*
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

#ifndef SOURCEVIEW_H
#define SOURCEVIEW_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-status.h>
#include <libanjuta/anjuta-plugin.h>
#include <gio/gio.h>

#define ANJUTA_TYPE_SOURCEVIEW         (sourceview_get_type ())
#define ANJUTA_SOURCEVIEW(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_SOURCEVIEW, Sourceview))
#define ANJUTA_SOURCEVIEW_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), ANJUTA_TYPE_SOURCEVIEW, SourceviewClass))
#define ANJUTA_IS_SOURCEVIEW(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_SOURCEVIEW))
#define ANJUTA_IS_SOURCEVIEW_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_SOURCEVIEW))
#define ANJUTA_SOURCEVIEW_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_SOURCEVIEW, SourceviewClass))

typedef struct SourceviewPrivate SourceviewPrivate;

typedef struct {
	GtkVBox parent;
	SourceviewPrivate *priv;
} Sourceview;

typedef struct {
	GtkVBoxClass parent_class;
	void (*update_ui);
	
} SourceviewClass;

GType sourceview_get_type(void);
Sourceview *sourceview_new(GFile* file, const gchar* filename, AnjutaPlugin* plugin);

#endif /* SOURCEVIEW_H */
