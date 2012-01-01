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

typedef struct _SearchBoxPrivate SearchBoxPrivate;

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

	/* Search options popup menu */
	GtkWidget* popup_menu;
	gboolean case_sensitive;
	gboolean highlight_all;	
	gboolean regex_mode;
	gboolean highlight_complete;
	
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
	gtk_widget_hide (GTK_WIDGET (search_box));
}

static void
on_document_changed (AnjutaDocman* docman, IAnjutaDocument* doc,
					SearchBox* search_box)
{
	SearchBoxPrivate* private = GET_PRIVATE(search_box);

	if (!doc || !IANJUTA_IS_EDITOR (doc))
	{
		gtk_widget_hide (GTK_WIDGET (search_box));
		private->current_editor = NULL;
	}
	else
	{
		private->current_editor = IANJUTA_EDITOR (doc);
	}
}

static void
on_goto_activated (GtkWidget* widget, SearchBox* search_box)
{
	SearchBoxPrivate* private = GET_PRIVATE(search_box);
	const gchar* str_line = gtk_entry_get_text (GTK_ENTRY (private->goto_entry));
	
	gint line = atoi (str_line);
	if (line > 0)
	{
		ianjuta_editor_goto_line (private->current_editor, line, NULL);
	}
}

static void
search_box_set_entry_color (SearchBox* search_box, gboolean found)
{
	SearchBoxPrivate* private = GET_PRIVATE(search_box);
	if (!found)
	{
		GdkColor red;
		GdkColor white;

		/* FIXME: a11y and theme */

		gdk_color_parse ("#FF6666", &red);
		gdk_color_parse ("white", &white);

		gtk_widget_modify_base (private->search_entry,
				        GTK_STATE_NORMAL,
				        &red);
		gtk_widget_modify_text (private->search_entry,
				        GTK_STATE_NORMAL,
				        &white);
	}
	else
	{
		gtk_widget_modify_base (private->search_entry,
				        GTK_STATE_NORMAL,
				        NULL);
		gtk_widget_modify_text (private->search_entry,
				        GTK_STATE_NORMAL,
				        NULL);
	}
}

