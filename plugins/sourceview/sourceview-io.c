/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-trunk
 * Copyright (C) Johannes Schmid 2008 <jhs@gnome.org>
 * 
 * anjuta-trunk is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta-trunk is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "sourceview-io.h"
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/anjuta-convert.h>
#include <libanjuta/anjuta-encodings.h>
#include <sourceview-private.h>

#define READ_SIZE 4096
#define RATE_LIMIT 5000 /* Use a big rate limit to avoid duplicates */
#define TIMEOUT 5

enum
{
	SAVE_STATUS,
	SAVE_FINISHED,
	OPEN_STATUS,
	OPEN_FINISHED,
	OPEN_FAILED,
	SAVE_FAILED,

	LAST_SIGNAL
};

#define IO_ERROR_QUARK g_quark_from_string ("SourceviewIO-Error")

static guint io_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (SourceviewIO, sourceview_io, G_TYPE_OBJECT);

static void
sourceview_io_init (SourceviewIO *object)
{
	object->file = NULL;
	object->filename = NULL;
	object->read_buffer = NULL;
	object->write_buffer = NULL;
	object->cancel = g_cancellable_new();
	object->monitor = NULL;
	object->last_encoding = NULL;
	object->bytes_read = 0;
}

static void
sourceview_io_finalize (GObject *object)
{
	SourceviewIO* sio = SOURCEVIEW_IO(object);
	if (sio->file)
		g_object_unref (sio->file);
	g_free(sio->filename);
	g_free(sio->read_buffer);
	g_free(sio->write_buffer);
	g_object_unref (sio->cancel);
	if (sio->monitor_idle > 0)
		g_source_remove (sio->monitor_idle);
	if (sio->monitor)
		g_object_unref (sio->monitor);
	
	G_OBJECT_CLASS (sourceview_io_parent_class)->finalize (object);
}

static void
sourceview_io_class_init (SourceviewIOClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GObjectClass* parent_class = G_OBJECT_CLASS (klass);

	object_class->finalize = sourceview_io_finalize;

	klass->changed = NULL;
	klass->save_finished = NULL;
	klass->open_finished = NULL;
	klass->open_failed = NULL;
	klass->save_failed = NULL;

	io_signals[SAVE_STATUS] =
		g_signal_new ("changed",
		              G_OBJECT_CLASS_TYPE (klass),
		              0,
		              G_STRUCT_OFFSET (SourceviewIOClass, changed),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0,
		              NULL);

	io_signals[SAVE_FINISHED] =
		g_signal_new ("save-finished",
		              G_OBJECT_CLASS_TYPE (klass),
		              0,
		              G_STRUCT_OFFSET (SourceviewIOClass, save_finished),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0,
		              NULL);

	io_signals[OPEN_FINISHED] =
		g_signal_new ("open-finished",
		              G_OBJECT_CLASS_TYPE (klass),
		              0,
		              G_STRUCT_OFFSET (SourceviewIOClass, open_finished),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0,
		              NULL);

	io_signals[OPEN_FAILED] =
		g_signal_new ("open-failed",
		              G_OBJECT_CLASS_TYPE (klass),
		              0,
		              G_STRUCT_OFFSET (SourceviewIOClass, open_failed),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__POINTER,
		              G_TYPE_NONE, 1,
		              G_TYPE_POINTER);

	io_signals[SAVE_FAILED] =
		g_signal_new ("save-failed",
		              G_OBJECT_CLASS_TYPE (klass),
		              0,
		              G_STRUCT_OFFSET (SourceviewIOClass, save_failed),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__POINTER,
		              G_TYPE_NONE, 1,
		              G_TYPE_POINTER);
}

static void on_file_changed (GFileMonitor* monitor, 
							 GFile* file,
							 GFile* other_file,
							 GFileMonitorEvent event_type,
							 gpointer data)
{
	SourceviewIO* sio = SOURCEVIEW_IO(data);
	
	g_signal_emit_by_name (sio, "changed");
}

static gboolean
setup_monitor_idle(gpointer data)
{
	SourceviewIO* sio = SOURCEVIEW_IO(data);
	sio->monitor_idle = 0;
	if (sio->monitor != NULL)
		g_object_unref (sio->monitor);
	sio->monitor = g_file_monitor_file (sio->file,
										G_FILE_MONITOR_NONE,
										NULL,
										NULL);
	if (sio->monitor)
	{
		g_signal_connect (sio->monitor, "changed", 
						  G_CALLBACK(on_file_changed), sio);
		g_file_monitor_set_rate_limit (sio->monitor, RATE_LIMIT);
	}
	return FALSE;
}

static void
setup_monitor(SourceviewIO* sio)
{
	if (sio->monitor_idle > 0)
		g_source_remove (sio->monitor_idle);
	
	sio->monitor_idle = g_timeout_add_seconds (TIMEOUT,
											   setup_monitor_idle,
											   sio);
}

static void
cancel_monitor (SourceviewIO* sio)
{
	if (sio->monitor != NULL)
		g_object_unref (sio->monitor);
	sio->monitor = NULL;
}

