/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-encodings.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-language-provider.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-savable.h>
#include <libanjuta/interfaces/ianjuta-markable.h>
#include <libanjuta/interfaces/ianjuta-indicable.h>
#include <libanjuta/interfaces/ianjuta-print.h>
#include <libanjuta/interfaces/ianjuta-document.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>
#include <libanjuta/interfaces/ianjuta-editor-assist.h>
#include <libanjuta/interfaces/ianjuta-editor-tip.h>
#include <libanjuta/interfaces/ianjuta-editor-convert.h>
#include <libanjuta/interfaces/ianjuta-editor-language.h>
#include <libanjuta/interfaces/ianjuta-editor-search.h>
#include <libanjuta/interfaces/ianjuta-editor-hover.h>
#include <libanjuta/interfaces/ianjuta-editor-glade-signal.h>
#include <libanjuta/interfaces/ianjuta-language-provider.h>
#include <libanjuta/interfaces/ianjuta-provider.h>

#include <gtksourceview/gtksource.h>

#include "config.h"
#include "anjuta-view.h"

#include "sourceview.h"
#include "sourceview-io.h"
#include "sourceview-private.h"
#include "sourceview-prefs.h"
#include "sourceview-print.h"
#include "sourceview-cell.h"
#include "sourceview-provider.h"
#include "plugin.h"

#define FORWARD 	0
#define BACKWARD 	1

#define MONITOR_KEY "sourceview.enable.vfs"

#define LOCATION_TO_LINE(o) ((o) - 1)
#define LINE_TO_LOCATION(o) ((o) + 1)

#define MARK_NAME "anjuta-mark-"
#define CREATE_MARK_NAME(o) (g_strdup_printf (MARK_NAME "%d", (o)))

static void sourceview_class_init(SourceviewClass *klass);
static void sourceview_instance_init(Sourceview *sv);
static void sourceview_dispose(GObject *object);

static GObjectClass *parent_class = NULL;

static gboolean on_sourceview_hover_over (GtkWidget *widget, gint x, gint y,
										  gboolean keyboard_tip, GtkTooltip *tooltip,
										  gpointer data);

/* Utils */
/* Sync with IANJUTA_MARKABLE_MARKER  */

#define MARKER_PIXMAP_LINEMARKER "anjuta-linemark-16.png"
#define MARKER_PIXMAP_PROGRAM_COUNTER "anjuta-pcmark-16.png"
#define MARKER_PIXMAP_MESSAGE "anjuta-message-16.png"
#define MARKER_PIXMAP_BREAKPOINT_DISABLED "anjuta-breakpoint-disabled-16.png"
#define MARKER_PIXMAP_BREAKPOINT_ENABLED "anjuta-breakpoint-enabled-16.png"
#define MARKER_PIXMAP_BOOKMARK "anjuta-bookmark-16.png"

#define MARKER_TOOLTIP_DATA "__tooltip"

/* Keep in sync with IAnjutaMarkableMarker */

static const gchar* marker_types [] =
{
	"sv-linemarker",
	"sv-bookmark",
	"sv-message",
	"sv-breakpoint-enabled",
	"sv-breakpoint-disabled",
	"sv-program-counter",
	NULL
};

typedef struct
{
	gint handle;
	gint line;
	const gchar* category;
} MarkerReload;

/* HIGHLIGHTED TAGS */

#define IMPORTANT_INDIC "important_indic"
#define WARNING_INDIC "warning_indic"
#define CRITICAL_INDIC "critical_indic"

static GtkWidget *
anjuta_message_area_new (const gchar    *text,
                         GtkMessageType  type)
{
	GtkInfoBar *message_area;
	GtkWidget *content_area;
	GtkWidget *message_label = gtk_label_new ("");

	gtk_label_set_line_wrap (GTK_LABEL (message_label), TRUE);
	gtk_label_set_line_wrap_mode (GTK_LABEL (message_label), PANGO_WRAP_WORD);
	
	message_area = GTK_INFO_BAR (gtk_info_bar_new ());
	gtk_info_bar_set_message_type (message_area, type);
	content_area = gtk_info_bar_get_content_area (GTK_INFO_BAR (message_area));
	gtk_widget_show (message_label);
	gtk_container_add (GTK_CONTAINER (content_area), message_label);

	gchar *markup = g_strdup_printf ("<b>%s</b>", text);
	gtk_label_set_markup (GTK_LABEL (message_label), markup);
	g_free (markup);

	return GTK_WIDGET (message_area);
}

static gchar*
on_marker_tooltip (GtkSourceMarkAttributes* cat, GtkSourceMark* mark, gpointer data)
{
	//Sourceview* sv = ANJUTA_SOURCEVIEW (data);
	gchar* tooltip;
	tooltip = g_object_get_data (G_OBJECT (mark), MARKER_TOOLTIP_DATA);
	if (tooltip)
		return g_strdup (tooltip);
	else
		return NULL;
}

static void
sourceview_create_marker_category (Sourceview* sv, const gchar* marker_pixbuf,
                                   IAnjutaMarkableMarker marker_type)
{
	GdkPixbuf * pixbuf;
	GtkSourceView* view = 	GTK_SOURCE_VIEW(sv->priv->view);
	if ((pixbuf = gdk_pixbuf_new_from_file (marker_pixbuf, NULL)))
	{
		GtkSourceMarkAttributes* cat = gtk_source_mark_attributes_new ();
		gtk_source_mark_attributes_set_pixbuf (cat, pixbuf);
		g_signal_connect (cat, "query-tooltip-text", G_CALLBACK (on_marker_tooltip), sv);
		gtk_source_view_set_mark_attributes (view, marker_types [marker_type], cat, marker_type);
		g_object_unref (pixbuf);
	}
}

/* Create pixmaps for the markers */
static void sourceview_create_markers(Sourceview* sv)
{
	sourceview_create_marker_category (sv, PACKAGE_PIXMAPS_DIR"/"MARKER_PIXMAP_BOOKMARK,
	                                   IANJUTA_MARKABLE_BOOKMARK);
	sourceview_create_marker_category (sv, PACKAGE_PIXMAPS_DIR"/"MARKER_PIXMAP_BREAKPOINT_DISABLED,
	                                   IANJUTA_MARKABLE_BREAKPOINT_DISABLED);
	sourceview_create_marker_category (sv, PACKAGE_PIXMAPS_DIR"/"MARKER_PIXMAP_BREAKPOINT_ENABLED,
	                                   IANJUTA_MARKABLE_BREAKPOINT_ENABLED);
	sourceview_create_marker_category (sv, PACKAGE_PIXMAPS_DIR"/"MARKER_PIXMAP_PROGRAM_COUNTER,
	                                   IANJUTA_MARKABLE_PROGRAM_COUNTER);
	sourceview_create_marker_category (sv, PACKAGE_PIXMAPS_DIR"/"MARKER_PIXMAP_LINEMARKER,
	                                   IANJUTA_MARKABLE_LINEMARKER);
	sourceview_create_marker_category (sv, PACKAGE_PIXMAPS_DIR"/"MARKER_PIXMAP_MESSAGE,
	                                   IANJUTA_MARKABLE_MESSAGE);
}

#define PREF_COLOR_ERROR "color-error"
#define PREF_COLOR_WARNING "color-warning"
#define PREF_COLOR_IMPORTANT "color-important"


/* Create tags for highlighting */
static void sourceview_create_highlight_indic(Sourceview* sv)
{
	char* error_color =
		g_settings_get_string (sv->priv->msgman_settings,
		                       PREF_COLOR_ERROR);
	char* warning_color =
		g_settings_get_string (sv->priv->msgman_settings,
		                       PREF_COLOR_WARNING);
	char* important_color =
		g_settings_get_string (sv->priv->msgman_settings,
		                       PREF_COLOR_IMPORTANT);
	sv->priv->important_indic =
		gtk_text_buffer_create_tag (GTK_TEXT_BUFFER(sv->priv->document),
									IMPORTANT_INDIC,
									"background", important_color, NULL);
	sv->priv->warning_indic =
		gtk_text_buffer_create_tag (GTK_TEXT_BUFFER(sv->priv->document),
									WARNING_INDIC,
									"foreground", warning_color,
									"underline", PANGO_UNDERLINE_SINGLE,
									NULL);
	sv->priv->critical_indic =
		gtk_text_buffer_create_tag (GTK_TEXT_BUFFER(sv->priv->document),
									CRITICAL_INDIC,
									"foreground", error_color, "underline",
									PANGO_UNDERLINE_ERROR, NULL);
	g_free (error_color);
	g_free (warning_color);
	g_free (important_color);
}

static void
goto_line (Sourceview* sv, gint line)
{
	GtkTextIter iter;
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER (sv->priv->document);

	gtk_text_buffer_get_iter_at_line (buffer,
									  &iter,
									  line);
	gtk_text_buffer_select_range (buffer, &iter, &iter);
}

static void
on_destroy_message_area (Sourceview* sv, GObject *finalized_object)
{
	sv->priv->message_area = NULL;
	g_signal_emit_by_name (G_OBJECT (sv), "update-save-ui");
}

static void
sourceview_set_message_area (Sourceview* sv,  GtkWidget *message_area)
{
	if (sv->priv->message_area != NULL)
		gtk_widget_destroy (sv->priv->message_area);
	sv->priv->message_area = message_area;

	if (sv->priv->message_area == NULL)
		return;

	gtk_widget_show (message_area);
	gtk_box_pack_start (GTK_BOX (sv),
						message_area,
						FALSE,
						FALSE,
						0);
	g_object_weak_ref (G_OBJECT (sv->priv->message_area),
					   (GWeakNotify)on_destroy_message_area, sv);

	g_signal_emit_by_name (G_OBJECT (sv), "update-save-ui");
}

/* Callbacks */
static void
on_assist_tip_destroyed (Sourceview* sv, gpointer where_object_was)
{
	sv->priv->assist_tip = NULL;
}

