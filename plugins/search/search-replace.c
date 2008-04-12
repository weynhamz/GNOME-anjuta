/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * search-replace.c: Generic Search and Replace
 * Copyright (C) 2004 Biswapesh Chattopadhyay
 * Copyright (C) 2004-2008 Naba Kumar  <naba@gnome.org>
 *
 * This file is part of anjuta.
 * Anjuta is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * Anjuta is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with anjuta. If not, contact the Free Software Foundation,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>
#include <fnmatch.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <gnome.h>
#include <glade/glade.h>
#include <libgnomevfs/gnome-vfs.h>

#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-message-manager.h>
#include <libanjuta/interfaces/ianjuta-message-view.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-editor-search.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>
#include <libanjuta/interfaces/ianjuta-markable.h>
#include <libanjuta/interfaces/ianjuta-bookmark.h>
#include <libanjuta/interfaces/ianjuta-indicable.h>
#include <libanjuta/interfaces/ianjuta-file-savable.h>

#include "search-replace.h"

#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include <glib/gi18n.h>

#define GLADE_FILE_SEARCH_REPLACE PACKAGE_DATA_DIR"/glade/anjuta-search.glade"
/* some checks involve label text, this needs to conform to the tab label set
   via glade (not currently translateable or translated) */
#define GLADE_PREFS_TITLE "Parameters"
#define SEARCH_GUI_KEY "_anj_sr_guidata_"
#define MAX_ITEMS_SEARCH_COMBO 16
/* limit on search-string length */
#define MAX_LENGTH_SEARCH 128

static SearchReplaceGUI *def_sg;

/* translated button labels */
static const gchar *button_search_label;
static const gchar *button_replace_label;
static const gchar *button_replace_all_label;

AnjutaUtilStringMap search_target_strings[] =
{
	{SR_BUFFER, N_("Current file")},
	{SR_SELECTION, N_("Current selection")},
	{SR_BLOCK, N_("Current block")},
	{SR_FUNCTION, N_("Current function")},
	{SR_OPEN_BUFFERS, N_("All open files")},
	{SR_PROJECT, N_("All project files")},
/*	CHECKME is this for the "variable" widget in the glade UI ?
	{SR_VARIABLE, N_("These files ...")}, */
	{SR_FILES, N_("Files like these ...")},
	{-1, NULL}
};

AnjutaUtilStringMap search_action_strings[] =
{
	{SA_SELECT, N_("Select nearest match")},
	{SA_BOOKMARK, N_("Bookmark all matched lines")},
	{SA_HIGHLIGHT, N_("Mark all matches")},
	{SA_UNLIGHT, N_("Clear all match-marks")},
	{SA_FIND_PANE, N_("List matches in find pane")},
	{SA_REPLACE, N_("Replace interactively")},
	{SA_REPLACEALL, N_("Replace all matches")},
	{-1, NULL}
};

static GUIElement glade_widgets[] =
{
	/* CLOSE_BUTTON */
 	{GE_BUTTON, "button.close", NULL},
	/* STOP_BUTTON */
	{GE_BUTTON, "button.stop", NULL},
	/* REPLACE_BUTTON */
	{GE_BUTTON, "button.replace", NULL},
	/* SEARCH_BUTTON */
	{GE_BUTTON, "button.start", NULL},
#if 0
	/* LOG_SRCH_BUTTON */
	{GE_BUTTON, "button.log.search", NULL},
	/* LOG_REPL_BUTTON */
	{GE_BUTTON, "button.log.replace", NULL},
#endif
	
	/* FILE_FILTER_FRAME */
	{GE_NONE, "frame.file.filter", NULL},
	/* SEARCH_SCOPE_FRAME */
	{GE_NONE, "frame.sr.scope", NULL},
	
	/* LABEL_REPLACE */
	{GE_NONE, "label.replace", NULL},
	/* SEARCH_STRING */
	{GE_COMBO_ENTRY, "search.string.combo", NULL},
	/* MATCH_FILES */
	{GE_COMBO_ENTRY, "file.filter.match.combo", NULL},
	/* UNMATCH_FILES */
	{GE_COMBO_ENTRY, "file.filter.unmatch.combo", NULL},
	/* MATCH_DIRS */
	{GE_COMBO_ENTRY, "dir.filter.match.combo", NULL},
	/* UNMATCH_DIRS */
	{GE_COMBO_ENTRY, "dir.filter.unmatch.combo", NULL},
	/* REPLACE_STRING */
	{GE_COMBO_ENTRY, "replace.string.combo", NULL},

	/* ACTIONS_MAX */
	{GE_TEXT, "actions.max", NULL},
	/* SEARCH_REGEX */
	{GE_BOOLEAN, "search.regex", NULL},
	/* GREEDY */
	{GE_BOOLEAN, "search.greedy", NULL},
	/* IGNORE_CASE */
	{GE_BOOLEAN, "search.ignore.case", NULL},
	/* WHOLE_WORD */
	{GE_BOOLEAN, "search.match.whole.word", NULL},
	/* WORD_START */
	{GE_BOOLEAN, "search.match.word.start", NULL},
	/* WHOLE_LINE */
	{GE_BOOLEAN, "search.match.whole.line", NULL},
	/* IGNORE_HIDDEN_FILES */
	{GE_BOOLEAN, "ignore.hidden.files", NULL},
	/* IGNORE_HIDDEN_DIRS */
	{GE_BOOLEAN, "ignore.hidden.dirs", NULL},
	/* SEARCH_RECURSIVE */
	{GE_BOOLEAN, "search.dir.recursive", NULL},
	/* REPLACE_REGEX */
	{GE_BOOLEAN, "replace.regex", NULL},

	/* SEARCH_WHOLE */
	{GE_BOOLEAN, "search.whole", NULL},
	/* SEARCH_FORWARD */
	{GE_BOOLEAN, "search.forward", NULL},
	/* SEARCH_BACKWARD */
	{GE_BOOLEAN, "search.backward", NULL},
	/* ACTIONS_NO_LIMIT */
	{GE_BOOLEAN, "actions.no_limit", NULL},
	/* ACTIONS_LIMIT */
	{GE_BOOLEAN, "actions.limit", NULL},
	/* SEARCH_STRING_COMBO */
	{GE_COMBO, "search.string.combo", NULL},
	/* SEARCH_TARGET_COMBO */
	{GE_COMBO, "search.target.combo", search_target_strings},
	/* SEARCH_ACTION_COMBO */
	{GE_COMBO, "search.action.combo", search_action_strings},
	/* MATCH_FILES_COMBO */
	{GE_COMBO, "file.filter.match.combo", NULL},
	/* UNMATCH_FILES_COMBO */
	{GE_COMBO, "file.filter.unmatch.combo", NULL},
	/* MATCH_DIRS_COMBO */
	{GE_COMBO, "dir.filter.match.combo", NULL},
	/* UNMATCH_DIRS_COMBO */
	{GE_COMBO, "dir.filter.unmatch.combo", NULL},
	/* REPLACE_STRING_COMBO */
	{GE_COMBO, "replace.string.combo", NULL},
};

/* LibGlade's auto-signal-connect will connect to these callbacks.
 * Do not declare them static.
 */
/*void on_search_dialog_page_switch (GtkNotebook *notebook,
									GtkNotebookPage *page,
									guint page_num,
									gpointer user_data);
*/
void on_search_match_whole_word_toggled (GtkToggleButton *togglebutton,
										 gpointer user_data);
void on_search_match_whole_line_toggled (GtkToggleButton *togglebutton,
										 gpointer user_data);
void on_search_match_word_start_toggled (GtkToggleButton *togglebutton,
										 gpointer user_data);
//void on_replace_regex_toggled (GtkToggleButton *togglebutton, gpointer user_data);
void on_search_regex_toggled (GtkToggleButton *togglebutton, gpointer user_data);
void on_search_actions_no_limit_toggled (GtkToggleButton *togglebutton,
										 gpointer user_data);
gboolean on_search_dialog_delete_event (GtkWidget *window, GdkEvent *event,
										 gpointer user_data);
void on_search_action_changed (GtkComboBox *combo, gpointer user_data);
void on_search_target_changed (GtkComboBox *combo, gpointer user_data);
void on_search_expression_changed (GtkComboBox *combo, gpointer user_data);
void on_search_button_close_clicked (GtkButton *button, gpointer user_data);
void on_search_button_help_clicked (GtkButton *button, gpointer user_data);
void on_search_button_start_clicked (GtkButton *button, gpointer user_data);
void on_search_button_replace_clicked (GtkButton *button, gpointer user_data);
void on_search_expression_activate (GtkEditable *edit, gpointer user_data);
void on_search_button_save_clicked (GtkButton *button, gpointer user_data);
void on_search_button_stop_clicked (GtkButton *button, gpointer user_data);

void on_search_direction_changed (GtkToggleButton *togglebutton, gpointer user_data);
void on_search_full_buffer_toggled (GtkToggleButton *togglebutton,
									gpointer user_data);
//void on_search_forward_toggled (GtkToggleButton *togglebutton,
//								gpointer user_data);
//void on_search_backward_toggled (GtkToggleButton *togglebutton,
//								gpointer user_data);
//void on_setting_basic_search_toggled (GtkToggleButton *togglebutton,
//									  gpointer user_data);

/***********************************************************/

static gboolean on_search_dialog_key_press (GtkWidget *widget,
											GdkEventKey *event,
										    gpointer user_data);
static gboolean on_message_clicked (IAnjutaMessageView *view,
									gchar *message,
									gpointer user_data);
static void on_message_buffer_flush (IAnjutaMessageView *view,
									 const gchar *one_line,
									 gpointer user_data);
static void anj_sr_write_match_message (IAnjutaMessageView *view,
										SearchEntry *se,
										gchar *project_root_uri,
										gint rootlen);
static void anj_sr_set_action (SearchReplaceGUI *sg, SearchAction action);
static void anj_sr_set_target (SearchReplaceGUI *sg, SearchRangeType target);
static void anj_sr_set_direction (SearchReplaceGUI *sg, SearchDirection dir);
static void anj_sr_populate_value (SearchReplaceGUI *sg,
									GUIElementId id,
									gpointer val_ptr);
static void anj_sr_reset_flags (SearchReplace *sr);
//static void anj_sr_roll_editor_over (SearchReplace *sr);
static gboolean anj_sr_end_alert (SearchReplace *sr);
static void anj_sr_max_results_alert (SearchReplace *sr);
static void anj_sr_total_results_alert (SearchReplace *sr);
static void anj_sr_show_replace (SearchReplaceGUI *sg, gboolean hide);
static void anj_sr_modify_button (SearchReplaceGUI *sg,
								  GUIElementId button_name,
								  const gchar *name,
								  const gchar *stock_id);
static void anj_sr_show_replace_button (SearchReplaceGUI *sg, gboolean show);
static void anj_sr_enable_replace_button (SearchReplaceGUI *sg, gboolean show);
static void anj_sr_reset_replace_buttons (SearchReplaceGUI *sg);
static gboolean anj_sr_create_dialog (SearchReplace *sr);
static void anj_sr_present_dialog (SearchReplaceGUI *sg);
static gboolean anj_sr_find_in_list (GList *list, gchar *word);
static void anj_sr_trim_list (GList **list, guint nb_max);
static void anj_sr_update_search_combos (SearchReplaceGUI *sg);
static void anj_sr_update_replace_combos (SearchReplaceGUI *sg);
static void anj_sr_conform_direction_change (SearchReplaceGUI *sg,
											SearchDirection dir);
static void anj_sr_disconnect_set_toggle_connect (SearchReplaceGUI *sg,
												  GUIElementId id,
												  GCallback function,
												  gboolean active);
static void anj_sr_select_nearest (SearchReplace *sr, SearchDirection dir);
//CHECKME keep this ?
//static void basic_search_toggled (void);

/***********************************************************/

/**
 * anj_sr_execute:
 * @sr: pointer to search/replace data struct
 * @dlg: TRUE when the operation involves the search-dialog
 *
 * Conduct a search [and replace] operation in accordance with data in @sr
 * Depending on the operation performed, this loops sequentially through all
 * matches in the chosen scope, and when relevant, then through all relevant
 * files.
 * But if the operation is interactive-replacement, then each such operation
 * involves 2 "phases" - before and after confirmation - and those both,
 * separately, invoke a call to this func. In such calls, the loop terminates
 * either immediately (after a search-button click) or after a second pass to
 * find the next match (after a replace-button click). These quick exits mean
 * that most of the data about subsequent files to search, accumulated at the
 * start of the function, are discarded immediately and need to be re-created
 * when the second phase happens.
 * This is wasteful, but it allows complete freedom between phases to change
 * the search-operation or or -scope or -expression, and it avoidss blocking
 * any other UI or other functionality while waiting for confirmation.
 *
 * Return value: none
 */
