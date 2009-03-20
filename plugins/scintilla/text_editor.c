/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * text_editor.c
 * Copyright (C) 2000 - 2004  Naba Kumar
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <gio/gio.h>
#include <gnome.h>
#include <errno.h>

#include <libanjuta/resources.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-encodings.h>
#include <libanjuta/anjuta-convert.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-message-area.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>
#include <libanjuta/interfaces/ianjuta-editor-convert.h>
#include <libanjuta/interfaces/ianjuta-editor-line-mode.h>
#include <libanjuta/interfaces/ianjuta-editor-view.h>
#include <libanjuta/interfaces/ianjuta-editor-folds.h>
#include <libanjuta/interfaces/ianjuta-editor-comment.h>
#include <libanjuta/interfaces/ianjuta-editor-zoom.h>
#include <libanjuta/interfaces/ianjuta-editor-goto.h>
#include <libanjuta/interfaces/ianjuta-editor-language.h>
#include <libanjuta/interfaces/ianjuta-editor-assist.h>
#include <libanjuta/interfaces/ianjuta-editor-search.h>
#include <libanjuta/interfaces/ianjuta-editor-hover.h>
#include <libanjuta/interfaces/ianjuta-editor-factory.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-savable.h>
#include <libanjuta/interfaces/ianjuta-markable.h>
#include <libanjuta/interfaces/ianjuta-indicable.h>
#include <libanjuta/interfaces/ianjuta-print.h>
#include <libanjuta/interfaces/ianjuta-document.h>

#include "properties.h"
#include "text_editor.h"
#include "text_editor_cbs.h"
#include "text-editor-iterable.h"
#include "print.h"

#define GTK
#undef PLAT_GTK
#define PLAT_GTK 1
#include "Scintilla.h"
#include "SciLexer.h"
#include "ScintillaWidget.h"

#include "lexer.h"
#include "aneditor.h"
#include "text_editor_prefs.h"

/* Order is important, as marker with the lowest number is drawn first */
#define TEXT_EDITOR_BOOKMARK                0
#define TEXT_EDITOR_BREAKPOINT_DISABLED     1
#define TEXT_EDITOR_BREAKPOINT_ENABLED      2
#define TEXT_EDITOR_PROGRAM_COUNTER         3
#define TEXT_EDITOR_LINEMARKER              4

/* Include marker pixmaps */
#include "anjuta-bookmark-16.xpm"
#include "anjuta-breakpoint-disabled-16.xpm"
#include "anjuta-breakpoint-enabled-16.xpm"
#include "anjuta-pcmark-16.xpm"
#include "anjuta-linemark-16.xpm"

static gchar** marker_pixmap[] = 
{
	anjuta_bookmark_16_xpm,
	anjuta_breakpoint_disabled_16_xpm,
	anjuta_breakpoint_enabled_16_xpm,
	anjuta_pcmark_16_xpm,
	anjuta_linemark_16_xpm,
	NULL
};

/* Editor language supports */
static GList *supported_languages = NULL;
static GHashTable *supported_languages_name = NULL;
static GHashTable *supported_languages_ext = NULL;
static GHashTable *supported_languages_by_lexer = NULL;

static void text_editor_finalize (GObject *obj);
static void text_editor_dispose (GObject *obj);
static void text_editor_hilite_one (TextEditor * te, AnEditorID editor,
									gboolean force);

static GtkVBoxClass *parent_class;

static void
text_editor_instance_init (TextEditor *te)
{
	te->filename = NULL;
	te->uri = NULL;
	te->views = NULL;
	te->popup_menu = NULL;
	
	te->monitor = NULL;
	te->preferences = NULL;
	te->force_hilite = NULL;
	te->freeze_count = 0;
	te->current_line = 0;
	te->popup_menu = NULL;
	te->props_base = 0;
	te->first_time_expose = TRUE;
	te->encoding = NULL;
	te->notify_ids = NULL;
	te->hover_tip_on = FALSE;
	te->last_saved_content = NULL;
	te->force_not_saved = FALSE;
	te->message_area = NULL;
}

static void
text_editor_class_init (TextEditorClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	
	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = text_editor_finalize;
	object_class->dispose = text_editor_dispose;
}

#if 0
static void
check_tm_file(TextEditor *te)
{
	if (NULL == te->tm_file)
	{
		// FIXME:
		// te->tm_file = tm_workspace_find_object(
		  //TM_WORK_OBJECT(app->tm_workspace), te->uri, FALSE);
		if (NULL == te->tm_file)
		{
			te->tm_file = tm_source_file_new(te->uri, TRUE);
			if (NULL != te->tm_file)
				tm_workspace_add_object(te->tm_file);
		}
	}
}
#endif

static void
initialize_markers (TextEditor* te, GtkWidget *scintilla)
{
	gint marker;
	gchar ***xpm;
	g_return_if_fail (te != NULL);
	
	marker = 0;
	for (xpm = marker_pixmap;*xpm != NULL; xpm++)
	{
		scintilla_send_message (SCINTILLA (scintilla), SCI_MARKERDEFINEPIXMAP,
								marker, (sptr_t)*xpm);
		marker++;
	}
}

#ifdef DEBUG
static void
on_scintila_already_destroyed (gpointer te, GObject *obj)
{
	/* DEBUG_PRINT ("%s", "Scintilla object has been destroyed"); */
}

static void
on_te_already_destroyed (gpointer te, GObject *obj)
{
	/* DEBUG_PRINT ("%s", "TextEditor object has been destroyed"); */
}
#endif

void
text_editor_add_view (TextEditor *te)
{
	AnEditorID editor_id;
	GtkWidget *scintilla;
	gint current_line;
	gint current_point;
	
	if (te->views)
	{
		current_line = text_editor_get_current_lineno (te);
		current_point = text_editor_get_current_position (te);
	}
	else
	{
		current_line = 0;
		current_point = 0;
	}
	editor_id = aneditor_new (sci_prop_get_pointer (te->props_base));
	scintilla = aneditor_get_widget (editor_id);
	
	/* Set notifications to receive */
	scintilla_send_message (SCINTILLA (scintilla), SCI_SETMODEVENTMASK,
							(SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT), 0);
	
	/* Set parent, if it is not primary view */
	if (te->views)
	{
		aneditor_set_parent (editor_id, GPOINTER_TO_INT(te->editor_id));
	}
	te->views = g_list_prepend (te->views, GINT_TO_POINTER (editor_id));
	te->editor_id = editor_id;
	te->scintilla = scintilla;
	
	/*
	aneditor_command (te->editor_id, ANE_SETACCELGROUP,
			  (glong) app->accel_group, 0);
	*/
	
	gtk_widget_set_size_request (scintilla, 50, 50);
	gtk_widget_show (scintilla);
	
	gtk_box_set_spacing (GTK_BOX (te->vbox), 3);
	gtk_box_pack_start (GTK_BOX (te->vbox), scintilla, TRUE, TRUE, 0);
	gtk_widget_grab_focus (scintilla);

	g_signal_connect (G_OBJECT (scintilla), "event",
			    G_CALLBACK (on_text_editor_text_event), te);
	g_signal_connect (G_OBJECT (scintilla), "button_press_event",
			    G_CALLBACK (on_text_editor_text_buttonpress_event), te);
	g_signal_connect (G_OBJECT (scintilla), "key_release_event",
			    G_CALLBACK (on_text_editor_text_keyrelease_event), te);
	g_signal_connect_after (G_OBJECT (scintilla), "size_allocate",
			    G_CALLBACK (on_text_editor_scintilla_size_allocate), te);
	g_signal_connect (G_OBJECT (scintilla), "sci-notify",
			    G_CALLBACK (on_text_editor_scintilla_notify), te);
	g_signal_connect (G_OBJECT (scintilla), "focus_in_event",
				G_CALLBACK (on_text_editor_scintilla_focus_in), te);
	
	initialize_markers (te, scintilla);
	text_editor_hilite_one (te, editor_id, FALSE);
	text_editor_set_line_number_width (te);
	
	if (current_line)
		text_editor_goto_line (te, current_line, FALSE, TRUE);
	if (current_point)
		text_editor_goto_point (te, current_point);
	
#ifdef DEBUG
	g_object_weak_ref (G_OBJECT (scintilla), on_scintila_already_destroyed, te);
#endif
}

/* Remove the current view */
void
text_editor_remove_view (TextEditor *te)
{
	if (!te->editor_id)
		return;
	if (te->views == NULL ||
		g_list_length (te->views) <= 1)
		return;
	
	g_signal_handlers_disconnect_by_func (G_OBJECT (te->scintilla),
				G_CALLBACK (on_text_editor_text_event), te);
	g_signal_handlers_disconnect_by_func (G_OBJECT (te->scintilla),
				G_CALLBACK (on_text_editor_text_buttonpress_event), te);
	g_signal_handlers_disconnect_by_func (G_OBJECT (te->scintilla),
				G_CALLBACK (on_text_editor_text_keyrelease_event), te);
	g_signal_handlers_disconnect_by_func (G_OBJECT (te->scintilla),
				G_CALLBACK (on_text_editor_scintilla_size_allocate), te);
	g_signal_handlers_disconnect_by_func (G_OBJECT (te->scintilla),
				G_CALLBACK (on_text_editor_scintilla_notify), te);
	g_signal_handlers_disconnect_by_func (G_OBJECT (te->scintilla),
				G_CALLBACK (on_text_editor_scintilla_focus_in), te);
	
	te->views = g_list_remove (te->views, GINT_TO_POINTER(te->editor_id));
	gtk_container_remove (GTK_CONTAINER (te->vbox), te->scintilla);
	aneditor_destroy(te->editor_id);
	
	/* Set current view */
	if (te->views)
	{
		te->editor_id = GPOINTER_TO_INT(te->views->data);
		te->scintilla = aneditor_get_widget (te->editor_id);
		gtk_widget_grab_focus (te->scintilla);
	}
	else
	{
		gtk_box_set_spacing (GTK_BOX (te->vbox), 0);
		te->editor_id = 0;
		te->scintilla = NULL;
	}
}

static void
on_reload_dialog_response (GtkWidget *message_area, gint res, TextEditor *te)
{
	if (res == GTK_RESPONSE_YES)
	{
		text_editor_load_file (te);
	}
	else
	{
		text_editor_set_saved (te, FALSE);
		gtk_widget_destroy (message_area);
	}
}

static void
on_destroy_message_area (gpointer data, GObject *finalized_object)
{
	TextEditor *te = (TextEditor *)data;
	
	te->message_area = NULL;
	g_signal_emit_by_name (G_OBJECT (te), "update-save-ui");
}

static void
on_close_dialog_response (GtkWidget *message_area, gint res, TextEditor *te)
{
	if (res == GTK_RESPONSE_YES)
	{
		IAnjutaDocumentManager *docman;
		
		docman = anjuta_shell_get_interface (te->shell, IAnjutaDocumentManager, NULL);
		if (docman == NULL) return;

		ianjuta_document_manager_remove_document (docman, IANJUTA_DOCUMENT (te), FALSE, NULL);
	}
	else
	{
		text_editor_set_saved (te, FALSE);
		gtk_widget_destroy (message_area);
	}
}

static void
text_editor_set_message_area (TextEditor *te,  GtkWidget *message_area)
{
	if (te->message_area != NULL)
		gtk_widget_destroy (te->message_area);
	te->message_area = message_area;

	if (te->message_area == NULL)
		return;
		
	gtk_widget_show (message_area);
	gtk_box_pack_start (GTK_BOX (te),
						message_area,
						FALSE,
						FALSE,
						0);
	g_object_weak_ref (G_OBJECT (te->message_area),
					   on_destroy_message_area, te);
	
	g_signal_emit_by_name (G_OBJECT (te), "update-save-ui");
}

static void
on_text_editor_uri_changed (GFileMonitor *monitor,
							GFile *file,
							GFile *other_file,
							GFileMonitorEvent event_type,
							gpointer user_data)
{
	TextEditor *te = TEXT_EDITOR (user_data);
	GtkWidget *message_area;
	gchar *buff;
	
	/* DEBUG_PRINT ("%s", "File changed!!!"); */
		
	switch (event_type)
	{
		case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
			
			if (!anjuta_util_diff (te->uri, te->last_saved_content))
			{
				/* The file content is same. Remove any previous prompt for reload */
				if (te->message_area)
					gtk_widget_destroy (te->message_area);
				te->message_area = NULL;
				return;
			}
			/* No break here */

		case G_FILE_MONITOR_EVENT_CREATED:
			if (text_editor_is_saved (te))
			{
				buff = g_strdup_printf (_("The file '%s' has been changed.\n"
								  "Do you want to reload it ?"),
								 te->filename);
			}
			else
			{
				buff = g_strdup_printf (_("The file '%s' has been changed.\n"
								  "Do you want to loose your changes and reload it ?"),
								 te->filename);
			}
			message_area = anjuta_message_area_new (buff, GTK_STOCK_DIALOG_WARNING);
			g_free (buff);
			anjuta_message_area_add_button (ANJUTA_MESSAGE_AREA (message_area),
											GTK_STOCK_REFRESH,
											GTK_RESPONSE_YES);
			anjuta_message_area_add_button (ANJUTA_MESSAGE_AREA (message_area),
											GTK_STOCK_CANCEL,
											GTK_RESPONSE_NO);
			g_signal_connect (G_OBJECT(message_area), "response",
							  G_CALLBACK (on_reload_dialog_response),
							  te);
			break;
		case G_FILE_MONITOR_EVENT_DELETED:
			if (text_editor_is_saved (te))
			{
				buff = g_strdup_printf (_
							 ("The file '%s' has been deleted.\n"
							  "Do you confirm and close it ?"),
							 te->filename);
			}
			else
			{
				buff = g_strdup_printf (_
							 ("The file '%s' has been deleted.\n"
							  "Do you want to loose your changes and close it ?"),
							 te->filename);
			}
			message_area = anjuta_message_area_new (buff, GTK_STOCK_DIALOG_WARNING);
			g_free (buff);
			anjuta_message_area_add_button (ANJUTA_MESSAGE_AREA (message_area),
											GTK_STOCK_DELETE,
											GTK_RESPONSE_YES);
			anjuta_message_area_add_button (ANJUTA_MESSAGE_AREA (message_area),
											GTK_STOCK_CANCEL,
											GTK_RESPONSE_NO);
			g_signal_connect (G_OBJECT(message_area), "response",
							  G_CALLBACK (on_close_dialog_response),
							  te);
			break;
		case G_FILE_MONITOR_EVENT_CHANGED:
		case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED:
		case G_FILE_MONITOR_EVENT_PRE_UNMOUNT:
		case G_FILE_MONITOR_EVENT_UNMOUNTED:
			return;
		default:
			g_warn_if_reached ();
			return;
	}
	
	text_editor_set_message_area (te, message_area);
}

