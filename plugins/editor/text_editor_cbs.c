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
#include <libanjuta/interfaces/ianjuta-iterable.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-editor-cell.h>
#include <libanjuta/interfaces/ianjuta-editor-assist.h>

#include "text_editor.h"
#include "text_editor_cbs.h"

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

static void
text_editor_check_assist (TextEditor *te, gchar ch, gint p)
{
	gint selStart = text_editor_get_selection_start (te);
	gint selEnd = text_editor_get_selection_end (te);
	gint position = selStart - 1;
	
	DEBUG_PRINT ("Text editor char added: %c, p = %d, position = %d",
				 ch, p, position);
	if ((selEnd == selStart) && (selStart > 0))
	{
		int style = scintilla_send_message (SCINTILLA (te->scintilla),
											SCI_GETSTYLEAT, position, 0);
		if (style != 1)
		{
			gboolean found = FALSE;
			gint i;
			for (i = 1; i <= max_trigger_size (te->assist_triggers); i++)
			{
				gchar* trigger;
				IAnjutaEditorAssistContextParser parser = NULL;
				trigger = (gchar*) aneditor_command (te->editor_id,
													 ANE_GETTEXTRANGE,
													 ((selStart - i) < 0? 0 : (selStart - i)),
													 selStart);
				DEBUG_PRINT ("Trigger found: \"%s\"", trigger);
				parser = g_hash_table_lookup (te->assist_triggers, trigger);
				if (parser)
				{
					gchar* context = parser (IANJUTA_EDITOR (te),
											 position + 1);
					DEBUG_PRINT ("Context is: \"%s\"", context);
					
					/* If there is an autocomplete in progress, end it
					 * because autocompletes are not recursive */
					if (te->assist_active)
						g_signal_emit_by_name (te, "assist-end");
					g_signal_emit_by_name (G_OBJECT (te), "assist_begin",
										   context, trigger);
					g_free (trigger);
					g_free (context);
					found = TRUE;
					break;
				}
			}
			if (!found)
			{
				/* Consider simple autocomplete */
				if (te->assist_active && ch != '_' && !isalnum (ch))
					/* (!wordCharacters.contains(ch)) */ /* FIXME */
				{
					/* End autocompletion if non word char is typed */
					g_signal_emit_by_name (te, "assist-end");
				}
				else
				{
					/* Pick up autocomplete word */
					gchar *context =
						text_editor_get_word_before_carat (te);
					if (context)
					{
						te->assist_position_end = position;
						te->assist_position_begin = position - strlen (context);
						if (te->assist_active)
							g_signal_emit_by_name (te, "assist-update", context,
												   NULL);
						else
							g_signal_emit_by_name (te, "assist-begin", context,
												   NULL);
						DEBUG_PRINT ("Context is: %s", context);
						g_free (context);
					}
				}
			}
		}
	}
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
		if (te->assist_active)
		{
			gint cur_position = text_editor_get_current_position (te);
			
			DEBUG_PRINT ("Update UI: cur_pos = %d, assist_begin = %d, assist_end = %d",
						 cur_position, te->assist_position_begin,
						 te->assist_position_end);
			
			/* If cursor moves out of autocomplete work range, end it */
			if (cur_position <= te->assist_position_begin ||
				(cur_position > te->assist_position_end + 1))
			{
				g_signal_emit_by_name (te, "assist-end");
			}
			else
			{
				gchar *context = text_editor_get_word_before_carat (te);
				if (context)
				{
					if (te->assist_active)
						g_signal_emit_by_name (te, "assist-update", context,
											   NULL);
				}
			}
		}
		return;
		
	case SCN_CHARADDED:
		te->current_line = text_editor_get_current_lineno (te);
		position = text_editor_get_current_position (te) - 1;
		g_signal_emit_by_name(G_OBJECT (te), "char_added", position,
							  (gchar)nt->ch);
		
		/* Assist */
		text_editor_check_assist (te, (gchar)nt->ch, position);
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
			gboolean added = nt->modificationType & SC_MOD_INSERTTEXT;
			g_signal_emit_by_name (G_OBJECT (te), "changed", nt->position,
								   added, nt->length, nt->linesAdded,
								   nt->text);
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