void
anj_sr_execute (SearchReplace *sr, gboolean dlg)
{
	GList *node;
	Search *s;
	IAnjutaMessageView *view;
	GtkWidget *button;	/* the STOP_BUTTON widget */
	gboolean backward;
	SearchReplaceGUI *sg;
	gchar *project_root_uri;
	gint rootlen;
	IAnjutaIterable* set_start;
	IAnjutaIterable* set_end;

	g_return_if_fail (sr);

	if (sr->search.stop_count < 0)
	{
		/* something triggered an abort */
		sr->search.stop_count = 0;
		return;
	}
	s = &(sr->search);
	sg = sr->sg;

	s->busy = TRUE;	/* mutex */
	if (s->expr.search_str == NULL)	/* CHECKME when repeating regex search */
	{
		s->busy = FALSE;
		return;
	}

	dlg = (dlg && sg != NULL && sg->dialog != NULL);	/* bullet-proofing */
	if (s->action == SA_REPLACE && !dlg)
		return;

	if (s->candidates == NULL)		/* if starting a fresh operation */
		create_search_entries (sr); /* grab a list of all items to be searched */
	if (s->candidates == NULL)
	{
		s->busy = FALSE;
		return;
	}

	if (s->action == SA_FIND_PANE)	/* list matches in msgman pane */
	{
		gchar *name;
//		AnjutaShell *shell;
		IAnjutaMessageManager *msgman;

//		g_object_get (G_OBJECT (sr->docman), "shell", &shell, NULL);
//		msgman = anjuta_shell_get_interface(shell, IAnjutaMessageManager, NULL);
		msgman = anjuta_shell_get_interface(ANJUTA_PLUGIN (sr->docman)->shell,
											IAnjutaMessageManager,
											NULL);
		if (msgman == NULL)
			s->busy = FALSE;
		g_return_if_fail (msgman != NULL);

		project_root_uri = NULL;
		anjuta_shell_get (ANJUTA_PLUGIN (sr->docman)->shell,
						  "project_root_uri", G_TYPE_STRING,
						  &project_root_uri, NULL);
		if (project_root_uri)
			rootlen = strlen (project_root_uri);
		else
			rootlen = 0;	/* warning prevention */

		name = g_strconcat(_("Find: "), s->expr.search_str, NULL);
		view = ianjuta_message_manager_get_view_by_name (msgman, name, NULL);
		if (view == NULL)
		{
			// FIXME: Put a nice icon here:
			view = ianjuta_message_manager_add_view (msgman, name,
													GTK_STOCK_FIND_AND_REPLACE, NULL);
			if (view == NULL)
				s->busy = FALSE;
			g_return_if_fail (view != NULL);
			g_signal_connect (G_OBJECT (view), "buffer-flushed",
			                  G_CALLBACK (on_message_buffer_flush), NULL);
			g_signal_connect (G_OBJECT (view), "message-clicked",
			                  G_CALLBACK (on_message_clicked), sr->docman);
		}
		else
			ianjuta_message_view_clear (view, NULL);
		ianjuta_message_manager_set_current_view (msgman, view, NULL);
	}
	else
	{
		/* warning prevention */
		view = NULL;
		project_root_uri = NULL;
		rootlen = 0;
	}

	if (dlg)	/* operation involves dialog */
	{
		anj_sr_update_search_combos (sg);
		if (s->action == SA_REPLACE || s->action == SA_REPLACEALL)
			anj_sr_update_replace_combos (sg);
		button = anj_sr_get_ui_widget (STOP_BUTTON);
		gtk_widget_set_sensitive (button, TRUE);
	}
	else
		button = NULL;	/* warning prevention */

	s->stop_count = 0;
	backward = (s->range.direction == SD_BACKWARD);
	s->limited =  (s->range.target == SR_SELECTION
				|| s->range.target == SR_FUNCTION
				|| s->range.target == SR_BLOCK);

	set_start = set_end = NULL;

	for (node = s->candidates; node != NULL; node = g_list_next (node))
	{
		FileBuffer *fb = NULL;
		SearchEntry *se;
		gboolean fresh; 	/* file was opened for this search operation */

		if (node->data == NULL)	/* this candidate was cleared before */
			continue;

		if (dlg && s->stop_count != 0)
			break;
		while (gtk_events_pending ())
			gtk_main_iteration();

		se = (SearchEntry *) node->data;

		if (s->action == SA_REPLACE && sr->replace.phase != SA_REPL_FIRST)
		{
			/* this is either a confirmation (sr->replace.phase == SA_REPL_CONFIRM)
				or skip (sr->replace.phase == SA_REPL_SKIP)*/
			/* buffer from the prior phase 1 is still present */
			fresh = se->fresh;
		}
		else if (se->type == SE_BUFFER)	/* operation intended to apply to part or all
										of an open buffer or to all open buffers */
		{
			se->fb = file_buffer_new_from_te (se->te);
			fresh = FALSE;
		}
		else /* se->type == SE_FILE, unopened file, operation intended to apply
				to all project files or to pattern-matching files */
		{
			se->fb = file_buffer_new_from_uri (sr, se->uri, NULL, -1);
			fresh = TRUE;
		}

		fb = se->fb;

		if (fb != NULL && fb->len > 0)
		{
			position_t replace_length;
			gboolean terminate; /* stop doing this file immediately */
			gboolean dirty;	/* fresh file has been changed somehow */
			gboolean save_file;

			if ((s->action == SA_HIGHLIGHT || s->action == SA_UNLIGHT)
				&& !fresh	/* no reason to do this for newly-opened files */
				&& fb->te != NULL)
				ianjuta_indicable_clear (IANJUTA_INDICABLE (fb->te), NULL);

			/* local operations work in byte-positions, all editors work with chars */
			s->expr.postype = (fb->te == NULL) ? SP_BYTES : SP_CHARS;

			/* CHECKME simply test for fb->start_pos and fb->end_pos not both == 0 ? */
			if (s->limited)
			{
				if (se->sel_first_start == se->sel_first_end)	/* both 0 @ first usage */
				{
					/* one of these will be updated after a match is found */
					se->sel_first_start = se->start_pos;
					se->sel_first_end = se->end_pos;
				}
				else
				{
					/* when scope is the selection, use the original settings cuz the
						current selection can move around in the searching process */
					se->start_pos = se->sel_first_start; /* CHECKME when SA_REPLACE */
					se->end_pos = se->sel_first_end;
				}
			}
			if (s->action != SA_REPLACE || sr->replace.phase == SA_REPL_FIRST)
			{
				if (s->expr.postype == SP_BYTES)
				{
					if (se->start_pos != 0)
						se->start_pos = file_buffer_get_char_offset (fb, se->start_pos);
					if (se->end_pos == -1)
						se->end_pos = fb->len - 1;
					else
						se->end_pos = file_buffer_get_char_offset (fb, se->end_pos);
				}
				fb->start_pos = se->start_pos;
				fb->end_pos = se->end_pos;
			}

			/* pity about repetition of this inside loop, but can't do it before
			   each editor (maybe of different sorts) is present and its
			   position-type is known */
			if (!s->expr.regex)
			{
				if (s->expr.postype == SP_BYTES)
				{
					/* except for regex searching, the match-length is constant */
					se->mi.len = strlen (s->expr.search_str);
					/* likewise the replacement length */
					if (s->action == SA_REPLACE || s->action == SA_REPLACEALL)
						replace_length = strlen (sr->replace.repl_str);
					else
						replace_length = 0;	/* warning prevention */
				}
				else
				{
					se->mi.len = g_utf8_strlen (s->expr.search_str, -1);
					if (s->action == SA_REPLACE || s->action == SA_REPLACEALL)
						replace_length = g_utf8_strlen (sr->replace.repl_str, -1);
					else
						replace_length = 0;	/* warning prevention */
				}
				/* and the adjustment needed after a replacement */
				/* +ve differential when replacement string is longer than original */
//				if (s->action == SA_REPLACE || s->action == SA_REPLACEALL)
//					se->offset = replace_length - se->mi.len;
			}
			else
			{
				if (s->action == SA_REPLACE || s->action == SA_REPLACEALL)
				/* length will be changed later if replacement string has backrefs */
					replace_length = strlen (sr->replace.repl_str);
				else
					replace_length = 0;	/* warning prevention */
//				se->offset = 0;
			}

			/* CHECKME to eliminate un-needed moves, when not SA_BOOKMARK, this could be
			   current line i.e. ianjuta_editor_get_lineno (IANJUTA_EDITOR (fe->te), NULL);
			   or sometimes last-line ? */
			if (se->found_line == 0)
				se->found_line = (s->action == SA_BOOKMARK) ? -1 : 1;
again:
			dirty = FALSE;
			terminate = FALSE;
			save_file = FALSE;

			/* AT LAST, WE BEGIN SEARCHING */
			while (sr->replace.phase == SA_REPL_CONFIRM	/* always proceed if this is phase 2 of a confirmed replacement */
					|| get_next_match (fb, s->range.direction, &(s->expr), &(se->mi)))
			{
				if (//s->action != SA_REPLACE ||
					sr->replace.phase != SA_REPL_CONFIRM)
				{
					/* this is not phase 2 of a confirmed replacement */
					position_t thisoffset;

					if (backward)
					{
						thisoffset = (s->expr.postype == SP_BYTES) ?
							file_buffer_get_char_offset (fb, se->mi.pos) : se->mi.pos;
						if (thisoffset < fb->start_pos)
							break;
					}

					if (!backward && fb->end_pos != -1)
					{
						thisoffset = (s->expr.postype == SP_BYTES) ?
							file_buffer_get_char_offset (fb, se->mi.pos + se->mi.len) :
							se->mi.pos + se->mi.len;
						if (thisoffset > fb->end_pos + 1)	/* match at end of buffer will go past last char */
							break;
					}
				}

				/* NOTE - mi->line is "editor-style" 1-based, but some things
					here use/expect 0-base, so adjustments are made as needed.

					Lengths and last chars are 1-past actual last char to process,
					to suit sourceview-style positioning with iter affer that
					last char. Scintilla needs downstream adjustments for this */

				switch (s->action)
				{
					case SA_HIGHLIGHT:
						if (fb->te && IANJUTA_IS_INDICABLE (fb->te))
						{
							if (set_start == NULL)
							{
								set_start = ianjuta_editor_get_start_position (fb->te, NULL);
								set_end = ianjuta_iterable_clone (set_start, NULL);
							}

							if (ianjuta_iterable_set_position (IANJUTA_ITERABLE (set_start),
																se->mi.pos,
																NULL)
							&& ianjuta_iterable_set_position (IANJUTA_ITERABLE (set_end),
																se->mi.pos + se->mi.len,
																NULL))
								 ianjuta_indicable_set (IANJUTA_INDICABLE (fb->te),
														IANJUTA_ITERABLE (set_start),
														IANJUTA_ITERABLE (set_end),
														IANJUTA_INDICABLE_IMPORTANT, NULL);
							else
							{
								//FIXME failure warning
							}
							dirty = TRUE;
						}
						else
							terminate = TRUE;

						break;

					case SA_BOOKMARK:
						if (fb->te && IANJUTA_IS_MARKABLE (fb->te))
						{
							if (se->found_line != se->mi.line)
							{
								se->found_line = se->mi.line;

								if (!ianjuta_markable_is_marker_set (
														IANJUTA_MARKABLE (fb->te),
														se->mi.line,
														IANJUTA_MARKABLE_BOOKMARK,
														NULL))
								{
									ianjuta_bookmark_toggle (IANJUTA_BOOKMARK (fb->te),
														se->mi.line, FALSE, NULL);
									dirty = TRUE;
								}
							}
						}
						else
							terminate = TRUE;

						break;

					case SA_FIND_PANE:
						anj_sr_write_match_message (view, se, project_root_uri, rootlen);
						break;

					case SA_SELECT:
						if (fresh) /* not an open file */
						{
							editor_new_from_file_buffer (se);
							if (fb->te && IANJUTA_IS_EDITOR_SELECTION (fb->te))
							{
								fresh = FALSE;
								s->expr.postype = SP_CHARS;
							}
							else if (fb->te)
							{
								/* new editor which is not selectable */
								ianjuta_document_manager_remove_document
												(sr->docman,
												IANJUTA_DOCUMENT (fb->te),
												FALSE, NULL);
								terminate = TRUE;
							}
						}
						if (fb->te && IANJUTA_IS_EDITOR_SELECTION (fb->te))
						{
							if (se->found_line != se->mi.line)
							{
								se->found_line = se->mi.line;
								ianjuta_editor_goto_line (fb->te, se->mi.line, NULL);
							}

							if (set_start == NULL)
							{
								set_start = ianjuta_editor_get_start_position (fb->te, NULL);
								set_end = ianjuta_iterable_clone (set_start, NULL);
							}

							if (ianjuta_iterable_set_position (IANJUTA_ITERABLE (set_start),
																se->mi.pos,
																NULL)
							&& ianjuta_iterable_set_position (IANJUTA_ITERABLE (set_end),
																se->mi.pos + se->mi.len,
																NULL))
								ianjuta_editor_selection_set (IANJUTA_EDITOR_SELECTION (fb->te),
															  IANJUTA_ITERABLE (set_start),
															  IANJUTA_ITERABLE (set_end),
															  NULL);
							else
							{
								//FIXME failure warning
							}
							dirty = TRUE;
						}
						else
							terminate = TRUE;

						break;

					case SA_REPLACE:
						if (fresh) /* not an open file */
						{
							editor_new_from_file_buffer (se);
							if (fb->te && IANJUTA_IS_EDITOR_SELECTION (fb->te))
							{
								fresh = FALSE;
								s->expr.postype = SP_CHARS;
							}
							else if (fb->te)
							{
								/* new editor which is not selectable */
								ianjuta_document_manager_remove_document
												(sr->docman,
												IANJUTA_DOCUMENT (fb->te),
												FALSE, NULL);
								terminate = TRUE;
							}
						}
						if (fb->te && IANJUTA_IS_EDITOR_SELECTION (fb->te))
						{
							if (se->found_line != se->mi.line)
							{
								se->found_line = se->mi.line;
								ianjuta_editor_goto_line (fb->te, se->found_line, NULL);
							}

							if (sr->replace.phase == SA_REPL_CONFIRM)
							{
								/* this is phase 2, after confirmation */
								gchar *substitute;
								position_t newlen, match_pos;

								if (sr->replace.regex && s->expr.regex)
								{
									/* this replacement includes backref(s), so it's not constant */
									substitute = regex_backref (sr, &(se->mi), fb);
									match_info_free_subs (&(se->mi));
									se->mi.subs = NULL;
									newlen = (s->expr.postype == SP_BYTES) ?
										strlen (substitute) :
										g_utf8_strlen (substitute, -1);
								}
								else
								{
									substitute = sr->replace.repl_str;
									newlen = replace_length;
								}

								match_pos = se->mi.pos;

//								if (!backward)
//									se->mi.pos += se->total_offset;
								if (ianjuta_iterable_set_position (IANJUTA_ITERABLE (set_start),
																	se->mi.pos,
																	NULL)
								&& ianjuta_iterable_set_position (IANJUTA_ITERABLE (set_end),
																	se->mi.pos + se->mi.len,
																	NULL))
								{
									ianjuta_editor_selection_set (IANJUTA_EDITOR_SELECTION (fb->te),
																  IANJUTA_ITERABLE (set_start),
																  IANJUTA_ITERABLE (set_end),
																  NULL);
									ianjuta_editor_selection_replace (IANJUTA_EDITOR_SELECTION (fb->te),
																	 substitute,
																	 newlen,
																	 NULL);
								}
								else
								{
									//FIXME failure warning
									break;
								}
								/* after a replace, the following find will need
								   total_offset for operations not using editor-native
								   search to get the match-position and -length
								   ATM only regex uses local search buffer */
								/* CHECKME if replacing in local buffer as well as
									editor-native, probably no need for offset ? */
								//if (s->expr.regex) all is local ATM
								//{
//									if (!backward)
//										se->total_offset += newlen - se->mi.len;
									if (!backward)
									{
										if (s->expr.postype == SP_BYTES)
											fb->start_pos +=
												g_utf8_strlen (substitute, -1) -
												g_utf8_strlen (fb->buf+ match_pos, se->mi.len);
										else
											fb->start_pos += newlen - se->mi.len;
									}
									/* using local buffer for search */
									replace_in_local_buffer (fb, match_pos, se->mi.len, substitute);
									/* CHECKME any EOL(s) added or removed ? hence must freshen
										Otherwise, freshen needed or will offset suffice ? */
									if (!backward)
										file_buffer_freshen_lines_from_pos (fb, match_pos, s->expr.postype == SP_BYTES);
								//}
								if (substitute && sr->replace.regex && s->expr.regex)
								{
									g_free (substitute);
								}
								dirty = TRUE;
								sr->replace.phase = SA_REPL_SKIP; /* trigger another pass of this loop to find the next match */
							}
							else
							{
								/* this is before pause for confirm or skip */
//								if (s->expr.regex && all searching is local ATM
//								if (!backward)
//									se->mi.pos += se->total_offset;
								if (set_start == NULL)
								{
									set_start = ianjuta_editor_get_start_position (fb->te, NULL);
									set_end = ianjuta_iterable_clone (set_start, NULL);
								}
								if (ianjuta_iterable_set_position (IANJUTA_ITERABLE (set_start),
																	se->mi.pos,
																	NULL)
								&& ianjuta_iterable_set_position (IANJUTA_ITERABLE (set_end),
																	se->mi.pos + se->mi.len,
																	NULL))
									ianjuta_editor_selection_set (IANJUTA_EDITOR_SELECTION (fb->te),
																  IANJUTA_ITERABLE (set_start),
																  IANJUTA_ITERABLE (set_end),
																  NULL);
								else
								{
									//FIXME failure warning
								}
								/* prevent double-offset in phase 2, if the user confirms */
//								if (!backward)
//									se->mi.pos -= se->total_offset;

								if (dlg)
									anj_sr_enable_replace_button (sg, TRUE);

								if (s->limited)
								{
									/* update the remembered search scope */
									if (backward)
										se->sel_first_end = fb->start_pos;
									else
										se->sel_first_start = fb->start_pos;
								}

								/* trigger immediate exit from loop and then
								   phase 2 (confirmed) or another phase 1 (skipped) */
								sr->replace.phase = SA_REPL_CONFIRM;
							}
						}
						else
							terminate = TRUE;

						break;

					case SA_REPLACEALL:
						if (fresh) /* not an open file */
						{
							gchar *substitute;

							if (sr->replace.regex && s->expr.regex)
							{
								substitute = regex_backref (sr, &(se->mi), fb);
								match_info_free_subs (&(se->mi));
								se->mi.subs = NULL;
							}
							else
							{
								substitute = sr->replace.repl_str;
							}

							if (replace_in_local_buffer (fb, se->mi.pos, se->mi.len, substitute))
							{
//								position_t thisoffset;

								save_file = TRUE;
/* un-opened files don't need backward searching
								if (backward)
								{
									GList *node;
									/ * update length data in last member of list * /
									node = g_list_last (fb->lines);
									node = g_list_previous (node);
									thisoffset = ((EOLdata *)node->data)->offb;
									file_buffer_freshen_lines_from_pos (fb, thisoffset, TRUE);
								}
								else
								{
*/
									/*freshen lines from current*/
//									file_buffer_freshen_lines_from_pos (fb, se->mi.pos, TRUE);
									//sr->total_offset += se->mi.len - newlen;
//								}
							}
						}
						else
						{
							if (fb->te && IANJUTA_IS_EDITOR_SELECTION (fb->te))
							{
								gchar *substitute;
								position_t newlen, match_pos;

								if (sr->replace.regex && s->expr.regex)
								{
									substitute = regex_backref (sr, &(se->mi), fb);
									match_info_free_subs (&(se->mi));
									se->mi.subs = NULL;
									newlen = g_utf8_strlen (substitute, -1);
								}
								else
								{
									substitute = sr->replace.repl_str;
									newlen = replace_length;
								}
								/* the select/replace will need total_offset for
								   operations not using editor-native search
								   to get the match-position and -length
								   ATM only regex uses local search buffer */
								/* CHECKME if replacing in local buffer as well as
									editor-native, probably no need for offset ? */
								//if (s->expr.regex) all is local ATM

								match_pos = se->mi.pos;

								if (!backward)
								{
									se->mi.pos += se->total_offset;
									se->total_offset += newlen - se->mi.len;
								}

								if (set_start == NULL)
								{
									set_start = ianjuta_editor_get_start_position (fb->te, NULL);
									set_end = ianjuta_iterable_clone (set_start, NULL);
								}
								if (ianjuta_iterable_set_position (IANJUTA_ITERABLE (set_start),
																	se->mi.pos,
																	NULL)
								&& ianjuta_iterable_set_position (IANJUTA_ITERABLE (set_end),
																	se->mi.pos + se->mi.len,
																	NULL))
								{
									ianjuta_editor_selection_set (IANJUTA_EDITOR_SELECTION (fb->te),
																  IANJUTA_ITERABLE (set_start),
																  IANJUTA_ITERABLE (set_end),
																  NULL);
									ianjuta_editor_selection_replace (IANJUTA_EDITOR_SELECTION (fb->te),
																substitute,
																newlen,
																NULL);
								}
								else
								{
									//FIXME failure warning
								}

/*								if (s->expr.regex)	/ * using local buffer for search * /
								{ */
									replace_in_local_buffer (fb, match_pos, se->mi.len, substitute);
									/* CHECKME any EOL(s) added or removed ? hence must freshen
										Otherwise, freshen needed or will offset suffice ? */
									if (!backward)
										file_buffer_freshen_lines_from_pos (fb, match_pos, s->expr.postype == SP_BYTES);
//								}

								if (substitute && sr->replace.regex && s->expr.regex)
								{
									g_free (substitute);
								}
								dirty = TRUE;
							}
							else
								terminate = TRUE;
						}

						break;

					default:
						g_warning ("Not implemented - File %s - Line %d\n", __FILE__, __LINE__);
					case SA_UNLIGHT:	/* the action was done before the loope*/
						terminate = TRUE;
						break;
				}  /* switch */

				if (terminate /* immediate end to processing this file */
				|| (s->action == SA_REPLACE && sr->replace.phase == SA_REPL_CONFIRM)) /* wait for confirmation */
					break;
				s->matches_sofar++;
				if ((!backward && fb->start_pos >= fb->len) /* can't go further forward */
				 || (backward && fb->end_pos <= 0)	/* can't go further back */
				 || (!s->expr.no_limit && s->matches_sofar == s->expr.actions_max) /* limit reached */
				 || s->action != SA_REPLACE)	/* after a replacement, go back to find next match */
					break;

			} /* while something to do to this buffer */

			if (s->matches_sofar == 0
				&& !fresh
				&& (s->action != SA_REPLACE || sr->replace.phase != SA_REPL_CONFIRM)
				&& s->action != SA_UNLIGHT)
			{
				if (anj_sr_end_alert (sr))
				{
					/* rollover is allowed and wanted */
//					anj_sr_roll_editor_over (sr); no need for this
					if (backward)
						fb->end_pos = fb->len;/* from end, all prvious offsets
												 will be wrong ? */
					else
					{
						fb->start_pos = 0;
						se->total_offset = 0; /* from start, offset can't be re-used,
												BUT there will be a misalignment
												problem if the searching continues
												past the point of first replacement
												in the previous stage ?*/
					}
					goto again;
				}
			}

			if (save_file)
			{
				if (!save_file_buffer (fb))
				{
					//FIXME UI warning
					g_warning ("Error saving modified file %s", se->uri);
				}
				save_file = FALSE;
			}

			if (s->action == SA_REPLACE && sr->replace.phase == SA_REPL_CONFIRM)
			{
				/* save these for the confirmation phase */
				se->fresh = fresh;
				/* remember the next search-range in case this replacement is skipped */
				se->start_pos = fb->start_pos;
				se->end_pos = fb->end_pos;
			}
			else
				se->fb = NULL;	/* trigger a cleanup, fb still non-NULL */
		} /* if (fb && te) */
		else	/* fb == NULL or fb->len == 0 */
			if (fb->len == 0)
		{
			se->fb = NULL;	/* trigger a cleanup, fb still non-NULL */
		}
		else
		{
			//FIXME UI warning
			g_warning ("Skipping %s, problem with editor or file buffer", se->uri);
		}

		if (se->fb == NULL)	/* indicates ready for cleanup */
		{
			/* this candidate now finished so NULL its data for post-loop cleanup */
			/* no need to clean file-buffer here, it's handled above */
			g_free (se->uri);
			g_free (se->regx_repl_str);
			if (fb != NULL)
				file_buffer_free (fb);
			if (se->mi.subs != NULL)
				match_info_free_subs (&(se->mi));
			g_slice_free1 (sizeof (SearchEntry), se);
			node->data = NULL;
		}

		if (s->action == SA_SELECT && s->matches_sofar > 0)
			break;
	} /* candidates loop */

	if (s->action == SA_FIND_PANE)
	{
		ianjuta_message_view_append (view, IANJUTA_MESSAGE_VIEW_TYPE_INFO,
									 _("Search complete"), "", NULL);
	}

	if (dlg)
	{
		if (s->action != SA_REPLACE || sr->replace.phase != SA_REPL_CONFIRM)
		{
			gtk_widget_set_sensitive (button, FALSE);	/* disable the stop button */
			anj_sr_enable_replace_button (sg, FALSE);
		}
		else //redundant if (s->action == SA_REPLACE)
			anj_sr_enable_replace_button (sg, TRUE);
	}

	if (s->action == SA_REPLACE	&& sr->replace.phase == SA_REPL_CONFIRM)
		s->candidates = g_list_remove_all (s->candidates, NULL);
	else
	{
		if (s->expr.regex)
		{
			pcre_info_free (s->expr.re);
			s->expr.re = NULL;
		}
		clear_search_entries (&(s->candidates));
	}

	if (s->action != SA_UNLIGHT)
	{
		/* 0-matches is reported at the end of the file-loop */
		if (s->matches_sofar > 0)
		{
			if (!s->expr.no_limit &&
				s->matches_sofar >= s->expr.actions_max)
				anj_sr_max_results_alert (sr);
			else if (s->action == SA_REPLACEALL)
				anj_sr_total_results_alert (sr);

			if (s->action != SA_REPLACE)
				s->matches_sofar = 0; /* we can't rollover unless there really
										is no replacement cuz all the offsets
										will be wrong after rolling */
			/* For replaces, 0 the count in the button callback */
		}
	}

	if (set_start != NULL)
			g_object_unref (G_OBJECT (set_start));
	if (set_end != NULL)
			g_object_unref (G_OBJECT (set_end));

	if (project_root_uri)
		g_free (project_root_uri);

	if (s->action != SA_REPLACE || sr->replace.phase != SA_REPL_CONFIRM)
		s->busy = FALSE;

	/* CHECKME any other convenient adjustments ? */
	if (dlg && s->range.direction == SD_WHOLE &&
		(s->action == SA_SELECT || s->action == SA_REPLACE))
	{
		s->range.direction = SD_FORWARD;
		anj_sr_set_direction (sg, SD_FORWARD);
	}
}

