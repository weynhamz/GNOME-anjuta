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
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-savable.h>
#include <libanjuta/interfaces/ianjuta-markable.h>
#include <libanjuta/interfaces/ianjuta-indicable.h>
#include <libanjuta/interfaces/ianjuta-bookmark.h>
#include <libanjuta/interfaces/ianjuta-print.h>
#include <libanjuta/interfaces/ianjuta-language-support.h>
#include <libanjuta/interfaces/ianjuta-document.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>
#include <libanjuta/interfaces/ianjuta-editor-assist.h>
#include <libanjuta/interfaces/ianjuta-editor-convert.h>
#include <libanjuta/interfaces/ianjuta-editor-language.h>
#include <libanjuta/interfaces/ianjuta-editor-search.h>
#include <libanjuta/interfaces/ianjuta-editor-hover.h>

#include <libgnomevfs/gnome-vfs-init.h>
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs.h>

#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourcelanguage.h>
#include <gtksourceview/gtksourcelanguagemanager.h>
#include <gtksourceview/gtksourcebuffer.h>
#include <gtksourceview/gtksourceiter.h>

#include "config.h"
#include "anjuta-encodings.h"
#include "anjuta-document.h"
#include "anjuta-view.h"

#include "sourceview.h"
#include "sourceview-private.h"
#include "sourceview-prefs.h"
#include "sourceview-print.h"
#include "sourceview-cell.h"
#include "plugin.h"

#define HAVE_TOOLTIP_API (GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 12))
		 
#define FORWARD 	0
#define BACKWARD 	1

#define MONITOR_KEY "sourceview.enable.vfs"

#define LOCATION_TO_LINE(o) ((o) - 1)
#define LINE_TO_LOCATION(o) ((o) + 1)

#define CREATE_MARK_NAME(o) (g_strdup_printf ("anjuta-mark-%d", (o)))

static void sourceview_class_init(SourceviewClass *klass);
static void sourceview_instance_init(Sourceview *sv);
static void sourceview_finalize(GObject *object);
static void sourceview_dispose(GObject *object);
static GObjectClass *parent_class = NULL;

#if HAVE_TOOLTIP_API
static gboolean on_sourceview_hover_over (GtkWidget *widget, gint x, gint y,
										  gboolean keyboard_tip, GtkTooltip *tooltip,
										  gpointer data); 
#endif
static gboolean sourceview_add_monitor(Sourceview* sv);

/* Callbacks */

static void
on_assist_window_destroyed (AssistWindow* window, Sourceview* sv)
{
	sv->priv->assist_win = NULL;
}

static void 
on_assist_tip_destroyed (AssistTip* tip, Sourceview* sv)
{
	sv->priv->assist_tip = NULL;
}

static void 
on_assist_chosen(AssistWindow* assist_win, gint num, Sourceview* sv)
{
	g_signal_emit_by_name(G_OBJECT(sv), "assist-chosen", num);
}

static void 
on_assist_cancel(AssistWindow* assist_win, Sourceview* sv)
{
	if (sv->priv->assist_win)
	{
		gtk_widget_destroy(GTK_WIDGET(sv->priv->assist_win));
	}
}

static void on_insert_text (GtkTextBuffer* buffer, 
							GtkTextIter* location,
							char* text,
							gint len,
							Sourceview* sv)
{
	/* We only want ascii characters */
	if (len > 1)
		return;
	else
	{
		int offset = gtk_text_iter_get_offset (location);
		SourceviewCell* cell = sourceview_cell_new (location, 
													GTK_TEXT_VIEW(sv->priv->view));
		ianjuta_iterable_previous (IANJUTA_ITERABLE (cell), NULL);
		g_signal_handlers_block_by_func (buffer, on_insert_text, sv);
		g_signal_emit_by_name(G_OBJECT(sv), "char_added", cell, text[0]);
		g_signal_handlers_unblock_by_func (buffer, on_insert_text, sv);
		/* Reset iterator */
		gtk_text_buffer_get_iter_at_offset (buffer, location,
											offset);
	}
}

/* Called whenever the document is changed */
static void on_document_modified_changed(AnjutaDocument* buffer, Sourceview* sv)
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
	else
	{
		/* Set dirty */
		gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(sv->priv->document), TRUE);
	}
	gtk_widget_destroy (dlg);
}

/* Update Monitor on load/save */
static void
sourceview_remove_monitor(Sourceview* sv)
{
	gboolean monitor_enabled = anjuta_preferences_get_int (sv->priv->prefs, MONITOR_KEY);

	if (monitor_enabled && sv->priv->monitor != NULL)
	{
		DEBUG_PRINT ("Monitor removed for %s", anjuta_document_get_uri(sv->priv->document));
		gnome_vfs_monitor_cancel(sv->priv->monitor);
		sv->priv->monitor = NULL;
	}
}

static gboolean
on_sourceview_uri_changed_prompt (Sourceview* sv)
{
	GtkWidget *dlg;
	GtkWidget *parent;
	gchar *buff;
	
	buff =
		g_strdup_printf (_
						 ("The file '%s' on the disk is more recent than\n"
						  "the current buffer.\nDo you want to reload it?"),
						 ianjuta_document_get_filename(IANJUTA_DOCUMENT(sv), NULL));
	
	parent = gtk_widget_get_toplevel (GTK_WIDGET (sv));
	
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
	
	g_signal_connect (G_OBJECT(dlg), "response",
					  G_CALLBACK (on_reload_dialog_response),
					  sv);
	gtk_widget_show (dlg);

	g_signal_connect_swapped (G_OBJECT(dlg), "delete-event",
					  G_CALLBACK (gtk_widget_destroy),
					  dlg);
	
	return FALSE;
}

