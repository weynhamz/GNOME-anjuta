/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */

/***************************************************************************
 *            search_incremental.c
 *
 *  Mon Jan  5 21:17:56 2004
 *  Copyright  2004  Jean-Noel Guiheneuf
 *  jnoel@lotuscompounds.com
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
 

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include <gnome.h>

#include "anjuta.h"
#include "text_editor.h"

#define GTK
#undef PLAT_GTK
#define PLAT_GTK 1
#include "Scintilla.h"
#include "SciLexer.h"
#include "ScintillaWidget.h"

#include "search-replace_backend.h"
#include "search-replace.h"


static void
toolbar_find_start_over (SearchDirection direction);

static void
toolbar_find_clicked (SearchDirection direction);

void
on_toolbar_find_clicked (GtkButton * button, gpointer user_data);

static gint
incremental_search(TextEditor *te, gchar *string, SearchDirection direction);



static SearchReplace *sr = NULL;


static
incremental_save_retrieve_expr(gboolean save)
{
	static gboolean save_regex = FALSE;
	static gboolean save_greedy = FALSE;
	static gboolean save_ignore_case = FALSE;
	static gboolean save_whole_word = FALSE;
	static gboolean save_whole_line = FALSE;
	static gboolean save_word_start = FALSE;
		
	if (save)
	{
		save_regex = sr->search.expr.regex;
		save_greedy = sr->search.expr.greedy;
		save_ignore_case = sr->search.expr.ignore_case;
		save_whole_word = sr->search.expr.whole_word;
		save_whole_line = sr->search.expr.whole_line;
		save_word_start = sr->search.expr.word_start;
	}
	else
	{
		sr->search.expr.regex = save_regex;
		sr->search.expr.greedy = save_greedy;
		sr->search.expr.ignore_case = save_ignore_case;
		sr->search.expr.whole_word = save_whole_word;
		sr->search.expr.whole_line = save_whole_line;
		sr->search.expr.word_start = save_word_start;		
	}
}


static gint
incremental_search(TextEditor *te, gchar *string, SearchDirection direction)
{
	FileBuffer *fb;
	static MatchInfo *mi;
	Search *s;
		
	fb = file_buffer_new_from_te(te);
	s = &(sr->search);
	
	if (s->expr.search_str)
		g_free(s->expr.search_str);
	s->expr.search_str = g_strdup(string);
	
	if ((mi = get_next_match(fb, direction, &(s->expr))))
	{
		scintilla_send_message(SCINTILLA(fb->te->widgets.editor),
			SCI_SETSEL, mi->pos, (mi->pos + mi->len));
		return mi->pos;	
	}
	else
		return -1;
}


void
enter_selection_as_search_target(void)
{
	gchar *selectionText = NULL;

	if (sr == NULL)
		sr = create_search_replace_instance();
	
	selectionText = anjuta_get_current_selection ();
	if (selectionText != NULL && selectionText[0] != '\0')
	{
		sr->search.expr_history =
			update_string_list (sr->search.expr_history,
								selectionText, COMBO_LIST_LENGTH);
		entry_set_text_n_select (app->widgets.toolbar.main_toolbar.find_entry,
								 selectionText, FALSE);
	}
	if (selectionText != NULL)
		g_free (selectionText);
}


gboolean
on_toolbar_find_incremental_start (GtkEntry *entry, GdkEvent *e, gpointer user_data)
{
	gchar *string;
	const gchar *string1;
	TextEditor *te = anjuta_get_current_text_editor();
	
	if (sr == NULL)
		sr = create_search_replace_instance();
		
	if (!te) return FALSE;

	/* Updated find combo history */
	string1 = gtk_entry_get_text (GTK_ENTRY
				    (app->widgets.toolbar.main_toolbar.find_entry));

	if (string1 && strlen (string1) > 0)
	{
		string = g_strdup (string1);
		sr->search.expr_history = update_string_list (sr->search.expr_history, 
													string, COMBO_LIST_LENGTH);
		gtk_combo_set_popdown_strings (GTK_COMBO
						   (app->widgets.toolbar.main_toolbar.find_combo),
						   sr->search.expr_history);
		g_free (string);
	}
	/* Prepare to begin incremental search */	
	sr->search.incremental_pos = text_editor_get_current_position(te);
	sr->search.incremental_wrap = FALSE;
	return FALSE;
}

