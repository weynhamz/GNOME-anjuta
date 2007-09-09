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
#include "anjuta-document.h"
#include "anjuta-view.h"

#include "sourceview.h"
#include "sourceview-private.h"
#include "sourceview-prefs.h"
#include "sourceview-print.h"
#include "sourceview-cell.h"
#include "plugin.h"

#define FORWARD 	0
#define BACKWARD 	1

#define MONITOR_KEY "sourceview.enable.vfs"

static void sourceview_class_init(SourceviewClass *klass);
static void sourceview_instance_init(Sourceview *sv);
static void sourceview_finalize(GObject *object);
static void sourceview_dispose(GObject *object);
static GObjectClass *parent_class = NULL;

static gboolean sourceview_add_monitor(Sourceview* sv);

/* Callbacks */

static void
on_assist_window_destroyed (AssistWindow* window, Sourceview* sv)
{
	g_signal_emit_by_name(G_OBJECT(sv), "assist_end");
	sv->priv->assist_active = FALSE;
	sv->priv->assist_win = NULL;
}

static void 
on_assist_tip_destroyed (AssistTip* tip, Sourceview* sv)
{
	sv->priv->assist_tip = NULL;
	sv->priv->tip_active = FALSE;
	g_signal_emit_by_name(G_OBJECT(sv), "assist_end");
	DEBUG_PRINT("Assist-tip-cancel");
}

static void 
on_assist_chosen(AssistWindow* assist_win, gint num, Sourceview* sv)
{
	g_signal_emit_by_name(G_OBJECT(sv), "assist_chosen", num);
	DEBUG_PRINT("Assist-choosen");
}

static void 
on_assist_cancel(AssistWindow* assist_win, Sourceview* sv)
{
	g_signal_emit_by_name(G_OBJECT(sv), "assist_canceled");
	DEBUG_PRINT("Assist-cancel");
	if (sv->priv->assist_win)
	{
		gtk_widget_destroy(GTK_WIDGET(sv->priv->assist_win));
	}
	else
		sv->priv->assist_active = FALSE;
}

static void
trigger_size (gpointer pkey, gpointer value, gpointer data)
{
	int* size = (int*)data;
	const gchar* trigger = (const char*) pkey;
	int new_size = strlen (trigger);

	if (new_size > *size)
	{
		*size = new_size; 
	}
}

static int 
max_trigger_size (GHashTable* triggers)
{
	int max_size = 0;
	if (!triggers)
		return 0;
	g_hash_table_foreach (triggers, (GHFunc) trigger_size, &max_size);
	return max_size;
}