static void
on_sourceview_uri_changed (GnomeVFSMonitorHandle *handle,
							const gchar *monitor_uri,
							const gchar *info_uri,
							GnomeVFSMonitorEventType event_type,
							gpointer user_data)
{
	Sourceview *sv = ANJUTA_SOURCEVIEW (user_data);
	
	if (!(event_type == GNOME_VFS_MONITOR_EVENT_CHANGED ||
		  event_type == GNOME_VFS_MONITOR_EVENT_CREATED))
		return;
	
	if (!anjuta_util_diff (anjuta_document_get_uri(sv->priv->document), sv->priv->last_saved_content))
	{
		return;
	}
	
	
	if (strcmp (monitor_uri, info_uri) != 0)
		return;
	
	on_sourceview_uri_changed_prompt(sv);	
}

static gboolean 
sourceview_add_monitor(Sourceview* sv)
{
	gboolean monitor_enabled = anjuta_preferences_get_int (sv->priv->prefs, MONITOR_KEY);

	if (monitor_enabled)
	{
		gchar* uri;
		g_return_val_if_fail(sv->priv->monitor == NULL, FALSE);
		DEBUG_PRINT ("Monitor added for %s", anjuta_document_get_uri(sv->priv->document));
		uri = anjuta_document_get_uri(sv->priv->document);
		gnome_vfs_monitor_add(&sv->priv->monitor, uri,
							  GNOME_VFS_MONITOR_FILE,
							  on_sourceview_uri_changed, sv);
		g_free (uri);
	}
	return FALSE; /* for g_idle_add */
}

/* Called when document is loaded completly */
static void on_document_loaded(AnjutaDocument* doc, GError* err, Sourceview* sv)
{
	const gchar *lang;
	if (err)
	{
		anjuta_util_dialog_error(NULL,
			 "Could not open file: %s", err->message);
	}
	gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(doc), FALSE);
    g_signal_emit_by_name(G_OBJECT(sv), "save_point",
						  TRUE);
	
	if (sv->priv->goto_line > 0)
	{
		anjuta_document_goto_line(doc, LOCATION_TO_LINE (sv->priv->goto_line));
		sv->priv->goto_line = -1;
	}
	anjuta_view_scroll_to_cursor(sv->priv->view);
	sv->priv->loading = FALSE;
	
	sourceview_add_monitor(sv);

	/* Autodetect language */
	ianjuta_editor_language_set_language(IANJUTA_EDITOR_LANGUAGE(sv), NULL, NULL);

	lang = ianjuta_editor_language_get_language(IANJUTA_EDITOR_LANGUAGE(sv), NULL);
	g_signal_emit_by_name (sv, "language-changed", lang);
	
	/* Get rid of reference from ifile_open */
	g_object_unref(G_OBJECT(sv));
}

/* Show nice progress bar */
static void on_document_loading(AnjutaDocument    *document,
					 GnomeVFSFileSize  size,
					 GnomeVFSFileSize  total_size,
					 Sourceview* sv)
{
	AnjutaShell* shell;
	AnjutaStatus* status;

	g_object_get(G_OBJECT(sv->priv->plugin), "shell", &shell, NULL);
	status = anjuta_shell_get_status(shell, NULL);
	
	if (!sv->priv->loading)
	{
		gint procentage = 0;
		if (size)
			procentage = total_size/size;
		anjuta_status_progress_add_ticks(status,procentage + 1);
		sv->priv->loading = TRUE;
	}
	anjuta_status_progress_tick(status, NULL,  _("Loading"));
}

/* Show nice progress bar */
static void on_document_saving(AnjutaDocument    *document,
					 GnomeVFSFileSize  size,
					 GnomeVFSFileSize  total_size,
					 Sourceview* sv)
{
	AnjutaShell* shell;
	AnjutaStatus* status;

	g_object_get(G_OBJECT(sv->priv->plugin), "shell", &shell, NULL);
	status = anjuta_shell_get_status(shell, NULL);
	
	if (!sv->priv->saving)
	{
		gint procentage = 0;
		if (size)
			procentage = total_size/size;
		anjuta_status_progress_add_ticks(status,procentage + 1);
		sv->priv->saving = TRUE;
	}
	anjuta_status_progress_tick(status, NULL, _("Saving..."));
}

static gboolean save_if_modified(AnjutaDocument* doc, GtkWindow* parent)
{
	GtkWidget* dialog = gtk_message_dialog_new(parent,
    	GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, 
    	_("The file %s was modified by another application. Save it anyway?"), anjuta_document_get_uri_for_display(doc));
    int result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    switch (result)
    {
    	case GTK_RESPONSE_YES:
    	{
    		return TRUE;
   		}
   		default:
   			return FALSE;
   	}
}

/* Need to avoid crash when unref is done before add_monitor */
static gboolean timeout_unref(Sourceview* sv)
{
	g_object_unref(G_OBJECT(sv));
	return FALSE;
}

