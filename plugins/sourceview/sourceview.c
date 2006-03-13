/****************************************************************************
 *            sourceview.c
 *
 *  Do Dez 29 00:50:15 2005
 *  Copyright  2005  Johannes Schmid
 *  jhs@gnome.org
 ****************************************************************************/

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
#include "sourceview-prefs.h"
#include "plugin.h"

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-savable.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-markable.h>
#include <libanjuta/interfaces/ianjuta-indicable.h>
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
#include <gtksourceview/gtksourceiter.h>

#include "config.h"
#include "anjuta-encodings.h"
#include "sourceview-private.h"
#include "anjuta-document.h"
#include "anjuta-view.h"
#include "sourceview-autocomplete.h"

#define READ_BUFFER_SIZE   16384

#define FORWARD 	0
#define BACKWARD 	1

#define REGISTER_NOTIFY(key, func) \
	notify_id = anjuta_preferences_notify_add (te->preferences, \
											   key, func, te, NULL); \
	te->gconf_notify_ids = g_list_prepend (te->gconf_notify_ids, \
										   (gpointer)(notify_id));

static void sourceview_class_init(SourceviewClass *klass);
static void sourceview_instance_init(Sourceview *sv);
static void sourceview_finalize(GObject *object);

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

/* Called whenever the document is changed */
static void on_document_changed(AnjutaDocument* buffer, Sourceview* sv)
{
	/* Emit IAnjutaFileSavable signals */
	g_signal_emit_by_name(G_OBJECT(sv), "save_point",
						  !gtk_text_buffer_get_modified(GTK_TEXT_BUFFER(buffer)));
}

/* Called whenever the curser moves */
static void on_cursor_moved(AnjutaDocument *widget,
							 Sourceview* sv)
{
	/* Emit IAnjutaEditor signals */
	g_signal_emit_by_name(G_OBJECT(sv), "update_ui");
}

/* Callback for dialog below */
static void 
on_reload_dialog_response (GtkWidget *dlg, gint res, Sourceview *sv)
{
	if (res == GTK_RESPONSE_YES)
	{
		ianjuta_file_open(IANJUTA_FILE(sv),
						  anjuta_document_get_uri(sv->priv->document), NULL);
	}
	gtk_widget_destroy (dlg);
}

/* Show popup menu */
static gboolean
on_menu_popup(GtkWidget* view, Sourceview* sv)
{
    gtk_menu_attach_to_widget(GTK_MENU(sv->priv->menu), view, NULL);
    gtk_menu_popup(GTK_MENU(sv->priv->menu), NULL, NULL, NULL, NULL, 0,
        gtk_get_current_event_time());
    DEBUG_PRINT("Popup");
    return TRUE;
}

/* VFS-Monitor Callback */
static void
on_sourceview_uri_changed (GnomeVFSMonitorHandle *handle,
							const gchar *monitor_uri,
							const gchar *info_uri,
							GnomeVFSMonitorEventType event_type,
							gpointer user_data)
{
	GtkWidget *dlg;
	GtkWidget *parent;
	gchar *buff;
	Sourceview *sv = ANJUTA_SOURCEVIEW (user_data);
	
	DEBUG_PRINT ("File changed!!!");
	
	if (!(event_type == GNOME_VFS_MONITOR_EVENT_CHANGED ||
		  event_type == GNOME_VFS_MONITOR_EVENT_CREATED))
		return;
	
	if (!anjuta_util_diff(anjuta_document_get_uri(sv->priv->document), 
						  ianjuta_editor_get_text(IANJUTA_EDITOR(sv),
						  0, ianjuta_editor_get_length(IANJUTA_EDITOR(sv), NULL),
							NULL)))
		return;
	
	if (strcmp (monitor_uri, info_uri) != 0)
		return;
	
	buff =
		g_strdup_printf (_
						 ("The file '%s' on the disk is more recent than\n"
						  "the current buffer.\nDo you want to reload it?"),
						 g_basename(anjuta_document_get_uri(sv->priv->document)));
	
	parent = gtk_widget_get_toplevel (GTK_WIDGET (sv->priv->view));
	
	dlg = gtk_message_dialog_new (GTK_WINDOW (parent),
								  GTK_DIALOG_DESTROY_WITH_PARENT,
								  GTK_MESSAGE_WARNING,
								  GTK_BUTTONS_NONE, buff);
	gtk_dialog_add_button (GTK_DIALOG (dlg),
						   GTK_STOCK_NO,
						   GTK_RESPONSE_NO);
	anjuta_util_dialog_add_button (GTK_DIALOG (dlg),
								   _("_Reload"),
								   GTK_STOCK_REFRESH,
								   GTK_RESPONSE_YES);
	g_free (buff);
	
	gtk_window_set_transient_for (GTK_WINDOW (dlg),
								  GTK_WINDOW (parent));
	
	g_signal_connect (dlg, "response",
					  G_CALLBACK (on_reload_dialog_response),
					  sv);
	g_signal_connect_swapped (dlg, "delete-event",
					  G_CALLBACK (gtk_widget_destroy),
					  dlg);
	gtk_widget_show (dlg);
}