#define SHORT_START "... "

/**
 * anj_sr_write_match_message:
 * @view: messageview data struct
 * @se: searchentry data struct
 * @project_root_uri: string for ellipsizing
 * @rootlen: byte-length of @project_root_uri
 *
 * Prepare and display in message manager info about a match
 *
 * Return value: none
 */
static void
anj_sr_write_match_message (IAnjutaMessageView *view, SearchEntry *se,
							gchar *project_root_uri, gint rootlen)
{
	FileBuffer *fb;
	gchar *match_line;
	gchar *tmp;
	gchar buf[BUFSIZ];

	fb = se->fb;

	if (se->type == SE_BUFFER)	/* operation applied to part or all of
									an open buffer or to all open buffers */
	{
		/* DEBUG_PRINT ("FB URI  %s", fb->uri); */
		tmp = fb->uri;
	}
	else /* se->type == SE_FILE, operation applied to all project files or
			to pattern-matching files */
		tmp = se->uri;

//	match_line = file_buffer_get_linetext_for_pos (fb, se->mi.pos);
	match_line = file_buffer_get_linetext_for_line (fb, se->mi.line);
	/* ellipsize uri @ start of message for files in project dir or descendant */
	if (project_root_uri != NULL && g_str_has_prefix (tmp, project_root_uri))
		snprintf (buf, BUFSIZ, "%s%s:%d:%s\n", SHORT_START, tmp + rootlen, se->mi.line, match_line);
	else
		/* FIXME omit any scheme, host, user, port, password etc from message */
		snprintf (buf, BUFSIZ, "%s:%d:%s\n", tmp, se->mi.line, match_line);

	g_free (match_line);
	ianjuta_message_view_buffer_append (view, buf, NULL);
}

/**
 * on_message_buffer_flush:
 * @view: messageview data struct
 * @one_line: string displayed as "summary" in msgman pane
 * @data: UNUSED data specified when callback was connected
 *
 * Callback for "buffer-flushed" signal emitted on @view
 *
 * Return value: none
 */
static void
on_message_buffer_flush (IAnjutaMessageView *view, const gchar *one_line,
						 gpointer data)
{
	ianjuta_message_view_append (view, IANJUTA_MESSAGE_VIEW_TYPE_NORMAL,
								 one_line, "", NULL);
}