static void
on_insert_text (GtkTextBuffer *buffer,
                GtkTextIter   *location,
                gchar         *text,
                gint           len,
                Sourceview    *sv)
{
	int i = 0, lines = 0;
	gchar* signal_text;
	SourceviewCell *cell = sourceview_cell_new (location, GTK_TEXT_VIEW (sv->priv->view));
	IAnjutaIterable *iter = ianjuta_iterable_clone (IANJUTA_ITERABLE (cell), NULL);
	GtkTextMark *mark = gtk_text_buffer_create_mark (buffer, NULL, location, TRUE);
	g_object_unref (cell);

	ianjuta_iterable_set_position (iter,
	                               ianjuta_iterable_get_position (iter, NULL) - len,
	                               NULL);

	/* Update the status bar */
	g_signal_emit_by_name (G_OBJECT (sv), "update-ui");

	if (len <= 1 && strlen (text) <= 1)
	{
		/* Send the "char-added" signal and revalidate the iterator */
		g_signal_emit_by_name (G_OBJECT (sv), "char-added", iter, text[0]);
		gtk_text_buffer_get_iter_at_mark (buffer, location, mark);
	}

	for (i = 0; i < len; i ++)
		if (text[i] == '\n')
			lines ++;

	/* text might not be NULL-terminated, so make sure to fix that. */
	signal_text = g_strndup (text, len);
	/* Send the "changed" signal and revalidate the iterator */
	g_signal_emit_by_name (G_OBJECT (sv), "changed", iter, TRUE, len, lines, signal_text);
	g_free (signal_text);

	gtk_text_buffer_get_iter_at_mark (buffer, location, mark);
}

static void
on_delete_range (GtkTextBuffer *buffer,
                 GtkTextIter   *start_iter,
                 GtkTextIter   *end_iter,
                 gpointer       user_data)
{
	Sourceview *sv = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SOURCEVIEW (user_data));
	sv = ANJUTA_SOURCEVIEW (user_data);

	sv->priv->deleted_text = gtk_text_buffer_get_text (buffer, start_iter, end_iter, TRUE);

}

static void
on_delete_range_after (GtkTextBuffer *buffer,
                       GtkTextIter   *start_iter,
                       GtkTextIter   *end_iter,
                       gpointer       user_data)
{
	Sourceview *sv = NULL;
	GtkTextMark *start_mark = NULL, *end_mark = NULL;
	SourceviewCell *cell = NULL;
	IAnjutaIterable *position = NULL;
	gint length = 0, i = 0, lines = 0;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SOURCEVIEW (user_data));
	sv = ANJUTA_SOURCEVIEW (user_data);

	/* Get the start iterator of the changed text */
	cell = sourceview_cell_new (start_iter, GTK_TEXT_VIEW (sv->priv->view));
	position = IANJUTA_ITERABLE (cell);

	/* We save the text before the default handler */
	length = g_utf8_strlen (sv->priv->deleted_text, -1);
	for (i = 0; i < length; i ++)
		if (sv->priv->deleted_text[i] == '\n')
			lines ++;

	/* Save the iterators */
	start_mark = gtk_text_buffer_create_mark (buffer, NULL, start_iter, TRUE);
	end_mark   = gtk_text_buffer_create_mark (buffer, NULL, end_iter, TRUE);

	g_signal_emit_by_name (G_OBJECT (sv), "changed",
	                       position, FALSE, length, lines, sv->priv->deleted_text);

	/* Revalidate the iterators */
	gtk_text_buffer_get_iter_at_mark (buffer, start_iter, start_mark);
	gtk_text_buffer_get_iter_at_mark (buffer, end_iter, end_mark);

	/* Delete the saved text */
	g_free (sv->priv->deleted_text);
	sv->priv->deleted_text = NULL;

}

static void
on_cursor_position_changed (GObject    *buffer_obj,
                            GParamSpec *param_spec,
                            gpointer    user_data)
{

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SOURCEVIEW (user_data));

	g_signal_emit_by_name (G_OBJECT (user_data), "cursor-moved");

}

/* Called whenever the document is changed */
static void on_document_modified_changed(GtkTextBuffer* buffer, Sourceview* sv)
{
	/* Emit IAnjutaFileSavable signals */
	g_signal_emit_by_name(G_OBJECT(sv), "update-save-ui",
						  !gtk_text_buffer_get_modified(buffer));
}

/* Update document status */
static void on_overwrite_toggled (GtkTextView* view,
								  Sourceview* sv)
{
	g_signal_emit_by_name(G_OBJECT(sv), "update_ui");
}


static void on_mark_set (GtkTextBuffer *buffer,
						 GtkTextIter* location,
						 GtkTextMark* mark,
						 Sourceview* sv)
{
	/* Emit IAnjutaEditor signal */
	if (mark == gtk_text_buffer_get_insert (buffer))
		g_signal_emit_by_name(G_OBJECT(sv), "update_ui");
}

static void on_backspace (GtkTextView* view,
						  Sourceview* sv)
{
	g_signal_emit_by_name(G_OBJECT(sv), "backspace");
	g_signal_emit_by_name(G_OBJECT(sv), "update_ui");
}

static void
on_line_mark_activated(GtkTextView* view,
                       GtkTextIter   *iter,
                       GdkEventButton *event,
                       Sourceview* sv)
{
	/* proceed only if its a double click with left button*/
	if( (event->button != 1) || (GDK_2BUTTON_PRESS != event->type) ) {
		return;
	}

	/* line number starts with 0, so add 1 */
	gint line_number = LINE_TO_LOCATION(gtk_text_iter_get_line(iter));

	if (!IANJUTA_IS_EDITOR(sv))
	{
		return;
	}
	g_signal_emit_by_name(G_OBJECT(sv), "line-marks-gutter-clicked", line_number);
}


/* Open / Save stuff */

static void
sourceview_reload_save_markers (Sourceview* sv)
{
	GSList* marks;
	GtkTextIter begin;
	GtkTextIter end;
	GtkTextIter* iter;
	GtkSourceMark* source_mark;

	gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (sv->priv->document), &begin, 0);
	gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (sv->priv->document), &end, -1);

	if (!gtk_source_buffer_forward_iter_to_source_mark (GTK_SOURCE_BUFFER (sv->priv->document),
	                                               &begin, NULL))
		return;

	iter = gtk_text_iter_copy (&begin);
	marks = gtk_source_buffer_get_source_marks_at_iter (GTK_SOURCE_BUFFER (sv->priv->document),
	                                                    iter, NULL);
	source_mark = marks->data;
	g_slist_free (marks);

	do
	{
		MarkerReload* reload = g_new0(MarkerReload, 1);

		gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (sv->priv->document),
		                                  iter, GTK_TEXT_MARK (source_mark));
		reload->line = gtk_text_iter_get_line (iter);
		reload->category = gtk_source_mark_get_category (source_mark);

		sscanf (gtk_text_mark_get_name (GTK_TEXT_MARK (source_mark)),
		        MARK_NAME "%d", &reload->handle);
		sv->priv->reload_marks = g_slist_append (sv->priv->reload_marks, reload);
	}
	while ((source_mark = gtk_source_mark_next (source_mark, NULL)));

	gtk_source_buffer_remove_source_marks (GTK_SOURCE_BUFFER (sv->priv->document), &begin, &end, NULL);
	gtk_text_iter_free (iter);
}

static void
sourceview_reload_restore_markers (Sourceview* sv)
{
	GSList* cur_mark;
	for (cur_mark = sv->priv->reload_marks; cur_mark != NULL;
	     cur_mark = g_slist_next (cur_mark))
	{
		MarkerReload* mark = cur_mark->data;
		GtkTextIter iter;
		gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (sv->priv->document),
		                                  &iter,
		                                  mark->line);
		gtk_source_buffer_create_source_mark (GTK_SOURCE_BUFFER (sv->priv->document),
		                                      CREATE_MARK_NAME (mark->handle),
		                                      mark->category, &iter);
	}
	g_slist_foreach (sv->priv->reload_marks, (GFunc)g_free, NULL);
	g_slist_free (sv->priv->reload_marks);
	sv->priv->reload_marks = NULL;
}

/* Callback for dialog below */
static void
on_reload_dialog_response (GtkWidget *message_area, gint res, Sourceview *sv)
{
	if (res == GTK_RESPONSE_YES)
	{
		GFile* file = sourceview_io_get_file (sv->priv->io);

		/* Save marks and position */
		sv->priv->goto_line =
			LOCATION_TO_LINE(ianjuta_editor_get_lineno (IANJUTA_EDITOR(sv), NULL));
		sourceview_reload_save_markers (sv);

		ianjuta_file_open(IANJUTA_FILE(sv),
						  file, NULL);
		g_object_unref (file);

	}
	else
	{
		/* Set dirty */
		gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(sv->priv->document), TRUE);
	}
	gtk_widget_destroy (message_area);
}

static void
on_close_dialog_response (GtkWidget *message_area, gint res, Sourceview *sv)
{
	if (res == GTK_RESPONSE_YES)
	{
		IAnjutaDocumentManager *docman;

		docman = anjuta_shell_get_interface (sv->priv->plugin->shell, IAnjutaDocumentManager, NULL);
		if (docman == NULL) return;

		ianjuta_document_manager_remove_document (docman, IANJUTA_DOCUMENT (sv), FALSE, NULL);
	}
	else
	{
		/* Set dirty */
		gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(sv->priv->document), TRUE);
	}
	gtk_widget_destroy (message_area);
}

static gboolean
on_file_changed (SourceviewIO* sio, Sourceview* sv)
{
	GtkWidget *message_area;
	gchar *buff;

	const gchar* filename = sourceview_io_get_filename (sio);

	buff =
		g_strdup_printf (_
						 ("The file \"%s\" on the disk is more recent than "
						  "the current buffer.\nDo you want to reload it?"),
						 filename);

	message_area = anjuta_message_area_new (buff, GTK_MESSAGE_WARNING);
	gtk_info_bar_add_button (GTK_INFO_BAR (message_area),
									GTK_STOCK_REFRESH,
									GTK_RESPONSE_YES);
	gtk_info_bar_add_button (GTK_INFO_BAR (message_area),
								    GTK_STOCK_CANCEL,
									GTK_RESPONSE_NO);
	g_free (buff);

	g_signal_connect (G_OBJECT(message_area), "response",
					  G_CALLBACK (on_reload_dialog_response),
					  sv);

	sourceview_set_message_area (sv, message_area);

	return FALSE;
}

