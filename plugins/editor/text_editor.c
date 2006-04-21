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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <gnome.h>
#include <libgnomevfs/gnome-vfs.h>
#include <errno.h>

#include <libanjuta/resources.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-encodings.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>
#include <libanjuta/interfaces/ianjuta-editor-convert.h>
#include <libanjuta/interfaces/ianjuta-editor-line-mode.h>
#include <libanjuta/interfaces/ianjuta-editor-assist.h>
#include <libanjuta/interfaces/ianjuta-editor-view.h>
#include <libanjuta/interfaces/ianjuta-editor-folds.h>
#include <libanjuta/interfaces/ianjuta-editor-comment.h>
#include <libanjuta/interfaces/ianjuta-editor-zoom.h>
#include <libanjuta/interfaces/ianjuta-bookmark.h>
#include <libanjuta/interfaces/ianjuta-editor-factory.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-savable.h>
#include <libanjuta/interfaces/ianjuta-markable.h>
#include <libanjuta/interfaces/ianjuta-indicable.h>
#include <libanjuta/interfaces/ianjuta-print.h>

#include "properties.h"
#include "text_editor.h"
#include "text_editor_cbs.h"
#include "text_editor_menu.h"
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

/* Marker 0 is defined for bookmarks */
#define TEXT_EDITOR_BOOKMARK                0
#define TEXT_EDITOR_LINEMARKER              1
#define TEXT_EDITOR_BREAKPOINT_DISABLED     2
#define TEXT_EDITOR_BREAKPOINT_ENABLED      3

#define MARKER_PROP_END 0xaaaaaa /* Do not define any color with this value */

/* marker fore and back colors */
static glong marker_prop[] = 
{
	SC_MARK_ROUNDRECT,      0x00007f, 0xffff80,	/* Bookmark */
	SC_MARK_SHORTARROW,     0x00007f, 0x00ffff,	/* Line mark */
	SC_MARK_CIRCLE,         0x00007f, 0xffffff,	/* Breakpoint mark (disabled) */
	SC_MARK_CIRCLE,         0x00007f, 0xff80ff,	/* Breakpoint mark (enabled) */
	MARKER_PROP_END,	                        /* end */
};

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
	te->gconf_notify_ids = NULL;
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
	gint i, marker;
	g_return_if_fail (te != NULL);
	
	marker = 0;
	for (i = 0;; i += 3)
	{
		if (marker_prop[i] == MARKER_PROP_END)
			break;
		scintilla_send_message (SCINTILLA (scintilla), SCI_MARKERDEFINE,
			marker, marker_prop[i+0]);
		scintilla_send_message (SCINTILLA (scintilla), SCI_MARKERSETFORE,
			marker, marker_prop[i+1]);
		scintilla_send_message (SCINTILLA (scintilla), SCI_MARKERSETBACK,
			marker, marker_prop[i+2]);
		marker++;
	}
}

#ifdef DEBUG
static void
on_scintila_already_destroyed (gpointer te, GObject *obj)
{
	DEBUG_PRINT ("Scintilla object has been destroyed");
}

static void
on_te_already_destroyed (gpointer te, GObject *obj)
{
	DEBUG_PRINT ("TextEditor object has been destroyed");
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
	
	gtk_widget_set_usize (scintilla, 50, 50);
	gtk_widget_show (scintilla);
	
	gtk_box_set_homogeneous (GTK_BOX (te), TRUE);
	gtk_box_set_spacing (GTK_BOX (te), 3);
	gtk_box_pack_start (GTK_BOX (te), scintilla, TRUE, TRUE, 0);
	gtk_widget_grab_focus (scintilla);

	g_signal_connect (G_OBJECT (scintilla), "event",
			    G_CALLBACK (on_text_editor_text_event), te);
	g_signal_connect (G_OBJECT (scintilla), "button_press_event",
			    G_CALLBACK (on_text_editor_text_buttonpress_event), te);
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
				G_CALLBACK (on_text_editor_scintilla_size_allocate), te);
	g_signal_handlers_disconnect_by_func (G_OBJECT (te->scintilla),
				G_CALLBACK (on_text_editor_scintilla_notify), te);
	g_signal_handlers_disconnect_by_func (G_OBJECT (te->scintilla),
				G_CALLBACK (on_text_editor_scintilla_focus_in), te);
	
	te->views = g_list_remove (te->views, GINT_TO_POINTER(te->editor_id));
	gtk_container_remove (GTK_CONTAINER (te), te->scintilla);
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
		gtk_box_set_spacing (GTK_BOX (te), 0);
		te->editor_id = 0;
		te->scintilla = NULL;
	}
}

static void
on_reload_dialog_response (GtkWidget *dlg, gint res, TextEditor *te)
{
	if (res == GTK_RESPONSE_YES)
	{
		text_editor_load_file (te);
	}
	gtk_widget_destroy (dlg);
}

static void
on_text_editor_uri_changed (GnomeVFSMonitorHandle *handle,
							const gchar *monitor_uri,
							const gchar *info_uri,
							GnomeVFSMonitorEventType event_type,
							gpointer user_data)
{
	GtkWidget *dlg;
	GtkWidget *parent;
	gchar *buff;
	TextEditor *te = TEXT_EDITOR (user_data);
	
	DEBUG_PRINT ("File changed!!!");
	
	if (!(event_type == GNOME_VFS_MONITOR_EVENT_CHANGED ||
		  event_type == GNOME_VFS_MONITOR_EVENT_CREATED))
		return;
	
	if (!anjuta_util_diff(te->uri, ianjuta_editor_get_text(IANJUTA_EDITOR(te),
													   0,
													   ianjuta_editor_get_length(IANJUTA_EDITOR(te), NULL),
													   NULL)))
		return;
	
	if (strcmp (monitor_uri, info_uri) != 0)
		return;
	
	buff =
		g_strdup_printf (_
						 ("The file '%s' on the disk is more recent than\n"
						  "the current buffer.\nDo you want to reload it?"),
						 te->filename);
	
	parent = gtk_widget_get_toplevel (GTK_WIDGET (te));
	
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
					  te);
	g_signal_connect_swapped (dlg, "delete-event",
					  G_CALLBACK (gtk_widget_destroy),
					  dlg);
	gtk_widget_show (dlg);
}

