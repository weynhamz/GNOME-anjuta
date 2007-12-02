/*
 * anjuta-document-saver.h
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA 02110-1301, USA.
 */
 
/*
 * Modified by the anjuta Team, 2005. See the AUTHORS file for a 
 * list of people on the anjuta Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id$
 */

#ifndef __ANJUTA_DOCUMENT_SAVER_H__
#define __ANJUTA_DOCUMENT_SAVER_H__

#include "anjuta-document.h"
#include <libgnomevfs/gnome-vfs-file-size.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define ANJUTA_TYPE_DOCUMENT_SAVER              (anjuta_document_saver_get_type())
#define ANJUTA_DOCUMENT_SAVER(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), ANJUTA_TYPE_DOCUMENT_SAVER, AnjutaDocumentSaver))
#define ANJUTA_DOCUMENT_SAVER_CONST(obj)        (G_TYPE_CHECK_INSTANCE_CAST((obj), ANJUTA_TYPE_DOCUMENT_SAVER, AnjutaDocumentSaver const))
#define ANJUTA_DOCUMENT_SAVER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), ANJUTA_TYPE_DOCUMENT_SAVER, AnjutaDocumentSaverClass))
#define ANJUTA_IS_DOCUMENT_SAVER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), ANJUTA_TYPE_DOCUMENT_SAVER))
#define ANJUTA_IS_DOCUMENT_SAVER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_DOCUMENT_SAVER))
#define ANJUTA_DOCUMENT_SAVER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), ANJUTA_TYPE_DOCUMENT_SAVER, AnjutaDocumentSaverClass))

/* Private structure type */
typedef struct _AnjutaDocumentSaverPrivate AnjutaDocumentSaverPrivate;

/*
 * Main object structure
 */
typedef struct _AnjutaDocumentSaver AnjutaDocumentSaver;

struct _AnjutaDocumentSaver 
{
	GObject object;

	/*< private > */
	AnjutaDocumentSaverPrivate *priv;
};

/*
 * Class definition
 */
typedef struct _AnjutaDocumentSaverClass AnjutaDocumentSaverClass;

struct _AnjutaDocumentSaverClass 
{
	GObjectClass parent_class;

	void (* saving) (AnjutaDocumentSaver *saver,
			 gboolean             completed,
			 const GError        *error);
};

/*
 * Public methods
 */
GType 		 	 anjuta_document_saver_get_type		(void) G_GNUC_CONST;

AnjutaDocumentSaver 	*anjuta_document_saver_new 		(AnjutaDocument        *doc);

/* If enconding == NULL, the encoding will be autodetected */
void			 anjuta_document_saver_save		(AnjutaDocumentSaver  *saver,
							 	 const gchar         *uri,
								 const AnjutaEncoding *encoding,
								 time_t               oldmtime,
								 AnjutaDocumentSaveFlags flags);

#if 0
void			 anjuta_document_saver_cancel		(AnjutaDocumentSaver  *saver);
#endif

const gchar		*anjuta_document_saver_get_uri		(AnjutaDocumentSaver  *saver);

/* If backup_uri is NULL no backup will be made */
const gchar		*anjuta_document_saver_get_backup_uri	(AnjutaDocumentSaver  *saver);
void			*anjuta_document_saver_set_backup_uri	(AnjutaDocumentSaver  *saver,
							 	 const gchar         *backup_uri);

const gchar		*anjuta_document_saver_get_mime_type	(AnjutaDocumentSaver  *saver);

time_t			 anjuta_document_saver_get_mtime		(AnjutaDocumentSaver  *saver);

/* Returns 0 if file size is unknown */
GnomeVFSFileSize	 anjuta_document_saver_get_file_size	(AnjutaDocumentSaver  *saver);									 

GnomeVFSFileSize	 anjuta_document_saver_get_bytes_written	(AnjutaDocumentSaver  *saver);									 


G_END_DECLS

#endif  /* __ANJUTA_DOCUMENT_SAVER_H__  */