static gboolean
on_file_deleted (SourceviewIO* sio, Sourceview* sv)
{
	GtkWidget *message_area;
	gchar *buff;

	const gchar* filename = sourceview_io_get_filename (sio);

	buff =
		g_strdup_printf (_
						 ("The file \"%s\" has been deleted on the disk.\n"
						  "Do you want to close it?"),
						 filename);

	message_area = anjuta_message_area_new (buff, GTK_MESSAGE_WARNING);
	gtk_info_bar_add_button (GTK_INFO_BAR (message_area),
									GTK_STOCK_DELETE,
									GTK_RESPONSE_YES);
	gtk_info_bar_add_button (GTK_INFO_BAR (message_area),
								    GTK_STOCK_CANCEL,
									GTK_RESPONSE_NO);
	g_free (buff);

	g_signal_connect (G_OBJECT(message_area), "response",
					  G_CALLBACK (on_close_dialog_response),
					  sv);

	sourceview_set_message_area (sv, message_area);

	return FALSE;
}

static void
on_open_failed (SourceviewIO* io, GError* err, Sourceview* sv)
{
	AnjutaShell* shell = ANJUTA_PLUGIN (sv->priv->plugin)->shell;
	IAnjutaDocumentManager *docman =
		anjuta_shell_get_interface (shell, IAnjutaDocumentManager, NULL);
	g_return_if_fail (docman != NULL);
	GList* documents = ianjuta_document_manager_get_doc_widgets (docman, NULL);
	GtkWidget* message_area;

	/* Could not open <filename>: <error message> */
	gchar* message = g_strdup_printf (_("Could not open %s: %s"),
									  sourceview_io_get_filename (sv->priv->io),
									  err->message);

	if (g_list_find (documents, sv))
	{
		message_area = anjuta_message_area_new (message, GTK_MESSAGE_WARNING);
		gtk_info_bar_add_button (GTK_INFO_BAR (message_area),
										GTK_STOCK_OK,
										GTK_RESPONSE_OK);
		g_signal_connect (message_area, "response", G_CALLBACK(gtk_widget_destroy), NULL);

		sourceview_set_message_area (sv, message_area);
	}
	else
	{
		GtkWidget* dialog = gtk_message_dialog_new (NULL, 0,
													GTK_MESSAGE_ERROR,
													GTK_BUTTONS_OK,
													"%s", message);
		g_signal_connect (dialog, "response", G_CALLBACK(gtk_widget_destroy), NULL);
		gtk_dialog_run (GTK_DIALOG (dialog));
	}
	g_free (message);
	sv->priv->loading = FALSE;
	gtk_text_view_set_editable (GTK_TEXT_VIEW (sv->priv->view), TRUE);

	/* Get rid of reference from ifile_open */
	g_object_unref(G_OBJECT(sv));
}

static void
on_read_only_dialog_response (GtkWidget *message_area, gint res, Sourceview *sv)
{
	if (res == GTK_RESPONSE_YES)
	{
		gtk_text_view_set_editable (GTK_TEXT_VIEW (sv->priv->view),
									TRUE);
		sv->priv->read_only = FALSE;
	}
	gtk_widget_destroy (message_area);
}

/* Called when document is loaded completly */
static void
on_open_finish(SourceviewIO* io, Sourceview* sv)
{
	const gchar *lang;

	gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(sv->priv->document), FALSE);

	if (sourceview_io_get_read_only (io))
	{
		const gchar* filename = sourceview_io_get_filename (io);
		gchar* buff = g_strdup_printf (_("The file \"%s\" is read-only! Edit anyway?"),
									   filename);
		GtkWidget* message_area;

		message_area = anjuta_message_area_new (buff, GTK_MESSAGE_WARNING);
		gtk_info_bar_add_button (GTK_INFO_BAR (message_area),
										GTK_STOCK_YES,
										GTK_RESPONSE_YES);
		gtk_info_bar_add_button (GTK_INFO_BAR (message_area),
										GTK_STOCK_NO,
										GTK_RESPONSE_NO);
		g_free (buff);

		g_signal_connect (G_OBJECT(message_area), "response",
						  G_CALLBACK (on_read_only_dialog_response),
						  sv);

		sv->priv->read_only = TRUE;

		sourceview_set_message_area (sv, message_area);
	}
	else
		gtk_text_view_set_editable (GTK_TEXT_VIEW (sv->priv->view), TRUE);

    g_signal_emit_by_name(G_OBJECT(sv), "update-save-ui");

	sourceview_reload_restore_markers (sv);

	if (sv->priv->goto_line > 0)
	{
		goto_line (sv, sv->priv->goto_line);
		sv->priv->goto_line = -1;
	}
	else
		goto_line (sv, 0);
	anjuta_view_scroll_to_cursor(sv->priv->view);
	sv->priv->loading = FALSE;

	/* Autodetect language */
	ianjuta_editor_language_set_language(IANJUTA_EDITOR_LANGUAGE(sv), NULL, NULL);

	lang = ianjuta_editor_language_get_language(IANJUTA_EDITOR_LANGUAGE(sv), NULL);
	g_signal_emit_by_name (sv, "language-changed", lang);

	g_signal_emit_by_name (sv, "opened");
	
	/* Get rid of reference from ifile_open */
	g_object_unref(G_OBJECT(sv));
}

static void on_save_failed (SourceviewIO* sio, GError* err, Sourceview* sv)
{
	AnjutaShell* shell = ANJUTA_PLUGIN (sv->priv->plugin)->shell;
	IAnjutaDocumentManager *docman =
		anjuta_shell_get_interface (shell, IAnjutaDocumentManager, NULL);
	g_return_if_fail (docman != NULL);
	GList* documents = ianjuta_document_manager_get_doc_widgets (docman, NULL);
	GtkWidget* message_area;

	g_signal_emit_by_name(G_OBJECT(sv), "saved", NULL);

	/* Could not open <filename>: <error message> */
	gchar* message = g_strdup_printf (_("Could not save %s: %s"),
									  sourceview_io_get_filename (sv->priv->io),
									  err->message);

	if (g_list_find (documents, sv))
	{
		message_area = anjuta_message_area_new (message, GTK_MESSAGE_ERROR);
		gtk_info_bar_add_button (GTK_INFO_BAR (message_area),
										GTK_STOCK_OK,
										GTK_RESPONSE_OK);
		g_signal_connect (message_area, "response", G_CALLBACK(gtk_widget_destroy), NULL);

		sourceview_set_message_area (sv, message_area);
	}
	else
	{
		GtkWidget* dialog = gtk_message_dialog_new (NULL, 0,
													GTK_MESSAGE_ERROR,
													GTK_BUTTONS_OK,
													"%s", message);
		g_signal_connect (dialog, "response", G_CALLBACK(gtk_widget_destroy), NULL);
		gtk_dialog_run (GTK_DIALOG (dialog));
	}
	g_free (message);

	g_object_unref (sv);
}

/* Called when document is saved completly */
static void on_save_finish(SourceviewIO* sio, Sourceview* sv)
{
	const gchar* lang;
	GFile* file = sourceview_io_get_file(sio);
	gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(sv->priv->document), FALSE);
	sv->priv->read_only = FALSE;
	g_signal_emit_by_name(G_OBJECT(sv), "saved", file);
	g_signal_emit_by_name(G_OBJECT(sv), "update-save-ui");
	g_object_unref (file);
	/* Autodetect language */
	ianjuta_editor_language_set_language(IANJUTA_EDITOR_LANGUAGE(sv), NULL, NULL);
	lang = ianjuta_editor_language_get_language(IANJUTA_EDITOR_LANGUAGE(sv), NULL);
	g_signal_emit_by_name (sv, "language-changed", lang);
	g_object_unref (sv);
}

static void
sourceview_adjustment_changed(GtkAdjustment* ad, Sourceview* sv)
{
	/* Hide assistance windows when scrolling vertically */
	if (sv->priv->assist_tip)
		gtk_widget_destroy (GTK_WIDGET (sv->priv->assist_tip));
}

static void
sourceview_instance_init(Sourceview* sv)

{
	sv->priv = G_TYPE_INSTANCE_GET_PRIVATE (sv,
	                                        ANJUTA_TYPE_SOURCEVIEW,
	                                        SourceviewPrivate);
	sv->priv->io = sourceview_io_new (sv);
	g_signal_connect (sv->priv->io, "changed", G_CALLBACK (on_file_changed), sv);
	g_signal_connect (sv->priv->io, "deleted", G_CALLBACK (on_file_deleted), sv);
	g_signal_connect (sv->priv->io, "open-finished", G_CALLBACK (on_open_finish),
					  sv);
	g_signal_connect (sv->priv->io, "open-failed", G_CALLBACK (on_open_failed),
					  sv);
	g_signal_connect (sv->priv->io, "save-finished", G_CALLBACK (on_save_finish),
					  sv);
	g_signal_connect (sv->priv->io, "save-failed", G_CALLBACK (on_save_failed),
					  sv);

	/* Create buffer */
	sv->priv->document = gtk_source_buffer_new(NULL);
	
	g_signal_connect_after(G_OBJECT(sv->priv->document), "modified-changed",
					 G_CALLBACK(on_document_modified_changed), sv);
	g_signal_connect_after(G_OBJECT(sv->priv->document), "mark-set",
					 G_CALLBACK(on_mark_set),sv);
	g_signal_connect_after (G_OBJECT(sv->priv->document), "insert-text",
	                  G_CALLBACK(on_insert_text), sv);
	g_signal_connect (G_OBJECT(sv->priv->document), "delete-range",
	                  G_CALLBACK(on_delete_range), sv);
	g_signal_connect_after (G_OBJECT(sv->priv->document), "delete-range",
	                  G_CALLBACK(on_delete_range_after), sv);

	g_signal_connect (G_OBJECT (sv->priv->document), "notify::cursor-position",
	                  G_CALLBACK (on_cursor_position_changed), sv);
	
	/* Create View instance */
	sv->priv->view = ANJUTA_VIEW(anjuta_view_new(sv));
	/* The view doesn't take a reference on the buffer, we have to unref it */
	g_object_unref (sv->priv->document);
	
	g_signal_connect_after (G_OBJECT(sv->priv->view), "toggle-overwrite",
					  G_CALLBACK(on_overwrite_toggled), sv);
	g_signal_connect (G_OBJECT(sv->priv->view), "query-tooltip",
					  G_CALLBACK (on_sourceview_hover_over), sv);
	g_signal_connect_after(G_OBJECT(sv->priv->view), "backspace",
					 G_CALLBACK(on_backspace),sv);

	g_object_set (G_OBJECT (sv->priv->view), "has-tooltip", TRUE, NULL);
	
	/* Apply Preferences */
	sourceview_prefs_init(sv);
	
	/* Create Markers */
	sourceview_create_markers(sv);
	
	/* Create Higlight Tag */
	sourceview_create_highlight_indic(sv);
}