/* Called when a character is added */
static void on_document_char_added(AnjutaView* view, gint pos,
								   gchar ch,
								   Sourceview* sv)
{
	gint i;
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->document);
	GtkTextIter begin;
	GtkTextIter end;
	gboolean found = FALSE;
	DEBUG_PRINT("on_document_char_added, assist: %d", sv->priv->assist_win != NULL);
	if (sv->priv->assist_active)
	{
		gtk_text_buffer_get_iter_at_mark(buffer, &end,
										 gtk_text_buffer_get_insert(buffer));
		gtk_text_buffer_get_iter_at_offset(buffer, &begin,
										   assist_window_get_position(sv->priv->assist_win));
		gchar* context = gtk_text_buffer_get_text(buffer, &begin, &end, FALSE);
		g_signal_emit_by_name(G_OBJECT(sv), "assist_update", context);
	}
	else if (sv->priv->tip_active)
	{
		gtk_text_buffer_get_iter_at_mark(buffer, &end,
										 gtk_text_buffer_get_insert(buffer));
		gtk_text_buffer_get_iter_at_offset(buffer, &begin,
										   assist_tip_get_position(sv->priv->assist_tip));
		gchar* context = gtk_text_buffer_get_text(buffer, &begin, &end, FALSE);
		g_signal_emit_by_name(G_OBJECT(sv), "assist_update", context);
	}
	if (!sv->priv->assist_active && !sv->priv->tip_active)
	{
		for (i = 1; i <= max_trigger_size (sv->priv->triggers); i++)
		{
			gchar* text;
			IAnjutaEditorAssistContextParser parser = 0;
			gtk_text_buffer_get_iter_at_mark(buffer, &begin, gtk_text_buffer_get_insert(buffer));
			gtk_text_buffer_get_iter_at_mark(buffer, &end, gtk_text_buffer_get_insert(buffer));		
			if (!gtk_text_iter_backward_chars(&begin, i))
				break;;
			text = gtk_text_buffer_get_text(buffer, &begin, &end, FALSE);
			parser = g_hash_table_lookup(sv->priv->triggers, text);
			if (parser)
			{
				gchar* context = parser(IANJUTA_EDITOR(sv), gtk_text_iter_get_offset(&end));
				g_signal_emit_by_name(G_OBJECT(sv), "assist_begin", context, text);
				g_free(text);
				found = TRUE;
				break;
			}
			g_free(text);
		}
		if (!found)
		{
			if (ch == '\0')
				return;
			gchar* word = anjuta_document_get_current_word(ANJUTA_DOCUMENT(sv->priv->document), TRUE);
			DEBUG_PRINT ("Beginning completion assist: %s", word);
			if (word != NULL && strlen(word) >= 3)
			{
				gtk_text_buffer_get_iter_at_mark(buffer, &begin, gtk_text_buffer_get_insert(buffer));
				gtk_text_iter_backward_chars(&begin, strlen(word));
				gint position =
					gtk_text_iter_get_offset (&begin);
				g_signal_emit_by_name(G_OBJECT(sv), "assist_begin", word, NULL);
			}
		}
	}
	if (ch != '\0')
		g_signal_emit_by_name(G_OBJECT(sv), "char_added", pos, ch);
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
		g_return_val_if_fail(sv->priv->monitor == NULL, FALSE);
		DEBUG_PRINT ("Monitor added for %s", anjuta_document_get_uri(sv->priv->document)); 	
		gnome_vfs_monitor_add(&sv->priv->monitor, anjuta_document_get_uri(sv->priv->document),
							  GNOME_VFS_MONITOR_FILE,
						  on_sourceview_uri_changed, sv);
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
		anjuta_document_goto_line(doc, sv->priv->goto_line - 1);
		sv->priv->goto_line = -1;
	}
	anjuta_view_scroll_to_cursor(sv->priv->view);
	sv->priv->loading = FALSE;
	
	sourceview_add_monitor(sv);

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
		gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(doc), FALSE);
		g_signal_emit_by_name(G_OBJECT(sv), "save_point", TRUE);
		/* Set up 2 sec timer */
		if (sv->priv->monitor_delay > 0)
 			g_source_remove (sv->priv->monitor_delay);
		sv->priv->monitor_delay = g_timeout_add (2000,
							(GSourceFunc)sourceview_add_monitor, sv);
		sv->priv->saving = FALSE;
	}
	g_timeout_add(3000, (GSourceFunc)timeout_unref, sv);
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
	if (cobj->priv->assist_win)
		on_assist_cancel(cobj->priv->assist_win, cobj);
	if (cobj->priv->assist_tip)
		gtk_widget_destroy(GTK_WIDGET(cobj->priv->assist_tip));
	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
sourceview_finalize(GObject *object)
{
	Sourceview *cobj;
	cobj = ANJUTA_SOURCEVIEW(object);
	
	DEBUG_PRINT("=========== finalise =============");
	
	sourceview_remove_monitor(cobj);
	sourceview_prefs_destroy(cobj);
	
	g_object_unref(cobj->priv->view);
	
	g_free(cobj->priv);
	G_OBJECT_CLASS(parent_class)->finalize(object);
}

/* Sync with IANJUTA_MARKABLE_MARKER  */

#define MARKER_PIXMAP_LINEMARKER "linemarker.png"
#define MARKER_PIXMAP_PROGRAM_COUNTER "program-counter.png"
#define MARKER_PIXMAP_BREAKPOINT_DISABLED "breakpoint-disabled.png"
#define MARKER_PIXMAP_BREAKPOINT_ENABLED "breakpoint-enabled.png"

#define MARKER_PIXMAP_BOOKMARK "bookmark.png"

#define MARKER_NONE "sv-none"
#define MARKER_LINEMARKER "sv-linemarker"
#define MARKER_PROGRAM_COUNTER "sv-program-counter"
#define MARKER_BREAKPOINT_DISABLED "sv-breakpoint-disabled"
#define MARKER_BREAKPOINT_ENABLED "sv-breakpoint-enabled"