static void
text_editor_update_monitor (TextEditor *te, gboolean disable_it)
{
	if (te->monitor)
	{
		/* Shutdown existing monitor */
		g_file_monitor_cancel (te->monitor);
		te->monitor = NULL;
	}
	if (te->message_area) 
	{
		/* Remove existing message area */
		gtk_widget_destroy (te->message_area);
		te->message_area = NULL;
	}
	if (te->uri && !disable_it)
	{
		GFile *gio_uri;
		GError *error = NULL;
		/* DEBUG_PRINT ("%s", "Setting up Monitor for %s", te->uri); */

		gio_uri = g_file_new_for_uri (te->uri);
		te->monitor = g_file_monitor_file (gio_uri, 
										G_FILE_MONITOR_NONE, 
										NULL, 
										&error);
		g_signal_connect (te->monitor, "changed",
				  G_CALLBACK (on_text_editor_uri_changed), te);
		g_object_unref (gio_uri);
		
		if (error != NULL)
		{
			DEBUG_PRINT ("Error while setting up file monitor: %s",
					   error->message);
			g_error_free (error);
		}
		
	}
}

GtkWidget *
text_editor_new (AnjutaStatus *status, AnjutaPreferences *eo, AnjutaShell *shell, const gchar *uri, const gchar *name)
{
	gint zoom_factor;
	static guint new_file_count;
	TextEditor *te = TEXT_EDITOR (gtk_widget_new (TYPE_TEXT_EDITOR, NULL));
	
	te->status = status; 
	te->shell = shell;
	
	te->preferences = eo;
	te->props_base = text_editor_get_props();
	if (name && strlen(name) > 0)
		te->filename = g_strdup(name); 
	else 
		te->filename = g_strdup_printf ("Newfile#%d", ++new_file_count);
	if (uri && strlen(uri) > 0)
	{	
		new_file_count--;
		g_free (te->filename);
		g_free (te->uri);

		GFile *gio_uri;
		gio_uri = g_file_new_for_uri (uri);
		te->filename = g_file_get_basename (gio_uri);
		g_object_unref (gio_uri);

		te->uri = g_strdup (uri);
	}
	
	text_editor_prefs_init (te);
	
	/* Create primary view */
	te->vbox = gtk_vbox_new (TRUE, 3);
	gtk_box_pack_end (GTK_BOX (te), te->vbox, TRUE, TRUE, 0);
	text_editor_add_view (te);

	if (te->uri)
	{	
		if (text_editor_load_file (te) == FALSE)
		{
			/* Unable to load file */
			gtk_widget_destroy (GTK_WIDGET (te));
			return NULL;
		}
	}
	text_editor_update_controls (te);
	
	/* Apply font zoom separately */
	zoom_factor = anjuta_preferences_get_int (te->preferences, TEXT_ZOOM_FACTOR);
	/* DEBUG_PRINT ("%s", "Initializing zoom factor to: %d", zoom_factor); */
	text_editor_set_zoom_factor (te, zoom_factor);
	
#ifdef DEBUG
	g_object_weak_ref (G_OBJECT (te), on_te_already_destroyed, te);
#endif
	return GTK_WIDGET (te);
}