/* Called when document is saved completly */
static void on_document_saved(AnjutaDocument* doc, GError* err, Sourceview* sv)
{
	if (err)
	{
		switch(err->code)
		{
			case ANJUTA_DOCUMENT_ERROR_EXTERNALLY_MODIFIED:
			{
    			if (save_if_modified(doc, GTK_WINDOW(sv->priv->plugin->shell)))
    			{
    				anjuta_document_save(doc, ANJUTA_DOCUMENT_SAVE_IGNORE_MTIME);
    			}
    			break;
			}
			default:
			{
				anjuta_util_dialog_error(NULL,
				 		"Could not save file %s: %s",anjuta_document_get_uri_for_display(doc),  err->message);
				break;
			}
		}
	}
	else
	{
		const gchar* lang;
		gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(doc), FALSE);
		g_signal_emit_by_name(G_OBJECT(sv), "save_point", TRUE);
		/* Set up 2 sec timer */
		if (sv->priv->monitor_delay > 0)
 			g_source_remove (sv->priv->monitor_delay);
		sv->priv->monitor_delay = g_timeout_add (2000,
							(GSourceFunc)sourceview_add_monitor, sv);
		sv->priv->saving = FALSE;
		/* Autodetect language */
		ianjuta_editor_language_set_language(IANJUTA_EDITOR_LANGUAGE(sv), NULL, NULL);
		lang = ianjuta_editor_language_get_language(IANJUTA_EDITOR_LANGUAGE(sv), NULL);
		g_signal_emit_by_name (sv, "language-changed", lang);
	}
	g_timeout_add(3000, (GSourceFunc)timeout_unref, sv);
}

static void 
sourceview_instance_init(Sourceview* sv)
{
	sv->priv = g_slice_new0 (SourceviewPrivate);
}

static void
sourceview_class_init(SourceviewClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	parent_class = g_type_class_peek_parent(klass);
	object_class->dispose = sourceview_dispose;
	object_class->finalize = sourceview_finalize;
	
	/* Create signals here:
	   sourceview_signals[SIGNAL_TYPE_EXAMPLE] = g_signal_new(...)
 	*/
}

