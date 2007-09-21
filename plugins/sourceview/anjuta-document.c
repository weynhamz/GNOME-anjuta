/*
 * anjuta-document.c
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi 
 * Copyright (C) 2002-2005 Paolo Maggi
 * Copyright (C) 2006 Johannes Schmid
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
 * Modified by the anjuta Team, 1998-2005. See the AUTHORS file for a 
 * list of people on the anjuta Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdlib.h>

#include <glib/gi18n.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libanjuta/anjuta-debug.h>

#include "anjuta-encodings.h"
#include "anjuta-document.h"
#include "anjuta-document-loader.h"
#include "anjuta-document-saver.h"
#include "anjuta-marshal.h"
#include "anjuta-utils.h"

#include <gtksourceview/gtksourceiter.h>
#include <gtksourceview/gtksourcelanguagesmanager.h>

#define ANJUTA_MAX_PATH_LEN  2048

#define ANJUTA_DOCUMENT_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), ANJUTA_TYPE_DOCUMENT, AnjutaDocumentPrivate))

static void	anjuta_document_set_readonly	(AnjutaDocument *doc,
						 gboolean       readonly);
						 
static void	delete_range_cb 		(AnjutaDocument *doc, 
						 GtkTextIter   *start,
						 GtkTextIter   *end);
			     
struct _AnjutaDocumentPrivate
{
	gint	     readonly : 1;
	gint	     last_save_was_manually : 1; 	
	gint	     language_set_by_user : 1;
	gint         is_saving_as : 1;
	gint         stop_cursor_moved_emission : 1;

	gchar	    *uri;
	gint 	     untitled_number;

	GnomeVFSURI *vfs_uri;

	const AnjutaEncoding *encoding;
	GtkSourceLanguagesManager* lang_manager;

	gchar	    *mime_type;

	time_t       mtime;

	GTimeVal     time_of_last_save_or_load;

	/* Temp data while loading */
	AnjutaDocumentLoader *loader;
	gboolean             create; /* Create file if uri points 
	                              * to a non existing file */
	const AnjutaEncoding *requested_encoding;
	gint                 requested_line_pos;

	/* Saving stuff */
	AnjutaDocumentSaver *saver;
};

enum {
	PROP_0,

	PROP_URI,
	PROP_SHORTNAME,
	PROP_MIME_TYPE,
	PROP_READ_ONLY,
	PROP_ENCODING
};

enum {
	CURSOR_MOVED,
	LOADING,
	LOADED,
	SAVING,
	SAVED,
	CHAR_ADDED,
	LAST_SIGNAL
};

static guint document_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE(AnjutaDocument, anjuta_document, GTK_TYPE_SOURCE_BUFFER)

GQuark
anjuta_document_error_quark (void)
{
	static GQuark quark = 0;

	if (G_UNLIKELY (quark == 0))
		quark = g_quark_from_static_string ("anjuta_io_load_error");

	return quark;
}

static GHashTable *allocated_untitled_numbers = NULL;

static gint
get_untitled_number (void)
{
	gint i = 1;

	if (allocated_untitled_numbers == NULL)
		allocated_untitled_numbers = g_hash_table_new (NULL, NULL);

	g_return_val_if_fail (allocated_untitled_numbers != NULL, -1);

	while (TRUE)
	{
		if (g_hash_table_lookup (allocated_untitled_numbers, GINT_TO_POINTER (i)) == NULL)
		{
			g_hash_table_insert (allocated_untitled_numbers, 
					     GINT_TO_POINTER (i),
					     GINT_TO_POINTER (i));

			return i;
		}

		++i;
	}
}

static void
release_untitled_number (gint n)
{
	g_return_if_fail (allocated_untitled_numbers != NULL);

	g_hash_table_remove (allocated_untitled_numbers, GINT_TO_POINTER (n));
}