/* Update Monitor on load/save */
static void
sourceview_remove_monitor(Sourceview* sv)
{
	if (sv->priv->monitor != NULL) 
	{
		gnome_vfs_monitor_cancel(sv->priv->monitor);
		sv->priv->monitor = NULL;
	}
}

static void 
sourceview_add_monitor(Sourceview* sv)
{
	g_return_if_fail(sv->priv->monitor == NULL);
	gnome_vfs_monitor_add(&sv->priv->monitor, anjuta_document_get_uri(sv->priv->document),
						  GNOME_VFS_MONITOR_FILE,
						  on_sourceview_uri_changed, sv);
}

/* Called when document is loaded completly */
static void on_document_loaded(AnjutaDocument* doc, GError* err, Sourceview* sv)
{
	gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(doc), FALSE);
    g_signal_emit_by_name(G_OBJECT(sv), "save_point",
						  TRUE);
	sourceview_add_monitor(sv);
}

/* Called when document is loaded completly */
static void on_document_saved(AnjutaDocument* doc, GError* err, Sourceview* sv)
{
	gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(doc), FALSE);
	g_signal_emit_by_name(G_OBJECT(sv), "save_point",
						  TRUE);
	sourceview_add_monitor(sv);
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
	
	sourceview_remove_monitor(cobj);
	
	gtk_widget_destroy(GTK_WIDGET(cobj->priv->view));
	g_free(cobj->priv);
	G_OBJECT_CLASS(parent_class)->finalize(object);
}

/* Sync with IANJUTA_MARKABLE_MARKER  */

#define MARKER_PIXMAP "pointer.png"
#define MARKER_NONE "sv-mark-none"
#define MARKER_BASIC "sv-mark-basic"
#define MARKER_LIGHT "sv-mark-light"
#define MARKER_ATTENTIVE "sv-mark-attentive"
#define MARKER_INTENSE "sv-mark-intense"

/* HIGHLIGHTED TAGS */

#define IMPORTANT_INDIC "important_indic"
#define WARNING_INDIC "warning_indic"
#define CRITICAL_INDIC "critical_indic"