static void
sourceview_dispose(GObject *object)
{
	Sourceview *cobj = ANJUTA_SOURCEVIEW(object);
	GSList* node;
	if (cobj->priv->assist_win)
		on_assist_cancel(cobj->priv->assist_win, cobj);
	if (cobj->priv->assist_tip)
		gtk_widget_destroy(GTK_WIDGET(cobj->priv->assist_tip));
	
	for (node = cobj->priv->idle_sources; node != NULL; node = g_slist_next (node))
	{
		g_source_remove (GPOINTER_TO_UINT (node->data));
	}
	g_slist_free (cobj->priv->idle_sources);
	
	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
sourceview_finalize(GObject *object)
{
	Sourceview *cobj;
	cobj = ANJUTA_SOURCEVIEW(object);
	
	sourceview_remove_monitor(cobj);
	sourceview_prefs_destroy(cobj);
	
	g_object_unref(cobj->priv->view);
	
	g_slice_free(SourceviewPrivate, cobj->priv);
	G_OBJECT_CLASS(parent_class)->finalize(object);
	DEBUG_PRINT("=========== finalise =============");
}

/* Sync with IANJUTA_MARKABLE_MARKER  */

#define MARKER_PIXMAP_LINEMARKER "anjuta-linemark-16.png"
#define MARKER_PIXMAP_PROGRAM_COUNTER "anjuta-pcmark-16.png"
#define MARKER_PIXMAP_BREAKPOINT_DISABLED "anjuta-breakpoint-disabled-16.png"
#define MARKER_PIXMAP_BREAKPOINT_ENABLED "anjuta-breakpoint-enabled-16.png"
#define MARKER_PIXMAP_BOOKMARK "anjuta-bookmark-16.png"


/* Keep in sync with IAnjutaMarkableMarker */

static const gchar* marker_types [] =
{
	"sv-linemarker",
	"sv-bookmark",
	"sv-breakpoint-enabled",
	"sv-breakpoint-disabled",
	"sv-program-counter",
	NULL
};

/* HIGHLIGHTED TAGS */

#define IMPORTANT_INDIC "important_indic"
#define WARNING_INDIC "warning_indic"
#define CRITICAL_INDIC "critical_indic"

/* Create pixmaps for the markers */
static void sourceview_create_markers(Sourceview* sv)
{
	GdkPixbuf * pixbuf;
	GtkSourceView* view = 	GTK_SOURCE_VIEW(sv->priv->view);

	
	if ((pixbuf = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"MARKER_PIXMAP_BOOKMARK, NULL)))
	{
		gtk_source_view_set_mark_category_pixbuf (view, 
			marker_types[IANJUTA_MARKABLE_BOOKMARK], pixbuf);
		gtk_source_view_set_mark_category_priority (view, marker_types [IANJUTA_MARKABLE_BOOKMARK],
											 IANJUTA_MARKABLE_BOOKMARK);
		g_object_unref (pixbuf);
	}
	if ((pixbuf = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"MARKER_PIXMAP_BREAKPOINT_DISABLED, NULL)))
	{
		gtk_source_view_set_mark_category_pixbuf (view, 
			marker_types[IANJUTA_MARKABLE_BREAKPOINT_DISABLED], pixbuf);
		gtk_source_view_set_mark_category_priority (view, marker_types [IANJUTA_MARKABLE_BREAKPOINT_DISABLED],
											 IANJUTA_MARKABLE_BREAKPOINT_DISABLED);

		g_object_unref (pixbuf);
	}
	if ((pixbuf = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"MARKER_PIXMAP_BREAKPOINT_ENABLED, NULL)))
	{
		gtk_source_view_set_mark_category_pixbuf (view, 
			marker_types[IANJUTA_MARKABLE_BREAKPOINT_ENABLED], pixbuf);
		gtk_source_view_set_mark_category_priority (view, marker_types [IANJUTA_MARKABLE_BREAKPOINT_ENABLED],
											 IANJUTA_MARKABLE_BREAKPOINT_ENABLED);
		g_object_unref (pixbuf);
	}
	if ((pixbuf = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"MARKER_PIXMAP_PROGRAM_COUNTER, NULL)))
	{
		gtk_source_view_set_mark_category_pixbuf (view, 
			marker_types[IANJUTA_MARKABLE_PROGRAM_COUNTER], pixbuf);
		gtk_source_view_set_mark_category_priority (view, marker_types [IANJUTA_MARKABLE_PROGRAM_COUNTER],
											 IANJUTA_MARKABLE_PROGRAM_COUNTER);
		g_object_unref (pixbuf);
	}
	if ((pixbuf = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"MARKER_PIXMAP_LINEMARKER, NULL)))
	{
		gtk_source_view_set_mark_category_pixbuf (view, 
			marker_types[IANJUTA_MARKABLE_LINEMARKER], pixbuf);
		gtk_source_view_set_mark_category_priority (view, marker_types [IANJUTA_MARKABLE_LINEMARKER],
											 IANJUTA_MARKABLE_LINEMARKER);
		g_object_unref (pixbuf);
	}
}

/* Create tags for highlighting */
static void sourceview_create_highligth_indic(Sourceview* sv)
{	
	sv->priv->important_indic = 
		gtk_text_buffer_create_tag (GTK_TEXT_BUFFER(sv->priv->document),
									IMPORTANT_INDIC,
									"background", "#FFFF00", NULL);  
	sv->priv->warning_indic = 
		gtk_text_buffer_create_tag (GTK_TEXT_BUFFER(sv->priv->document),
									WARNING_INDIC,
									"foreground", "#00FF00", NULL); 
	sv->priv->critical_indic = 
		gtk_text_buffer_create_tag (GTK_TEXT_BUFFER(sv->priv->document),
									CRITICAL_INDIC,
									"foreground", "#FF0000", "underline", 
									PANGO_UNDERLINE_ERROR, NULL);
}


/* Create a new sourceview instance. If uri is valid,
the file will be loaded in the buffer */

Sourceview *
sourceview_new(const gchar* uri, const gchar* filename, AnjutaPlugin* plugin)
{
	AnjutaShell* shell;
	
	Sourceview *sv = ANJUTA_SOURCEVIEW(g_object_new(ANJUTA_TYPE_SOURCEVIEW, NULL));
	
	/* Create buffer */
	sv->priv->document = anjuta_document_new();
	g_signal_connect_after(G_OBJECT(sv->priv->document), "modified-changed", 
					 G_CALLBACK(on_document_modified_changed), sv);
	g_signal_connect_after(G_OBJECT(sv->priv->document), "cursor-moved", 
					 G_CALLBACK(on_cursor_moved),sv);
	g_signal_connect_after(G_OBJECT(sv->priv->document), "loaded", 
					 G_CALLBACK(on_document_loaded), sv);
	g_signal_connect(G_OBJECT(sv->priv->document), "loading", 
					 G_CALLBACK(on_document_loading), sv);				 
	g_signal_connect_after(G_OBJECT(sv->priv->document), "saved", 
					 G_CALLBACK(on_document_saved), sv);
	g_signal_connect(G_OBJECT(sv->priv->document), "saving", 
					 G_CALLBACK(on_document_saving), sv);
	g_signal_connect_after (G_OBJECT(sv->priv->document), "insert-text",
					  G_CALLBACK(on_insert_text), sv);
					 
	/* Create View instance */
	sv->priv->view = ANJUTA_VIEW(anjuta_view_new(sv));
#if HAVE_TOOLTIP_API
	g_signal_connect (G_OBJECT(sv->priv->view), "query-tooltip",
					  G_CALLBACK (on_sourceview_hover_over), sv);
	g_object_set (G_OBJECT (sv->priv->view), "has-tooltip", TRUE, NULL);
#endif
	gtk_source_view_set_smart_home_end(GTK_SOURCE_VIEW(sv->priv->view), FALSE);
	g_object_ref(sv->priv->view);
	
	/* VFS monitor */
	sv->priv->last_saved_content = NULL;
	
	/* Apply Preferences */
	g_object_get(G_OBJECT(plugin), "shell", &shell, NULL);
	sv->priv->prefs = anjuta_shell_get_preferences(shell, NULL);
	sourceview_prefs_init(sv);
	sv->priv->plugin = plugin;
	
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
	
	DEBUG_PRINT("============ Creating new editor =============");
	
	return sv;
}

/* IAnjutaFile interface */

/* Open uri in Editor */
static void
ifile_open (IAnjutaFile* file, const gchar *uri, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(file);
	sourceview_remove_monitor(sv);
	/* Hold a reference here to avoid a destroyed editor */
	g_object_ref(G_OBJECT(sv));
	anjuta_document_load(sv->priv->document, uri, NULL,
						 -1, FALSE);
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
	
	g_object_ref(G_OBJECT(sv));
	anjuta_document_save(sv->priv->document, 0);
}

/* Save file as */
static void 
ifile_savable_save_as (IAnjutaFileSavable* file, const gchar *uri, GError** e)
{
	GtkTextIter start_iter;
	GtkTextIter end_iter;
	Sourceview* sv = ANJUTA_SOURCEVIEW(file);
	sourceview_remove_monitor(sv);
	/* TODO: Set correct encoding */
	gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER(sv->priv->document),
								&start_iter, &end_iter);
	g_free(sv->priv->last_saved_content);
	sv->priv->last_saved_content = gtk_text_buffer_get_slice (
															  GTK_TEXT_BUFFER(sv->priv->document),
															  &start_iter, &end_iter, TRUE);
	g_object_ref(G_OBJECT(sv));
	anjuta_document_save_as(sv->priv->document, 
							uri, anjuta_encoding_get_current(), 0);
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
		anjuta_document_goto_line(sv->priv->document, LOCATION_TO_LINE (line));
		anjuta_view_scroll_to_cursor(sv->priv->view);
	}
	else
		sv->priv->goto_line = line;
}

/* Scroll to position */
static void ieditor_goto_position(IAnjutaEditor *editor, IAnjutaIterable* icell,
								  GError **e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL (icell);
	GtkTextIter* iter = sourceview_cell_get_iter (cell);
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (sv->priv->document), iter);
	gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (sv->priv->view),
								  iter, 0, FALSE, 0, 0);
}

/* Return a newly allocated pointer containing the whole text */
static gchar* ieditor_get_text (IAnjutaEditor* editor, 
								IAnjutaIterable* start,
								IAnjutaIterable* end, GError **e)
{
	GtkTextIter* start_iter;
	GtkTextIter* end_iter;
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	
	start_iter = sourceview_cell_get_iter (SOURCEVIEW_CELL (start));
	end_iter = sourceview_cell_get_iter (SOURCEVIEW_CELL (end));
	
	return gtk_text_buffer_get_slice(GTK_TEXT_BUFFER(sv->priv->document),
									start_iter, end_iter, TRUE);
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
	text = gtk_text_buffer_get_slice(GTK_TEXT_BUFFER(sv->priv->document),
									&start_iter, &end_iter, TRUE);
	length = g_utf8_strlen(text,  -1);
	g_free(text);

	return length;
}

/* Return word on cursor position */
static gchar* ieditor_get_current_word(IAnjutaEditor *editor, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	return anjuta_document_get_current_word(sv->priv->document, FALSE);
}

/* Insert text at position */
static void ieditor_insert(IAnjutaEditor *editor, IAnjutaIterable* icell,
							   const gchar* text, gint length, GError **e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL (icell);
	GtkTextIter* iter = sourceview_cell_get_iter (cell);
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	
	g_signal_handlers_block_by_func (sv->priv->document,
										  on_insert_text,
										  sv);
	gtk_text_buffer_insert(GTK_TEXT_BUFFER(sv->priv->document),
						   iter, text, length);
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
	gtk_text_buffer_insert(GTK_TEXT_BUFFER(sv->priv->document),
						   &iter, text, length);
}

static void ieditor_erase(IAnjutaEditor* editor, IAnjutaIterable* istart_cell, 
						  IAnjutaIterable* iend_cell, GError **e)
{
	SourceviewCell* start_cell = SOURCEVIEW_CELL (istart_cell);
	GtkTextIter* start = sourceview_cell_get_iter (start_cell);
	SourceviewCell* end_cell = SOURCEVIEW_CELL (iend_cell);
	GtkTextIter* end = sourceview_cell_get_iter (end_cell);
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->document);
	
	gtk_text_buffer_delete (buffer, start, end);
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
	GtkTextIter* iter = sourceview_cell_get_iter (cell);
	return LINE_TO_LOCATION (gtk_text_iter_get_line(iter));
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
		gtk_source_buffer_undo(GTK_SOURCE_BUFFER(sv->priv->document));
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
	if (sv->priv->filename == NULL)
		sv->priv->filename = anjuta_document_get_short_name_for_display(sv->priv->document);
	return sv->priv->filename;
}

static void 
idocument_cut(IAnjutaDocument* edit, GError** ee)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	anjuta_view_cut_clipboard(sv->priv->view);
}

static void 
idocument_copy(IAnjutaDocument* edit, GError** ee)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	anjuta_view_copy_clipboard(sv->priv->view);
}

static void 
idocument_paste(IAnjutaDocument* edit, GError** ee)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	anjuta_view_paste_clipboard(sv->priv->view);
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
set_select(Sourceview* sv, GtkTextIter* start_iter, GtkTextIter* end_iter)
{
	gtk_text_buffer_select_range (GTK_TEXT_BUFFER (sv->priv->document), start_iter, end_iter);
	gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(sv->priv->view),
								 gtk_text_buffer_get_insert(GTK_TEXT_BUFFER(sv->priv->document)));
}

/* IAnjutaEditorSelection */

// TODO: Move these to ilanguage-support?
static void
iselect_to_brace(IAnjutaEditorSelection* edit, GError** e)
{
	
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
	text = gtk_text_buffer_get_slice
	(GTK_TEXT_BUFFER(sv->priv->document),
										&start_iter, &end_iter, TRUE);
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
			found = FALSE;
			//found = gtk_source_iter_find_matching_bracket (&end_iter);
			if (found)
				set_select(sv, &start_iter, &end_iter);
		}
		g_free(text);
	}

}

static void
iselect_set (IAnjutaEditorSelection* edit, 
			 IAnjutaIterable* istart,
			 IAnjutaIterable* iend,
			 GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	gtk_text_buffer_select_range (GTK_TEXT_BUFFER (sv->priv->document),
								  sourceview_cell_get_iter (SOURCEVIEW_CELL (istart)),
								  sourceview_cell_get_iter (SOURCEVIEW_CELL (iend)));
	gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (sv->priv->view),
												 sourceview_cell_get_iter (SOURCEVIEW_CELL (istart)),
												 0, FALSE, 0, 0);
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
	iface->select_to_brace = iselect_to_brace;
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
  GtkTextIter* start = sourceview_cell_get_iter (SOURCEVIEW_CELL (start_position));
  GtkTextIter* end = sourceview_cell_get_iter (SOURCEVIEW_CELL (end_position));
	
  gchar* text_buffer = gtk_text_buffer_get_text (buffer,
											start, end, TRUE);
  gtk_text_buffer_begin_user_action (buffer);
  gtk_text_buffer_delete (buffer, start, end);
  gtk_text_buffer_insert (buffer, start, g_utf8_strup (text_buffer, -1), -1);
  gtk_text_buffer_end_user_action (buffer);
  g_free (text_buffer);
}