void
text_editor_dispose (GObject *obj)
{
	TextEditor *te = TEXT_EDITOR (obj);
	if (te->monitor)
	{
		text_editor_update_monitor (te, TRUE);
		te->monitor = NULL;
	}
	if (te->popup_menu)
	{
		g_object_unref (te->popup_menu);
		te->popup_menu = NULL;
	}
	if (te->views)
	{
		GtkWidget *scintilla;
		AnEditorID editor_id;
		GList *node;
		
		node = te->views;
		while (node)
		{
			editor_id = GPOINTER_TO_INT (node->data);
			scintilla = aneditor_get_widget (editor_id);
			
			g_signal_handlers_disconnect_by_func (G_OBJECT (scintilla),
						G_CALLBACK (on_text_editor_text_event), te);
			g_signal_handlers_disconnect_by_func (G_OBJECT (scintilla),
						G_CALLBACK (on_text_editor_text_buttonpress_event), te);
			g_signal_handlers_disconnect_by_func (G_OBJECT (scintilla),
						G_CALLBACK (on_text_editor_text_keyrelease_event), te);
			g_signal_handlers_disconnect_by_func (G_OBJECT (scintilla),
						G_CALLBACK (on_text_editor_scintilla_size_allocate), te);
			g_signal_handlers_disconnect_by_func (G_OBJECT (scintilla),
						G_CALLBACK (on_text_editor_scintilla_notify), te);
			g_signal_handlers_disconnect_by_func (G_OBJECT (scintilla),
						G_CALLBACK (on_text_editor_scintilla_focus_in), te);
			
			aneditor_destroy (editor_id);
			node = g_list_next (node);
		}			
		te->scintilla = NULL;
		te->editor_id = 0;
		te->views = NULL;
	}
	if (te->notify_ids)
	{
		text_editor_prefs_finalize (te);
		te->notify_ids = NULL;
	}
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

void
text_editor_finalize (GObject *obj)
{
	TextEditor *te = TEXT_EDITOR (obj);
	g_free (te->filename);
	g_free (te->uri);
	g_free (te->force_hilite);
	g_free (te->last_saved_content);
	
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

void
text_editor_freeze (TextEditor *te)
{
	te->freeze_count++;
}

void
text_editor_thaw (TextEditor *te)
{
	te->freeze_count--;
	if (te->freeze_count < 0)
		te->freeze_count = 0;
}

void
text_editor_set_hilite_type (TextEditor * te, const gchar *file_extension)
{
	const gchar *past_language;
	const gchar *curr_language;
	
	past_language = ianjuta_editor_language_get_language (IANJUTA_EDITOR_LANGUAGE (te), NULL);
	
	g_free (te->force_hilite);
	if (file_extension)
		te->force_hilite = g_strdup (file_extension);
	else
		te->force_hilite = NULL;
	
	curr_language = ianjuta_editor_language_get_language (IANJUTA_EDITOR_LANGUAGE (te), NULL);
	if (past_language != curr_language)
		g_signal_emit_by_name (te, "language-changed", curr_language);
}

static void
text_editor_hilite_one (TextEditor * te, AnEditorID editor_id,
						gboolean override_by_pref)
{
	/* If syntax highlighting is disabled ... */
	if (override_by_pref &&
		anjuta_preferences_get_bool (ANJUTA_PREFERENCES (te->preferences),
									DISABLE_SYNTAX_HILIGHTING))
	{
		aneditor_command (editor_id, ANE_SETHILITE, (glong) "plain.txt", 0);
	}
	else if (te->force_hilite)
	{
		aneditor_command (editor_id, ANE_SETHILITE, (glong) te->force_hilite, 0);
	}
	else if (te->uri)
	{
		gchar *basename;
		basename = g_path_get_basename (te->uri);
		aneditor_command (editor_id, ANE_SETHILITE, (glong) basename, 0);
		g_free (basename);
	}
	else if (te->filename)
	{
		aneditor_command (editor_id, ANE_SETHILITE, (glong) te->filename, 0);
	}
	else
	{
		aneditor_command (editor_id, ANE_SETHILITE, (glong) "plain.txt", 0);
	} 
}

void
text_editor_hilite (TextEditor * te, gboolean override_by_pref)
{
	GList *node;
	
	node = te->views;
	while (node)
	{
		text_editor_hilite_one (te, GPOINTER_TO_INT (node->data),
								override_by_pref);
		node = g_list_next (node);
	}
}

void
text_editor_set_zoom_factor (TextEditor * te, gint zfac)
{
	text_editor_command (te, ANE_SETZOOM, zfac,  0);
}

TextEditorAttrib
text_editor_get_attribute (TextEditor *te, gint position)
{
	int lexer;
	int style;
	TextEditorAttrib attrib = TEXT_EDITOR_ATTRIB_TEXT;
	
	lexer = scintilla_send_message (SCINTILLA (te->scintilla), SCI_GETLEXER,
									0, 0);
	style = scintilla_send_message (SCINTILLA (te->scintilla), SCI_GETSTYLEAT,
									position, 0);
	switch (lexer)
	{
		case SCLEX_CPP:
			switch (style)
			{
				case SCE_C_CHARACTER:
				case SCE_C_STRING:
					attrib = TEXT_EDITOR_ATTRIB_STRING;
					break;
				case SCE_C_COMMENT:
				case SCE_C_COMMENTLINE:
				case SCE_C_COMMENTDOC:
				case SCE_C_COMMENTLINEDOC:
				case SCE_C_COMMENTDOCKEYWORD:
				case SCE_C_COMMENTDOCKEYWORDERROR:
					attrib = TEXT_EDITOR_ATTRIB_COMMENT;
					break;
				case SCE_C_WORD:
					attrib = TEXT_EDITOR_ATTRIB_KEYWORD;
					break;
			}
			break;
	}
	return attrib;
}

glong
text_editor_find (TextEditor * te, const gchar * str, gint scope,
				  gboolean forward, gboolean regexp,
				  gboolean ignore_case, gboolean whole_word, gboolean wrap)
{
	glong ret;
	GtkWidget *editor;
	glong flags;
	int current_pos, current_anchor;
	
	if (!te) return -1;
	editor = te->scintilla;
	
	flags = (ignore_case ? 0 : SCFIND_MATCHCASE)
		| (regexp ? SCFIND_REGEXP : 0)
		| (whole_word ? SCFIND_WHOLEWORD : 0)
		| (forward ? 0 : ANEFIND_REVERSE_FLAG);
		
	switch (scope)
	{
		case TEXT_EDITOR_FIND_SCOPE_WHOLE:
				if (forward)
				{
					scintilla_send_message (SCINTILLA (editor), SCI_SETANCHOR,
											0, 0);
					scintilla_send_message (SCINTILLA (editor),
											SCI_SETCURRENTPOS, 0, 0);
				}
				else
				{
					glong length;
					length = scintilla_send_message (SCINTILLA (editor),
													 SCI_GETTEXTLENGTH, 0, 0);
					scintilla_send_message (SCINTILLA (editor),
											SCI_SETCURRENTPOS, length-1, 0);
					scintilla_send_message (SCINTILLA (editor),
											SCI_SETANCHOR, length-1, 0);
				}
			break;
		default:
			break;
	}
	current_pos = scintilla_send_message (SCINTILLA (editor),
										  SCI_GETCURRENTPOS, 0, 0);
	current_anchor = scintilla_send_message (SCINTILLA (editor),
											 SCI_GETANCHOR, 0, 0);
	ret = aneditor_command (te->editor_id, ANE_FIND, flags, (long)str);
	if (scope == TEXT_EDITOR_FIND_SCOPE_CURRENT && wrap && ret < 0) {
		/* If wrap is requested, wrap it. */
		if (forward)
		{
			scintilla_send_message (SCINTILLA (editor), SCI_SETANCHOR, 0, 0);
			scintilla_send_message (SCINTILLA (editor), SCI_SETCURRENTPOS,
									0, 0);
		}
		else
		{
			glong length;
			length = scintilla_send_message (SCINTILLA (editor),
											 SCI_GETTEXTLENGTH, 0, 0);
			scintilla_send_message (SCINTILLA (editor), SCI_SETCURRENTPOS,
									length-1, 0);
			scintilla_send_message (SCINTILLA (editor), SCI_SETANCHOR,
									length-1, 0);
		}
		ret = aneditor_command (te->editor_id, ANE_FIND, flags, (long)str);
		/* If the text is still not found, restore current pos and anchor */
		if (ret < 0) {
			scintilla_send_message (SCINTILLA (editor), SCI_SETANCHOR,
									current_anchor, 0);
			scintilla_send_message (SCINTILLA (editor), SCI_SETCURRENTPOS,
									current_pos, 0);
		}
	}
	return ret;
}

void
text_editor_replace_selection (TextEditor * te, const gchar* r_str)
{
	if (!te) return;
	scintilla_send_message (SCINTILLA(te->scintilla), SCI_REPLACESEL, 0,
							(long)r_str);
}

guint
text_editor_get_total_lines (TextEditor * te)
{
	guint i;
	guint count = 0;
	if (te == NULL)
		return 0;
	if (IS_SCINTILLA (te->scintilla) == FALSE)
		return 0;
	for (i = 0;
	     i < scintilla_send_message (SCINTILLA (te->scintilla),
					 SCI_GETLENGTH, 0, 0); i++)
	{
		if (scintilla_send_message
		    (SCINTILLA (te->scintilla), SCI_GETCHARAT, i,
		     0) == '\n')
			count++;
	}
	return count;
}

guint
text_editor_get_current_lineno (TextEditor * te)
{
	guint count;
	
	g_return_val_if_fail (te != NULL, 0);

	count =	scintilla_send_message (SCINTILLA (te->scintilla),
					SCI_GETCURRENTPOS, 0, 0);
	count =	scintilla_send_message (SCINTILLA (te->scintilla),
					SCI_LINEFROMPOSITION, count, 0);
	return linenum_scintilla_to_text_editor(count);
}

guint
text_editor_get_position_lineno (TextEditor * te, gint position)
{
	guint count;
	g_return_val_if_fail (te != NULL, 0);

	count =	scintilla_send_message (SCINTILLA (te->scintilla),
					SCI_LINEFROMPOSITION, position, 0);
	return linenum_scintilla_to_text_editor(count);
}

guint
text_editor_get_current_column (TextEditor * te)
{
	g_return_val_if_fail (te != NULL, 0);
	return scintilla_send_message (SCINTILLA (te->scintilla),
								   SCI_GETCOLUMN,
								   text_editor_get_current_position (te), 0);
}

gboolean
text_editor_get_overwrite (TextEditor * te)
{
	g_return_val_if_fail (te != NULL, 0);
	return scintilla_send_message (SCINTILLA (te->scintilla),
								   SCI_GETOVERTYPE, 0, 0);
}

guint
text_editor_get_line_from_position (TextEditor * te, glong pos)
{
	guint count;
	
	g_return_val_if_fail (te != NULL, 0);

	count =	scintilla_send_message (SCINTILLA (te->scintilla),
					SCI_LINEFROMPOSITION, pos, 0);
	return linenum_scintilla_to_text_editor(count);
}

glong
text_editor_get_current_position (TextEditor * te)
{
	guint count;
	
	g_return_val_if_fail (te != NULL, 0);

	count =	scintilla_send_message (SCINTILLA (te->scintilla),
					SCI_GETCURRENTPOS, 0, 0);
	return count;
}

gboolean
text_editor_goto_point (TextEditor * te, glong point)
{
	g_return_val_if_fail (te != NULL, FALSE);
	g_return_val_if_fail(IS_SCINTILLA (te->scintilla) == TRUE, FALSE);

	scintilla_send_message (SCINTILLA (te->scintilla), SCI_GOTOPOS, point, 0);
	return TRUE;
}

gboolean
text_editor_goto_line (TextEditor * te, glong line,
					   gboolean mark, gboolean ensure_visible)
{
	gint selpos;
	g_return_val_if_fail (te != NULL, FALSE);
	g_return_val_if_fail(IS_SCINTILLA (te->scintilla) == TRUE, FALSE);
	g_return_val_if_fail(line >= 0, FALSE);

	te->current_line = line;
	if (mark) text_editor_set_line_marker (te, line);
	if (ensure_visible)
		scintilla_send_message (SCINTILLA (te->scintilla),
								SCI_ENSUREVISIBLE,
								linenum_text_editor_to_scintilla (line), 0);
	selpos = scintilla_send_message(SCINTILLA (te->scintilla),
									SCI_POSITIONFROMLINE,
								linenum_text_editor_to_scintilla (line), 0);
	scintilla_send_message (SCINTILLA (te->scintilla),
							SCI_SETSELECTIONSTART, selpos, 0);
	scintilla_send_message (SCINTILLA (te->scintilla),
							SCI_SETSELECTIONEND, selpos, 0);
	
	/* This ensures that we have arround 5 lines visible below the mark */
	scintilla_send_message (SCINTILLA (te->scintilla), SCI_GOTOLINE, 
		linenum_text_editor_to_scintilla (line)+5, 0);
	scintilla_send_message (SCINTILLA (te->scintilla), SCI_GOTOLINE, 
		linenum_text_editor_to_scintilla (line), 0);
	return TRUE;
}

gint
text_editor_goto_block_start (TextEditor* te)
{
	gint line;
	line = aneditor_command (te->editor_id, ANE_GETBLOCKSTARTLINE, 0, 0);
	if (line >= 0) text_editor_goto_line (te, line, TRUE, TRUE);
	else gdk_beep();
	return line;
}

gint
text_editor_goto_block_end (TextEditor* te)
{
	gint line;
	line = aneditor_command (te->editor_id, ANE_GETBLOCKENDLINE, 0, 0);
	if (line >= 0) text_editor_goto_line (te, line, TRUE, TRUE);
	else gdk_beep();
	return line;
}

gint
text_editor_set_marker (TextEditor *te, glong line, gint marker)
{
	g_return_val_if_fail (te != NULL, -1);
	g_return_val_if_fail(IS_SCINTILLA (te->scintilla) == TRUE, -1);

	/* Scintilla interprets line+1 rather than line */
	/* A bug perhaps */
	/* So counterbalance it with line-1 */
	/* Using the macros linenum_* */
	return scintilla_send_message (SCINTILLA (te->scintilla),
								   SCI_MARKERADD,
								   linenum_text_editor_to_scintilla (line),
								   marker);
}

gint
text_editor_set_indicator (TextEditor *te, gint start,
						   gint end, gint indicator)
{
	gchar ch;
	glong indic_mask[] = {INDIC0_MASK, INDIC1_MASK, INDIC2_MASK};
	gint current_styling_pos;
	
	g_return_val_if_fail (te != NULL, -1);
	g_return_val_if_fail (IS_SCINTILLA (te->scintilla) == TRUE, -1);

	if (start >= 0) {
		end --;	/* supplied end-location is one-past the last char to process */
		if (end < start)
			return -1;

		do
		{
			ch = scintilla_send_message (SCINTILLA (te->scintilla),
										 SCI_GETCHARAT, start, 0);
			start++;
		} while (isspace(ch));
		start--;
		
		do {
			ch = scintilla_send_message (SCINTILLA (te->scintilla),
										 SCI_GETCHARAT, end, 0);
			end--;
		} while (isspace(ch));
		end++;
		if (end < start) return -1;
		
		current_styling_pos = scintilla_send_message (SCINTILLA (te->scintilla),
													  SCI_GETENDSTYLED, 0, 0);
		if (indicator >= 0 && indicator < 3) {
			char current_mask;
			current_mask =
				scintilla_send_message (SCINTILLA (te->scintilla),
										SCI_GETSTYLEAT, start, 0);
			current_mask &= INDICS_MASK;
			current_mask |= indic_mask[indicator];
			scintilla_send_message (SCINTILLA (te->scintilla),
									SCI_STARTSTYLING, start, INDICS_MASK);
			scintilla_send_message (SCINTILLA (te->scintilla),
									SCI_SETSTYLING, end-start+1, current_mask);
		} else {
			scintilla_send_message (SCINTILLA (te->scintilla),
									SCI_STARTSTYLING, start, INDICS_MASK);
			scintilla_send_message (SCINTILLA (te->scintilla),
									SCI_SETSTYLING, end-start+1, 0);
		}
		if (current_styling_pos < start)
			scintilla_send_message (SCINTILLA (te->scintilla),
									SCI_STARTSTYLING, current_styling_pos,
									0x1F);
	} else {
		if (indicator < 0) {
			char current_mask;
			glong i, last, start_style_pos = 0;
			
			last = scintilla_send_message (SCINTILLA (te->scintilla),
										   SCI_GETTEXTLENGTH, 0, 0);
			current_styling_pos = scintilla_send_message (SCINTILLA (te->scintilla),
														  SCI_GETENDSTYLED, 0, 0);
			for (i = 0; i < last; i++)
			{
				current_mask =
					scintilla_send_message (SCINTILLA (te->scintilla),
											SCI_GETSTYLEAT, i, 0);
				current_mask &= INDICS_MASK;
				if (current_mask != 0)
				{
					if (start_style_pos == 0)
						start_style_pos = i;
					scintilla_send_message (SCINTILLA (te->scintilla),
											SCI_STARTSTYLING, i, INDICS_MASK);
					scintilla_send_message (SCINTILLA (te->scintilla),
											SCI_SETSTYLING, 1, 0);
				}
			}
			if (current_styling_pos < start_style_pos)
				scintilla_send_message (SCINTILLA (te->scintilla),
										SCI_STARTSTYLING, current_styling_pos,
										0x1F);
		}
	}
	return 0;
}

void
text_editor_set_line_marker (TextEditor *te, glong line)
{
	g_return_if_fail (te != NULL);
	g_return_if_fail(IS_SCINTILLA (te->scintilla) == TRUE);

	// FIXME: anjuta_delete_all_marker (TEXT_EDITOR_LINEMARKER);
	text_editor_delete_marker_all (te, TEXT_EDITOR_LINEMARKER);
	text_editor_set_marker (te, line, TEXT_EDITOR_LINEMARKER);
}

/* Support for DOS-Files
 *
 * On load, Anjuta will detect DOS-files by finding <CR><LF>. 
 * Anjuta will translate some chars >= 128 to native charset.
 * On save the DOS_EOL_CHECK(preferences->editor) will be checked
 * and chars >=128 will be replaced by DOS-codes, if any translation 
 * match(see struct tr_dos in this file) and <CR><LF> will used 
 * instead of <LF>.
 * The DOS_EOL_CHECK-checkbox will be set on loading a DOS-file.
 *
 *  23.Sep.2001  Denis Boehme <boehme at syncio dot de>
 */

/*
 * this is a translation table from unix->dos 
 * this table will be used by filter_chars and save filtered.
 */
static struct {
	unsigned char c; /* unix-char */
	unsigned char b; /* dos-char */
} tr_dos[]= {
	{ 'ä', 0x84 },
	{ 'Ä', 0x8e },
	{ 'ß', 0xe1 },
	{ 'ü', 0x81 },
	{ 'Ü', 0x9a },
	{ 'ö', 0x94 },
	{ 'Ö', 0x99 },
	{ 'é', 0x82 },
	{ 'É', 0x90 },
	{ 'è', 0x9a },
	{ 'È', 0xd4 },
	{ 'ê', 0x88 },
	{ 'Ê', 0xd2 },
	{ 'á', 0xa0 },
	{ 'Á', 0xb5 },
	{ 'à', 0x85 },
	{ 'À', 0xb7 },
	{ 'â', 0x83 },
	{ 'Â', 0xb6 },
	{ 'ú', 0xa3 },
	{ 'Ú', 0xe9 },
	{ 'ù', 0x97 },
	{ 'Ù', 0xeb },
	{ 'û', 0x96 },
	{ 'Û', 0xea }
};

/*
 * filter chars in buffer, by using tr_dos-table. 
 *
 */
static size_t
filter_chars_in_dos_mode(gchar *data_, size_t size )
{
	int k;
	size_t i;
	unsigned char * data = (unsigned char*)data_;
	unsigned char * tr_map;

	tr_map = (unsigned char *)malloc( 256 );
	memset( tr_map, 0, 256 );
	for ( k = 0; k < sizeof(tr_dos )/2 ; k++ )
		tr_map[tr_dos[k].b] = tr_dos[k].c;

	for ( i = 0; i < size; i++ )
	{
      		if ( (data[i] >= 128) && ( tr_map[data[i]] != 0) )
			data[i] = tr_map[data[i]];;
	}

	if ( tr_map )
		free( tr_map );

	return size;
}

/*
 * save buffer. filter chars and set dos-like CR/LF if dos_text is set.
 */
static gboolean
save_filtered_in_dos_mode(GFileOutputStream* stream, gchar *data_,
						  gsize size)
{
	gboolean result;
	gsize i, j;
	unsigned char *data;
	unsigned char *tr_map;
	int k;

	/* build the translation table */
	tr_map = malloc( 256 );
	memset( tr_map, 0, 256 );
	
	for ( k = 0; k < sizeof(tr_dos)/2; k++)
	  tr_map[tr_dos[k].c] = tr_dos[k].b;

	data = (unsigned char*)data_;
	i = 0; 
	j = 0;
	while ( i < size )
	{
		if (data[i]>=128) 
		{
			/* convert dos-text */
			if ( tr_map[data[i]] != 0 )
			{	
				gsize bytes_written;
				result = g_output_stream_write_all (G_OUTPUT_STREAM (stream), 
														&tr_map[data[i]], 1, 
														&bytes_written, 
														NULL, NULL);
	
				j += bytes_written;
			}
			else
			{
				/* char not found, skip transform */
				gsize bytes_written;
				result = g_output_stream_write_all (G_OUTPUT_STREAM (stream), 
														&data[i], 1, 
														&bytes_written, 
														NULL, NULL);
				j += bytes_written;
			}
			i++;
		} 
		else 
		{
			gsize bytes_written;
			result = g_output_stream_write_all (G_OUTPUT_STREAM (stream), 
														&data[i], 1, 
														&bytes_written, 
														NULL, NULL);
			j += bytes_written;
			i++;
		}
		if (!result)
			break;
	}

	if (tr_map)
		free (tr_map);
	return result;
}

static gint
determine_editor_mode(gchar* buffer, glong size)
{
	gint i;
	guint cr, lf, crlf, max_mode;
	gint mode;
	
	cr = lf = crlf = 0;
	
	for ( i = 0; i < size ; i++ )
	{
		if ( buffer[i] == 0x0a ){
			// LF
			// mode = SC_EOF_LF;
			lf++;
		} else if ( buffer[i] == 0x0d ) {
			if (i >= (size-1)) {
				// Last char
				// CR
				// mode = SC_EOL_CR;
				cr++;
			} else {
				if (buffer[i+1] != 0x0a) {
					// CR
					// mode = SC_EOL_CR;
					cr++;
				} else {
					// CRLF
					// mode = SC_EOL_CRLF;
					crlf++;
				}
				i++;
			}
		}
	}

	/* Vote for the maximum */
	mode = SC_EOL_LF;
	max_mode = lf;
	if (crlf > max_mode) {
		mode = SC_EOL_CRLF;
		max_mode = crlf;
	}
	if (cr > max_mode) {
		mode = SC_EOL_CR;
		max_mode = cr;
	}
	/* DEBUG_PRINT ("EOL chars: LR = %d, CR = %d, CRLF = %d", lf, cr, crlf); */
	/* DEBUG_PRINT ("Autodetected Editor mode [%d]", mode); */
	return mode;
}

static gchar *
convert_to_utf8 (PropsID props, const gchar *content, gsize len,
			     const AnjutaEncoding **encoding_used)
{
	GError* conv_error = NULL;
	gchar* new_content;
	gsize new_len;

	new_content = anjuta_convert_to_utf8 (content, 
										  len, 
										  encoding_used,
										  &new_len, 
										  &conv_error);
	if  (new_content == NULL)	
	{
		/* Last change, let's try 8859-15 */
		*encoding_used =  
			anjuta_encoding_get_from_charset("ISO-8859-15");
			
		new_content = anjuta_convert_to_utf8 (content,
											  len,
											  encoding_used,
											  &new_len,
											  &conv_error);
	}
	
	if (conv_error)
		g_error_free (conv_error);
	
	return new_content;
}

static gboolean
load_from_file (TextEditor *te, gchar *uri, gchar **err)
{
	GFile *gio_uri;
	GFileInputStream *stream;
	gboolean result;
	GFileInfo *info;
	gsize nchars;
	gint dos_filter, editor_mode;
	gchar *file_content = NULL;
	gchar *buffer = NULL;
	guint64 size; 

	scintilla_send_message (SCINTILLA (te->scintilla), SCI_CLEARALL,
							0, 0);
	gio_uri = g_file_new_for_uri (uri);
	info = g_file_query_info (gio_uri,
								G_FILE_ATTRIBUTE_STANDARD_SIZE,
								G_FILE_QUERY_INFO_NONE,
								NULL,
								NULL);

	if (info == NULL)
	{
		*err = g_strdup (_("Could not get file info"));
		g_object_unref (gio_uri);

		return FALSE;
	}
	size = g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_STANDARD_SIZE);
	g_object_unref (info);

	buffer = g_malloc (size + 1);
	if (buffer == NULL && size != 0)
	{
		/* DEBUG_PRINT ("%s", "This file is too big. Unable to allocate memory."); */
		*err = g_strdup (_("This file is too big. Unable to allocate memory."));
		g_object_unref (gio_uri);

		return FALSE;
	}
	
	stream = g_file_read (gio_uri, NULL, NULL);
	if (stream == NULL)
	{
		*err = g_strdup (_("Could not open file"));		
		g_object_unref (gio_uri);

		return FALSE;
	}
	/* Crude way of loading, but faster */
	result = g_input_stream_read_all (G_INPUT_STREAM (stream), 
										buffer, size, &nchars, NULL, NULL);
	if (!result)
	{
		g_free(buffer);
		*err = g_strdup (_("Error while reading from file"));
		g_object_unref (gio_uri);

		return FALSE;
	}
	
	if (buffer)
	{
		buffer[size] = '\0';
		file_content = g_strdup (buffer);
	}
	
	if (size != nchars)
	{
		/* DEBUG_PRINT ("File size and loaded size not matching"); */
	}
	dos_filter = 
		anjuta_preferences_get_bool (ANJUTA_PREFERENCES (te->preferences),
									DOS_EOL_CHECK);
	
	/* Set editor mode */
	editor_mode =  determine_editor_mode (buffer, nchars);
	scintilla_send_message (SCINTILLA (te->scintilla),
							SCI_SETEOLMODE, editor_mode, 0);

	/* DEBUG_PRINT ("Loaded in editor mode [%d]", editor_mode); */

	/* Determine character encoding and convert to utf-8*/
	if (nchars > 0)
	{
		if (g_utf8_validate (buffer, nchars, NULL))
		{
			te->encoding = NULL;
		}
		else
		{
			gchar *converted_text;
	
			converted_text = convert_to_utf8 (te->props_base,
											  buffer, nchars, &te->encoding);
			
			if (converted_text == NULL)
			{
				/* bail out */
				g_free (buffer);
				g_free (file_content);
				*err = g_strdup (_("The file does not look like a text file or the file encoding is not supported."
								   " Please check if the encoding of file is in the supported encodings list."
								   " If not, add it from the preferences."));
				g_object_unref (gio_uri);

				return FALSE;
			}
			g_free (buffer);
			buffer = converted_text;
			nchars = strlen (converted_text);
		}
	}
	if (dos_filter && editor_mode == SC_EOL_CRLF){
		/* DEBUG_PRINT ("Filtering Extrageneous DOS characters in dos mode [Dos => Unix]"); */
		nchars = filter_chars_in_dos_mode( buffer, nchars );
	}
	scintilla_send_message (SCINTILLA (te->scintilla), SCI_ADDTEXT,
							nchars, (long) buffer);
	
	g_free (buffer);

	/* Save the buffer as last saved content */
	g_free (te->last_saved_content);
	te->last_saved_content = file_content;
	
	g_object_unref (gio_uri);

	return TRUE;
}

static gboolean
save_to_file (TextEditor *te, gchar *uri, GError **error)
{
	GFileOutputStream *stream;
	GFile *gio_uri;
	gsize nchars, size;
	gint strip;
	gchar *data;
	gboolean result;

	gio_uri = g_file_new_for_uri (uri);
	stream = g_file_replace (gio_uri, NULL, FALSE, G_FILE_CREATE_NONE, NULL, error);

 	if (stream == NULL)
		return FALSE;

	result = TRUE;
	
	nchars = scintilla_send_message (SCINTILLA (te->scintilla),
									 SCI_GETLENGTH, 0, 0);
	data =	(gchar *) aneditor_command (te->editor_id,
										ANE_GETTEXTRANGE, 0, nchars);
	if (data)
	{
		gint dos_filter, editor_mode;
		
		size = strlen (data);
		

		/* Save in original encoding */
		if ((te->encoding != NULL))
		{
			GError *conv_error = NULL;
			gchar* converted_file_contents = NULL;
			gsize new_len;
			
			/* DEBUG_PRINT ("Using encoding %s", te->encoding); */
			
			/* Try to convert it from UTF-8 to original encoding */
			converted_file_contents = anjuta_convert_from_utf8 (data, -1, 
																te->encoding,
																&new_len,
																&conv_error); 
			
			if (conv_error != NULL)
			{
				/* Conversion error */
				g_error_free (conv_error);
			}
			else
			{
				g_free (data);
				data = converted_file_contents;
				size = strlen (converted_file_contents);
			}
		}
		else
		{
			/* Save in utf-8 */
			/* DEBUG_PRINT ("Using utf-8 encoding"); */
		}				
		
		/* Strip trailing spaces */
		strip = anjuta_preferences_get_bool (te->preferences,
											STRIP_TRAILING_SPACES);
		if (strip)
		{
			while (size > 0 && isspace(data[size - 1]))
				-- size;
		}
		if ((size > 1) && ('\n' != data[size-1]))
		{
			data[size] = '\n';
			++ size;
		}
		dos_filter = anjuta_preferences_get_bool (te->preferences,
												 DOS_EOL_CHECK);
		editor_mode =  scintilla_send_message (SCINTILLA (te->scintilla),
											   SCI_GETEOLMODE, 0, 0);
		/* DEBUG_PRINT ("Saving in editor mode [%d]", editor_mode); */
		nchars = size;
		if (editor_mode == SC_EOL_CRLF && dos_filter)
		{
			/* DEBUG_PRINT ("Filtering Extrageneous DOS characters in dos mode [Unix => Dos]"); */
			size = save_filtered_in_dos_mode (stream, data, size);
		}
		else
		{
			result = g_output_stream_write_all (G_OUTPUT_STREAM (stream), 
														data, size, 
														&nchars, NULL, error);
		}
	}
	
	/* Set last content saved to data */
	g_free (te->last_saved_content);
	te->last_saved_content = data;
	
	if (result)
		result = g_output_stream_close (G_OUTPUT_STREAM (stream), NULL, error);
	else
		g_output_stream_close (G_OUTPUT_STREAM (stream), NULL, NULL);

	g_object_unref (gio_uri);
	
	return result;
}

gboolean
text_editor_load_file (TextEditor * te)
{
	gchar *err = NULL;
	
	if (te == NULL || te->filename == NULL)
		return FALSE;
	if (IS_SCINTILLA (te->scintilla) == FALSE)
		return FALSE;
	anjuta_status (te->status, _("Loading file..."), 5); 
	
	text_editor_freeze (te);
	
	// te->modified_time = time (NULL);
	text_editor_update_monitor (te, FALSE);
	if (load_from_file (te, te->uri, &err) == FALSE)
	{
		anjuta_util_dialog_error (NULL,
								  _("Could not load file: %s\n\nDetails: %s"),
								  te->filename, err);
		g_free (err);
		text_editor_thaw (te);
		return FALSE;
	}
	scintilla_send_message (SCINTILLA (te->scintilla), SCI_GOTOPOS,
							0, 0);
	// check_tm_file(te);
	text_editor_thaw (te);
	te->force_not_saved = FALSE;
	scintilla_send_message (SCINTILLA (te->scintilla),
							SCI_SETSAVEPOINT, 0, 0);
	scintilla_send_message (SCINTILLA (te->scintilla),
							SCI_EMPTYUNDOBUFFER, 0, 0);
	text_editor_set_hilite_type (te, NULL);
	if (anjuta_preferences_get_bool (te->preferences, FOLD_ON_OPEN))
	{
		aneditor_command (te->editor_id, ANE_CLOSE_FOLDALL, 0, 0);
	}
	text_editor_set_line_number_width(te);
	anjuta_status (te->status, _("File loaded successfully"), 5); 
	return TRUE;
}

gboolean
text_editor_save_file (TextEditor * te, gboolean update)
{
	GtkWindow *parent;
	GError *error = NULL;
	gboolean ret = FALSE;

	g_return_val_if_fail (te != NULL, FALSE);
	g_return_val_if_fail (IS_SCINTILLA (te->scintilla), FALSE);	
	
	text_editor_freeze (te);
	text_editor_set_line_number_width(te);
	parent = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (te)));
	
	anjuta_status (te->status, _("Saving file..."), 5); 
	text_editor_update_monitor (te, TRUE);
	
	if (!save_to_file (te, te->uri, &error))
	{	
		text_editor_thaw (te);
		g_return_val_if_fail (error != NULL, FALSE);
		
		anjuta_util_dialog_error (parent,
								  _("Could not save intermediate file %s: %s"),
								  te->uri,
								  error->message);
		g_signal_emit_by_name (G_OBJECT (te), "saved", NULL);
		g_error_free (error);
	}
	else
	{
		GFile* file = g_file_new_for_uri (te->uri);
		/* we need to update UI with the call to scintilla */
		text_editor_thaw (te);
		te->force_not_saved = FALSE;
		scintilla_send_message (SCINTILLA (te->scintilla),
								SCI_SETSAVEPOINT, 0, 0);
		g_signal_emit_by_name (G_OBJECT (te), "saved", file);
		g_object_unref (file);
		anjuta_status (te->status, _("File saved successfully"), 5);
		ret = TRUE;
	}
	text_editor_update_monitor (te, FALSE);

	return ret;
}

