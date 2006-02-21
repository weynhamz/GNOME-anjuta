/***************************************************************************
 *            sourceview.c
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "sourceview.h"
#include "plugin.h"

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-savable.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-markable.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>
#include <libanjuta/interfaces/ianjuta-editor-assist.h>
#include <libanjuta/interfaces/ianjuta-editor-convert.h>

#include <libgnomevfs/gnome-vfs-init.h>
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs.h>

#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourcelanguage.h>
#include <gtksourceview/gtksourcelanguagesmanager.h>
#include <gtksourceview/gtksourcebuffer.h>

/* Use same value as sourceview test application... */
#define READ_BUFFER_SIZE   4096

static void sourceview_class_init(SourceviewClass *klass);
static void sourceview_instance_init(Sourceview *sv);
static void sourceview_finalize(GObject *object);

struct SourceviewPrivate {
	/* filename of the loaded file */
	gchar* filename;
	/* GnomeVFS uri of the loaded file */
	gchar* uri;
	
	/* GtkSouceView */
	GtkWidget* source_view;
	
	/* GtkSourceBuffer */
	GtkSourceBuffer* source_buffer;
	
	/* GtkSourceLanguagesManager */
	GtkSourceLanguagesManager* languages_manager;
	
	/* Markers */
	GList* markers;
};

typedef enum {
	SIGNAL_TYPE_EXAMPLE,
	LAST_SIGNAL
} SourceviewSignalType;

typedef struct {
	Sourceview *object;
} SourceviewSignal;

static guint sourceview_signals[LAST_SIGNAL] = { 0 };
static GObjectClass *parent_class = NULL;

/* Callbacks */

/* Called whenever the source_buffer is changed */
static void on_source_buffer_changed(GtkSourceBuffer* buffer, Sourceview* sv)
{
	/* Emit IAnjutaFileSavable signals */
	g_signal_emit_by_name(G_OBJECT(sv), "save_point",
						  !gtk_text_buffer_get_modified(GTK_TEXT_BUFFER(buffer)));
}

/* Called whenever the curser moves */
static void on_cursor_moved(GtkTextView *widget, GtkMovementStep step,
							 gint count, gboolean extend_selection,
							 Sourceview* sv)
{
	/* Emit IAnjutaEditor signals */
	g_signal_emit_by_name(G_OBJECT(sv), "update_ui");
}

static void 
sourceview_instance_init(Sourceview* sv)
{
	sv->priv = g_new0(SourceviewPrivate, 1);
}

static void
sourceview_class_init(SourceviewClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	parent_class = g_type_class_peek_parent(klass);
	object_class->finalize = sourceview_finalize;
	
	/* Create signals here:
	   sourceview_signals[SIGNAL_TYPE_EXAMPLE] = g_signal_new(...)
 	*/
}

static void
sourceview_finalize(GObject *object)
{
	Sourceview *cobj;
	cobj = ANJUTA_SOURCEVIEW(object);
	
	/* Free private members, etc. */
	
	gtk_widget_destroy(cobj->priv->source_view);
	g_object_unref(cobj->priv->languages_manager);
	
	g_free(cobj->priv);
	G_OBJECT_CLASS(parent_class)->finalize(object);
}

/* Create a new sourceview instance. If uri is valid,
the file will be loaded in the buffer */