static void
sourceview_class_init(SourceviewClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	parent_class = g_type_class_peek_parent(klass);
	object_class->dispose = sourceview_dispose;

	g_type_class_add_private (klass, sizeof (SourceviewPrivate));
}

static void
sourceview_dispose(GObject *object)
{
	Sourceview *cobj = ANJUTA_SOURCEVIEW(object);
	GSList* node;

	for (node = cobj->priv->idle_sources; node != NULL; node = g_slist_next (node))
	{
		g_source_remove (GPOINTER_TO_UINT (node->data));
	}
	g_slist_free (cobj->priv->idle_sources);
	cobj->priv->idle_sources = NULL;	

	if (cobj->priv->assist_tip)
	{
		gtk_widget_destroy(GTK_WIDGET(cobj->priv->assist_tip));
		cobj->priv->assist_tip = NULL;
	}
	if (cobj->priv->io)
	{
		g_clear_object (&cobj->priv->io);
	}

	if (cobj->priv->tooltip_cell)
	{
		g_clear_object (&cobj->priv->tooltip_cell);
	}

	sourceview_prefs_destroy(cobj);
	
	G_OBJECT_CLASS (parent_class)->dispose (object);
}

/* Create a new sourceview instance. If uri is valid,
the file will be loaded in the buffer */

Sourceview *
sourceview_new(GFile* file, const gchar* filename, AnjutaPlugin* plugin)
{
	GtkAdjustment* v_adj;

	Sourceview *sv = ANJUTA_SOURCEVIEW(g_object_new(ANJUTA_TYPE_SOURCEVIEW, NULL));

	sv->priv->plugin = plugin;

	/* Add View */
	sv->priv->window = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_end (GTK_BOX (sv), sv->priv->window, TRUE, TRUE, 0);

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sv->priv->window),
				      GTK_POLICY_AUTOMATIC,
				      GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(sv->priv->window), GTK_WIDGET(sv->priv->view));
	gtk_widget_show_all(GTK_WIDGET(sv));
	v_adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (sv->priv->window));
	g_signal_connect (v_adj, "value-changed", G_CALLBACK (sourceview_adjustment_changed), sv);

	if (file != NULL)
	{
		ianjuta_file_open(IANJUTA_FILE(sv), file, NULL);
	}
	else if (filename != NULL && strlen(filename) > 0)
		sourceview_io_set_filename (sv->priv->io, filename);

	DEBUG_PRINT("%s", "============ Creating new editor =============");

	g_signal_emit_by_name (G_OBJECT(sv), "update-ui");
	g_signal_connect_after(G_OBJECT(sv->priv->view), "line-mark-activated",
					 G_CALLBACK(on_line_mark_activated),
					 G_OBJECT(sv)
					 );

	return sv;
}

/* IAnjutaFile interface */

/* Open uri in Editor */
static void
ifile_open (IAnjutaFile* ifile, GFile* file, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(ifile);
	/* Hold a reference here to avoid a destroyed editor */
	g_object_ref(G_OBJECT(sv));
	gtk_text_buffer_set_text (GTK_TEXT_BUFFER(sv->priv->document),
							  "",
							  0);
	gtk_text_view_set_editable (GTK_TEXT_VIEW (sv->priv->view),
								FALSE);
	sv->priv->loading = TRUE;
	sourceview_io_open (sv->priv->io, file);
}

/* Return the currently loaded uri */

static GFile*
ifile_get_file (IAnjutaFile* ifile, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(ifile);
	return sourceview_io_get_file (sv->priv->io);
}

/* IAnjutaFileSavable interface */

/* Save file */
static void
ifile_savable_save (IAnjutaFileSavable* file, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(file);

	g_object_ref(G_OBJECT(sv));
	sourceview_io_save (sv->priv->io);
}

/* Save file as */
static void
ifile_savable_save_as (IAnjutaFileSavable* ifile, GFile* file, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(ifile);

	g_object_ref(G_OBJECT(sv));
	sourceview_io_save_as (sv->priv->io, file);
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

static gboolean
ifile_savable_is_read_only (IAnjutaFileSavable* file, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW (file);
	return sv->priv->read_only;
}

static gboolean
ifile_savable_is_conflict (IAnjutaFileSavable* file, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW (file);
	return sv->priv->message_area != NULL;
}

static void
isavable_iface_init (IAnjutaFileSavableIface *iface)
{
	iface->save = ifile_savable_save;
	iface->save_as = ifile_savable_save_as;
	iface->set_dirty = ifile_savable_set_dirty;
	iface->is_dirty = ifile_savable_is_dirty;
	iface->is_read_only = ifile_savable_is_read_only;
	iface->is_conflict = ifile_savable_is_conflict;
}

static void
ifile_iface_init (IAnjutaFileIface *iface)
{
	iface->open = ifile_open;
	iface->get_file = ifile_get_file;
}

/* IAnjutaEditor interface */

static gint
ieditor_get_tab_size (IAnjutaEditor *editor, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	return gtk_source_view_get_tab_width (GTK_SOURCE_VIEW (sv->priv->view));
}

static void
ieditor_set_tab_size (IAnjutaEditor *editor, gint tabsize, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(sv->priv->view), tabsize);
	gtk_source_view_set_indent_width (GTK_SOURCE_VIEW (sv->priv->view), tabsize);
}

static gboolean
ieditor_get_use_spaces (IAnjutaEditor *editor, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	return gtk_source_view_get_insert_spaces_instead_of_tabs (GTK_SOURCE_VIEW(sv->priv->view));
}

static void
ieditor_set_use_spaces (IAnjutaEditor *editor, gboolean use_spaces, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	gtk_source_view_set_insert_spaces_instead_of_tabs(GTK_SOURCE_VIEW(sv->priv->view),
													  use_spaces);
}

static void
ieditor_set_auto_indent (IAnjutaEditor *editor, gboolean auto_indent, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	gtk_source_view_set_auto_indent(GTK_SOURCE_VIEW(sv->priv->view),
									auto_indent);
}


/* Scroll to line */
static void ieditor_goto_line(IAnjutaEditor *editor, gint line, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);

	if (!sv->priv->loading)
	{
		goto_line(sv, LOCATION_TO_LINE (line));
		anjuta_view_scroll_to_cursor(sv->priv->view);
		gtk_widget_grab_focus (GTK_WIDGET (sv->priv->view));
	}
	else
		sv->priv->goto_line = LOCATION_TO_LINE (line);
}

/* Scroll to position */
static void ieditor_goto_position(IAnjutaEditor *editor, IAnjutaIterable* icell,
								  GError **e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL (icell);
	GtkTextIter iter;
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	sourceview_cell_get_iter (cell, &iter);
	gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (sv->priv->document), &iter);
	gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (sv->priv->view),
								  &iter, 0, FALSE, 0, 0);
}

/* Return a newly allocated pointer containing the whole text */
static gchar* ieditor_get_text (IAnjutaEditor* editor,
								IAnjutaIterable* start,
								IAnjutaIterable* end, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);

	GtkTextIter start_iter;
	GtkTextIter end_iter;
	sourceview_cell_get_iter (SOURCEVIEW_CELL(start), &start_iter);
	sourceview_cell_get_iter (SOURCEVIEW_CELL(end), &end_iter);

	return gtk_text_buffer_get_slice(GTK_TEXT_BUFFER(sv->priv->document),
									&start_iter, &end_iter, TRUE);
}

static gchar*
ieditor_get_text_all (IAnjutaEditor* edit, GError **e)
{
	GtkTextIter start_iter;
	GtkTextIter end_iter;
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER (sv->priv->document);

	gtk_text_buffer_get_iter_at_offset (buffer, &start_iter, 0);
	gtk_text_buffer_get_iter_at_offset (buffer, &end_iter, -1);

	return gtk_text_buffer_get_slice(GTK_TEXT_BUFFER(sv->priv->document),
									&start_iter, &end_iter, TRUE);
}

/* Get cursor position */
static IAnjutaIterable*
ieditor_get_position (IAnjutaEditor* editor, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->document);
	GtkTextIter iter;
	SourceviewCell* cell;

	gtk_text_buffer_get_iter_at_mark(buffer, &iter,
									 gtk_text_buffer_get_insert(buffer));

	cell = sourceview_cell_new (&iter, GTK_TEXT_VIEW (sv->priv->view));

	return IANJUTA_ITERABLE (cell);
}

static gint
ieditor_get_offset (IAnjutaEditor* editor, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->document);
	GtkTextIter iter;

	gtk_text_buffer_get_iter_at_mark(buffer, &iter,
									 gtk_text_buffer_get_insert(buffer));

	return gtk_text_iter_get_offset (&iter);
}

/* Return line of cursor */
static gint ieditor_get_lineno(IAnjutaEditor *editor, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->document);
	GtkTextIter iter;

	gtk_text_buffer_get_iter_at_mark(buffer, &iter,
									 gtk_text_buffer_get_insert(buffer));
	return gtk_text_iter_get_line(&iter) + 1;
}

/* Return the length of the text in the buffer */
static gint ieditor_get_length(IAnjutaEditor *editor, GError **e)
{
	/* We do not use gtk_text_buffer_get_char_count here because
	 we need a size in bytes not in characters */

	GtkTextIter start_iter;
	GtkTextIter end_iter;
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	gchar* text;
	gint length;

	gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(sv->priv->document),
								   &start_iter);
	gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(sv->priv->document),
								   &end_iter);
	text = gtk_text_buffer_get_slice(GTK_TEXT_BUFFER(sv->priv->document),
									&start_iter, &end_iter, TRUE);
	length = strlen(text);
	g_free(text);

	return length;
}

/* Return word on cursor position */
static gchar* ieditor_get_current_word(IAnjutaEditor *editor, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	GtkTextIter start;
	GtkTextIter end;

	anjuta_view_get_current_word (sv->priv->view,
	                              &start, &end);

	return gtk_text_buffer_get_text (gtk_text_iter_get_buffer (&start),
	                                 &start, &end, FALSE);
}