void
text_editor_set_saved (TextEditor *te, gboolean saved)
{
	if (saved)
	{
		scintilla_send_message (SCINTILLA (te->scintilla), SCI_SETSAVEPOINT, 0, 0);
	}
	te->force_not_saved = !saved;
	g_signal_emit_by_name(G_OBJECT (te), "update-save-ui");
}

gboolean
text_editor_save_yourself (TextEditor * te, FILE * stream)
{
	return TRUE;
}

gboolean
text_editor_recover_yourself (TextEditor * te, FILE * stream)
{
	return TRUE;
}

void
text_editor_undo (TextEditor * te)
{
	scintilla_send_message (SCINTILLA (te->scintilla),
				SCI_UNDO, 0, 0);
}

void
text_editor_redo (TextEditor * te)
{
	scintilla_send_message (SCINTILLA (te->scintilla),
				SCI_REDO, 0, 0);
}

void
text_editor_update_controls (TextEditor * te)
{
#if 0 //FIXME
	gboolean F, P, L, C, S;

	if (te == NULL)
		return;
	S = text_editor_is_saved (te);
	L = anjuta_launcher_is_busy (app->launcher);
	P = app->project_dbase->project_is_open;

	switch (get_file_ext_type (te->filename))
	{
	case FILE_TYPE_C:
	case FILE_TYPE_CPP:
	case FILE_TYPE_ASM:
		F = TRUE;
		break;
	default:
		F = FALSE;
	}
	switch (get_file_ext_type (te->filename))
	{
	case FILE_TYPE_C:
	case FILE_TYPE_CPP:
	case FILE_TYPE_HEADER:
		C = TRUE;
		break;
	default:
		C = FALSE;
	}
	gtk_widget_set_sensitive (te->buttons.save, !S);
	gtk_widget_set_sensitive (te->buttons.reload,
				  (te->uri != NULL));
	gtk_widget_set_sensitive (te->buttons.compile, F && !L);
	gtk_widget_set_sensitive (te->buttons.build, (F || P) && !L);
#endif
}

gboolean
text_editor_is_saved (TextEditor * te)
{
	return !(scintilla_send_message (SCINTILLA (te->scintilla),
					 SCI_GETMODIFY, 0, 0)) && (!te->force_not_saved);
}

gboolean
text_editor_has_selection (TextEditor * te)
{
	gint from, to;
	from = scintilla_send_message (SCINTILLA (te->scintilla),
				       SCI_GETSELECTIONSTART, 0, 0);
	to = scintilla_send_message (SCINTILLA (te->scintilla),
				     SCI_GETSELECTIONEND, 0, 0);
	return (from == to) ? FALSE : TRUE;
}

glong text_editor_get_selection_start (TextEditor * te)
{
	return scintilla_send_message (SCINTILLA (te->scintilla),
				       SCI_GETSELECTIONSTART, 0, 0);
}
	
glong text_editor_get_selection_end (TextEditor * te)
{
	return scintilla_send_message (SCINTILLA (te->scintilla),
				     SCI_GETSELECTIONEND, 0, 0);
}

gchar*
text_editor_get_selection (TextEditor * te)
{
	guint from, to;
	struct TextRange tr;

	from = scintilla_send_message (SCINTILLA (te->scintilla),
				       SCI_GETSELECTIONSTART, 0, 0);
	to = scintilla_send_message (SCINTILLA (te->scintilla),
				     SCI_GETSELECTIONEND, 0, 0);
	if (from == to)
		return NULL;
	tr.chrg.cpMin = MIN(from, to);
	tr.chrg.cpMax = MAX(from, to);
	tr.lpstrText = g_malloc (sizeof(gchar)*(tr.chrg.cpMax-tr.chrg.cpMin)+5);
	scintilla_send_message (SCINTILLA(te->scintilla), SCI_GETTEXTRANGE, 0, (long)(&tr));
	return tr.lpstrText;
}

gboolean
text_editor_is_marker_set (TextEditor* te, glong line, gint marker)
{
	gint state;

	g_return_val_if_fail (te != NULL, FALSE);
	g_return_val_if_fail (line >= 0, FALSE);
	g_return_val_if_fail (marker < 32, FALSE);
	
	state = scintilla_send_message (SCINTILLA(te->scintilla),
		SCI_MARKERGET, linenum_text_editor_to_scintilla (line), 0);
	return ((state & (1 << marker)));
}

void
text_editor_delete_marker_all (TextEditor *te, gint marker)
{
	g_return_if_fail (IS_TEXT_EDITOR (te));
	g_return_if_fail (marker < 32);
	scintilla_send_message (SCINTILLA (te->scintilla),
							SCI_MARKERDELETEALL, marker, 0);
}

void
text_editor_delete_marker (TextEditor* te, glong line, gint marker)
{
	g_return_if_fail (IS_TEXT_EDITOR (te));
	g_return_if_fail (line >= 0);
	g_return_if_fail (marker < 32);
	
	scintilla_send_message (SCINTILLA(te->scintilla),
		SCI_MARKERDELETE, linenum_text_editor_to_scintilla (line), marker);
}

gint
text_editor_line_from_handle (TextEditor* te, gint marker_handle)
{
	gint line;
	
	g_return_val_if_fail (te != NULL, -1);
	
	line = scintilla_send_message (SCINTILLA(te->scintilla),
		SCI_MARKERLINEFROMHANDLE, marker_handle, 0);
	
	return linenum_scintilla_to_text_editor (line);
}

gint
text_editor_get_bookmark_line( TextEditor* te, const glong nLineStart )
{
	return aneditor_command (te->editor_id, ANE_GETBOOKMARK_POS, nLineStart, 0);
}

gint
text_editor_get_num_bookmarks(TextEditor* te)
{
	gint	nLineNo = -1 ;
	gint	nMarkers = 0 ;
	
	g_return_val_if_fail (te != NULL, 0 );

	while( ( nLineNo = text_editor_get_bookmark_line( te, nLineNo ) ) >= 0 )
	{
		//printf( "Line %d\n", nLineNo );
		nMarkers++;
	}
	//printf( "out Line %d\n", nLineNo );
	return nMarkers ;
}

/* Gets the word before just before carat */
gchar*
text_editor_get_word_before_carat (TextEditor *te)
{
	gchar buffer[512];
	buffer[0] = '\0';
	aneditor_command (TEXT_EDITOR (te)->editor_id,
					  ANE_GETWORDBEFORECARAT, (glong) buffer, 512);
	if (buffer[0] != '\0')
		return g_strdup (buffer);
	else
		return NULL;
}

/*
 *Get the current selection. If there is no selection, or if the selection
 * is all blanks, get the word under teh cursor.
 */
gchar*
text_editor_get_current_word (TextEditor *te)
{
	char *buf = text_editor_get_selection(te);
	if (buf)
	{
		g_strstrip(buf);
		if ('\0' == *buf)
		{
			g_free(buf);
			buf = NULL;
		}
	}
	if (NULL == buf)
	{
		int ret;
		buf = g_new(char, 256);
		ret = aneditor_command (te->editor_id, ANE_GETCURRENTWORD, (long)buf, 255L);
		if (!ret)
		{
			g_free(buf);
			buf = NULL;
		}
	}
/*
#ifdef DEBUG
	if (buf)
		DEBUG_PRINT ("Current word is '%s'", buf);
#endif
*/
	return buf;
}

void
text_editor_function_select(TextEditor *te)
{
	gint pos;
	gint line;
	gint fold_level;
	gint start, end;	
	gint line_count;
	gint tmp;

	line_count = scintilla_send_message(SCINTILLA(te->scintilla), 
	                                    SCI_GETLINECOUNT, 0, 0);
	pos = scintilla_send_message(SCINTILLA(te->scintilla), 
	                             SCI_GETCURRENTPOS, 0, 0);
	line = scintilla_send_message(SCINTILLA(te->scintilla),
	                              SCI_LINEFROMPOSITION, pos, 0);

	tmp = line + 1;	
	fold_level = scintilla_send_message(SCINTILLA(te->scintilla), 
	                                    SCI_GETFOLDLEVEL, line, 0) ;	
	if ((fold_level & 0xFF) != 0)
	{
		while((fold_level & 0x10FF) != 0x1000 && line >= 0)
			fold_level = scintilla_send_message(SCINTILLA(te->scintilla), 
	                                    SCI_GETFOLDLEVEL, --line, 0) ;
		start = scintilla_send_message(SCINTILLA(te->scintilla), 
	                                    SCI_POSITIONFROMLINE, line + 1, 0);
		line = tmp;
		fold_level = scintilla_send_message(SCINTILLA(te->scintilla), 
	                                        SCI_GETFOLDLEVEL, line, 0) ;
		while((fold_level & 0x10FF) != 0x1000 && line < line_count)
			fold_level = scintilla_send_message(SCINTILLA(te->scintilla), 
	                                            SCI_GETFOLDLEVEL, ++line, 0) ;

		end = scintilla_send_message(SCINTILLA(te->scintilla), 
	                                 SCI_POSITIONFROMLINE, line , 0);
		scintilla_send_message(SCINTILLA(te->scintilla), 
	                           SCI_SETSEL, start, end) ;
	}
}

