/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Johannes Schmid 2007 <jhs@gnome.org>
 * 
 * anjuta is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * anjuta is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with anjuta.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include <glib/gi18n.h>
#include "search-box.h"

#include <stdlib.h>
#include <gtk/gtk.h>

#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-status.h>
#include <libanjuta/anjuta-debug.h>

#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-editor-search.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>
#include <libanjuta/interfaces/ianjuta-indicable.h>

#define ANJUTA_STOCK_GOTO_LINE "anjuta-goto-line"

/* width of the entries in chars, that include the space for the icon */
#define LINE_ENTRY_WIDTH 7
#define SEARCH_ENTRY_WIDTH 45

/* Time spend to do search in the idle callback */
#define CONTINUOUS_SEARCH_TIMEOUT 0.1

struct _SearchBoxPrivate
{
	GtkWidget* grid;

	GtkWidget* search_entry;
	GtkWidget* replace_entry;

	GtkWidget* close_button;
	GtkWidget* next_button;
	GtkWidget* previous_button;

	GtkWidget* replace_button;
	GtkWidget* replace_all_button;	

	GtkWidget* goto_entry;
	GtkWidget* goto_button;

	IAnjutaEditor* current_editor;
	AnjutaStatus* status;
	AnjutaShell* shell;

	/* Search options popup menu */
	GtkWidget* popup_menu;
	GtkAction* case_action;
	GtkAction* highlight_action;
	GtkAction* regex_action;
	
	gboolean case_sensitive;
	gboolean highlight_all;	
	gboolean regex_mode;
	
	IAnjutaEditorCell *start_highlight;
	IAnjutaEditorCell *end_highlight;
	guint idle_id;
};

#ifdef GET_PRIVATE
# undef GET_PRIVATE
#endif
#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE((o), SEARCH_TYPE_BOX, SearchBoxPrivate))

G_DEFINE_TYPE (SearchBox, search_box, GTK_TYPE_HBOX);

static void
on_search_box_hide (GtkWidget* button, SearchBox* search_box)
{
	search_box_hide (search_box);
}

static void
on_document_changed (AnjutaDocman* docman, IAnjutaDocument* doc,
					SearchBox* search_box)
{
	if (!doc || !IANJUTA_IS_EDITOR (doc))
	{
		gtk_widget_hide (GTK_WIDGET (search_box));
		search_box->priv->current_editor = NULL;
	}
	else
	{
		search_box->priv->current_editor = IANJUTA_EDITOR (doc);
		if (search_box->priv->highlight_all) search_box_highlight_all (search_box);
	}
}

static void
on_goto_activated (GtkWidget* widget, SearchBox* search_box)
{
	const gchar* str_line = gtk_entry_get_text (GTK_ENTRY (search_box->priv->goto_entry));
	
	gint line = atoi (str_line);
	if (line > 0)
	{
		ianjuta_editor_goto_line (search_box->priv->current_editor, line, NULL);
	}
}

static void
search_box_set_entry_color (SearchBox* search_box, gboolean found)
{
	if (!found)
	{
		GdkColor red;
		GdkColor white;

		/* FIXME: a11y and theme */

		gdk_color_parse ("#FF6666", &red);
		gdk_color_parse ("white", &white);

		gtk_widget_modify_base (search_box->priv->search_entry,
				        GTK_STATE_NORMAL,
				        &red);
		gtk_widget_modify_text (search_box->priv->search_entry,
				        GTK_STATE_NORMAL,
				        &white);
	}
	else
	{
		gtk_widget_modify_base (search_box->priv->search_entry,
				        GTK_STATE_NORMAL,
				        NULL);
		gtk_widget_modify_text (search_box->priv->search_entry,
				        GTK_STATE_NORMAL,
				        NULL);
	}
}

static gboolean
on_goto_key_pressed (GtkWidget* entry, GdkEventKey* event, SearchBox* search_box)
{
	switch (event->keyval)
	{
		case GDK_KEY_0:
		case GDK_KEY_1:
		case GDK_KEY_2:
		case GDK_KEY_3:
		case GDK_KEY_4:
		case GDK_KEY_5:
		case GDK_KEY_6:
		case GDK_KEY_7:
		case GDK_KEY_8:
		case GDK_KEY_9:
		case GDK_KEY_KP_0:
		case GDK_KEY_KP_1:
		case GDK_KEY_KP_2:
		case GDK_KEY_KP_3:
		case GDK_KEY_KP_4:
		case GDK_KEY_KP_5:
		case GDK_KEY_KP_6:
		case GDK_KEY_KP_7:
		case GDK_KEY_KP_8:
		case GDK_KEY_KP_9:
		case GDK_KEY_Return:
		case GDK_KEY_KP_Enter:
		case GDK_KEY_BackSpace:
		case GDK_KEY_Delete:
		case GDK_KEY_Tab:
		{
			/* This is a number or enter which is ok */
			break;
		}
		case GDK_KEY_Escape:
		{
			search_box_hide (search_box);
		}
		default:
		{
			/* Not a number */
			gdk_beep ();
			return TRUE;
		}
	}
	return FALSE;
}