/* Insert text at position */
static void ieditor_insert(IAnjutaEditor *editor, IAnjutaIterable* icell,
							   const gchar* text, gint length, GError **e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL (icell);
	GtkTextIter iter;
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	sourceview_cell_get_iter (cell, &iter);

	/* Avoid processing text that is inserted programatically */
	g_signal_handlers_block_by_func (sv->priv->document,
	                                 on_insert_text,
	                                 sv);

	gtk_text_buffer_insert(GTK_TEXT_BUFFER(sv->priv->document),
						   &iter, text, length);
	g_signal_handlers_unblock_by_func (sv->priv->document,
	                                   on_insert_text,
	                                   sv);
}

/* Append text to buffer */
static void ieditor_append(IAnjutaEditor *editor, const gchar* text,
							   gint length, GError **e)
{
	GtkTextIter iter;
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);

	gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(sv->priv->document),
									   &iter);

	/* Avoid processing text that is inserted programatically */
	g_signal_handlers_block_by_func (sv->priv->document,
	                                 on_insert_text,
	                                 sv);
	gtk_text_buffer_insert(GTK_TEXT_BUFFER(sv->priv->document),
						   &iter, text, length);
	g_signal_handlers_unblock_by_func (sv->priv->document,
	                                   on_insert_text,
	                                   sv);
}

static void ieditor_erase(IAnjutaEditor* editor, IAnjutaIterable* istart_cell,
						  IAnjutaIterable* iend_cell, GError **e)
{
	SourceviewCell* start_cell = SOURCEVIEW_CELL (istart_cell);
	GtkTextIter start;
	GtkTextIter end;
	SourceviewCell* end_cell = SOURCEVIEW_CELL (iend_cell);
	sourceview_cell_get_iter (end_cell, &end);
	sourceview_cell_get_iter (start_cell, &start);

	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->document);

	gtk_text_buffer_delete (buffer, &start, &end);
}

static void ieditor_erase_all(IAnjutaEditor *editor, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	gtk_text_buffer_set_text(GTK_TEXT_BUFFER(sv->priv->document), "", 0);
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
    g_object_set(G_OBJECT(sv->priv->view), "popup", menu, NULL);
}

/* Convert from position to line */
static gint ieditor_get_line_from_position(IAnjutaEditor *editor,
										   IAnjutaIterable* icell, GError **e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL (icell);
	GtkTextIter iter;
	sourceview_cell_get_iter (cell, &iter);
	return LINE_TO_LOCATION (gtk_text_iter_get_line(&iter));
}

static IAnjutaIterable* ieditor_get_line_begin_position(IAnjutaEditor *editor,
											gint line, GError **e)
{
	GtkTextIter iter;
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);

	gtk_text_buffer_get_iter_at_line_offset (GTK_TEXT_BUFFER(sv->priv->document),
											 &iter, LOCATION_TO_LINE (line), 0);
	return IANJUTA_ITERABLE (sourceview_cell_new (&iter, GTK_TEXT_VIEW (sv->priv->view)));
}

static IAnjutaIterable* ieditor_get_line_end_position(IAnjutaEditor *editor,
											gint line, GError **e)
{
	GtkTextIter iter;
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);

	gtk_text_buffer_get_iter_at_line_offset (GTK_TEXT_BUFFER(sv->priv->document),
											 &iter, LOCATION_TO_LINE (line), 0);
	/* If iter is not at line end, move it */
	if (!gtk_text_iter_ends_line(&iter))
		gtk_text_iter_forward_to_line_end (&iter);
	return IANJUTA_ITERABLE (sourceview_cell_new (&iter, GTK_TEXT_VIEW (sv->priv->view)));
}

static IAnjutaIterable*
ieditor_get_position_from_offset(IAnjutaEditor* edit, gint position, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->document);
	GtkTextIter iter;
	SourceviewCell* cell;

	gtk_text_buffer_get_iter_at_offset(buffer, &iter, position);
	cell = sourceview_cell_new(&iter, GTK_TEXT_VIEW(sv->priv->view));

	return IANJUTA_ITERABLE(cell);
}

static IAnjutaIterable*
ieditor_get_start_position (IAnjutaEditor* edit, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->document);
	GtkTextIter iter;
	SourceviewCell* cell;

	gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0);
	cell = sourceview_cell_new(&iter, GTK_TEXT_VIEW(sv->priv->view));

	return IANJUTA_ITERABLE(cell);
}

static IAnjutaIterable*
ieditor_get_end_position (IAnjutaEditor* edit, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->document);
	GtkTextIter iter;
	SourceviewCell* cell;

	gtk_text_buffer_get_iter_at_offset(buffer, &iter, -1);
	cell = sourceview_cell_new(&iter, GTK_TEXT_VIEW(sv->priv->view));

	return IANJUTA_ITERABLE(cell);
}

static void
ieditor_goto_start (IAnjutaEditor* edit, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (sv->priv->document),
										&iter, 0);
	gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (sv->priv->document), &iter);
	gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (sv->priv->view),
								  &iter, 0, FALSE, 0, 0);
}

static void
ieditor_goto_end (IAnjutaEditor* edit, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (sv->priv->document),
										&iter, -1);
	gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (sv->priv->document), &iter);
	gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (sv->priv->view),
								  &iter, 0, FALSE, 0, 0);
}

static void
ieditor_iface_init (IAnjutaEditorIface *iface)
{
	iface->get_tabsize = ieditor_get_tab_size;
	iface->set_tabsize = ieditor_set_tab_size;
	iface->get_use_spaces = ieditor_get_use_spaces;
	iface->set_use_spaces = ieditor_set_use_spaces;
    iface->set_auto_indent = ieditor_set_auto_indent;
	iface->goto_line = ieditor_goto_line;
	iface->goto_position = ieditor_goto_position;
	iface->get_text = ieditor_get_text;
	iface->get_text_all = ieditor_get_text_all;
	iface->get_position = ieditor_get_position;
	iface->get_offset = ieditor_get_offset;
	iface->get_lineno = ieditor_get_lineno;
	iface->get_length = ieditor_get_length;
	iface->get_current_word = ieditor_get_current_word;
	iface->insert = ieditor_insert;
	iface->append = ieditor_append;
	iface->erase = ieditor_erase;
	iface->erase_all = ieditor_erase_all;
	iface->get_column = ieditor_get_column;
	iface->get_overwrite = ieditor_get_overwrite;
	iface->set_popup_menu = ieditor_set_popup_menu;
	iface->get_line_from_position = ieditor_get_line_from_position;
	iface->get_line_begin_position = ieditor_get_line_begin_position;
	iface->get_line_end_position = ieditor_get_line_end_position;
	iface->goto_start = ieditor_goto_start;
	iface->goto_end = ieditor_goto_end;
	iface->get_position_from_offset = ieditor_get_position_from_offset;
	iface->get_start_position = ieditor_get_start_position;
	iface->get_end_position = ieditor_get_end_position;
}

/* Return true if editor can redo */
static gboolean idocument_can_redo(IAnjutaDocument *editor, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	return gtk_source_buffer_can_redo(GTK_SOURCE_BUFFER(sv->priv->document));
}

/* Return true if editor can undo */
static gboolean idocument_can_undo(IAnjutaDocument *editor, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	return gtk_source_buffer_can_undo(GTK_SOURCE_BUFFER(sv->priv->document));
}

/* Return true if editor can undo */
static void idocument_begin_undo_action (IAnjutaDocument *editor, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	gtk_text_buffer_begin_user_action (GTK_TEXT_BUFFER(sv->priv->document));
}

/* Return true if editor can undo */
static void idocument_end_undo_action (IAnjutaDocument *editor, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	gtk_text_buffer_end_user_action (GTK_TEXT_BUFFER(sv->priv->document));
}


static void
idocument_undo(IAnjutaDocument* edit, GError** ee)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	
	if (idocument_can_undo(edit, NULL))
	{
		g_signal_handlers_block_by_func (sv->priv->document, on_insert_text, sv);
		gtk_source_buffer_undo(GTK_SOURCE_BUFFER(sv->priv->document));
		g_signal_handlers_unblock_by_func (sv->priv->document, on_insert_text, sv);
	}
	
	anjuta_view_scroll_to_cursor(sv->priv->view);
	g_signal_emit_by_name(G_OBJECT(sv), "update_ui", sv);
}

static void
idocument_redo(IAnjutaDocument* edit, GError** ee)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	if (idocument_can_redo(edit, NULL))
		gtk_source_buffer_redo(GTK_SOURCE_BUFFER(sv->priv->document));
	anjuta_view_scroll_to_cursor(sv->priv->view);
	g_signal_emit_by_name(G_OBJECT(sv), "update_ui", sv);
}

/* Grab focus */
static void idocument_grab_focus (IAnjutaDocument *editor, GError **e)
{
	gtk_widget_grab_focus (GTK_WIDGET (ANJUTA_SOURCEVIEW (editor)->priv->view));
}

/* Return the opened filename */
static const gchar* idocument_get_filename(IAnjutaDocument *editor, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	return sourceview_io_get_filename (sv->priv->io);
}

static void
idocument_cut(IAnjutaDocument* edit, GError** ee)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);

	g_signal_handlers_block_by_func (sv->priv->document, on_insert_text, sv);
	anjuta_view_cut_clipboard(sv->priv->view);
	g_signal_handlers_unblock_by_func (sv->priv->document, on_insert_text, sv);
}

static void
idocument_copy(IAnjutaDocument* edit, GError** ee)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	g_signal_handlers_block_by_func (sv->priv->document, on_insert_text, sv);
	anjuta_view_copy_clipboard(sv->priv->view);
	g_signal_handlers_unblock_by_func (sv->priv->document, on_insert_text, sv);
}

static void
idocument_paste(IAnjutaDocument* edit, GError** ee)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	g_signal_handlers_block_by_func (sv->priv->document, on_insert_text, sv);
	anjuta_view_paste_clipboard(sv->priv->view);
	g_signal_handlers_unblock_by_func (sv->priv->document, on_insert_text, sv);
}