/* Create pixmaps for the markers */
static void sourceview_create_markers(Sourceview* sv)
{
	GdkPixbuf * pixbuf;
	GtkSourceView* view = 	GTK_SOURCE_VIEW(sv->priv->view);

	
	if ((pixbuf = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"MARKER_PIXMAP, NULL)))
	{
		/* TODO: Use different pixmaps */
		gtk_source_view_set_marker_pixbuf (view, 
			MARKER_BASIC, pixbuf);
		gtk_source_view_set_marker_pixbuf (view, 
			MARKER_INTENSE, pixbuf);
		gtk_source_view_set_marker_pixbuf (view, 
			MARKER_LIGHT, pixbuf);
		gtk_source_view_set_marker_pixbuf (view, 
			MARKER_ATTENTIVE, pixbuf);
		
		g_object_unref (pixbuf);
	}
	else
		DEBUG_PRINT("Pixmap not found: %s!",PACKAGE_PIXMAPS_DIR"/"MARKER_PIXMAP);
}

/* Create tags for highlighting */
static void sourceview_create_highligth_indic(Sourceview* sv)
{	
	sv->priv->important_indic = 
	          gtk_text_buffer_create_tag (GTK_TEXT_BUFFER(sv->priv->document),
	          IMPORTANT_INDIC,
	   		  "foreground", "red", NULL);  
	//  TO BE DEFINED
	sv->priv->warning_indic = sv->priv->important_indic; 
	sv->priv->critical_indic = sv->priv->important_indic;
}


/* Create a new sourceview instance. If uri is valid,
the file will be loaded in the buffer */

Sourceview *
sourceview_new(const gchar* uri, const gchar* filename, AnjutaPreferences* prefs)
{
	Sourceview *sv = ANJUTA_SOURCEVIEW(g_object_new(ANJUTA_TYPE_SOURCEVIEW, NULL));

	/* Create buffer */
	sv->priv->document = anjuta_document_new();
	g_signal_connect_after(G_OBJECT(sv->priv->document), "changed", 
					 G_CALLBACK(on_document_changed), sv);
	g_signal_connect_after(G_OBJECT(sv->priv->document), "cursor-moved", 
					 G_CALLBACK(on_cursor_moved),sv);
	g_signal_connect_after(G_OBJECT(sv->priv->document), "loaded", 
					 G_CALLBACK(on_document_loaded), sv);
	g_signal_connect_after(G_OBJECT(sv->priv->document), "saved", 
					 G_CALLBACK(on_document_saved), sv);
					 
	/* Create View instance */
	sv->priv->view = ANJUTA_VIEW(anjuta_view_new(sv->priv->document));
	gtk_source_view_set_smart_home_end(GTK_SOURCE_VIEW(sv->priv->view), FALSE);
    g_signal_connect(G_OBJECT(sv->priv->view), "popup-menu", G_CALLBACK(on_menu_popup), sv);

	/* Apply Preferences (TODO) */
	sv->priv->prefs = prefs;
	sourceview_prefs_init(sv);
	
	/* Create Markers */
	sourceview_create_markers(sv);
		
	/* Add View */
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sv),
				      GTK_POLICY_AUTOMATIC,
				      GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(sv), GTK_WIDGET(sv->priv->view));
	gtk_widget_show_all(GTK_WIDGET(sv));
	
	if (uri != NULL && strlen(uri) > 0)
	{
		ianjuta_file_open(IANJUTA_FILE(sv), uri, NULL);
	}
	else if (filename != NULL && strlen(filename) > 0)
		sv->priv->filename = g_strdup(filename);	
	
	/* Create Higlight Tag */
	sourceview_create_highligth_indic(sv);
	
	return sv;
}

/* IAnjutaFile interface */

/* Open uri in Editor */
static void 
ifile_open (IAnjutaFile* file, const gchar *uri, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(file);
	sourceview_remove_monitor(sv);
	anjuta_document_load(sv->priv->document, uri, anjuta_encoding_get_utf8(),
						 0, FALSE);
}

/* Return the currently loaded uri */

static gchar* 
ifile_get_uri (IAnjutaFile* file, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(file);
	return anjuta_document_get_uri(sv->priv->document);
}

/* IAnjutaFileSavable interface */

/* Save file */
static void 
ifile_savable_save (IAnjutaFileSavable* file, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(file);
	sourceview_remove_monitor(sv);
					 
	anjuta_document_save(sv->priv->document, ANJUTA_DOCUMENT_SAVE_IGNORE_BACKUP);
}

/* Save file as */
static void 
ifile_savable_save_as (IAnjutaFileSavable* file, const gchar *uri, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(file);
	sourceview_remove_monitor(sv);
	anjuta_document_save_as(sv->priv->document, 
							uri, anjuta_encoding_get_utf8(),
							ANJUTA_DOCUMENT_SAVE_IGNORE_BACKUP);
	if (sv->priv->filename)
	{
		g_free(sv->priv->filename);
		sv->priv->filename = NULL;
	}
}