static void
set_display_name (SourceviewIO* sio)
{
	GFileInfo* file_info = g_file_query_info (sio->file,
											  G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
											  G_FILE_QUERY_INFO_NONE,
											  NULL,
											  NULL);
	if (file_info)
	{
		g_free (sio->filename);
		sio->filename = g_strdup(g_file_info_get_display_name (file_info));
	}
	else
	{
		g_free (sio->filename);
		sio->filename = NULL;
	}
	g_object_unref (file_info);
}

static void
on_save_finished (GObject* output_stream, GAsyncResult* result, gpointer data)
{
	SourceviewIO* sio = SOURCEVIEW_IO(data);
	GError* err = NULL;
	g_output_stream_write_finish (G_OUTPUT_STREAM(output_stream),
								  result,
								  &err);
	if (err)
	{
		g_signal_emit_by_name (sio, "save-failed", err);
		g_error_free (err);
	}
	else
	{
		set_display_name (sio);
		g_output_stream_close(G_OUTPUT_STREAM (output_stream), NULL, NULL);
		g_signal_emit_by_name (sio, "save-finished");
		setup_monitor (sio);
	}
	g_free (sio->write_buffer);
	sio->write_buffer = NULL;
	g_object_unref (output_stream);
}

void
sourceview_io_save (SourceviewIO* sio)
{
	if (!sio->file)
	{
		GError* error = NULL;
		g_set_error (&error, IO_ERROR_QUARK, 0, 
					 _("Could not save file because filename not yet specified"));
		g_signal_emit_by_name (sio, "save-failed", error);
		g_error_free(error);
	}
	else
		sourceview_io_save_as (sio, sio->file);
}

void
sourceview_io_save_as (SourceviewIO* sio, GFile* file)
{
	GFileOutputStream* output_stream;
	GError* err = NULL;
	g_return_if_fail (file != NULL);
	
	cancel_monitor (sio);
	
	output_stream = g_file_create (file, G_FILE_CREATE_NONE, NULL, &err);
	if (!output_stream)
	{
		if (err->code != G_IO_ERROR_EXISTS)
		{
			g_signal_emit_by_name (sio, "save-failed", err);
			g_error_free (err);
			return;
		}
		else
		{
			output_stream = g_file_replace (file, NULL, TRUE, G_FILE_CREATE_NONE,
											NULL, NULL);
			if (!output_stream)
			{
				g_signal_emit_by_name (sio, "save-failed", err);
				g_error_free (err);
				return;
			}
		}
	}
	
	if (sio->last_encoding == NULL)
	{
		sio->write_buffer = ianjuta_editor_get_text_all (IANJUTA_EDITOR(sio->sv), 
														 NULL);
	}
	else
	{
		GError* err = NULL;
		gchar* buffer_text = ianjuta_editor_get_text_all (IANJUTA_EDITOR(sio->sv), 
														  NULL);
		sio->write_buffer = anjuta_convert_from_utf8 (buffer_text,
													  -1,
													  sio->last_encoding,
													  NULL,
													  &err);
		g_free (buffer_text);
		if (err != NULL)
		{
			g_signal_emit_by_name (sio, "save-failed", err);
			g_error_free(err);
			return;
		}
	}
	g_cancellable_reset (sio->cancel);
	g_output_stream_write_async (G_OUTPUT_STREAM (output_stream),
								 sio->write_buffer,
								 strlen (sio->write_buffer),
								 G_PRIORITY_LOW,
								 sio->cancel,
								 on_save_finished,
								 sio);
	
	if (sio->file != file)
	{
		if (sio->file)
			g_object_unref (sio->file);
		sio->file = file;
		g_object_ref (file);
	}
}

static void insert_text_in_document(SourceviewIO* sio, const gchar* text, gsize len)
{
	GtkSourceBuffer* document = GTK_SOURCE_BUFFER (sio->sv->priv->document);
	gtk_source_buffer_begin_not_undoable_action (GTK_SOURCE_BUFFER (sio->sv->priv->document));

	/* Insert text in the buffer */
	gtk_text_buffer_set_text (GTK_TEXT_BUFFER (document), 
							  text,
							  len);

	gtk_text_buffer_set_modified (GTK_TEXT_BUFFER (document),
				      FALSE);

	gtk_source_buffer_end_not_undoable_action (document);
}

static gboolean
append_buffer (SourceviewIO* sio, gsize size)
{
	/* Text is utf-8 - good */
	if (g_utf8_validate (sio->read_buffer, size, NULL))
	{
		insert_text_in_document (sio, sio->read_buffer, size);
	}
	else
	{
		/* Text is not utf-8 */
		GError *conv_error = NULL;
		gchar *converted_text = NULL;
		gsize new_len = size;
		const AnjutaEncoding* enc = NULL;
			
		converted_text = anjuta_convert_to_utf8 (sio->read_buffer,
												 size,
												 &enc,
												 &new_len,
												 &conv_error);
		if  (converted_text == NULL)	
		{
			/* Last chance, let's try 8859-15 */
			enc = anjuta_encoding_get_from_charset( "ISO-8859-15");
			conv_error = NULL;
			converted_text = anjuta_convert_to_utf8 (sio->read_buffer,
													 size,
													 &enc,
													 &new_len,
													 &conv_error);
		}
		if (converted_text == NULL)
		{
			g_return_val_if_fail (conv_error != NULL, FALSE);
			
			g_signal_emit_by_name (sio, "open-failed", conv_error);
			g_error_free (conv_error);
			g_cancellable_cancel (sio->cancel);
			return FALSE;
		}
		sio->last_encoding = enc;
		insert_text_in_document (sio, converted_text, new_len);
		g_free (converted_text);
	}
	return TRUE;
}