Sourceview *
sourceview_new(const gchar* uri, const gchar* filename)
{
	Sourceview *sv = ANJUTA_SOURCEVIEW(g_object_new(ANJUTA_TYPE_SOURCEVIEW, NULL));
	static guint new_file_count = 0;
	
	/* Create buffer */
	sv->priv->source_buffer = gtk_source_buffer_new(NULL);
	g_signal_connect_after(G_OBJECT(sv->priv->source_buffer), "changed", 
					 G_CALLBACK(on_source_buffer_changed), sv);
	
	/* Create languages manager */
	sv->priv->languages_manager = gtk_source_languages_manager_new();
	
	/* Create SourceView instance */
	sv->priv->source_view = gtk_source_view_new_with_buffer(sv->priv->source_buffer);
	g_signal_connect_after(G_OBJECT(sv->priv->source_view), "move-cursor", 
					 G_CALLBACK(on_cursor_moved),sv);
	
	/* Add View */
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sv),
				      GTK_POLICY_AUTOMATIC,
				      GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(sv), sv->priv->source_view);
	gtk_widget_show_all(GTK_WIDGET(sv));
	
	/* Create Marker list */
	sv->priv->markers = NULL;
	
	g_message("URI = %s", sv->priv->uri);

	if (filename && strlen(filename) > 0)
		sv->priv->filename = g_strdup(filename); 
	else 
		sv->priv->filename = g_strdup_printf ("Newfile#%d", ++new_file_count);
	if (uri && strlen(uri) > 0)
	{
		GnomeVFSResult result;
		GnomeVFSURI* vfs_uri;
		GnomeVFSFileInfo info = {0,0};
		
		new_file_count--;
		if (sv->priv->filename)
			g_free (sv->priv->filename);
		vfs_uri = gnome_vfs_uri_new(uri);
		result = gnome_vfs_get_file_info_uri(vfs_uri, &info, GNOME_VFS_SET_FILE_INFO_NONE);
		gnome_vfs_uri_unref(vfs_uri); 
		sv->priv->filename = g_strdup(info.name);
		sv->priv->uri = g_strdup(uri);
	}
	
	if (uri != NULL)
		ianjuta_file_open(IANJUTA_FILE(sv), uri, NULL);
	
	return sv;
}

/* IAnjutaFile interface */

/* Open uri in Editor */
static void 
ifile_open (IAnjutaFile* file, const gchar *uri, GError** e)
{
	GtkSourceLanguage *language = NULL;
	gchar *mime_type;

	Sourceview* sv = ANJUTA_SOURCEVIEW(file);
	
	/* Do nothing on unsaved files */
	if (ianjuta_file_savable_is_dirty(IANJUTA_FILE_SAVABLE(sv), NULL))
		return;
	
	g_free(sv->priv->uri);
	sv->priv->uri = g_strdup(uri);
	g_free(sv->priv->filename);
	sv->priv->filename = g_path_get_basename(uri);

	mime_type = gnome_vfs_get_mime_type (uri);
	if (mime_type)
	{
		language = gtk_source_languages_manager_get_language_from_mime_type (sv->priv->languages_manager,
										     mime_type);

		if (language == NULL)
		{
			DEBUG_PRINT ("No language found for mime type `%s'\n", mime_type);
			g_object_set (G_OBJECT (sv->priv->source_buffer), "highlight", FALSE, NULL);
		}
		else
		{	
			g_object_set (G_OBJECT (sv->priv->source_buffer), "highlight", TRUE, NULL);

			gtk_source_buffer_set_language (sv->priv->source_buffer, language);
		}
			
		g_free (mime_type);
	}
	else
	{
		g_object_set (G_OBJECT (sv->priv->source_buffer), "highlight", FALSE, NULL);

		DEBUG_PRINT ("Couldn't get mime type for file `%s'", uri);
	}
	
	/* Clear buffer */
	gtk_source_buffer_begin_not_undoable_action(sv->priv->source_buffer);
	gtk_text_buffer_set_text(GTK_TEXT_BUFFER(sv->priv->source_buffer), "", 0);
	
	/* Load file */
	{
		GnomeVFSHandle* handle;
		GtkTextIter iter;
		GnomeVFSFileSize buffer_size;
		gchar* file_buffer = g_malloc(READ_BUFFER_SIZE);
		
		gnome_vfs_open(&handle, uri, GNOME_VFS_OPEN_READ);
		while (gnome_vfs_read(handle, file_buffer, READ_BUFFER_SIZE, &buffer_size)
			   == GNOME_VFS_OK)
		{
			gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(sv->priv->source_buffer), &iter);
			gtk_text_buffer_insert(GTK_TEXT_BUFFER(sv->priv->source_buffer), &iter,
												   file_buffer, buffer_size);
		}
	}
	gtk_source_buffer_end_not_undoable_action(sv->priv->source_buffer);
	gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(sv->priv->source_buffer), FALSE);
}

/* Return the currently loaded uri */

