/*
 * text_editor_cbs.
 * Copyright (C) 2000  Kh. Naba Kumar Singh
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

#include <gnome.h>
#include <ctype.h>

#include <libanjuta/resources.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-debug.h>

#include "text_editor.h"
#include "text_editor_cbs.h"
#include "text-editor-iterable.h"

#define GTK
#undef PLAT_GTK
#define PLAT_GTK 1
#include "Scintilla.h"
#include "SciLexer.h"
#include "ScintillaWidget.h"

gboolean
on_text_editor_scintilla_focus_in (GtkWidget* scintilla, GdkEvent *event,
								   TextEditor *te)
{
	GList *node;
	
	node = te->views;
	while (node)
	{
		if (aneditor_get_widget (GPOINTER_TO_INT (node->data)) == scintilla)
		{
			DEBUG_PRINT ("Switching editor view ...");
			te->editor_id = GPOINTER_TO_INT (node->data);
			te->scintilla = aneditor_get_widget (te->editor_id);
			break;
		}
		node = g_list_next (node);
	}
	return FALSE;
}

gboolean
on_text_editor_text_buttonpress_event (GtkWidget * widget,
									   GdkEventButton * event,
									   gpointer user_data)
{
	TextEditor *te = user_data;
	gtk_widget_grab_focus (GTK_WIDGET (te->scintilla));
	return FALSE;
}

gboolean
on_text_editor_text_event (GtkWidget * widget,
						   GdkEvent * event, gpointer user_data)
{
	GdkEventButton *bevent;
	if (event->type != GDK_BUTTON_PRESS)
		return FALSE;
	if (((GdkEventButton *) event)->button != 3)
		return FALSE;
	bevent = (GdkEventButton *) event;
	bevent->button = 1;
	gtk_menu_popup (GTK_MENU (TEXT_EDITOR (user_data)->popup_menu),
					NULL, NULL, NULL, NULL,
					bevent->button, bevent->time);
	return TRUE;
}

void
on_text_editor_text_changed (GtkEditable * editable, gpointer user_data)
{
	TextEditor *te = TEXT_EDITOR (user_data);
	if (text_editor_is_saved (te))
	{
		//FIXME:
		// anjuta_update_title ();
		// update_main_menubar ();
		text_editor_update_controls (te);
	}
}

void
on_text_editor_insert_text (GtkEditable * text,
			    const gchar * insertion_text,
			    gint length, gint * pos, TextEditor * te)
{
}


static void 
scintilla_uri_dropped (TextEditor *te, const char *uri)
{
	GtkWidget *parent;
	GtkSelectionData tmp;
	
	tmp.data = (guchar *) uri;

	parent = gtk_widget_get_toplevel (GTK_WIDGET (te));
	if (parent)
		g_signal_emit_by_name (G_OBJECT (parent), "drag_data_received",
							   NULL, 0, 0, &tmp, 0, 0);
	return;
}

gboolean timerclick = FALSE;

static gboolean
click_timeout (TextEditor *te)
{
	gint line = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (te),
													"marker_line"));
	
	/*  If not second clic after timeout : Single Click  */
	if (timerclick)
	{
		timerclick = FALSE;
		/* Emit (single) clicked signal */
		g_signal_emit_by_name (G_OBJECT (te), "marker_clicked", FALSE, line);
	}
	return FALSE;
}