void
text_editor_grab_focus (TextEditor *te)
{
	g_return_if_fail (IS_TEXT_EDITOR (te));
	scintilla_send_message (SCINTILLA (te->scintilla), SCI_GRABFOCUS, 0, 0);
}

void
text_editor_set_popup_menu (TextEditor *te, GtkWidget *popup_menu)
{
	if (popup_menu)
		g_object_ref (popup_menu);
	if (te->popup_menu)
		g_object_unref (te->popup_menu);
	te->popup_menu = popup_menu;
}

void
text_editor_set_busy (TextEditor *te, gboolean state)
{
	if (state)
		scintilla_send_message (SCINTILLA (te->scintilla),
								SCI_SETCURSOR, SC_CURSORWAIT, 0);
	else
		scintilla_send_message (SCINTILLA (te->scintilla),
								SCI_SETCURSOR, SC_CURSORNORMAL, 0);
}

gint
text_editor_get_props ()
{
	/* Built in values */
	static PropsID props_built_in = 0;
	
	/* System values */
	static PropsID props_global = 0;
	
	/* User values */ 
	// static PropsID props_local = 0;
	
	/* Session values */
	static PropsID props_session = 0;
	
	/* Instance values */
	static PropsID props = 0;
	
	gchar *propdir, *propfile;

	if (props)
		return props;
	
	props_built_in = sci_prop_set_new ();
	props_global = sci_prop_set_new ();
	// props_local = sci_prop_set_new ();
	props_session = sci_prop_set_new ();
	props = sci_prop_set_new ();

	sci_prop_clear (props_built_in);
	sci_prop_clear (props_global);
	// sci_prop_clear (props_local);
	sci_prop_clear (props_session);
	sci_prop_clear (props);

	sci_prop_set_parent (props_global, props_built_in);
	// sci_prop_set_parent (props_local, props_global);
	// sci_prop_set_parent (props_session, props_local);
	sci_prop_set_parent (props_session, props_global);
	sci_prop_set_parent (props, props_session);
	
	propdir = g_build_filename (PACKAGE_DATA_DIR, "properties/", NULL);
	propfile = g_build_filename (PACKAGE_DATA_DIR, "properties",
								 "anjuta.properties", NULL);
	/* DEBUG_PRINT ("Reading file: %s", propfile); */
	
	if (g_file_test (propfile, G_FILE_TEST_EXISTS) == FALSE)
	{
		anjuta_util_dialog_error (NULL,
			_("Cannot load Global defaults and configuration files:\n"
			 "%s.\n"
			 "This may result in improper behaviour or instabilities.\n"
			 "Anjuta will fall back to built in (limited) settings"),
			 propfile);
	}
	sci_prop_read (props_global, propfile, propdir);
	g_free (propfile);
	g_free (propdir);

	propdir = anjuta_util_get_user_config_file_path ("scintilla/",NULL);
	propfile = anjuta_util_get_user_config_file_path ("scintilla","editor-style.properties",NULL);
	/* DEBUG_PRINT ("Reading file: %s", propfile); */
	
	/* Create user.properties file, if it doesn't exist */
	if (g_file_test (propfile, G_FILE_TEST_EXISTS) == FALSE) {
		gchar* old_propfile = anjuta_util_get_user_config_file_path ("scintilla", "session.properties", NULL);
		if (g_file_test (old_propfile, G_FILE_TEST_EXISTS) == TRUE)
			anjuta_util_copy_file (old_propfile, propfile, FALSE);
		g_free (old_propfile);
	}
	sci_prop_read (props_session, propfile, propdir);
	g_free (propdir);
	g_free (propfile);

	return props;
}

void
text_editor_set_line_number_width (TextEditor* te)
{
	/* Set line numbers with according to file size */
	if (anjuta_preferences_get_bool_with_default(te->preferences,
			"margin.linenumber.visible", FALSE))
	{
		int lines, line_number_width;
		gchar* line_number;
		gchar* line_number_dummy;
		
		lines = 
			(int) scintilla_send_message
				(SCINTILLA(te->scintilla), SCI_GETLINECOUNT, 0,0);
		line_number = g_strdup_printf("%d", lines);
		line_number_dummy = g_strnfill(strlen(line_number) + 1, '9');
		line_number_width = 
			(int) scintilla_send_message (SCINTILLA(te->scintilla),
										  SCI_TEXTWIDTH,
										  STYLE_LINENUMBER,
										  (long) line_number_dummy);
		text_editor_command (te, ANE_SETLINENUMWIDTH, line_number_width, 0);
		g_free(line_number_dummy);
		g_free(line_number);
	}
}

gboolean
text_editor_can_undo (TextEditor *te)
{
	g_return_val_if_fail (IS_TEXT_EDITOR (te), FALSE);
	return scintilla_send_message (SCINTILLA (te->scintilla),
								   SCI_CANUNDO, 0, 0);
}

gboolean
text_editor_can_redo (TextEditor *te)
{
	g_return_val_if_fail (IS_TEXT_EDITOR (te), FALSE);
	return scintilla_send_message (SCINTILLA (te->scintilla),
								   SCI_CANREDO, 0, 0);
}

void
text_editor_show_hover_tip (TextEditor *te, gint position, const gchar *info)
{
	text_editor_hide_hover_tip (te);
	if (!te->hover_tip_on)
	{
		scintilla_send_message (SCINTILLA (te->scintilla), SCI_CALLTIPSHOW,
								position, (long)info);
		scintilla_send_message (SCINTILLA (te->scintilla), SCI_CALLTIPSETHLT,
								strlen (info), 0);
		te->hover_tip_on = TRUE;
	}
}

void
text_editor_hide_hover_tip (TextEditor *te)
{
	if (te->hover_tip_on)
	{
		scintilla_send_message (SCINTILLA (te->scintilla),
								SCI_CALLTIPCANCEL, 0, 0);
		te->hover_tip_on = FALSE;
	}
}

void
text_editor_command (TextEditor *te, gint command, glong wparam, glong lparam)
{
	GList *node;
	
	node = te->views;
	while (node)
	{
		aneditor_command (GPOINTER_TO_INT (node->data), command, wparam, lparam);
		node = g_list_next(node);
	}
}

void
text_editor_scintilla_command (TextEditor *te, gint command, glong wparam,
							   glong lparam)
{
	GList *node;
	
	node = te->views;
	while (node)
	{
		GtkWidget *scintilla;
		scintilla = aneditor_get_widget (GPOINTER_TO_INT (node->data));
		scintilla_send_message (SCINTILLA(scintilla), command, wparam, lparam);
		node = g_list_next(node);
	}
}

/* IAnjutaEditor interface implementation */

static gint
itext_editor_get_tab_size (IAnjutaEditor *editor, GError **e)
{
	return scintilla_send_message (SCINTILLA (TEXT_EDITOR (editor)->scintilla),
								   SCI_GETTABWIDTH, 0, 0);
}

static void
itext_editor_set_tab_size (IAnjutaEditor *editor, gint tabsize, GError **e)
{
	scintilla_send_message (SCINTILLA (TEXT_EDITOR (editor)->scintilla),
							SCI_SETTABWIDTH, tabsize, 0);
}

static gboolean
itext_editor_get_use_spaces (IAnjutaEditor *editor, GError **e)
{
	return !scintilla_send_message (SCINTILLA (TEXT_EDITOR (editor)->scintilla),
								    SCI_GETUSETABS, 0, 0);
}

static void
itext_editor_set_use_spaces (IAnjutaEditor *editor, gboolean use_spaces, GError **e)
{
	scintilla_send_message (SCINTILLA (TEXT_EDITOR (editor)->scintilla),
							SCI_SETUSETABS, !use_spaces, 0);
}

static void
itext_editor_set_auto_indent (IAnjutaEditor *editor, gboolean auto_indent, GError **e)
{
  text_editor_command (TEXT_EDITOR(editor), ANE_SETINDENTMAINTAIN, auto_indent, 0);
}

static void
itext_editor_goto_line (IAnjutaEditor *editor, gint lineno, GError **e)
{
	text_editor_goto_line (TEXT_EDITOR (editor), lineno, FALSE, TRUE);
	gtk_widget_grab_focus (TEXT_EDITOR (editor)->scintilla);
}

static void
itext_editor_goto_start (IAnjutaEditor *editor, GError **e)
{
	text_editor_goto_point (TEXT_EDITOR (editor), 0);
}

static void
itext_editor_goto_end (IAnjutaEditor *editor, GError **e)
{
	text_editor_goto_point (TEXT_EDITOR (editor), -1);
}

static void
itext_editor_goto_position (IAnjutaEditor *editor, IAnjutaIterable *position,
							GError **e)
{
	text_editor_goto_point (TEXT_EDITOR (editor),
							text_editor_cell_get_position (TEXT_EDITOR_CELL
														   (position)));
}

static gchar*
itext_editor_get_text_all (IAnjutaEditor *editor, GError **e)
{
	TextEditor *te = TEXT_EDITOR (editor);
	gint length = scintilla_send_message (SCINTILLA (te->scintilla),
										  SCI_GETLENGTH, 0, 0);
	if (length > 0)
		return (gchar *) aneditor_command (te->editor_id, ANE_GETTEXTRANGE,
										   0, length);
	else
		return NULL;
}

static gchar*
itext_editor_get_text (IAnjutaEditor *editor, IAnjutaIterable* begin,
					   IAnjutaIterable* end, GError **e)
{
	gchar *data;
	gint start_pos = text_editor_cell_get_position (TEXT_EDITOR_CELL (begin));
	gint end_pos = text_editor_cell_get_position (TEXT_EDITOR_CELL (end));
	TextEditor *te = TEXT_EDITOR (editor);
	data =	(gchar *) aneditor_command (te->editor_id, ANE_GETTEXTRANGE,
										  start_pos, end_pos);
	return data;
}

static IAnjutaIterable*
itext_editor_get_position (IAnjutaEditor* editor, GError **e)
{
	TextEditor *te = TEXT_EDITOR (editor);
	gint position = text_editor_get_current_position (te);
	TextEditorCell *position_iter = text_editor_cell_new (te, position);
	return IANJUTA_ITERABLE (position_iter);
}

static gint
itext_editor_get_offset (IAnjutaEditor *editor, GError **e)
{
	gint pos;
	IAnjutaIterable *iter = itext_editor_get_position (editor, NULL);
	pos = ianjuta_iterable_get_position (iter, NULL);
	g_object_unref (iter);
	return pos;
}

static gint
itext_editor_get_lineno (IAnjutaEditor *editor, GError **e)
{
	return text_editor_get_current_lineno (TEXT_EDITOR (editor));
}

static gint
itext_editor_get_length (IAnjutaEditor *editor, GError **e)
{
        return aneditor_command (TEXT_EDITOR (editor)->editor_id,
		       	ANE_GETLENGTH, 0, 0);
}

static gchar*
itext_editor_get_current_word (IAnjutaEditor *editor, GError **e)
{
	gchar buffer[512];
	buffer[0] = '\0';
	aneditor_command (TEXT_EDITOR (editor)->editor_id,
					  ANE_GETCURRENTWORD, (glong) buffer, 512);
	if (buffer[0] != '\0')
		return g_strdup (buffer);
	else
		return NULL;
}

static void
itext_editor_insert (IAnjutaEditor *editor, IAnjutaIterable *position,
					 const gchar *txt, gint length, GError **e)
{
	gchar *text_to_insert;
	if (length >= 0)
		text_to_insert = g_strndup (txt, length);
	else
		text_to_insert = g_strdup (txt);
	
	aneditor_command (TEXT_EDITOR(editor)->editor_id, ANE_INSERTTEXT,
					  text_editor_cell_get_position
					  (TEXT_EDITOR_CELL (position)),
					  (long)text_to_insert);
	g_free (text_to_insert);
}

static void
itext_editor_append (IAnjutaEditor *editor, const gchar *txt,
					 gint length, GError **e)
{
	gchar *text_to_insert;
	if (length >= 0)
		text_to_insert = g_strndup (txt, length);
	else
		text_to_insert = g_strdup (txt);
	
	scintilla_send_message (SCINTILLA (TEXT_EDITOR (editor)->scintilla),
							SCI_APPENDTEXT, strlen(text_to_insert),
							(long)text_to_insert);
	g_free (text_to_insert);
}

static void
itext_editor_erase (IAnjutaEditor *editor,
					IAnjutaIterable *position_start,
					IAnjutaIterable *position_end, GError **e)
{
	gint start, end;
	
	/* If both positions are NULL, erase all */
	if (position_start == NULL && position_end == NULL)
	{
		scintilla_send_message (SCINTILLA (TEXT_EDITOR (editor)->scintilla), 
								SCI_CLEARALL,
								0, 0);
		return;
	}
	
	/* Determine correct start and end byte positions */
	if (position_start)
		start = text_editor_cell_get_position (TEXT_EDITOR_CELL (position_start));
	else
		start = 0;
	
	if (position_end)
		end = text_editor_cell_get_position (TEXT_EDITOR_CELL (position_end));
	else
		end = scintilla_send_message (SCINTILLA (TEXT_EDITOR (editor)->scintilla),
									  SCI_GETLENGTH, 0, 0);
	if (start != end)
	{
		scintilla_send_message (SCINTILLA(TEXT_EDITOR (editor)->scintilla),
								SCI_SETSEL, start, end);
		text_editor_replace_selection (TEXT_EDITOR (editor), "");
	}
}

static void
itext_editor_erase_all (IAnjutaEditor *editor, GError **e)
{
	scintilla_send_message (SCINTILLA (TEXT_EDITOR (editor)->scintilla), 
							SCI_CLEARALL,
							0, 0);
}

static int
itext_editor_get_column (IAnjutaEditor *editor, GError **e)
{
	return text_editor_get_current_column (TEXT_EDITOR(editor));
}

static gboolean
itext_editor_get_overwrite (IAnjutaEditor *editor, GError **e)
{
	return text_editor_get_overwrite (TEXT_EDITOR (editor));
}

static void
itext_editor_set_popup_menu (IAnjutaEditor *editor, GtkWidget* menu, GError **e)
{
	text_editor_set_popup_menu (TEXT_EDITOR (editor), menu);
}