static void
anjuta_document_finalize (GObject *object)
{
	AnjutaDocument *doc = ANJUTA_DOCUMENT (object); 

	if (doc->priv->untitled_number > 0)
	{
		g_return_if_fail (doc->priv->uri == NULL);
		release_untitled_number (doc->priv->untitled_number);
	}

	if (doc->priv->uri != NULL)
	{
		GtkTextIter iter;
		// gchar *position;

		gtk_text_buffer_get_iter_at_mark (
				GTK_TEXT_BUFFER (doc),			
				&iter,
				gtk_text_buffer_get_insert (GTK_TEXT_BUFFER (doc)));
	}

	g_free (doc->priv->uri);
	if (doc->priv->vfs_uri != NULL)
		gnome_vfs_uri_unref (doc->priv->vfs_uri);

	g_free (doc->priv->mime_type);

	if (doc->priv->loader)
		g_object_unref (doc->priv->loader);
}

static void
anjuta_document_get_property (GObject    *object,
			     guint       prop_id,
			     GValue     *value,
			     GParamSpec *pspec)
{
	AnjutaDocument *doc = ANJUTA_DOCUMENT (object);

	switch (prop_id)
	{
		case PROP_URI:
			g_value_set_string (value, doc->priv->uri);
			break;
		case PROP_SHORTNAME:
			g_value_take_string (value, anjuta_document_get_short_name_for_display (doc));
			break;
		case PROP_MIME_TYPE:
			g_value_set_string (value, doc->priv->mime_type);
			break;
		case PROP_READ_ONLY:
			g_value_set_boolean (value, doc->priv->readonly);
			break;
		case PROP_ENCODING:
			g_value_set_boxed (value, doc->priv->encoding);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
anjuta_document_set_property (GObject      *object,
			     guint         prop_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
	// AnjutaDocument *doc = ANJUTA_DOCUMENT (object);

	switch (prop_id)
	{
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
emit_cursor_moved (AnjutaDocument *doc)
{
	if (!doc->priv->stop_cursor_moved_emission)
	{
		g_signal_emit (doc,
			       document_signals[CURSOR_MOVED],
			       0);
	}
}

static void
anjuta_document_mark_set (GtkTextBuffer     *buffer,
                         const GtkTextIter *iter,
                         GtkTextMark       *mark)
{
	AnjutaDocument *doc = ANJUTA_DOCUMENT (buffer);

	if (GTK_TEXT_BUFFER_CLASS (anjuta_document_parent_class)->mark_set)
		GTK_TEXT_BUFFER_CLASS (anjuta_document_parent_class)->mark_set (buffer,
									       iter,
									       mark);

	if (mark == gtk_text_buffer_get_insert (buffer))
	{
		emit_cursor_moved (doc);
	}
}

static void
anjuta_document_changed (GtkTextBuffer *buffer)
{
	emit_cursor_moved (ANJUTA_DOCUMENT (buffer));

	GTK_TEXT_BUFFER_CLASS (anjuta_document_parent_class)->changed (buffer);
}

static void 
anjuta_document_class_init (AnjutaDocumentClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkTextBufferClass *buf_class = GTK_TEXT_BUFFER_CLASS (klass);

	object_class->finalize = anjuta_document_finalize;
	object_class->get_property = anjuta_document_get_property;
	object_class->set_property = anjuta_document_set_property;	

	buf_class->mark_set = anjuta_document_mark_set;
	buf_class->changed = anjuta_document_changed;
	
	g_object_class_install_property (object_class, PROP_URI,
					 g_param_spec_string ("uri",
					 		      "URI",
					 		      "The document's URI",
					 		      NULL,
					 		      G_PARAM_READABLE));
	g_object_class_install_property (object_class, PROP_SHORTNAME,
					 g_param_spec_string ("shortname",
					 		      "Short Name",
					 		      "The document's short name",
					 		      NULL,
					 		      G_PARAM_READABLE));
	g_object_class_install_property (object_class, PROP_MIME_TYPE,
					 g_param_spec_string ("mime-type",
					 		      "MIME Type",
					 		      "The document's MIME Type",
					 		      "text/plain",
					 		      G_PARAM_READABLE));
	g_object_class_install_property (object_class, PROP_READ_ONLY,
					 g_param_spec_boolean ("read-only",
					 		       "Read Only",
					 		       "Whether the document is read only or not",
					 		       FALSE,
					 		       G_PARAM_READABLE));
	g_object_class_install_property (object_class, PROP_ENCODING,
					 g_param_spec_boxed ("encoding",
					 		     "Encoding",
					 		     "The AnjutaEncoding used for the document",
					 		     ANJUTA_TYPE_ENCODING,
					 		     G_PARAM_READABLE));

	/* This signal is used to update the cursor position is the statusbar,
	 * it's emitted either when the insert mark is moved explicitely or
	 * when the buffer changes (insert/delete).
	 * We prevent the emission of the signal during replace_all to 
	 * improve performance.
	 */
	document_signals[CURSOR_MOVED] =
   		g_signal_new ("cursor-moved",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AnjutaDocumentClass, cursor_moved),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);

	document_signals[LOADING] =
   		g_signal_new ("loading",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AnjutaDocumentClass, loading),
			      NULL, NULL,
			      anjuta_marshal_VOID__UINT64_UINT64,
			      G_TYPE_NONE,
			      2,
			      G_TYPE_UINT64,
			      G_TYPE_UINT64);

	document_signals[LOADED] =
   		g_signal_new ("loaded",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AnjutaDocumentClass, loaded),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_POINTER);

	document_signals[SAVING] =
   		g_signal_new ("saving",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AnjutaDocumentClass, saving),
			      NULL, NULL,
			      anjuta_marshal_VOID__UINT64_UINT64,
			      G_TYPE_NONE,
			      2,
			      G_TYPE_UINT64,
			      G_TYPE_UINT64);

	document_signals[SAVED] =
   		g_signal_new ("saved",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AnjutaDocumentClass, saved),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_POINTER);
				  
	g_type_class_add_private (object_class, sizeof(AnjutaDocumentPrivate));
}