void
on_text_editor_scintilla_notify (GtkWidget * sci, gint wParam, gpointer lParam,
								 gpointer data)
{
	TextEditor *te;
	struct SCNotification *nt;
	gint line, position, autoc_sel;
	
	te = data;
	if (te->freeze_count != 0)
		return;
	nt = lParam;
	switch (nt->nmhdr.code)
	{
	case SCN_URIDROPPED:
		scintilla_uri_dropped(te, nt->text);
		break;
	case SCN_SAVEPOINTREACHED:
		g_signal_emit_by_name(G_OBJECT (te), "save_point", TRUE);
		break;
	case SCN_SAVEPOINTLEFT:
		g_signal_emit_by_name(G_OBJECT (te), "save_point", FALSE);
		//FIXME:
		// anjuta_update_title ();
		// update_main_menubar ();
		text_editor_update_controls (te);
		return;
	case SCN_UPDATEUI:
		te->current_line = text_editor_get_current_lineno (te);
		g_signal_emit_by_name(G_OBJECT (te), "update_ui");
		return;
		
	case SCN_CHARADDED:
		{
			position = text_editor_get_current_position (te) - 1;
			TextEditorCell *position_iter = text_editor_cell_new (te, position);
			te->current_line = text_editor_get_current_lineno (te);
			g_signal_emit_by_name(G_OBJECT (te), "char-added", position_iter,
								  (gchar)nt->ch);
			g_object_unref (position_iter);
		}
		return;
	case SCN_AUTOCSELECTION:
	case SCN_USERLISTSELECTION:
		autoc_sel = (gint) scintilla_send_message (SCINTILLA (te->scintilla),
												   SCI_AUTOCGETCURRENT,
												   0, 0);
		scintilla_send_message (SCINTILLA (te->scintilla),
								SCI_AUTOCCANCEL, 0, 0);
		g_signal_emit_by_name (te, "assist-chosen", autoc_sel);
		return;
	case SCN_MODIFIED:
		if (nt->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT))
		{
			TextEditorCell *position_iter =
					text_editor_cell_new (te, nt->position);
			gboolean added = nt->modificationType & SC_MOD_INSERTTEXT;
			g_signal_emit_by_name (G_OBJECT (te), "changed", position_iter,
								   added, nt->length, nt->linesAdded,
								   nt->text);
			g_object_unref (position_iter);
		}
		return;
	case SCN_MARGINCLICK:
		line =	text_editor_get_line_from_position (te, nt->position);
		if (nt->margin == 1)  /*  Bookmarks and Breakpoints  margin */
		{
			/* if second click before timeout : Double click  */
			if (timerclick)
			{
				timerclick = FALSE;
				text_editor_goto_line (te, line, -1, TRUE);
				aneditor_command (te->editor_id, ANE_BOOKMARK_TOGGLE, 0, 0);
				/* Emit (double) clicked signal */
				g_signal_emit_by_name (G_OBJECT (te), "marker_clicked",
									   TRUE, line);
			}
			else
			{
				timerclick = TRUE;
				g_object_set_data (G_OBJECT (te), "marker_line",
								   GINT_TO_POINTER (line));
				/* Timeout after 400ms  */
				/* If 2 clicks before the timeout ==> Single Click */
				g_timeout_add (400, (void*) click_timeout, te);
			}
	}
	return;
	case SCN_DWELLSTART:
    {
      TextEditorCell* cell = nt->position < 0 ? NULL : text_editor_cell_new (te, nt->position); 
      g_signal_emit_by_name (te, "hover-over", cell);
      if (cell) g_object_unref (cell);
      return;
    }
		
	case SCN_DWELLEND:
    {
      TextEditorCell* cell = nt->position < 0 ? NULL : text_editor_cell_new (te, nt->position); 
      
      text_editor_hide_hover_tip (te);
      g_signal_emit_by_name (te, "hover-leave", cell);
      if (cell) g_object_unref (cell);
      return;
    }

/*	case SCEN_SETFOCUS:
	case SCEN_KILLFOCUS:
	case SCN_STYLENEEDED:
	case SCN_DOUBLECLICK:
	case SCN_MODIFIED:
	case SCN_NEEDSHOWN:
*/
	default:
		return;
	}
}

void
on_text_editor_scintilla_size_allocate (GtkWidget * widget,
										GtkAllocation * allocation,
										gpointer data)
{
	TextEditor *te;
	te = data;
	g_return_if_fail (te != NULL);

	if (te->first_time_expose == FALSE)
		return;

	te->first_time_expose = FALSE;
	text_editor_goto_line (te, te->current_line, FALSE, FALSE);
}