#define MARKER_BOOKMARK "sv-bookmark"

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
		gtk_source_view_set_marker_pixbuf (view, 
			MARKER_BOOKMARK, pixbuf);
		g_object_unref (pixbuf);
	}
	if ((pixbuf = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"MARKER_PIXMAP_BREAKPOINT_DISABLED, NULL)))
	{
		gtk_source_view_set_marker_pixbuf (view, 
			MARKER_BREAKPOINT_DISABLED, pixbuf);
		g_object_unref (pixbuf);
	}
	if ((pixbuf = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"MARKER_PIXMAP_BREAKPOINT_ENABLED, NULL)))
	{
		gtk_source_view_set_marker_pixbuf (view, 
			MARKER_BREAKPOINT_ENABLED, pixbuf);
		g_object_unref (pixbuf);
	}
	if ((pixbuf = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"MARKER_PIXMAP_PROGRAM_COUNTER, NULL)))
	{
		gtk_source_view_set_marker_pixbuf (view, 
			MARKER_PROGRAM_COUNTER, pixbuf);
		g_object_unref (pixbuf);
	}
	if ((pixbuf = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"MARKER_PIXMAP_LINEMARKER, NULL)))
	{
		gtk_source_view_set_marker_pixbuf (view, 
			MARKER_LINEMARKER, pixbuf);
		g_object_unref (pixbuf);
	}
}

/* Create tags for highlighting */
static void sourceview_create_highligth_indic(Sourceview* sv)
{	
	sv->priv->important_indic = 
		gtk_text_buffer_create_tag (GTK_TEXT_BUFFER(sv->priv->document),
		IMPORTANT_INDIC,
		"foreground", "#0000FF", NULL);  
	sv->priv->warning_indic = 
		gtk_text_buffer_create_tag (GTK_TEXT_BUFFER(sv->priv->document),
		WARNING_INDIC,
		"foreground", "#00FF00", NULL); 
	sv->priv->critical_indic = 
		gtk_text_buffer_create_tag (GTK_TEXT_BUFFER(sv->priv->document),
		CRITICAL_INDIC,
		"foreground", "#FF0000", "underline", PANGO_UNDERLINE_ERROR, NULL);
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
					 
	/* Create View instance */
	sv->priv->view = ANJUTA_VIEW(anjuta_view_new(sv));
	g_signal_connect_after(G_OBJECT(sv->priv->view), "char_added",
					G_CALLBACK(on_document_char_added), sv);
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
	return gtk_source_view_get_tabs_width (GTK_SOURCE_VIEW (sv->priv->view));
}

static void
ieditor_set_tab_size (IAnjutaEditor *editor, gint tabsize, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	gtk_source_view_set_tabs_width(GTK_SOURCE_VIEW(sv->priv->view), tabsize);
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
	gtk_source_view_set_insert_spaces_instead_of_tabs(GTK_SOURCE_VIEW(sv->priv->view), use_spaces);
}

static void
ieditor_set_auto_indent (IAnjutaEditor *editor, gboolean auto_indent, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	DEBUG_PRINT("Setting auto-indent to %d", auto_indent);
	gtk_source_view_set_auto_indent(GTK_SOURCE_VIEW(sv->priv->view), auto_indent);
}


/* Scroll to line */
static void ieditor_goto_line(IAnjutaEditor *editor, gint line, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);

	if (!sv->priv->loading)
	{
		anjuta_document_goto_line(sv->priv->document, line - 1);
		anjuta_view_scroll_to_cursor(sv->priv->view);
	}
	else
		sv->priv->goto_line = line;
}

/* Scroll to position */
static void ieditor_goto_position(IAnjutaEditor *editor, gint position, GError **e)
{
	GtkTextIter iter;
	
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(sv->priv->document),
									   &iter, position);
	gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (sv->priv->document), &iter);
	gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (sv->priv->view),
								  &iter, 0, FALSE, 0, 0);
}

/* Return a newly allocated pointer containing the whole text */
static gchar* ieditor_get_text(IAnjutaEditor* editor, gint position,
							   gint length, GError **e)
{
	GtkTextIter start_iter;
	GtkTextIter end_iter;
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	
	g_return_val_if_fail (position >= 0, NULL);
	if (length == 0)
		return NULL;

	gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(sv->priv->document),
								   &start_iter, position);
	if (length > 0)
		gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(sv->priv->document),
										   &end_iter, position + length);
	else
		gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(sv->priv->document),
										   &end_iter, -1);
	return gtk_text_buffer_get_slice(GTK_TEXT_BUFFER(sv->priv->document),
									&start_iter, &end_iter, TRUE);
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

static void ieditor_erase(IAnjutaEditor* editor, gint position, gint length, GError **e)
{
	GtkTextIter start, end;
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->document);

	g_return_if_fail (position >= 0);
	if (length == 0)
		return;

	gtk_text_buffer_get_iter_at_offset(buffer, &start, position);

	if (length > 0)
		gtk_text_buffer_get_iter_at_offset(buffer, &end, position + length);
	else
		gtk_text_buffer_get_iter_at_offset(buffer, &end, -1);
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
										   gint position, GError **e)
{
	GtkTextIter iter;
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	gint line;
	gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(sv->priv->document),
									   &iter, position);

	line = gtk_text_iter_get_line(&iter) + 1;
	/* line = line ? line - 1 : 0; */
	
	return line;
}

