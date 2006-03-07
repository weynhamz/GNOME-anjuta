/*
 * anjuta-document.h
 * This file is part of anjuta
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi 
 * Copyright (C) 2002-2005 Paolo Maggi 
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
 * Modified by the anjuta Team, 1998-2005. See the AUTHORS file for a 
 * list of people on the anjuta Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id$
 */
 
#ifndef __ANJUTA_DOCUMENT_H__
#define __ANJUTA_DOCUMENT_H__

#include <gtk/gtk.h>
#include <gtksourceview/gtksourcebuffer.h>
#include <libgnomevfs/gnome-vfs.h>

#include <libanjuta/anjuta-encodings.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define ANJUTA_TYPE_DOCUMENT              (anjuta_document_get_type())
#define ANJUTA_DOCUMENT(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), ANJUTA_TYPE_DOCUMENT, AnjutaDocument))
#define ANJUTA_DOCUMENT_CONST(obj)        (G_TYPE_CHECK_INSTANCE_CAST((obj), ANJUTA_TYPE_DOCUMENT, AnjutaDocument const))
#define ANJUTA_DOCUMENT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), ANJUTA_TYPE_DOCUMENT, AnjutaDocumentClass))
#define ANJUTA_IS_DOCUMENT(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), ANJUTA_TYPE_DOCUMENT))
#define ANJUTA_IS_DOCUMENT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_DOCUMENT))
#define ANJUTA_DOCUMENT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), ANJUTA_TYPE_DOCUMENT, AnjutaDocumentClass))

/* Private structure type */
typedef struct _AnjutaDocumentPrivate    AnjutaDocumentPrivate;

/*
 * Main object structure
 */
typedef struct _AnjutaDocument           AnjutaDocument;
 
struct _AnjutaDocument
{
	GtkSourceBuffer buffer;
	
	/*< private > */
	AnjutaDocumentPrivate *priv;
};

/*
 * Class definition
 */
typedef struct _AnjutaDocumentClass 	AnjutaDocumentClass;

struct _AnjutaDocumentClass
{
	GtkSourceBufferClass parent_class;

	/* Signals */ // CHECK: ancora da rivedere

	void (* cursor_moved)		(AnjutaDocument    *document);

	/* Document load */
	void (* loading)		(AnjutaDocument    *document,
					 GnomeVFSFileSize  size,
					 GnomeVFSFileSize  total_size);

	void (* loaded)			(AnjutaDocument    *document,
					 const GError     *error);

	/* Document save */
	void (* saving)			(AnjutaDocument    *document,
					 GnomeVFSFileSize  size,
					 GnomeVFSFileSize  total_size);

	void (* saved)  		(AnjutaDocument    *document,
					 const GError     *error);

};


typedef enum
{
	/* save file despite external modifications */
	ANJUTA_DOCUMENT_SAVE_IGNORE_MTIME 	= 1 << 0,

	/* write the file directly without attempting to backup */
	ANJUTA_DOCUMENT_SAVE_IGNORE_BACKUP	= 1 << 1,
	
	/* preserve previous backup file, needed to support autosaving */
	ANJUTA_DOCUMENT_SAVE_PRESERVE_BACKUP	= 1 << 2
} AnjutaDocumentSaveFlags;


#define ANJUTA_DOCUMENT_ERROR anjuta_document_error_quark ()

enum
{
	/* start at GNOME_VFS_NUM_ERRORS since we use GnomeVFSResult 
	 * for the error codes */ 
	ANJUTA_DOCUMENT_ERROR_EXTERNALLY_MODIFIED = GNOME_VFS_NUM_ERRORS,
	ANJUTA_DOCUMENT_ERROR_NOT_REGULAR_FILE,
	ANJUTA_DOCUMENT_ERROR_CANT_CREATE_BACKUP,
	ANJUTA_DOCUMENT_NUM_ERRORS 
};

GQuark		 anjuta_document_error_quark	(void);

GType		 anjuta_document_get_type      	(void) G_GNUC_CONST;

AnjutaDocument   *anjuta_document_new 		(void);

gchar		*anjuta_document_get_uri 	(AnjutaDocument       *doc);

gchar		*anjuta_document_get_uri_for_display
						(AnjutaDocument       *doc);
gchar		*anjuta_document_get_short_name_for_display
					 	(AnjutaDocument       *doc);

gchar		*anjuta_document_get_mime_type 	(AnjutaDocument       *doc);

gboolean	 anjuta_document_get_readonly 	(AnjutaDocument       *doc);

void		 anjuta_document_load 		(AnjutaDocument       *doc,
						 const gchar         *uri,
						 const AnjutaEncoding *encoding,
						 gint                 line_pos,
						 gboolean             create); 

gboolean	 anjuta_document_insert_file	(AnjutaDocument       *doc,
						 GtkTextIter         *iter, 
						 const gchar         *uri, 
						 const AnjutaEncoding *encoding);

gboolean	 anjuta_document_load_cancel	(AnjutaDocument       *doc);

void		 anjuta_document_save 		(AnjutaDocument       *doc,
						 AnjutaDocumentSaveFlags flags);

void		 anjuta_document_save_as 	(AnjutaDocument       *doc,	
						 const gchar         *uri, 
						 const AnjutaEncoding *encoding,
						 AnjutaDocumentSaveFlags flags);

gboolean	 anjuta_document_is_untouched 	(AnjutaDocument       *doc);

gboolean	 anjuta_document_get_deleted	(AnjutaDocument       *doc);
/* Ancora da discutere
gboolean	 anjuta_document_get_externally_modified
						(AnjutaDocument       *doc);
*/

gboolean	 anjuta_document_goto_line 	(AnjutaDocument       *doc, 
						 gint                 line);

void 		 anjuta_document_set_language 	(AnjutaDocument       *doc,
						 GtkSourceLanguage   *lang);
GtkSourceLanguage 
		*anjuta_document_get_language 	(AnjutaDocument       *doc);

const AnjutaEncoding 
		*anjuta_document_get_encoding	(AnjutaDocument       *doc);

void		 anjuta_document_set_enable_search_highlighting 
						(AnjutaDocument       *doc,
						 gboolean             enable);

gboolean	 anjuta_document_get_enable_search_highlighting
						(AnjutaDocument       *doc);

/* 
 * Non exported functions
 */
gboolean	_anjuta_document_is_saving_as	(AnjutaDocument       *doc);

// CHECK: va bene un gint?
glong		 _anjuta_document_get_seconds_since_last_save_or_load 
						(AnjutaDocument       *doc);

/* private because the property will move to gtk */
gboolean	 _anjuta_document_get_has_selection
						(AnjutaDocument       *doc);
						  
G_END_DECLS

#endif /* __ANJUTA_DOCUMENT_H__ */