static gboolean
on_search_box_key_pressed (GtkWidget* widget, GdkEventKey* event, SearchBox* search_box)
{
	switch (event->keyval)
	{
		case GDK_KEY_Escape:
		{
			gtk_widget_hide (GTK_WIDGET (search_box));
			search_box_set_entry_color (search_box, TRUE);
			if (search_box->priv->current_editor)
			{
				ianjuta_document_grab_focus (IANJUTA_DOCUMENT (search_box->priv->current_editor), 
											 NULL);
			}
		}
		default:
		{
			/* Do nothing... */
		}
	}
	return FALSE;
}

static gboolean
on_search_focus_out (GtkWidget* widget, GdkEvent* event, SearchBox* search_box)
{
	anjuta_status_pop (search_box->priv->status);

	return FALSE;
}


/* Search regular expression in text, return TRUE and matching part as start and
 * end integer position */
static gboolean
search_regex_in_text (const gchar* search_entry, const gchar* editor_text, gboolean search_forward, gint * start_pos, gint * end_pos)
{
	GRegex * regex;
	GMatchInfo *match_info;
	gboolean found;
	GError * err = NULL;

	regex = g_regex_new (search_entry, 0, 0, &err);
	if (err)
	{
		g_message ("%s",err->message);
		g_error_free (err);
		g_regex_unref(regex);
		return FALSE;
	}

	found = g_regex_match (regex, editor_text, 0, &match_info);

	if (found)
	{
		if (search_forward)
			g_match_info_fetch_pos(match_info, 0, start_pos, end_pos);
		else
		{
			do
			{
				g_match_info_fetch_pos(match_info, 0, start_pos, end_pos);
			}
			while (g_match_info_next(match_info, NULL));
		}

		*start_pos = g_utf8_pointer_to_offset(editor_text, &editor_text[*start_pos]);
		*end_pos = g_utf8_pointer_to_offset(editor_text, &editor_text[*end_pos]);
	}

	if (regex)
		g_regex_unref(regex);
	if (match_info)
		g_match_info_free (match_info);

	return found;

}

/* Search string in text, return TRUE and matching part as start and
 * end integer position */
static gboolean
search_str_in_text (const gchar* search_entry, const gchar* editor_text, gboolean case_sensitive, gint * start_pos, gint * end_pos)
{
	gboolean found;
	
	if (strlen (editor_text) >= strlen (search_entry))
	{
		gchar* selected_text_case;
		gchar* search_text_case;

		if (case_sensitive)
		{
			selected_text_case = g_strdup (editor_text);
			search_text_case = g_strdup (search_entry);
		}
		else
		{
			selected_text_case = g_utf8_casefold (editor_text, strlen (editor_text));
			search_text_case = g_utf8_casefold (search_entry, strlen (search_entry));
		}

		gchar* strstr = g_strstr_len (selected_text_case, -1, search_text_case);

		if (strstr)
		{
			*start_pos = g_utf8_pointer_to_offset(selected_text_case, strstr);
			*end_pos = g_utf8_pointer_to_offset(selected_text_case, strstr + strlen (search_entry));
			found = TRUE;
		}

		g_free (selected_text_case);
		g_free (search_text_case);
	}

	return found;
}

/* Search string in editor, return TRUE and matching part as start and
 * end editor cell */
static gboolean
editor_search (IAnjutaEditor *editor,
               const gchar *search_text, 
               gboolean case_sensitive,
               gboolean search_forward,
               gboolean regex_mode,
               IAnjutaEditorCell* search_start,
               IAnjutaEditorCell* search_end,
               IAnjutaEditorCell** result_start,
               IAnjutaEditorCell** result_end)
{
	gboolean found;

	if (regex_mode)
	{
		gint start_pos;
		gint end_pos;
		gchar *text_to_search;

		text_to_search = ianjuta_editor_get_text (editor,
		                                          IANJUTA_ITERABLE (search_start),
		                                          IANJUTA_ITERABLE (search_end), NULL);

		found = search_regex_in_text (search_text, text_to_search, search_forward, &start_pos, &end_pos);

		start_pos += ianjuta_iterable_get_position(IANJUTA_ITERABLE (search_start), NULL);
		end_pos += ianjuta_iterable_get_position(IANJUTA_ITERABLE (search_start), NULL);

		if (found && start_pos >= 0)
		{
			*result_start = IANJUTA_EDITOR_CELL (ianjuta_editor_get_start_position (editor,
			                                                                        NULL));
			*result_end = IANJUTA_EDITOR_CELL (ianjuta_editor_get_start_position (editor,
			                                                                      NULL));

			if (!ianjuta_iterable_set_position(IANJUTA_ITERABLE(*result_start), start_pos, NULL) ||
				!ianjuta_iterable_set_position(IANJUTA_ITERABLE(*result_end), end_pos, NULL))
			{
				g_object_unref(*result_start);
				g_object_unref(*result_end);
				found = FALSE;
			}
		}

		g_free(text_to_search);
	}
	else
	{
		if (search_forward)
		{
			found = ianjuta_editor_search_forward (IANJUTA_EDITOR_SEARCH (editor),
			                                       search_text, case_sensitive,
			                                       search_start, search_end,
			                                       result_start,
			                                       result_end, NULL);
		}
		else
		{
			found = ianjuta_editor_search_backward (IANJUTA_EDITOR_SEARCH (editor),
			                                        search_text, case_sensitive,
			                                        search_end, search_start,
			                                        result_start,
			                                        result_end, NULL);
		}
	}

	return found;
}