static void
iconvert_to_lower(IAnjutaEditorConvert* edit, IAnjutaIterable *start_position,
				  IAnjutaIterable *end_position, GError** e)
{
  Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
  GtkTextBuffer* buffer = GTK_TEXT_BUFFER (sv->priv->document);
  GtkTextIter* start = sourceview_cell_get_iter (SOURCEVIEW_CELL (start_position));
  GtkTextIter* end = sourceview_cell_get_iter (SOURCEVIEW_CELL (end_position));
	
  gchar* text_buffer = gtk_text_buffer_get_text (buffer,
											start, end, TRUE);
  gtk_text_buffer_begin_user_action (buffer);
  gtk_text_buffer_delete (buffer, start, end);
  gtk_text_buffer_insert (buffer, start, g_utf8_strdown (text_buffer, -1), -1);
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
	IAnjutaMarkableMarker marker = svmark->marker;
	gchar* name;
	
	if (sv->priv->loading || sv->priv->saving)
	{
		/* Wait until loading/saving is finished */
		return TRUE;
	}
	
	gtk_text_buffer_get_iter_at_line(GTK_TEXT_BUFFER(sv->priv->document),
									 &iter, LOCATION_TO_LINE (location));
	
	category = marker_types[marker];
	name = CREATE_MARK_NAME (marker_count);
	
	
	source_mark = gtk_source_buffer_create_source_mark(GTK_SOURCE_BUFFER(sv->priv->document), 
												name, category, &iter);
	
	g_source_remove (svmark->source);
	
	g_free (name);
	g_slice_free (SVMark, svmark);
	return FALSE;
}