static void
set_encoding (AnjutaDocument       *doc, 
	      const AnjutaEncoding *encoding,
	      gboolean             set_by_user)
{
	g_return_if_fail (encoding != NULL);

	if (doc->priv->encoding == encoding)
		return;

	doc->priv->encoding = encoding;

	if (set_by_user)
	{
		const gchar *charset;

		charset = anjuta_encoding_get_charset (encoding);
	}

	g_object_notify (G_OBJECT (doc), "encoding");
}

static void
anjuta_document_init (AnjutaDocument *doc)
{

	doc->priv = ANJUTA_DOCUMENT_GET_PRIVATE (doc);

	doc->priv->uri = NULL;
	doc->priv->vfs_uri = NULL;
	doc->priv->untitled_number = get_untitled_number ();

	doc->priv->mime_type = g_strdup ("text/plain");

	doc->priv->readonly = FALSE;

	doc->priv->stop_cursor_moved_emission = FALSE;

	doc->priv->last_save_was_manually = TRUE;
	doc->priv->language_set_by_user = FALSE;

	doc->priv->mtime = 0;

	g_get_current_time (&doc->priv->time_of_last_save_or_load);

	doc->priv->encoding = anjuta_encoding_get_utf8 ();

	gtk_source_buffer_set_check_brackets (GTK_SOURCE_BUFFER (doc), 
					     TRUE);

	g_signal_connect_after (doc, 
			  	"delete-range",
			  	G_CALLBACK (delete_range_cb),
			  	NULL);		
}

AnjutaDocument *
anjuta_document_new (void)
{

	return ANJUTA_DOCUMENT (g_object_new (ANJUTA_TYPE_DOCUMENT, NULL));
}