gboolean
search_box_incremental_search (SearchBox* search_box,
                               gboolean search_forward,
                               gboolean search_next,
                               gboolean wrap)
{
	IAnjutaIterable* real_start;
	IAnjutaEditorCell* search_start;
	IAnjutaEditorCell* search_end;
	IAnjutaEditorCell* result_start;
	IAnjutaEditorCell* result_end;
	IAnjutaEditorSelection* selection;

	const gchar* search_text = gtk_entry_get_text (GTK_ENTRY (search_box->priv->search_entry));

	gboolean found = FALSE;

	if (!search_box->priv->current_editor || !search_text || !strlen (search_text))
		return FALSE;

	selection = IANJUTA_EDITOR_SELECTION (search_box->priv->current_editor);

	if (ianjuta_editor_selection_has_selection (selection, NULL))
	{
		search_start =
			IANJUTA_EDITOR_CELL (ianjuta_editor_selection_get_start (selection, NULL));
	}
	else
	{
		search_start =
			IANJUTA_EDITOR_CELL (ianjuta_editor_get_position (search_box->priv->current_editor,
			                                                  NULL));
	}

	real_start =
		ianjuta_iterable_clone (IANJUTA_ITERABLE (search_start), NULL);

	/* If forward, set search start and end to current position of
	 * cursor and editor end, respectively, or if backward to editor
	 * start and current position of cursor, respectively. Current
	 * position of cursor is selection start if have selection. */
	if (search_forward)
	{
		search_end = IANJUTA_EDITOR_CELL (ianjuta_editor_get_position (search_box->priv->current_editor,
	                                                                   NULL));
		ianjuta_iterable_last (IANJUTA_ITERABLE (search_end), NULL);
	}
	else
	{
		search_end = search_start;
		search_start = IANJUTA_EDITOR_CELL (ianjuta_editor_get_position (search_box->priv->current_editor,
	                                                                     NULL));
		ianjuta_iterable_first (IANJUTA_ITERABLE (search_start), NULL);
	}

	/* When there's a selection, if forward, set search start and end
     * to match end and editor end, respectively, or if backward to
     * editor start and match start, respectively. If forward and
     * selection starts with a match, look for next match.
	 */
	if (ianjuta_editor_selection_has_selection (selection,
	                                            NULL) && search_next)
	{
		gchar* selected_text =
			ianjuta_editor_selection_get (selection, NULL);

		gint start_pos, end_pos;
		gboolean selected_have_search_text = FALSE;

		selected_have_search_text = search_box->priv->regex_mode ?
			search_regex_in_text (search_text, selected_text, TRUE, &start_pos, &end_pos) :
			search_str_in_text (search_text, selected_text, search_box->priv->case_sensitive, &start_pos, &end_pos);

		if (selected_have_search_text)
		{
			IAnjutaIterable* selection_start =
				ianjuta_editor_selection_get_start (selection, NULL);

			if (search_forward && start_pos == 0)
			{
				end_pos += ianjuta_iterable_get_position(IANJUTA_ITERABLE (selection_start), NULL);
				ianjuta_iterable_set_position (IANJUTA_ITERABLE(search_start), end_pos, NULL);
				ianjuta_iterable_last (IANJUTA_ITERABLE (search_end), NULL);
			}
			else if (!search_forward)
			{
				start_pos += ianjuta_iterable_get_position(IANJUTA_ITERABLE (selection_start), NULL);
				ianjuta_iterable_set_position (IANJUTA_ITERABLE(search_end), start_pos, NULL);
				ianjuta_iterable_first (IANJUTA_ITERABLE (search_start), NULL);
			}
			g_object_unref (selection_start);
		}

		g_free (selected_text);
	}

	/* Try searching in current position */
	found = editor_search (search_box->priv->current_editor,
	                       search_text,
	                       search_box->priv->case_sensitive,
	                       search_forward,
	                       search_box->priv->regex_mode,
	                       search_start,
	                       search_end,
	                       &result_start,
	                       &result_end);

	if (found)
	{
		anjuta_status_pop (ANJUTA_STATUS (search_box->priv->status));
	}
	else if (wrap)
	{
		/* Try to wrap if not found */
		ianjuta_iterable_first (IANJUTA_ITERABLE (search_start), NULL);
		ianjuta_iterable_last (IANJUTA_ITERABLE (search_end), NULL);

		/* Try to search again */
		found = editor_search (search_box->priv->current_editor,
		                       search_text,
		                       search_box->priv->case_sensitive,
		                       search_forward,
		                       search_box->priv->regex_mode,
		                       search_start,
		                       search_end,
		                       &result_start,
		                       &result_end);

		/* Check if successful */
		if (found)
		{
			if (ianjuta_iterable_compare (IANJUTA_ITERABLE (result_start),
			                              real_start, NULL) != 0)
			{
				anjuta_status_pop (search_box->priv->status);
				if (search_forward)
				{
					anjuta_status_push (search_box->priv->status,
					                    _("Search for \"%s\" reached the end and was continued at the top."), search_text);
				}
				else
				{
					anjuta_status_push (search_box->priv->status,
					                    _("Search for \"%s\" reached top and was continued at the bottom."), search_text);
				}
			}
			else if (ianjuta_editor_selection_has_selection (selection, NULL))
			{
				found = FALSE;
				anjuta_status_pop (search_box->priv->status);
				if (search_forward)
				{
					anjuta_status_push (search_box->priv->status,
						                _("Search for \"%s\" reached the end and was continued at the top but no new match was found."), search_text);
				}
				else
				{
					anjuta_status_push (search_box->priv->status,
						                _("Search for \"%s\" reached top and was continued at the bottom but no new match was found."), search_text);
				}
			}
			else
			{
				found = FALSE;
			}
		}
	}

	if (found)
	{
		ianjuta_editor_selection_set (selection,
		                              IANJUTA_ITERABLE (result_start),
		                              IANJUTA_ITERABLE (result_end), TRUE, NULL);
		g_object_unref (result_start);
		g_object_unref (result_end);
	}

	search_box_set_entry_color (search_box, found);	
	g_object_unref (real_start);
	g_object_unref (search_start);
	g_object_unref (search_end);

	return found;
}