static void
text_editor_update_monitor (TextEditor *te, gboolean disable_it)
{
	if (te->monitor)
	{
		/* Shutdown existing monitor */
		gnome_vfs_monitor_cancel (te->monitor);
		te->monitor = NULL;
	}
	if (te->uri && !disable_it)
	{
		GnomeVFSResult res;
		DEBUG_PRINT ("Setting up Monitor for %s", te->uri);
		res = gnome_vfs_monitor_add (&te->monitor, te->uri,
									 GNOME_VFS_MONITOR_FILE,
									 on_text_editor_uri_changed, te);
		/*
		if (res != GNOME_VFS_OK)
		{
			DEBUG_PRINT ("Error while setting up file monitor: %s",
					   gnome_vfs_result_to_string (res));
		}
		*/
	}
}

GtkWidget *
text_editor_new (AnjutaStatus *status, AnjutaPreferences *eo, const gchar *uri, const gchar *name)
{
	static guint new_file_count;
	TextEditor *te = TEXT_EDITOR (gtk_widget_new (TYPE_TEXT_EDITOR, NULL));
	
	te->status = status; 
	
	te->preferences = eo;
	te->props_base = text_editor_get_props();
	if (name && strlen(name) > 0)
		te->filename = g_strdup(name); 
	else 
		te->filename = g_strdup_printf ("Newfile#%d", ++new_file_count);
	if (uri && strlen(uri) > 0)
	{
		GnomeVFSResult result;
		GnomeVFSURI* vfs_uri;
		GnomeVFSFileInfo info = {0,0};
		
		new_file_count--;
		if (te->filename)
			g_free (te->filename);
		if (te->uri)
			g_free (te->uri);
		vfs_uri = gnome_vfs_uri_new(uri);
		result = gnome_vfs_get_file_info_uri(vfs_uri, &info, GNOME_VFS_SET_FILE_INFO_NONE);
		gnome_vfs_uri_unref(vfs_uri); 
		te->filename = g_strdup(info.name);
		te->uri = g_strdup(uri);
	}
	
	text_editor_prefs_init (te);
	
	/* Create primary view */
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
	if (te->gconf_notify_ids)
	{
		text_editor_prefs_finalize (te);
		te->gconf_notify_ids = NULL;
	}
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (G_OBJECT(te)));
}

void
text_editor_finalize (GObject *obj)
{
	TextEditor *te = TEXT_EDITOR (obj);
	if (te->filename)
		g_free (te->filename);
	if (te->uri)
		g_free (te->uri);
	if (te->encoding)
		g_free (te->encoding);
	if (te->force_hilite)
		g_free (te->force_hilite);
	
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (G_OBJECT(te)));
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
	g_free (te->force_hilite);
	if (file_extension)
		te->force_hilite = g_strdup (file_extension);
	else
		te->force_hilite = NULL;
}