static gint ieditor_get_line_begin_position(IAnjutaEditor *editor,
											gint line, GError **e)
{
	GtkTextIter iter;
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	
	gtk_text_buffer_get_iter_at_line_offset (GTK_TEXT_BUFFER(sv->priv->document),
											 &iter, line - 1, 0);
	return gtk_text_iter_get_offset (&iter);
}

static gint ieditor_get_line_end_position(IAnjutaEditor *editor,
											gint line, GError **e)
{
	GtkTextIter iter;
	Sourceview* sv = ANJUTA_SOURCEVIEW(editor);
	
	gtk_text_buffer_get_iter_at_line(GTK_TEXT_BUFFER(sv->priv->document),
									 &iter, line - 1);
	/* If iter is not at line end, move it */
	if (!gtk_text_iter_ends_line(&iter))
		gtk_text_iter_forward_to_line_end (&iter);
	return gtk_text_iter_get_offset(&iter);
}

static IAnjutaIterable*
ieditor_get_cell_iter(IAnjutaEditor* edit, gint position, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(edit);
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->document);
	GtkTextIter iter;
	SourceviewCell* cell;
	
	gtk_text_buffer_get_iter_at_offset(buffer, &iter, position);
	cell = sourceview_cell_new(&iter, GTK_TEXT_VIEW(sv->priv->view));
	
	return IANJUTA_ITERABLE(cell);
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
	iface->get_position = ieditor_get_position;
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
	iface->get_cell_iter = ieditor_get_cell_iter;
	iface->get_line_begin_position = ieditor_get_line_begin_position;
	iface->get_line_end_position = ieditor_get_line_end_position;	
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
	if (sv->priv->filename != NULL)
		return sv->priv->filename;
	else
		return anjuta_document_get_short_name_for_display(sv->priv->document);
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
	gtk_text_buffer_move_mark_by_name(GTK_TEXT_BUFFER(sv->priv->document),
									  "insert", start_iter);
	gtk_text_buffer_move_mark_by_name(GTK_TEXT_BUFFER(sv->priv->document),
									  "selection_bound", end_iter);
	gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(sv->priv->view),
								 gtk_text_buffer_get_insert(GTK_TEXT_BUFFER(sv->priv->document)),
								 0,  FALSE, 0, 0);			
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
		return gtk_text_buffer_get_slice(GTK_TEXT_BUFFER(sv->priv->document),
										&start_iter, &end_iter, TRUE);
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
}

/* IAnjutaEditorConvert Interface */

static void
iconvert_to_upper(IAnjutaEditorConvert* edit, gint start_pos, gint end_pos, GError** e)
{
	gchar *buffer;
	
	buffer = iselect_get(IANJUTA_EDITOR_SELECTION(edit), e);
	if (buffer)
	{
		buffer = g_utf8_strup(buffer, g_utf8_strlen(buffer, -1));
		iselect_replace(IANJUTA_EDITOR_SELECTION(edit), buffer, g_utf8_strlen(buffer,  -1), e);	
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
		buffer = g_utf8_strdown(buffer, g_utf8_strlen(buffer, -1));
		iselect_replace(IANJUTA_EDITOR_SELECTION(edit), buffer, g_utf8_strlen(buffer,  -1), e);	
		g_free(buffer);
	}
}