/* If mime type is null, we guess from the filename */
/* If uri is null, we only set the mime-type */
static void
set_uri (AnjutaDocument *doc,
	 const gchar   *uri,
	 const gchar   *mime_type)
{

	g_return_if_fail ((uri == NULL) || anjuta_utils_is_valid_uri (uri));

	if (uri != NULL)
	{
		if (doc->priv->uri == uri)
			return;

		g_free (doc->priv->uri);
		doc->priv->uri = g_strdup (uri);

		if (doc->priv->vfs_uri != NULL)
			gnome_vfs_uri_unref (doc->priv->vfs_uri);

		/* Note: vfs_uri may be NULL for some valid but
		 * unsupported uris */
		doc->priv->vfs_uri = gnome_vfs_uri_new (uri);

		if (doc->priv->untitled_number > 0)
		{
			release_untitled_number (doc->priv->untitled_number);
			doc->priv->untitled_number = 0;
		}
	}

	g_free (doc->priv->mime_type);
	if (mime_type != NULL)
	{
		doc->priv->mime_type = g_strdup (mime_type);
	}
	else
	{
		gchar *base_name = NULL;

		/* Guess the mime type from file extension or fallback to "text/plain" */
		if (doc->priv->vfs_uri != NULL)
			base_name = gnome_vfs_uri_extract_short_path_name (doc->priv->vfs_uri);
		if (base_name != NULL)
		{
			const gchar *detected_mime;

			detected_mime = gnome_vfs_get_mime_type_for_name (base_name);
			if (detected_mime == NULL ||
			    strcmp (GNOME_VFS_MIME_TYPE_UNKNOWN, detected_mime) == 0)
				detected_mime = "text/plain";

			doc->priv->mime_type = g_strdup (detected_mime);

			g_free (base_name);
		}
		else
		{
			doc->priv->mime_type = g_strdup ("text/plain");
		}
	}
	g_object_notify (G_OBJECT (doc), "uri");
	g_object_notify (G_OBJECT (doc), "shortname");
}

gchar *
anjuta_document_get_uri (AnjutaDocument *doc)
{
	g_return_val_if_fail (ANJUTA_IS_DOCUMENT (doc), NULL);

	return g_strdup (doc->priv->uri);
}

/* Never returns NULL */
gchar *
anjuta_document_get_uri_for_display (AnjutaDocument *doc)
{
	g_return_val_if_fail (ANJUTA_IS_DOCUMENT (doc), "");

	if (doc->priv->uri == NULL)
		return g_strdup_printf (_("Unsaved Document %d"),
					doc->priv->untitled_number);	      
	else
		return gnome_vfs_format_uri_for_display (doc->priv->uri);
}

/* move to anjuta-utils? */
static gchar *
get_uri_shortname_for_display (GnomeVFSURI *uri)
{
	gchar *name;	
	gboolean  validated;

	validated = FALSE;

	name = gnome_vfs_uri_extract_short_name (uri);
	
	if (name == NULL)
	{
		name = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_PASSWORD);
	}
	else if (g_ascii_strcasecmp (uri->method_string, "file") == 0)
	{
		gchar *text_uri;
		gchar *local_file;
		text_uri = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_PASSWORD);
		local_file = gnome_vfs_get_local_path_from_uri (text_uri);

		if (local_file != NULL)
		{
			g_free (name);
			name = g_filename_display_basename (local_file);
			validated = TRUE;
		}

		g_free (local_file);
		g_free (text_uri);
	} 
	else if (!gnome_vfs_uri_has_parent (uri)) 
	{
		const gchar *method;

		method = uri->method_string;

		if (name == NULL ||
		    strcmp (name, GNOME_VFS_URI_PATH_STR) == 0) 
		{
			g_free (name);
			name = g_strdup (method);
		} 
		/*
		else 
		{
			gchar *tmp;

			tmp = name;
			name = g_strdup_printf ("%s: %s", method, name);
			g_free (tmp);
		}
		*/
	}

	if (!validated && !g_utf8_validate (name, -1, NULL)) 
	{
		gchar *utf8_name;

		utf8_name = anjuta_utils_make_valid_utf8 (name);
		g_free (name);
		name = utf8_name;
	}

	return name;
}