static void 
ifile_savable_set_dirty (IAnjutaFileSavable* file, gboolean dirty, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(file);
	gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(sv->priv->document), 
								 dirty);
}

static gboolean 
ifile_savable_is_dirty (IAnjutaFileSavable* file, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(file);
	return gtk_text_buffer_get_modified(GTK_TEXT_BUFFER(sv->priv->document));
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
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	anjuta_document_goto_line(sv->priv->document, line - 1);
	anjuta_view_scroll_to_cursor(sv->priv->view);
	DEBUG_PRINT("Line %d", line);
}

/* Scroll to position */
static void ieditor_goto_position(IAnjutaEditor *editor, gint position, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	anjuta_document_goto_line(sv->priv->document, 
							 ianjuta_editor_get_line_from_position(editor, position, NULL));
}

/* Return a newly allocated pointer containing the whole text */
static gchar* ieditor_get_text(IAnjutaEditor* editor, gint start,
							   gint end, GError **e)
{
	GtkTextIter start_iter;
	GtkTextIter end_iter;
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	
	gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(sv->priv->document),
								   &start_iter, start);
	gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(sv->priv->document),
								   &end_iter, end);
	return gtk_text_buffer_get_text(GTK_TEXT_BUFFER(sv->priv->document),
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
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->document);
	GtkTextIter iter;
	
	gtk_text_buffer_get_iter_at_mark(buffer, &iter, 
									 gtk_text_buffer_get_insert(buffer));

	return gtk_text_iter_get_offset(&iter);
}


/* Return line of cursor */
static gint ieditor_get_lineno(IAnjutaEditor *editor, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->document);
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
	
	gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(sv->priv->document),
								   &start_iter);
	gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(sv->priv->document),
								   &end_iter);
	text = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(sv->priv->document),
									&start_iter, &end_iter, FALSE);
	length = strlen(text);
	g_free(text);

	return length;
}

/* Return word on cursor position */
static gchar* ieditor_get_current_word(IAnjutaEditor *editor, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->document);
	GtkTextIter iter_begin;
	GtkTextIter iter_end;
	GtkTextIter cursor;
	
	gtk_text_buffer_get_iter_at_mark(buffer, &iter_begin, 
									 gtk_text_buffer_get_insert(buffer));
	gtk_text_buffer_get_iter_at_mark(buffer, &cursor, 
									 gtk_text_buffer_get_insert(buffer));
	gtk_text_buffer_get_iter_at_mark(buffer, &iter_end, 
									 gtk_text_buffer_get_insert(buffer));
	
	if (!gtk_text_iter_starts_word(&iter_begin))
		gtk_text_iter_backward_word_start(&iter_begin);
	if (!gtk_text_iter_ends_word(&iter_end))
		gtk_text_iter_forward_word_end(&iter_end);
	
	DEBUG_PRINT("Current word = %s", 
				gtk_text_buffer_get_text(buffer, &iter_begin, &iter_end, FALSE));
	
	/* Reset cursor */
	gtk_text_buffer_move_mark(buffer, gtk_text_buffer_get_insert(buffer), &cursor);
	
	return gtk_text_buffer_get_text(buffer, &iter_begin, &iter_end, FALSE);
}

/* Insert text at position */
static void ieditor_insert(IAnjutaEditor *editor, gint position,
							   const gchar* text, gint length, GError **e)
{
	GtkTextIter iter;
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	
	gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(sv->priv->document),
									   &iter, position);
	gtk_text_buffer_insert(GTK_TEXT_BUFFER(sv->priv->document),
						   &iter, text, length);
}

/* Append text to buffer */
static void ieditor_append(IAnjutaEditor *editor, const gchar* text,
							   gint length, GError **e)
{
	GtkTextIter iter;
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);

	gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(sv->priv->document),
									   &iter);
	gtk_text_buffer_insert(GTK_TEXT_BUFFER(sv->priv->document),
						   &iter, text, length);
}

