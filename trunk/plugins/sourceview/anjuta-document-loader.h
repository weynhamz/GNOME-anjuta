/*
 * anjuta-document-loader.h
 * This file is part of anjuta
 *
 * Copyright (C) 2005 - Paolo Maggi 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA.
 */
 
/*
 * Modified by the anjuta Team, 2005. See the AUTHORS file for a 
 * list of people on the anjuta Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id$
 */

#ifndef __ANJUTA_DOCUMENT_LOADER_H__
#define __ANJUTA_DOCUMENT_LOADER_H__

#include "anjuta-document.h"
#include <libgnomevfs/gnome-vfs-file-size.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define ANJUTA_TYPE_DOCUMENT_LOADER              (anjuta_document_loader_get_type())
#define ANJUTA_DOCUMENT_LOADER(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), ANJUTA_TYPE_DOCUMENT_LOADER, AnjutaDocumentLoader))
#define ANJUTA_DOCUMENT_LOADER_CONST(obj)        (G_TYPE_CHECK_INSTANCE_CAST((obj), ANJUTA_TYPE_DOCUMENT_LOADER, AnjutaDocumentLoader const))
#define ANJUTA_DOCUMENT_LOADER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), ANJUTA_TYPE_DOCUMENT_LOADER, AnjutaDocumentLoaderClass))
#define ANJUTA_IS_DOCUMENT_LOADER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), ANJUTA_TYPE_DOCUMENT_LOADER))
#define ANJUTA_IS_DOCUMENT_LOADER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_DOCUMENT_LOADER))
#define ANJUTA_DOCUMENT_LOADER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), ANJUTA_TYPE_DOCUMENT_LOADER, AnjutaDocumentLoaderClass))

/* Private structure type */
typedef struct _AnjutaDocumentLoaderPrivate AnjutaDocumentLoaderPrivate;

/*
 * Main object structure
 */
typedef struct _AnjutaDocumentLoader AnjutaDocumentLoader;

struct _AnjutaDocumentLoader 
{
	GObject object;

	/*< private > */
	AnjutaDocumentLoaderPrivate *priv;
};

/*
 * Class definition
 */
typedef struct _AnjutaDocumentLoaderClass AnjutaDocumentLoaderClass;

struct _AnjutaDocumentLoaderClass 
{
	GObjectClass parent_class;

	void (* loading) (AnjutaDocumentLoader *loader,
			  gboolean             completed,
			  const GError        *error);
};

/*
 * Public methods
 */
GType 		 	 anjuta_document_loader_get_type		(void) G_GNUC_CONST;

AnjutaDocumentLoader 	*anjuta_document_loader_new 		(AnjutaDocument       *doc);

/* If enconding == NULL, the encoding will be autodetected */
void			 anjuta_document_loader_load		(AnjutaDocumentLoader *loader,
							 	 const gchar         *uri,
								 const AnjutaEncoding *encoding);
#if 0
gboolean		 anjuta_document_loader_load_from_stdin	(AnjutaDocumentLoader *loader);
#endif		 
gboolean		 anjuta_document_loader_cancel		(AnjutaDocumentLoader *loader);


/* Returns STDIN_URI if loading from stdin */
#define STDIN_URI "stdin:" 
const gchar		*anjuta_document_loader_get_uri		(AnjutaDocumentLoader *loader);

const gchar		*anjuta_document_loader_get_mime_type	(AnjutaDocumentLoader *loader);

time_t 			 anjuta_document_loader_get_mtime 	(AnjutaDocumentLoader *loader);

/* In the case the loader does not know if the file is readonly, for example for most
remote files, the function returns FALSE */
gboolean		 anjuta_document_loader_get_readonly	(AnjutaDocumentLoader *loader);

const AnjutaEncoding	*anjuta_document_loader_get_encoding	(AnjutaDocumentLoader *loader);

/* Returns 0 if file size is unknown */
GnomeVFSFileSize	 anjuta_document_loader_get_file_size	(AnjutaDocumentLoader *loader);									 

GnomeVFSFileSize	 anjuta_document_loader_get_bytes_read	(AnjutaDocumentLoader *loader);									 


G_END_DECLS

#endif  /* __ANJUTA_DOCUMENT_LOADER_H__  */