static gboolean
on_message_clicked (IAnjutaMessageView *view, gchar* message, gpointer user_data)
{
	gchar *ptr, *ptr2;
	gchar *uri, *nline;
	line_t line;
	gboolean condensed;

	condensed = g_str_has_prefix (message, SHORT_START"/");
	if (condensed)
	{
		ptr = strchr (message, ':');
		if (ptr == NULL)
			return FALSE;
	}
	else
	{
/*	FIXME find a valid way to pass scheme, host, user, port, password etc
		gchar *path;
		path = gnome_vfs_get_uri_scheme (message);
		len = strlen (scheme);
		g_free (scheme);
		ptr = strchr (message + len + 1, ':');
*/
		ptr = strchr (message, ':');
		if (ptr == NULL)	/* end of "file:" */
			return FALSE;
		ptr = strchr (ptr+1, ':');
		if (ptr == NULL)
			return FALSE;
	}
	ptr2 = strchr (ptr + 1, ':');
	if (ptr2 == NULL)
		return FALSE;
	if (condensed)
	{
		gchar *project_root_uri;
		/* FIXME what if the project is changed before the message is clicked ? */
		project_root_uri = NULL;
		anjuta_shell_get (ANJUTA_PLUGIN ((IAnjutaDocumentManager *)user_data)->shell,
						  "project_root_uri", G_TYPE_STRING,
						  &project_root_uri, NULL);
		if (project_root_uri != NULL)
		{
			gchar *freeme;
			/* 4 is length of SHORT_START */
			freeme = g_strndup (message + 4, ptr - message - 4);
			uri = g_strconcat (project_root_uri, freeme, NULL);
			g_free (freeme);
			g_free (project_root_uri);
		}
		else
			return FALSE;
	}
	else
		uri = g_strndup (message, ptr - message);
	nline = g_strndup (ptr + 1, ptr2 - ptr - 1);
	line = atoi (nline);
	ianjuta_document_manager_goto_uri_line_mark ((IAnjutaDocumentManager *)user_data,
													uri, line, TRUE, NULL);
	g_free (uri);
	g_free (nline);
	return FALSE;
}

/* replace search-expression string in @sr if possible */
static void
anj_sr_set_search_string (SearchReplace *sr)
{
	if (sr->search.range.target != SR_SELECTION)
	{
		IAnjutaDocument *doc;

		doc = ianjuta_document_manager_get_current_document (sr->docman, NULL);
		if (IANJUTA_IS_EDITOR (doc))
		{
			IAnjutaEditor *te;
			gchar *current_word;

			te = IANJUTA_EDITOR (doc);
			current_word = ianjuta_editor_selection_get (IANJUTA_EDITOR_SELECTION (te), NULL);
			if (current_word == NULL)
				current_word = ianjuta_editor_get_current_word (te, NULL);
			if (current_word != NULL && *current_word != 0)
			{
				if (strlen (current_word) > MAX_LENGTH_SEARCH)
					current_word[MAX_LENGTH_SEARCH] = '\0';
				g_free (sr->search.expr.search_str);
				sr->search.expr.search_str = current_word;
			}
		}
	}
}

static void
anj_sr_select_nearest (SearchReplace *sr, SearchDirection dir)
{
	if (!(sr == NULL || sr->search.busy))
	{
		SearchAction save_action;
		SearchRangeType save_target;
		SearchDirection save_direction;

		save_action = sr->search.action;
		save_target = sr->search.range.target;
		save_direction = sr->search.range.direction;
		sr->search.range.direction = dir;
		if (save_target == SR_OPEN_BUFFERS ||
			save_target == SR_PROJECT ||
			save_target == SR_FILES)
				sr->search.range.direction = SD_WHOLE;
		sr->search.action = SA_SELECT;

		anj_sr_execute (sr, TRUE);

		sr->search.action = save_action;
		sr->search.range.target = save_target;
		sr->search.range.direction = save_direction;
	}
	else
	{
		DEBUG_PRINT ("busy or no sr data");
	}
}

/**
 * anj_sr_select_next:
 * @sg: Pointer to search data struct
 *
 * Backend for action callback on_findnext1_activate, etc
 *
 * Return value: None
 */
void
anj_sr_select_next (SearchReplaceGUI *sg)
{
	SearchReplace *sr;

	if (sg == NULL || sg->dialog == NULL)
	{
		sr = search_get_default_data ();
		if (sr == NULL)
			return;
		anj_sr_create_dialog (sr);
		anj_sr_set_search_string (sr); /* not overwritten by anythig in active search */
	}
	else
	{
		sr = sg->sr;
		anj_sr_populate_dialog (sg);
	}
	anj_sr_select_nearest (sr, SD_FORWARD);
}

/**
 * anj_sr_select_previous:
 * @sg: Pointer to search data struct
 *
 * Backend for action callback on_findprevious1_activate, etc
 *
 * Return value: None
 */
void
anj_sr_select_previous (SearchReplaceGUI *sg)
{
	SearchReplace *sr;

	if (sg == NULL || sg->dialog == NULL)
	{
		sr = search_get_default_data ();
		if (sr == NULL)
			return;
		anj_sr_create_dialog (sr);
		anj_sr_set_search_string (sr); /* not overwritten by anythig in active search */
	}
	else
	{
		sr = sg->sr;
		anj_sr_populate_dialog (sg);
	}
	anj_sr_select_nearest (sr, SD_BACKWARD);
}

/* there's no gui involved in this action */
void
anj_sr_list_all_uses (const gchar *symbol)
{
	gchar *project_root_uri;
	AnjutaShell* shell;
	SearchReplace *sr;

	sr = search_replace_data_new ();
	sr->search.expr.search_str = g_strdup (symbol);
	sr->search.expr.ignore_case = TRUE;	//CHECKME
	sr->search.expr.whole_word = TRUE;
	sr->search.expr.no_limit = TRUE;
	sr->search.expr.actions_max = G_MAXINT;

	g_object_get (G_OBJECT(sr->docman), "shell", &shell, NULL);
	project_root_uri = NULL;
	anjuta_shell_get (shell, "project_root_uri", G_TYPE_STRING,
					  &project_root_uri, NULL);
	sr->search.range.target =
		project_root_uri != NULL ? SR_PROJECT : SR_OPEN_BUFFERS;
	g_free (project_root_uri);

	sr->search.range.direction = SD_WHOLE;

//	sr->search.range.files.ignore_binary_files = TRUE;
	sr->search.range.files.ignore_hidden_files = TRUE;
	sr->search.range.files.ignore_hidden_dirs = TRUE;
	sr->search.range.files.recurse = TRUE;
	sr->search.action = SA_FIND_PANE;
	sr->search.incremental_wrap = TRUE;

	anj_sr_execute (sr, FALSE);	/* no dialog usage */

	search_replace_data_destroy (sr);
}

/**
 * anj_sr_activate:
 * @replace: TRUE if operation involves replacement
 * @project: TRUE if operation spans all project files
 *
 * Backend for action callbacks: on_find1, on_find_and_replace1, on_find_in_files1
 * These may be used before there is a s/r dialog - one will be created if needed
 *
 * Return value: None
 */
void
anj_sr_activate (gboolean replace, gboolean project)
{
	SearchReplaceGUI *sg;
	SearchReplace *sr;
	GtkWidget *search_entry, *combo;

	sg = NULL;
	sr = NULL;
	anj_sr_get_best_uidata (&sg, &sr);

	if (sr->search.busy)
		return;

	if (sg == NULL || sg->dialog == NULL)
	{
		anj_sr_create_dialog (sr);
		sg = sr->sg;
	}

	search_entry = anj_sr_get_ui_widget (SEARCH_STRING);

	anj_sr_reset_flags (sr);	/* CHECKME selection flag ? just sr->replace.phase = SA_REPL_FIRST; ?*/
	if (!(sg->showing || replace || project))
	{
		/* update the backend search string, it will be be over-written if the
		   active search includes a specified search string */
		anj_sr_set_search_string (sr);
	}
	else /* sg->showing || replace || project */
	{
		if (sg->showing)
		{
			anj_sr_populate_data (sg);
			anj_sr_set_search_string (sr);
			if (search_entry)
				gtk_entry_set_text (GTK_ENTRY (search_entry),
									sr->search.expr.search_str);
		}
		else
		{
			anj_sr_set_search_string (sr);
			anj_sr_populate_dialog (sg);
		}

		anj_sr_reset_replace_buttons (sg);

		if (replace)
		{
			if (!(sr->search.action == SA_REPLACE ||
				  sr->search.action == SA_REPLACEALL))
			{
				anj_sr_set_action (sg, SA_REPLACE);
				sr->search.action = SA_REPLACE;
				anj_sr_show_replace (sg, TRUE);
			}
		}
		else
		{
			if (sr->search.action == SA_REPLACE ||
				sr->search.action == SA_REPLACEALL)
			{
				anj_sr_set_action(sg, SA_SELECT);
				sr->search.action = SA_SELECT;
				anj_sr_show_replace (sg, FALSE);
			}
		}
		if (sr->search.action != SA_REPLACEALL)
			anj_sr_modify_button (sg, SEARCH_BUTTON, button_search_label, GTK_STOCK_FIND);
		else
			anj_sr_modify_button (sg, SEARCH_BUTTON, button_replace_all_label, GTK_STOCK_FIND_AND_REPLACE);

		if (project)
		{
			anj_sr_set_target (sg, SR_PROJECT);
			if (!replace)
			{
				anj_sr_set_action (sg, SA_FIND_PANE);
				anj_sr_set_direction (sg, SD_WHOLE);
			}
		}
		anj_sr_show_replace_button (sg, FALSE);	/* hide the replace button */
		anj_sr_enable_replace_button (sg, FALSE);
	}

	if (search_entry)
	{
		gtk_editable_select_region (GTK_EDITABLE (search_entry), 0, -1);
//		gtk_widget_grab_focus (search_entry);
	}

	combo = anj_sr_get_ui_widget (SEARCH_ACTION_COMBO);
	if (combo)
		gtk_widget_grab_focus (combo);

	/* Show the dialog */
	anj_sr_present_dialog (sg);
}

/**
 * anj_sr_repeat:
 * @sg: pointer to sr data struct containing operation data
 *
 * Repeat previous main (i.e. not-incremental) search operation, using data
 * from that time
 *
 * Return value: none
 */
void
anj_sr_repeat (SearchReplaceGUI *sg)
{
	SearchReplace *sr;

	anj_sr_get_best_uidata (&sg, &sg->sr);
	sr = sg->sr;

	if (!sr->search.busy
		/* minimal check for whether there was a prior search */
		&& anj_sr_create_dialog (sr)
		&& (sr->search.expr.search_str || sr->search.expr.re))
	{
		anj_sr_execute (sr, TRUE);
		/* show search dialog if currently hidden */
		if (!sg->showing)
		{
			gtk_widget_show (sg->dialog);
			sg->showing = TRUE;
		}
	}
}

/****************************************************************/

/**
 * anj_sr_is_idle:
 * @sg: pointer to dialog data struct or NULL
 *
 * Check whether a search is in progress now
 *
 * Return value: TRUE if sg is non-NULL and no search (involving current sr data) is busy
 */
static gboolean
anj_sr_is_idle (SearchReplaceGUI *sg)
{
	SearchReplace *sr;

	if (sg == NULL)
		return FALSE;
	sr = sg->sr;
	g_return_val_if_fail (sr, TRUE);
	return !sr->search.busy;
}

static void
anj_sr_set_popdown_strings (GtkComboBoxEntry *combo, GList* strings)
{
	GtkListStore *store;
	gboolean init;

	init = gtk_combo_box_get_model (GTK_COMBO_BOX(combo)) == NULL;

	store = gtk_list_store_new (1, G_TYPE_STRING);
	for (; strings != NULL; strings = g_list_next(strings))
	{
		GtkTreeIter iter;

		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, strings->data, -1);
	}
	gtk_combo_box_set_model (GTK_COMBO_BOX(combo), GTK_TREE_MODEL (store));
	g_object_unref (store);

	if (init) gtk_combo_box_entry_set_text_column (combo, 0);
}

static void
anj_sr_set_popdown_map (GtkComboBox *combo, AnjutaUtilStringMap *map)
{
	GtkListStore *store;
	gboolean init;
	gint i;

	init = gtk_combo_box_get_model (combo) == NULL;

	store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);
	for (i = 0; map[i].type != -1; ++i)
	{
		GtkTreeIter iter;

		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, map[i].name, 1, map[i].type, -1);
	}
	gtk_combo_box_set_model (combo, GTK_TREE_MODEL (store));
	g_object_unref (store);
	gtk_combo_box_set_active (combo, 0);

	if (init)
	{
		GtkCellRenderer *cell;

		cell = gtk_cell_renderer_text_new ();
  		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, TRUE);
  		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), cell,
    	    	                      "text", 0, NULL);
	}
}

static void
anj_sr_activate_combo_item (GtkComboBox *combo, gint item)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean search;

	model = gtk_combo_box_get_model(combo);

	/* Find item corresponding to command */
	for (search = gtk_tree_model_get_iter_first(model, &iter); search;
		 gtk_tree_model_iter_next (model, &iter))
	{
		gint id;

		gtk_tree_model_get (model, &iter, 1, &id, -1);

		if (id == item)
		{
			gtk_combo_box_set_active_iter (combo, &iter);
			break;
		}
	}

}

static void
anj_sr_activate_combo_id_item (SearchReplaceGUI *sg, GUIElementId id_combo, gint item)
{
	if (anj_sr_is_idle (sg))
	{
		GtkComboBox *combo;

		combo = GTK_COMBO_BOX (anj_sr_get_ui_widget (id_combo));
		anj_sr_activate_combo_item (combo, item);
	}
}

static void
anj_sr_set_action (SearchReplaceGUI *sg, SearchAction action)
{
	anj_sr_activate_combo_id_item (sg, SEARCH_ACTION_COMBO, action);
}

static void
anj_sr_set_target (SearchReplaceGUI *sg, SearchRangeType target)
{
	anj_sr_activate_combo_id_item (sg, SEARCH_TARGET_COMBO, target);
}

/* set "direction" radio-button */
static void
anj_sr_set_direction (SearchReplaceGUI *sg, SearchDirection dir)
{
	if (anj_sr_is_idle (sg))
	{
		GUIElementId id;
		GtkWidget *widget;

		switch (dir)
		{
//			case SD_WHOLE:
			default:
				id = SEARCH_WHOLE;
				break;
			case SD_FORWARD:
				id = SEARCH_FORWARD;
				break;
			case SD_BACKWARD:
				id = SEARCH_BACKWARD;
				break;
		}
		widget = anj_sr_get_ui_widget (id);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
	}
}