static gchar* 
ifile_get_uri (IAnjutaFile* file, GError** e)
{
	return g_strdup(ANJUTA_SOURCEVIEW(file)->priv->uri);
}

/* IAnjutaFileSavable interface */

/* Save file */
static void 
ifile_savable_save (IAnjutaFileSavable* file, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(file);
	gchar* uri;
	
	/* We cannot save the file if it does not yet have an uri */
	if (sv->priv->uri == NULL)
		return;
	uri = g_strdup(sv->priv->uri);
	ianjuta_file_savable_save_as(file, uri, e);
	g_free(uri);
}

/* Save file as */
static void 
ifile_savable_save_as (IAnjutaFileSavable* file, const gchar *uri, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(file);
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->source_buffer);
	GnomeVFSHandle* handle;
	GnomeVFSFileSize bytes_written;
	GnomeVFSFileSize bytes_to_write;
	GnomeVFSResult result;
	GtkTextIter start_iter;
	GtkTextIter end_iter;
	gchar* text;
	
	gtk_text_buffer_get_bounds(buffer, &start_iter, &end_iter);
	text = gtk_text_buffer_get_text(buffer, &start_iter, &end_iter, FALSE);
	bytes_to_write = strlen(text);
	
	DEBUG_PRINT("URI = %s", uri);
	
	result = gnome_vfs_create (&handle, uri, GNOME_VFS_OPEN_WRITE, FALSE, 0664);
	if (result == GNOME_VFS_OK)
	{
		result = gnome_vfs_write(handle, text, bytes_to_write, &bytes_written);
			 
		if ((result == GNOME_VFS_OK) && (bytes_to_write == bytes_written))
		{
			gtk_text_buffer_set_modified(buffer, FALSE);
			g_free(sv->priv->uri);
			g_free(sv->priv->filename);
			sv->priv->uri = g_strdup(uri);
			sv->priv->filename = g_path_get_basename(uri);
			/* Signals of IAnjutaFileSavable */
			DEBUG_PRINT("MyURI = %s", sv->priv->uri);
			DEBUG_PRINT("URI = %s", uri);
			g_signal_emit_by_name (G_OBJECT (sv), "saved", sv->priv->uri);
			/* Emit IAnjutaFileSavable signals */
			g_signal_emit_by_name(G_OBJECT(sv), "save_point",
								  !gtk_text_buffer_get_modified(GTK_TEXT_BUFFER(buffer)));
			/* Signals of IAnjutaEditor */
			g_signal_emit_by_name (G_OBJECT(sv), "update_ui");
			
			return;
		}
		gnome_vfs_close(handle);
	}
	if (result != GNOME_VFS_OK || bytes_to_write != bytes_written)
	{
		DEBUG_PRINT("Error: %s", gnome_vfs_result_to_string(result));
	}
}

static void 
ifile_savable_set_dirty (IAnjutaFileSavable* file, gboolean dirty, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(file);
	gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(sv->priv->source_buffer), 
								 dirty);
}

static gboolean 
ifile_savable_is_dirty (IAnjutaFileSavable* file, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(file);
	return gtk_text_buffer_get_modified(GTK_TEXT_BUFFER(sv->priv->source_buffer));
}

static void
isavable_iface_init (IAnjutaFileSavableIface *iface)
{
	iface->save = ifile_savable_save;
	iface->save_as = ifile_savable_save_as;
	iface->set_dirty = ifile_savable_set_dirty;
	iface->is_dirty = ifile_savable_is_dirty;
}

static void
ifile_iface_init (IAnjutaFileIface *iface)
{
	iface->open = ifile_open;
	iface->get_uri = ifile_get_uri;
}

/* IAnjutaEditor interface */

/* Scroll to line */
static void ieditor_goto_line(IAnjutaEditor *editor, gint line, GError **e)
{
	GtkTextIter iter;
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	const gfloat LEFT = 0.0;
	const gfloat CENTER = 0.5;
	
	gtk_text_buffer_get_iter_at_line(GTK_TEXT_BUFFER(sv->priv->source_buffer),
									 &iter, line - 1);
	gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(sv->priv->source_view),
											   &iter, 0, TRUE, LEFT, CENTER);
	gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(sv->priv->source_buffer),
									 &iter);
}