static gint
imark_mark(IAnjutaMarkable* mark, gint location, IAnjutaMarkableMarker marker,
		   GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(mark);
	SVMark* svmark = g_slice_new0 (SVMark);
	
	static gint marker_count = 0;
	
	marker_count++;
	
	svmark->sv = sv;
	svmark->location = location;
	svmark->handle = marker_count;
	svmark->marker = marker;
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

	gtk_text_buffer_apply_tag (GTK_TEXT_BUFFER(sv->priv->document), tag, 
	                           sourceview_cell_get_iter (SOURCEVIEW_CELL (ibegin)),
							   sourceview_cell_get_iter (SOURCEVIEW_CELL (iend)));
}

static void
iindic_iface_init(IAnjutaIndicableIface* iface)
{
	iface->clear = iindic_clear;
	iface->set = iindic_set;
}

static void
ibookmark_toggle(IAnjutaBookmark* bmark, gint location, gboolean ensure_visible, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(bmark);
	GtkSourceBuffer* buffer = GTK_SOURCE_BUFFER(sv->priv->document);
	
	GSList* markers;
	
	markers = gtk_source_buffer_get_source_marks_at_line (buffer, LOCATION_TO_LINE (location),
												   marker_types[IANJUTA_MARKABLE_BOOKMARK]);
	if (markers != NULL)
	{
		GtkTextIter begin;
		GtkTextIter end;
		
		gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (buffer), &begin, LOCATION_TO_LINE (location));
		gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (buffer), &end, LOCATION_TO_LINE (location));
		
		gtk_source_buffer_remove_source_marks (buffer, &begin, &end, 
										marker_types[IANJUTA_MARKABLE_BOOKMARK]);
	}
	else
	{
		GtkTextIter line;
		GtkSourceMark* bookmark;
		gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (buffer),
															 &line, LOCATION_TO_LINE (location));
		
		bookmark = 
			gtk_source_buffer_create_source_mark (buffer, NULL,
											 marker_types [IANJUTA_MARKABLE_BOOKMARK],
											 &line);
	}
}

static void
goto_bookmark (Sourceview* sv, GtkTextIter* iter, gboolean backward)
{
	GtkSourceBuffer* buffer = GTK_SOURCE_BUFFER(sv->priv->document);
	gboolean found = FALSE;
	
	if (backward)
		found = gtk_source_buffer_backward_iter_to_source_mark (buffer, iter, 
														 marker_types[IANJUTA_MARKABLE_BOOKMARK]);
	else
		found = gtk_source_buffer_forward_iter_to_source_mark (buffer, iter, 
														marker_types[IANJUTA_MARKABLE_BOOKMARK]);
	if (found)
	{
		ianjuta_editor_goto_line(IANJUTA_EDITOR(sv), 
								 LINE_TO_LOCATION (gtk_text_iter_get_line (iter)), NULL);
	}
}

static void
ibookmark_first(IAnjutaBookmark* bmark, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(bmark);
	GtkSourceBuffer* buffer = GTK_SOURCE_BUFFER(sv->priv->document);
	GtkTextIter begin;
	
	gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (buffer), &begin, 0);
	
	goto_bookmark (sv, &begin, FALSE);
}


static void
ibookmark_last(IAnjutaBookmark* bmark, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(bmark);
	GtkSourceBuffer* buffer = GTK_SOURCE_BUFFER(sv->priv->document);;
	GtkTextIter begin;
	
	gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (buffer), &begin, -1);
	
	goto_bookmark (sv, &begin, TRUE);
}

static void
ibookmark_next(IAnjutaBookmark* bmark, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(bmark);
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->document);
	GtkTextIter begin;
	
	gtk_text_buffer_get_iter_at_mark (buffer, &begin,
									  gtk_text_buffer_get_insert (buffer));
	gtk_text_iter_forward_line (&begin);
	
	goto_bookmark (sv, &begin, FALSE);
}

static void
ibookmark_previous(IAnjutaBookmark* bmark, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(bmark);
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->document);
	GtkTextIter begin;
	
	gtk_text_buffer_get_iter_at_mark (buffer, &begin,
									  gtk_text_buffer_get_insert (buffer));
	gtk_text_iter_backward_line (&begin);
	
	goto_bookmark (sv, &begin, TRUE);

}