static gboolean
on_goto_key_pressed (GtkWidget* entry, GdkEventKey* event, SearchBox* search_box)
{
	SearchBoxPrivate* private = GET_PRIVATE(search_box);
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
			gtk_widget_hide (GTK_WIDGET (search_box));
			search_box_set_entry_color (search_box, TRUE);
			if (private->current_editor)
			{
				ianjuta_document_grab_focus (IANJUTA_DOCUMENT (private->current_editor), 
											 NULL);
			}
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
on_entry_key_pressed (GtkWidget* entry, GdkEventKey* event, SearchBox* search_box)
{

	SearchBoxPrivate* private = GET_PRIVATE(search_box);
	switch (event->keyval)
	{
		case GDK_KEY_Escape:
		{
			gtk_widget_hide (GTK_WIDGET (search_box));
			search_box_set_entry_color (search_box, TRUE);
			if (private->current_editor)
			{
				ianjuta_document_grab_focus (IANJUTA_DOCUMENT (private->current_editor), 
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
	SearchBoxPrivate* private = GET_PRIVATE(search_box);
	anjuta_status_pop (private->status);

	return FALSE;
}



static gboolean
incremental_regex_search (const gchar* search_entry, const gchar* editor_text, gint * start_pos, gint * end_pos, gboolean search_forward)
{
	GRegex * regex;
	GMatchInfo *match_info;
	gboolean result;
	GError * err = NULL;

	regex = g_regex_new (search_entry, 0, 0, &err);
	if (err)
	{
		g_message ("%s",err->message);
		g_error_free (err);
		g_regex_unref(regex);
		return FALSE;
	}

	result = g_regex_match (regex, editor_text, 0, &match_info);

	if (result)
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

	return result;

}

gboolean
search_box_incremental_search (SearchBox* search_box, gboolean search_forward,
                               gboolean wrap)
{
	IAnjutaIterable* real_start;
	IAnjutaEditorCell* search_start;
	IAnjutaEditorCell* search_end;
	IAnjutaEditorCell* result_start;
	IAnjutaEditorCell* result_end;
	IAnjutaEditorSelection* selection;
	SearchBoxPrivate* private = GET_PRIVATE (search_box);

	const gchar* search_text = gtk_entry_get_text (GTK_ENTRY (private->search_entry));

	gboolean found = FALSE;

	if (!private->current_editor || !search_text || !strlen (search_text))
		return FALSE;

	selection = IANJUTA_EDITOR_SELECTION (private->current_editor);

	if (ianjuta_editor_selection_has_selection (selection, NULL))
	{
		search_start =
			IANJUTA_EDITOR_CELL (ianjuta_editor_selection_get_start (selection, NULL));
	}
	else
	{
		search_start =
			IANJUTA_EDITOR_CELL (ianjuta_editor_get_position (private->current_editor,
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
		search_end = IANJUTA_EDITOR_CELL (ianjuta_editor_get_position (private->current_editor,
	                                                                   NULL));
		ianjuta_iterable_last (IANJUTA_ITERABLE (search_end), NULL);
	}
	else
	{
		search_end = search_start;
		search_start = IANJUTA_EDITOR_CELL (ianjuta_editor_get_position (private->current_editor,
	                                                                     NULL));
		ianjuta_iterable_first (IANJUTA_ITERABLE (search_start), NULL);
	}

	/* When there's a selection, if forward, set search start and end
     * to match end and editor end, respectively, or if backward to
     * editor start and match start, respectively. If forward and
     * selection starts with a match, look for next match.
	 */
	if (ianjuta_editor_selection_has_selection (selection,
	                                            NULL))
	{
		IAnjutaIterable* selection_start =
			ianjuta_editor_selection_get_start (selection, NULL);

		gchar* selected_text =
			ianjuta_editor_selection_get (selection, NULL);

		gint start_pos, end_pos;
		gboolean selected_have_search_text = FALSE;

		if (private->regex_mode)
		{
			/* Always look for first match */
			if (incremental_regex_search (search_text, selected_text, &start_pos, &end_pos, TRUE))
			{
				selected_have_search_text = TRUE;
			}
		}
		else if (strlen (selected_text) >= strlen (search_text))
		{
			gchar* selected_text_case;
			gchar* search_text_case;

			if (private->case_sensitive)
			{
				selected_text_case = g_strdup (selected_text);
				search_text_case = g_strdup (search_text);
			}
			else
			{
				selected_text_case = g_utf8_casefold (selected_text, strlen (selected_text));
				search_text_case = g_utf8_casefold (search_text, strlen (search_text));
			}

			gchar* strstr = g_strstr_len (selected_text_case, -1, search_text_case);

			if (strstr)
			{
				start_pos = g_utf8_pointer_to_offset(selected_text_case, strstr);
				end_pos = g_utf8_pointer_to_offset(selected_text_case, strstr + strlen (search_text));
				selected_have_search_text = TRUE;
			}

			g_free (selected_text_case);
			g_free (search_text_case);
		}

		if (selected_have_search_text)
		{
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
		}

		g_free (selected_text);

		g_object_unref (selection_start);
	}

	gboolean result_set = FALSE;

	gint start_pos, end_pos;
	gchar * text_to_search;
	gboolean result;

	if (!found)
	{
		/* Try searching in current position */
		if (private->regex_mode)
		{
			text_to_search = ianjuta_editor_get_text (private->current_editor,
				                                      IANJUTA_ITERABLE (search_start),
				                                      IANJUTA_ITERABLE (search_end), NULL);

			result = incremental_regex_search (search_text, text_to_search, &start_pos, &end_pos, search_forward);

			start_pos += ianjuta_iterable_get_position(IANJUTA_ITERABLE (search_start), NULL);
			end_pos += ianjuta_iterable_get_position(IANJUTA_ITERABLE (search_start), NULL);

			if (result && start_pos >= 0)
			{
				result_start = IANJUTA_EDITOR_CELL (ianjuta_editor_get_start_position (private->current_editor,
					                                                                   NULL));
				result_end = IANJUTA_EDITOR_CELL (ianjuta_editor_get_start_position (private->current_editor,
					                                                                 NULL));

				if (ianjuta_iterable_set_position(IANJUTA_ITERABLE(result_start), start_pos, NULL) &&
					ianjuta_iterable_set_position(IANJUTA_ITERABLE(result_end), end_pos, NULL))
				{
					found = TRUE;
				}

				if (!found)
				{
					g_object_unref(result_start);
					g_object_unref(result_end);
				}
			}

			g_free(text_to_search);
		}
		else
		{
			if (search_forward)
			{
				if (ianjuta_editor_search_forward (IANJUTA_EDITOR_SEARCH (private->current_editor),
					                               search_text, private->case_sensitive,
					                               search_start, search_end,
					                               &result_start,
					                               &result_end, NULL))
				{
					found = TRUE;
				}
			}
			else
			{
				if (ianjuta_editor_search_backward (IANJUTA_EDITOR_SEARCH (private->current_editor),
					                                search_text, private->case_sensitive,
					                                search_end, search_start,
					                                &result_start,
					                                &result_end, NULL))
				{
					found = TRUE;
				}
			}
		}
	}

	if (found)
	{
		anjuta_status_pop (ANJUTA_STATUS (private->status));
	}
	else if (wrap)
	{
		/* Try to wrap if not found */
		ianjuta_iterable_first (IANJUTA_ITERABLE (search_start), NULL);
		ianjuta_iterable_last (IANJUTA_ITERABLE (search_end), NULL);

		/* Try to search again */
		if (private->regex_mode)
		{
			text_to_search = ianjuta_editor_get_text (private->current_editor,
			                                          IANJUTA_ITERABLE(search_start),
			                                          IANJUTA_ITERABLE(search_end), NULL);

			result = incremental_regex_search (search_text, text_to_search, &start_pos, &end_pos, search_forward);

			start_pos += ianjuta_iterable_get_position(IANJUTA_ITERABLE(search_start), NULL);
			end_pos += ianjuta_iterable_get_position(IANJUTA_ITERABLE(search_start), NULL);

			if (result && start_pos >= 0)
			{
				result_start = IANJUTA_EDITOR_CELL (ianjuta_editor_get_start_position (private->current_editor,
				                                                                       NULL));
				result_end = IANJUTA_EDITOR_CELL (ianjuta_editor_get_start_position (private->current_editor,
				                                                                     NULL));

				if (ianjuta_iterable_set_position(IANJUTA_ITERABLE(result_start), start_pos, NULL) &&
				    ianjuta_iterable_set_position(IANJUTA_ITERABLE(result_end), end_pos, NULL))
				{
					result_set = TRUE;
				}

				if (!result_set)
				{
					g_object_unref(result_start);
					g_object_unref(result_end);
				}
			}
		}
		else
		{
			if (search_forward)
			{
				if (ianjuta_editor_search_forward (IANJUTA_EDITOR_SEARCH (private->current_editor),
				                                   search_text, private->case_sensitive,
				                                   search_start, search_end,
				                                   &result_start,
				                                   &result_end, NULL))
				{
					result_set = TRUE;
				}
			}
			else
			{
				if (ianjuta_editor_search_backward (IANJUTA_EDITOR_SEARCH (private->current_editor),
				                                    search_text, private->case_sensitive,
				                                    search_end, search_start,
				                                    &result_start,
				                                    &result_end, NULL))
				{
					result_set = TRUE;
				}
			}
		}

		/* Check if successful */
		if (result_set)
		{
			if (ianjuta_iterable_compare (IANJUTA_ITERABLE (result_start),
			                              real_start, NULL) != 0)
			{
				found = TRUE;
				anjuta_status_pop (private->status);
				if (search_forward)
				{
					anjuta_status_push (private->status,
					                    _("Search for \"%s\" reached the end and was continued at the top."), search_text);
				}
				else
				{
					anjuta_status_push (private->status,
					                    _("Search for \"%s\" reached top and was continued at the bottom."), search_text);
				}
			}
			else if (ianjuta_editor_selection_has_selection (selection, NULL))
			{
				anjuta_status_pop (private->status);
				if (search_forward)
				{
					anjuta_status_push (private->status,
						                _("Search for \"%s\" reached the end and was continued at the top but no new match was found."), search_text);
				}
				else
				{
					anjuta_status_push (private->status,
						                _("Search for \"%s\" reached top and was continued at the bottom but no new match was found."), search_text);
				}
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

void 
search_box_clear_highlight (SearchBox * search_box)
{
	SearchBoxPrivate* private = GET_PRIVATE(search_box);
	
	if (!private->current_editor)
		return;

	ianjuta_indicable_clear(IANJUTA_INDICABLE(private->current_editor), NULL);	
	private->highlight_complete = FALSE;
}

void 
search_box_toggle_highlight (SearchBox * search_box, gboolean status)
{
	SearchBoxPrivate* private = GET_PRIVATE(search_box);

	if (!private->current_editor)
		return;

	private->highlight_all = status;

	if (!status)
	{
		ianjuta_indicable_clear(IANJUTA_INDICABLE(private->current_editor), NULL);
		private->highlight_complete = FALSE;
	}
}

void 
search_box_toggle_case_sensitive (SearchBox * search_box, gboolean status)
{
	SearchBoxPrivate* private = GET_PRIVATE(search_box);

	if (!private->current_editor)
		return;

	private->case_sensitive = status;
	search_box_clear_highlight(search_box);
}

void
search_box_toggle_regex (SearchBox * search_box, gboolean status)
{
	SearchBoxPrivate* private = GET_PRIVATE(search_box);

	if (!private->current_editor)
		return;

	private->regex_mode = status;
	search_box_clear_highlight(search_box);

}

static void
search_box_search_highlight_all (SearchBox * search_box, gboolean search_forward)
{
	IAnjutaEditorCell * highlight_start;
	IAnjutaEditorSelection * selection;
	gboolean entry_found;

	SearchBoxPrivate* private = GET_PRIVATE(search_box);
	
	highlight_start = NULL;
	ianjuta_indicable_clear(IANJUTA_INDICABLE(private->current_editor), NULL);

	/* Search through editor and highlight instances of search_entry */
	while ((entry_found = search_box_incremental_search (search_box, search_forward, TRUE)) == TRUE)
	{
		IAnjutaEditorCell * result_begin, * result_end;
		selection = IANJUTA_EDITOR_SELECTION (private->current_editor);

		result_begin = 
			IANJUTA_EDITOR_CELL (ianjuta_editor_selection_get_start (selection, NULL));	
		result_end = 
			IANJUTA_EDITOR_CELL (ianjuta_editor_selection_get_end (selection, NULL));

		if (!highlight_start)
		{
			highlight_start = 
				IANJUTA_EDITOR_CELL (ianjuta_iterable_clone (IANJUTA_ITERABLE (result_begin), NULL));
		} 
		else if (ianjuta_iterable_compare (IANJUTA_ITERABLE (result_begin),
											IANJUTA_ITERABLE (highlight_start), NULL) == 0)
		{
			g_object_unref (result_begin);
			g_object_unref (result_end);
			g_object_unref (highlight_start);
			highlight_start = NULL;
			break;
		}

		ianjuta_indicable_set(IANJUTA_INDICABLE(private->current_editor), 
								IANJUTA_ITERABLE (result_begin), 
								IANJUTA_ITERABLE (result_end),
								IANJUTA_INDICABLE_IMPORTANT, NULL);
		g_object_unref (result_begin);
		g_object_unref (result_end);
	}
	if (highlight_start)
		g_object_unref (highlight_start);
	private->highlight_complete = TRUE;

}

static void
on_search_box_entry_changed (GtkWidget * widget, SearchBox * search_box)
{
	SearchBoxPrivate* private = GET_PRIVATE(search_box);

	if (!private->regex_mode)
	{
		GtkEntryBuffer* buffer = gtk_entry_get_buffer (GTK_ENTRY(widget));
		if (gtk_entry_buffer_get_length (buffer))
			search_box_incremental_search (search_box, TRUE, TRUE);
		else
		{
			/* clear selection */
			IAnjutaIterable* cursor = 
				ianjuta_editor_get_position (IANJUTA_EDITOR (private->current_editor),
				                             NULL);
			ianjuta_editor_selection_set (IANJUTA_EDITOR_SELECTION (private->current_editor),
			                              cursor,
			                              cursor,
			                              FALSE, NULL);
		}
	}
}

static void
search_box_forward_search (SearchBox * search_box, GtkWidget* widget)
{
	SearchBoxPrivate* private = GET_PRIVATE(search_box);

	if (private->highlight_all && !private->highlight_complete)
	{
		search_box_search_highlight_all (search_box, TRUE);
	}
	else
	{
		search_box_incremental_search (search_box, TRUE, TRUE);
	}

}

static void
on_search_box_backward_search (GtkWidget * widget, SearchBox * search_box)
{
	SearchBoxPrivate* private = GET_PRIVATE(search_box);

	if (private->highlight_all && !private->highlight_complete)
	{
		search_box_search_highlight_all (search_box, FALSE);
	}
	else
	{
		search_box_incremental_search (search_box, FALSE, TRUE);
	}
}

static gboolean
search_box_replace (SearchBox * search_box, GtkWidget * widget,
                    gboolean undo /* treat as undo action */)
{

	IAnjutaEditorSelection* selection;
	gchar * selection_text;
	gboolean replace_successful = FALSE;
	SearchBoxPrivate* private = GET_PRIVATE(search_box);

	const gchar* replace_text = gtk_entry_get_text (GTK_ENTRY (private->replace_entry));
	const gchar* search_text = gtk_entry_get_text (GTK_ENTRY (private->search_entry));
		
	selection = IANJUTA_EDITOR_SELECTION (private->current_editor);
	selection_text = ianjuta_editor_selection_get (selection, NULL);

	if (ianjuta_editor_selection_has_selection (selection, NULL))
	{
		if (private->regex_mode)
		{
			GRegex * regex;
			gchar * replacement_text;
			gint start_pos, end_pos;
			GError * err = NULL;
			gboolean result = incremental_regex_search (search_text, selection_text, &start_pos, &end_pos, TRUE);
				
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
		else if ((private->case_sensitive && g_str_equal (selection_text, search_text)) ||
		         (!private->case_sensitive && strcasecmp (selection_text, search_text) == 0))
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

	SearchBoxPrivate* private = GET_PRIVATE(search_box);

	if (!private->current_editor)
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
	SearchBoxPrivate* private = GET_PRIVATE(search_box);

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

	if (!gtk_menu_get_attach_widget(GTK_MENU (private->popup_menu)))
		gtk_menu_attach_to_widget (GTK_MENU (private->popup_menu), widget, NULL);
	gtk_menu_popup (GTK_MENU (private->popup_menu), NULL, NULL, NULL, NULL,
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

	SearchBoxPrivate* private = GET_PRIVATE(search_box);
	IAnjutaIterable* cursor;

	if (!private->current_editor)
		return;

	/* Cache current position and search from begin */
	cursor = ianjuta_editor_get_position (IANJUTA_EDITOR (private->current_editor),
	                                      NULL);
	ianjuta_editor_goto_start (IANJUTA_EDITOR (private->current_editor), NULL);
	
	/* Replace all instances of search_entry with replace_entry text */
	ianjuta_document_begin_undo_action (IANJUTA_DOCUMENT (private->current_editor), NULL);
	while (search_box_incremental_search (search_box, TRUE, FALSE))
	{
		search_box_replace (search_box, widget, FALSE);
	}
	ianjuta_document_end_undo_action (IANJUTA_DOCUMENT (private->current_editor), NULL);

	/* Back to cached position */
	ianjuta_editor_selection_set (IANJUTA_EDITOR_SELECTION (private->current_editor),
	                              cursor, cursor, TRUE, NULL);
	g_object_unref (cursor);
}

static void
search_box_init (SearchBox *object)
{
	SearchBoxPrivate* private = GET_PRIVATE(object);
	GList* focus_chain = NULL;
	
	/* Button images */
	GtkWidget* close = 
		gtk_image_new_from_stock (GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
	
	/* Searching */
	private->search_entry = gtk_entry_new();
	gtk_widget_set_tooltip_text (private->search_entry,
	                             _("Use the context menu of the \"Find\" icon for more search options"));
	g_signal_connect_swapped (G_OBJECT (private->search_entry), "activate", 
	                          G_CALLBACK (search_box_forward_search),
	                          object);
	g_signal_connect (G_OBJECT (private->search_entry), "key-press-event",
					  G_CALLBACK (on_entry_key_pressed),
					  object);
	g_signal_connect (G_OBJECT (private->search_entry), "changed",
					  G_CALLBACK (on_search_box_entry_changed),
					  object);
	g_signal_connect (G_OBJECT (private->search_entry), "focus-out-event",
					  G_CALLBACK (on_search_focus_out),
					  object);
	g_signal_connect (G_OBJECT (private->search_entry), "icon-press", 
					  G_CALLBACK (on_search_entry_icon_pressed),
					  object);
	g_signal_connect (G_OBJECT (private->search_entry), "popup-menu",
					  G_CALLBACK (on_search_entry_popup_menu),
					  object);	
	
	private->close_button = gtk_button_new();
	gtk_button_set_image (GTK_BUTTON (private->close_button), close);
	gtk_button_set_relief (GTK_BUTTON (private->close_button), GTK_RELIEF_NONE);
	
	g_signal_connect (G_OBJECT (private->close_button), "clicked",
					  G_CALLBACK (on_search_box_hide), object);
	
	/* Previous, Next Navigation */
	private->next_button = gtk_button_new ();
	gtk_container_add (GTK_CONTAINER (private->next_button),
	                   gtk_image_new_from_stock (GTK_STOCK_GO_FORWARD,
	                                             GTK_ICON_SIZE_BUTTON));
	gtk_button_set_relief (GTK_BUTTON (private->next_button), GTK_RELIEF_NONE);
	g_signal_connect_swapped (G_OBJECT(private->next_button), "clicked", 
	                          G_CALLBACK (search_box_forward_search), object);
	private->previous_button = gtk_button_new ();
	gtk_container_add (GTK_CONTAINER (private->previous_button),
	                   gtk_image_new_from_stock (GTK_STOCK_GO_BACK,
	                                             GTK_ICON_SIZE_BUTTON));
	gtk_button_set_relief (GTK_BUTTON (private->previous_button), GTK_RELIEF_NONE);
	g_signal_connect (G_OBJECT(private->previous_button), "clicked", 
					G_CALLBACK (on_search_box_backward_search), object);
	
	/* Goto line */
	private->goto_entry = gtk_entry_new ();
	gtk_entry_set_width_chars (GTK_ENTRY (private->goto_entry), LINE_ENTRY_WIDTH);
	gtk_entry_set_icon_from_stock (GTK_ENTRY (private->goto_entry),
	                               GTK_ENTRY_ICON_SECONDARY,
	                               ANJUTA_STOCK_GOTO_LINE);
	g_signal_connect (G_OBJECT (private->goto_entry), "activate", 
					  G_CALLBACK (on_goto_activated),
					  object);
	g_signal_connect (G_OBJECT (private->goto_entry), "key-press-event",
					  G_CALLBACK (on_goto_key_pressed),
					  object);
	/* Replace */
	private->replace_entry = gtk_entry_new();
	g_signal_connect (G_OBJECT (private->replace_entry), 
					"key-press-event", G_CALLBACK (on_entry_key_pressed),
 					object);
	g_signal_connect (G_OBJECT (private->replace_entry), "activate", 
					  G_CALLBACK (on_replace_activated),
					  object);
	
	private->replace_button = gtk_button_new_with_label(_("Replace"));
	gtk_button_set_relief (GTK_BUTTON (private->replace_button), GTK_RELIEF_NONE);
	g_signal_connect (G_OBJECT(private->replace_button), "clicked", 
					G_CALLBACK (on_replace_activated), object);

	private->replace_all_button = gtk_button_new_with_label(_("Replace all"));
	gtk_button_set_relief (GTK_BUTTON (private->replace_all_button), GTK_RELIEF_NONE);
	g_signal_connect (G_OBJECT(private->replace_all_button), "clicked", 
					G_CALLBACK (on_replace_all_activated), object);

	/* Popup Menu Options */
	private->regex_mode = FALSE;
	private->highlight_all = FALSE;
	private->case_sensitive = FALSE;
	private->highlight_complete = FALSE;

	/* Initialize search_box grid */
	private->grid = gtk_grid_new();
	gtk_orientable_set_orientation (GTK_ORIENTABLE (private->grid), 
									GTK_ORIENTATION_VERTICAL);
	gtk_grid_set_row_spacing (GTK_GRID (private->grid), 5);
	
	/* Attach search elements to grid */
	gtk_grid_attach (GTK_GRID (private->grid), private->goto_entry, 0, 0, 1, 1);
	
	gtk_grid_attach (GTK_GRID (private->grid), private->search_entry, 1, 0, 1, 1);
	
	gtk_grid_attach (GTK_GRID (private->grid), private->previous_button, 2, 0, 1, 1);
	gtk_grid_attach (GTK_GRID (private->grid), private->next_button, 3, 0, 1, 1);

	gtk_grid_attach (GTK_GRID (private->grid), private->close_button, 4, 0, 1, 1);

	/* Add Replace elements to search box on 2nd level */
	gtk_grid_attach (GTK_GRID (private->grid), private->replace_entry, 1, 1, 1, 1);
	gtk_grid_attach (GTK_GRID (private->grid), private->replace_button, 2, 1, 1, 1);	
	gtk_grid_attach (GTK_GRID (private->grid), private->replace_all_button, 3, 1, 1, 1);		

	/* Expand search entries (a bit) */
	gtk_entry_set_width_chars (GTK_ENTRY (private->search_entry), SEARCH_ENTRY_WIDTH);
	gtk_entry_set_width_chars (GTK_ENTRY (private->replace_entry), SEARCH_ENTRY_WIDTH);

	/* Set nice icons */
	gtk_entry_set_icon_from_stock (GTK_ENTRY (private->search_entry),
	                               GTK_ENTRY_ICON_PRIMARY,
	                               GTK_STOCK_FIND);
	gtk_entry_set_icon_from_stock (GTK_ENTRY (private->replace_entry),
	                               GTK_ENTRY_ICON_PRIMARY,
	                               GTK_STOCK_FIND_AND_REPLACE);
	
	/* Pack grid into search box */
	gtk_box_pack_start (GTK_BOX(object), private->grid, TRUE, TRUE, 0);

	/* Set focus chain */
	focus_chain = g_list_prepend (focus_chain, private->search_entry);
	focus_chain = g_list_prepend (focus_chain, private->replace_entry);
	focus_chain = g_list_prepend (focus_chain, private->next_button);
	focus_chain = g_list_prepend (focus_chain, private->previous_button);
	focus_chain = g_list_prepend (focus_chain, private->replace_button);
	focus_chain = g_list_prepend (focus_chain, private->replace_all_button);
	focus_chain = g_list_prepend (focus_chain, private->goto_entry);
	focus_chain = g_list_prepend (focus_chain, private->close_button);
	focus_chain = g_list_prepend (focus_chain, private->search_entry);
	focus_chain = g_list_reverse (focus_chain);
	gtk_container_set_focus_chain (GTK_CONTAINER (private->grid),
	                               focus_chain);
	g_list_free (focus_chain);
	
	gtk_widget_show_all (GTK_WIDGET (object));
	
}

static void
search_box_finalize (GObject *object)
{

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
	GtkWidget* search_box;
	SearchBoxPrivate* private;
	AnjutaUI *ui;

	search_box = GTK_WIDGET (g_object_new (SEARCH_TYPE_BOX, "homogeneous",
											FALSE, NULL));

	g_signal_connect (G_OBJECT (docman), "document-changed",
					  G_CALLBACK (on_document_changed), search_box);

	private = GET_PRIVATE (search_box);
	private->status = anjuta_shell_get_status (docman->shell, NULL);
	
	ui = anjuta_shell_get_ui (docman->shell, NULL);
	private->popup_menu = gtk_ui_manager_get_widget (GTK_UI_MANAGER (ui),
											"/SearchboxPopup");
	g_assert (private->popup_menu != NULL && GTK_IS_MENU (private->popup_menu));

	g_signal_connect (private->popup_menu, "deactivate",
					  G_CALLBACK (gtk_widget_hide), NULL);

	return search_box;
}

void
search_box_fill_search_focus (SearchBox* search_box, gboolean on_replace)
{

	SearchBoxPrivate* private = GET_PRIVATE(search_box);
	IAnjutaEditor* te = private->current_editor;

	if (IANJUTA_IS_EDITOR (te) && !private->regex_mode)
	{
		gchar *buffer;

		buffer = ianjuta_editor_selection_get (IANJUTA_EDITOR_SELECTION (te), NULL);
		if (buffer != NULL)
		{
			g_strstrip (buffer);
			if (*buffer != 0)
			{
			
				gtk_entry_set_text (GTK_ENTRY (private->search_entry), buffer);
				gtk_editable_select_region (GTK_EDITABLE (private->search_entry), 0, -1);

			}
			g_free (buffer);
		}
	}

	/* Toggle replace level (replace entry, replace buttons) of search box */
	search_box_set_replace (search_box, on_replace);

	gtk_widget_grab_focus (private->search_entry);
}

void
search_box_grab_line_focus (SearchBox* search_box)
{
	SearchBoxPrivate* private = GET_PRIVATE(search_box);
	gtk_widget_grab_focus (private->goto_entry);
}

void
search_box_set_replace (SearchBox* object, gboolean replace)
{

	SearchBoxPrivate* private = GET_PRIVATE(object);

	if (replace) 
	{
		gtk_widget_show (private->replace_entry);
		gtk_widget_show (private->replace_button);
		gtk_widget_show (private->replace_all_button);		
	}
	else 
	{
		gtk_widget_hide (private->replace_entry);
		gtk_widget_hide (private->replace_button);
		gtk_widget_hide (private->replace_all_button);
	}	
}