/* Scroll to position */
static void ieditor_goto_position(IAnjutaEditor *editor, gint position, GError **e)
{
	GtkTextIter iter;
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	const gfloat LEFT = 0.0;
	const gfloat CENTER = 0.5;
	
	gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(sv->priv->source_buffer),
									 &iter, position);
	gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(sv->priv->source_view),
											   &iter, 0, TRUE, LEFT, CENTER);
}

/* Return a newly allocated pointer containing the whole text */
static gchar* ieditor_get_text(IAnjutaEditor* editor, gint start,
							   gint end, GError **e)
{
	GtkTextIter start_iter;
	GtkTextIter end_iter;
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	
	gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(sv->priv->source_buffer),
								   &start_iter, start);
	gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(sv->priv->source_buffer),
								   &end_iter, end);
	return gtk_text_buffer_get_text(GTK_TEXT_BUFFER(sv->priv->source_buffer),
									&start_iter, &end_iter, FALSE);
}

/* TODO! No documentation, so we do nothing */
static gchar* ieditor_get_attributes(IAnjutaEditor* editor, 
									 gint start, gint end, GError **e)
{
	return NULL;
}

/* Get cursor position */
static gint ieditor_get_position(IAnjutaEditor* editor, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->source_buffer);
	GtkTextIter iter;
	
	gtk_text_buffer_get_iter_at_mark(buffer, &iter, 
									 gtk_text_buffer_get_insert(buffer));
	return gtk_text_iter_get_offset(&iter);
}


/* Return line of cursor */
static gint ieditor_get_lineno(IAnjutaEditor *editor, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->source_buffer);
	GtkTextIter iter;
	
	gtk_text_buffer_get_iter_at_mark(buffer, &iter, 
									 gtk_text_buffer_get_insert(buffer));
	return gtk_text_iter_get_line(&iter);
}

/* Return the length of the text in the buffer */
static gint ieditor_get_length(IAnjutaEditor *editor, GError **e)
{
	/* We do not use gtk_text_buffer_get_char_count here because
	someone may rely that this is the size of a buffer for all the text */
	
	GtkTextIter start_iter;
	GtkTextIter end_iter;
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	gchar* text;
	gint length;
	
	gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(sv->priv->source_buffer),
								   &start_iter);
	gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(sv->priv->source_buffer),
								   &end_iter);
	text = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(sv->priv->source_buffer),
									&start_iter, &end_iter, FALSE);
	length = strlen(text);
	g_free(text);
	return length;
}

/* Return word on cursor position */
static gchar* ieditor_get_current_word(IAnjutaEditor *editor, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->source_buffer);
	GtkTextIter iter_begin;
	GtkTextIter iter_end;
	
	gtk_text_buffer_get_iter_at_mark(buffer, &iter_begin, 
									 gtk_text_buffer_get_insert(buffer));
	gtk_text_buffer_get_iter_at_mark(buffer, &iter_end, 
									 gtk_text_buffer_get_insert(buffer));
	
	gtk_text_iter_backward_word_start(&iter_begin);
	gtk_text_iter_forward_word_end(&iter_end);
	
	DEBUG_PRINT("Current word = %s", 
				gtk_text_buffer_get_text(buffer, &iter_begin, &iter_end, FALSE));
	
	return gtk_text_buffer_get_text(buffer, &iter_begin, &iter_end, FALSE);
}

/* Insert text at position */
static void ieditor_insert(IAnjutaEditor *editor, gint position,
							   const gchar* text, gint length, GError **e)
{
	GtkTextIter iter;
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);

	gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(sv->priv->source_buffer),
									   &iter, position);
	gtk_text_buffer_insert(GTK_TEXT_BUFFER(sv->priv->source_buffer),
						   &iter, text, length);
}

/* Append text to buffer */
static void ieditor_append(IAnjutaEditor *editor, const gchar* text,
							   gint length, GError **e)
{
	GtkTextIter iter;
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);

	gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(sv->priv->source_buffer),
									   &iter);
	gtk_text_buffer_insert(GTK_TEXT_BUFFER(sv->priv->source_buffer),
						   &iter, text, length);
}