static void ieditor_erase_all(IAnjutaEditor *editor, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	gtk_text_buffer_set_text(GTK_TEXT_BUFFER(sv->priv->document), "", 0);
}

/* Return true if editor can redo */
static gboolean ieditor_can_redo(IAnjutaEditor *editor, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	return gtk_source_buffer_can_redo(GTK_SOURCE_BUFFER(sv->priv->document));
}

/* Return true if editor can undo */
static gboolean ieditor_can_undo(IAnjutaEditor *editor, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	return gtk_source_buffer_can_undo(GTK_SOURCE_BUFFER(sv->priv->document));
}

/* Return column of cursor */
static gint ieditor_get_column(IAnjutaEditor *editor, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->document);
	GtkTextIter iter;
	
	gtk_text_buffer_get_iter_at_mark(buffer, &iter, 
									 gtk_text_buffer_get_insert(buffer));
	return gtk_text_iter_get_line_offset(&iter);
}

/* Return TRUE if editor is in overwrite mode */
static gboolean ieditor_get_overwrite(IAnjutaEditor *editor, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	return gtk_text_view_get_overwrite(GTK_TEXT_VIEW(sv->priv->view));
}


/* Set the editor popup menu */
static void ieditor_set_popup_menu(IAnjutaEditor *editor, 
								   GtkWidget* menu, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
    sv->priv->menu = menu;
}

/* Return the opened filename */
static const gchar* ieditor_get_filename(IAnjutaEditor *editor, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	if (sv->priv->filename != NULL)
		return sv->priv->filename;
	else
		return anjuta_document_get_short_name_for_display(sv->priv->document);
}

/* Convert from position to line */
static gint ieditor_get_line_from_position(IAnjutaEditor *editor, 
										   gint position, GError **e)
{
	GtkTextIter iter;
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	gint line;
	gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(sv->priv->document),
									   &iter, position);

	line = gtk_text_iter_get_line(&iter);
	line = line ? line - 1 : 0;
	
	return line;
}

static void 
ieditor_undo(IAnjutaEditor* edit, GError** ee)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	gtk_source_buffer_undo(GTK_SOURCE_BUFFER(sv->priv->document));
	g_signal_emit_by_name(G_OBJECT(sv), "update_ui", sv); 
}

static void 
ieditor_redo(IAnjutaEditor* edit, GError** ee)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	gtk_source_buffer_redo(GTK_SOURCE_BUFFER(sv->priv->document));
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

static void
set_select(Sourceview* sv, GtkTextIter* start_iter, GtkTextIter* end_iter)
{
	gtk_text_buffer_move_mark_by_name(GTK_TEXT_BUFFER(sv->priv->document),
									  "insert", start_iter);
	gtk_text_buffer_move_mark_by_name(GTK_TEXT_BUFFER(sv->priv->document),
									  "selection_bound", end_iter);
	gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(sv->priv->view),
		 gtk_text_buffer_get_insert(GTK_TEXT_BUFFER(sv->priv->document)));			
}

/* IAnjutaEditorSelection */

static void
iselect_to_brace(IAnjutaEditorSelection* edit, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->document);
	GtkTextIter start_iter;
	GtkTextIter end_iter;
	gboolean found;
	
	gtk_text_buffer_get_iter_at_mark(buffer, &start_iter, 
									 gtk_text_buffer_get_insert(buffer));
	end_iter = start_iter;
	found = gtk_source_iter_find_matching_bracket (&end_iter);
	if (found)
		set_select(sv, &start_iter, &end_iter);
}