static gint
itext_editor_get_line_from_position (IAnjutaEditor *editor,
									 IAnjutaIterable *position, GError **e)
{
	return text_editor_get_line_from_position (TEXT_EDITOR (editor),
				text_editor_cell_get_position (TEXT_EDITOR_CELL (position)));
}

static IAnjutaIterable*
itext_editor_get_line_begin_position (IAnjutaEditor *editor, gint line,
									  GError **e)
{
	gint ln, byte_pos;
	TextEditor *te;
	
	g_return_val_if_fail (line > 0, NULL);
	te = TEXT_EDITOR (editor);
	ln = linenum_text_editor_to_scintilla (line);
	byte_pos = scintilla_send_message (SCINTILLA (te->scintilla),
								   SCI_POSITIONFROMLINE, ln, 0);
	return IANJUTA_ITERABLE (text_editor_cell_new (te, byte_pos));
}

static IAnjutaIterable*
itext_editor_get_line_end_position (IAnjutaEditor *editor, gint line,
									  GError **e)
{
	gint ln, byte_pos;
	TextEditor *te;
	
	g_return_val_if_fail (line > 0, NULL);
	te = TEXT_EDITOR (editor);
	
	ln = linenum_text_editor_to_scintilla (line);
	byte_pos = scintilla_send_message (SCINTILLA (te->scintilla),
								   SCI_GETLINEENDPOSITION, ln, 0);
	return IANJUTA_ITERABLE (text_editor_cell_new (te, byte_pos));
}

static IAnjutaIterable*
itext_editor_get_position_from_offset (IAnjutaEditor *editor, gint offset, GError **e)
{
	TextEditorCell *editor_cell = text_editor_cell_new (TEXT_EDITOR (editor), 0);
	/* Set to the right utf8 character offset */
	ianjuta_iterable_set_position (IANJUTA_ITERABLE (editor_cell), offset, NULL);
	return IANJUTA_ITERABLE (editor_cell);
}

static IAnjutaIterable*
itext_editor_get_start_position (IAnjutaEditor *editor, GError **e)
{
	TextEditorCell *editor_cell = text_editor_cell_new (TEXT_EDITOR (editor), 0);
	return IANJUTA_ITERABLE (editor_cell);
}

static IAnjutaIterable*
itext_editor_get_end_position (IAnjutaEditor *editor, GError **e)
{
	gint length = scintilla_send_message (SCINTILLA (TEXT_EDITOR (editor)->scintilla),
										  SCI_GETLENGTH, 0, 0);
	TextEditorCell *editor_cell = text_editor_cell_new (TEXT_EDITOR (editor),
														length);
	return IANJUTA_ITERABLE (editor_cell);
}

static void
itext_editor_iface_init (IAnjutaEditorIface *iface)
{
	iface->get_tabsize = itext_editor_get_tab_size;
	iface->set_tabsize = itext_editor_set_tab_size;
	iface->get_use_spaces = itext_editor_get_use_spaces;
	iface->set_use_spaces = itext_editor_set_use_spaces;
	iface->set_auto_indent = itext_editor_set_auto_indent;	
	iface->goto_line = itext_editor_goto_line;
	iface->goto_start = itext_editor_goto_start;
	iface->goto_end = itext_editor_goto_end;
	iface->goto_position = itext_editor_goto_position;
	iface->get_text_all = itext_editor_get_text_all;
	iface->get_text = itext_editor_get_text;
	iface->get_offset = itext_editor_get_offset;
	iface->get_position = itext_editor_get_position;
	iface->get_lineno = itext_editor_get_lineno;
	iface->get_length = itext_editor_get_length;
	iface->get_current_word = itext_editor_get_current_word;
	iface->insert = itext_editor_insert;
	iface->append = itext_editor_append;
	iface->erase = itext_editor_erase;
	iface->erase_all = itext_editor_erase_all;
	iface->get_column = itext_editor_get_column;
	iface->get_overwrite = itext_editor_get_overwrite;
	iface->set_popup_menu = itext_editor_set_popup_menu;
	iface->get_line_from_position = itext_editor_get_line_from_position;
	iface->get_line_begin_position = itext_editor_get_line_begin_position;
	iface->get_line_end_position = itext_editor_get_line_end_position;
	iface->get_position_from_offset = itext_editor_get_position_from_offset;
	iface->get_start_position = itext_editor_get_start_position;
	iface->get_end_position = itext_editor_get_end_position;
}

static const gchar *
idocument_get_filename (IAnjutaDocument *editor, GError **e)
{
	return (TEXT_EDITOR (editor))->filename;
}

static gboolean
idocument_can_undo(IAnjutaDocument* editor, GError **e)
{
	return text_editor_can_undo(TEXT_EDITOR(editor));
}

static gboolean
idocument_can_redo(IAnjutaDocument *editor, GError **e)
{
	return text_editor_can_redo(TEXT_EDITOR(editor));
}

static void 
idocument_undo(IAnjutaDocument* te, GError** ee)
{
	text_editor_command(TEXT_EDITOR(te), ANE_UNDO, 0, 0);
}

static void 
idocument_begin_undo_action (IAnjutaDocument* te, GError** ee)
{
	scintilla_send_message (SCINTILLA (TEXT_EDITOR (te)->scintilla),
							SCI_BEGINUNDOACTION, 0, 0);
}

static void 
idocument_end_undo_action (IAnjutaDocument* te, GError** ee)
{
	scintilla_send_message (SCINTILLA (TEXT_EDITOR (te)->scintilla),
							SCI_ENDUNDOACTION, 0, 0);
}

static void 
idocument_redo(IAnjutaDocument* te, GError** ee)
{
	text_editor_command(TEXT_EDITOR(te), ANE_REDO, 0, 0);
}

static void
idocument_grab_focus (IAnjutaDocument *editor, GError **e)
{
	text_editor_grab_focus (TEXT_EDITOR (editor));
}

static void 
idocument_cut(IAnjutaDocument * te, GError** ee)
{
	text_editor_command(TEXT_EDITOR(te), ANE_CUT, 0, 0);
}

static void 
idocument_copy(IAnjutaDocument * te, GError** ee)
{
	text_editor_command(TEXT_EDITOR(te), ANE_COPY, 0, 0);
}

static void 
idocument_paste(IAnjutaDocument * te, GError** ee)
{
	text_editor_command(TEXT_EDITOR(te), ANE_PASTE, 0, 0);
}

static void 
idocument_clear(IAnjutaDocument * te, GError** ee)
{
	text_editor_command(TEXT_EDITOR(te), ANE_CLEAR, 0, 0);
}

static void
idocument_iface_init(IAnjutaDocumentIface* iface)
{
	iface->get_filename = idocument_get_filename;
	iface->can_undo = idocument_can_undo;
	iface->can_redo = idocument_can_redo;
	iface->undo = idocument_undo;
	iface->redo = idocument_redo;
	iface->begin_undo_action = idocument_begin_undo_action;
	iface->end_undo_action = idocument_end_undo_action;
	iface->grab_focus = idocument_grab_focus;
	iface->cut = idocument_cut;
	iface->copy = idocument_copy;
	iface->paste = idocument_paste;
	iface->clear = idocument_clear;
}

/* IAnjutaEditorSelection implementation */

static gchar*
iselection_get (IAnjutaEditorSelection *editor, GError **error)
{
	return text_editor_get_selection (TEXT_EDITOR (editor));
}

static void
iselection_set (IAnjutaEditorSelection* edit, 
				IAnjutaIterable* istart,
				IAnjutaIterable* iend,
				gboolean scroll, /* TODO: Is is possible to set this in scintilla? */
				GError** e)
{
	TextEditorCell* start = TEXT_EDITOR_CELL (istart);
	TextEditorCell* end = TEXT_EDITOR_CELL (iend);
	int start_pos = text_editor_cell_get_position (start);
	int end_pos = text_editor_cell_get_position (end);
	
	scintilla_send_message (SCINTILLA (TEXT_EDITOR (edit)->scintilla),
						    SCI_SETSEL, start_pos, end_pos);
}

static gboolean
iselection_has_selection (IAnjutaEditorSelection *editor, GError **e)
{
	return text_editor_has_selection (TEXT_EDITOR (editor));
}

static IAnjutaIterable*
iselection_get_start (IAnjutaEditorSelection *edit, GError **e)
{
	gint start =  scintilla_send_message (SCINTILLA(TEXT_EDITOR(edit)->scintilla),
										  SCI_GETSELECTIONSTART, 0, 0);
	gint end = scintilla_send_message (SCINTILLA(TEXT_EDITOR(edit)->scintilla),
									   SCI_GETSELECTIONEND, 0, 0);
	if (start != end)
	{
		return IANJUTA_ITERABLE (text_editor_cell_new (TEXT_EDITOR (edit), start));
	}
	else
		return NULL;
}

static IAnjutaIterable*
iselection_get_end (IAnjutaEditorSelection *edit, GError **e)
{
	gint start =  scintilla_send_message (SCINTILLA(TEXT_EDITOR(edit)->scintilla),
										  SCI_GETSELECTIONSTART, 0, 0);
	gint end = scintilla_send_message (SCINTILLA(TEXT_EDITOR(edit)->scintilla),
									   SCI_GETSELECTIONEND, 0, 0);
	if (start != end)
	{
		return IANJUTA_ITERABLE (text_editor_cell_new (TEXT_EDITOR (edit), end));
	}
	else
		return NULL;
}

static void
iselection_replace (IAnjutaEditorSelection *editor, const gchar *txt,
					gint length, GError **e)
{
	gchar *text_to_insert;
	if (length >= 0)
		text_to_insert = g_strndup (txt, length);
	else
		text_to_insert = g_strdup (txt);
	
	text_editor_replace_selection (TEXT_EDITOR (editor), text_to_insert);

	g_free (text_to_insert);
}

static void
iselection_select_all (IAnjutaEditorSelection* te, GError** ee)
{
	text_editor_command (TEXT_EDITOR (te), ANE_SELECTALL, 0, 0);
}

static void
iselection_select_block (IAnjutaEditorSelection *te, GError **e)
{
	text_editor_command (TEXT_EDITOR(te), ANE_SELECTBLOCK, 0, 0);
}

static void
iselection_select_function (IAnjutaEditorSelection *editor, GError **e)
{
	TextEditor* te = TEXT_EDITOR(editor);
	gint pos;
	gint line;
	gint fold_level;
	gint start, end;	
	gint line_count;
	gint tmp;

	line_count = scintilla_send_message (SCINTILLA (te->scintilla), 
										 SCI_GETLINECOUNT, 0, 0);
	pos = scintilla_send_message (SCINTILLA (te->scintilla), 
								  SCI_GETCURRENTPOS, 0, 0);
	line = scintilla_send_message (SCINTILLA (te->scintilla),
								   SCI_LINEFROMPOSITION, pos, 0);

	tmp = line + 1;	
	fold_level = scintilla_send_message (SCINTILLA (te->scintilla), 
										 SCI_GETFOLDLEVEL, line, 0) ;	
	if ((fold_level & 0xFF) != 0)
	{
		while ((fold_level & 0x10FF) != 0x1000 && line >= 0)
			fold_level = scintilla_send_message (SCINTILLA (te->scintilla), 
												 SCI_GETFOLDLEVEL, --line, 0) ;
		start = scintilla_send_message (SCINTILLA (te->scintilla), 
										SCI_POSITIONFROMLINE, line + 1, 0);
		line = tmp;
		fold_level = scintilla_send_message (SCINTILLA (te->scintilla), 
											 SCI_GETFOLDLEVEL, line, 0) ;
		while ((fold_level & 0x10FF) != 0x1000 && line < line_count)
			fold_level = scintilla_send_message (SCINTILLA (te->scintilla), 
												 SCI_GETFOLDLEVEL, ++line, 0) ;

		end = scintilla_send_message (SCINTILLA (te->scintilla), 
									  SCI_POSITIONFROMLINE, line , 0);
		scintilla_send_message (SCINTILLA (te->scintilla), 
								SCI_SETSEL, start, end) ;
	}
}

static void
iselection_iface_init (IAnjutaEditorSelectionIface *iface)
{
	iface->has_selection = iselection_has_selection;
	iface->get = iselection_get;
	iface->set = iselection_set;
	iface->get_start = iselection_get_start;
	iface->get_end = iselection_get_end;
	iface->replace = iselection_replace;
	iface->select_all = iselection_select_all;
	iface->select_block = iselection_select_block;
	iface->select_function = iselection_select_function;
}

/* IAnjutaFile implementation */

static GFile*
ifile_get_file (IAnjutaFile *editor, GError **error)
{
	TextEditor *text_editor;
	text_editor = TEXT_EDITOR(editor);
	if (text_editor->uri)
		return g_file_new_for_uri (text_editor->uri);
	else
		return NULL;
}

static void
ifile_open (IAnjutaFile *editor, GFile* file, GError **error)
{
	/* Close current file and open new file in this editor */
	TextEditor* text_editor;
	text_editor = TEXT_EDITOR(editor);

	/* Remove path */
	g_free (text_editor->uri);
	text_editor->uri = g_file_get_uri (file);
	g_free (text_editor->filename);
	text_editor->filename = g_file_get_basename (file);
	text_editor_load_file (text_editor);
}

static void
isaveable_save (IAnjutaFileSavable* editor, GError** e)
{
	TextEditor *text_editor = TEXT_EDITOR(editor);

	if (text_editor->uri != NULL)
		text_editor_save_file(text_editor, FALSE);
	else
		g_signal_emit_by_name (G_OBJECT (text_editor), "saved", NULL);
}

static void
isavable_save_as (IAnjutaFileSavable* editor, GFile* file, GError** e)
{
	const gchar *past_language;
	const gchar *curr_language;
	TextEditor *text_editor = TEXT_EDITOR(editor);
	
	past_language =
		ianjuta_editor_language_get_language (IANJUTA_EDITOR_LANGUAGE (text_editor),
											  NULL);
	
	/* Remove path */
	g_free (text_editor->uri);
	text_editor->uri = g_file_get_uri (file);
	g_free (text_editor->filename);
	text_editor->filename = g_file_get_basename (file);
	text_editor_save_file (text_editor, FALSE);
	text_editor_set_hilite_type (text_editor, NULL);
	text_editor_hilite (text_editor, FALSE);
	
	/* We have to take care of 'language-change' signal ourself because
	 * text_editor_set_hilite_type() only emits it for forced hilite type
	 */
	curr_language =
		ianjuta_editor_language_get_language (IANJUTA_EDITOR_LANGUAGE (text_editor),
											  NULL);
	if (past_language != curr_language)
		g_signal_emit_by_name (text_editor, "language-changed", curr_language);
	
}

static gboolean
isavable_is_dirty (IAnjutaFileSavable* editor, GError** e)
{
	TextEditor *text_editor = TEXT_EDITOR(editor);
	return !text_editor_is_saved (text_editor);
}

static void
isavable_set_dirty (IAnjutaFileSavable* editor, gboolean dirty, GError** e)
{
	TextEditor *text_editor = TEXT_EDITOR(editor);
	text_editor_set_saved (text_editor, !dirty);
}

static gboolean
isavable_is_read_only (IAnjutaFileSavable* savable, GError** e)
{
	/* FIXME */
	return FALSE;
}