static void
ibookmark_clear_all(IAnjutaBookmark* bmark, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(bmark);
	GtkSourceBuffer* buffer = GTK_SOURCE_BUFFER(sv->priv->document);
	GtkTextIter begin;
	GtkTextIter end;
	
	gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (buffer), &begin, 0);
	gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (buffer), &end, -1);
	
	gtk_source_buffer_remove_source_marks (buffer, &begin, &end, marker_types[IANJUTA_MARKABLE_BOOKMARK]);
}

static void
ibookmark_iface_init(IAnjutaBookmarkIface* iface)
{
	iface->toggle = ibookmark_toggle;
	iface->first = ibookmark_first;
	iface->last = ibookmark_last;
	iface->next = ibookmark_next;
	iface->previous = ibookmark_previous;
	iface->clear_all = ibookmark_clear_all;
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
		GStrv langs;
		GStrv lang;
		g_object_get (gtk_source_language_manager_get_default(), "language-ids", &langs, NULL);
		
		for (lang = langs; *lang != NULL; lang++)
		{
			languages = g_list_append (languages, *lang);
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
	const gchar* uri = anjuta_document_get_uri (sv->priv->document);
	const gchar* vfs_mime_type = gnome_vfs_get_mime_type (uri);
	GStrv languages;
	GStrv cur_lang;
	const gchar* detected_language = NULL;
	g_object_get (G_OBJECT (gtk_source_language_manager_get_default ()), "language-ids",
							&languages, NULL);
	if (!vfs_mime_type)
	{
		vfs_mime_type = 
			gnome_vfs_get_mime_type_for_name (ianjuta_document_get_filename (IANJUTA_DOCUMENT (sv),
																			 NULL));
	}
	if (!vfs_mime_type)
		return NULL;
	
	for (cur_lang = languages; *cur_lang != NULL; cur_lang++)
	{
		GtkSourceLanguage* language = 
			gtk_source_language_manager_get_language (gtk_source_language_manager_get_default(),
													  *cur_lang);
		GStrv mime_types = gtk_source_language_get_mime_types (language);
		if (!mime_types)
			continue;
		GStrv mime_type;
		for (mime_type = mime_types; *mime_type != NULL; mime_type++)
		{
			if (g_str_equal (*mime_type, vfs_mime_type))
			{
				detected_language = gtk_source_language_get_id (language);				
				g_signal_emit_by_name (G_OBJECT(sv), "language-changed", 
									   detected_language);
				gtk_source_buffer_set_language (GTK_SOURCE_BUFFER (sv->priv->document), language);
				g_strfreev (mime_types);
				goto out;
			}
		}
		g_strfreev (mime_types);
	}
	out:
		g_strfreev(languages);
	
	return detected_language;
}

static void
ilanguage_set_language (IAnjutaEditorLanguage *ilanguage,
						const gchar *language, GError **err)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW (ilanguage);
	gboolean found = FALSE;
	GStrv languages;
	GStrv cur_lang;
	g_object_get (G_OBJECT (gtk_source_language_manager_get_default ()), "language-ids",
							&languages, NULL);
	for (cur_lang = languages; *cur_lang != NULL && language != NULL; cur_lang++)
	{
		GtkSourceLanguage* source_language = 
			gtk_source_language_manager_get_language (gtk_source_language_manager_get_default(),
													  *cur_lang);
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
	g_strfreev(languages);
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
		return gtk_source_language_get_name(lang);
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

static GList*
iassist_get_suggestions (IAnjutaEditorAssist *iassist, const gchar *context, GError **err)
{
	/* Sourceview* sv = ANJUTA_SOURCEVIEW(iassist); */
	/* We don't make own suggestions yet */
	return NULL;
}

static void
iassist_suggest (IAnjutaEditorAssist *iassist, GList* choices, IAnjutaIterable* ipos,
				 int char_alignment, GError **err)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(iassist);
	
	if (choices == NULL)
	{
		if (sv->priv->assist_win)
			gtk_widget_destroy(GTK_WIDGET(sv->priv->assist_win));
	}
	else
	{
		if (!sv->priv->assist_win)
		{
			sv->priv->assist_win = assist_window_new(GTK_TEXT_VIEW(sv->priv->view), NULL,
													 ianjuta_iterable_get_position (ipos, NULL));
			g_signal_connect(G_OBJECT(sv->priv->assist_win), "destroy", 
								 G_CALLBACK(on_assist_window_destroyed), sv);
			g_signal_connect(G_OBJECT(sv->priv->assist_win), "chosen", 
								 G_CALLBACK(on_assist_chosen), sv);
			g_signal_connect(G_OBJECT(sv->priv->assist_win), "cancel", 
								 G_CALLBACK(on_assist_cancel), sv);
		}
		assist_window_update(sv->priv->assist_win, choices);
		gtk_widget_show(GTK_WIDGET(sv->priv->assist_win));
		if (char_alignment > 0)
		{
			/* Calculate offset */
			GtkTextIter cursor;
			GtkTextBuffer* buffer = GTK_TEXT_BUFFER (sv->priv->document);
			gtk_text_buffer_get_iter_at_mark (buffer,
											  &cursor,
											  gtk_text_buffer_get_insert(buffer));
			
			gint offset = gtk_text_iter_get_offset (&cursor);
			assist_window_move(sv->priv->assist_win, offset - char_alignment);
		}
	}
}

static void
iassist_hide_suggestions (IAnjutaEditorAssist* iassist, GError** err)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(iassist);
	if (sv->priv->assist_win)
	{
		gtk_widget_hide (GTK_WIDGET (sv->priv->assist_win));
	}
}