static void ieditor_erase_all(IAnjutaEditor *editor, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	gtk_text_buffer_set_text(GTK_TEXT_BUFFER(sv->priv->source_buffer), "", 0);
}

/* Return true if editor can redo */
static gboolean ieditor_can_redo(IAnjutaEditor *editor, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	DEBUG_PRINT("Can redo = %d", gtk_source_buffer_can_redo(sv->priv->source_buffer));
	return gtk_source_buffer_can_redo(sv->priv->source_buffer);
}

/* Return true if editor can undo */
static gboolean ieditor_can_undo(IAnjutaEditor *editor, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	DEBUG_PRINT("Can undo = %d", gtk_source_buffer_can_undo(sv->priv->source_buffer));
	return gtk_source_buffer_can_undo(sv->priv->source_buffer);
}

/* Return column of cursor */
static gint ieditor_get_column(IAnjutaEditor *editor, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->source_buffer);
	GtkTextIter iter;
	
	gtk_text_buffer_get_iter_at_mark(buffer, &iter, 
									 gtk_text_buffer_get_insert(buffer));
	return gtk_text_iter_get_line_offset(&iter);
}

/* Return TRUE if editor is in overwrite mode */
static gboolean ieditor_get_overwrite(IAnjutaEditor *editor, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	return gtk_text_view_get_overwrite(GTK_TEXT_VIEW(sv->priv->source_view));
}


/* Set the editor popup menu */
static void ieditor_set_popup_menu(IAnjutaEditor *editor, 
								   GtkWidget* menu, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	gtk_menu_attach_to_widget(GTK_MENU(menu), sv->priv->source_view,
							  NULL);
}

/* Return the opened filename */
static const gchar* ieditor_get_filename(IAnjutaEditor *editor, GError **e)
{
	return ANJUTA_SOURCEVIEW(editor)->priv->filename;
}

/* Convert from position to line */
static gint ieditor_get_line_from_position(IAnjutaEditor *editor, 
										   gint position, GError **e)
{
	GtkTextIter iter;
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(sv->priv->source_buffer),
									   &iter, position);
	return gtk_text_iter_get_line(&iter);
}

static void 
ieditor_undo(IAnjutaEditor* edit, GError** ee)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	gtk_source_buffer_undo(sv->priv->source_buffer);
	g_signal_emit_by_name(G_OBJECT(sv), "update_ui", sv); 
}

static void 
ieditor_redo(IAnjutaEditor* edit, GError** ee)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	gtk_source_buffer_redo(sv->priv->source_buffer);
}

static void
ieditor_iface_init (IAnjutaEditorIface *iface)
{
	iface->goto_line = ieditor_goto_line;
	iface->goto_position = ieditor_goto_position;
	iface->get_text = ieditor_get_text;
	iface->get_attributes = ieditor_get_attributes;
	iface->get_position = ieditor_get_position;
	iface->get_lineno = ieditor_get_lineno;
	iface->get_length = ieditor_get_length;
	iface->get_current_word = ieditor_get_current_word;
	iface->insert = ieditor_insert;
	iface->append = ieditor_append;
	iface->erase_all = ieditor_erase_all;
	iface->get_filename = ieditor_get_filename;
	iface->can_undo = ieditor_can_undo;
	iface->can_redo = ieditor_can_redo;
	iface->get_column = ieditor_get_column;
	iface->get_overwrite = ieditor_get_overwrite;
	iface->set_popup_menu = ieditor_set_popup_menu;
	iface->get_line_from_position = ieditor_get_line_from_position;
	iface->undo = ieditor_undo;
	iface->redo = ieditor_redo;
}

/* IAnjutaEditorConvert Interface */

static void
iconvert_to_upper(IAnjutaEditorConvert* edit, gint start_pos, gint end_pos, GError** ee)
{
	// TODO: lot's of unicode magic...
}

static void
iconvert_to_lower(IAnjutaEditorConvert* edit, gint start_pos, gint end_pos, GError** ee)
{
	
}