static void
text_editor_hilite_one (TextEditor * te, AnEditorID editor_id,
						gboolean override_by_pref)
{
	/* If syntax highlighting is disabled ... */
	if (override_by_pref &&
		anjuta_preferences_get_int (ANJUTA_PREFERENCES (te->preferences),
									DISABLE_SYNTAX_HILIGHTING))
	{
		aneditor_command (editor_id, ANE_SETHILITE, (guint) "plain.txt", 0);
	}
	else if (te->force_hilite)
	{
		aneditor_command (editor_id, ANE_SETHILITE, (guint) te->force_hilite, 0);
	}
	else if (te->uri)
	{
		gchar *basename;
		basename = g_path_get_basename (te->uri);
		aneditor_command (editor_id, ANE_SETHILITE, (guint) basename, 0);
		g_free (basename);
	}
	else if (te->filename)
	{
		aneditor_command (editor_id, ANE_SETHILITE, (guint) te->filename, 0);
	}
	else
	{
		aneditor_command (editor_id, ANE_SETHILITE, (guint) "plain.txt", 0);
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

	scintilla_send_message (SCINTILLA (te->scintilla), SCI_GOTOPOS,
				point, 0);
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
	
	g_return_val_if_fail (te != NULL, -1);
	g_return_val_if_fail (IS_SCINTILLA (te->scintilla) == TRUE, -1);

	if (start >= 0) {
		if (end <= start)
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
	} else {
		if (indicator < 0) {
			glong last;
			last = scintilla_send_message (SCINTILLA (te->scintilla),
										   SCI_GETTEXTLENGTH, 0, 0);
			if (last > 0) {
				scintilla_send_message (SCINTILLA (te->scintilla),
										SCI_STARTSTYLING, 0, INDICS_MASK);
				scintilla_send_message (SCINTILLA (te->scintilla),
										SCI_SETSTYLING, last, 0);
			}
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
static size_t
save_filtered_in_dos_mode(GnomeVFSHandle* vfs_write, gchar *data_,
						  GnomeVFSFileSize size)
{
	size_t i, j;
	unsigned char *data;
	unsigned char *tr_map;
	int k;

	/* build the translation table */
	tr_map = malloc( 256 );
	memset( tr_map, 0, 256 );
	for ( k = 0; k < sizeof(tr_dos)/2; k++)
	  tr_map[tr_dos[k].c] = tr_dos[k].b;

	data = (unsigned char*)data_;
	i = 0; j = 0;
	while ( i < size )
	{
		if (data[i]>=128) {
			/* convert dos-text */
			if ( tr_map[data[i]] != 0 )
			{	
				GnomeVFSFileSize bytes_written;
				gnome_vfs_write(vfs_write, &tr_map[data[i]], 1, &bytes_written);
				j += bytes_written;
			}
			else
			{
				/* char not found, skip transform */
				GnomeVFSFileSize bytes_written;
				gnome_vfs_write(vfs_write, &data[i], 1, &bytes_written);
				j += bytes_written;
			}
			i++;
		} 
		else 
		{
			GnomeVFSFileSize bytes_written;
			gnome_vfs_write(vfs_write, &data[i], 1, &bytes_written);
			j += bytes_written;
			i++;
		}
	}

	if ( tr_map )
		free(tr_map);
	return size;
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
	DEBUG_PRINT ("EOL chars: LR = %d, CR = %d, CRLF = %d", lf, cr, crlf);
	DEBUG_PRINT ("Autodetected Editor mode [%d]", mode);
	return mode;
}

static gchar *
convert_to_utf8_from_charset (const gchar *content,
							  gsize len,
							  const gchar *charset)
{
	gchar *utf8_content = NULL;
	GError *conv_error = NULL;
	gchar* converted_contents = NULL;
	gsize bytes_written;

	g_return_val_if_fail (content != NULL, NULL);

	DEBUG_PRINT ("Trying to convert from %s to UTF-8", charset);

	converted_contents = g_convert (content, len, "UTF-8",
									charset, NULL, &bytes_written,
									&conv_error); 
						
	if ((conv_error != NULL) || 
	    !g_utf8_validate (converted_contents, bytes_written, NULL))		
	{
		DEBUG_PRINT ("Couldn't convert from %s to UTF-8.", charset);
		
		if (converted_contents != NULL)
			g_free (converted_contents);

		if (conv_error != NULL)
		{
			g_error_free (conv_error);
			conv_error = NULL;
		}

		utf8_content = NULL;
	} else {
		DEBUG_PRINT ("Converted from %s to UTF-8.", charset);
		utf8_content = converted_contents;
	}
	return utf8_content;
}

static gchar *
convert_to_utf8 (PropsID props, const gchar *content, gsize len,
			     gchar **encoding_used)
{
	GList *encodings = NULL;
	GList *start;
	const gchar *locale_charset;
	GList *encoding_strings;
	
	g_return_val_if_fail (!g_utf8_validate (content, len, NULL), 
			g_strndup (content, len < 0 ? strlen (content) : len));

	encoding_strings = sci_prop_glist_from_data (props, SUPPORTED_ENCODINGS);
	encodings = anjuta_encoding_get_encodings (encoding_strings);
	anjuta_util_glist_strings_free (encoding_strings);
	
	if (g_get_charset (&locale_charset) == FALSE) 
	{
		const AnjutaEncoding *locale_encoding;

		/* not using a UTF-8 locale, so try converting
		 * from that first */
		if (locale_charset != NULL)
		{
			locale_encoding = anjuta_encoding_get_from_charset (locale_charset);
			encodings = g_list_prepend (encodings,
						(gpointer) locale_encoding);
			DEBUG_PRINT ("Current charset = %s", locale_charset);
		}
	}

	start = encodings;

	while (encodings != NULL) 
	{
		AnjutaEncoding *enc;
		const gchar *charset;
		gchar *utf8_content;

		enc = (AnjutaEncoding *) encodings->data;

		charset = anjuta_encoding_get_charset (enc);

		DEBUG_PRINT ("Trying to convert %d bytes of data into UTF-8.", len);
		
		fflush (stdout);
		utf8_content = convert_to_utf8_from_charset (content, len, charset);

		if (utf8_content != NULL) {
			if (encoding_used != NULL)
			{
				if (*encoding_used != NULL)
					g_free (*encoding_used);
				
				*encoding_used = g_strdup (charset);
			}

			return utf8_content;
		}

		encodings = encodings->next;
	}

	g_list_free (start);
	
	return NULL;
}

static gboolean
load_from_file (TextEditor *te, gchar *uri, gchar **err)
{
	GnomeVFSURI* vfs_uri;
	GnomeVFSHandle* vfs_read;
	GnomeVFSResult result;
	GnomeVFSFileInfo info;
	GnomeVFSFileSize nchars;

	gchar *buffer;
	gint dos_filter, editor_mode;

	scintilla_send_message (SCINTILLA (te->scintilla), SCI_CLEARALL,
							0, 0);
	vfs_uri = gnome_vfs_uri_new(uri);
	result = gnome_vfs_get_file_info_uri(vfs_uri, &info, GNOME_VFS_FILE_INFO_DEFAULT);
	if (result != GNOME_VFS_OK)
	{
		*err = g_strdup (_("Could not get file info"));
		return FALSE;
	}
	buffer = g_malloc (info.size);
	if (buffer == NULL && info.size != 0)
	{
		DEBUG_PRINT ("This file is too big. Unable to allocate memory.");
		*err = g_strdup (_("This file is too big. Unable to allocate memory."));
		return FALSE;
	}
	
	result = gnome_vfs_open_uri(&vfs_read, vfs_uri, GNOME_VFS_OPEN_READ); 
	if (result != GNOME_VFS_OK)
	{
		*err = g_strdup (_("Could not open file"));		
		return FALSE;
	}
	/* Crude way of loading, but faster */
 	result = gnome_vfs_read (vfs_read, buffer, info.size, &nchars);
	if (result != GNOME_VFS_OK && !(result == GNOME_VFS_ERROR_EOF && info.size == 0))
	{
		g_free(buffer);
		*err = g_strdup (_("Error while reading from file"));
		return FALSE;
	}

	if (info.size != nchars) DEBUG_PRINT ("File size and loaded size not matching");
	dos_filter = 
		anjuta_preferences_get_int (ANJUTA_PREFERENCES (te->preferences),
									DOS_EOL_CHECK);
	
	/* Set editor mode */
	editor_mode =  determine_editor_mode (buffer, nchars);
	scintilla_send_message (SCINTILLA (te->scintilla),
							SCI_SETEOLMODE, editor_mode, 0);

	DEBUG_PRINT ("Loaded in editor mode [%d]", editor_mode);

	/* Determine character encoding and convert to utf-8*/
	if (nchars > 0)
	{
		if (g_utf8_validate (buffer, nchars, NULL))
		{
			if (te->encoding)
				g_free (te->encoding);
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
				*err = g_strdup (_("The file does not look like a text file or the file encoding is not supported."
								   " Please check if the encoding of file is in the supported encodings list."
								   " If not, add it from the preferences."));
				gnome_vfs_close(vfs_read);
				return FALSE;
			}
			g_free (buffer);
			buffer = converted_text;
			nchars = strlen (converted_text);
		}
	}
	if (dos_filter && editor_mode == SC_EOL_CRLF){
		DEBUG_PRINT ("Filtering Extrageneous DOS characters in dos mode [Dos => Unix]");
		nchars = filter_chars_in_dos_mode( buffer, nchars );
	}
	scintilla_send_message (SCINTILLA (te->scintilla), SCI_ADDTEXT,
							nchars, (long) buffer);
	
	g_free (buffer);
	gnome_vfs_close(vfs_read);
	return TRUE;
}

static gboolean
save_to_file (TextEditor *te, gchar * uri)
{
	GnomeVFSHandle* vfs_write;
	GnomeVFSResult result;
	GnomeVFSFileSize nchars, size;
	gint strip;
	gchar *data;

	result = gnome_vfs_create (&vfs_write, uri, GNOME_VFS_OPEN_WRITE,
							   FALSE, 0664);
 	if (result != GNOME_VFS_OK)
		return FALSE;
	nchars = scintilla_send_message (SCINTILLA (te->scintilla),
									 SCI_GETLENGTH, 0, 0);
	data =	(gchar *) aneditor_command (te->editor_id,
										ANE_GETTEXTRANGE, 0, nchars);
	if (data)
	{
		gint dos_filter, editor_mode;
		
		size = strlen (data);
		
		/* Save according to the correct encoding */
		if (anjuta_preferences_get_int (te->preferences,
										SAVE_ENCODING_CURRENT_LOCALE))
		{
			/* Save in current locate */
			GError *conv_error = NULL;
			gchar* converted_file_contents = NULL;

			DEBUG_PRINT ("Using current locale's encoding");

			converted_file_contents = g_locale_from_utf8 (data, -1, NULL,
														  NULL, &conv_error);
	
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
			/* Save in original encoding */
			if ((te->encoding != NULL) &&
				anjuta_preferences_get_int (te->preferences,
											SAVE_ENCODING_ORIGINAL))
			{
				GError *conv_error = NULL;
				gchar* converted_file_contents = NULL;
	
				DEBUG_PRINT ("Using encoding %s", te->encoding);

				/* Try to convert it from UTF-8 to original encoding */
				converted_file_contents = g_convert (data, -1, 
													 te->encoding,
													 "UTF-8", NULL, NULL,
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
				DEBUG_PRINT ("Using utf-8 encoding");
			}				
		}
		
		/* Strip trailing spaces */
		strip = anjuta_preferences_get_int (te->preferences,
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
		dos_filter = anjuta_preferences_get_int (te->preferences,
												 DOS_EOL_CHECK);
		editor_mode =  scintilla_send_message (SCINTILLA (te->scintilla),
											   SCI_GETEOLMODE, 0, 0);
		DEBUG_PRINT ("Saving in editor mode [%d]", editor_mode);
		nchars = size;
		if (editor_mode == SC_EOL_CRLF && dos_filter)
		{
			DEBUG_PRINT ("Filtering Extrageneous DOS characters in dos mode [Unix => Dos]");
			size = save_filtered_in_dos_mode(vfs_write, data, size);
		}
		else
		{
			gnome_vfs_write(vfs_write, data, size, &nchars);
		}
		g_free (data);
		/* FIXME: Find a nice way to check that all the bytes have been written */
		/* if (size != nchars)
			DEBUG_PRINT("Text length and number of bytes saved is not equal [%d != %d]", nchars, size);
		*/
	}
	result = gnome_vfs_close(vfs_write);
	if (result == GNOME_VFS_OK)
		return TRUE;
	else
		return FALSE;
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
	scintilla_send_message (SCINTILLA (te->scintilla),
							SCI_SETSAVEPOINT, 0, 0);
	scintilla_send_message (SCINTILLA (te->scintilla),
							SCI_EMPTYUNDOBUFFER, 0, 0);
	text_editor_set_hilite_type (te, NULL);
	if (anjuta_preferences_get_int (te->preferences
, FOLD_ON_OPEN))
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
	gboolean ret = FALSE;
	
	if (te == NULL)
		return FALSE;
	if (IS_SCINTILLA (te->scintilla) == FALSE)
		return FALSE;
	
	text_editor_freeze (te);
	text_editor_set_line_number_width(te);
	
	anjuta_status (te->status, _("Saving file..."), 5); 
	text_editor_update_monitor (te, TRUE);
	
	if (save_to_file (te, te->uri) == FALSE)
	{
		GtkWindow *parent;
		parent = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (te)));
		text_editor_thaw (te);
		anjuta_util_dialog_error_system (parent, errno,
										 _("Could not save file: %s."),
										 te->uri);
	}
	else
	{
		/* te->modified_time = time (NULL); */
		/* we need to update UI with the call to scintilla */
		text_editor_thaw (te);
		scintilla_send_message (SCINTILLA (te->scintilla),
					SCI_SETSAVEPOINT, 0, 0);
		g_signal_emit_by_name (G_OBJECT (te), "saved", te->uri);
		anjuta_status (te->status, _("File saved successfully"), 5);
		ret = TRUE;
	}
	text_editor_update_monitor (te, FALSE);
	return ret;
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
					 SCI_GETMODIFY, 0, 0));
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

/*
 *Get the current selection. If there is no selection, or if the selection
 * is all blanks, get the word under teh cursor.
 */
gchar
*text_editor_get_current_word(TextEditor *te)
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
#ifdef DEBUG
	if (buf)
		DEBUG_PRINT ("Current word is '%s'", buf);
#endif
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
	DEBUG_PRINT ("Reading file: %s", propfile);
	
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

	propdir = g_build_filename (g_get_home_dir(), ".anjuta" PREF_SUFFIX "/", NULL);
	propfile = g_build_filename (g_get_home_dir(), ".anjuta" PREF_SUFFIX,
								 "editor-style.properties", NULL);
	DEBUG_PRINT ("Reading file: %s", propfile);
	
	/* Create user.properties file, if it doesn't exist */
	if (g_file_test (propfile, G_FILE_TEST_EXISTS) == FALSE) {
		gchar* old_propfile = g_build_filename (g_get_home_dir(),
												".anjuta"PREF_SUFFIX,
												"session.properties", NULL);
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
	if (anjuta_preferences_get_int_with_default(te->preferences,
			"margin.linenumber.visible", 0))
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

static void
itext_editor_goto_line (IAnjutaEditor *editor, gint lineno, GError **e)
{
	text_editor_goto_line (TEXT_EDITOR (editor), lineno, FALSE, TRUE);
}

static void
itext_editor_goto_position (IAnjutaEditor *editor, gint position, GError **e)
{
	text_editor_goto_point (TEXT_EDITOR (editor), position);
}

static gchar*
itext_editor_get_text (IAnjutaEditor *editor, gint start, gint end,
						 GError **e)
{
	gint nchars;
	gchar *data;
	TextEditor *te = TEXT_EDITOR (editor);
	nchars = scintilla_send_message (SCINTILLA (te->scintilla),
									 SCI_GETLENGTH, 0, 0);
	data =	(gchar *) aneditor_command (te->editor_id,
										ANE_GETTEXTRANGE, 0, nchars);
	return data;
}

static gchar*
itext_editor_get_attributes (IAnjutaEditor *editor, gint start,
							   gint end, GError **e)
{
	DEBUG_PRINT("get_attributes: Not yet implemented in EditorPlugin");
	return NULL;
}

static gint
itext_editor_get_position (IAnjutaEditor *editor, GError **e)
{
	return text_editor_get_current_position (TEXT_EDITOR(editor));
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
	aneditor_command (TEXT_EDITOR (editor)->editor_id,
					  ANE_GETCURRENTWORD, (glong) buffer, 512);
	return g_strdup (buffer);
}

static void
itext_editor_insert (IAnjutaEditor *editor, gint pos, const gchar *txt,
					 gint length, GError **e)
{
	gchar *text_to_insert;
	if (length >= 0)
		text_to_insert = g_strndup (txt, length);
	else
		text_to_insert = g_strdup (txt);
	
	aneditor_command (TEXT_EDITOR(editor)->editor_id, ANE_INSERTTEXT,
					  pos, (long)text_to_insert);
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
itext_editor_erase_all (IAnjutaEditor *editor, GError **e)
{
	scintilla_send_message (SCINTILLA (TEXT_EDITOR (editor)->scintilla), SCI_CLEARALL,
							0, 0);
}

static const gchar *
itext_editor_get_filename (IAnjutaEditor *editor, GError **e)
{
	return (TEXT_EDITOR (editor))->filename;
}

static gboolean
itext_editor_can_undo(IAnjutaEditor *editor, GError **e)
{
	return text_editor_can_undo(TEXT_EDITOR(editor));
}

static gboolean
itext_editor_can_redo(IAnjutaEditor *editor, GError **e)
{
	return text_editor_can_redo(TEXT_EDITOR(editor));
}

static void 
itext_editor_undo(IAnjutaEditor* te, GError** ee)
{
	text_editor_command(TEXT_EDITOR(te), ANE_UNDO, 0, 0);
}

static void 
itext_editor_redo(IAnjutaEditor* te, GError** ee)
{
	text_editor_command(TEXT_EDITOR(te), ANE_REDO, 0, 0);
}

static int
itext_editor_get_column(IAnjutaEditor *editor, GError **e)
{
	return text_editor_get_current_column(TEXT_EDITOR(editor));
}

static gboolean
itext_editor_get_overwrite(IAnjutaEditor *editor, GError **e)
{
	return text_editor_get_overwrite(TEXT_EDITOR(editor));
}

static void
itext_editor_set_popup_menu(IAnjutaEditor *editor, GtkWidget* menu, GError **e)
{
	text_editor_set_popup_menu(TEXT_EDITOR(editor), menu);
}

static gint
itext_editor_get_line_from_position (IAnjutaEditor *editor, gint pos, GError **e)
{
	return 	scintilla_send_message (SCINTILLA(TEXT_EDITOR(editor)->scintilla),
								    SCI_LINEFROMPOSITION, pos, 0);
}

static gint
itext_editor_get_line_begin_position (IAnjutaEditor *editor, gint line,
									  GError **e)
{
	gint ln;
	
	g_return_val_if_fail (line > 0, -1);
	
	ln = linenum_text_editor_to_scintilla (line);
	return scintilla_send_message (SCINTILLA (TEXT_EDITOR (editor)->scintilla),
								   SCI_POSITIONFROMLINE, ln, 0);
}

static gint
itext_editor_get_line_end_position (IAnjutaEditor *editor, gint line,
									  GError **e)
{
	gint ln;
	
	g_return_val_if_fail (line > 0, -1);
	
	ln = linenum_text_editor_to_scintilla (line);
	return scintilla_send_message (SCINTILLA (TEXT_EDITOR (editor)->scintilla),
								   SCI_POSITIONFROMLINE, ln + 1, 0) - 1;
}

static void
itext_editor_iface_init (IAnjutaEditorIface *iface)
{
	iface->goto_line = itext_editor_goto_line;
	iface->goto_position = itext_editor_goto_position;
	iface->get_text = itext_editor_get_text;
	iface->get_attributes = itext_editor_get_attributes;
	iface->get_position = itext_editor_get_position;
	iface->get_lineno = itext_editor_get_lineno;
	iface->get_length = itext_editor_get_length;
	iface->get_current_word = itext_editor_get_current_word;
	iface->insert = itext_editor_insert;
	iface->append = itext_editor_append;
	iface->erase_all = itext_editor_erase_all;
	iface->get_filename = itext_editor_get_filename;
	iface->can_undo = itext_editor_can_undo;
	iface->can_redo = itext_editor_can_redo;
	iface->undo = itext_editor_undo;
	iface->redo = itext_editor_redo;
	iface->get_column = itext_editor_get_column;
	iface->get_overwrite = itext_editor_get_overwrite;
	iface->set_popup_menu = itext_editor_set_popup_menu;
	iface->get_line_from_position = itext_editor_get_line_from_position;
	iface->get_line_begin_position = itext_editor_get_line_begin_position;
	iface->get_line_end_position = itext_editor_get_line_end_position;
}

/* IAnjutaEditorSelection implementation */

static gchar*
iselection_get (IAnjutaEditorSelection *editor, GError **error)
{
	return text_editor_get_selection (TEXT_EDITOR (editor));
}

static void
iselection_set (IAnjutaEditorSelection *editor, gint start, gint end,
		gboolean backwards, GError **e)
{
    if (!backwards)
	scintilla_send_message(SCINTILLA(TEXT_EDITOR(editor)->scintilla),
						   SCI_SETSEL, start, end);
    else
	scintilla_send_message(SCINTILLA(TEXT_EDITOR(editor)->scintilla),
						   SCI_SETSEL, end, start);
}

static int
iselection_get_start(IAnjutaEditorSelection *editor, GError **e)
{
	return scintilla_send_message(SCINTILLA(TEXT_EDITOR(editor)->scintilla),
								  SCI_GETSELECTIONSTART,0,0);
}

static int
iselection_get_end(IAnjutaEditorSelection *editor, GError **e)
{
	return scintilla_send_message(SCINTILLA(TEXT_EDITOR(editor)->scintilla),
								  SCI_GETSELECTIONEND, 0, 0);
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
iselection_select_all(IAnjutaEditorSelection* te, GError** ee)
{
	text_editor_command(TEXT_EDITOR(te), ANE_SELECTALL, 0, 0);
}

static void
iselection_select_to_brace(IAnjutaEditorSelection* te, GError** ee)
{
	text_editor_command(TEXT_EDITOR(te), ANE_SELECTTOBRACE, 0, 0);
}

static void
iselection_select_block(IAnjutaEditorSelection *te, GError **e)
{
	text_editor_command(TEXT_EDITOR(te), ANE_SELECTBLOCK, 0, 0);
}

static void
iselection_select_function(IAnjutaEditorSelection *editor, GError **e)
{
	TextEditor* te = TEXT_EDITOR(editor);
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

static void 
iselection_cut(IAnjutaEditorSelection* te, GError** ee)
{
	text_editor_command(TEXT_EDITOR(te), ANE_CUT, 0, 0);
}

static void 
iselection_copy(IAnjutaEditorSelection* te, GError** ee)
{
	text_editor_command(TEXT_EDITOR(te), ANE_COPY, 0, 0);
}

static void 
iselection_paste(IAnjutaEditorSelection* te, GError** ee)
{
	text_editor_command(TEXT_EDITOR(te), ANE_PASTE, 0, 0);
}

static void 
iselection_clear(IAnjutaEditorSelection* te, GError** ee)
{
	text_editor_command(TEXT_EDITOR(te), ANE_CLEAR, 0, 0);
}

static void
iselection_iface_init (IAnjutaEditorSelectionIface *iface)
{
	iface->get = iselection_get;
	iface->set = iselection_set;
	iface->get_start = iselection_get_start;
	iface->get_end = iselection_get_end;
	iface->replace = iselection_replace;
	iface->select_all = iselection_select_all;
	iface->select_to_brace = iselection_select_to_brace;
	iface->select_block = iselection_select_block;
	iface->select_function = iselection_select_function;
	iface->cut = iselection_cut;
	iface->copy = iselection_copy;
	iface->paste = iselection_paste;
	iface->clear = iselection_clear;
}

/* IAnjutaFile implementation */

static gchar*
ifile_get_uri (IAnjutaFile *editor, GError **error)
{
	TextEditor *text_editor;
	text_editor = TEXT_EDITOR(editor);
	if (text_editor->uri)
		return g_strdup (text_editor->uri);
/*
	else if (text_editor->filename)
		return gnome_vfs_get_uri_from_local_path (text_editor->filename);
*/
	else
		return NULL;
}

static void
ifile_open (IAnjutaFile *editor, const gchar* uri, GError **error)
{
	/* Close current file and open new file in this editor */
	TextEditor* text_editor;
	text_editor = TEXT_EDITOR(editor);
	
	/* Do nothing if current file is not saved */
	if (!text_editor_is_saved (text_editor))
		return;
	text_editor->uri = g_strdup (uri);
	
	/* Remove path */
	text_editor->filename = g_strdup (g_basename (uri));
	text_editor_load_file (text_editor);
}

static void
isaveable_save (IAnjutaFileSavable* editor, GError** e)
{
	TextEditor *text_editor = TEXT_EDITOR(editor);
	if (text_editor->uri != NULL)
		text_editor_save_file(text_editor, FALSE);
}

static void
isavable_save_as (IAnjutaFileSavable* editor, const gchar* filename, GError** e)
{
	TextEditor *text_editor = TEXT_EDITOR(editor);
	text_editor->uri = g_strdup(filename);
	/* Remove path */
	text_editor->filename = g_strdup(strrchr(filename, '/') + 1);
	text_editor_save_file(text_editor, FALSE);
}

static gboolean
isavable_is_dirty (IAnjutaFileSavable* editor, GError** e)
{
	TextEditor *text_editor = TEXT_EDITOR(editor);
	return !text_editor_is_saved(text_editor);
}

static void
isavable_set_dirty (IAnjutaFileSavable* editor, gboolean dirty, GError** e)
{
	DEBUG_PRINT("set_dirty: Not implemented in EditorPlugin");
}


static void
isavable_iface_init (IAnjutaFileSavableIface *iface)
{
	iface->save = isaveable_save;
	iface->save_as = isavable_save_as;
	iface->set_dirty = isavable_set_dirty;
	iface->is_dirty = isavable_is_dirty;
}

static void
ifile_iface_init (IAnjutaFileIface *iface)
{
	iface->open = ifile_open;
	iface->get_uri = ifile_get_uri;
}

/* Implementation of the IAnjutaMarkable interface */

static gint
marker_ianjuta_to_editor (IAnjutaMarkableMarker marker)
{
	gint mark;
	switch (marker)
	{
		case IANJUTA_MARKABLE_BASIC:
			mark = TEXT_EDITOR_LINEMARKER;
			break;
		case IANJUTA_MARKABLE_LIGHT:
			mark = TEXT_EDITOR_BOOKMARK;
			break;
		case IANJUTA_MARKABLE_ATTENTIVE:
			mark = TEXT_EDITOR_BREAKPOINT_DISABLED;
			break;
		case IANJUTA_MARKABLE_INTENSE:
			mark = TEXT_EDITOR_BREAKPOINT_ENABLED;
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

/* IAnjutaEditorFactory implementation */

static IAnjutaEditor*
itext_editor_factory_new_editor(IAnjutaEditorFactory* factory, 
								const gchar* uri,
								const gchar* filename, 
								GError** error)
{
	TextEditor *current_editor = TEXT_EDITOR (factory);
	GtkWidget* editor = text_editor_new (current_editor->status,
										 current_editor->preferences,
										 uri, filename);
	return IANJUTA_EDITOR (editor);
}

static void
itext_editor_factory_iface_init (IAnjutaEditorFactoryIface *iface)
{
	iface->new_editor = itext_editor_factory_new_editor;
}

/* IAnjutaEditorConvert implementation */

static void
iconvert_to_upper(IAnjutaEditorConvert* te, gint start_position,
				  gint end_position, GError** ee)
{
	scintilla_send_message (SCINTILLA (TEXT_EDITOR(te)->scintilla),
						   SCI_SETSEL, start_position, end_position);
	text_editor_command (TEXT_EDITOR(te), ANE_UPRCASE, 0, 0);
}

static void
iconvert_to_lower(IAnjutaEditorConvert* te, gint start_position,
				  gint end_position, GError** ee)
{
	scintilla_send_message (SCINTILLA (TEXT_EDITOR(te)->scintilla),
						   SCI_SETSEL, start_position, end_position);
	text_editor_command (TEXT_EDITOR(te), ANE_LWRCASE, 0, 0);	
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
iassist_autocomplete(IAnjutaEditorAssist* te, GError** ee)
{
	text_editor_command(TEXT_EDITOR(te), ANE_COMPLETEWORD, 0, 0);
}

static void
iassist_iface_init(IAnjutaEditorAssistIface* iface)
{
	iface->autocomplete = iassist_autocomplete;
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

/* IAnjutaBookmark implementation */
static void
ibookmark_toggle(IAnjutaBookmark* view, gint location,
			  gboolean ensure_visible, GError **e)
{
	text_editor_goto_line (TEXT_EDITOR(view), location, FALSE, ensure_visible);
	text_editor_command(TEXT_EDITOR(view), ANE_BOOKMARK_TOGGLE, 0, 0);
}

static void
ibookmark_first(IAnjutaBookmark* view, GError **e)
{
	text_editor_command(TEXT_EDITOR(view), ANE_BOOKMARK_FIRST, 0, 0);
}

static void
ibookmark_last(IAnjutaBookmark* view, GError **e)
{
	text_editor_command(TEXT_EDITOR(view), ANE_BOOKMARK_LAST, 0, 0);
}

static void
ibookmark_next(IAnjutaBookmark* view, GError **e)
{
	text_editor_command(TEXT_EDITOR(view), ANE_BOOKMARK_NEXT, 0, 0);
}

static void
ibookmark_previous(IAnjutaBookmark* view, GError **e)
{
	text_editor_command(TEXT_EDITOR(view), ANE_BOOKMARK_PREV, 0, 0);
}

static void
ibookmark_clear_all(IAnjutaBookmark* view, GError **e)
{
	text_editor_command(TEXT_EDITOR(view), ANE_BOOKMARK_CLEAR, 0, 0);
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
iindicable_set (IAnjutaIndicable *te, gint begin_location, gint end_location,
				IAnjutaIndicableIndicator indicator, GError **err)
{
	switch (indicator)
	{
		case IANJUTA_INDICABLE_NONE:
			text_editor_set_indicator (TEXT_EDITOR (te), begin_location,
									   end_location, -1);
		break;
		case IANJUTA_INDICABLE_IMPORTANT:
			text_editor_set_indicator (TEXT_EDITOR (te), begin_location,
									   end_location, 0);
		break;
		case IANJUTA_INDICABLE_WARNING:
			text_editor_set_indicator (TEXT_EDITOR (te), begin_location,
									   end_location, 1);
		break;
		case IANJUTA_INDICABLE_CRITICAL:
			text_editor_set_indicator (TEXT_EDITOR (te), begin_location,
									   end_location, 2);
		break;
		default:
			g_warning ("Unsupported indicator %d", indicator);
			text_editor_set_indicator (TEXT_EDITOR (te), begin_location,
									   end_location, -1);
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
on_zoom_text_activate (const gchar *zoom_text,
					   TextEditor* te)
{
	gint zoom;
	gchar buf[20];
	
	if (!zoom_text)
		zoom = 0;
	else if (0 == strncmp(zoom_text, "++", 2))
		zoom = sci_prop_get_int(te->props_base, TEXT_ZOOM_FACTOR, 0) + 2;
	else if (0 == strncmp(zoom_text, "--", 2))
		zoom = sci_prop_get_int(te->props_base, TEXT_ZOOM_FACTOR, 0) - 2;
	else
		zoom = atoi(zoom_text);
	if (zoom > MAX_ZOOM_FACTOR)
		zoom = MAX_ZOOM_FACTOR;
	else if (zoom < MIN_ZOOM_FACTOR)
		zoom = MIN_ZOOM_FACTOR;
	g_snprintf(buf, 20, "%d", zoom);
	sci_prop_set_with_key (te->props_base, TEXT_ZOOM_FACTOR, buf);
	text_editor_set_zoom_factor(te, zoom);
}

static void
izoom_in(IAnjutaEditorZoom* zoom, GError** e)
{
	TextEditor* te = TEXT_EDITOR(zoom);
   	on_zoom_text_activate ("++", te);
}

static void
izoom_out(IAnjutaEditorZoom* zoom, GError** e)
{
	TextEditor* te = TEXT_EDITOR(zoom);
   	on_zoom_text_activate ("--", te);
}

static void
izoom_iface_init(IAnjutaEditorZoomIface* iface)
{
	iface->in = izoom_in;
	iface->out = izoom_out;
}

ANJUTA_TYPE_BEGIN(TextEditor, text_editor, GTK_TYPE_VBOX);
ANJUTA_TYPE_ADD_INTERFACE(ifile, IANJUTA_TYPE_FILE);
ANJUTA_TYPE_ADD_INTERFACE(isavable, IANJUTA_TYPE_FILE_SAVABLE);
ANJUTA_TYPE_ADD_INTERFACE(itext_editor, IANJUTA_TYPE_EDITOR);
ANJUTA_TYPE_ADD_INTERFACE(ilinemode, IANJUTA_TYPE_EDITOR_LINE_MODE);
ANJUTA_TYPE_ADD_INTERFACE(iselection, IANJUTA_TYPE_EDITOR_SELECTION);
ANJUTA_TYPE_ADD_INTERFACE(iconvert, IANJUTA_TYPE_EDITOR_CONVERT);
ANJUTA_TYPE_ADD_INTERFACE(iassist, IANJUTA_TYPE_EDITOR_ASSIST);
ANJUTA_TYPE_ADD_INTERFACE(iview, IANJUTA_TYPE_EDITOR_VIEW);
ANJUTA_TYPE_ADD_INTERFACE(ifolds, IANJUTA_TYPE_EDITOR_FOLDS);
ANJUTA_TYPE_ADD_INTERFACE(ibookmark, IANJUTA_TYPE_BOOKMARK);
ANJUTA_TYPE_ADD_INTERFACE(imarkable, IANJUTA_TYPE_MARKABLE);
ANJUTA_TYPE_ADD_INTERFACE(iindicable, IANJUTA_TYPE_INDICABLE);
ANJUTA_TYPE_ADD_INTERFACE(iprint, IANJUTA_TYPE_PRINT);
ANJUTA_TYPE_ADD_INTERFACE(icomment, IANJUTA_TYPE_EDITOR_COMMENT);
ANJUTA_TYPE_ADD_INTERFACE(izoom, IANJUTA_TYPE_EDITOR_ZOOM);

/* FIXME: Is factory definition really required for editor class? */
ANJUTA_TYPE_ADD_INTERFACE(itext_editor_factory, IANJUTA_TYPE_EDITOR_FACTORY);
ANJUTA_TYPE_END;