/* get enumerator for active radio-button */
static SearchDirection
anj_sr_get_direction (SearchReplaceGUI *sg)
{
	gint i;
	GUIElementId dir_widgets[] =
	{
		SEARCH_WHOLE, SEARCH_FORWARD, SEARCH_BACKWARD
	};
	SearchDirection id[] =
	{
		SD_WHOLE, SD_FORWARD, SD_BACKWARD
	};

	for (i = 0; i < 3; i++)
	{
		GtkWidget *widget;

		widget = sg->widgets[dir_widgets[i]];
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
			return (id[i]);
	}
	return SD_FORWARD;
}

static gint
anj_sr_get_combo_active_value (GtkComboBox *combo)
{
	gint item;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean sel;

	sel = gtk_combo_box_get_active_iter (combo, &iter);
	model = gtk_combo_box_get_model (combo);
	gtk_tree_model_get (model, &iter, 1, &item, -1);

	return item;
}

static gint
anj_sr_get_combo_id_active_value (SearchReplaceGUI *sg, GUIElementId id)
{
	GtkWidget *combo = anj_sr_get_ui_widget (id);
	return anj_sr_get_combo_active_value (GTK_COMBO_BOX (combo));
}

static void
anj_sr_conform_direction_change (SearchReplaceGUI *sg, SearchDirection dir)
{
	SearchRangeType tgt;

	tgt = anj_sr_get_combo_id_active_value (sg, SEARCH_TARGET_COMBO);
	if (dir != SD_WHOLE)
	{
		/* can only go forward|backward in current file, selection, function or block */
		if (tgt == SR_OPEN_BUFFERS || tgt == SR_PROJECT || tgt == SR_FILES)
			anj_sr_set_target (sg, SR_BUFFER);
	}
/*	else	/ * search in all of something * /
	{
		if ((tgt == SR_BUFFER ||tgt == SR_SELECTION
			|| tgt == SR_FUNCTION || tgt == SR_BLOCK))
		{
			SearchAction act;

			act = anj_sr_get_combo_id_active_value (sg, SEARCH_ACTION_COMBO);
			if (act == SA_SELECT)
				anj_sr_set_action (sg, SA_BOOKMARK);
		}
	}
*/
}

/* this assumes an upstream busy-check is ok */
static void
anj_sr_populate_value (SearchReplaceGUI *sg, GUIElementId id, gpointer val_ptr)
{
	GtkWidget *widget;

	g_return_if_fail (id >=0 && id < GUI_ELEMENT_COUNT);
	g_return_if_fail (val_ptr);

	widget = sg->widgets[id];

	switch (glade_widgets[id].type) /* lookup static data */
	{
		case GE_COMBO_ENTRY:
		case GE_TEXT:
			if (*((gchar **) val_ptr))
				g_free(* ((gchar **) val_ptr));
			*((gchar **) val_ptr) = gtk_editable_get_chars
				(GTK_EDITABLE (widget), 0, -1);
			break;
		case GE_BOOLEAN:
			*((gboolean *) val_ptr) = gtk_toggle_button_get_active
			(GTK_TOGGLE_BUTTON (widget));
			break;
		case GE_COMBO:
			g_return_if_fail (glade_widgets[id].extra != NULL);
			*((gint *) val_ptr) = anj_sr_get_combo_active_value
				(GTK_COMBO_BOX (widget));
			break;
		default:
			g_warning ("Bad option %d to anj_sr_populate_value", glade_widgets[id].type);
			break;
	}
}

static void
anj_sr_reset_flags (SearchReplace *sr)
{
	if (!sr->search.busy)
	{
		sr->search.limited = FALSE;
		sr->replace.phase = SA_REPL_FIRST;
	}
}

static void
anj_sr_reset_replace_buttons (SearchReplaceGUI *sg)
{
	if (sg->sr->search.action != SA_REPLACEALL)
		anj_sr_modify_button (sg, SEARCH_BUTTON, button_search_label, GTK_STOCK_FIND);
	else
		anj_sr_modify_button (sg, SEARCH_BUTTON, button_replace_all_label, GTK_STOCK_FIND_AND_REPLACE);

	anj_sr_show_replace_button (sg, FALSE);	/* hide the replace button */
	anj_sr_enable_replace_button (sg, FALSE);
}

/* returns true if the user is allowed to and wants to roll around to the other end */
static gboolean
anj_sr_end_alert (SearchReplace *sr)
{
	GtkWidget *dialog;
	gboolean retval;

	if (sr->search.range.target == SR_BUFFER && sr->search.range.direction != SD_WHOLE)
	{
		// Ask if user wants to wrap around the doc
		// Dialog to be made HIG compliant.
		dialog = gtk_message_dialog_new (GTK_WINDOW (sr->sg->dialog),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_QUESTION,
					GTK_BUTTONS_YES_NO,
					_("The match \"%s\" was not found. Wrap search around the document?"),
					sr->search.expr.search_str);

		gtk_dialog_set_default_response (GTK_DIALOG (dialog),
										 GTK_RESPONSE_YES);
		g_signal_connect (G_OBJECT(dialog), "key-press-event",
						G_CALLBACK(on_search_dialog_key_press), sr->sg);
		retval = (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES);
	}
	else
	{
		dialog = gtk_message_dialog_new (GTK_WINDOW (sr->sg->dialog),
					GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
					_("The match \"%s\" was not found."), sr->search.expr.search_str);
		g_signal_connect (G_OBJECT (dialog), "key-press-event",
						G_CALLBACK(on_search_dialog_key_press), sr->sg);
		gtk_dialog_run (GTK_DIALOG (dialog));
		retval = FALSE;
	}
	gtk_widget_destroy (dialog);
	return retval;
}

static void
anj_sr_max_results_alert (SearchReplace *sr)
{
	GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (sr->sg->dialog),
				GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
		        	_("The maximum number of results has been reached."));
	g_signal_connect (G_OBJECT (dialog), "key-press-event",
					G_CALLBACK(on_search_dialog_key_press), sr->sg);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	anj_sr_reset_flags (sr);
}

static void
anj_sr_total_results_alert (SearchReplace *sr)
{
	GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (sr->sg->dialog),
				GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
		        	_("%u matches have been replaced."), sr->search.matches_sofar);
	g_signal_connect (G_OBJECT (dialog), "key-press-event",
					G_CALLBACK(on_search_dialog_key_press), sr->sg);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	anj_sr_reset_flags (sr);
}

static void
anj_sr_show_replace (SearchReplaceGUI *sg, gboolean hide)
{
	GUIElementId hide_widgets[] =
	{
		REPLACE_REGEX, REPLACE_STRING_COMBO, LABEL_REPLACE
	};
	gint i;
	GtkWidget *widget;

	for (i = 0; i < sizeof(hide_widgets)/sizeof(hide_widgets[0]); i++)
	{
		widget = anj_sr_get_ui_widget (hide_widgets[i]);
		if (widget != NULL)
		{
			if (hide)	/* CHECKME */
				gtk_widget_show (widget);
			else
				gtk_widget_hide (widget);
		}
	}
}

/* change label and/or image of button whose id is @button_id*/
static void
anj_sr_modify_button (SearchReplaceGUI *sg, GUIElementId button_id,
					  const gchar *name, const gchar *stock_id)
{
	GtkWidget *button;

	button = anj_sr_get_ui_widget (button_id);

	if (name == NULL)
		name = "";
	gtk_button_set_label (GTK_BUTTON (button), name);


	if (stock_id != NULL)
	{
		/* FIXME handle non-stock images too */
		GtkWidget *image;
		image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_BUTTON);
		gtk_button_set_image (GTK_BUTTON (button), image);
	}
	else
		gtk_button_set_image (GTK_BUTTON (button), NULL);
}

/********************************************************************/

#define POP_LIST(indx,listvar)\
	s = NULL; anj_sr_populate_value(sg,indx,&s);\
	if (s != NULL)\
	{ anjuta_util_glist_strings_free (sr->search.range.files.listvar);\
	  sr->search.range.files.listvar = anjuta_util_glist_from_string(s); }

/********************************************************************/

/**
 * anj_sr_populate_data:
 * @sr: pointer to search/replace data struct
 *
 * Populate @sr with values from the GUI
 *
 * Return value: none
 */
void
anj_sr_populate_data (SearchReplaceGUI *sg)
{
	SearchReplace *sr;
	gchar *s;

	g_return_if_fail (sg->dialog);

	sr = sg->sr;
	if (sr == NULL)
	{
		sr = sg->sr = search_get_default_data (); //FIXME non-static
	}
	if (sr->search.busy)
		return;

	anj_sr_populate_value (sg, SEARCH_STRING, &(sr->search.expr.search_str));
	anj_sr_populate_value (sg, SEARCH_REGEX, &(sr->search.expr.regex));
	anj_sr_populate_value (sg, IGNORE_CASE, &(sr->search.expr.ignore_case));
	anj_sr_populate_value (sg, WHOLE_WORD, &(sr->search.expr.whole_word));
	anj_sr_populate_value (sg, WHOLE_LINE, &(sr->search.expr.whole_line));
	anj_sr_populate_value (sg, WORD_START, &(sr->search.expr.word_start));
	anj_sr_populate_value (sg, GREEDY, &(sr->search.expr.greedy));
	anj_sr_populate_value (sg, REPLACE_REGEX, &(sr->replace.regex));
	anj_sr_populate_value (sg, SEARCH_TARGET_COMBO, &(sr->search.range.target));
	sr->search.range.direction = anj_sr_get_direction (sg);
	anj_sr_populate_value (sg, ACTIONS_NO_LIMIT, &(sr->search.expr.no_limit));
//	anj_sr_populate_value (SEARCH_BASIC, &(sr->search.basic_search));

	if (sr->search.expr.no_limit)
		sr->search.expr.actions_max = G_MAXINT;
	else
	{
		gchar *max = NULL;

		anj_sr_populate_value (sg, ACTIONS_MAX, &(max));
		if (max)
		{
			sr->search.expr.actions_max = atoi (max);
			if (sr->search.expr.actions_max <= 0)
				sr->search.expr.actions_max = 200;
			g_free (max);
		}
		else
			sr->search.expr.actions_max = 1;
	}

	switch (sr->search.range.target)
	{
		case SR_FUNCTION:	/* CHECKME this seems wrong */
		case SR_BLOCK:
			if (sr->search.limited)
			/* CHECKME why is this setting used for all limited-scope operations */
				sr->search.range.target = SR_SELECTION;
			break;
		case SR_FILES:
			POP_LIST (MATCH_FILES, match_files);
			if (sr->search.range.files.match_files == NULL)
				sr->search.range.files.match_files = g_list_prepend (NULL, g_strdup ("*"));
			POP_LIST (UNMATCH_FILES, ignore_files);
			POP_LIST (MATCH_DIRS, match_dirs);
			if (sr->search.range.files.match_dirs == NULL)
				sr->search.range.files.match_dirs = g_list_prepend (NULL, g_strdup ("*"));
			POP_LIST (UNMATCH_DIRS, ignore_dirs);
		    anj_sr_populate_value (sg, SEARCH_RECURSIVE, &(sr->search.range.files.recurse));
//		    anj_sr_populate_value (sg, IGNORE_BINARY_FILES, &(sr->search.range.files.ignore_binary_files));
		    anj_sr_populate_value (sg, IGNORE_HIDDEN_FILES, &(sr->search.range.files.ignore_hidden_files));
		    anj_sr_populate_value (sg, IGNORE_HIDDEN_DIRS, &(sr->search.range.files.ignore_hidden_dirs));
		default:
			break;
	}
	anj_sr_populate_value (sg, SEARCH_ACTION_COMBO, &(sr->search.action));
	switch (sr->search.action)
	{
		case SA_REPLACE:
		case SA_REPLACEALL:
			anj_sr_populate_value (sg, REPLACE_STRING, &(sr->replace.repl_str));
			anj_sr_populate_value (sg, REPLACE_REGEX, &(sr->replace.regex));
		default:
			break;
	}
}

static void
anj_sr_show_replace_button (SearchReplaceGUI *sg, gboolean show)
{
	GtkWidget *button;

	button = anj_sr_get_ui_widget (REPLACE_BUTTON);
	if (show)
		gtk_widget_show (button);
	else
		gtk_widget_hide (button);
}

static void
anj_sr_enable_replace_button (SearchReplaceGUI *sg, gboolean sensitive)
{
	GtkWidget *button;

	button = anj_sr_get_ui_widget (REPLACE_BUTTON);
	gtk_widget_set_sensitive (button, sensitive);
}

/**
 * anj_sr_get_current_uidata:
 *
 * Get pointer to static data struct with search-dialog data
 *
 * Return value: the data
 */
SearchReplaceGUI *
anj_sr_get_default_uidata (void)
{
	/* CHECKME relevance of other sg* when a search is active */
	return def_sg;
}

/**
 * anj_sr_get_best_uidata:
 * sg: store for pointer to gui data, or NULL if not wanted
 * sr: store for pointer to s/r data, or NULL if not wanted
 *
 * Get pointers to search data
 *
 * Return value: None
 */
void
anj_sr_get_best_uidata (SearchReplaceGUI **sg, SearchReplace **sr)
{
	if (sg != NULL)
	{
		if (*sg == NULL)
			*sg = def_sg;	/* FIXME find any other current dialog ?? */
		if (sr != NULL)
		{
			if (*sg != NULL && (*sg)->sr != NULL)
				*sr = (*sg)->sr;
			else
				*sr = search_get_default_data ();
		}
	}
	else /* sg == NULL */
		if (sr != NULL)
	{
		*sr = search_get_default_data ();
	}
}

SearchReplaceGUI *
anj_sr_get_current_uidata (GtkWidget *widget)
{
	GtkWidget *window;
	gpointer data;

	window = gtk_widget_get_toplevel (widget);
	g_return_val_if_fail (GTK_WIDGET_TOPLEVEL (window), NULL);

	data = g_object_get_data (G_OBJECT (window), SEARCH_GUI_KEY);
	g_return_val_if_fail (data != NULL, NULL);
	return (SearchReplaceGUI *) data;
}