/* Never returns NULL */
gchar *
anjuta_document_get_short_name_for_display (AnjutaDocument *doc)
{
	g_return_val_if_fail (ANJUTA_IS_DOCUMENT (doc), "");

	if (doc->priv->uri == NULL)
		return g_strdup_printf (_("Unsaved Document %d"),
					doc->priv->untitled_number);	      
	else if (doc->priv->vfs_uri == NULL)
		return g_strdup (doc->priv->uri);
	else
		return get_uri_shortname_for_display (doc->priv->vfs_uri);
}

/* Never returns NULL */
gchar *
anjuta_document_get_mime_type (AnjutaDocument *doc)
{
	g_return_val_if_fail (ANJUTA_IS_DOCUMENT (doc), "text/plain");
	g_return_val_if_fail (doc->priv->mime_type != NULL, "text/plain");

 	return g_strdup (doc->priv->mime_type);
}

/* Note: do not emit the notify::read-only signal */
static void
set_readonly (AnjutaDocument *doc,
	      gboolean       readonly)
{	
	readonly = (readonly != FALSE);

	if (doc->priv->readonly == readonly) 
		return;

	doc->priv->readonly = readonly;
}

static void
anjuta_document_set_readonly (AnjutaDocument *doc,
			     gboolean       readonly)
{
	g_return_if_fail (ANJUTA_IS_DOCUMENT (doc));

	set_readonly (doc, readonly);

	g_object_notify (G_OBJECT (doc), "read-only");
}

gboolean
anjuta_document_get_readonly (AnjutaDocument *doc)
{
	g_return_val_if_fail (ANJUTA_IS_DOCUMENT (doc), TRUE);

	return doc->priv->readonly;
}

static void
reset_temp_loading_data (AnjutaDocument       *doc)
{
	/* the loader has been used, throw it away */
	g_object_unref (doc->priv->loader);
	doc->priv->loader = NULL;

	doc->priv->requested_encoding = NULL;
	doc->priv->requested_line_pos = 0;
}

static void
document_loader_loaded (AnjutaDocumentLoader *loader,
			const GError        *error,
			AnjutaDocument       *doc)
{
	/* load was successful */
	if (error == NULL)
	{
		GtkTextIter iter;
		const gchar *mime_type;

		mime_type = anjuta_document_loader_get_mime_type (loader);

		doc->priv->mtime = anjuta_document_loader_get_mtime (loader);

		g_get_current_time (&doc->priv->time_of_last_save_or_load);

		set_readonly (doc,
		      anjuta_document_loader_get_readonly (loader));

		set_encoding (doc, 
			      anjuta_document_loader_get_encoding (loader),
			      (doc->priv->requested_encoding != NULL));
		      
		/* We already set the uri */
		set_uri (doc, NULL, mime_type);

		/* move the cursor at the requested line if any */
		if (doc->priv->requested_line_pos > 0)
		{
			/* line_pos - 1 because get_iter_at_line counts from 0 */
			gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (doc),
							  &iter,
							  doc->priv->requested_line_pos - 1);
		}
		else
			gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (doc),
							  &iter, 0);
		gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (doc), &iter);
	}

	/* special case creating a named new doc */
	else if (doc->priv->create &&
	         (error->code == GNOME_VFS_ERROR_NOT_FOUND))
	{
		reset_temp_loading_data (doc);
		// FIXME: do other stuff??

		g_signal_emit (doc,
			       document_signals[LOADED],
			       0,
			       NULL);

		return;
	}
	
	g_signal_emit (doc,
		       document_signals[LOADED],
		       0,
		       error);

	reset_temp_loading_data (doc);
}

static void
document_loader_loading (AnjutaDocumentLoader *loader,
			 gboolean             completed,
			 const GError        *error,
			 AnjutaDocument       *doc)
{
	if (completed)
	{
		document_loader_loaded (loader, error, doc);
	}
	else
	{
		GnomeVFSFileSize size;
		GnomeVFSFileSize read;

		size = anjuta_document_loader_get_file_size (loader);
		read = anjuta_document_loader_get_bytes_read (loader);

		g_signal_emit (doc, 
			       document_signals[LOADING],
			       0,
			       read,
			       size);
	}
}