static void
on_read_finished (GObject* input, GAsyncResult* result, gpointer data)
{
	SourceviewIO* sio = SOURCEVIEW_IO(data);
	GInputStream* input_stream = G_INPUT_STREAM(input);
	gsize current_bytes = 0;
	GError* err = NULL;
	
	current_bytes = g_input_stream_read_finish (input_stream, result, &err);
	if (err)
	{
		g_signal_emit_by_name (sio, "open-failed", err);
		g_error_free (err);
		g_object_unref (input_stream);
		g_free (sio->read_buffer);
		sio->read_buffer = NULL;
		sio->bytes_read = 0;
		return;
	}
	
	sio->bytes_read += current_bytes;
	if (current_bytes != 0)
	{
		sio->read_buffer = g_realloc (sio->read_buffer, sio->bytes_read + READ_SIZE);
		g_input_stream_read_async (G_INPUT_STREAM (input_stream),
								   sio->read_buffer + sio->bytes_read,
								   READ_SIZE,
								   G_PRIORITY_LOW,
								   sio->cancel,
								   on_read_finished,
								   sio);
		return;
	}
	else
	{
		if (append_buffer (sio, sio->bytes_read))
			g_signal_emit_by_name (sio, "open-finished");
		sio->bytes_read = 0;
		g_object_unref (input_stream);
		setup_monitor (sio);
		g_free (sio->read_buffer);
		sio->read_buffer = NULL;
	}
}

void
sourceview_io_open (SourceviewIO* sio, GFile* file)
{
	GFileInputStream* input_stream;
	GError* err = NULL;
	
	g_return_if_fail (file != NULL);
	
	if (sio->file)
		g_object_unref (sio->file);
	sio->file = file;
	g_object_ref (sio->file);
	set_display_name(sio);
	
	input_stream = g_file_read (file, NULL, &err);
	if (!input_stream)
	{
		g_signal_emit_by_name (sio, "open-failed", err);
		g_error_free (err);
		return;
	}
	sio->read_buffer = g_realloc (sio->read_buffer, READ_SIZE);
	g_input_stream_read_async (G_INPUT_STREAM (input_stream),
							   sio->read_buffer,
							   READ_SIZE,
							   G_PRIORITY_LOW,
							   sio->cancel,
							   on_read_finished,
							   sio);
}

GFile*
sourceview_io_get_file (SourceviewIO* sio)
{
	if (sio->file)
		g_object_ref (sio->file);
	return sio->file;
}

void 
sourceview_io_cancel (SourceviewIO* sio)
{
	g_cancellable_cancel (sio->cancel);
}

gchar* 
sourceview_io_get_filename (SourceviewIO* sio)
{
	static gint new_file_count = 0;
	if (sio->filename)
		return g_strdup(sio->filename);	
	else /* new file */
	{
		sio->filename = g_strdup_printf (_("New file %d"), new_file_count++);
		return g_strdup (sio->filename);
	}									 
}

void 
sourceview_io_set_filename (SourceviewIO* sio, const gchar* filename)
{
	g_free (sio->filename);
	sio->filename = g_strdup(filename);
}

gchar* 
sourceview_io_get_mime_type (SourceviewIO* sio)
{
	GFileInfo* file_info;
	
	if (!sio->file)
		return NULL;
	
	file_info = g_file_query_info (sio->file,
								   G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
								   G_FILE_QUERY_INFO_NONE,
								   NULL,
								   NULL);
	if (file_info)
	{
		gchar* mime_type = g_strdup (g_file_info_get_content_type (file_info));
		g_object_unref (file_info);
		return mime_type;
	}
	else
		return NULL;
	
}

gboolean
sourceview_io_get_read_only (SourceviewIO* sio)
{
	GFileInfo* file_info;
	gboolean retval;
	
	if (!sio->file)
		return FALSE;
	
	file_info = g_file_query_info (sio->file,
								   G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE,
								   G_FILE_QUERY_INFO_NONE,
								   NULL,
								   NULL);
	if (!file_info)
		return FALSE;
	
	retval = !g_file_info_get_attribute_boolean (file_info,
												G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE);
	g_object_unref (file_info);
	return retval;
}

SourceviewIO*
sourceview_io_new (Sourceview* sv)
{
	SourceviewIO* sio = SOURCEVIEW_IO(g_object_new (SOURCEVIEW_TYPE_IO, NULL));
	sio->sv = sv;
	return sio;
}