static void 
iselect_block(IAnjutaEditorSelection* edit, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->document);
	GtkTextIter start_iter;
	GtkTextIter end_iter;
	GtkTextIter iter;
	gchar *text;
	gint position;
	
	gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(sv->priv->document),
								   &start_iter);
	gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(sv->priv->document),
								   &end_iter);
	text = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(sv->priv->document),
										&start_iter, &end_iter, FALSE);
	if (text)
	{
		gboolean found = FALSE;
		gint cpt = 0;
		gtk_text_buffer_get_iter_at_mark(buffer, &iter, 
									 gtk_text_buffer_get_insert(buffer));
		position = gtk_text_iter_get_offset(&iter);
		
		while((--position >= 0) && !found)
		{
			if (text[position] == '{')
				if (cpt-- == 0)
					found = TRUE;
			if (text[position] == '}')
				cpt++;
		}			
		if (found)
		{
			gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(sv->priv->document),
									           &start_iter, position + 2);
			gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(sv->priv->document),
									     &start_iter);
			end_iter = start_iter;
			found = gtk_source_iter_find_matching_bracket (&end_iter);
			if (found)
				set_select(sv, &start_iter, &end_iter);
		}
		g_free(text);
	}

}


/* Select range between start and end */
static void iselect_set(IAnjutaEditorSelection *editor, gint start, 
								  gint end, gboolean backward, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	GtkTextIter start_iter;
	GtkTextIter end_iter;

	if (backward)
	{
		gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(sv->priv->document),
									   &start_iter, start);
		gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(sv->priv->document),
									   &end_iter, end);
	}	
	else
	{
		gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(sv->priv->document),
									   &start_iter, end);
		gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(sv->priv->document),
									   &end_iter, start);
	}
	set_select(sv, &start_iter, &end_iter);
}

static void
iselect_all(IAnjutaEditorSelection* edit, GError** e)
{	
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	anjuta_view_select_all(sv->priv->view);
}

/* Return a newly allocated pointer containing selected text or
NULL if no text is selected */
static gchar* iselect_get(IAnjutaEditorSelection* editor, GError **e)
{
	GtkTextIter start_iter;
	GtkTextIter end_iter;
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	
	if (gtk_text_buffer_get_selection_bounds(GTK_TEXT_BUFFER(sv->priv->document),
										 &start_iter, &end_iter))
	{
		return gtk_text_buffer_get_text(GTK_TEXT_BUFFER(sv->priv->document),
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
	
	gtk_text_buffer_get_iter_at_mark(GTK_TEXT_BUFFER(sv->priv->document),
									 &end_iter,
									 gtk_text_buffer_get_insert(GTK_TEXT_BUFFER(sv->priv->document)));
	return gtk_text_iter_get_offset(&end_iter);
}

static void iselect_function(IAnjutaEditorSelection *editor, GError **e)
{
	// TODO
}

/* If text is selected, replace with given text */
static void iselect_replace(IAnjutaEditorSelection* editor, 
								  const gchar* text, gint length, GError **e)
{
	GtkTextIter start_iter;
	GtkTextIter end_iter;
	GtkTextIter iter;
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	gint position;
	
	if (gtk_text_buffer_get_selection_bounds(GTK_TEXT_BUFFER(sv->priv->document),
										 &start_iter, &end_iter))
	{
		position = gtk_text_iter_get_offset(&start_iter);
		gtk_text_buffer_delete_selection(GTK_TEXT_BUFFER(sv->priv->document),
										 FALSE, TRUE);	
		gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(sv->priv->document),
                                           &iter, position);		
		gtk_text_buffer_insert(GTK_TEXT_BUFFER(sv->priv->document),
							   &iter, text, length);	
	}
}

static void 
iselect_cut(IAnjutaEditorSelection* edit, GError** ee)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	anjuta_view_cut_clipboard(sv->priv->view);
}

static void 
iselect_copy(IAnjutaEditorSelection* edit, GError** ee)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	anjuta_view_copy_clipboard(sv->priv->view);
}

static void 
iselect_paste(IAnjutaEditorSelection* edit, GError** ee)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	anjuta_view_paste_clipboard(sv->priv->view);
}

static void 
iselect_clear(IAnjutaEditorSelection* edit, GError** ee)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	anjuta_view_delete_selection(sv->priv->view);
}