static gboolean
highlight_in_background (SearchBox *search_box)
{
	gboolean found = FALSE;
	
	GTimer *timer = g_timer_new();

	if (search_box->priv->start_highlight != NULL)
	{
		const gchar* search_text = gtk_entry_get_text (GTK_ENTRY (search_box->priv->search_entry));

		do
		{
			IAnjutaEditorCell* result_start;
			IAnjutaEditorCell* result_end;

			found = editor_search (search_box->priv->current_editor,
			                       search_text,
			                       search_box->priv->case_sensitive,
			                       TRUE,
			                       search_box->priv->regex_mode,
			                       search_box->priv->start_highlight,
			                       search_box->priv->end_highlight,
			                       &result_start,
			                       &result_end);
			if (found)
			{
				ianjuta_indicable_set(IANJUTA_INDICABLE(search_box->priv->current_editor), 
				                      IANJUTA_ITERABLE (result_start), 
				                      IANJUTA_ITERABLE (result_end),
				                      IANJUTA_INDICABLE_IMPORTANT, NULL);
				g_object_unref (result_start);
				g_object_unref (search_box->priv->start_highlight);
				search_box->priv->start_highlight = result_end;
			}
		}
		while (found && g_timer_elapsed(timer, NULL) < CONTINUOUS_SEARCH_TIMEOUT);
		g_timer_destroy (timer);
	}

	if (!found)
	{
		search_box->priv->idle_id = 0;
		g_clear_object (&search_box->priv->start_highlight);
		g_clear_object (&search_box->priv->end_highlight);
	}

	return found;
}

void
search_box_highlight_all (SearchBox *search_box)
{
	if (!search_box->priv->current_editor)
		return;

	ianjuta_indicable_clear(IANJUTA_INDICABLE(search_box->priv->current_editor), NULL);
	if (search_box->priv->start_highlight != NULL) g_object_unref (search_box->priv->start_highlight);
	if (search_box->priv->end_highlight != NULL) g_object_unref (search_box->priv->end_highlight);
	search_box->priv->start_highlight = IANJUTA_EDITOR_CELL (ianjuta_editor_get_start_position (search_box->priv->current_editor, NULL));
	search_box->priv->end_highlight = IANJUTA_EDITOR_CELL (ianjuta_editor_get_end_position (search_box->priv->current_editor, NULL));

	if (search_box->priv->idle_id == 0)
	{
		search_box->priv->idle_id = g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,
		                                            (GSourceFunc)highlight_in_background,
		                                            search_box,
		                                            NULL);
	}
}

void 
search_box_clear_highlight (SearchBox * search_box)
{
	if (!search_box->priv->current_editor)
		return;

	ianjuta_indicable_clear(IANJUTA_INDICABLE(search_box->priv->current_editor), NULL);	
}

void 
search_box_toggle_highlight (SearchBox * search_box, gboolean status)
{
	if (!search_box->priv->current_editor)
		return;

	search_box->priv->highlight_all = status;
	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(search_box->priv->highlight_action),
	                             status);

	if (!status)
	{
		ianjuta_indicable_clear(IANJUTA_INDICABLE(search_box->priv->current_editor), NULL);
		g_clear_object (&search_box->priv->start_highlight);
		g_clear_object (&search_box->priv->end_highlight);
	}
	else
	{
		search_box_highlight_all (search_box);
	}
}

void 
search_box_toggle_case_sensitive (SearchBox * search_box, gboolean status)
{	
	if (!search_box->priv->current_editor)
		return;
	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(search_box->priv->case_action),
	                             status);
	                               
	search_box->priv->case_sensitive = status;
	if (search_box->priv->highlight_all) search_box_highlight_all (search_box);
}

void
search_box_toggle_regex (SearchBox * search_box, gboolean status)
{
	if (!search_box->priv->current_editor)
		return;

	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(search_box->priv->regex_action),
	                             status);
	
	search_box->priv->regex_mode = status;
	if (search_box->priv->highlight_all) search_box_highlight_all (search_box);
}

static void
on_search_box_entry_changed (GtkWidget * widget, SearchBox * search_box)
{
	if (!search_box->priv->regex_mode)
	{
		GtkEntryBuffer* buffer = gtk_entry_get_buffer (GTK_ENTRY(widget));
		if (gtk_entry_buffer_get_length (buffer))
			search_box_incremental_search (search_box, TRUE, FALSE, TRUE);
		else
		{
			/* clear selection */
			IAnjutaIterable* cursor = 
				ianjuta_editor_get_position (IANJUTA_EDITOR (search_box->priv->current_editor),
				                             NULL);
			ianjuta_editor_selection_set (IANJUTA_EDITOR_SELECTION (search_box->priv->current_editor),
			                              cursor,
			                              cursor,
			                              FALSE, NULL);
		}
	}

	if (search_box->priv->highlight_all) search_box_highlight_all (search_box);
}