void
anj_sr_set_dialog_searchdata (SearchReplaceGUI *sg, SearchReplace *sr)
{
	g_return_if_fail (sg && sr);
	sg->sr = sr;
	sr->sg = sg;
}

static
void anj_sr_translate_dialog_strings (AnjutaUtilStringMap labels[])
{
	guint i;

	for (i = 0; labels[i].name != NULL; i++)
		labels[i].name = gettext (labels[i].name);
}

/**
 * anj_sr_create_dialog:
 * @sr: s/r data struct, currently this assumes there's only one of those
 *
 * Create a new s/r dialog, with settings initialised from preferences data
 * Dialog is not shown, as widget values may need to be changed before that happens
 *
 * Return value: the data
 */
static gboolean
anj_sr_create_dialog (SearchReplace *sr)
{
	static gboolean labels_translated = FALSE; /* ensure dialog labels are translated only once */
	SearchReplaceGUI *sg;
	GtkWidget *widget;
	GladeXML *xml;
	gint i;

	g_return_val_if_fail (sr != NULL, FALSE);
	if (sr->sg != NULL && sr->sg->dialog != NULL)	/* dialog already created */
		return TRUE;

	xml = glade_xml_new (GLADE_FILE_SEARCH_REPLACE, SEARCH_REPLACE_DIALOG, NULL);
	if (xml == NULL)
	{
		anjuta_util_dialog_error (NULL, _("Unable to build user interface for Search And Replace"));
		return FALSE;
	}

	if (!labels_translated)
	{
		labels_translated = TRUE;
		anj_sr_translate_dialog_strings (search_target_strings);
		anj_sr_translate_dialog_strings (search_action_strings);
		button_search_label = _("S_earch");	/* s,a,r,c are unavailable for mnemonic */
		button_replace_label = _("_Replace");
		button_replace_all_label = _("Repace _all");
	}

	sg = g_slice_alloc (sizeof (SearchReplaceGUI));
	sg->xml = xml;
	sg->dialog = glade_xml_get_widget (sg->xml, SEARCH_REPLACE_DIALOG);
/*	gtk_window_set_transient_for (GTK_WINDOW (sg->dialog),
								  GTK_WINDOW (app->widgets.window)); */
	/* all dialog widgets can access sg, and therefore sr, by getting this data
	   from their toplevel window */
	g_object_set_data (G_OBJECT (sg->dialog), SEARCH_GUI_KEY, sg);
	anj_sr_set_dialog_searchdata (sg, sr);

	for (i = 0; i < GUI_ELEMENT_COUNT; i++)
	{
		widget = glade_xml_get_widget (xml, glade_widgets[i].name);
		if (glade_widgets[i].type == GE_BUTTON)
		{
			gtk_button_set_use_underline (GTK_BUTTON (widget), TRUE);
		}
		else if (glade_widgets[i].type == GE_COMBO_ENTRY)
		{
			/* Get child of GtkComboBoxEntry */
			widget = GTK_BIN (widget)->child;
		}
		/* CHECKME no obvious reason for this
		g_object_ref (G_OBJECT (w->widget));*/
		if (glade_widgets[i].type == GE_COMBO && glade_widgets[i].extra != NULL)
		{
			anj_sr_set_popdown_map (GTK_COMBO_BOX (widget),
									(AnjutaUtilStringMap *)glade_widgets[i].extra);
		}
		sg->widgets[i] = widget;
	}

	/* glade can't independently set stock-button labels */
	anj_sr_modify_button (sg, SEARCH_BUTTON, button_search_label, GTK_STOCK_FIND);
	anj_sr_modify_button (sg, REPLACE_BUTTON, button_replace_label, GTK_STOCK_FIND_AND_REPLACE);

	/* replace button presented when that action is in progress */
	anj_sr_show_replace_button (sg, FALSE);
	anj_sr_enable_replace_button (sg, FALSE);
	widget = anj_sr_get_ui_widget (ACTIONS_MAX);
	gtk_widget_set_sensitive (widget, FALSE);

	/* other callbacks which don't need specific data are connected via glade */
	g_signal_connect (G_OBJECT (sg->dialog), "key-press-event",
					G_CALLBACK (on_search_dialog_key_press), sg);
	widget = anj_sr_get_ui_widget (SEARCH_WHOLE);
	g_signal_connect (G_OBJECT (widget), "toggled",
					G_CALLBACK (on_search_direction_changed),
					GINT_TO_POINTER (SD_WHOLE));
	widget = anj_sr_get_ui_widget (SEARCH_FORWARD);
	g_signal_connect (G_OBJECT (widget), "toggled",
					G_CALLBACK (on_search_direction_changed),
					GINT_TO_POINTER (SD_FORWARD));
	widget = anj_sr_get_ui_widget (SEARCH_BACKWARD);
	g_signal_connect (G_OBJECT (widget), "toggled",
					G_CALLBACK (on_search_direction_changed),
					GINT_TO_POINTER (SD_BACKWARD));

	glade_xml_signal_autoconnect (sg->xml);

	sg->showing = FALSE;
	def_sg = sg;	//FIXME non-static data
	return TRUE;
}

static void
anj_sr_present_dialog (SearchReplaceGUI *sg)
{
	gtk_window_present (GTK_WINDOW (sg->dialog));
	sg->showing = TRUE;
}

/* note: no included search_replace_data_destroy(sg->sr) */
void
anj_sr_destroy_ui_data (SearchReplaceGUI *sg)
{
	if (sg != NULL)
	{
		if (sg->dialog != NULL)
			gtk_widget_destroy (sg->dialog);
		if (sg->xml != NULL)
			g_object_unref (G_OBJECT (sg->xml));

		g_slice_free1 (sizeof (SearchReplaceGUI), sg);
	}
}

static gboolean
anj_sr_find_in_list (GList *list, gchar *word)
{
	GList *node;

	for (node = list; node != NULL; node = g_list_next (node))
	{
		if (strcmp ((gchar *)node->data, word) == 0)
			return TRUE;
	}
	return FALSE;
}

/*  Remove last item of the list if > nb_max  */
static void
anj_sr_trim_list (GList **list, guint nb_max)
{
	GList *node;

	node = *list;
	if (node != NULL && g_list_length (node) > nb_max)
	{

		node = g_list_last (node);
		g_free (node->data);
		*list = g_list_delete_link (*list, node);
	}
}

static void
anj_sr_update_search_combos (SearchReplaceGUI *sg)
{
	SearchReplace *sr;
	IAnjutaDocument *doc;

	sr = sg->sr;
	g_return_if_fail (sr != NULL);

	doc = ianjuta_document_manager_get_current_document (sr->docman, NULL);
	if (doc && IANJUTA_IS_EDITOR (doc))
	{
		GtkWidget *search_entry;

		sg = sr->sg;
		search_entry = anj_sr_get_ui_widget (SEARCH_STRING);
		if (search_entry != NULL)
		{
			gchar *search_word;

			search_word = g_strdup (gtk_entry_get_text (GTK_ENTRY (search_entry)));
			if (search_word != NULL && *search_word != 0)
			{
				if (!anj_sr_find_in_list (sr->search.expr_history, search_word))
				{
					GtkWidget *search_list =
						anj_sr_get_ui_widget (SEARCH_STRING_COMBO);
					sr->search.expr_history = g_list_prepend (sr->search.expr_history,
						search_word);
					anj_sr_trim_list (&sr->search.expr_history,
										MAX_ITEMS_SEARCH_COMBO);
					anj_sr_set_popdown_strings(GTK_COMBO_BOX_ENTRY (search_list),
						sr->search.expr_history);

					//search_toolbar_set_text(search_word);
					//  FIXME  comboentry instead of entry
					//~ entry_set_text_n_select (app->widgets.toolbar.main_toolbar.find_entry,
									 //~ search_word, FALSE);
				}
			}
		}
	}
}

static void
anj_sr_update_replace_combos (SearchReplaceGUI *sg)
{
	SearchReplace *sr;
	IAnjutaDocument *doc;

	sr = sg->sr;
	g_return_if_fail (sr != NULL);

	doc = ianjuta_document_manager_get_current_document (sr->docman, NULL);
	if (IANJUTA_IS_EDITOR (doc))
	{
		GtkWidget *replace_entry;

		sg = sr->sg;
		replace_entry = anj_sr_get_ui_widget (REPLACE_STRING);
		if (replace_entry != NULL)
		{
			gchar *replace_word;

			replace_word = g_strdup (gtk_entry_get_text (GTK_ENTRY (replace_entry)));
			if (replace_word != NULL && *replace_word != 0)
			{
				if (!anj_sr_find_in_list(sr->replace.expr_history, replace_word))
				{
					GtkWidget *replace_list =
						anj_sr_get_ui_widget (REPLACE_STRING_COMBO);
					sr->replace.expr_history = g_list_prepend(sr->replace.expr_history,
						replace_word);
					anj_sr_trim_list (&sr->replace.expr_history,
										MAX_ITEMS_SEARCH_COMBO);
					anj_sr_set_popdown_strings (GTK_COMBO_BOX_ENTRY (replace_list),
						sr->replace.expr_history);
				}
			}
		}
	}
}

void
anj_sr_populate_dialog (SearchReplaceGUI *sg)
{
	SearchReplace *sr;
	GtkWidget *widget;
	Search *s;

	sr = sg->sr;
	g_return_if_fail (sr != NULL);

	s = &(sr->search);

	if (s->expr.search_str)
	{
		widget = anj_sr_get_ui_widget (SEARCH_STRING);
		gtk_entry_set_text(GTK_ENTRY(widget), s->expr.search_str);
	}
	if (sr->replace.repl_str)
	{
		widget = anj_sr_get_ui_widget (REPLACE_STRING);
		gtk_entry_set_text(GTK_ENTRY(widget), sr->replace.repl_str);
	}
	widget = anj_sr_get_ui_widget (SEARCH_REGEX);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), s->expr.regex);
	widget = anj_sr_get_ui_widget (IGNORE_CASE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), s->expr.ignore_case);
	widget = anj_sr_get_ui_widget (WHOLE_WORD);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), s->expr.whole_word);
	widget = anj_sr_get_ui_widget (WHOLE_LINE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), s->expr.whole_line);
	widget = anj_sr_get_ui_widget (WORD_START);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), s->expr.word_start);
	widget = anj_sr_get_ui_widget (GREEDY);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), s->expr.greedy);
	widget = anj_sr_get_ui_widget (REPLACE_REGEX);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), sr->replace.regex);

	if (s->expr.no_limit)
		widget = anj_sr_get_ui_widget (ACTIONS_NO_LIMIT);
	else
		widget = anj_sr_get_ui_widget (ACTIONS_LIMIT);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (widget), TRUE);

	widget = anj_sr_get_ui_widget (ACTIONS_MAX);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), s->expr.actions_max);

	widget = anj_sr_get_ui_widget (GREEDY);
	gtk_widget_set_sensitive(widget, s->expr.regex);
	widget = anj_sr_get_ui_widget (REPLACE_REGEX);
	gtk_widget_set_sensitive(widget, s->expr.regex);

	widget = anj_sr_get_ui_widget (SEARCH_BUTTON);
	gtk_widget_set_sensitive (widget, (s->expr.search_str != NULL) && (*s->expr.search_str != '\0'));
	widget = anj_sr_get_ui_widget (REPLACE_BUTTON);
	gtk_widget_set_sensitive (widget, (s->expr.search_str != NULL) && (*s->expr.search_str != '\0'));

	anj_sr_set_direction (sg, s->range.direction);

	widget = anj_sr_get_ui_widget (SEARCH_ACTION_COMBO);
	anj_sr_activate_combo_item (GTK_COMBO_BOX(widget), s->action);

	anj_sr_show_replace (sg, s->action == SA_REPLACE || s->action == SA_REPLACEALL);

	widget = anj_sr_get_ui_widget (SEARCH_TARGET_COMBO);
	anj_sr_activate_combo_item (GTK_COMBO_BOX(widget), s->range.target);
/* CHECKME populate specific files combo's ? */
	widget = anj_sr_get_ui_widget (SEARCH_RECURSIVE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (widget), s->range.files.recurse);
//	widget = anj_sr_get_ui_widget (IGNORE_BINARY_FILES);
//	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (widget), s->range.files.ignore_binary_files);
	widget = anj_sr_get_ui_widget (IGNORE_HIDDEN_FILES);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (widget), s->range.files.ignore_hidden_files);
	widget = anj_sr_get_ui_widget (IGNORE_HIDDEN_DIRS);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (widget), s->range.files.ignore_hidden_dirs);

//	widget = anj_sr_get_ui_widget (SEARCH_BASIC);
//	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), s->basic_search);

	widget = anj_sr_get_ui_widget (STOP_BUTTON);
	gtk_widget_set_sensitive (widget, FALSE);

//	basic_search_toggled ();
}
/*
static void
basic_search_toggled (void)
{
	GtkToggleButton *togglebutton;
	SearchReplaceGUI *sg;

	sg = ;
	togglebutton = GTK_TOGGLE_BUTTON(anj_sr_get_ui_widget (SEARCH_BASIC));
	on_setting_basic_search_toggled (togglebutton, NULL);	/ * don't care if busy * /
}
*/
/* hide dialog and cleanup if currently supended awaiting replacement confirmation */
static void
anj_sr_interrupt_nicely (SearchReplaceGUI *sg)
{
	SearchReplace *sr;

	sr = sg->sr;
	if (sg->showing)	/* should never fail */
	{
		GtkWidget *button;

		gtk_widget_hide (sg->dialog);
		sg->showing = FALSE;
		button = anj_sr_get_ui_widget (STOP_BUTTON);
		gtk_widget_set_sensitive (button, FALSE);
		anj_sr_reset_flags (sr);	/* CHECKME selection flag ? just sr->replace.phase = SA_REPL_FIRST; ?*/
//		anj_sr_reset_replace_buttons (sr);
		anj_sr_enable_replace_button (sg, FALSE);
	}
	if (sr->search.action == SA_REPLACE)
	{
//		while (sr->replace.phase != SA_REPL_CONFIRM)
//			usleep (10000);	/* must wait until current search ends */
		sr->search.stop_count = -1;	/* trigger an abort on next pass */
		/* cleanup ready for next document */
		//CHECKME report count of processed items ?
		sr->search.matches_sofar = 0;
		sr->replace.phase = SA_REPL_FIRST;
	}
	sr->search.busy = FALSE;
}