static void
idocument_clear(IAnjutaDocument* edit, GError** ee)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	if (gtk_text_buffer_get_has_selection(GTK_TEXT_BUFFER(sv->priv->document)))
		anjuta_view_delete_selection(sv->priv->view);
	else
	{
		GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->document);
		GtkTextIter cursor;
		gtk_text_buffer_get_iter_at_mark(buffer, &cursor, gtk_text_buffer_get_insert(buffer));

		/* Fix #388731 */
		gtk_text_iter_forward_char(&cursor);
		gtk_text_buffer_backspace(buffer, &cursor, TRUE, TRUE);
	}
}

static void
idocument_iface_init (IAnjutaDocumentIface *iface)
{
	iface->grab_focus = idocument_grab_focus;
	iface->get_filename = idocument_get_filename;
	iface->can_undo = idocument_can_undo;
	iface->can_redo = idocument_can_redo;
	iface->begin_undo_action = idocument_begin_undo_action;
	iface->end_undo_action = idocument_end_undo_action;
	iface->undo = idocument_undo;
	iface->redo = idocument_redo;
	iface->cut = idocument_cut;
	iface->copy = idocument_copy;
	iface->paste = idocument_paste;
	iface->clear = idocument_clear;
}

static void
set_select(Sourceview* sv, GtkTextIter* start_iter, GtkTextIter* end_iter, gboolean scroll)
{
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER (sv->priv->document);
	gtk_text_buffer_select_range (buffer, start_iter, end_iter);

	if (scroll)
		gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (sv->priv->view),
									  gtk_text_buffer_get_insert (buffer),
									  0.25,
									  FALSE,
									  0.0,
									  0.0);
}

/* IAnjutaEditorSelection */

/* Find the previous open brace that begins the current indentation level. */
static gboolean find_open_bracket(GtkTextIter *iter)
{
	int level = 1;

	while (gtk_text_iter_backward_char (iter))
	{
		switch (gtk_text_iter_get_char (iter))
		{
		case '{':
			if (!--level)
				return TRUE;
			break;
		case '}':
			++level;
			break;
		}
	}

	return FALSE;
}

/* Find the following close brace that ends the current indentation level. */
static gboolean find_close_bracket(GtkTextIter *iter)
{
	int level = 1;

	while (gtk_text_iter_forward_char (iter))
	{
		switch (gtk_text_iter_get_char (iter))
		{
		case '{':
			++level;
			break;
		case '}':
			if (!--level)
				return TRUE;
			break;
		}
	}

	return FALSE;
}

static void
iselect_block(IAnjutaEditorSelection* edit, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->document);

	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_mark (buffer, &iter,
								 gtk_text_buffer_get_insert(buffer));
	if (find_open_bracket (&iter))
	{
		GtkTextIter end_iter;
		gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER(sv->priv->document),
								     &iter);
		end_iter = iter;
		if (find_close_bracket (&end_iter))
		{
			gtk_text_iter_forward_char (&end_iter);	  /* move past brace */
			set_select (sv, &iter, &end_iter, TRUE);
		}
	}
}

static void
iselect_set (IAnjutaEditorSelection* edit,
			 IAnjutaIterable* istart,
			 IAnjutaIterable* iend,
			 gboolean scroll,
			 GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	GtkTextIter start, end;
	sourceview_cell_get_iter (SOURCEVIEW_CELL (istart), &start);
	sourceview_cell_get_iter (SOURCEVIEW_CELL (iend), &end);
	set_select(sv,
			   &start,
			   &end,
			   scroll);
}


static void
iselect_all(IAnjutaEditorSelection* edit, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	anjuta_view_select_all(sv->priv->view);
}

static gboolean
iselect_has_selection (IAnjutaEditorSelection *editor, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);

	return gtk_text_buffer_get_has_selection (GTK_TEXT_BUFFER(sv->priv->document));
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
		return gtk_text_buffer_get_slice(GTK_TEXT_BUFFER(sv->priv->document),
										&start_iter, &end_iter, TRUE);
	}
	else
		return NULL;

}

static IAnjutaIterable*
iselect_get_start (IAnjutaEditorSelection *edit, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	GtkTextIter start;
	if (gtk_text_buffer_get_selection_bounds (GTK_TEXT_BUFFER (sv->priv->document),
											  &start, NULL))
	{
		return IANJUTA_ITERABLE (sourceview_cell_new (&start, GTK_TEXT_VIEW (sv->priv->view)));
	}
	return NULL;
}

static IAnjutaIterable*
iselect_get_end (IAnjutaEditorSelection *edit, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	GtkTextIter end;
	if (gtk_text_buffer_get_selection_bounds (GTK_TEXT_BUFFER (sv->priv->document),
											  NULL, &end))
	{
		return IANJUTA_ITERABLE (sourceview_cell_new (&end, GTK_TEXT_VIEW (sv->priv->view)));
	}
	return NULL;
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
iselect_iface_init(IAnjutaEditorSelectionIface *iface)
{
	iface->set = iselect_set;
	iface->has_selection = iselect_has_selection;
	iface->get_start = iselect_get_start;
	iface->get_end = iselect_get_end;
	iface->select_block = iselect_block;
	iface->select_function = iselect_function;
	iface->select_all = iselect_all;
	iface->select_block = iselect_block;
	iface->get = iselect_get;
	iface->replace = iselect_replace;
}

/* IAnjutaEditorConvert Interface */

static void
iconvert_to_upper(IAnjutaEditorConvert* edit, IAnjutaIterable *start_position,
				  IAnjutaIterable *end_position, GError** e)
{
  Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
  GtkTextBuffer* buffer = GTK_TEXT_BUFFER (sv->priv->document);
  GtkTextIter start, end;
  sourceview_cell_get_iter (SOURCEVIEW_CELL (end_position), &end);
  sourceview_cell_get_iter (SOURCEVIEW_CELL (start_position), &start);

  gchar* text_buffer = gtk_text_buffer_get_text (buffer,
											&start, &end, TRUE);
  gtk_text_buffer_begin_user_action (buffer);
  gtk_text_buffer_delete (buffer, &start, &end);
  gtk_text_buffer_insert (buffer, &start, g_utf8_strup (text_buffer, -1), -1);
  gtk_text_buffer_end_user_action (buffer);
  g_free (text_buffer);
}

static void
iconvert_to_lower(IAnjutaEditorConvert* edit, IAnjutaIterable *start_position,
				  IAnjutaIterable *end_position, GError** e)
{
  Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
  GtkTextBuffer* buffer = GTK_TEXT_BUFFER (sv->priv->document);
  GtkTextIter start, end;
  sourceview_cell_get_iter (SOURCEVIEW_CELL (end_position), &end);
  sourceview_cell_get_iter (SOURCEVIEW_CELL (start_position), &start);

  gchar* text_buffer = gtk_text_buffer_get_text (buffer,
											&start, &end, TRUE);
  gtk_text_buffer_begin_user_action (buffer);
  gtk_text_buffer_delete (buffer, &start, &end);
  gtk_text_buffer_insert (buffer, &start, g_utf8_strdown (text_buffer, -1), -1);
  gtk_text_buffer_end_user_action (buffer);
  g_free (text_buffer);

}

static void
iconvert_iface_init(IAnjutaEditorConvertIface* iface)
{
	iface->to_upper = iconvert_to_upper;
	iface->to_lower = iconvert_to_lower;
}

typedef struct
{
	IAnjutaMarkableMarker marker;
	gint location;
	gint handle;
	guint source;
	gchar* tooltip;
	Sourceview* sv;
} SVMark;

static gboolean mark_real (gpointer data)
{
	SVMark* svmark = data;
	Sourceview* sv = svmark->sv;
	GtkTextIter iter;
	GtkSourceMark* source_mark;
	const gchar* category;
	gint location = svmark->location;
	gint marker_count = svmark->handle;
	gchar* tooltip = svmark->tooltip;
	IAnjutaMarkableMarker marker = svmark->marker;
	gchar* name;

	if (sv->priv->loading)
	{
		/* Wait until loading is finished */
		return TRUE;
	}

	gtk_text_buffer_get_iter_at_line(GTK_TEXT_BUFFER(sv->priv->document),
									 &iter, LOCATION_TO_LINE (location));

	category = marker_types[marker];
	name = CREATE_MARK_NAME (marker_count);


	source_mark = gtk_source_buffer_create_source_mark(GTK_SOURCE_BUFFER(sv->priv->document),
												name, category, &iter);
	g_object_set_data_full (G_OBJECT (source_mark), MARKER_TOOLTIP_DATA, tooltip,
	                        (GDestroyNotify) g_free);

	g_source_remove (svmark->source);

	g_free (name);
	g_slice_free (SVMark, svmark);
	return FALSE;
}

static gint
imark_mark(IAnjutaMarkable* mark, gint location, IAnjutaMarkableMarker marker,
           const gchar* tooltip, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(mark);
	SVMark* svmark = g_slice_new0 (SVMark);

	if (location <= 0)
	{
		g_set_error (e, IANJUTA_MARKABLE_ERROR, IANJUTA_MARKABLE_INVALID_LOCATION,
		             "Invalid marker location: %d!", location);
		return -1;
	}

	static gint marker_count = 0;

	marker_count++;

	svmark->sv = sv;
	svmark->location = location;
	svmark->handle = marker_count;
	svmark->marker = marker;
	svmark->tooltip = tooltip ? g_strdup (tooltip) : NULL;
	svmark->source = g_idle_add (mark_real, svmark);

	sv->priv->idle_sources = g_slist_prepend (sv->priv->idle_sources,
											  GUINT_TO_POINTER (svmark->source));

	return marker_count;
}

static void
imark_unmark(IAnjutaMarkable* mark, gint location, IAnjutaMarkableMarker marker,
			 GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(mark);
	GtkSourceBuffer* buffer = GTK_SOURCE_BUFFER(sv->priv->document);
	GtkTextIter begin;
	GtkTextIter end;

	gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (buffer), &begin, LOCATION_TO_LINE (location));
	gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (buffer), &end, LOCATION_TO_LINE (location));

	gtk_source_buffer_remove_source_marks (buffer, &begin, &end, marker_types[marker]);

}