gboolean
on_toolbar_find_incremental_end (GtkEntry *entry,
	GdkEvent *e, gpointer user_data)
{
	sr->search.incremental_pos = -1;
	return FALSE;
}

void
on_toolbar_find_incremental (GtkEntry *entry, gpointer user_data)
{
	const gchar *entry_text;
	TextEditor *te;
	
	if (sr == NULL)
		sr = create_search_replace_instance();
	
	if (!(te = anjuta_get_current_text_editor()))
		return;
	if (sr->search.incremental_pos < 0)
		return;
	
	text_editor_goto_point (te, sr->search.incremental_pos);

	entry_text = gtk_entry_get_text (GTK_ENTRY
				    (app->widgets.toolbar.main_toolbar.find_entry));
	if (!entry_text || strlen(entry_text) < 1) return;
	
	toolbar_find_clicked (SD_FORWARD);
}


static void
toolbar_find_clicked (SearchDirection direction)
{
	TextEditor *te;
	const gchar *string;
	gint ret;
	gboolean search_wrap = FALSE;
	
	if (sr == NULL)
		sr = create_search_replace_instance();
	
	te = anjuta_get_current_text_editor ();
	if (!te)
		return;

	string = gtk_entry_get_text (GTK_ENTRY
								 (app->widgets.toolbar.main_toolbar.
							     find_entry));
	
	/* The 2 below is checked to make sure the wrapping is only done when
	   it is called by 'activate' (and not 'changed') signal */ 
	if (sr->search.incremental_pos >= 0 &&
		(sr->search.incremental_wrap == 2 ||
		 (sr->search.incremental_wrap == 1 )))
	{
		/* If incremental search wrap requested, so wrap it. */
		search_wrap = TRUE;
		sr->search.incremental_wrap = 2;
	}
	if (sr->search.incremental_pos >= 0)
	{
		/* If incremental search */
		incremental_save_retrieve_expr(TRUE); // Save parameters
		sr->search.expr.regex = FALSE;
		sr->search.expr.greedy = FALSE;
		sr->search.expr.ignore_case = TRUE;
		sr->search.expr.whole_word = FALSE;
		sr->search.expr.whole_line = FALSE;
		sr->search.expr.word_start = FALSE;
		
		ret = incremental_search(te, (gchar*)string, direction);
		
		incremental_save_retrieve_expr(FALSE); // Retieve parameters
	}
	else
	{
		/* Normal search */
		ret = incremental_search(te, (gchar*)string, direction);
	}
	if (ret < 0) {
		if (sr->search.incremental_pos < 0)
		{
			GtkWidget *dialog;
			// Dialog to be made HIG compliant.
			dialog = gtk_message_dialog_new (GTK_WINDOW (app->widgets.window),
											 GTK_DIALOG_DESTROY_WITH_PARENT,
											 GTK_MESSAGE_QUESTION,
											 GTK_BUTTONS_YES_NO,
					_("No matches. Wrap search around the document?"));
			if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
				toolbar_find_start_over (direction);
			gtk_widget_destroy (dialog);
		}
		else
		{
			if (search_wrap == FALSE)
			{
				anjuta_status(
				"Failling I-Search: '%s'. Press Enter or click Find to overwrap.",
				string);
				sr->search.incremental_wrap = TRUE;
				if (anjuta_preferences_get (ANJUTA_PREFERENCES (app->preferences),
											BEEP_ON_BUILD_COMPLETE))
					gdk_beep();
			}
			else
			{
				anjuta_status ("Failling Overwrapped I-Search: %s.", string);
			}
		}
	}
}


void
on_toolbar_find_clicked_cb (GtkButton * button, gpointer user_data)
{
	toolbar_find_clicked (SD_FORWARD);
}


static void
toolbar_find_start_over (SearchDirection direction)
{
	TextEditor *te = anjuta_get_current_text_editor();
	long length;
	
	length = aneditor_command(te->editor_id, ANE_GETLENGTH, 0, 0);

	if (direction != SD_BACKWARD)
		/* search from doc start */
		text_editor_goto_point (te, 0);
	else
		/* search from doc end */
		text_editor_goto_point (te, length);

	toolbar_find_clicked (direction);
}