static void
search_box_forward_search (SearchBox * search_box, GtkWidget* widget)
{
	search_box_incremental_search (search_box, TRUE, TRUE, TRUE);
}

static void
on_search_box_backward_search (GtkWidget * widget, SearchBox * search_box)
{
	search_box_incremental_search (search_box, FALSE, TRUE, TRUE);
}

static gboolean
search_box_replace (SearchBox * search_box, GtkWidget * widget,
                    gboolean undo /* treat as undo action */)
{

	IAnjutaEditorSelection* selection;
	gchar * selection_text;
	gboolean replace_successful = FALSE;

	const gchar* replace_text = gtk_entry_get_text (GTK_ENTRY (search_box->priv->replace_entry));
	const gchar* search_text = gtk_entry_get_text (GTK_ENTRY (search_box->priv->search_entry));
		
	selection = IANJUTA_EDITOR_SELECTION (search_box->priv->current_editor);
	selection_text = ianjuta_editor_selection_get (selection, NULL);

	if (ianjuta_editor_selection_has_selection (selection, NULL))
	{
		if (search_box->priv->regex_mode)
		{
			GRegex * regex;
			gchar * replacement_text;
			gint start_pos, end_pos;
			GError * err = NULL;
			gboolean result = search_regex_in_text (search_text, selection_text, TRUE, &start_pos, &end_pos);
				
			if (result)
			{
				regex = g_regex_new (search_text, 0, 0, NULL);
				replacement_text = g_regex_replace(regex, selection_text, strlen(selection_text), 0, 
													replace_text, 0, &err);
				if (err)
				{
					g_message ("%s",err->message);
					g_error_free (err);
					g_regex_unref(regex);
				}
                else
				{
					if (undo)
						ianjuta_document_begin_undo_action (IANJUTA_DOCUMENT (selection), NULL);
					ianjuta_editor_selection_replace (selection, replacement_text, strlen(replacement_text), NULL);
					if (undo)
						ianjuta_document_end_undo_action (IANJUTA_DOCUMENT (selection), NULL);
 
					replace_successful = TRUE;
				}

				if (regex)
					g_regex_unref(regex);
				if (replacement_text)			
					g_free(replacement_text);
			}
		}
		else if ((search_box->priv->case_sensitive && g_str_equal (selection_text, search_text)) ||
		         (!search_box->priv->case_sensitive && strcasecmp (selection_text, search_text) == 0))
		{
			if (undo)
				ianjuta_document_begin_undo_action (IANJUTA_DOCUMENT (selection), NULL);
			ianjuta_editor_selection_replace (selection, replace_text, strlen(replace_text), NULL);
			if (undo)
				ianjuta_document_end_undo_action (IANJUTA_DOCUMENT (selection), NULL);

			replace_successful = TRUE;
		}
	
		g_free(selection_text);
	}
		
	return replace_successful;

}

static void
on_replace_activated (GtkWidget* widget, SearchBox* search_box)
{
	gboolean successful_replace;

	if (!search_box->priv->current_editor)
		return;

	/* Either replace search-term or try to move search forward to next occurence */

	successful_replace = search_box_replace (search_box, widget, TRUE);

	if (successful_replace)
	{
		search_box_forward_search (search_box, widget);
	}	
}

static void 
do_popup_menu (GtkWidget* widget, GdkEventButton *event, SearchBox* search_box)
{
	int button, event_time;

	if (event)
	{
		button = event->button;
		event_time = event->time;
	}
	else
	{
		button = 0;
		event_time = gtk_get_current_event_time ();
	}

	if (!gtk_menu_get_attach_widget(GTK_MENU (search_box->priv->popup_menu)))
		gtk_menu_attach_to_widget (GTK_MENU (search_box->priv->popup_menu), widget, NULL);

	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(search_box->priv->case_action), search_box->priv->case_sensitive);
	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(search_box->priv->regex_action), search_box->priv->regex_mode);
	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(search_box->priv->highlight_action), search_box->priv->highlight_all);

	gtk_menu_popup (GTK_MENU (search_box->priv->popup_menu), NULL, NULL, NULL, NULL,
                  button, event_time);

}

static gboolean
on_search_entry_icon_pressed (GtkWidget* widget, GtkEntryIconPosition pos, 
								GdkEvent* event, SearchBox * search_box)
{
	do_popup_menu (widget, (GdkEventButton*)event, search_box);
	return TRUE;
}

static gboolean
on_search_entry_popup_menu (GtkWidget* widget, SearchBox* search_box)
{
	do_popup_menu (widget, NULL, search_box);
	return TRUE;
}