static void
iconvert_iface_init(IAnjutaEditorConvertIface* iface)
{
	iface->to_upper = iconvert_to_upper;
	iface->to_lower = iconvert_to_lower;
}

/* IAnjutaEditorSelection */

static void
iselect_all(IAnjutaEditorSelection* edit, GError** ee)
{
	
}

static void
iselect_to_brace(IAnjutaEditorSelection* edit, GError** ee)
{
	
}

static void 
iselect_block(IAnjutaEditorSelection* edit, GError** ee)
{
	
}


/* Select range between start and end */
static void iselect_set(IAnjutaEditorSelection *editor, gint start, 
								  gint end, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	GtkTextIter start_iter;
	GtkTextIter end_iter;
	
	gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(sv->priv->source_buffer),
									   &start_iter, start);
	gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(sv->priv->source_buffer),
									   &end_iter, end);
	gtk_text_buffer_move_mark_by_name(GTK_TEXT_BUFFER(sv->priv->source_buffer),
									  "insert", &start_iter);
	gtk_text_buffer_move_mark_by_name(GTK_TEXT_BUFFER(sv->priv->source_buffer),
									  "selection_bound", &start_iter);
	
}

/* Return a newly allocated pointer containing selected text or
NULL if no text is selected */
static gchar* iselect_get(IAnjutaEditorSelection* editor, GError **e)
{
	GtkTextIter start_iter;
	GtkTextIter end_iter;
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	
	if (gtk_text_buffer_get_selection_bounds(GTK_TEXT_BUFFER(sv->priv->source_buffer),
										 &start_iter, &end_iter))
	{
		return gtk_text_buffer_get_text(GTK_TEXT_BUFFER(sv->priv->source_buffer),
										&start_iter, &end_iter, FALSE);
	}
	else
		return NULL;
	
}

/* Get start point of selection */
static gint iselect_get_start(IAnjutaEditorSelection *editor, GError **e)
{
	/* This is the same within gtk_text_buffer */
	return ianjuta_editor_get_position(IANJUTA_EDITOR(editor), e);
}

/* Get end position of selection */
static gint iselect_get_end(IAnjutaEditorSelection *editor, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	GtkTextIter end_iter;
	
	gtk_text_buffer_get_iter_at_mark(GTK_TEXT_BUFFER(sv->priv->source_buffer),
									 &end_iter,
									 gtk_text_buffer_get_insert(GTK_TEXT_BUFFER(sv->priv->source_buffer)));
	return gtk_text_iter_get_offset(&end_iter);
}


static void iselect_select_block(IAnjutaEditorSelection *editor, GError **e)
{
	// TODO
}

static void iselect_select_function(IAnjutaEditorSelection *editor, GError **e)
{
	// TODO
}

/* If text is selected, replace with given text */
static void iselect_replace(IAnjutaEditorSelection* editor, 
								  const gchar* text, gint length, GError **e)
{
	GtkTextIter start_iter;
	GtkTextIter end_iter;
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	
	if (gtk_text_buffer_get_selection_bounds(GTK_TEXT_BUFFER(sv->priv->source_buffer),
										 &start_iter, &end_iter))
	{
		gtk_text_buffer_delete_selection(GTK_TEXT_BUFFER(sv->priv->source_buffer),
														 FALSE, TRUE);
		gtk_text_buffer_insert(GTK_TEXT_BUFFER(sv->priv->source_buffer),
							   &start_iter, text, length);
	}
}

static void 
iselect_cut(IAnjutaEditorSelection* edit, GError** ee)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->source_buffer);
	GtkClipboard* clipboard = gtk_clipboard_get(GDK_NONE);
	gtk_text_buffer_cut_clipboard(buffer, clipboard, TRUE);
}

static void 
iselect_copy(IAnjutaEditorSelection* edit, GError** ee)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->source_buffer);
	GtkClipboard* clipboard = gtk_clipboard_get(GDK_NONE);
	gtk_text_buffer_copy_clipboard(buffer, clipboard);	
}

static void 
iselect_paste(IAnjutaEditorSelection* edit, GError** ee)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->source_buffer);
	GtkClipboard* clipboard = gtk_clipboard_get(GDK_NONE);
	gtk_text_buffer_paste_clipboard(buffer, clipboard, NULL, TRUE);	
}

