/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Johannes Schmid 2007 <jhs@gnome.org>
 * 
 * anjuta is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * anjuta is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with anjuta.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef _ANJUTA_GLADE_NOTEBOOK_H_
#define _ANJUTA_GLADE_NOTEBOOK_H_

#include <glib-object.h>
#include <gtk/gtknotebook.h>
#include "plugin.h"

G_BEGIN_DECLS

#define ANJUTA_TYPE_GLADE_NOTEBOOK             (anjuta_glade_notebook_get_type ())
#define ANJUTA_GLADE_NOTEBOOK(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_GLADE_NOTEBOOK, AnjutaGladeNotebook))
#define ANJUTA_GLADE_NOTEBOOK_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_GLADE_NOTEBOOK, AnjutaGladeNotebookClass))
#define ANJUTA_IS_GLADE_NOTEBOOK(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_GLADE_NOTEBOOK))
#define ANJUTA_IS_GLADE_NOTEBOOK_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_GLADE_NOTEBOOK))
#define ANJUTA_GLADE_NOTEBOOK_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_GLADE_NOTEBOOK, AnjutaGladeNotebookClass))

typedef struct _AnjutaGladeNotebookClass AnjutaGladeNotebookClass;
typedef struct _AnjutaGladeNotebook AnjutaGladeNotebook;
typedef struct _AnjutaGladeNotebookPrivate AnjutaGladeNotebookPrivate;

struct _AnjutaGladeNotebookClass
{
	GtkNotebookClass parent_class;
};

struct _AnjutaGladeNotebook
{
	GtkNotebook parent_instance;
};

GType anjuta_glade_notebook_get_type (void) G_GNUC_CONST;
GtkWidget* 
anjuta_glade_notebook_new (GladePlugin* glade_plugin);

G_END_DECLS

#endif /* _ANJUTA_GLADE_NOTEBOOK_H_ */