static gboolean
imark_is_marker_set(IAnjutaMarkable* mark, gint location,
					IAnjutaMarkableMarker marker, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(mark);
	GtkSourceBuffer* buffer = GTK_SOURCE_BUFFER(sv->priv->document);
	GSList* markers;
	gboolean retval;

	markers = gtk_source_buffer_get_source_marks_at_line (buffer,
												   LOCATION_TO_LINE (location),
												   marker_types[marker]);

	retval = (markers != NULL);

	g_slist_free (markers);
	return retval;
}

static gint
imark_location_from_handle(IAnjutaMarkable* imark, gint handle, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(imark);
	GtkSourceBuffer* buffer = GTK_SOURCE_BUFFER(sv->priv->document);
	GtkTextMark* mark;
	GtkTextIter iter;
	gint location;
	gchar* name = CREATE_MARK_NAME (handle);

	mark = gtk_text_buffer_get_mark (GTK_TEXT_BUFFER (buffer), name);
	if (mark)
	{
		gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (buffer), &iter, mark);
		location = LINE_TO_LOCATION (gtk_text_iter_get_line (&iter));
	}
	else
		location = -1;

	g_free (name);

	return location;
}

static void
imark_delete_all_markers(IAnjutaMarkable* imark, IAnjutaMarkableMarker marker,
						 GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(imark);
	GtkSourceBuffer* buffer = GTK_SOURCE_BUFFER(sv->priv->document);
	GtkTextIter begin;
	GtkTextIter end;

	gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (buffer), &begin, 0);
	gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (buffer), &end, -1);

	gtk_source_buffer_remove_source_marks (buffer, &begin, &end, marker_types[marker]);
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
	gtk_text_buffer_remove_tag_by_name (GTK_TEXT_BUFFER(sv->priv->document),
	                                     WARNING_INDIC,
	                                     &start, &end);
	gtk_text_buffer_remove_tag_by_name (GTK_TEXT_BUFFER(sv->priv->document),
	                                     CRITICAL_INDIC,
	                                     &start, &end);
}

static void
iindic_set (IAnjutaIndicable *indic, IAnjutaIterable* ibegin, IAnjutaIterable *iend,
            IAnjutaIndicableIndicator indicator, GError **e)
{
	GtkTextTag *tag = NULL;
	Sourceview* sv = ANJUTA_SOURCEVIEW(indic);
	GtkTextIter start, end;

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
	sourceview_cell_get_iter (SOURCEVIEW_CELL (ibegin), &start);
	sourceview_cell_get_iter (SOURCEVIEW_CELL (iend), &end);
	gtk_text_buffer_apply_tag (GTK_TEXT_BUFFER(sv->priv->document), tag,
	                           &start,
							   &end);
}

static void
iindic_iface_init(IAnjutaIndicableIface* iface)
{
	iface->clear = iindic_clear;
	iface->set = iindic_set;
}

static void
iprint_print(IAnjutaPrint* print, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(print);
	sourceview_print(sv);
}

static void
iprint_print_preview(IAnjutaPrint* print, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(print);
	sourceview_print_preview(sv);
}

static void
iprint_iface_init(IAnjutaPrintIface* iface)
{
	iface->print = iprint_print;
	iface->print_preview = iprint_print_preview;
}


static const GList*
ilanguage_get_supported_languages (IAnjutaEditorLanguage *ilanguage,
								   GError **err)
{
	/* Cache the list */
	static GList* languages = NULL;
	if (!languages)
	{
		const gchar* const * langs = gtk_source_language_manager_get_language_ids (gtk_source_language_manager_get_default());
		if (langs)
		{
			const gchar* const * lang;

			for (lang = langs; *lang != NULL; lang++)
			{
				languages = g_list_append (languages, (gpointer)*lang);
			}
		}
	}
	return languages;
}

static const gchar*
ilanguage_get_language_name (IAnjutaEditorLanguage *ilanguage,
							 const gchar *language, GError **err)
{
	return language;
}

static const gchar*
autodetect_language (Sourceview* sv)
{
	gchar* io_mime_type = sourceview_io_get_mime_type (sv->priv->io);
	const gchar* filename = sourceview_io_get_filename (sv->priv->io);
	GtkSourceLanguage *language;
	const gchar* detected_language = NULL;

	language = gtk_source_language_manager_guess_language (gtk_source_language_manager_get_default (), filename, io_mime_type);
	if (!language)
	{
		goto out;
	}

	detected_language = gtk_source_language_get_id (language);

	g_signal_emit_by_name (sv, "language-changed", detected_language);
	gtk_source_buffer_set_language (GTK_SOURCE_BUFFER (sv->priv->document), language);

out:
	if (io_mime_type)
		g_free (io_mime_type);

	return detected_language;
}

static void
ilanguage_set_language (IAnjutaEditorLanguage *ilanguage,
						const gchar *language, GError **err)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW (ilanguage);
	gboolean found = FALSE;
	const GList* languages = ilanguage_get_supported_languages(ilanguage, err);
	const GList* cur_lang;
	for (cur_lang = languages; cur_lang != NULL && language != NULL; cur_lang = g_list_next (cur_lang))
	{
		GtkSourceLanguage* source_language =
			gtk_source_language_manager_get_language (gtk_source_language_manager_get_default(),
													  cur_lang->data);
		const gchar* id = gtk_source_language_get_id (source_language);

		if (g_str_equal (language, id))
		{
			g_signal_emit_by_name (G_OBJECT(sv), "language-changed",
									   id);
			gtk_source_buffer_set_language (GTK_SOURCE_BUFFER (sv->priv->document),
											source_language);
			found = TRUE;
			break;
		}
	}
	if (!found)
	{
		autodetect_language (sv);
	}
}

static const gchar*
ilanguage_get_language (IAnjutaEditorLanguage *ilanguage, GError **err)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(ilanguage);
	GtkSourceBuffer* buffer = GTK_SOURCE_BUFFER(sv->priv->document);
	GtkSourceLanguage* lang = gtk_source_buffer_get_language(buffer);
	if (lang)
	{
		return gtk_source_language_get_id(lang);
	}
	else
	{
		const gchar* language = autodetect_language (sv);
		return language;
	}
}

static void
ilanguage_iface_init (IAnjutaEditorLanguageIface *iface)
{
	iface->get_supported_languages = ilanguage_get_supported_languages;
	iface->get_language_name = ilanguage_get_language_name;
	iface->get_language = ilanguage_get_language;
	iface->set_language = ilanguage_set_language;
}

static void
itips_show (IAnjutaEditorTip *iassist, GList* tips, IAnjutaIterable* ipos,
            GError **err)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(iassist);
	SourceviewCell* cell = SOURCEVIEW_CELL (ipos);
	GtkTextIter iter;
	sourceview_cell_get_iter(cell, &iter);

	g_return_if_fail (tips != NULL);

	if (!sv->priv->assist_tip)
	{
		sv->priv->assist_tip =
			ASSIST_TIP (assist_tip_new (GTK_TEXT_VIEW (sv->priv->view), tips));

		g_object_weak_ref (G_OBJECT(sv->priv->assist_tip),
		                   (GWeakNotify) on_assist_tip_destroyed,
		                   sv);
		assist_tip_move (sv->priv->assist_tip, GTK_TEXT_VIEW (sv->priv->view), &iter);
		gtk_widget_show (GTK_WIDGET (sv->priv->assist_tip));
	}
	else
	{
		assist_tip_set_tips (sv->priv->assist_tip, tips);
		assist_tip_move (sv->priv->assist_tip, GTK_TEXT_VIEW (sv->priv->view), &iter);
	}
}

static void
itips_cancel (IAnjutaEditorTip* iassist, GError** err)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(iassist);
	if (sv->priv->assist_tip)
		gtk_widget_destroy (GTK_WIDGET (sv->priv->assist_tip));
}

static gboolean
itips_visible (IAnjutaEditorTip* iassist, GError** err)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(iassist);
	return (sv->priv->assist_tip != NULL);
}

static void
itip_iface_init(IAnjutaEditorTipIface* iface)
{
	iface->show = itips_show;
	iface->cancel = itips_cancel;
	iface->visible = itips_visible;
}

static void
iassist_add(IAnjutaEditorAssist* iassist,
            IAnjutaProvider* provider,
            GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(iassist);
	GtkSourceCompletion* completion = gtk_source_view_get_completion(GTK_SOURCE_VIEW(sv->priv->view));
	gtk_source_completion_add_provider(completion,
	                                   GTK_SOURCE_COMPLETION_PROVIDER(sourceview_provider_new(sv, provider)),
	                                   NULL);
	DEBUG_PRINT("Adding provider: %s", ianjuta_provider_get_name(provider, NULL));
}

static void
iassist_remove(IAnjutaEditorAssist* iassist,
               IAnjutaProvider* provider,
               GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(iassist);
	GtkSourceCompletion* completion = gtk_source_view_get_completion(GTK_SOURCE_VIEW(sv->priv->view));
	GList* node;
	for (node = gtk_source_completion_get_providers(completion); node != NULL; node = g_list_next(node))
	{
		SourceviewProvider* prov;
		if (!SOURCEVIEW_IS_PROVIDER(node->data))
		    continue;
		prov = SOURCEVIEW_PROVIDER(node->data);
		if (prov->iprov == provider)
		{
			DEBUG_PRINT("Removing provider: %s", ianjuta_provider_get_name(provider, NULL));
			gtk_source_completion_remove_provider(completion,
	        		                              GTK_SOURCE_COMPLETION_PROVIDER(prov),
	        		                              NULL);
	    }
	}
}

static void
iassist_invoke(IAnjutaEditorAssist* iassist,
               IAnjutaProvider* provider,
               GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(iassist);
	GtkSourceCompletion* completion = gtk_source_view_get_completion(GTK_SOURCE_VIEW(sv->priv->view));
	GList* node;
	SourceviewProvider* prov = NULL;
	GList* providers = NULL;
	GtkTextIter iter;
	GtkSourceCompletionContext* context;

	for (node = gtk_source_completion_get_providers(completion); node != NULL; node = g_list_next(node))
	{
		if (provider == NULL)
		{
			providers = g_list_append (providers, node->data);
			continue;
		}
		if (!SOURCEVIEW_IS_PROVIDER (node->data))
			break;
		prov = SOURCEVIEW_PROVIDER(node->data);
		if (prov->iprov == provider)
		{
			providers = g_list_append (providers, prov);
		}

	}
	gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (sv->priv->document),
	                                  &iter,
	                                  gtk_text_buffer_get_insert(GTK_TEXT_BUFFER(sv->priv->document)));
	context =
		gtk_source_completion_create_context(completion, &iter);

	gtk_source_completion_show(completion, providers, context);
	g_list_free(providers);
}