static void
on_replace_all_activated (GtkWidget* widget, SearchBox* search_box)
{
	IAnjutaIterable* cursor;

	if (!search_box->priv->current_editor)
		return;

	/* Cache current position and search from begin */
	cursor = ianjuta_editor_get_position (IANJUTA_EDITOR (search_box->priv->current_editor),
	                                      NULL);
	ianjuta_editor_goto_start (IANJUTA_EDITOR (search_box->priv->current_editor), NULL);
	
	/* Replace all instances of search_entry with replace_entry text */
	ianjuta_document_begin_undo_action (IANJUTA_DOCUMENT (search_box->priv->current_editor), NULL);
	while (search_box_incremental_search (search_box, TRUE, TRUE, FALSE))
	{
		search_box_replace (search_box, widget, FALSE);
	}
	ianjuta_document_end_undo_action (IANJUTA_DOCUMENT (search_box->priv->current_editor), NULL);

	/* Back to cached position */
	ianjuta_editor_selection_set (IANJUTA_EDITOR_SELECTION (search_box->priv->current_editor),
	                              cursor, cursor, TRUE, NULL);
	g_object_unref (cursor);
}

static void
search_box_init (SearchBox *search_box)
{
	search_box->priv = GET_PRIVATE(search_box);
	GList* focus_chain = NULL;
	
	/* Button images */
	GtkWidget* close = 
		gtk_image_new_from_stock (GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
	
	/* Searching */
	search_box->priv->search_entry = gtk_entry_new();
	gtk_widget_set_tooltip_text (search_box->priv->search_entry,
	                             _("Use the context menu of the \"Find\" icon for more search options"));
	g_signal_connect_swapped (G_OBJECT (search_box->priv->search_entry), "activate", 
	                          G_CALLBACK (search_box_forward_search),
	                          search_box);
	g_signal_connect (G_OBJECT (search_box), "key-press-event",
					  G_CALLBACK (on_search_box_key_pressed),
					  search_box);
	g_signal_connect (G_OBJECT (search_box->priv->search_entry), "changed",
					  G_CALLBACK (on_search_box_entry_changed),
					  search_box);
	g_signal_connect (G_OBJECT (search_box->priv->search_entry), "focus-out-event",
					  G_CALLBACK (on_search_focus_out),
					  search_box);
	g_signal_connect (G_OBJECT (search_box->priv->search_entry), "icon-press", 
					  G_CALLBACK (on_search_entry_icon_pressed),
					  search_box);
	g_signal_connect (G_OBJECT (search_box->priv->search_entry), "popup-menu",
					  G_CALLBACK (on_search_entry_popup_menu),
					  search_box);	
	
	search_box->priv->close_button = gtk_button_new();
	gtk_button_set_image (GTK_BUTTON (search_box->priv->close_button), close);
	gtk_button_set_relief (GTK_BUTTON (search_box->priv->close_button), GTK_RELIEF_NONE);
	
	g_signal_connect (G_OBJECT (search_box->priv->close_button), "clicked",
					  G_CALLBACK (on_search_box_hide), search_box);
	
	/* Previous, Next Navigation */
	search_box->priv->next_button = gtk_button_new ();
	gtk_container_add (GTK_CONTAINER (search_box->priv->next_button),
	                   gtk_image_new_from_stock (GTK_STOCK_GO_FORWARD,
	                                             GTK_ICON_SIZE_BUTTON));
	gtk_button_set_relief (GTK_BUTTON (search_box->priv->next_button), GTK_RELIEF_NONE);
	g_signal_connect_swapped (G_OBJECT(search_box->priv->next_button), "clicked", 
	                          G_CALLBACK (search_box_forward_search), search_box);
	search_box->priv->previous_button = gtk_button_new ();
	gtk_container_add (GTK_CONTAINER (search_box->priv->previous_button),
	                   gtk_image_new_from_stock (GTK_STOCK_GO_BACK,
	                                             GTK_ICON_SIZE_BUTTON));
	gtk_button_set_relief (GTK_BUTTON (search_box->priv->previous_button), GTK_RELIEF_NONE);
	g_signal_connect (G_OBJECT(search_box->priv->previous_button), "clicked", 
					G_CALLBACK (on_search_box_backward_search), search_box);
	
	/* Goto line */
	search_box->priv->goto_entry = gtk_entry_new ();
	gtk_entry_set_width_chars (GTK_ENTRY (search_box->priv->goto_entry), LINE_ENTRY_WIDTH);
	gtk_entry_set_icon_from_stock (GTK_ENTRY (search_box->priv->goto_entry),
	                               GTK_ENTRY_ICON_SECONDARY,
	                               ANJUTA_STOCK_GOTO_LINE);
	g_signal_connect (G_OBJECT (search_box->priv->goto_entry), "activate", 
					  G_CALLBACK (on_goto_activated),
					  search_box);
	g_signal_connect (G_OBJECT (search_box->priv->goto_entry), "key-press-event",
					  G_CALLBACK (on_goto_key_pressed),
					  search_box);
	/* Replace */
	search_box->priv->replace_entry = gtk_entry_new();
	g_signal_connect (G_OBJECT (search_box->priv->replace_entry), "activate", 
					  G_CALLBACK (on_replace_activated),
					  search_box);
	
	search_box->priv->replace_button = gtk_button_new_with_label(_("Replace"));
	gtk_button_set_relief (GTK_BUTTON (search_box->priv->replace_button), GTK_RELIEF_NONE);
	g_signal_connect (G_OBJECT(search_box->priv->replace_button), "clicked", 
					G_CALLBACK (on_replace_activated), search_box);

	search_box->priv->replace_all_button = gtk_button_new_with_label(_("Replace all"));
	gtk_button_set_relief (GTK_BUTTON (search_box->priv->replace_all_button), GTK_RELIEF_NONE);
	g_signal_connect (G_OBJECT(search_box->priv->replace_all_button), "clicked", 
					G_CALLBACK (on_replace_all_activated), search_box);

	/* Popup Menu Options */
	search_box->priv->regex_mode = FALSE;
	search_box->priv->highlight_all = FALSE;
	search_box->priv->case_sensitive = FALSE;

	/* Highlight iterator */
	search_box->priv->start_highlight = NULL;
	search_box->priv->end_highlight = NULL;
	search_box->priv->idle_id = 0;
	
	/* Initialize search_box grid */
	search_box->priv->grid = gtk_grid_new();
	gtk_orientable_set_orientation (GTK_ORIENTABLE (search_box->priv->grid), 
									GTK_ORIENTATION_VERTICAL);
	gtk_grid_set_row_spacing (GTK_GRID (search_box->priv->grid), 5);
	
	/* Attach search elements to grid */
	gtk_grid_attach (GTK_GRID (search_box->priv->grid), search_box->priv->goto_entry, 0, 0, 1, 1);
	
	gtk_grid_attach (GTK_GRID (search_box->priv->grid), search_box->priv->search_entry, 1, 0, 1, 1);
	
	gtk_grid_attach (GTK_GRID (search_box->priv->grid), search_box->priv->previous_button, 2, 0, 1, 1);
	gtk_grid_attach (GTK_GRID (search_box->priv->grid), search_box->priv->next_button, 3, 0, 1, 1);

	gtk_grid_attach_next_to (GTK_GRID (search_box->priv->grid),
	                         search_box->priv->close_button,
	                         search_box->priv->next_button,
	                         GTK_POS_RIGHT, 1, 1);
	gtk_widget_set_hexpand(search_box->priv->close_button, TRUE);
	gtk_widget_set_halign(search_box->priv->close_button,
	                      GTK_ALIGN_END);

	/* Add Replace elements to search box on 2nd level */
	gtk_grid_attach (GTK_GRID (search_box->priv->grid), search_box->priv->replace_entry, 1, 1, 1, 1);
	gtk_grid_attach (GTK_GRID (search_box->priv->grid), search_box->priv->replace_button, 2, 1, 1, 1);	
	gtk_grid_attach (GTK_GRID (search_box->priv->grid), search_box->priv->replace_all_button, 3, 1, 1, 1);		

	/* Expand search entries (a bit) */
	gtk_entry_set_width_chars (GTK_ENTRY (search_box->priv->search_entry), SEARCH_ENTRY_WIDTH);
	gtk_entry_set_width_chars (GTK_ENTRY (search_box->priv->replace_entry), SEARCH_ENTRY_WIDTH);

	/* Set nice icons */
	gtk_entry_set_icon_from_stock (GTK_ENTRY (search_box->priv->search_entry),
	                               GTK_ENTRY_ICON_PRIMARY,
	                               GTK_STOCK_FIND);
	gtk_entry_set_icon_from_stock (GTK_ENTRY (search_box->priv->replace_entry),
	                               GTK_ENTRY_ICON_PRIMARY,
	                               GTK_STOCK_FIND_AND_REPLACE);
	
	/* Pack grid into search box */
	gtk_box_pack_start (GTK_BOX(search_box), search_box->priv->grid, TRUE, TRUE, 0);

	/* Set focus chain */
	focus_chain = g_list_prepend (focus_chain, search_box->priv->search_entry);
	focus_chain = g_list_prepend (focus_chain, search_box->priv->replace_entry);
	focus_chain = g_list_prepend (focus_chain, search_box->priv->next_button);
	focus_chain = g_list_prepend (focus_chain, search_box->priv->previous_button);
	focus_chain = g_list_prepend (focus_chain, search_box->priv->replace_button);
	focus_chain = g_list_prepend (focus_chain, search_box->priv->replace_all_button);
	focus_chain = g_list_prepend (focus_chain, search_box->priv->goto_entry);
	focus_chain = g_list_prepend (focus_chain, search_box->priv->close_button);
	focus_chain = g_list_prepend (focus_chain, search_box->priv->search_entry);
	focus_chain = g_list_reverse (focus_chain);
	gtk_container_set_focus_chain (GTK_CONTAINER (search_box->priv->grid),
	                               focus_chain);
	g_list_free (focus_chain);

	/* Show all children but keep the top box hidden. */
	gtk_widget_show_all (GTK_WIDGET (search_box));
	gtk_widget_hide (GTK_WIDGET (search_box));
}

static void
search_box_finalize (GObject *object)
{
	SearchBox *search_box = SEARCH_BOX (object);

	if (search_box->priv->idle_id) g_source_remove (search_box->priv->idle_id);
	if (search_box->priv->start_highlight) g_object_unref (search_box->priv->start_highlight);
	if (search_box->priv->end_highlight) g_object_unref (search_box->priv->end_highlight);

	G_OBJECT_CLASS (search_box_parent_class)->finalize (object);
}

static void
search_box_class_init (SearchBoxClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (SearchBoxPrivate));
	
	object_class->finalize = search_box_finalize;
}