static void
iconvert_iface_init(IAnjutaEditorConvertIface* iface)
{
	iface->to_upper = iconvert_to_upper;
	iface->to_lower = iconvert_to_lower;
}

static gint
imark_mark(IAnjutaMarkable* mark, gint location, IAnjutaMarkableMarker marker,
		   GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(mark);
	GtkTextIter iter;
	GtkSourceMarker* source_marker;
	gchar* name;
	static gint marker_count = 0;
	
	gtk_text_buffer_get_iter_at_line(GTK_TEXT_BUFFER(sv->priv->document),
									 &iter, location  - 1);
	switch (marker)
	{
		case IANJUTA_MARKABLE_LINEMARKER:
			name = MARKER_LINEMARKER;
			break;
		case IANJUTA_MARKABLE_PROGRAM_COUNTER:
			name = MARKER_PROGRAM_COUNTER;
			break;
		case IANJUTA_MARKABLE_BOOKMARK:
			name = MARKER_BOOKMARK;
			break;
		case IANJUTA_MARKABLE_BREAKPOINT_DISABLED:
			name = MARKER_BREAKPOINT_DISABLED;
			break;
		case IANJUTA_MARKABLE_BREAKPOINT_ENABLED:
			name = MARKER_BREAKPOINT_ENABLED;
			break;
		default:
			DEBUG_PRINT("Unknown marker type: %d!", marker);
			name = MARKER_NONE;
	}
		
	source_marker = gtk_source_buffer_create_marker(GTK_SOURCE_BUFFER(sv->priv->document), 
													NULL, name, &iter);

	g_object_set_data(G_OBJECT(source_marker), "handle", GINT_TO_POINTER(marker_count++));
	g_object_set_data(G_OBJECT(source_marker), "type", GINT_TO_POINTER(marker));
	g_object_set_data(G_OBJECT(source_marker), "location", GINT_TO_POINTER(location));
	
	return marker_count;
}

static void
imark_unmark(IAnjutaMarkable* mark, gint location, IAnjutaMarkableMarker marker,
			 GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(mark);
	GtkSourceBuffer* buffer = GTK_SOURCE_BUFFER(sv->priv->document);
	GtkSourceMarker* smarker;
	for (smarker = gtk_source_buffer_get_first_marker(buffer);
		 smarker != NULL; smarker = gtk_source_marker_next(smarker))
	{
		int marker_location = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(smarker), "location"));
		if (marker_location == location)
			gtk_source_buffer_delete_marker(buffer, smarker);
		/* We do not break here because there might be multiple markers at this location */
	}
}

static gboolean
imark_is_marker_set(IAnjutaMarkable* mark, gint location, 
					IAnjutaMarkableMarker marker, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(mark);
	GtkSourceBuffer* buffer = GTK_SOURCE_BUFFER(sv->priv->document);
	GtkSourceMarker* smarker;
	for (smarker = gtk_source_buffer_get_first_marker(buffer);
		 smarker != NULL; smarker = gtk_source_marker_next(smarker))
	{
		int marker_location = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(smarker), "location"));
		if (marker_location == location)
			return TRUE;
	}
	return FALSE;
}

static gint
imark_location_from_handle(IAnjutaMarkable* mark, gint handle, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(mark);
	GtkSourceBuffer* buffer = GTK_SOURCE_BUFFER(sv->priv->document);
	GtkSourceMarker* marker;
	for (marker = gtk_source_buffer_get_first_marker(buffer);
		 marker != NULL; marker = gtk_source_marker_next(marker))
	{
		int marker_handle = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(marker), "handle"));
		if (marker_handle == handle)
			return GPOINTER_TO_INT(g_object_get_data(G_OBJECT(marker), "location"));
	}
	return 0;
}