static void 
iselect_clear(IAnjutaEditorSelection* edit, GError** ee)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);

	gtk_text_buffer_delete_selection(GTK_TEXT_BUFFER(sv->priv->source_buffer),
									 TRUE, TRUE);
}

static void
iselect_iface_init(IAnjutaEditorSelectionIface *iface)
{
	iface->set = iselect_set;
	iface->get_start = iselect_get_start;
	iface->get_end = iselect_get_end;
	iface->select_block = iselect_select_block;
	iface->select_function = iselect_select_function;
	iface->select_all = iselect_all;
	iface->select_to_brace = iselect_to_brace;
	iface->select_block = iselect_block;
	iface->get = iselect_get;
	iface->replace = iselect_replace;
	iface->cut = iselect_cut;
	iface->copy = iselect_copy;
	iface->paste = iselect_paste;
	iface->clear = iselect_clear;
}



/* IAnjutaAssist Interface */
static void 
iassist_autocomplete(IAnjutaEditorAssist* edit, GError** ee)
{
	
}

static void iassist_iface_init(IAnjutaEditorAssistIface* iface)
{
	iface->autocomplete = iassist_autocomplete;
}

/* Marker Struture to connect handles and markers */
typedef struct
{
	gint handle;
	GtkSourceMarker* marker;
} SVMarker;

static gint marker_count = 0;

static gint
imark_mark(IAnjutaMarkable* mark, gint location, IAnjutaMarkableMarker marker,
		   GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(mark);
	GtkTextIter iter;
	GtkSourceMarker* source_marker;
	
	gtk_text_buffer_get_iter_at_line(GTK_TEXT_BUFFER(sv->priv->source_buffer),
									 &iter, location);
	source_marker = gtk_source_buffer_create_marker(sv->priv->source_buffer, 
													NULL, NULL, &iter);
	SVMarker* sv_marker = g_new0(SVMarker, 1);
	sv_marker->handle = marker_count++;
	sv_marker->marker = source_marker;
	
	sv->priv->markers = g_list_append(sv->priv->markers, sv_marker);
	
	return sv_marker->handle;
}

static void
imark_unmark(IAnjutaMarkable* mark, gint location, IAnjutaMarkableMarker marker,
			 GError **e)
{
	
}

static gboolean
imark_is_marker_set(IAnjutaMarkable* mark, gint location, 
					IAnjutaMarkableMarker marker, GError **e)
{
	
}

static gint
imark_location_from_handle(IAnjutaMarkable* mark, gint handle, GError **e)
{
	
}

static void
imark_delete_all_markers(IAnjutaMarkable* mark, IAnjutaMarkableMarker marker,
						 GError **e)
{
	
}

static void
imark_iface_init(IAnjutaMarkableIface* iface)
{
	iface->mark = imark_mark;
	iface->unmark = imark_unmark;
	iface->location_from_handle = imark_location_from_handle;
	iface->is_marker_set = imark_is_marker_set;
	iface->delete_all_markers = imark_delete_all_markers;
}


ANJUTA_TYPE_BEGIN(Sourceview, sourceview, GTK_TYPE_SCROLLED_WINDOW);
ANJUTA_TYPE_ADD_INTERFACE(ifile, IANJUTA_TYPE_FILE);
ANJUTA_TYPE_ADD_INTERFACE(isavable, IANJUTA_TYPE_FILE_SAVABLE);
ANJUTA_TYPE_ADD_INTERFACE(ieditor, IANJUTA_TYPE_EDITOR);
ANJUTA_TYPE_ADD_INTERFACE(imark, IANJUTA_TYPE_MARKABLE);
ANJUTA_TYPE_ADD_INTERFACE(iselect, IANJUTA_TYPE_EDITOR_SELECTION);
ANJUTA_TYPE_ADD_INTERFACE(iassist, IANJUTA_TYPE_EDITOR_ASSIST);
ANJUTA_TYPE_ADD_INTERFACE(iconvert, IANJUTA_TYPE_EDITOR_CONVERT);
ANJUTA_TYPE_END;