static void 
iassist_show_tips (IAnjutaEditorAssist *iassist, GList* tips, IAnjutaIterable* ipos,
				   gint char_alignment, GError **err)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(iassist);
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER (sv->priv->document);
	GtkTextIter iter;
	gint tip_position;
	gtk_text_buffer_get_iter_at_mark (buffer, &iter,
									  gtk_text_buffer_get_insert (buffer));
	
	tip_position = gtk_text_iter_get_offset (&iter) - char_alignment;
	
	if (tips == NULL)
		return;
	
	if (!sv->priv->assist_tip)
	{
		sv->priv->assist_tip = 
			ASSIST_TIP (assist_tip_new (GTK_TEXT_VIEW (sv->priv->view), tips));
		
		g_signal_connect (G_OBJECT(sv->priv->assist_tip), "destroy", G_CALLBACK(on_assist_tip_destroyed),
						  sv);
		assist_tip_move (sv->priv->assist_tip, GTK_TEXT_VIEW (sv->priv->view), tip_position);
		gtk_widget_show (GTK_WIDGET (sv->priv->assist_tip));
	}
	else
	{
		assist_tip_set_tips (sv->priv->assist_tip, tips);
		assist_tip_move (sv->priv->assist_tip, GTK_TEXT_VIEW (sv->priv->view), tip_position);
	}
}

static void
iassist_cancel_tips (IAnjutaEditorAssist* iassist, GError** err)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(iassist);
	if (sv->priv->assist_tip)
		gtk_widget_destroy (GTK_WIDGET (sv->priv->assist_tip));
}

static void
iassist_iface_init(IAnjutaEditorAssistIface* iface)
{
	iface->suggest = iassist_suggest;
	iface->hide_suggestions = iassist_hide_suggestions;
	iface->get_suggestions = iassist_get_suggestions;
	iface->show_tips = iassist_show_tips;
	iface->cancel_tips = iassist_cancel_tips;
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
	
	GtkTextIter* start_iter = sourceview_cell_get_iter (start);
	GtkTextIter* end_iter = sourceview_cell_get_iter (end);
	
	GtkTextIter result_start, result_end;
	
	GtkSourceSearchFlags flags = 0;
	
	if (!case_sensitive)
	{
		flags = GTK_SOURCE_SEARCH_CASE_INSENSITIVE;
	}
	
	gboolean result = 
		gtk_source_iter_forward_search (start_iter, search, flags, &result_start, &result_end,
									end_iter);

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
	
	GtkTextIter* start_iter = sourceview_cell_get_iter (start);
	GtkTextIter* end_iter = sourceview_cell_get_iter (end);
	
	GtkTextIter result_start, result_end;
	
	GtkSourceSearchFlags flags = 0;
	
	if (!case_sensitive)
	{
		flags = GTK_SOURCE_SEARCH_CASE_INSENSITIVE;
	}
	
	gboolean result = 
		gtk_source_iter_backward_search (start_iter, search, flags, &result_start, &result_end,
									end_iter);

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
#if HAVE_TOOLTIP_API
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
	gint bx, by, trailing;
		
	gtk_text_view_window_to_buffer_coords (text_view, GTK_TEXT_WINDOW_TEXT,
											   x, y, &bx, &by);
	gtk_text_view_get_iter_at_position (text_view, &iter, &trailing, bx, by);
	
	cell = sourceview_cell_new (&iter, text_view);
	
	/* This should call ianjuta_hover_display() */
	g_signal_emit_by_name (G_OBJECT (sv), "hover-over", cell);
	
	if (sv->priv->tooltip)
	{
		gtk_tooltip_set_text (tooltip, sv->priv->tooltip);
		g_object_weak_ref (G_OBJECT (tooltip), on_sourceview_hover_leave, sv);
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
#endif

ANJUTA_TYPE_BEGIN(Sourceview, sourceview, GTK_TYPE_SCROLLED_WINDOW);
ANJUTA_TYPE_ADD_INTERFACE(idocument, IANJUTA_TYPE_DOCUMENT);
ANJUTA_TYPE_ADD_INTERFACE(ifile, IANJUTA_TYPE_FILE);
ANJUTA_TYPE_ADD_INTERFACE(isavable, IANJUTA_TYPE_FILE_SAVABLE);
ANJUTA_TYPE_ADD_INTERFACE(ieditor, IANJUTA_TYPE_EDITOR);
ANJUTA_TYPE_ADD_INTERFACE(imark, IANJUTA_TYPE_MARKABLE);
ANJUTA_TYPE_ADD_INTERFACE(iindic, IANJUTA_TYPE_INDICABLE);
ANJUTA_TYPE_ADD_INTERFACE(iselect, IANJUTA_TYPE_EDITOR_SELECTION);
ANJUTA_TYPE_ADD_INTERFACE(iassist, IANJUTA_TYPE_EDITOR_ASSIST);
ANJUTA_TYPE_ADD_INTERFACE(iconvert, IANJUTA_TYPE_EDITOR_CONVERT);
ANJUTA_TYPE_ADD_INTERFACE(ibookmark, IANJUTA_TYPE_BOOKMARK);
ANJUTA_TYPE_ADD_INTERFACE(iprint, IANJUTA_TYPE_PRINT);
ANJUTA_TYPE_ADD_INTERFACE(ilanguage, IANJUTA_TYPE_EDITOR_LANGUAGE);
ANJUTA_TYPE_ADD_INTERFACE(isearch, IANJUTA_TYPE_EDITOR_SEARCH);
#if HAVE_TOOLTIP_API
ANJUTA_TYPE_ADD_INTERFACE(ihover, IANJUTA_TYPE_EDITOR_HOVER);
#endif
ANJUTA_TYPE_END;