/* -------------- Callbacks --------------------- */

/* delete-event callback for main s/r window */
gboolean
on_search_dialog_delete_event (GtkWidget *window, GdkEvent *event,
								gpointer user_data)
{
	SearchReplaceGUI *sg;

	sg = anj_sr_get_current_uidata (window);
	if (sg != NULL)
		anj_sr_interrupt_nicely (sg);
	return TRUE;
}

/* key-press callback for various dialog's entry-widget(s) */
static gboolean
on_search_dialog_key_press (GtkWidget *widget, GdkEventKey *event,
							gpointer user_data)
{
	SearchReplaceGUI *sg;

	sg = (SearchReplaceGUI *) user_data;	/* for speed, this time we get the data directly */

	if (event->keyval == GDK_Escape)
	{
		if (widget == sg->dialog)
		{
			/* Escape pressed in Find window */
//			gtk_widget_hide (widget);
//			sg->showing = FALSE;
			anj_sr_interrupt_nicely (sg);
		}
		else
		{
			/* Escape pressed in message dialog */
			gtk_dialog_response (GTK_DIALOG (widget), GTK_RESPONSE_NO);
		}
		return TRUE;
	}
	else if (widget == sg->dialog)
	{
		if (sg->sr->search.busy)
			return TRUE;	/* prevent string changes while replacing interactively */
		/* FIXME get relevant shortcuts instead of this */
		if ((event->state & GDK_CONTROL_MASK) &&
				((event->keyval & 0x5F) == GDK_G))
		{
			if (event->state & GDK_SHIFT_MASK)
				anj_sr_select_previous (sg);
			else
				anj_sr_select_next (sg);
			return TRUE;
		}
	}
	return FALSE;
}

static void
anj_sr_disconnect_set_toggle_connect (SearchReplaceGUI *sg, GUIElementId id,
									  GCallback function, gboolean active)
{
	GtkWidget *button;

	button = anj_sr_get_ui_widget (id);
	g_signal_handlers_disconnect_by_func(G_OBJECT(button), function, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), active);
	g_signal_connect(G_OBJECT(button), "toggled", function, NULL);
}


void
on_search_match_whole_word_toggled (GtkToggleButton *togglebutton,
									gpointer user_data)
{
	SearchReplaceGUI *sg;
	gboolean state;

	state = gtk_toggle_button_get_active (togglebutton);
	sg = anj_sr_get_current_uidata (GTK_WIDGET (togglebutton));
	if (anj_sr_is_idle (sg))
	{
		if (state)
		{
			anj_sr_disconnect_set_toggle_connect (sg, WHOLE_LINE,
				(GCallback)on_search_match_whole_line_toggled, FALSE);
			anj_sr_disconnect_set_toggle_connect (sg, WORD_START,
				(GCallback)on_search_match_word_start_toggled, FALSE);
		}
	}
	else
	{
		g_signal_handlers_block_by_func (G_OBJECT (togglebutton),
										(gpointer) on_search_match_whole_word_toggled,
										user_data);
		gtk_toggle_button_set_active (togglebutton, !state);
		g_signal_handlers_unblock_by_func (G_OBJECT (togglebutton),
										(gpointer) on_search_match_whole_word_toggled,
										user_data);
	}
}

void
on_search_match_whole_line_toggled (GtkToggleButton *togglebutton,
									gpointer user_data)
{
	SearchReplaceGUI *sg;
	gboolean state;

	state = gtk_toggle_button_get_active (togglebutton);
	sg = anj_sr_get_current_uidata (GTK_WIDGET (togglebutton));
	if (anj_sr_is_idle (sg))
	{
		if (state)
		{
			anj_sr_disconnect_set_toggle_connect (sg, WHOLE_WORD,
				(GCallback)on_search_match_whole_word_toggled, FALSE);
			anj_sr_disconnect_set_toggle_connect (sg, WORD_START,
				(GCallback)on_search_match_word_start_toggled, FALSE);
		}
	}
	else
	{
		g_signal_handlers_block_by_func (G_OBJECT (togglebutton),
										(gpointer) on_search_match_whole_line_toggled,
										user_data);
		gtk_toggle_button_set_active (togglebutton, !state);
		g_signal_handlers_unblock_by_func (G_OBJECT (togglebutton),
										(gpointer) on_search_match_whole_line_toggled,
										user_data);
	}
}

void
on_search_match_word_start_toggled (GtkToggleButton *togglebutton,
									gpointer user_data)
{
	SearchReplaceGUI *sg;
	gboolean state;

	state = gtk_toggle_button_get_active (togglebutton);
	sg = anj_sr_get_current_uidata (GTK_WIDGET (togglebutton));
	if (anj_sr_is_idle (sg))
	{
		if (state)
		{
			anj_sr_disconnect_set_toggle_connect (sg, WHOLE_WORD,
				(GCallback)on_search_match_whole_word_toggled, FALSE);
			anj_sr_disconnect_set_toggle_connect (sg, WHOLE_LINE,
				(GCallback)on_search_match_whole_line_toggled, FALSE);
		}
	}
	else
	{
		g_signal_handlers_block_by_func (G_OBJECT (togglebutton),
										(gpointer) on_search_match_word_start_toggled,
										user_data);
		gtk_toggle_button_set_active (togglebutton, !state);
		g_signal_handlers_unblock_by_func (G_OBJECT (togglebutton),
										(gpointer) on_search_match_word_start_toggled,
										user_data);
	}
}

/*
static void
search_make_sensitive (gboolean sensitive)
{
	static char *widgets[] = {
		SEARCH_EXPR_FRAME, SEARCH_TARGET_FRAME, CLOSE_BUTTON, SEARCH_BUTTON,
		JUMP_BUTTON
	};
	gint i;
	GtkWidget *widget;
	SearchReplaceGUI *sg;

	sg = ;

	for (i=0; i < sizeof(widgets)/sizeof(widgets[0]); ++i)
	{
		widget = anj_sr_get_ui_widget (widgets[i]);
		if (NULL != widget)
			gtk_widget_set_sensitive(widget, sensitive);
	}
}
*/

void
on_search_regex_toggled (GtkToggleButton *togglebutton, gpointer user_data)
{
	SearchReplaceGUI *sg;
	gboolean state;

	state = gtk_toggle_button_get_active (togglebutton);
	sg = anj_sr_get_current_uidata (GTK_WIDGET (togglebutton));
	if (anj_sr_is_idle (sg))
	{
		SearchAction act;
		GUIElementId dependent_widgets[] =
		{
			GREEDY, REPLACE_REGEX, IGNORE_CASE, WHOLE_WORD, WHOLE_LINE, WORD_START
		};
		GtkWidget *widget;
		gint i;

		widget = anj_sr_get_ui_widget (SEARCH_BACKWARD); /* SD_WHOLE is ok */
		if (state)
		{
			if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
				anj_sr_set_direction (sg, SD_FORWARD);
		}
		gtk_widget_set_sensitive (widget, !state);

		for (i = 0; i < sizeof(dependent_widgets)/sizeof(dependent_widgets[0]); ++i)
		{
			widget = anj_sr_get_ui_widget (dependent_widgets[i]);
			if (widget != NULL)
			{
				if (i < 2) /* GREEDY and REPLACE_REGEX work only with regex */
					gtk_widget_set_sensitive (widget, state);
				else	/* the other options must be expressed in the regex */
				{
					gtk_widget_set_sensitive (widget, !state);
					gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), FALSE);
				}
			}
		}

		act = anj_sr_get_combo_id_active_value (sg, SEARCH_ACTION_COMBO);
		if (act == SA_REPLACEALL)
		{
			SearchRangeType tgt;

			tgt = anj_sr_get_combo_id_active_value (sg, SEARCH_TARGET_COMBO);
			if (tgt == SR_OPEN_BUFFERS || tgt == SR_PROJECT || tgt == SR_FILES)
			{
				widget = anj_sr_get_ui_widget (ACTIONS_LIMIT);
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
			}
		}
	}
	else /* busy*/
	{
		g_signal_handlers_block_by_func (G_OBJECT (togglebutton),
										(gpointer) on_search_regex_toggled,
										user_data);
		gtk_toggle_button_set_active (togglebutton, !state);
		g_signal_handlers_unblock_by_func (G_OBJECT (togglebutton),
										(gpointer) on_search_regex_toggled,
										user_data);
	}
}

void
on_search_actions_no_limit_toggled (GtkToggleButton *togglebutton, gpointer user_data)
{
	SearchReplaceGUI *sg;
	gboolean state;

	state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (togglebutton));
	sg = anj_sr_get_current_uidata (GTK_WIDGET (togglebutton));
	if (anj_sr_is_idle (sg))
	{
		GtkWidget *actions_max;

		actions_max = anj_sr_get_ui_widget (ACTIONS_MAX);
		gtk_widget_set_sensitive (actions_max, !state);
	}
	else
	{
		g_signal_handlers_block_by_func (G_OBJECT (togglebutton),
										(gpointer) on_search_actions_no_limit_toggled,
										user_data);
		gtk_toggle_button_set_active (togglebutton, !state);
		g_signal_handlers_unblock_by_func (G_OBJECT (togglebutton),
										(gpointer) on_search_actions_no_limit_toggled,
										user_data);
	}
}

/* for old dialog
static void
search_set_toggle_direction (SearchDirection dir)
{
	switch (dir)
	{
//		case SD_WHOLE :
		default:
			anj_sr_disconnect_set_toggle_connect (SEARCH_FULL_BUFFER,
				(GCallback) on_search_full_buffer_toggled, TRUE);
			break;
		case SD_FORWARD :
			anj_sr_disconnect_set_toggle_connect (SEARCH_FORWARD,
				(GCallback) on_search_forward_toggled, TRUE);
			break;
		case SD_BACKWARD :
			anj_sr_disconnect_set_toggle_connect (SEARCH_BACKWARD,
				(GCallback) on_search_backward_toggled, TRUE);
			break;
	}
}
*/

/* idle callback to revert a radio-button changed when a search is in progress */
static gboolean
anj_sr_revert_button (GtkWidget *btn)
{
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (btn), TRUE);
	return FALSE;
}

/* callback for all search-direction radio-buttons
	user_data is a pointerised enumerator of corresponding SearchDirection */
void
on_search_direction_changed (GtkToggleButton *togglebutton, gpointer user_data)
{
	SearchReplaceGUI *sg;
	gboolean state;
	SearchDirection dir;

	sg = anj_sr_get_current_uidata (GTK_WIDGET (togglebutton));
	if (sg == NULL)
		return;

	dir = GPOINTER_TO_INT (user_data);
	state = gtk_toggle_button_get_active (togglebutton);
	if (anj_sr_is_idle (sg))
	{
		if (state)
		{
			anj_sr_conform_direction_change (sg, dir);
		}
	}
	else
	{
		if (state)
		{
			SearchReplace *sr;

			sr = sg->sr;
			if (sr->search.range.direction != dir) /* attempt to change away from blocked value */
			{
				/* setup to re-activate the current button when allowed */
				gint i;
				SearchDirection id[] =
				{
					SD_WHOLE, SD_FORWARD, SD_BACKWARD
				};
				GUIElementId dir_widgets[] =
				{
					SEARCH_WHOLE, SEARCH_FORWARD, SEARCH_BACKWARD
				};

				for (i = 0; i < 3; i++)
				{
					if (sr->search.range.direction == id[i])
					{
						GtkWidget *btn;

						btn = anj_sr_get_ui_widget (dir_widgets[i]);
						g_idle_add ((GSourceFunc) anj_sr_revert_button, btn);
						break;
					}
				}
			}
		}
	}
}

void
on_search_action_changed (GtkComboBox *combo, gpointer user_data)
{
	SearchReplaceGUI *sg;

	sg = anj_sr_get_current_uidata (GTK_WIDGET (combo));
	if (anj_sr_is_idle (sg))
	{
		SearchAction act;
		SearchRangeType tgt;
		GtkWidget *wid;

	//	anj_sr_reset_flags ((SearchReplace *)user_data);	/* CHECKME bad to clear selection scope flag, we may have just selected that*/
		sg->sr->replace.phase = SA_REPL_FIRST;
		act = anj_sr_get_combo_active_value (combo);
		tgt = anj_sr_get_combo_id_active_value (sg, SEARCH_ACTION_COMBO);

		switch (act)
		{
			case SA_SELECT:
				anj_sr_show_replace (sg, FALSE);	/* hide all replacment widgets */
				anj_sr_show_replace_button (sg, FALSE);	/* hide the replace button */
				anj_sr_modify_button (sg, SEARCH_BUTTON, button_search_label, GTK_STOCK_FIND);
//				if (tgt == SR_OPEN_BUFFERS || tgt == SR_PROJECT || tgt == SR_FILES)
//					anj_sr_set_target (sg, SR_BUFFER);
				break;
			case SA_REPLACE:
				anj_sr_show_replace (sg, TRUE);
				anj_sr_enable_replace_button (sg, FALSE);
				anj_sr_show_replace_button (sg, TRUE);	/* show (insensitive) replace button */
				anj_sr_modify_button (sg, SEARCH_BUTTON, button_search_label, GTK_STOCK_FIND);
//				if (tgt == SR_OPEN_BUFFERS || tgt == SR_PROJECT || tgt == SR_FILES)
//					anj_sr_set_target (sg, SR_BUFFER);
				/* CHECKME something safe if regex is active ? */
				break;
			case SA_REPLACEALL:
				anj_sr_show_replace (sg, TRUE);
				anj_sr_show_replace_button (sg, FALSE);	/* the search button is the starter */
				anj_sr_modify_button (sg, SEARCH_BUTTON, button_replace_all_label, GTK_STOCK_FIND_AND_REPLACE);
				wid = anj_sr_get_ui_widget (SEARCH_REGEX);
				if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (wid)))
				{
					wid = anj_sr_get_ui_widget (ACTIONS_LIMIT);
					gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (wid), TRUE);
					/* CHECKME other safety if regex is active ? */
				}
				break;
			case SA_HIGHLIGHT:
			case SA_BOOKMARK:
				if (tgt == SR_PROJECT || tgt == SR_FILES)
				{
					anj_sr_set_target (sg, SR_BUFFER);
					anj_sr_set_direction (sg, SD_WHOLE);
				}
				anj_sr_show_replace (sg, FALSE);
				anj_sr_show_replace_button (sg, FALSE);
				anj_sr_modify_button (sg, SEARCH_BUTTON, button_search_label, GTK_STOCK_FIND);
			case SA_UNLIGHT:
				if (!(tgt == SR_BUFFER || tgt == SR_OPEN_BUFFERS))
					anj_sr_set_target (sg, SR_BUFFER);
				anj_sr_set_direction (sg, SD_WHOLE);
				/* CHECKME desensitize direction combo */
			default:
				anj_sr_show_replace (sg, FALSE);
				anj_sr_show_replace_button (sg, FALSE);
				anj_sr_modify_button (sg, SEARCH_BUTTON, button_search_label, GTK_STOCK_FIND);
				break;
		}