void
anjuta_document_load (AnjutaDocument       *doc,
		     const gchar         *uri,
		     const AnjutaEncoding *encoding,
		     gint                 line_pos,
		     gboolean             create)
{
	g_return_if_fail (ANJUTA_IS_DOCUMENT (doc));
	g_return_if_fail (uri != NULL);
	g_return_if_fail (anjuta_utils_is_valid_uri (uri));

	g_return_if_fail (doc->priv->loader == NULL);

	/* create a loader. It will be destroyed when loading is completed */
	doc->priv->loader = anjuta_document_loader_new (doc);

	g_signal_connect (doc->priv->loader,
			  "loading",
			  G_CALLBACK (document_loader_loading),
			  doc);

	doc->priv->create = create;
	doc->priv->requested_encoding = encoding;
	doc->priv->requested_line_pos = line_pos;

	set_uri (doc, uri, NULL);

	anjuta_document_loader_load (doc->priv->loader,
				    uri,
				    encoding);
}

gboolean
anjuta_document_load_cancel (AnjutaDocument *doc)
{
	g_return_val_if_fail (ANJUTA_IS_DOCUMENT (doc), FALSE);

	if (doc->priv->loader == NULL)
		return FALSE;

	return anjuta_document_loader_cancel (doc->priv->loader);
}

static void
document_saver_saving (AnjutaDocumentSaver *saver,
		       gboolean            completed,
		       const GError       *error,
		       AnjutaDocument      *doc)
{

	if (completed)
	{
		/* save was successful */
		if (error == NULL)
		{
			const gchar *uri;
			const gchar *mime_type;

			uri = anjuta_document_saver_get_uri (saver);
			mime_type = anjuta_document_saver_get_mime_type (saver);

			doc->priv->mtime = anjuta_document_saver_get_mtime (saver);

			g_get_current_time (&doc->priv->time_of_last_save_or_load);

			anjuta_document_set_readonly (doc, FALSE);

			gtk_text_buffer_set_modified (GTK_TEXT_BUFFER (doc),
						      FALSE);

			set_uri (doc, uri, mime_type);

			set_encoding (doc, 
				      doc->priv->requested_encoding, 
				      TRUE);
		}

		/* the saver has been used, throw it away */
		g_object_unref (doc->priv->saver);
		doc->priv->saver = NULL;
		doc->priv->is_saving_as = FALSE;
		
		g_signal_emit (doc,
			       document_signals[SAVED],
			       0,
			       error);
	}
	else
	{
		GnomeVFSFileSize size = 0;
		GnomeVFSFileSize written = 0;

		size = anjuta_document_saver_get_file_size (saver);
		written = anjuta_document_saver_get_bytes_written (saver);

		g_signal_emit (doc,
			       document_signals[SAVING],
			       0,
			       written,
			       size);
	}
}

static void
document_save_real (AnjutaDocument          *doc,
		    const gchar            *uri,
		    const AnjutaEncoding    *encoding,
		    time_t                  mtime,
		    AnjutaDocumentSaveFlags  flags)
{
	g_return_if_fail (doc->priv->saver == NULL);

	/* create a saver, it will be destroyed once saving is complete */
	doc->priv->saver = anjuta_document_saver_new (doc);

	g_signal_connect (doc->priv->saver,
			  "saving",
			  G_CALLBACK (document_saver_saving),
			  doc);

	doc->priv->requested_encoding = encoding;

	anjuta_document_saver_save (doc->priv->saver,
				   uri,
				   encoding,
				   mtime,
				   flags);
}