static void
iselect_iface_init(IAnjutaEditorSelectionIface *iface)
{
	iface->set = iselect_set;
	iface->get_start = iselect_get_start;
	iface->get_end = iselect_get_end;
	iface->select_block = iselect_block;
	iface->select_function = iselect_function;
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

/* IAnjutaEditorConvert Interface */

static void
iconvert_to_upper(IAnjutaEditorConvert* edit, gint start_pos, gint end_pos, GError** e)
{
	gchar *buffer;
	
	buffer = iselect_get(IANJUTA_EDITOR_SELECTION(edit), e);
	if (buffer)
	{
		buffer = g_utf8_strup(buffer, strlen(buffer));
		iselect_replace(IANJUTA_EDITOR_SELECTION(edit), buffer, strlen(buffer), e);	
		g_free(buffer);
	}

}

static void
iconvert_to_lower(IAnjutaEditorConvert* edit, gint start_pos, gint end_pos, GError** e)
{
	gchar *buffer;
	
	buffer = iselect_get(IANJUTA_EDITOR_SELECTION(edit), e);
	if (buffer)
	{
		buffer = g_utf8_strdown(buffer, strlen(buffer));
		iselect_replace(IANJUTA_EDITOR_SELECTION(edit), buffer, strlen(buffer), e);	
		g_free(buffer);
	}
}

static void
iconvert_iface_init(IAnjutaEditorConvertIface* iface)
{
	iface->to_upper = iconvert_to_upper;
	iface->to_lower = iconvert_to_lower;
}


/* IAnjutaAssist Interface */
static void 
iassist_autocomplete(IAnjutaEditorAssist* edit, GError** ee)
{
    Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
    sourceview_autocomplete(sv);
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
	gint location;
	IAnjutaMarkableMarker type;
} SVMarker;

static gint
imark_mark(IAnjutaMarkable* mark, gint location, IAnjutaMarkableMarker marker,
		   GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(mark);
	GtkTextIter iter;
	GtkSourceMarker* source_marker;
	gchar* name;
	
	gtk_text_buffer_get_iter_at_line(GTK_TEXT_BUFFER(sv->priv->document),
									 &iter, location  - 1);
	switch (marker)
	{
		case IANJUTA_MARKABLE_NONE:
			name = MARKER_NONE;
			break;
		case IANJUTA_MARKABLE_BASIC:
			name = MARKER_BASIC;
			break;
		case IANJUTA_MARKABLE_LIGHT:
			name = MARKER_LIGHT;
			break;
		case IANJUTA_MARKABLE_INTENSE:
			name = MARKER_INTENSE;
			break;
		case IANJUTA_MARKABLE_ATTENTIVE:
			name = MARKER_ATTENTIVE;
			break;
		default:
			DEBUG_PRINT("Unkonown marker type: %d!", marker);
			name = MARKER_NONE;
	}
		
	source_marker = gtk_source_buffer_create_marker(GTK_SOURCE_BUFFER(sv->priv->document), 
													NULL, name, &iter);
	SVMarker* sv_marker = g_new0(SVMarker, 1);
	sv_marker->handle = sv->priv->marker_count++;
	sv_marker->marker = source_marker;
	sv_marker->location = location;
	sv_marker->type = marker;
	
	sv->priv->markers = g_list_append(sv->priv->markers, sv_marker);
	
	return sv_marker->handle;
}

static void
imark_unmark(IAnjutaMarkable* mark, gint location, IAnjutaMarkableMarker marker,
			 GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(mark);
	GList* node = sv->priv->markers;
	while (node)
	{
		SVMarker* sv_marker = node->data;
		if (sv_marker->location == location)
		{
			gtk_source_buffer_delete_marker(GTK_SOURCE_BUFFER(sv->priv->document),
											sv_marker->marker);
			sv->priv->markers = g_list_remove(sv->priv->markers, node);
			return;
		}
		node = g_list_next(node);
	}	
}

static gboolean
imark_is_marker_set(IAnjutaMarkable* mark, gint location, 
					IAnjutaMarkableMarker marker, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(mark);
	GList* node = sv->priv->markers;
	while (node)
	{
		SVMarker* sv_marker = node->data;
		if (sv_marker->location == location)
		{
			return TRUE;
		}
		node = g_list_next(node);
	}
	return FALSE;
}

static gint
imark_location_from_handle(IAnjutaMarkable* mark, gint handle, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(mark);
	GList* node = sv->priv->markers;
	while (node)
	{
		SVMarker* sv_marker = node->data;
		if (sv_marker->handle == handle)
		{
			return sv_marker->location;
		}
		node = g_list_next(node);
	}
	return FALSE;
}

static void
imark_delete_all_markers(IAnjutaMarkable* mark, IAnjutaMarkableMarker marker,
						 GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(mark);
	GList* node = sv->priv->markers;
	while (node)
	{
		SVMarker* sv_marker = node->data;
		if (sv_marker->type == marker)
		{
			gtk_source_buffer_delete_marker(GTK_SOURCE_BUFFER(sv->priv->document),
											sv_marker->marker);
			sv->priv->markers = g_list_delete_link(sv->priv->markers, node);
			node = sv->priv->markers;
		}
		node = g_list_next(node);
	}
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

/* IanjutaIndic Interface */


static void
iindic_clear (IAnjutaIndicable *indic, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(indic);
	GtkTextIter start, end;
	
	gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER(sv->priv->document), 
	                                    &start, 0);
	gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER(sv->priv->document), 
	                                    &end, -1);
	gtk_text_buffer_remove_tag_by_name (GTK_TEXT_BUFFER(sv->priv->document),
	                                     IMPORTANT_INDIC,
	                                     &start, &end);
	// Remove warning_indic, critical_indic
}