GtkWidget*
search_box_new (AnjutaDocman *docman)
{
	SearchBox* search_box;
	AnjutaUI *ui;

	search_box = SEARCH_BOX (g_object_new (SEARCH_TYPE_BOX, "homogeneous",
											FALSE, NULL));

	g_signal_connect (G_OBJECT (docman), "document-changed",
					  G_CALLBACK (on_document_changed), search_box);

	search_box->priv->status = anjuta_shell_get_status (docman->shell, NULL);
	
	ui = anjuta_shell_get_ui (docman->shell, NULL);
	search_box->priv->popup_menu = gtk_ui_manager_get_widget (GTK_UI_MANAGER (ui),
											"/SearchboxPopup");
	g_assert (search_box->priv->popup_menu != NULL && GTK_IS_MENU (search_box->priv->popup_menu));

	search_box->priv->case_action = 
		gtk_ui_manager_get_action (GTK_UI_MANAGER (ui),
		                           "/SearchboxPopup/CaseCheck");

	search_box->priv->highlight_action =
		gtk_ui_manager_get_action (GTK_UI_MANAGER (ui),
		                           "/SearchboxPopup/HighlightAll");
	search_box->priv->regex_action = 
		gtk_ui_manager_get_action (GTK_UI_MANAGER (ui),
		                           "/SearchboxPopup/RegexSearch");

	g_signal_connect (search_box->priv->popup_menu, "deactivate",
					  G_CALLBACK (gtk_widget_hide), NULL);

	return GTK_WIDGET (search_box);
}