static void
imark_delete_all_markers(IAnjutaMarkable* mark, IAnjutaMarkableMarker marker,
						 GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(mark);
	GtkSourceBuffer* buffer = GTK_SOURCE_BUFFER(sv->priv->document);
	GtkSourceMarker* smarker;
	for (smarker = gtk_source_buffer_get_first_marker(buffer);
		 smarker != NULL; smarker = gtk_source_marker_next(smarker))
	{
		IAnjutaMarkableMarker type = 
			GPOINTER_TO_INT(g_object_get_data(G_OBJECT(smarker), "type"));
		if (type == marker)
			gtk_source_buffer_delete_marker(buffer, smarker);
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
	gtk_text_buffer_remove_tag_by_name (GTK_TEXT_BUFFER(sv->priv->document),
	                                     WARNING_INDIC,
	                                     &start, &end);
	gtk_text_buffer_remove_tag_by_name (GTK_TEXT_BUFFER(sv->priv->document),
	                                     CRITICAL_INDIC,
	                                     &start, &end);
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

typedef struct
{
	GtkSourceMarker* marker;
	gint line;
} SVBookmark;

static int bookmark_compare(SVBookmark* bmark1, SVBookmark* bmark2)
{
	if (bmark1->line < bmark2->line)
		return -1;
	if (bmark1->line > bmark2->line)
		return 1;
	else
		return 0;
}

static SVBookmark*
ibookmark_is_bookmark_set(IAnjutaBookmark* bmark, gint location, GError **e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(bmark);
	GList* node = sv->priv->bookmarks;
	while (node)
	{
		SVBookmark* bookmark = node->data;
		if (bookmark->line == location)
			return bookmark;
		if (bookmark->line > location)
			return NULL;
		node = g_list_next(node);
	}
	return NULL;
}

static void
ibookmark_toggle(IAnjutaBookmark* bmark, gint location, gboolean ensure_visible, GError** e)
{
	SVBookmark* bookmark;
	Sourceview* sv = ANJUTA_SOURCEVIEW(bmark);

	if ((bookmark = ibookmark_is_bookmark_set(bmark, location, e)) != NULL)
	{
		gtk_source_buffer_delete_marker(GTK_SOURCE_BUFFER(sv->priv->document),
		                                bookmark->marker);
		sv->priv->bookmarks = g_list_remove(sv->priv->bookmarks, bookmark);
		g_free(bookmark);
	}
	else
	{
		bookmark = g_new0(SVBookmark, 1);
		GtkTextIter iter;
		bookmark->line = location;
		gtk_text_buffer_get_iter_at_line(GTK_TEXT_BUFFER(sv->priv->document), &iter, bookmark->line - 1);
		bookmark->marker = gtk_source_buffer_create_marker(GTK_SOURCE_BUFFER(sv->priv->document), 
		                                                   NULL, MARKER_BOOKMARK, &iter);
		sv->priv->bookmarks = g_list_append(sv->priv->bookmarks, bookmark);
		sv->priv->cur_bmark = sv->priv->bookmarks;
		sv->priv->bookmarks = g_list_sort(sv->priv->bookmarks, (GCompareFunc) bookmark_compare);
	}
}

static void
ibookmark_first(IAnjutaBookmark* bmark, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(bmark);
	
	GList* bookmark;
	SVBookmark* mark;
	bookmark = g_list_first(sv->priv->bookmarks);
	if (bookmark)
	{
		mark = bookmark->data;
		ianjuta_editor_goto_line(IANJUTA_EDITOR(bmark), 
			mark->line, NULL);
		sv->priv->cur_bmark = bookmark;
	}
}

static void
ibookmark_last(IAnjutaBookmark* bmark, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(bmark);
	GList* bookmark;
	SVBookmark* mark;
	bookmark = g_list_last(sv->priv->bookmarks);
	if (bookmark)
	{
		mark = bookmark->data;
		ianjuta_editor_goto_line(IANJUTA_EDITOR(bmark), 
			mark->line, NULL);
		sv->priv->cur_bmark = bookmark;
	}
}

static void
ibookmark_next(IAnjutaBookmark* bmark, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(bmark);
	GList* node = sv->priv->bookmarks;
	SVBookmark* bookmark;
	gint location;

	location = ieditor_get_lineno(IANJUTA_EDITOR(bmark), NULL);	
	while (node)
	{
		bookmark = node->data;
		if (bookmark->line > location)
			break;
		node = g_list_next(node);
	}
	if (node)
	{
		ianjuta_editor_goto_line(IANJUTA_EDITOR(bmark), 
		                         bookmark->line, NULL);
		sv->priv->cur_bmark = node;
	}
}

static void
ibookmark_previous(IAnjutaBookmark* bmark, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(bmark);
	GList* node = sv->priv->bookmarks;
	SVBookmark* bookmark;
	gint location;

	location = ieditor_get_lineno(IANJUTA_EDITOR(bmark), NULL);	
	if (node)
	{
		node = g_list_last(node);
		while (node)
		{
			bookmark = node->data;
			if (bookmark->line < location)
				break;
			node = g_list_previous(node);
		}
		if (node)
		{
			ianjuta_editor_goto_line(IANJUTA_EDITOR(bmark), 
		                             bookmark->line, NULL);
			sv->priv->cur_bmark = node;
		}
	}
}

static void
ibookmark_clear_all(IAnjutaBookmark* bmark, GError** e)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(bmark);
	GList* node = sv->priv->bookmarks;
	while (node)
	{
		SVBookmark* mark = node->data;
		gtk_source_buffer_delete_marker(GTK_SOURCE_BUFFER(sv->priv->document),
											mark->marker);
		g_free(mark);
		node = g_list_next(node);
	}
	g_list_free(sv->priv->bookmarks);
	sv->priv->bookmarks = NULL;
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
	GtkSourceLanguagesManager* manager = gtk_source_languages_manager_new();
	const GSList* manager_languages = gtk_source_languages_manager_get_available_languages(manager);
	GList* languages = NULL;
	
	while (manager_languages)
	{
		languages = g_list_append(languages, gtk_source_language_get_name(
			GTK_SOURCE_LANGUAGE(manager_languages->data)));
		manager_languages = g_slist_next(manager_languages);
	}
	return languages;
}

static const gchar*
ilanguage_get_language_name (IAnjutaEditorLanguage *ilanguage,
							 const gchar *language, GError **err)
{	
	return language;
}

static void
ilanguage_set_language (IAnjutaEditorLanguage *ilanguage,
						const gchar *language, GError **err)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(ilanguage);
	GtkSourceBuffer* buffer = GTK_SOURCE_BUFFER(sv->priv->document);
	GtkSourceLanguagesManager* manager = gtk_source_languages_manager_new();
	const GSList* langs = gtk_source_languages_manager_get_available_languages(manager);
	
	while (langs)
	{
		gchar* name = gtk_source_language_get_name(GTK_SOURCE_LANGUAGE(langs->data));
		if (g_str_equal(name, language))
		{
			gtk_source_buffer_set_language(buffer, GTK_SOURCE_LANGUAGE(langs->data));
			g_signal_emit_by_name (ilanguage, "language-changed", language);
			return;
		}
		langs = g_slist_next(langs);
	}
	/* Language not found -> use autodetection */
	{
		gchar* mime_type = anjuta_document_get_mime_type(ANJUTA_DOCUMENT(buffer));
		GtkSourceLanguage* lang = gtk_source_languages_manager_get_language_from_mime_type(
			manager, mime_type);
		if (lang != NULL)
		{
			gtk_source_buffer_set_language(buffer, lang);
			g_signal_emit_by_name (ilanguage, "language-changed", 
				gtk_source_language_get_name(lang));
		}
	}	
}