static void
iindic_set (IAnjutaIndicable *indic, gint begin_location, gint end_location, 
            IAnjutaIndicableIndicator indicator, GError **e)
{
	GtkTextTag *tag = NULL;
	GtkTextIter start, end;
	Sourceview* sv = ANJUTA_SOURCEVIEW(indic);
	
	switch (indicator)
	{
		case IANJUTA_INDICABLE_IMPORTANT : 
			tag = sv->priv->important_indic;
			break;
		case IANJUTA_INDICABLE_WARNING :
			tag = sv->priv->warning_indic;
			break;
		case IANJUTA_INDICABLE_CRITICAL :
			tag = sv->priv->critical_indic;
			break;
		default:
			return;
	}

	gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER(sv->priv->document), 
	                                    &start, begin_location);
	gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER(sv->priv->document), 
	                                    &end, end_location);
	gtk_text_buffer_apply_tag (GTK_TEXT_BUFFER(sv->priv->document), tag, 
	                           &start, &end);
}

static void
iindic_iface_init(IAnjutaIndicableIface* iface)
{
	iface->clear = iindic_clear;
	iface->set = iindic_set;
}


ANJUTA_TYPE_BEGIN(Sourceview, sourceview, GTK_TYPE_SCROLLED_WINDOW);
ANJUTA_TYPE_ADD_INTERFACE(ifile, IANJUTA_TYPE_FILE);
ANJUTA_TYPE_ADD_INTERFACE(isavable, IANJUTA_TYPE_FILE_SAVABLE);
ANJUTA_TYPE_ADD_INTERFACE(ieditor, IANJUTA_TYPE_EDITOR);
ANJUTA_TYPE_ADD_INTERFACE(imark, IANJUTA_TYPE_MARKABLE);
ANJUTA_TYPE_ADD_INTERFACE(iindic, IANJUTA_TYPE_INDICABLE);
ANJUTA_TYPE_ADD_INTERFACE(iselect, IANJUTA_TYPE_EDITOR_SELECTION);
ANJUTA_TYPE_ADD_INTERFACE(iassist, IANJUTA_TYPE_EDITOR_ASSIST);
ANJUTA_TYPE_ADD_INTERFACE(iconvert, IANJUTA_TYPE_EDITOR_CONVERT);
ANJUTA_TYPE_END;
