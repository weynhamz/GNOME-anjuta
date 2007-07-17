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

#ifndef _ANJUTA_DESIGN_DOCUMENT_H_
#define _ANJUTA_DESIGN_DOCUMENT_H_

#include <glib-object.h>
#include <gladeui/glade-design-view.h>
#include "plugin.h"

G_BEGIN_DECLS

#define ANJUTA_TYPE_DESIGN_DOCUMENT             (anjuta_design_document_get_type ())
#define ANJUTA_DESIGN_DOCUMENT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_DESIGN_DOCUMENT, AnjutaDesignDocument))
#define ANJUTA_DESIGN_DOCUMENT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_DESIGN_DOCUMENT, AnjutaDesignDocumentClass))
#define ANJUTA_IS_DESIGN_DOCUMENT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_DESIGN_DOCUMENT))
#define ANJUTA_IS_DESIGN_DOCUMENT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_DESIGN_DOCUMENT))
#define ANJUTA_DESIGN_DOCUMENT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_DESIGN_DOCUMENT, AnjutaDesignDocumentClass))

typedef struct _AnjutaDesignDocumentClass AnjutaDesignDocumentClass;
typedef struct _AnjutaDesignDocument AnjutaDesignDocument;
typedef struct _AnjutaDesignDocumentPrivate AnjutaDesignDocumentPrivate;

struct _AnjutaDesignDocumentClass
{
	GladeDesignViewClass parent_class;
};

struct _AnjutaDesignDocument
{
	GladeDesignView parent_instance;
};

GType anjuta_design_document_get_type (void) G_GNUC_CONST;
GtkWidget* 
anjuta_design_document_new (GladePlugin* glade_plugin, GladeProject* project);

G_END_DECLS

#endif /* _ANJUTA_DESIGN_DOCUMENT_H_ */