static gboolean
isavable_is_conflict (IAnjutaFileSavable* savable, GError** e)
{
	TextEditor *te = TEXT_EDITOR(savable);
	return  te->message_area != NULL;
}

static void
isavable_iface_init (IAnjutaFileSavableIface *iface)
{
	iface->save = isaveable_save;
	iface->save_as = isavable_save_as;
	iface->set_dirty = isavable_set_dirty;
	iface->is_dirty = isavable_is_dirty;
	iface->is_read_only = isavable_is_read_only;
	iface->is_conflict = isavable_is_conflict;
}

static void
ifile_iface_init (IAnjutaFileIface *iface)
{
	iface->open = ifile_open;
	iface->get_file = ifile_get_file;
}

/* Implementation of the IAnjutaMarkable interface */

static gint
marker_ianjuta_to_editor (IAnjutaMarkableMarker marker)
{
	gint mark;
	switch (marker)
	{
		case IANJUTA_MARKABLE_LINEMARKER:
			mark = TEXT_EDITOR_LINEMARKER;
			break;
		case IANJUTA_MARKABLE_BOOKMARK:
			mark = TEXT_EDITOR_BOOKMARK;
			break;
		case IANJUTA_MARKABLE_BREAKPOINT_DISABLED:
			mark = TEXT_EDITOR_BREAKPOINT_DISABLED;
			break;
		case IANJUTA_MARKABLE_BREAKPOINT_ENABLED:
			mark = TEXT_EDITOR_BREAKPOINT_ENABLED;
			break;
		case IANJUTA_MARKABLE_PROGRAM_COUNTER:
			mark = TEXT_EDITOR_PROGRAM_COUNTER;
			break;
		default:
			mark = TEXT_EDITOR_LINEMARKER;
	}
	return mark;
}

static gint
imarkable_mark (IAnjutaMarkable* editor, gint location,
				IAnjutaMarkableMarker marker, GError** e)
{
	return text_editor_set_marker (TEXT_EDITOR (editor), location,
								   marker_ianjuta_to_editor (marker));
}

static gint
imarkable_location_from_handle (IAnjutaMarkable* editor, gint handle, GError** e)
{
	return text_editor_line_from_handle (TEXT_EDITOR (editor), handle);
}

static void
imarkable_unmark (IAnjutaMarkable* editor, gint location,
				  IAnjutaMarkableMarker marker, GError** e)
{
	text_editor_delete_marker (TEXT_EDITOR (editor), location,
							   marker_ianjuta_to_editor (marker));
}

static gboolean
imarkable_is_marker_set (IAnjutaMarkable* editor, gint location,
						 IAnjutaMarkableMarker marker, GError** e)
{
	return text_editor_is_marker_set (TEXT_EDITOR (editor), location,
									  marker_ianjuta_to_editor (marker));
}

static void
imarkable_delete_all_markers (IAnjutaMarkable* editor,
							  IAnjutaMarkableMarker marker, GError** e)
{
	text_editor_delete_marker_all (TEXT_EDITOR (editor),
								   marker_ianjuta_to_editor (marker));
}

static void
imarkable_iface_init (IAnjutaMarkableIface *iface)
{
	iface->mark = imarkable_mark;
	iface->location_from_handle = imarkable_location_from_handle;
	iface->unmark = imarkable_unmark;
	iface->is_marker_set = imarkable_is_marker_set;
	iface->delete_all_markers = imarkable_delete_all_markers;
}

/* IAnjutaEditorConvert implementation */

static void
iconvert_to_upper (IAnjutaEditorConvert* te, IAnjutaIterable *start_position,
				   IAnjutaIterable *end_position, GError** ee)
{
	gint start, end;
	start = text_editor_cell_get_position (TEXT_EDITOR_CELL (start_position));
	end = text_editor_cell_get_position (TEXT_EDITOR_CELL (end_position));
	scintilla_send_message (SCINTILLA (TEXT_EDITOR(te)->scintilla),
							SCI_SETSEL, start, end);
	text_editor_command (TEXT_EDITOR(te), ANE_UPRCASE, 0, 0);
}

static void
iconvert_to_lower(IAnjutaEditorConvert* te, IAnjutaIterable *start_position,
				  IAnjutaIterable *end_position, GError** ee)
{
	gint start, end;
	start = text_editor_cell_get_position (TEXT_EDITOR_CELL (start_position));
	end = text_editor_cell_get_position (TEXT_EDITOR_CELL (end_position));
	scintilla_send_message (SCINTILLA (TEXT_EDITOR(te)->scintilla),
							SCI_SETSEL, start, end);
	text_editor_command (TEXT_EDITOR (te), ANE_LWRCASE, 0, 0);	
}

static void
iconvert_iface_init (IAnjutaEditorConvertIface *iface)
{
	iface->to_lower = iconvert_to_lower;
	iface->to_upper = iconvert_to_upper;
}

/* IAnjutaEditorLineMode implementation */

static IAnjutaEditorLineModeType
ilinemode_get (IAnjutaEditorLineMode* te, GError** err)
{
	glong eolmode;
	IAnjutaEditorLineModeType retmode;
	
	g_return_val_if_fail (IS_TEXT_EDITOR (te), IANJUTA_EDITOR_LINE_MODE_LF);
	
	eolmode = scintilla_send_message (SCINTILLA (TEXT_EDITOR (te)->scintilla),
									  SCI_GETEOLMODE, 0, 0);
	
	switch (eolmode) {
		case SC_EOL_CR:
			retmode = IANJUTA_EDITOR_LINE_MODE_CR;
		break;
		case SC_EOL_CRLF:
			retmode = IANJUTA_EDITOR_LINE_MODE_CRLF;
		break;
		case SC_EOL_LF:
			retmode = IANJUTA_EDITOR_LINE_MODE_LF;
		break;
		default:
			retmode = IANJUTA_EDITOR_LINE_MODE_LF;
			g_warning ("Should not be here");
	}
	return retmode;
}

static void
ilinemode_set (IAnjutaEditorLineMode* te, IAnjutaEditorLineModeType mode,
			   GError** err)
{
	g_return_if_fail (IS_TEXT_EDITOR (te));
	
	switch (mode)
	{
		case IANJUTA_EDITOR_LINE_MODE_LF:
			text_editor_command(TEXT_EDITOR(te), ANE_EOL_LF, 0, 0);
		break;
		
		case IANJUTA_EDITOR_LINE_MODE_CR:
			text_editor_command(TEXT_EDITOR(te), ANE_EOL_CR, 0, 0);
		break;
		
		case IANJUTA_EDITOR_LINE_MODE_CRLF:
			text_editor_command(TEXT_EDITOR(te), ANE_EOL_CRLF, 0, 0);
		break;
		
		default:
			g_warning ("Should not reach here");
		break;
	}
}

static void
ilinemode_convert (IAnjutaEditorLineMode *te, IAnjutaEditorLineModeType mode,
				   GError **err)
{
	switch (mode)
	{
		case IANJUTA_EDITOR_LINE_MODE_LF:
			text_editor_command (TEXT_EDITOR (te), ANE_EOL_CONVERT,
								ANE_EOL_LF, 0);
		break;
		
		case IANJUTA_EDITOR_LINE_MODE_CR:
			text_editor_command (TEXT_EDITOR (te), ANE_EOL_CONVERT,
								 ANE_EOL_CR, 0);
		break;
		
		case IANJUTA_EDITOR_LINE_MODE_CRLF:
			text_editor_command (TEXT_EDITOR (te), ANE_EOL_CONVERT,
								 ANE_EOL_CRLF, 0);
		break;
		
		default:
			g_warning ("Should not reach here");
		break;
	}
}

static void
ilinemode_fix (IAnjutaEditorLineMode* te, GError** err)
{
	IAnjutaEditorLineModeType mode = ilinemode_get (te, NULL);
	ilinemode_convert (te, mode, NULL);
}

static void
ilinemode_iface_init (IAnjutaEditorLineModeIface *iface)
{
	iface->set = ilinemode_set;
	iface->get = ilinemode_get;
	iface->convert = ilinemode_convert;
	iface->fix = ilinemode_fix;
}

/* IAnjutaEditorAssist implementation */

static void
iassist_suggest (IAnjutaEditorAssist *iassist, GList* choices,
				 IAnjutaIterable *position, int char_alignment, GError **err)
{
	GString *words;
	GList *choice;
	TextEditor *te = TEXT_EDITOR (iassist);
	
	g_return_if_fail (IS_TEXT_EDITOR (te));
					 
	if (!choices)
	{
		scintilla_send_message (SCINTILLA (te->scintilla), SCI_AUTOCCANCEL,
								0, 0);
		scintilla_send_message (SCINTILLA (te->scintilla), SCI_CALLTIPCANCEL,
								0, 0);
		return;
	}
	
	words = g_string_sized_new (256);
	
	choice = choices;
	while (choice)
	{
		if (choice->data)
		{
			if (words->len > 0)
				g_string_append_c (words, ' ');
			g_string_append(words, choice->data);
		}
		choice = g_list_next (choice);
	}
	scintilla_send_message (SCINTILLA (te->scintilla),
							SCI_AUTOCSETAUTOHIDE, 1, 0);
	scintilla_send_message (SCINTILLA (te->scintilla),
							SCI_AUTOCSETDROPRESTOFWORD, 1, 0);
	scintilla_send_message (SCINTILLA (te->scintilla),
							SCI_AUTOCSETCANCELATSTART, 0, 0);
	scintilla_send_message (SCINTILLA (te->scintilla),
							SCI_AUTOCSETCHOOSESINGLE, 0, 0);
						 
	if (char_alignment == 0)
		scintilla_send_message (SCINTILLA (te->scintilla),
								SCI_USERLISTSHOW, 1 /* dummy */,
								(uptr_t) words->str);
	else
		scintilla_send_message (SCINTILLA (te->scintilla),
								SCI_AUTOCSHOW, char_alignment,
								(uptr_t) words->str);
	g_string_free (words, TRUE);
}

static GList*
iassist_get_suggestions (IAnjutaEditorAssist *iassist, const gchar *context, GError **err)
{
	/* FIXME: Implement new IAnjutaEditorAssist interface */

	return NULL;
}

static void 
iassist_show_tips (IAnjutaEditorAssist *iassist, GList* tips,
				   IAnjutaIterable *position, gint char_alignment, GError **err)
{
	gint lineno, cur_pos, cur_col, real_pos, real_col;
	GString *calltip;
	GList *tip;
	gint tips_count;
	TextEditor *te = TEXT_EDITOR (iassist);
	
	g_return_if_fail (IS_TEXT_EDITOR (te));
	g_return_if_fail (tips != NULL);
	tips_count = g_list_length (tips);
	g_return_if_fail (tips_count > 0);
	
	DEBUG_PRINT ("Number of calltips found %d\n", tips_count);
	
	calltip = g_string_sized_new (256);

	tip = tips;
	while (tip)
	{
		if (calltip->len > 0)
			g_string_append_c (calltip, '\n');
		g_string_append (calltip, (gchar*) tip->data);
		tip = g_list_next (tip);
	}
/*	
	if (tips_count > 1)
	{
		g_string_prepend_c (calltip, '\001');
		g_string_append_c (calltip, '\002');
	}
*/
	/* Calculate real calltip position */
	cur_pos = scintilla_send_message (SCINTILLA (te->scintilla),
									 SCI_GETCURRENTPOS, 0, 0);
	lineno = scintilla_send_message (SCINTILLA (te->scintilla),
									 SCI_LINEFROMPOSITION, cur_pos, 0);
	cur_col = scintilla_send_message (SCINTILLA (te->scintilla),
									 SCI_GETCOLUMN, cur_pos, 0);
	real_col = cur_col - char_alignment;
	if (real_col < 0)
		real_col = 0;
	real_pos = scintilla_send_message (SCINTILLA (te->scintilla),
									   SCI_FINDCOLUMN, lineno, real_col);
	scintilla_send_message (SCINTILLA (te->scintilla),
							SCI_CALLTIPSHOW,
							real_pos,
							(uptr_t) calltip->str);
	g_string_free (calltip, TRUE);
	/* ContinueCallTip_new(); */
}

static void
iassist_cancel_tips (IAnjutaEditorAssist *iassist, GError **err)
{
	TextEditor *te = TEXT_EDITOR (iassist);
	scintilla_send_message (SCINTILLA (te->scintilla), SCI_CALLTIPCANCEL, 0, 0);
}

static void
iassist_hide_suggestions (IAnjutaEditorAssist *iassist, GError **err)
{
	TextEditor *te = TEXT_EDITOR (iassist);
	
	g_return_if_fail (IS_TEXT_EDITOR (te));
	scintilla_send_message (SCINTILLA (te->scintilla), SCI_AUTOCCANCEL, 0, 0);
}

static void
iassist_iface_init(IAnjutaEditorAssistIface* iface)
{
	iface->get_suggestions = iassist_get_suggestions;
	iface->suggest = iassist_suggest;
	iface->hide_suggestions = iassist_hide_suggestions;
	iface->show_tips = iassist_show_tips;
	iface->cancel_tips = iassist_cancel_tips;
}

/* IAnutaEditorFolds implementation */

static void
ifolds_open_all(IAnjutaEditorFolds* view, GError **e)
{
	text_editor_command(TEXT_EDITOR(view), ANE_OPEN_FOLDALL, 0, 0);
}

static void
ifolds_close_all(IAnjutaEditorFolds* view, GError **e)
{
	text_editor_command(TEXT_EDITOR(view), ANE_CLOSE_FOLDALL, 0, 0);
}

static void
ifolds_toggle_current(IAnjutaEditorFolds* view, GError **e)
{
	text_editor_command(TEXT_EDITOR(view), ANE_TOGGLE_FOLD, 0, 0);
}

static void
ifolds_iface_init(IAnjutaEditorFoldsIface* iface)
{
	iface->open_all = ifolds_open_all;
	iface->close_all = ifolds_close_all;
	iface->toggle_current = ifolds_toggle_current;
}

/* IAnjutaEditorView implementation */
static void
iview_create (IAnjutaEditorView *view, GError **err)
{
	g_return_if_fail (IS_TEXT_EDITOR (view));
	text_editor_add_view (TEXT_EDITOR (view));
}

static void
iview_remove_current (IAnjutaEditorView *view, GError **err)
{
	g_return_if_fail (IS_TEXT_EDITOR (view));
	text_editor_remove_view (TEXT_EDITOR (view));
}

static gint
iview_get_count (IAnjutaEditorView *view, GError **err)
{
	g_return_val_if_fail (IS_TEXT_EDITOR (view), -1);
	return g_list_length (TEXT_EDITOR (view)->views);
}

static void
iview_iface_init (IAnjutaEditorViewIface *iface)
{
	iface->create = iview_create;
	iface->remove_current = iview_remove_current;
	iface->get_count = iview_get_count;
}