static const gchar*
ilanguage_get_language (IAnjutaEditorLanguage *ilanguage, GError **err)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(ilanguage);
	GtkSourceBuffer* buffer = GTK_SOURCE_BUFFER(sv->priv->document);
	GtkSourceLanguage* lang = gtk_source_buffer_get_language(buffer);
	if (lang)
		return gtk_source_language_get_name(lang);
	else
		return NULL;
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
iassist_add_trigger (IAnjutaEditorAssist* iassist, const gchar* trigger,
					 IAnjutaEditorAssistContextParser context_parser, GError **err)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(iassist);
	if (!sv->priv->triggers)
	{
		sv->priv->triggers = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	}
	g_hash_table_insert(sv->priv->triggers, g_strdup(trigger), context_parser);
}

static void
iassist_remove_trigger (IAnjutaEditorAssist* iassist, const gchar* trigger, GError **err)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(iassist);
	if (sv->priv->triggers)
	{
		g_hash_table_remove(sv->priv->triggers, trigger);
	}
}

static GList*
iassist_get_suggestions (IAnjutaEditorAssist *iassist, const gchar *context, GError **err)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(iassist);
	/* We don't make own suggestions yet */
	return NULL;
}

static void
iassist_init_suggestions (IAnjutaEditorAssist* iassist, gint position, GError **err)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(iassist);
	
	sv->priv->assist_win = assist_window_new(GTK_TEXT_VIEW(sv->priv->view), NULL,
											 position);
	g_signal_connect(G_OBJECT(sv->priv->assist_win), "destroy", 
						 G_CALLBACK(on_assist_window_destroyed), sv);
	g_signal_connect(G_OBJECT(sv->priv->assist_win), "chosen", 
						 G_CALLBACK(on_assist_chosen), sv);
	g_signal_connect(G_OBJECT(sv->priv->assist_win), "cancel", 
						 G_CALLBACK(on_assist_cancel), sv);
	sv->priv->assist_active = TRUE;
}