void
search_box_fill_search_focus (SearchBox* search_box, gboolean on_replace)
{
	IAnjutaEditor* te = search_box->priv->current_editor;

	if (IANJUTA_IS_EDITOR (te) && !search_box->priv->regex_mode)
	{
		gchar *buffer;

		buffer = ianjuta_editor_selection_get (IANJUTA_EDITOR_SELECTION (te), NULL);
		if (buffer != NULL)
		{
			g_strstrip (buffer);
			if (*buffer != 0)
			{
			
				gtk_entry_set_text (GTK_ENTRY (search_box->priv->search_entry), buffer);
				gtk_editable_select_region (GTK_EDITABLE (search_box->priv->search_entry), 0, -1);

			}
			g_free (buffer);
		}
	}

	/* Toggle replace level (replace entry, replace buttons) of search box */
	search_box_set_replace (search_box, on_replace);

	gtk_widget_grab_focus (search_box->priv->search_entry);
}

void
search_box_grab_line_focus (SearchBox* search_box)
{
	gtk_widget_grab_focus (search_box->priv->goto_entry);
}

void
search_box_hide (SearchBox* search_box)
{
	gtk_widget_hide (GTK_WIDGET (search_box));
	search_box_set_entry_color (search_box, TRUE);
	if (search_box->priv->current_editor)
	{
		ianjuta_document_grab_focus (IANJUTA_DOCUMENT (search_box->priv->current_editor),
		                             NULL);
	}
}

void
search_box_set_replace (SearchBox* search_box, gboolean replace)
{
	if (replace) 
	{
		gtk_widget_show (search_box->priv->replace_entry);
		gtk_widget_show (search_box->priv->replace_button);
		gtk_widget_show (search_box->priv->replace_all_button);		
	}
	else 
	{
		gtk_widget_hide (search_box->priv->replace_entry);
		gtk_widget_hide (search_box->priv->replace_button);
		gtk_widget_hide (search_box->priv->replace_all_button);
	}	
}

const gchar* search_box_get_search_string (SearchBox* search_box)
{
	g_return_val_if_fail (search_box != NULL && SEARCH_IS_BOX(search_box), NULL);
	
	return gtk_entry_get_text (GTK_ENTRY (search_box->priv->search_entry));
}

void search_box_set_search_string (SearchBox* search_box, const gchar* search)
{
	g_return_if_fail (search_box != NULL && SEARCH_IS_BOX(search_box));

	gtk_entry_set_text (GTK_ENTRY (search_box->priv->search_entry), search);
}

const gchar* search_box_get_replace_string (SearchBox* search_box)
{
	g_return_val_if_fail (search_box != NULL && SEARCH_IS_BOX(search_box), NULL);
	
	return gtk_entry_get_text (GTK_ENTRY (search_box->priv->replace_entry));

}

void search_box_set_replace_string (SearchBox* search_box, const gchar* replace)
{
	g_return_if_fail (search_box != NULL && SEARCH_IS_BOX(search_box));

	gtk_entry_set_text (GTK_ENTRY (search_box->priv->replace_entry), replace);
}

void
search_box_session_load (SearchBox* search_box, AnjutaSession* session)
{
	g_return_if_fail (search_box != NULL && SEARCH_IS_BOX(search_box));

	search_box->priv->case_sensitive = anjuta_session_get_int (session, "Search Box", "Case Sensitive") ? TRUE : FALSE;
	search_box->priv->regex_mode = anjuta_session_get_int (session, "Search Box", "Regular Expression") ? TRUE : FALSE;
	search_box->priv->highlight_all = anjuta_session_get_int (session, "Search Box", "Highlight Match") ? TRUE : FALSE;
}

void 
search_box_session_save (SearchBox* search_box, AnjutaSession* session)
{
	g_return_if_fail (search_box != NULL && SEARCH_IS_BOX(search_box));

	anjuta_session_set_int (session, "Search Box", "Case Sensitive", search_box->priv->case_sensitive ? 1 : 0);
	anjuta_session_set_int (session, "Search Box", "Regular Expression", search_box->priv->regex_mode ? 1 : 0);
	anjuta_session_set_int (session, "Search Box", "Highlight Match", search_box->priv->highlight_all ? 1 : 0);
}
