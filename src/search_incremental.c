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

#include <string.h>

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
#include "search_incremental.h"

static void toolbar_search_start_over (void);
static void toolbar_search_clicked (void);

static gint
incremental_search(TextEditor *te, gchar *string);



static SearchReplace *sr = NULL;


static void
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
incremental_search(TextEditor *te, gchar *string)
{
	FileBuffer *fb;
	static MatchInfo *mi;
	Search *s;

	fb = file_buffer_new_from_te(te);
	s = &(sr->search);
	
	if (s->expr.search_str)
		g_free(s->expr.search_str);
	s->expr.search_str = g_strdup(string);
	
	if ((mi = get_next_match(fb, SD_FORWARD, &(s->expr))))
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


void
toolbar_search_incremental_start (void)
{
	gchar *string;
	const gchar *string1;
	TextEditor *te;

	if (!(te = anjuta_get_current_text_editor()))
		return;
	if (sr == NULL)
		sr = create_search_replace_instance();
		
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
	sr->search.incremental_wrap = 0;
}

void
toolbar_search_incremental_end (void)
{
	sr->search.incremental_pos = -1;
}

void
toolbar_search_incremental (void)
{
	const gchar *entry_text;
	TextEditor *te;
	
	if (!(te = anjuta_get_current_text_editor()))
		return;
	if (sr == NULL)
		sr = create_search_replace_instance();
	
	if (sr->search.incremental_pos < 0)
		return;
	
	text_editor_goto_point (te, sr->search.incremental_pos);

	entry_text = gtk_entry_get_text (GTK_ENTRY
				    (app->widgets.toolbar.main_toolbar.find_entry));
	if (!entry_text || strlen(entry_text) < 1) return;
	
	toolbar_search_clicked ();
}


static void
toolbar_search_clicked (void)
{
	TextEditor *te;
	const gchar *string;
	gint ret;
	/* gboolean search_wrap = FALSE; */

	if (!(te = anjuta_get_current_text_editor()))
		return;
	if (sr == NULL)
		sr = create_search_replace_instance();

	string = gtk_entry_get_text (GTK_ENTRY(app->widgets.toolbar.main_toolbar.
							     find_entry));
	
	if (sr->search.incremental_pos >= 0)
	{
		incremental_save_retrieve_expr(TRUE); // Save parameters
		sr->search.expr.regex = FALSE;
		sr->search.expr.greedy = FALSE;
		sr->search.expr.ignore_case = TRUE;
		sr->search.expr.whole_word = FALSE;
		sr->search.expr.whole_line = FALSE;
		sr->search.expr.word_start = FALSE;
		
		ret = incremental_search(te, (gchar*)string);
		
		incremental_save_retrieve_expr(FALSE); // Retrieve parameters
	}
	else
	{
		/* Normal search */
		ret = incremental_search(te, (gchar*)string);
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
				toolbar_search_start_over ();
			gtk_widget_destroy (dialog);
		}
		else
		{
			if (sr->search.incremental_wrap == 1)
			{
				anjuta_status(
				"Failling I-Search: '%s'. Press Enter or click Find to overwrap.",
				string);
				sr->search.incremental_wrap = FALSE;
				
				sr->search.incremental_wrap = 2;
			}
			else
				anjuta_status ("Failling Overwrapped I-Search: %s.", string);
				
			if (anjuta_preferences_get (ANJUTA_PREFERENCES (app->preferences),
										BEEP_ON_BUILD_COMPLETE))
				gdk_beep();
		}
	}
	else
		sr->search.incremental_pos = ret;
}


void
toolbar_search_clicked_cb (void)
{
	/* Make sure that sr isn't null. We can't just assume that the user entered
	   text. If the user didn't enter any text, sr will be null, so we must exit
	   the function so that it doesn't get referenced, thereby avoiding a 
	   segfault */
	
	if (sr != NULL)
	{
		if (sr->search.incremental_wrap == 2)
		{
			sr->search.incremental_wrap = 1;
			toolbar_search_start_over ();
		}
		else 
		{
			if (sr->search.incremental_wrap == 0)
				sr->search.incremental_wrap = 1;
			toolbar_search_clicked ();
		}
	}
}


static void
toolbar_search_start_over (void)
{
	TextEditor *te;

	if (!(te = anjuta_get_current_text_editor()))
		return;

	text_editor_goto_point (te, 0);
	toolbar_search_clicked ();
}