//		g_object_set_data (G_OBJECT (combo), "OLDINDX", GINT_TO_POINTER (act));
	}
/*	else
	{
		gint oldindex;

		oldindex = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (combo), "OLDINDX"));
		g_signal_handlers_block_by_func (G_OBJECT (combo),
										(gpointer) on_search_action_changed,
										user_data);
		gtk_combo_box_set_active (combo, oldindex);
		g_signal_handlers_unblock_by_func (G_OBJECT (combo),
										(gpointer) on_search_action_changed,
										user_data);
	} */
}

void
on_search_target_changed (GtkComboBox *combo, gpointer user_data)
{
	SearchReplaceGUI *sg;

	sg = anj_sr_get_current_uidata (GTK_WIDGET (combo));
	if (anj_sr_is_idle (sg))
	{
		SearchReplace *sr;
		SearchRangeType tgt;
//		SearchDirection dir;
		SearchAction act;
		GtkWidget *file_filter_frame;
		
		file_filter_frame = anj_sr_get_ui_widget (FILE_FILTER_FRAME);
		tgt = anj_sr_get_combo_active_value (combo);
		switch (tgt)
		{
			case SR_FILES:
				gtk_widget_show (file_filter_frame);
				break;
			default:
				gtk_widget_hide (file_filter_frame);
				break;
		}

		sr = sg->sr;
//		dir = anj_sr_get_direction (sr->sg);

		if (tgt == SR_OPEN_BUFFERS || tgt == SR_PROJECT || tgt == SR_FILES)
		{
			anj_sr_set_direction (sg, SD_WHOLE);

			act = anj_sr_get_combo_id_active_value (sg, SEARCH_ACTION_COMBO);
			if (act != SA_REPLACE && act != SA_REPLACEALL)
			{
				if (tgt == SR_OPEN_BUFFERS)
					anj_sr_set_action (sg, SA_BOOKMARK);
				else
					anj_sr_set_action (sg, SA_FIND_PANE);
			}
			else
			{
				anj_sr_set_action (sg, SA_REPLACEALL);
				sr->search.action = SA_REPLACEALL;
			}
		}
		if (!(tgt == SR_BUFFER || tgt == SR_OPEN_BUFFERS))
		{
			act = anj_sr_get_combo_id_active_value (sg, SEARCH_ACTION_COMBO);
			if (act == SA_UNLIGHT)
			{
				anj_sr_set_action (sg, SA_SELECT);
				anj_sr_set_direction (sg, SD_FORWARD);
			}
		}

	//	anj_sr_reset_flags (sr);	/* CHECKME selection flag ? just sr->replace.phase = SA_REPL_FIRST; ?*/
		sr->replace.phase = SA_REPL_FIRST;
		/*  Resize dialog  */
		gtk_window_resize (GTK_WINDOW (sr->sg->dialog), 10, 10);
//		g_object_set_data (G_OBJECT (combo), "OLDINDX", GINT_TO_POINTER (tgt));
	}
/*	else
	{
		gint oldindex;

		oldindex = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (combo), "OLDINDX"));
		g_signal_handlers_block_by_func (G_OBJECT (combo),
										(gpointer) on_search_target_changed,
										user_data);
		gtk_combo_box_set_active (combo, oldindex);
		g_signal_handlers_unblock_by_func (G_OBJECT (combo),
										(gpointer) on_search_target_changed,
										user_data);
	} */
}

void
on_search_expression_changed (GtkComboBox *combo, gpointer user_data)
{
	SearchReplaceGUI *sg;
	GtkWidget *search_entry;
	GtkWidget *widget;
	gboolean sensitive;

	sg = anj_sr_get_current_uidata (GTK_WIDGET (combo));
	if (anj_sr_is_idle (sg))
	{
	//	search_entry = anj_sr_get_ui_widget (SEARCH_STRING);
		search_entry = GTK_BIN (combo)->child;
		sensitive = (*gtk_entry_get_text (GTK_ENTRY (search_entry)) != '\0');
		widget = anj_sr_get_ui_widget (SEARCH_BUTTON);
		gtk_widget_set_sensitive (widget, sensitive);
//		g_object_set_data (G_OBJECT (combo), "OLDINDX", GINT_TO_POINTER (?));
	}
/*	else
	{
		gint oldindex;

		oldindex = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (combo), "OLDINDX"));
		g_signal_handlers_block_by_func (G_OBJECT (combo),
										(gpointer) on_search_expression_changed,
										user_data);
		gtk_combo_box_set_active (combo, oldindex);
		g_signal_handlers_unblock_by_func (G_OBJECT (combo),
										(gpointer) on_search_expression_changed,
										user_data);
	} */
}
/*
void
on_replace_expression_changed (GtkComboBox *combo, gpointer user_data)
{
	SearchReplaceGUI *sg;

	sg = anj_sr_get_current_uidata (GTK_WIDGET (combo));
	if (anj_sr_is_idle (sg))
	{
		g_object_set_data (G_OBJECT (combo), "OLDINDX", GINT_TO_POINTER (?));
	}
	else
	{
		gint oldindex;

		oldindex = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (combo), "OLDINDX"));
		g_signal_handlers_block_by_func (G_OBJECT (combo),
										(gpointer) on_replace_expression_changed,
										user_data);
		gtk_combo_box_set_active (combo, oldindex);
		g_signal_handlers_unblock_by_func (G_OBJECT (combo),
										(gpointer) on_replace_expression_changed,
										user_data);
	}
}
*/
void
on_search_button_close_clicked (GtkButton *button, gpointer user_data)
{
	SearchReplaceGUI *sg;

	sg = anj_sr_get_current_uidata (GTK_WIDGET (button));
	if (sg != NULL)
		anj_sr_interrupt_nicely (sg);
}

void
on_search_button_stop_clicked (GtkButton *button, gpointer user_data)
{
	SearchReplaceGUI *sg;

	sg = anj_sr_get_current_uidata (GTK_WIDGET (button));
	if (sg != NULL)
	{
		SearchReplace *sr;

		sr = sg->sr;

		if (sr->search.action == SA_REPLACE && sr->replace.phase == SA_REPL_CONFIRM)
		{
			/* stop the current interactive replace */
			gtk_widget_set_sensitive (GTK_WIDGET (button), FALSE);
			anj_sr_show_replace_button (sg, FALSE);
			anj_sr_enable_replace_button (sg, FALSE);
			sr->replace.phase = SA_REPL_FIRST;
			sr->search.busy = FALSE;
			if (sr->search.expr.regex)
			{
				pcre_info_free (sr->search.expr.re);
				sr->search.expr.re = NULL;
			}
			clear_search_entries (&(sr->search.candidates));
			anj_sr_set_action (sg, SA_SELECT);
		}
		else
			sr->search.stop_count++;
		}
}

/* callback for "search|replace-all" button in s/r dialog */
/* this initiates a s/r operation or skips to the next location when
   denying a replacement */
void
on_search_button_start_clicked (GtkButton *button, gpointer user_data)
{
	SearchReplaceGUI *sg;
	SearchReplace *sr;

	sg = anj_sr_get_current_uidata (GTK_WIDGET (button));
	if (anj_sr_is_idle (sg))
	{
		anj_sr_populate_data (sg);
		sr = sg->sr;
		if (sr->search.action == SA_REPLACE)
			sr->replace.phase = SA_REPL_FIRST;
		sr->search.matches_sofar = 0;
		anj_sr_execute (sr, TRUE);
	}
	else
	{
		sr = sg->sr;
		if (sr->search.action == SA_REPLACE
			&& sr->replace.phase == SA_REPL_CONFIRM)
		{
			/* want to skip a replacement */
			sr->replace.phase = SA_REPL_SKIP;
			anj_sr_execute (sr, TRUE);
		}
	}
}

/* callback for "replace" button when confirming a replacement in s/r dialog */
void
on_search_button_replace_clicked (GtkButton *button, gpointer user_data)
{
	SearchReplaceGUI *sg;

	sg = anj_sr_get_current_uidata (GTK_WIDGET (button));
	if (!anj_sr_is_idle (sg))	/* this action valid only when paused for confirmation == busy */
	{
		anj_sr_populate_data (sg);
		anj_sr_execute (sg->sr, TRUE);
	/*	if (sr->search.action != SA_REPLACE || sr->replace.phase != SA_REPL_CONFIRM)
		{
			anj_sr_reset_flags (sr);	/ * CHECKME selection flag ? just sr->replace.phase = SA_REPL_FIRST; ?* /
			anj_sr_reset_replace_buttons (sr);
		}
	*/
	}
}

void
on_search_expression_activate (GtkEditable *edit, gpointer user_data)
{
	SearchReplaceGUI *sg;

	sg = anj_sr_get_current_uidata (GTK_WIDGET (edit));
	if (anj_sr_is_idle (sg))
	{
//		GtkWidget *combo;
//		SearchReplace *sr;

		anj_sr_populate_data (sg);
//		sr = sg->sr;
		anj_sr_execute (sg->sr, TRUE);
//		combo = GTK_WIDGET(edit)->parent;
/*		if (sr->search.action != SA_REPLACE || sr->replace.phase != SA_REPL_CONFIRM)
		{
			/ * this is not a first-pass replacement * /
			anj_sr_reset_flags (sr);	/ * CHECKME selection flag ? just sr->replace.phase = SA_REPL_FIRST; ?* /
			anj_sr_reset_replace_buttons (sr);
		}
*/
	}
}
/*
void
on_search_full_buffer_toggled (GtkToggleButton *togglebutton, gpointer user_data)
{
	SearchReplaceGUI *sg;
	gboolean state;

	state = gtk_toggle_button_get_active (togglebutton);
	sg = anj_sr_get_current_uidata (GTK_WIDGET (togglebutton));
	if (anj_sr_is_idle (sg))
	{
		if (state)
			anj_sr_set_direction (SD_WHOLE);
	}
	else
	{
		g_signal_handlers_block_by_func (G_OBJECT (togglebutton),
										(gpointer) ,
										user_data);
		gtk_toggle_button_set_active (togglebutton, !state);
		g_signal_handlers_unblock_by_func (G_OBJECT (togglebutton),
										(gpointer) ,
										user_data);
	}
}
*/
/*
void
on_search_forward_toggled (GtkToggleButton *togglebutton,
							gpointer user_data)
{
	SearchReplaceGUI *sg;
	gboolean state;

	state = gtk_toggle_button_get_active (togglebutton);
	sg = anj_sr_get_current_uidata (GTK_WIDGET (togglebutton));
	if (anj_sr_is_idle (sg))
	{
		if (state)
			anj_sr_set_direction (SD_FORWARD);
	}
	else
	{
		g_signal_handlers_block_by_func (G_OBJECT (togglebutton),
										(gpointer) ,
										user_data);
		gtk_toggle_button_set_active (togglebutton, !state);
		g_signal_handlers_unblock_by_func (G_OBJECT (togglebutton),
										(gpointer) ,
										user_data);
	}
}

void
on_search_backward_toggled (GtkToggleButton *togglebutton,
							gpointer user_data)
{
	SearchReplaceGUI *sg;
	gboolean state;

	state = gtk_toggle_button_get_active (togglebutton);
	sg = anj_sr_get_current_uidata (GTK_WIDGET (togglebutton));
	if (anj_sr_is_idle (sg))
	{
		if (state)
			anj_sr_set_direction (SD_BACKWARD);
	}
	else
	{
		g_signal_handlers_block_by_func (G_OBJECT (togglebutton),
										(gpointer) ,
										user_data);
		gtk_toggle_button_set_active (togglebutton, !state);
		g_signal_handlers_unblock_by_func (G_OBJECT (togglebutton),
										(gpointer) ,
										user_data);
	}
*/
/* diabled basic-search option
void
on_setting_basic_search_toggled (GtkToggleButton *togglebutton,
								 gpointer user_data)
{
	SearchReplaceGUI *sg;
	gboolean state;

	state = gtk_toggle_button_get_active (togglebutton);
	sg = anj_sr_get_current_uidata (GTK_WIDGET (togglebutton));
	if (anj_sr_is_idle (sg))
	{
		GtkWidget *frame;

		frame = anj_sr_get_ui_widget (SEARCH_SCOPE_FRAME); / * CHECKME with new dialog layout * /
		if (state)
		{
			SearchAction act;

			gtk_widget_show (frame);
			anj_sr_set_target (SR_BUFFER);
			anj_sr_set_direction (SD_FORWARD);

			act = anj_sr_get_combo_id_active_value (SEARCH_ACTION_COMBO);
			if (act == SA_REPLACE || act == SA_REPLACEALL)
				anj_sr_set_action (SA_REPLACE);
			else
				anj_sr_set_action (SA_SELECT);
		}
		else
			gtk_widget_hide (frame);
	}
	else
	{
		g_signal_handlers_block_by_func (G_OBJECT (togglebutton),
										(gpointer) on_setting_basic_search_toggled,
										user_data);
		gtk_toggle_button_set_active (togglebutton, !state);
		g_signal_handlers_unblock_by_func (G_OBJECT (togglebutton),
										(gpointer) on_setting_basic_search_toggled,
										user_data);
	}
}
*/