static void
iassist_proposals(IAnjutaEditorAssist* iassist,
                  IAnjutaProvider* provider,
                  GList* proposals,
                  const gchar* pre_word,
                  gboolean finished,
                  GError** e)
{
	/* Hide if the only suggetions is exactly the typed word */
	if (pre_word && proposals && g_list_length (proposals) == 1)
	{
		IAnjutaEditorAssistProposal* proposal = proposals->data;
		AnjutaLanguageProposalData* data = proposal->data;
		if (g_str_equal (pre_word, data->name))
			proposals = NULL;
	}
	
	Sourceview* sv = ANJUTA_SOURCEVIEW(iassist);
	GtkSourceCompletion* completion = gtk_source_view_get_completion(GTK_SOURCE_VIEW(sv->priv->view));
	GList* node;
	for (node = gtk_source_completion_get_providers(completion); node != NULL; node = g_list_next(node))
	{
		SourceviewProvider* prov;
		if (!SOURCEVIEW_IS_PROVIDER (node->data))
			continue;

		prov = SOURCEVIEW_PROVIDER(node->data);
		if (prov->iprov == provider)
		{
			g_return_if_fail (!prov->cancelled);

			GList* prop;
			GList* items = NULL;
			for (prop = proposals; prop != NULL; prop = g_list_next(prop))
			{
				IAnjutaEditorAssistProposal* proposal = prop->data;
				GtkSourceCompletionItem* item;
				if (proposal->markup)
				{
					item = gtk_source_completion_item_new_with_markup(proposal->markup,
					                                                  proposal->text,
					                                                  proposal->icon,
					                                                  proposal->info);
				}
				else
				{
					item = gtk_source_completion_item_new(proposal->label,
					                                      proposal->text,
					                                      proposal->icon,
					                                      proposal->info);
				}
				items = g_list_append (items, item);
				g_object_set_data (G_OBJECT(item), "__data", proposal->data);
			}
			gtk_source_completion_context_add_proposals (prov->context, GTK_SOURCE_COMPLETION_PROVIDER(prov),
			                                             items, finished);
			break;
		}
	}
}

static void	iassist_iface_init(IAnjutaEditorAssistIface* iface)
{
	iface->add = iassist_add;
	iface->remove = iassist_remove;
	iface->invoke = iassist_invoke;
	iface->proposals = iassist_proposals;
}


static gboolean
isearch_forward (IAnjutaEditorSearch* isearch,
				 const gchar* search,
				 gboolean case_sensitive,
				 IAnjutaEditorCell* istart,
				 IAnjutaEditorCell* iend,
				 IAnjutaEditorCell** iresult_start,
				 IAnjutaEditorCell** iresult_end,
				 GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW (isearch);

	SourceviewCell* start = SOURCEVIEW_CELL (istart);
	SourceviewCell* end = SOURCEVIEW_CELL (iend);

	GtkTextIter start_iter;
	GtkTextIter end_iter;

	GtkTextIter result_start, result_end;

	GtkTextSearchFlags flags = 0;

	sourceview_cell_get_iter (start, &start_iter);
	sourceview_cell_get_iter (end, &end_iter);


	if (!case_sensitive)
	{
		flags = GTK_TEXT_SEARCH_CASE_INSENSITIVE;
	}

	gboolean result =
		gtk_text_iter_forward_search (&start_iter, search, flags, &result_start, &result_end,
									&end_iter);

	if (result)
	{
		if (iresult_start)
		{
			*iresult_start = IANJUTA_EDITOR_CELL (sourceview_cell_new (&result_start,
																	   GTK_TEXT_VIEW (sv->priv->view)));
		}
		if (iresult_end)
		{
			*iresult_end = IANJUTA_EDITOR_CELL (sourceview_cell_new (&result_end,
																	 GTK_TEXT_VIEW (sv->priv->view)));
		}
	}

	return result;
}

static gboolean
isearch_backward (IAnjutaEditorSearch* isearch,
				  const gchar* search,
				  gboolean case_sensitive,
				  IAnjutaEditorCell* istart,
				  IAnjutaEditorCell* iend,
				  IAnjutaEditorCell** iresult_start,
				  IAnjutaEditorCell** iresult_end,
				  GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW (isearch);

	SourceviewCell* start = SOURCEVIEW_CELL (istart);
	SourceviewCell* end = SOURCEVIEW_CELL (iend);

	GtkTextIter start_iter;
	GtkTextIter end_iter;
	GtkTextIter result_start, result_end;

	GtkTextSearchFlags flags = 0;

	sourceview_cell_get_iter (start, &start_iter);
	sourceview_cell_get_iter (end, &end_iter);


	if (!case_sensitive)
	{
		flags = GTK_TEXT_SEARCH_CASE_INSENSITIVE;
	}

	gboolean result =
		gtk_text_iter_backward_search (&start_iter, search, flags, &result_start, &result_end,
									&end_iter);

	if (result)
	{
		if (iresult_start)
		{
			*iresult_start = IANJUTA_EDITOR_CELL (sourceview_cell_new (&result_start,
																	   GTK_TEXT_VIEW (sv->priv->view)));
		}
		if (iresult_end)
		{
			*iresult_end = IANJUTA_EDITOR_CELL (sourceview_cell_new (&result_end,
																	 GTK_TEXT_VIEW (sv->priv->view)));
		}
	}

	return result;
}

static void
isearch_iface_init(IAnjutaEditorSearchIface* iface)
{
	iface->forward = isearch_forward;
	iface->backward = isearch_backward;
}

/* IAnjutaHover */
static void on_sourceview_hover_destroy (gpointer data, GObject* where_the_data_was);
static void
on_sourceview_hover_leave(gpointer data, GObject* where_the_data_was)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW (data);

	if (sv->priv->tooltip_cell)
	{
		g_signal_emit_by_name (G_OBJECT (sv), "hover-leave", sv->priv->tooltip_cell);
		g_object_unref (sv->priv->tooltip_cell);
		sv->priv->tooltip_cell = NULL;
	}
	g_object_weak_unref (G_OBJECT(sv), on_sourceview_hover_destroy, where_the_data_was);
}

static void
on_sourceview_hover_destroy (gpointer data, GObject* where_the_data_was)
{
	g_object_weak_unref (G_OBJECT(data), on_sourceview_hover_leave, where_the_data_was);
}


static gboolean
on_sourceview_hover_over (GtkWidget *widget, gint x, gint y,
						  gboolean keyboard_tip, GtkTooltip *tooltip,
						  gpointer data)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW (data);
	SourceviewCell* cell;
	GtkTextIter iter;
	GtkTextView *text_view = GTK_TEXT_VIEW (widget);
	gint bx, by;

	gtk_text_view_window_to_buffer_coords (text_view, GTK_TEXT_WINDOW_WIDGET,
											   x, y, &bx, &by);
	gtk_text_view_get_iter_at_location (text_view, &iter, bx, by);

	cell = sourceview_cell_new (&iter, text_view);

	/* This should call ianjuta_hover_display() */
	g_signal_emit_by_name (G_OBJECT (sv), "hover-over", cell);

	if (sv->priv->tooltip)
	{
		gtk_tooltip_set_text (tooltip, sv->priv->tooltip);
		g_object_weak_ref (G_OBJECT (tooltip), on_sourceview_hover_leave, sv);
		g_object_weak_ref (G_OBJECT (sv), on_sourceview_hover_destroy, tooltip);
		g_free (sv->priv->tooltip);
		sv->priv->tooltip = NULL;
		sv->priv->tooltip_cell = cell;
		return TRUE;
	}

	return FALSE;
}

static void
ihover_display (IAnjutaEditorHover *ihover,
				IAnjutaIterable* position,
				const gchar* info,
				GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW (ihover);
	g_assert (sv->priv->tooltip == NULL);
	sv->priv->tooltip = g_strdup (info);
}

static void
ihover_iface_init(IAnjutaEditorHoverIface* iface)
{
	iface->display = ihover_display;
}

static void
iglade_iface_init(IAnjutaEditorGladeSignalIface* iface)
{
	/* only signals */
}

ANJUTA_TYPE_BEGIN(Sourceview, sourceview, GTK_TYPE_VBOX);
ANJUTA_TYPE_ADD_INTERFACE(idocument, IANJUTA_TYPE_DOCUMENT);
ANJUTA_TYPE_ADD_INTERFACE(ifile, IANJUTA_TYPE_FILE);
ANJUTA_TYPE_ADD_INTERFACE(isavable, IANJUTA_TYPE_FILE_SAVABLE);
ANJUTA_TYPE_ADD_INTERFACE(ieditor, IANJUTA_TYPE_EDITOR);
ANJUTA_TYPE_ADD_INTERFACE(imark, IANJUTA_TYPE_MARKABLE);
ANJUTA_TYPE_ADD_INTERFACE(iindic, IANJUTA_TYPE_INDICABLE);
ANJUTA_TYPE_ADD_INTERFACE(iselect, IANJUTA_TYPE_EDITOR_SELECTION);
ANJUTA_TYPE_ADD_INTERFACE(iassist, IANJUTA_TYPE_EDITOR_ASSIST);
ANJUTA_TYPE_ADD_INTERFACE(itip, IANJUTA_TYPE_EDITOR_TIP);
ANJUTA_TYPE_ADD_INTERFACE(iconvert, IANJUTA_TYPE_EDITOR_CONVERT);
ANJUTA_TYPE_ADD_INTERFACE(iprint, IANJUTA_TYPE_PRINT);
ANJUTA_TYPE_ADD_INTERFACE(ilanguage, IANJUTA_TYPE_EDITOR_LANGUAGE);
ANJUTA_TYPE_ADD_INTERFACE(isearch, IANJUTA_TYPE_EDITOR_SEARCH);
ANJUTA_TYPE_ADD_INTERFACE(ihover, IANJUTA_TYPE_EDITOR_HOVER);
ANJUTA_TYPE_ADD_INTERFACE(iglade, IANJUTA_TYPE_EDITOR_GLADE_SIGNAL);
ANJUTA_TYPE_END;