void
anjuta_document_save (AnjutaDocument          *doc,
		     AnjutaDocumentSaveFlags  flags)
{
	g_return_if_fail (ANJUTA_IS_DOCUMENT (doc));
	g_return_if_fail (doc->priv->uri != NULL);

	document_save_real (doc,
			    doc->priv->uri,
			    doc->priv->encoding,
			    doc->priv->mtime,
			    flags);
}

void
anjuta_document_save_as (AnjutaDocument          *doc,
			const gchar            *uri,
			const AnjutaEncoding    *encoding,
			AnjutaDocumentSaveFlags  flags)
{
	g_return_if_fail (ANJUTA_IS_DOCUMENT (doc));
	g_return_if_fail (uri != NULL);
	g_return_if_fail (encoding != NULL);

	doc->priv->is_saving_as = TRUE;
	
	document_save_real (doc, uri, encoding, 0, flags);
}

/*
 * If @line is bigger than the lines of the document, the cursor is moved
 * to the last line and FALSE is returned.
 */
gboolean
anjuta_document_goto_line (AnjutaDocument *doc, 
			  gint           line)
{
	gboolean ret = TRUE;
	guint line_count;
	GtkTextIter iter;

	g_return_val_if_fail (ANJUTA_IS_DOCUMENT (doc), FALSE);
	g_return_val_if_fail (line >= -1, FALSE);

	line_count = gtk_text_buffer_get_line_count (GTK_TEXT_BUFFER (doc));

	if (line >= line_count)
	{
		ret = FALSE;
		gtk_text_buffer_get_end_iter (GTK_TEXT_BUFFER (doc),
					      &iter);
	}
	else
	{
		gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (doc),
						  &iter,
						  line);
	}

	gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (doc), &iter);

	return ret;
}

const AnjutaEncoding *
anjuta_document_get_encoding (AnjutaDocument *doc)
{
	g_return_val_if_fail (ANJUTA_IS_DOCUMENT (doc), NULL);

	return doc->priv->encoding;
}


static gboolean
wordcharacters_contains (gchar c)
{
	const gchar* wordcharacters =
		"_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	gint pos;
	
	for (pos = 0; pos < strlen(wordcharacters); pos++)
	{
		if (wordcharacters[pos] == c)
		{
			return TRUE;
		}
	}
	return FALSE;
}

gchar* anjuta_document_get_current_word(AnjutaDocument* doc, gboolean end_position)
{
	GtkTextIter begin;
	GtkTextIter end;
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(doc);
	gchar* region;
	gchar* word;
	gint startword;
	gint endword;
	int cplen;
	const int maxlength = 100;
	
	gtk_text_buffer_get_iter_at_mark (buffer, &begin, 
																		gtk_text_buffer_get_insert(GTK_TEXT_BUFFER(buffer)));
	gtk_text_buffer_get_iter_at_mark (buffer, &end, 
																		gtk_text_buffer_get_insert(GTK_TEXT_BUFFER(buffer)));
	startword = gtk_text_iter_get_line_offset (&begin);	
	endword = gtk_text_iter_get_line_offset (&end);
	
	gtk_text_iter_set_line_offset (&begin, 0);
	gtk_text_iter_forward_to_line_end (&end);
	
	region = gtk_text_buffer_get_text (buffer, &begin, &end, FALSE);
	
	while (startword> 0 && wordcharacters_contains(region[startword - 1]))
		startword--;
	while (!end_position && region[endword] && wordcharacters_contains(region[endword]))
		endword++;
	if(startword == endword)
		return NULL;
	
	region[endword] = '\0';
	cplen = (maxlength < (endword-startword+1))?maxlength:(endword-startword+1);
	word = g_strndup (region + startword, cplen);
	
	DEBUG_PRINT ("region: %s\n start: %d end: %d word: %s", region, startword, endword, word);
	g_free(region);
	
	return word;
}
						 
static void	
delete_range_cb (AnjutaDocument *doc, 
		 GtkTextIter   *start,
		 GtkTextIter   *end)
{
	GtkTextIter d_start;
	GtkTextIter d_end;

	d_start = *start;
	d_end = *end;
	
}