static void
iindicable_set (IAnjutaIndicable *te, IAnjutaIterable *begin_location,
				IAnjutaIterable *end_location,
				IAnjutaIndicableIndicator indicator, GError **err)
{
	gint begin = text_editor_cell_get_position (TEXT_EDITOR_CELL (begin_location));
	gint end = text_editor_cell_get_position (TEXT_EDITOR_CELL (end_location));
	switch (indicator)
	{
		case IANJUTA_INDICABLE_NONE:
			text_editor_set_indicator (TEXT_EDITOR (te), begin, end, -1);
		break;
		case IANJUTA_INDICABLE_IMPORTANT:
			text_editor_set_indicator (TEXT_EDITOR (te), begin, end, 0);
		break;
		case IANJUTA_INDICABLE_WARNING:
			text_editor_set_indicator (TEXT_EDITOR (te),  begin, end, 1);
		break;
		case IANJUTA_INDICABLE_CRITICAL:
			text_editor_set_indicator (TEXT_EDITOR (te),  begin, end, 2);
		break;
		default:
			g_warning ("Unsupported indicator %d", indicator);
			text_editor_set_indicator (TEXT_EDITOR (te),  begin, end, -1);
		break;
	}
}

static void
iindicable_clear (IAnjutaIndicable *te, GError **err)
{
	text_editor_set_indicator (TEXT_EDITOR (te), -1, -1, -1);
}

static void
iindicable_iface_init (IAnjutaIndicableIface *iface)
{
	iface->set = iindicable_set;
	iface->clear = iindicable_clear;
}

static void
iprint_print(IAnjutaPrint* print, GError** e)
{
	TextEditor* te = TEXT_EDITOR(print);
	anjuta_print(FALSE, te->preferences, te);
}

static void
iprint_preview(IAnjutaPrint* print, GError** e)
{
	TextEditor* te = TEXT_EDITOR(print);
	anjuta_print(TRUE, te->preferences, te);
}

static void
iprint_iface_init(IAnjutaPrintIface* iface)
{
	iface->print = iprint_print;
	iface->print_preview = iprint_preview;
}

static void
icomment_block(IAnjutaEditorComment* comment, GError** e)
{
	TextEditor* te = TEXT_EDITOR(comment);
    aneditor_command (te->editor_id, ANE_BLOCKCOMMENT, 0, 0);
}

static void
icomment_stream(IAnjutaEditorComment* comment, GError** e)
{
	TextEditor* te = TEXT_EDITOR(comment);
    aneditor_command (te->editor_id, ANE_STREAMCOMMENT, 0, 0);
}

static void
icomment_box(IAnjutaEditorComment* comment, GError** e)
{
	TextEditor* te = TEXT_EDITOR(comment);
    aneditor_command (te->editor_id, ANE_BOXCOMMENT, 0, 0);
}

static void
icomment_iface_init(IAnjutaEditorCommentIface* iface)
{
	iface->block = icomment_block;
	iface->box = icomment_box;
	iface->stream = icomment_stream;
}

#define MAX_ZOOM_FACTOR 8
#define MIN_ZOOM_FACTOR -8

static void
izoom_in(IAnjutaEditorZoom* zoom, GError** e)
{
	TextEditor* te = TEXT_EDITOR(zoom);
	gint zoom_factor = anjuta_preferences_get_int (te->preferences,
												   TEXT_ZOOM_FACTOR) + 1;
	
	if (zoom_factor > MAX_ZOOM_FACTOR)
		zoom_factor = MAX_ZOOM_FACTOR;
	else if (zoom_factor < MIN_ZOOM_FACTOR)
		zoom_factor = MIN_ZOOM_FACTOR;
	
	anjuta_preferences_set_int (te->preferences, TEXT_ZOOM_FACTOR, zoom_factor);
}

static void
izoom_out(IAnjutaEditorZoom* zoom, GError** e)
{
	TextEditor* te = TEXT_EDITOR(zoom);
	gint zoom_factor = anjuta_preferences_get_int (te->preferences,
												   TEXT_ZOOM_FACTOR) - 1;
	
	if (zoom_factor > MAX_ZOOM_FACTOR)
		zoom_factor = MAX_ZOOM_FACTOR;
	else if (zoom_factor < MIN_ZOOM_FACTOR)
		zoom_factor = MIN_ZOOM_FACTOR;
	
	anjuta_preferences_set_int (te->preferences, TEXT_ZOOM_FACTOR, zoom_factor);
}

static void
izoom_iface_init(IAnjutaEditorZoomIface* iface)
{
	iface->in = izoom_in;
	iface->out = izoom_out;
}

static void
igoto_start_block(IAnjutaEditorGoto* editor, GError** e)
{
	TextEditor* te = TEXT_EDITOR(editor);
	text_editor_goto_block_start(te);
}

static void
igoto_end_block(IAnjutaEditorGoto* editor, GError** e)
{
	TextEditor* te = TEXT_EDITOR(editor);
	text_editor_goto_block_end(te);
}

static void
igoto_matching_brace(IAnjutaEditorGoto* editor, GError** e)
{
	TextEditor* te = TEXT_EDITOR(editor);
	text_editor_command (te, ANE_MATCHBRACE, 0, 0);
}

static void
igoto_iface_init(IAnjutaEditorGotoIface* iface)
{
	iface->start_block = igoto_start_block;
	iface->end_block = igoto_end_block;
	iface->matching_brace = igoto_matching_brace;
}

static const GList*
ilanguage_get_supported_languages (IAnjutaEditorLanguage *ilanguage,
								   GError **err)
{
	if (supported_languages == NULL)
	{
		gchar **strv;
		gchar **token;
		gchar *menu_entries;
		
		supported_languages_name =
			g_hash_table_new_full (g_str_hash, g_str_equal,
								   NULL, g_free);
		supported_languages_ext =
			g_hash_table_new_full (g_str_hash, g_str_equal,
								   NULL, g_free);
		
		supported_languages_by_lexer =
			g_hash_table_new_full (g_str_hash, g_str_equal,
								   g_free, NULL);
			
		menu_entries = sci_prop_get (text_editor_get_props (), "menu.language");
		g_return_val_if_fail (menu_entries != NULL, NULL);
		
		strv = g_strsplit (menu_entries, "|", -1);
		token = strv;
		while (*token)
		{
			gchar *lexer;
			gchar *possible_file;
			gchar *iter;
			gchar *name, *extension;
			GString *lang;
			
			lang = g_string_new ("");
			
			name = *token++;
			if (!name)
				break;
			
			extension = *token++;
			if (!extension)
				break;
			token++;
			
			if (name[0] == '#')
				continue;
			
			iter = name;
			while (*iter)
			{
				if (*iter == '&')
				{
					*iter = '_';
				}
				else
				{
					g_string_append_c (lang, g_ascii_tolower (*iter));
				}
				iter++;
			}
			
			/* HACK: Convert the weird c++ name to cpp */
			if (strcmp (lang->str, "c / c++") == 0)
			{
				g_string_assign (lang, "cpp");
			}
			
			/* Updated mapping hash tables */
			g_hash_table_insert (supported_languages_name, lang->str,
								 g_strdup (name));
			g_hash_table_insert (supported_languages_ext, lang->str,
								 g_strconcat ("file.", extension, NULL));
			/* Map lexer to language */
			possible_file = g_strconcat ("file.", extension, NULL);
			lexer = sci_prop_get_new_expand (TEXT_EDITOR (ilanguage)->props_base,
											 "lexer.", possible_file);
			g_free (possible_file);
			if (lexer)
			{
				/* We only map the first (which is hopefully the true) language */
				if (!g_hash_table_lookup (supported_languages_by_lexer, lexer))
				{
					/* DEBUG_PRINT ("Mapping (lexer)%s to (language)%s", lexer, lang->str); */
					g_hash_table_insert (supported_languages_by_lexer,
										 lexer, lang->str);
					/* lexer is taken in the hash, so no free */
				}
				else
				{
					g_free (lexer);
				}
				lexer = NULL;
			}
			supported_languages = g_list_prepend (supported_languages,
												  lang->str);
			g_string_free (lang, FALSE);
		}
		g_strfreev (strv);
	}
	return supported_languages;
}

static const gchar*
ilanguage_get_language_name (IAnjutaEditorLanguage *ilanguage,
							 const gchar *language, GError **err)
{
	if (!supported_languages_name)
		ilanguage_get_supported_languages (ilanguage, NULL);
	
	return g_hash_table_lookup (supported_languages_name, language);
}

static void
ilanguage_set_language (IAnjutaEditorLanguage *ilanguage,
						const gchar *language, GError **err)
{
	if (!supported_languages_ext)
		ilanguage_get_supported_languages (ilanguage, NULL);

	if (language)
		text_editor_set_hilite_type (TEXT_EDITOR (ilanguage),
									 g_hash_table_lookup (supported_languages_ext,
														  language));
	else /* Autodetect */
		text_editor_set_hilite_type (TEXT_EDITOR (ilanguage), NULL);
		
	text_editor_hilite (TEXT_EDITOR (ilanguage), FALSE);
}

static const gchar*
ilanguage_get_language (IAnjutaEditorLanguage *ilanguage, GError **err)
{
	const gchar *language = NULL;
	const gchar *filename = NULL;
	TextEditor *te = TEXT_EDITOR (ilanguage);
	
	if (te->force_hilite)
		filename = te->force_hilite;
	else if (te->filename)
		filename = te->filename;
	
	if (filename)
	{
		gchar *lexer = NULL;
		lexer = sci_prop_get_new_expand (te->props_base,
										 "lexer.", filename);
		
		/* No lexer, no language */
		if (lexer)
		{
			if (!supported_languages_by_lexer)
				ilanguage_get_supported_languages (ilanguage, NULL);
			
			language = g_hash_table_lookup (supported_languages_by_lexer, lexer);
			/* DEBUG_PRINT ("Found (language)%s for (lexer)%s", language, lexer); */
			g_free (lexer);
		}
	}
	return language;
}

static void
ilanguage_iface_init (IAnjutaEditorLanguageIface *iface)
{
	iface->get_supported_languages = ilanguage_get_supported_languages;
	iface->get_language_name = ilanguage_get_language_name;
	iface->get_language = ilanguage_get_language;
	iface->set_language = ilanguage_set_language;
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
	TextEditor *te = TEXT_EDITOR (isearch);
	gint start = text_editor_cell_get_position (TEXT_EDITOR_CELL (istart));
	gint end = text_editor_cell_get_position (TEXT_EDITOR_CELL (iend));

	gint flags = 0;
	gint retval;
	
	if (case_sensitive)
	{
		flags = SCFIND_MATCHCASE;
	}
	
	struct TextToFind to_find;
	
	to_find.chrg.cpMin = start;
	to_find.chrg.cpMax = end;
	
	to_find.lpstrText = (gchar*) search;
	
	retval = scintilla_send_message (SCINTILLA (te->scintilla),
									 SCI_FINDTEXT, flags, (long) &to_find);
	if (retval == -1)
		return FALSE;
	else
	{
		*iresult_start = IANJUTA_EDITOR_CELL (text_editor_cell_new (te, to_find.chrgText.cpMin));
		*iresult_end = IANJUTA_EDITOR_CELL (text_editor_cell_new (te, to_find.chrgText.cpMax));
		return TRUE;
	}
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
	TextEditor *te = TEXT_EDITOR (isearch);
	gint end = text_editor_cell_get_position (TEXT_EDITOR_CELL (istart));
	gint start = text_editor_cell_get_position (TEXT_EDITOR_CELL (iend));

	gint flags = 0;
	gint retval;
	
	if (case_sensitive)
	{
		flags = SCFIND_MATCHCASE;
	}
	
	struct TextToFind to_find;
	
	to_find.chrg.cpMin = start;
	to_find.chrg.cpMax = end;
	
	to_find.lpstrText = (gchar*) search;
	
	retval = scintilla_send_message (SCINTILLA (te->scintilla), SCI_FINDTEXT, flags, (long) &to_find);
	if (retval == -1)
		return FALSE;
	else
	{
		*iresult_start = IANJUTA_EDITOR_CELL (text_editor_cell_new (te, to_find.chrgText.cpMin));
		*iresult_end = IANJUTA_EDITOR_CELL (text_editor_cell_new (te, to_find.chrgText.cpMax));
		return TRUE;
	}
}

static void
isearch_iface_init(IAnjutaEditorSearchIface* iface)
{
	iface->forward = isearch_forward;
	iface->backward = isearch_backward;
}

static void
ihover_display (IAnjutaEditorHover *ihover, IAnjutaIterable *pos,
				const gchar *info, GError **e)
{
	TextEditor *te = TEXT_EDITOR (ihover);
	gint position = text_editor_cell_get_position (TEXT_EDITOR_CELL (pos));
	g_return_if_fail (position >= 0);
	g_return_if_fail (info != NULL);
	text_editor_show_hover_tip (te, position, info);
}

static void
ihover_iface_init(IAnjutaEditorHoverIface* iface)
{
	iface->display = ihover_display;
}

ANJUTA_TYPE_BEGIN(TextEditor, text_editor, GTK_TYPE_VBOX);
ANJUTA_TYPE_ADD_INTERFACE(ifile, IANJUTA_TYPE_FILE);
ANJUTA_TYPE_ADD_INTERFACE(isavable, IANJUTA_TYPE_FILE_SAVABLE);
ANJUTA_TYPE_ADD_INTERFACE(idocument, IANJUTA_TYPE_DOCUMENT);
ANJUTA_TYPE_ADD_INTERFACE(itext_editor, IANJUTA_TYPE_EDITOR);
ANJUTA_TYPE_ADD_INTERFACE(ilinemode, IANJUTA_TYPE_EDITOR_LINE_MODE);
ANJUTA_TYPE_ADD_INTERFACE(iselection, IANJUTA_TYPE_EDITOR_SELECTION);
ANJUTA_TYPE_ADD_INTERFACE(iconvert, IANJUTA_TYPE_EDITOR_CONVERT);
ANJUTA_TYPE_ADD_INTERFACE(iassist, IANJUTA_TYPE_EDITOR_ASSIST);
ANJUTA_TYPE_ADD_INTERFACE(ilanguage, IANJUTA_TYPE_EDITOR_LANGUAGE);
ANJUTA_TYPE_ADD_INTERFACE(iview, IANJUTA_TYPE_EDITOR_VIEW);
ANJUTA_TYPE_ADD_INTERFACE(ifolds, IANJUTA_TYPE_EDITOR_FOLDS);
ANJUTA_TYPE_ADD_INTERFACE(imarkable, IANJUTA_TYPE_MARKABLE);
ANJUTA_TYPE_ADD_INTERFACE(iindicable, IANJUTA_TYPE_INDICABLE);
ANJUTA_TYPE_ADD_INTERFACE(iprint, IANJUTA_TYPE_PRINT);
ANJUTA_TYPE_ADD_INTERFACE(icomment, IANJUTA_TYPE_EDITOR_COMMENT);
ANJUTA_TYPE_ADD_INTERFACE(izoom, IANJUTA_TYPE_EDITOR_ZOOM);
ANJUTA_TYPE_ADD_INTERFACE(igoto, IANJUTA_TYPE_EDITOR_GOTO);
ANJUTA_TYPE_ADD_INTERFACE(isearch, IANJUTA_TYPE_EDITOR_SEARCH);
ANJUTA_TYPE_ADD_INTERFACE(ihover, IANJUTA_TYPE_EDITOR_HOVER);
ANJUTA_TYPE_END;