static void
iassist_suggest (IAnjutaEditorAssist *iassist, GList* choices, int char_alignment, GError **err)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(iassist);
	
	if (choices == NULL)
	{
		if (sv->priv->assist_win)
			gtk_widget_destroy(GTK_WIDGET(sv->priv->assist_win));
	}
	else
	{
		assist_window_update(sv->priv->assist_win, choices);
		gtk_widget_show(GTK_WIDGET(sv->priv->assist_win));
		if (char_alignment != -1)
			assist_window_move(sv->priv->assist_win, char_alignment);
	}
}

static void
iassist_hide_suggestions (IAnjutaEditorAssist* iassist, GError** err)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(iassist);
	gtk_widget_hide(GTK_WIDGET (sv->priv->assist_win));
}

static void 
iassist_show_tips (IAnjutaEditorAssist *iassist, GList* tips,  gint char_alignment, GError **err)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(iassist);
	
	if (sv->priv->tip_active)
		return;
	sv->priv->assist_tip = 
		ASSIST_TIP (assist_tip_new (GTK_TEXT_VIEW (sv->priv->view), tips));
	
	g_signal_connect (G_OBJECT(sv->priv->assist_tip), "destroy", G_CALLBACK(on_assist_tip_destroyed),
					  sv);
	
	sv->priv->tip_active = TRUE;
	gtk_widget_show (GTK_WIDGET (sv->priv->assist_tip));
}

static void
iassist_cancel_tips (IAnjutaEditorAssist* iassist, GError** err)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(iassist);
	if (sv->priv->assist_tip)
		gtk_widget_destroy (GTK_WIDGET (sv->priv->assist_tip));
}

static void
iassist_react (IAnjutaEditorAssist *iassist, gint selection,  
			   const gchar *completion, GError **err)
{
	Sourceview* sv = ANJUTA_SOURCEVIEW(iassist);
	GtkTextIter begin, end;

	if (!completion)
		return;
	
	gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(sv->priv->document),
															   &begin, assist_window_get_position(sv->priv->assist_win));
	gtk_text_buffer_get_iter_at_mark(GTK_TEXT_BUFFER(sv->priv->document), &end, 
										   gtk_text_buffer_get_insert(GTK_TEXT_BUFFER(sv->priv->document)));
	g_signal_handlers_block_by_func(G_OBJECT(sv->priv->document), 
									on_document_char_added, sv);
	gtk_text_buffer_delete(GTK_TEXT_BUFFER(sv->priv->document), &begin, &end);
	gtk_text_buffer_insert_at_cursor(GTK_TEXT_BUFFER(sv->priv->document),
									 completion, strlen(completion));
	g_signal_handlers_unblock_by_func(G_OBJECT(sv->priv->document), 
									on_document_char_added, sv);
	
	gtk_widget_destroy (GTK_WIDGET(sv->priv->assist_win));
}

static void
iassist_iface_init(IAnjutaEditorAssistIface* iface)
{
	iface->add_trigger = iassist_add_trigger;
	iface->remove_trigger = iassist_remove_trigger;
	iface->init_suggestions = iassist_init_suggestions;
	iface->suggest = iassist_suggest;
	iface->hide_suggestions = iassist_hide_suggestions;
	iface->get_suggestions = iassist_get_suggestions;
	iface->show_tips = iassist_show_tips;
	iface->cancel_tips = iassist_cancel_tips;
	iface->react = iassist_react;
}

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
ANJUTA_TYPE_END;
