/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */

/*
** search-replace.c: Generic Search and Replace
** Author: Biswapesh Chattopadhyay
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

#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-message-manager.h>
#include <libanjuta/interfaces/ianjuta-message-view.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>
#include <libanjuta/interfaces/ianjuta-markable.h>
#include <libanjuta/interfaces/ianjuta-bookmark.h>
#include <libanjuta/interfaces/ianjuta-indicable.h>

#include <libegg/menu/egg-entry-action.h>

#include "search-replace_backend.h"
#include "search-replace.h"
#include "search_preferences.h"

#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include <glib/gi18n.h>

#define GLADE_FILE_SEARCH_REPLACE PACKAGE_DATA_DIR"/glade/anjuta-search.glade"

/* LibGlade's auto-signal-connect will connect to these signals.
 * Do not declare them static.
 */
gboolean
on_search_dialog_key_press_event(GtkWidget *widget, GdkEventKey *event,
                               gpointer user_data);
void on_search_match_whole_word_toggled (GtkToggleButton *togglebutton, 
					 gpointer user_data);
void on_search_match_whole_line_toggled (GtkToggleButton *togglebutton,
					 gpointer user_data);
void on_search_match_word_start_toggled (GtkToggleButton *togglebutton,
					 gpointer user_data);
gboolean on_search_replace_delete_event(GtkWidget *window, GdkEvent *event,
					gboolean user_data);
void on_replace_regex_toggled (GtkToggleButton *togglebutton, gpointer user_data);
void on_search_regex_toggled (GtkToggleButton *togglebutton, gpointer user_data);
void on_search_action_changed (GtkComboBox *combo, gpointer user_data);
void on_search_target_changed(GtkComboBox *combo, gpointer user_data);
void on_search_expression_changed(GtkComboBox *combo, gpointer user_data);
void on_actions_no_limit_clicked(GtkButton *button, gpointer user_data);
void on_search_button_close_clicked(GtkButton *button, gpointer user_data);
void on_search_button_close_clicked(GtkButton *button, gpointer user_data);
void on_search_button_help_clicked(GtkButton *button, gpointer user_data);
void on_search_button_next_clicked(GtkButton *button, gpointer user_data);
void on_search_button_jump_clicked(GtkButton *button, gpointer user_data);
void on_search_expression_activate (GtkEditable *edit, gpointer user_data);
void on_search_button_save_clicked(GtkButton *button, gpointer user_data);
void on_search_button_stop_clicked(GtkButton *button, gpointer user_data);

void on_search_direction_changed (GtkComboBox *combo, gpointer user_data);
void on_search_full_buffer_toggled (GtkToggleButton *togglebutton, 
									gpointer user_data);
void on_search_forward_toggled (GtkToggleButton *togglebutton, 
									gpointer user_data);
void on_search_backward_toggled (GtkToggleButton *togglebutton, 
									gpointer user_data);
void on_setting_basic_search_toggled (GtkToggleButton *togglebutton, 
									  gpointer user_data);

/* GUI dropdown option strings */
AnjutaUtilStringMap search_direction_strings[] = {
/* the order of these matters - it must match the order of the corresponding
   radio buttons on another page */
	{SD_BEGINNING, N_("Full Buffer")},
	{SD_FORWARD, N_("Forward")},
	{SD_BACKWARD, N_("Backward")},
	{-1, NULL}
};

AnjutaUtilStringMap search_target_strings[] = {
	{SR_BUFFER, N_("Current Buffer")},
	{SR_SELECTION, N_("Current Selection")},
	{SR_BLOCK, N_("Current Block")},
	{SR_FUNCTION, N_("Current Function")},
	{SR_OPEN_BUFFERS, N_("All Open Buffers")},
	{SR_PROJECT, N_("All Project Files")},
/*	{SR_VARIABLE, N_("Specify File List")},*/
	{SR_FILES, N_("Specify File Patterns")},
	{-1, NULL}
};

AnjutaUtilStringMap search_action_strings[] = {
	{SA_SELECT, N_("Select next match")},
	{SA_BOOKMARK, N_("Bookmark all matched lines")},
	{SA_HIGHLIGHT, N_("Mark all matches")},
	{SA_FIND_PANE, N_("List matches in find pane")},
	{SA_REPLACE, N_("Replace next match")},
	{SA_REPLACEALL, N_("Replace all matches")},
	{-1, NULL}
};


typedef struct _SearchReplaceGUI
{
	GladeXML *xml;
	GtkWidget *dialog;
	gboolean showing;
} SearchReplaceGUI;


static GladeWidget glade_widgets[] = {
	/* CLOSE_BUTTON */
	{GE_BUTTON, "button.close", NULL, NULL},
	/* STOP_BUTTON */
	{GE_BUTTON, "button.stop", NULL, NULL},
	/* SEARCH_BUTTON */
	{GE_BUTTON, "button.next", NULL, NULL},
	/* JUMP_BUTTON */
	{GE_BUTTON, "button.jump", NULL, NULL},
	/* SEARCH_NOTEBOOK */
	{GE_NONE, "search.notebook", NULL, NULL},
	/* SEARCH_EXPR_FRAME */
	{GE_NONE, "frame.search.expression", NULL, NULL},
	/* SEARCH_TARGET_FRAME */
	{GE_NONE, "frame.search.target", NULL, NULL},
	/* SEARCH_VAR_FRAME */
	{GE_NONE, "frame.search.var", NULL, NULL},
	/* FILE_FILTER_FRAME */
	{GE_NONE, "frame.file.filter", NULL, NULL},
	/* FRAME_SEARCH_BASIC */
	{GE_NONE, "frame.search.basic", NULL, NULL},
	/* LABEL_REPLACE */
	{GE_NONE, "label.replace", NULL, NULL},
	/* SEARCH_STRING */
	{GE_COMBO_ENTRY, "search.string.combo", NULL, NULL},
	/* SEARCH_VAR */
	{GE_COMBO_ENTRY, "search.var.combo", NULL, NULL},
	/* MATCH_FILES */
	{GE_COMBO_ENTRY, "file.filter.match.combo", NULL, NULL},
	/* UNMATCH_FILES */
	{GE_COMBO_ENTRY, "file.filter.unmatch.combo", NULL, NULL},
	/* MATCH_DIRS */
	{GE_COMBO_ENTRY, "dir.filter.match.combo", NULL, NULL},
	/* UNMATCH_DIRS */
	{GE_COMBO_ENTRY, "dir.filter.unmatch.combo", NULL, NULL},
	/* REPLACE_STRING */
	{GE_COMBO_ENTRY, "replace.string.combo", NULL, NULL},
	/* ACTIONS_MAX */
	{GE_TEXT, "actions.max", NULL, NULL},
	/* SETTING_PREF_ENTRY */
	{GE_TEXT, "setting.pref.entry", NULL, NULL},
	/* SEARCH_REGEX */
	{GE_BOOLEAN, "search.regex", NULL, NULL},
	/* GREEDY */
	{GE_BOOLEAN, "search.greedy", NULL, NULL},
       	/* IGNORE_CASE */
	{GE_BOOLEAN, "search.ignore.case", NULL, NULL},
	/* WHOLE_WORD */
	{GE_BOOLEAN, "search.match.whole.word", NULL, NULL},
	/* WORD_START */
	{GE_BOOLEAN, "search.match.word.start", NULL, NULL},
	/* WHOLE_LINE */
	{GE_BOOLEAN, "search.match.whole.line", NULL, NULL},
	/* IGNORE_HIDDEN_FILES */
	{GE_BOOLEAN, "ignore.hidden.files", NULL, NULL},
	/* IGNORE_BINARY_FILES */
	{GE_BOOLEAN, "ignore.binary.files", NULL, NULL},
	/* IGNORE_HIDDEN_DIRS */
	{GE_BOOLEAN, "ignore.hidden.dirs", NULL, NULL},
	/* SEARCH_RECURSIVE */
	{GE_BOOLEAN, "search.dir.recursive", NULL, NULL},
	/* REPLACE_REGEX */
	{GE_BOOLEAN, "replace.regex", NULL, NULL},
	/* ACTIONS_NO_LIMIT */
	{GE_BOOLEAN, "actions.no_limit", NULL, NULL},
	/* SEARCH_FULL_BUFFER */
	{GE_BOOLEAN, "search.full_buffer", NULL, NULL},
	/* SEARCH_FORWARD */
	{GE_BOOLEAN, "search.forward", NULL, NULL},
	/* SEARCH_BACKWARD */
	{GE_BOOLEAN, "search.backward", NULL, NULL},
	/* SEARCH_BASIC */
	{GE_BOOLEAN, "search.basic", NULL, NULL},
	/* SEARCH_STRING_COMBO */
	{GE_COMBO, "search.string.combo", NULL, NULL},
	/* SEARCH_TARGET_COMBO */
	{GE_COMBO, "search.target.combo", search_target_strings, NULL},
	/* SEARCH_ACTION_COMBO */
	{GE_COMBO, "search.action.combo", search_action_strings, NULL},
	/* SEARCH_VAR_COMBO */
	{GE_COMBO, "search.var.combo", NULL, NULL},
	/* MATCH_FILES_COMBO */
	{GE_COMBO, "file.filter.match.combo", NULL, NULL},
	/* UNMATCH_FILES_COMBO */
	{GE_COMBO, "file.filter.unmatch.combo", NULL, NULL},
	/* MATCH_DIRS_COMBO */
	{GE_COMBO, "dir.filter.match.combo", NULL, NULL},
	/* UNMATCH_DIRS_COMBO */
	{GE_COMBO, "dir.filter.unmatch.combo", NULL, NULL},
	/* REPLACE_STRING_COMBO */
	{GE_COMBO, "replace.string.combo", NULL, NULL},
	/* SEARCH_DIRECTION_COMBO */
	{GE_COMBO, "search.direction.combo", search_direction_strings, NULL},
	/* SETTING_PREF_TREEVIEW */
	{GE_NONE, "setting.pref.treeview", NULL, NULL},
	{GE_NONE, NULL, NULL, NULL}
};

/***********************************************************/

static void
write_message_pane(IAnjutaMessageView* view, FileBuffer *fb, SearchEntry *se, MatchInfo *mi);
static gboolean on_message_clicked (GObject* object, gchar* message, gpointer data);
static void on_message_buffer_flush (IAnjutaMessageView *view, const gchar *one_line, gpointer data);
static void save_not_opened_files(FileBuffer *fb);
static gboolean replace_in_not_opened_files(FileBuffer *fb, MatchInfo *mi, gchar *repl_str);
static void search_set_action(SearchAction action);
static void search_set_target(SearchRangeType target);
static void search_set_direction(SearchDirection dir);
static void populate_value(GladeWidgetId id, gpointer val_ptr);
static void reset_flags(void);
static void reset_flags_and_search_button (void);
static void search_start_over (SearchDirection direction);
static void search_end_alert (gchar *string);
static void max_results_alert (void);
static void nb_results_alert (gint nb);
static void search_show_replace(gboolean hide);
static void modify_label_image_button(GladeWidgetId button_name, gchar *name, char *stock_image);
static void show_jump_button (gboolean show);
static gboolean create_dialog(void);
static void show_dialog(void);
static gboolean word_in_list(GList *list, gchar *word);
static GList* list_max_items(GList *list, guint nb_max);
static void search_update_combos (void);
static void replace_update_combos (void);
static void search_direction_changed(SearchDirection dir);
static void search_set_direction(SearchDirection dir);
static void search_set_toggle_direction(SearchDirection dir);
static void search_disconnect_set_toggle_connect(GladeWidgetId id, 
	GCallback function, gboolean active);
static void search_replace_next_previous(SearchDirection dir);
static void basic_search_toggled(void);

static SearchReplaceGUI *sg = NULL;

static SearchReplace *sr = NULL;

static gboolean flag_select = FALSE;
static gboolean interactive = FALSE;
static gboolean end_activity = FALSE;
static gboolean labels_translated = FALSE;

/***********************************************************/

void
search_and_replace_init (IAnjutaDocumentManager *dm)
{
	sr = create_search_replace_instance (dm);
}

void
search_and_replace (void)
{
	GList *entries;
	GList *tmp;
	SearchEntry *se;
	FileBuffer *fb;
	static MatchInfo *mi;
	Search *s;
	gint offset;
	gint found_line = 0;
	static gint os = 0;
	gint nb_results;
	static long start_sel = 0;
	static long end_sel = 0;
	static gchar *ch = NULL;
	gchar *regx_pattern;
	gboolean save_file = FALSE;
	IAnjutaMessageManager* msgman;
	IAnjutaMessageView* view = NULL;
	gboolean backward;
					
	g_return_if_fail(sr);
	s = &(sr->search);
	
	if (s->expr.search_str == NULL)
		return;
	
	entries = create_search_entries(s);
	if (!entries)
		return;
	
	end_activity = FALSE;
	backward = (s->range.direction == SD_BACKWARD);
	
	search_update_combos (); 
	if (s->action == SA_REPLACE || s->action == SA_REPLACEALL)
		replace_update_combos ();

	if (SA_FIND_PANE == s->action)
	{
		gchar* name = g_strconcat(_("Find: "), s->expr.search_str, NULL);		
		AnjutaShell* shell;
		g_object_get(G_OBJECT(sr->docman), "shell", &shell, NULL);
		msgman = anjuta_shell_get_interface(shell, 
			IAnjutaMessageManager, NULL);
		g_return_if_fail(msgman != NULL);
		
		view = ianjuta_message_manager_get_view_by_name(msgman, name, NULL);
		if (view == NULL)	
		{
			// FIXME: Put a nice icon here:
			view = ianjuta_message_manager_add_view(msgman, name,
													GTK_STOCK_FIND_AND_REPLACE, NULL);	
			g_return_if_fail(view != NULL);
			g_signal_connect (G_OBJECT(view), "buffer_flushed",
			                  G_CALLBACK (on_message_buffer_flush), NULL);
			g_signal_connect (G_OBJECT(view), "message_clicked",
			                  G_CALLBACK (on_message_clicked), NULL);
		}
		else
			ianjuta_message_view_clear(view, NULL);
		ianjuta_message_manager_set_current_view(msgman, view, NULL);
	}
	gtk_widget_set_sensitive (sr_get_gladewidget(STOP_BUTTON)->widget, TRUE);
	nb_results = 0;
	for (tmp = entries; tmp && (nb_results <= s->expr.actions_max); 
		 tmp = g_list_next(tmp))
	{
		if (end_activity)
			break;
		while(gtk_events_pending())
			gtk_main_iteration();
		
		/*to eliminate un-needed moves, when not bookmarking, this could be
		  current line i.e. ianjuta_editor_get_lineno (IANJUTA_EDITOR (se->te), NULL);
		  or sometimes last-line ? */
		found_line = (s->action == SA_BOOKMARK) ? -1 : 1;

		se = (SearchEntry *) tmp->data;
		if (flag_select)
		{
			se->start_pos = start_sel;
			se->end_pos = end_sel;
		}
		else
			end_sel = se->end_pos;
		if (SE_BUFFER == se->type)
			fb = file_buffer_new_from_te(se->te);
		else /* if (SE_FILE == se->type) */
			fb = file_buffer_new_from_path(se->path, NULL, -1, 0);

		if (fb)
		{		
			fb->pos = se->start_pos;
			offset = 0;
/* NO - there's no reason for user to expect existing marks to be removed.
	And that can easily be done manually by user if so desired.
			if (s->action == SA_BOOKMARK && IANJUTA_IS_MARKABLE (fb->te))
				ianjuta_markable_delete_all_markers(IANJUTA_MARKABLE(fb->te), 
				                                    IANJUTA_MARKABLE_LINEMARKER, NULL);
*/
		//FIXME enable clearing of marks by some means other than a 0-match search
			if (s->action == SA_HIGHLIGHT)	
				ianjuta_indicable_clear (IANJUTA_INDICABLE(fb->te), NULL); 				
	
			while (interactive || 
				NULL != (mi = get_next_match(fb, s->range.direction, &(s->expr))))
			{
				if ((s->range.direction == SD_BACKWARD) && (mi->pos < se->end_pos))
						break; 
				if ((s->range.direction != SD_BACKWARD) && ((se->end_pos != -1) &&
					(mi->pos+mi->len > se->end_pos)))
						break; 
				nb_results++; 
				if (nb_results > sr->search.expr.actions_max)
					break;
				
				/* NOTE - mi->line is "editor-style" 1-based, but some things
					here use/expect 0-base, so adjustments are made as needed */
				
				switch (s->action)
				{
					case SA_HIGHLIGHT:
						found_line = mi->line;

						if (fb->te == NULL)
							fb->te =
								IANJUTA_EDITOR (ianjuta_document_manager_get_current_document
									(sr->docman, NULL));

						if (IANJUTA_INDICABLE (fb->te))
					/* end-location is correct for sourceview, 1-too-big for scintilla */
						ianjuta_indicable_set (IANJUTA_INDICABLE(fb->te),  
						                       mi->pos, mi->pos + mi->len,  
					            					IANJUTA_INDICABLE_IMPORTANT, NULL);
						break;

					case SA_BOOKMARK:
						if (found_line != mi->line)
						{
							found_line = mi->line;
						
							if (fb->te == NULL)
								fb->te =
									IANJUTA_EDITOR (ianjuta_document_manager_get_current_document
										(sr->docman, NULL));

							if (IANJUTA_IS_MARKABLE (fb->te) &&
								!ianjuta_markable_is_marker_set (
														IANJUTA_MARKABLE(fb->te),
														mi->line,
														IANJUTA_MARKABLE_BOOKMARK,
														NULL))
							{
								ianjuta_bookmark_toggle (IANJUTA_BOOKMARK(fb->te),
														mi->line, FALSE, NULL);
							}
						}
						break;
						
					case SA_SELECT:
						if (found_line != mi->line || fb->te == NULL)
						{
							if (fb->te)
								ianjuta_editor_goto_line (fb->te, mi->line, NULL);
							else
								fb->te = ianjuta_document_manager_goto_file_line_mark
									(sr->docman, fb->path, mi->line, FALSE, NULL);
							found_line = mi->line;
						}
						{
							IAnjutaIterable* start = ianjuta_editor_get_cell_iter (fb->te, mi->pos, NULL);
							IAnjutaIterable* end = ianjuta_editor_get_cell_iter (fb->te, mi->pos + mi->len, NULL);
							ianjuta_editor_selection_set(IANJUTA_EDITOR_SELECTION (fb->te), 
														 start,
														 end,
														 NULL);
							g_object_unref (start);
							g_object_unref (end);
							break;
						}
						
					case SA_FIND_PANE: 
						write_message_pane(view, fb, se, mi);
						break;
					
					case SA_REPLACE:
						if (found_line != mi->line || fb->te == NULL)
							{
							if (fb->te)
								ianjuta_editor_goto_line (fb->te, mi->line, NULL);
							else
								fb->te = ianjuta_document_manager_goto_file_line_mark
									(sr->docman, fb->path, mi->line, FALSE, NULL);
							found_line = mi->line;
							}

						if (!interactive)
						{
							IAnjutaIterable* start = ianjuta_editor_get_cell_iter (fb->te, mi->pos - offset, NULL);
							IAnjutaIterable* end = ianjuta_editor_get_cell_iter (fb->te, mi->pos - offset + mi->len, NULL);
							ianjuta_editor_selection_set(IANJUTA_EDITOR_SELECTION (fb->te), 
														 start,
														 end,
														 NULL);
							g_object_unref (start);
							g_object_unref (end);
							interactive = TRUE;
							os = offset;
							modify_label_image_button(SEARCH_BUTTON, _("Replace"), 
								                      GTK_STOCK_FIND_AND_REPLACE);
							show_jump_button(TRUE);
							if (sr->replace.regex && sr->search.expr.regex)
							{
									g_free (ch);
								ch = regex_backref (mi, fb);
								}
							}
						else
						{
						if (ch && sr->replace.regex && sr->search.expr.regex)
						{
								g_free (sr->replace.repl_str);
								sr->replace.repl_str = g_strdup (ch);
							g_free (ch);
								ch = NULL;
						}
							{
								IAnjutaIterable* start = ianjuta_editor_get_cell_iter (fb->te, mi->pos - os, NULL);
								IAnjutaIterable* end = ianjuta_editor_get_cell_iter (fb->te, mi->pos + mi->len - os, NULL);
								ianjuta_editor_selection_set(IANJUTA_EDITOR_SELECTION (fb->te),  
															 start,														 
															 end,
															 NULL);
								ianjuta_editor_selection_replace(IANJUTA_EDITOR_SELECTION (fb->te), 
																 sr->replace.repl_str,
																 strlen(sr->replace.repl_str),
																 NULL);
								g_object_unref (start);
								g_object_unref (end);
							}
						if (se->direction != SD_BACKWARD)
							offset += mi->len - (sr->replace.repl_str?strlen(sr->replace.repl_str):0);
						
						interactive = FALSE;
						}
						break;

					case SA_REPLACEALL:
						if (sr->replace.regex && sr->search.expr.regex)
						{
							regx_pattern = sr->replace.repl_str;	/* preserve for later matches */
							sr->replace.repl_str = regex_backref (mi, fb);
						}
						else
							regx_pattern = NULL;
						if (fb->te == NULL) /* NON OPENED FILES */
						{
							if (replace_in_not_opened_files(fb, mi, sr->replace.repl_str))
								save_file = TRUE;
						}
						else
						{
							IAnjutaIterable* start = ianjuta_editor_get_cell_iter (fb->te, mi->pos - offset, NULL);
							IAnjutaIterable* end = ianjuta_editor_get_cell_iter (fb->te, mi->pos + mi->len - offset, NULL);
							ianjuta_editor_selection_set(IANJUTA_EDITOR_SELECTION (fb->te),  
														 start,
														 end,
														 NULL);
							ianjuta_editor_selection_replace(IANJUTA_EDITOR_SELECTION (fb->te), 
															 sr->replace.repl_str,
															 strlen(sr->replace.repl_str),
															 NULL);
							g_object_unref (start);
							g_object_unref (end);
						}
						if (se->direction != SD_BACKWARD)
 							offset += mi->len - (sr->replace.repl_str?strlen(sr->replace.repl_str):0);
						if (regx_pattern)
						{
							g_free (sr->replace.repl_str);
							sr->replace.repl_str = regx_pattern;
						}
						break;

					default:
						g_warning ("Not implemented - File %s - Line %d\n", __FILE__, __LINE__);
						break;
				}  // switch

				if (se->direction != SD_BACKWARD)
					start_sel = mi->pos + mi->len - offset; 
				else
					start_sel = mi->pos - offset; 
				
				if (SA_REPLACE != s->action || !interactive)
					match_info_free(mi);
				
				if (SA_SELECT == s->action ||
					((SA_REPLACE == s->action || SA_REPLACEALL == s->action) && interactive))
					break;
			} // while
/* NO - leave the current position unchanged when marking-all
			if (nb_results > 0)
			{
				switch (s->action)
				{
					case SA_HIGHLIGHT:
					case SA_BOOKMARK:
						ianjuta_editor_goto_line (fb->te, found_line, NULL);
					default:
						break;
				}
			}
*/
		if (save_file)
		{
				save_not_opened_files (fb);
			save_file = FALSE;
		}
		
			file_buffer_free (fb);
		}  // if (fb)

		g_free (se->path);
		g_free (se);

		if (SA_SELECT == s->action && nb_results > 0)
			break;
	}

	gtk_widget_set_sensitive (sr_get_gladewidget(STOP_BUTTON)->widget, FALSE);
	
	if (s->range.type == SR_BLOCK || s->range.type == SR_FUNCTION || 
		s->range.type == SR_SELECTION)
			flag_select = TRUE;
	
	if (entries)
		g_list_free (entries);
	
	if (s->action == SA_FIND_PANE)
	{
		ianjuta_message_view_append (view, IANJUTA_MESSAGE_VIEW_TYPE_INFO,
									 _("Search complete"), "", NULL);
	}
	
	if (nb_results == 0)
	{
		search_end_alert(sr->search.expr.search_str);
	}
	else if (nb_results > sr->search.expr.actions_max)
		max_results_alert();
	else if (s->action == SA_REPLACEALL)
		nb_results_alert(nb_results);
	
	if ((s->range.direction == SD_BEGINNING) &&
		((s->action == SA_SELECT) || (s->action == SA_REPLACE)) )
	{
		search_set_direction(SD_FORWARD);
	}
}

static void
write_message_pane(IAnjutaMessageView* view, FileBuffer *fb, SearchEntry *se,
				   MatchInfo *mi)
{
	gchar *match_line;
	char buf[BUFSIZ];
	gchar *tmp;
	
	match_line = file_match_line_from_pos(fb, mi->pos);

	if (SE_BUFFER == se->type)
	{
		/* DEBUG_PRINT ("FBPATH  %s\n", fb->path); */
		const gchar* filename = ianjuta_document_get_filename(IANJUTA_DOCUMENT(se->te), NULL);
		tmp = g_strrstr(fb->path, "/");
		tmp = g_strndup(fb->path, tmp + 1 -(fb->path));
		snprintf(buf, BUFSIZ, "%s%s:%d:%s\n", tmp, filename,
		         mi->line, match_line);
		g_free(tmp);
	}
	else /* if (SE_FILE == se->type) */
	{
		snprintf(buf, BUFSIZ, "%s:%d:%s\n", se->path, mi->line + 1, match_line);
	}
	g_free(match_line);
	ianjuta_message_view_buffer_append (view, buf, NULL);
}

static void
on_message_buffer_flush (IAnjutaMessageView *view, const gchar *one_line,
						 gpointer data)
{
	ianjuta_message_view_append (view, IANJUTA_MESSAGE_VIEW_TYPE_NORMAL,
								 one_line, "", NULL);
}

static gboolean
on_message_clicked (GObject* object, gchar* message, gpointer data)
{
	gchar *ptr, *ptr2;
	gchar *path, *nline;
	gint line;
		
	if (!(ptr = g_strstr_len(message, strlen(message), ":")) )
		return FALSE;
	path = g_strndup(message, ptr - message);
	
	ptr++;
	if (!(ptr2 = g_strstr_len(ptr, strlen(ptr), ":")) )
		return FALSE;
	nline = g_strndup(ptr, ptr2 - ptr);
	line = atoi(nline);
							
	ianjuta_document_manager_goto_file_line_mark (sr->docman, path, line, TRUE, NULL);  
	g_free(path);
	g_free(nline);
	return FALSE;
}

static void
save_not_opened_files(FileBuffer *fb)
{
	FILE *fp;

	fp = fopen(fb->path, "wb");	
	if (!fp)
			return;
	fwrite(fb->buf, fb->len, 1, fp);
	fclose(fp);
}

static gboolean
replace_in_not_opened_files(FileBuffer *fb, MatchInfo *mi, gchar *repl_str)
{
	gint l;
	g_return_val_if_fail (repl_str != NULL, FALSE);
	
	if (strlen(repl_str) > mi->len)
	{
		l = fb->len - mi->pos;
		fb->len = fb->len + strlen(repl_str) - mi->len;
		if ( (fb->buf = g_realloc(fb->buf, fb->len)) == NULL )
			return FALSE;
		memmove((fb->buf) + mi->pos + strlen(repl_str) - mi->len, fb->buf + mi->pos,l);
	}
	if  (strlen(repl_str) < mi->len)
	{
		l = fb->len - mi->pos - mi->len ;
		memmove((fb->buf) + mi->pos + strlen(repl_str), fb->buf + mi->pos + mi->len,l);
		fb->len = fb->len + strlen(repl_str) - mi->len;
		if ( (fb->buf = g_realloc(fb->buf, fb->len)) == NULL)
			return FALSE;
	}

	for (l=0; l < strlen(repl_str); l++)
		(fb->buf)[(mi->pos)+l] = repl_str [l];
		
	return TRUE;
}


static void
search_replace_next_previous(SearchDirection dir)
{
	SearchDirection save_direction;
	SearchAction save_action;
	SearchRangeType save_type;
	
	if (sr)
	{
		save_action = sr->search.action;
		save_type = sr->search.range.type;
		save_direction = sr->search.range.direction;
		sr->search.range.direction = dir;	
		if (save_type == SR_OPEN_BUFFERS || save_type == SR_PROJECT ||
				save_type == SR_FILES)
			sr->search.range.direction = SR_BUFFER;
		sr->search.action = SA_SELECT;
		
		search_and_replace();
		
		sr->search.action = save_action;
		sr->search.range.type = save_type;
		sr->search.range.direction = save_direction;
	}	
	else
	{
		DEBUG_PRINT ("sr null\n");
	}
}

void
search_replace_next(void)
{
	search_replace_next_previous(SD_FORWARD);
}

void
search_replace_previous(void)
{
	search_replace_next_previous(SD_BACKWARD);
}

/****************************************************************/

GladeWidget *
sr_get_gladewidget(GladeWidgetId id)
{
	return &glade_widgets[id];
}

static void
search_set_popdown_strings (GtkComboBoxEntry *combo, GList* strings)
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
search_set_popdown_map (GtkComboBox *combo, AnjutaUtilStringMap *map)
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
search_select_item(GtkComboBox* combo, gint item)
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
			/* Find command */
			gtk_combo_box_set_active_iter(combo, &iter);
			break;
		}
	}

}

static void
search_set_combo(GladeWidgetId id_combo, gint command)
{
	GtkComboBox *combo;
	
	combo = GTK_COMBO_BOX(sr_get_gladewidget(id_combo)->widget);
	search_select_item (combo, command);	
}

static void
search_set_action(SearchAction action)
{
	search_set_combo(SEARCH_ACTION_COMBO, action); 
}

static void
search_set_target(SearchRangeType target)
{
	search_set_combo(SEARCH_TARGET_COMBO, target);
}

static void
search_set_direction(SearchDirection dir)
{
	search_set_combo(SEARCH_DIRECTION_COMBO, dir);
}

static gint
search_get_item_combo(GtkComboBox *combo)
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
search_get_item_combo_name(GladeWidgetId id)
{
	GtkWidget *combo = sr_get_gladewidget(id)->widget;
	return search_get_item_combo(GTK_COMBO_BOX(combo));
}

static void
search_direction_changed(SearchDirection dir)
{
	SearchEntryType tgt;
	SearchAction act;
	
	tgt = search_get_item_combo_name(SEARCH_TARGET_COMBO);
	if (dir != SD_BEGINNING)
	{
		if (tgt == SR_OPEN_BUFFERS || tgt == SR_PROJECT
				   || tgt == SR_FILES)
			search_set_target(SR_BUFFER);
	}
	else
	{
		if (tgt == SR_BUFFER ||tgt == SR_SELECTION || tgt == SR_BLOCK ||
				tgt == SR_FUNCTION)
			search_set_target(SR_BUFFER);
		else
		{
			act = search_get_item_combo_name(SEARCH_ACTION_COMBO);
			if (act == SA_SELECT)
				search_set_action(SA_BOOKMARK);
			if (act == SA_REPLACE)
				search_set_action(SA_REPLACEALL);				
		}
	}
}

static void
populate_value(GladeWidgetId id, gpointer val_ptr)
{
	GladeWidget *gw;

	g_return_if_fail(id && val_ptr);
	
	gw = sr_get_gladewidget(id);
	g_return_if_fail(gw);
	switch(gw->type)
	{
		case GE_COMBO_ENTRY:
		case GE_TEXT:
			if (*((char **) val_ptr))
				g_free(* ((char **) val_ptr));
			*((char **) val_ptr) = gtk_editable_get_chars(
			  GTK_EDITABLE(gw->widget), 0, -1);
			break;
		case GE_BOOLEAN:
			* ((gboolean *) val_ptr) = gtk_toggle_button_get_active(
			  GTK_TOGGLE_BUTTON(gw->widget));
			break;
		case GE_COMBO:
			g_return_if_fail (gw->extra != NULL);

			*((int *) val_ptr) = search_get_item_combo (GTK_COMBO_BOX(gw->widget));
			break;
		default:
			g_warning("Bad option %d to populate_value", gw->type);
			break;
	}
}

static void
reset_flags(void)
{
	flag_select = FALSE;
	interactive = FALSE;
}

static void
reset_flags_and_search_button(void)
{
	reset_flags();
	if (sr->search.action != SA_REPLACEALL)
		modify_label_image_button(SEARCH_BUTTON, _("Search"), GTK_STOCK_FIND);

	else
		modify_label_image_button(SEARCH_BUTTON, _("Replace All"), 
			                      GTK_STOCK_FIND_AND_REPLACE);

	show_jump_button(FALSE);
}

static void
search_start_over (SearchDirection direction)
{
	IAnjutaDocument* doc = ianjuta_document_manager_get_current_document(sr->docman,
																		NULL);
	IAnjutaEditor *te = NULL;
	if (IANJUTA_IS_EDITOR(doc))
		te = IANJUTA_EDITOR(doc);
	long length;
	
	if (te)
	{
		length = ianjuta_editor_get_length(te, NULL);;
	
		if (direction != SD_BACKWARD)
			/* search from doc start */
			ianjuta_editor_goto_position(te, 0, NULL);
		else
			/* search from doc end */
			ianjuta_editor_goto_position (te, length, NULL);
	}
}

static void 
search_end_alert(gchar *string)
{
	GtkWidget *dialog;
	
	if (sr->search.range.direction != SD_BEGINNING && !flag_select)
	{
		// Ask if user wants to wrap around the doc
		// Dialog to be made HIG compliant.
		dialog = gtk_message_dialog_new (GTK_WINDOW (sg->dialog),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_QUESTION,
					GTK_BUTTONS_YES_NO,
					_("The match \"%s\" was not found. Wrap search around the document?"),
					string);

		gtk_dialog_set_default_response (GTK_DIALOG (dialog),
										 GTK_RESPONSE_YES);
		g_signal_connect(G_OBJECT(dialog), "key-press-event",
			G_CALLBACK(on_search_dialog_key_press_event), NULL);
		if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
		{
			search_start_over (sr->search.range.direction);
			gtk_widget_destroy(dialog);
			reset_flags();
			search_and_replace ();
			return;
		}
	}
	else
	{
		dialog = gtk_message_dialog_new(GTK_WINDOW (sg->dialog),
					GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
					_("The match \"%s\" was not found."),
		        	string);
		g_signal_connect(G_OBJECT(dialog), "key-press-event",
			G_CALLBACK(on_search_dialog_key_press_event), NULL);
		gtk_dialog_run(GTK_DIALOG(dialog));
	}	
	gtk_widget_destroy(dialog);
	reset_flags();
}

static void 
max_results_alert(void)
{
	GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW (sg->dialog),
				GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
		        	_("The maximum number of results has been reached."));
	g_signal_connect(G_OBJECT(dialog), "key-press-event",
			G_CALLBACK(on_search_dialog_key_press_event), NULL);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	reset_flags();
}

static void 
nb_results_alert(gint nb)
{
	GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW (sg->dialog),
				GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
		        	_("%d matches have been replaced."), nb);
	g_signal_connect(G_OBJECT(dialog), "key-press-event",
			G_CALLBACK(on_search_dialog_key_press_event), NULL);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	reset_flags();
}

static void
search_show_replace(gboolean hide)
{
	static GladeWidgetId hide_widgets[] = {
		REPLACE_REGEX, REPLACE_STRING_COMBO, LABEL_REPLACE
	};
	int i;
	GtkWidget *widget;
	
	for (i=0; i < sizeof(hide_widgets)/sizeof(hide_widgets[0]); ++i)
	{
		widget = sr_get_gladewidget(hide_widgets[i])->widget;
		if (NULL != widget)
		{
			if (hide)
				gtk_widget_show(widget);
			else
				gtk_widget_hide(widget);
		}
	}
}

static void
modify_label_image_button(GladeWidgetId button_id, gchar *name, char *stock_image)
{
	GList *list, *l;
	GtkHBox *hbox;
	GtkWidget *alignment;
	GtkWidget *button = sr_get_gladewidget(button_id)->widget;
	
	list = gtk_container_get_children(GTK_CONTAINER (button));
	alignment = GTK_WIDGET(list->data);
	g_list_free(list);		
	list = gtk_container_get_children(GTK_CONTAINER (alignment));
	hbox = GTK_HBOX(list->data);
	g_list_free(list);
	list = gtk_container_get_children(GTK_CONTAINER (hbox));
	for (l=list; l; l = g_list_next(l))
	{
		if (GTK_IS_LABEL(l->data))
			gtk_label_set_text(GTK_LABEL(l->data), name);
		if (GTK_IS_IMAGE(l->data))
			gtk_image_set_from_stock(GTK_IMAGE(l->data), stock_image, 
									 GTK_ICON_SIZE_BUTTON);
	}
	g_list_free(list);
}


/********************************************************************/

#define POP_LIST(str, var) populate_value(str, &s);\
			if (s) \
			{\
				sr->search.range.files.var = anjuta_util_glist_from_string(s);\
			}
			
/********************************************************************/

void
search_replace_populate(void)
{
	char *s = NULL;
	char *max = NULL;
	
	/* Now, populate the instance with values from the GUI */
	populate_value(SEARCH_STRING, &(sr->search.expr.search_str));
	populate_value(SEARCH_REGEX, &(sr->search.expr.regex));
	populate_value(GREEDY, &(sr->search.expr.greedy));
	populate_value(IGNORE_CASE, &(sr->search.expr.ignore_case));
	populate_value(WHOLE_WORD, &(sr->search.expr.whole_word));
	populate_value(WHOLE_LINE, &(sr->search.expr.whole_line));
	populate_value(WORD_START, &(sr->search.expr.word_start));
	populate_value(SEARCH_TARGET_COMBO, &(sr->search.range.type));
	populate_value(SEARCH_DIRECTION_COMBO, &(sr->search.range.direction));
	populate_value(ACTIONS_NO_LIMIT, &(sr->search.expr.no_limit));

	populate_value(SEARCH_BASIC, &(sr->search.basic_search));
	
	if (sr->search.expr.no_limit)
		sr->search.expr.actions_max = G_MAXINT;	
	else
	{
		populate_value(ACTIONS_MAX, &(max));
		sr->search.expr.actions_max = atoi(max);
		if (sr->search.expr.actions_max <= 0)
			sr->search.expr.actions_max = 200;
		g_free(max);
	}

	switch (sr->search.range.type)
	{
		case SR_FUNCTION:
		case SR_BLOCK:
			if (flag_select)
				sr->search.range.type = SR_SELECTION;
			break;
		case SR_FILES:
			POP_LIST(MATCH_FILES, match_files);
			POP_LIST(UNMATCH_FILES, ignore_files);
			POP_LIST(MATCH_DIRS, match_dirs);
			POP_LIST(UNMATCH_DIRS, ignore_dirs);
		    populate_value(IGNORE_HIDDEN_FILES, &(sr->search.range.files.ignore_hidden_files));
		    populate_value(IGNORE_HIDDEN_DIRS, &(sr->search.range.files.ignore_hidden_dirs));
		    populate_value(SEARCH_RECURSIVE, &(sr->search.range.files.recurse));
			break;
		default:
			break;
	}
	populate_value(SEARCH_ACTION_COMBO, &(sr->search.action));
	switch (sr->search.action)
	{
		case SA_REPLACE:
		case SA_REPLACEALL:
			populate_value(REPLACE_STRING, &(sr->replace.repl_str));
			populate_value(REPLACE_REGEX, &(sr->replace.regex));
			break;
		default:
			break;
	}
}

static void
show_jump_button (gboolean show)
{
	GtkWidget *jump_button = sr_get_gladewidget(JUMP_BUTTON)->widget;
	if (show)
		gtk_widget_show(jump_button);
	else
		gtk_widget_hide(jump_button);
}

static
void translate_dialog_strings (AnjutaUtilStringMap labels[])
{
	guint i = 0;
	while (labels[i].name != NULL)
	{
		labels[i].name = gettext (labels[i].name);
		i++;
	}
}

static gboolean
create_dialog(void)
{
	GladeWidget *w;
	GtkWidget *widget;
	int i;

	g_return_val_if_fail(NULL != sr, FALSE);
	if (NULL != sg) return TRUE;
	sg = g_new0(SearchReplaceGUI, 1);

	if (NULL == (sg->xml = glade_xml_new(GLADE_FILE_SEARCH_REPLACE,
		SEARCH_REPLACE_DIALOG, NULL)))
	{
		anjuta_util_dialog_error(NULL, _("Unable to build user interface for Search And Replace"));
		g_free(sg);
		sg = NULL;
		return FALSE;
	}
	sg->dialog = glade_xml_get_widget(sg->xml, SEARCH_REPLACE_DIALOG);
	/* gtk_window_set_transient_for (GTK_WINDOW(sg->dialog)
	  , GTK_WINDOW(app->widgets.window)); */
		
	if (!labels_translated)
	{
		labels_translated = TRUE;
		translate_dialog_strings (search_direction_strings);
		translate_dialog_strings (search_target_strings);
		translate_dialog_strings (search_action_strings);
	}

	for (i=0; NULL != glade_widgets[i].name; ++i)
	{
		w = &(glade_widgets[i]);
		w->widget = glade_xml_get_widget(sg->xml, w->name);
		if (GE_COMBO_ENTRY == w->type)
		{
			/* Get child of GtkComboBoxEntry */
			w->widget = GTK_BIN(w->widget)->child;
		}
		gtk_widget_ref(w->widget);
		if (GE_COMBO == w->type && NULL != w->extra)
		{
			search_set_popdown_map(GTK_COMBO_BOX(w->widget), (AnjutaUtilStringMap *)w->extra);
		}
	}

	widget = sr_get_gladewidget(SEARCH_STRING_COMBO)->widget;
	g_signal_connect (widget, "changed", G_CALLBACK (on_search_expression_changed), NULL);
	widget = sr_get_gladewidget(SEARCH_STRING)->widget;
	g_signal_connect (widget, "activate", G_CALLBACK (on_search_expression_activate), NULL);
	widget = sr_get_gladewidget(REPLACE_STRING)->widget;
	g_signal_connect (widget, "activate", G_CALLBACK (on_search_expression_activate), NULL);
	widget = sr_get_gladewidget(SEARCH_ACTION_COMBO)->widget;
	g_signal_connect (widget, "changed", G_CALLBACK (on_search_action_changed), NULL);
	widget = sr_get_gladewidget(SEARCH_DIRECTION_COMBO)->widget;
	g_signal_connect (widget, "changed", G_CALLBACK (on_search_direction_changed), NULL);
	widget = sr_get_gladewidget(SEARCH_TARGET_COMBO)->widget;
	g_signal_connect (widget, "changed", G_CALLBACK (on_search_target_changed), NULL);


	search_preferences_initialize_setting_treeview(sg->dialog);
	search_preferences_init();
	
	glade_xml_signal_autoconnect(sg->xml);
	return TRUE;
}

static void
show_dialog(void)
{
	gtk_window_present (GTK_WINDOW (sg->dialog));
	sg->showing = TRUE;
}

static gboolean
word_in_list(GList *list, gchar *word)
{
	GList *l = list;

	while (l != NULL)
	{
		if (strcmp(l->data, word) == 0)
			return TRUE;
		l = g_list_next(l);
	}
	return FALSE;
}

/*  Remove last item of the list if > nb_max  */

static GList*
list_max_items(GList *list, guint nb_max)
{
	GList *last;

	if (g_list_length(list) > nb_max)
	{
		last = g_list_last(list);
		g_free(last->data);
		list = g_list_delete_link (list, last);
	}
	return list;
}

#define MAX_ITEMS_SEARCH_COMBO 16

void
search_toolbar_set_text(gchar *search_text)
{
	AnjutaUI *ui; 
	GtkAction *action; 
	
	AnjutaShell* shell;
	g_object_get(G_OBJECT(sr->docman), "shell", &shell, NULL);
	
	ui = anjuta_shell_get_ui (shell, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupNavigation",
								   "ActionEditSearchEntry");
	egg_entry_action_set_text (EGG_ENTRY_ACTION(action), search_text);	
}

static void
search_update_combos(void)
{
	GtkWidget *search_entry = NULL;
	gchar *search_word = NULL;
	IAnjutaDocument* doc = ianjuta_document_manager_get_current_document(sr->docman,
																		NULL);
	IAnjutaEditor *te = NULL;
	if (IANJUTA_IS_EDITOR(doc))
		te = IANJUTA_EDITOR(doc);

	search_entry = sr_get_gladewidget(SEARCH_STRING)->widget;
	if (search_entry && te)
	{
		search_word = g_strdup(gtk_entry_get_text(GTK_ENTRY (search_entry)));
		if (search_word  && strlen(search_word) > 0)
		{
			if (!word_in_list(sr->search.expr_history, search_word))
			{
				GtkWidget *search_list = 
					sr_get_gladewidget(SEARCH_STRING_COMBO)->widget;
				sr->search.expr_history = g_list_prepend(sr->search.expr_history,
					search_word);
				sr->search.expr_history = list_max_items(sr->search.expr_history,
					MAX_ITEMS_SEARCH_COMBO);
				search_set_popdown_strings(GTK_COMBO_BOX_ENTRY (search_list),
					sr->search.expr_history);
				
				//search_toolbar_set_text(search_word);	
				//  FIXME  comboentry instead of entry				
				//~ entry_set_text_n_select (app->widgets.toolbar.main_toolbar.find_entry,
								 //~ search_word, FALSE);
			}
		}
	}
}

static void
replace_update_combos(void)
{
	GtkWidget *replace_entry = NULL;
	gchar *replace_word = NULL;
	IAnjutaDocument* doc = ianjuta_document_manager_get_current_document(sr->docman,
																		NULL);
	IAnjutaEditor *te = NULL;
	if (IANJUTA_IS_EDITOR(doc))
		te = IANJUTA_EDITOR(doc);
	
	replace_entry = sr_get_gladewidget(REPLACE_STRING)->widget;
	if (replace_entry && te)
	{
		replace_word = g_strdup(gtk_entry_get_text(GTK_ENTRY (replace_entry)));
		if (replace_word  && strlen(replace_word) > 0)
		{
			if (!word_in_list(sr->replace.expr_history, replace_word))
			{
				GtkWidget *replace_list = 
					sr_get_gladewidget(REPLACE_STRING_COMBO)->widget;
				sr->replace.expr_history = g_list_prepend(sr->replace.expr_history,
					replace_word);
				sr->replace.expr_history = list_max_items(sr->replace.expr_history,
					MAX_ITEMS_SEARCH_COMBO);
				search_set_popdown_strings(GTK_COMBO_BOX_ENTRY (replace_list),
					sr->replace.expr_history);
			}
		}
	}
}

void 
search_update_dialog(void)
{
	GtkWidget *widget;
	Search *s;

	s = &(sr->search);
	widget = sr_get_gladewidget(SEARCH_REGEX)->widget;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), s->expr.regex);
	widget = sr_get_gladewidget(GREEDY)->widget;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), s->expr.greedy);
	widget = sr_get_gladewidget(IGNORE_CASE)->widget;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), s->expr.ignore_case);
	widget = sr_get_gladewidget(WHOLE_WORD)->widget;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), s->expr.whole_word);
	widget = sr_get_gladewidget(WHOLE_LINE)->widget;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), s->expr.whole_line);
	widget = sr_get_gladewidget(WORD_START)->widget;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), s->expr.word_start);
		
	widget = sr_get_gladewidget(ACTIONS_NO_LIMIT)->widget;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), s->expr.no_limit);
	widget = sr_get_gladewidget(ACTIONS_MAX)->widget;
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), s->expr.actions_max);
	
	widget = sr_get_gladewidget(REPLACE_REGEX)->widget;	
	gtk_widget_set_sensitive(widget, sr->search.expr.regex);
	
	widget = sr_get_gladewidget(SEARCH_BUTTON)->widget; 	
	gtk_widget_set_sensitive (widget, (s->expr.search_str != NULL) && (*s->expr.search_str != '\0'));

	widget = sr_get_gladewidget(SEARCH_STRING)->widget;
	if (s->expr.search_str)
		gtk_entry_set_text(GTK_ENTRY(widget), s->expr.search_str);
	
	widget = sr_get_gladewidget(SEARCH_DIRECTION_COMBO)->widget;
	search_select_item (GTK_COMBO_BOX(widget), s->range.direction);
	
	widget = sr_get_gladewidget(SEARCH_ACTION_COMBO)->widget;
	search_select_item (GTK_COMBO_BOX(widget), s->action);

	search_show_replace(s->action == SA_REPLACE || s->action == SA_REPLACEALL);
	
	widget = sr_get_gladewidget(SEARCH_TARGET_COMBO)->widget;
	search_select_item (GTK_COMBO_BOX(widget), s->range.type);
	
	widget = sr_get_gladewidget(SEARCH_BASIC)->widget;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), s->basic_search);

	widget = sr_get_gladewidget(STOP_BUTTON)->widget; 	
	gtk_widget_set_sensitive (widget, FALSE);

	basic_search_toggled();
}

/* -------------- Callbacks --------------------- */

gboolean
on_search_replace_delete_event(GtkWidget *window, GdkEvent *event,
			       gboolean user_data)
{
	if (sg->showing)
	{
		gtk_widget_hide(sg->dialog);
		sg->showing = FALSE;
	}
	return TRUE;
}

gboolean
on_search_dialog_key_press_event(GtkWidget *widget, GdkEventKey *event,
                               gpointer user_data)
{
	if (event->keyval == GDK_Escape)
	{
		if (user_data)
		{
			/* Escape pressed in Find window */
			gtk_widget_hide(widget);
			sg->showing = FALSE;
		}
		else
		{
			/* Escape pressed in wrap yes/no window */
			gtk_dialog_response (GTK_DIALOG (widget), GTK_RESPONSE_NO);
		}
		return TRUE;
	}
	else
	{
		if ( (event->state & GDK_CONTROL_MASK) &&
				((event->keyval & 0x5F) == GDK_G))
		{
			if (event->state & GDK_SHIFT_MASK)
				search_replace_previous();
			else
				search_replace_next();
		}
		return FALSE;
	}
}

static void
search_disconnect_set_toggle_connect(GladeWidgetId id, GCallback function, 
	                                 gboolean active)
{
	GtkWidget *button;
	
	button = sr_get_gladewidget(id)->widget;
	g_signal_handlers_disconnect_by_func(G_OBJECT(button), function, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), active);
	g_signal_connect(G_OBJECT(button), "toggled", function, NULL);
}


void
on_search_match_whole_word_toggled (GtkToggleButton *togglebutton, 
									gpointer user_data)
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton)))
	{
		search_disconnect_set_toggle_connect(WHOLE_LINE, (GCallback) 
				on_search_match_whole_line_toggled, FALSE);
		search_disconnect_set_toggle_connect(WORD_START, (GCallback) 
				on_search_match_word_start_toggled, FALSE);
	}
}

void
on_search_match_whole_line_toggled (GtkToggleButton *togglebutton, 
									gpointer user_data)
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton)))
	{
		search_disconnect_set_toggle_connect(WHOLE_WORD, (GCallback) 
				on_search_match_whole_word_toggled, FALSE);
		search_disconnect_set_toggle_connect(WORD_START, (GCallback) 
				on_search_match_word_start_toggled, FALSE);
	}
}

void
on_search_match_word_start_toggled (GtkToggleButton *togglebutton, 
									gpointer user_data)
{	
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton)))
	{
		search_disconnect_set_toggle_connect(WHOLE_WORD, (GCallback) 
				on_search_match_whole_word_toggled, FALSE);
		search_disconnect_set_toggle_connect(WHOLE_LINE, (GCallback) 
				on_search_match_whole_line_toggled, FALSE);
	}
}

/*
static void
search_make_sensitive(gboolean sensitive)
{
	static char *widgets[] = {
		SEARCH_EXPR_FRAME, SEARCH_TARGET_FRAME, CLOSE_BUTTON, SEARCH_BUTTON,
		JUMP_BUTTON
	};
	gint i;
	GtkWidget *widget;
	
	for (i=0; i < sizeof(widgets)/sizeof(widgets[0]); ++i)
	{
		widget = sr_get_gladewidget(widgets[i])->widget;
		if (NULL != widget)
			gtk_widget_set_sensitive(widget, sensitive);
	}
}
*/

void
on_search_regex_toggled (GtkToggleButton *togglebutton, gpointer user_data)
{
	static GladeWidgetId dependent_widgets[] = {
		GREEDY, IGNORE_CASE, WHOLE_WORD, WHOLE_LINE, WORD_START
	};
	int i;
	GtkWidget *dircombo = sr_get_gladewidget(SEARCH_DIRECTION_COMBO)->widget;
	GtkWidget *repl_regex = sr_get_gladewidget(REPLACE_REGEX)->widget;
	GtkWidget *widget;
	gboolean state = gtk_toggle_button_get_active(togglebutton);

	if (state)
	{
		search_set_direction(SD_FORWARD);
	}
	
	gtk_widget_set_sensitive(dircombo, !state);
	gtk_widget_set_sensitive(repl_regex, state);
	
	for (i=0; i < sizeof(dependent_widgets)/sizeof(dependent_widgets[0]); ++i)
	{
		widget = sr_get_gladewidget(dependent_widgets[i])->widget;
		if (NULL != widget)
		{
			gtk_widget_set_sensitive(widget, !state);
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(widget), FALSE);
		}
	}
}

static void
search_set_toggle_direction(SearchDirection dir)
{
	switch (dir)
	{
		case SD_FORWARD :
			search_disconnect_set_toggle_connect(SEARCH_FORWARD, (GCallback) 
				on_search_forward_toggled, TRUE);
			break;
		case SD_BACKWARD :
			search_disconnect_set_toggle_connect(SEARCH_BACKWARD, (GCallback) 
				on_search_backward_toggled, TRUE);
			break;
		case SD_BEGINNING :
			search_disconnect_set_toggle_connect(SEARCH_FULL_BUFFER, (GCallback) 
				on_search_full_buffer_toggled, TRUE);
			break;
	}
}

void
on_search_direction_changed (GtkComboBox *combo, gpointer user_data)
{
	SearchDirection dir;

	dir = search_get_item_combo(combo);
	search_set_toggle_direction(dir);
	search_direction_changed(dir);
}

void
on_search_action_changed (GtkComboBox *combo, gpointer user_data)
{
	SearchAction act;
	SearchRangeType rt;
	
	reset_flags();
	act = search_get_item_combo(combo);
	rt = search_get_item_combo_name(SEARCH_TARGET_COMBO);
	show_jump_button (FALSE);
	switch(act)
	{
		case SA_SELECT:
			search_show_replace(FALSE);
			modify_label_image_button(SEARCH_BUTTON, _("Search"), GTK_STOCK_FIND);
			if (rt == SR_OPEN_BUFFERS || rt == SR_PROJECT ||
					rt == SR_FILES)
				search_set_target(SR_BUFFER);
			break;
		case SA_REPLACE:
			search_show_replace(TRUE);
			modify_label_image_button(SEARCH_BUTTON, _("Search"), GTK_STOCK_FIND);
			if (rt == SR_OPEN_BUFFERS || rt == SR_PROJECT || 
					rt == SR_FILES)
				search_set_target(SR_BUFFER);
			break;
		case SA_REPLACEALL:
			search_show_replace(TRUE);
			modify_label_image_button(SEARCH_BUTTON, _("Replace All"), 
								          GTK_STOCK_FIND_AND_REPLACE);
			break;
		default:
			search_show_replace(FALSE);
			modify_label_image_button(SEARCH_BUTTON, _("Search"), GTK_STOCK_FIND);
			break;
	}
}

void
on_search_target_changed(GtkComboBox *combo, gpointer user_data)
{
	SearchRangeType tgt;
	SearchDirection dir;
	SearchAction act;
	GtkWidget *search_var_frame = sr_get_gladewidget(SEARCH_VAR_FRAME)->widget;
	GtkWidget *file_filter_frame = sr_get_gladewidget(FILE_FILTER_FRAME)->widget;

	tgt = search_get_item_combo(combo);
	switch(tgt)
	{
		case SR_FILES:
			gtk_widget_hide(search_var_frame);
			gtk_widget_show(file_filter_frame);
			break;
		default:
			gtk_widget_hide(search_var_frame);
			gtk_widget_hide(file_filter_frame);
			break;
	}
	
	dir = search_get_item_combo_name(SEARCH_DIRECTION_COMBO);	
	
	if (tgt == SR_SELECTION || tgt == SR_BLOCK || tgt == SR_FUNCTION)
	{
		
		if (dir == SD_BEGINNING)
		{
			search_set_direction(SD_FORWARD);
		}
	}
	if (tgt == SR_OPEN_BUFFERS || tgt == SR_PROJECT ||
			tgt == SR_FILES)
	{
		search_set_direction(SD_BEGINNING);

		act = search_get_item_combo_name(SEARCH_ACTION_COMBO);	
		if (act != SA_REPLACE && act != SA_REPLACEALL)
		{
			if (tgt == SR_OPEN_BUFFERS)
				search_set_action(SA_BOOKMARK);
			else
				search_set_action(SA_FIND_PANE);
		}
		else
		{
			search_set_action(SA_REPLACEALL);	
			sr->search.action = SA_REPLACEALL;
		}
	}
	reset_flags_and_search_button();
	/*  Resize dialog  */
	gtk_window_resize(GTK_WINDOW(sg->dialog), 10, 10);
}

void
on_search_expression_changed(GtkComboBox *combo, gpointer user_data)
{
	GtkWidget *search_entry = sr_get_gladewidget(SEARCH_STRING)->widget;
	GtkWidget *widget = sr_get_gladewidget(SEARCH_BUTTON)->widget;
	gboolean sensitive;

	sensitive = *gtk_entry_get_text (GTK_ENTRY (search_entry)) == '\0' ? FALSE: TRUE;
	gtk_widget_set_sensitive (widget, sensitive);
}


void
on_actions_no_limit_clicked(GtkButton *button, gpointer user_data)
{
	GtkWidget *actions_max = sr_get_gladewidget(ACTIONS_MAX)->widget;
	
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)))
    	gtk_widget_set_sensitive (actions_max, FALSE);
	else
		gtk_widget_set_sensitive (actions_max, TRUE);	
}

void
on_search_button_close_clicked(GtkButton *button, gpointer user_data)
{
	if (sg->showing)
	{
		gtk_widget_hide(sg->dialog);
		sg->showing = FALSE;
	}
}

void
on_search_button_stop_clicked(GtkButton *button, gpointer user_data)
{
	end_activity = TRUE;
}

void
on_search_button_next_clicked(GtkButton *button, gpointer user_data)
{	
	clear_pcre(); 	
	search_replace_populate();

	search_and_replace();
}

void search_replace_find_usage(const gchar *symbol)
{
	gchar *project_root_uri = NULL;
	SearchReplace *old_sr = sr;
	AnjutaShell* shell;
	
	sr = g_new (SearchReplace, 1);

	sr->search.expr.search_str = g_strdup (symbol);
	sr->search.expr.regex = FALSE;
	sr->search.expr.greedy = FALSE;
	sr->search.expr.ignore_case = FALSE;
	sr->search.expr.whole_word = TRUE;
	sr->search.expr.whole_line = FALSE;
	sr->search.expr.word_start = FALSE;
	sr->search.expr.no_limit = TRUE;
	sr->search.expr.actions_max = G_MAXINT;
	sr->search.expr.re = NULL;
		
	g_object_get(G_OBJECT(sr->docman), "shell", &shell, NULL);
	
	anjuta_shell_get (shell,
					  "project_root_uri", G_TYPE_STRING,
					  &project_root_uri, NULL);
	
	sr->search.range.type =
		project_root_uri != NULL ? SR_PROJECT : SR_OPEN_BUFFERS;
	g_free (project_root_uri);
	
	sr->search.range.direction = SD_BEGINNING;

	sr->search.range.var = NULL;

	sr->search.range.files.top_dir = NULL;
	sr->search.range.files.match_files = NULL;
	sr->search.range.files.match_dirs = NULL;
	sr->search.range.files.ignore_files = NULL;
	sr->search.range.files.ignore_dirs = NULL;
	sr->search.range.files.ignore_hidden_files = TRUE;
	sr->search.range.files.ignore_hidden_dirs = TRUE;
	sr->search.range.files.recurse = TRUE;

	sr->search.action = SA_FIND_PANE;

	sr->search.expr_history = NULL;
	sr->search.incremental_pos = 0;
	sr->search.incremental_wrap = TRUE;

	create_dialog ();

	search_and_replace();
	g_free (sr);
	sr = old_sr;
}

void
on_search_button_jump_clicked(GtkButton *button, gpointer user_data)
{
	if (sr)
		interactive = FALSE;
	gtk_widget_hide(GTK_WIDGET(button));

	search_replace_populate();
	search_and_replace();
}

void
on_search_expression_activate (GtkEditable *edit, gpointer user_data)
{
	GtkWidget *combo;

	search_replace_populate();
	
	search_and_replace();
	combo = GTK_WIDGET(edit)->parent;
	reset_flags_and_search_button();
}


void
on_search_full_buffer_toggled (GtkToggleButton *togglebutton, 
									gpointer user_data)
{
	if (gtk_toggle_button_get_active(togglebutton))
	{
		search_set_direction(SD_BEGINNING);
	}
}

void
on_search_forward_toggled (GtkToggleButton *togglebutton, 
									gpointer user_data)
{
	if (gtk_toggle_button_get_active(togglebutton))
	{
		search_set_direction(SD_FORWARD);
	}
}

void
on_search_backward_toggled (GtkToggleButton *togglebutton, 
									gpointer user_data)
{
	if (gtk_toggle_button_get_active(togglebutton))
	{
		search_set_direction(SD_BACKWARD);
	}
}

void
on_setting_basic_search_toggled (GtkToggleButton *togglebutton, 
									gpointer user_data)
{
	SearchAction act;
	GtkWidget *frame_basic = sr_get_gladewidget(FRAME_SEARCH_BASIC)->widget;

	if (gtk_toggle_button_get_active(togglebutton))
	{
    	gtk_widget_show(frame_basic);
		search_set_target(SR_BUFFER);	
		search_set_direction(SD_FORWARD);

		act = search_get_item_combo_name(SEARCH_ACTION_COMBO);
		if (act == SA_REPLACE || act == SA_REPLACEALL)
			search_set_action(SA_REPLACE);
		else		
			search_set_action(SA_SELECT);
	}
	else
		gtk_widget_hide(frame_basic);
}


static void
basic_search_toggled(void)
{
	GtkToggleButton *togglebutton;
	
	togglebutton = GTK_TOGGLE_BUTTON(sr_get_gladewidget(SEARCH_BASIC)->widget);
	
	on_setting_basic_search_toggled (togglebutton, NULL);
}

/***********************************************************************/

#define MAX_LENGTH_SEARCH 64

void 
anjuta_search_replace_activate (gboolean replace, gboolean project)
{
	GtkWidget *notebook;
	GtkWidget *search_entry;
	IAnjutaDocument *doc;
	IAnjutaEditor *te;

	create_dialog ();

	search_update_dialog();

	search_replace_populate();

	reset_flags_and_search_button();
	
	search_entry = sr_get_gladewidget(SEARCH_STRING)->widget;
	doc = ianjuta_document_manager_get_current_document(sr->docman, NULL);
	te = (IANJUTA_IS_EDITOR (doc)) ? IANJUTA_EDITOR (doc) : NULL;
	if (te && search_entry && sr->search.range.type != SR_SELECTION)
	{
		/* Set properties */
		gchar *current_word;

		current_word = ianjuta_editor_selection_get
						(IANJUTA_EDITOR_SELECTION (te), NULL);
		if (current_word == NULL)
			current_word = ianjuta_editor_get_current_word (te, NULL);

		if (current_word && strlen(current_word) > 0 )
		{
			if (strlen(current_word) > MAX_LENGTH_SEARCH)
				current_word[MAX_LENGTH_SEARCH] = '\0';
			gtk_entry_set_text(GTK_ENTRY (search_entry), current_word);
			g_free(current_word);
		}
	}
	
	if (replace)
	{
		if ( !(sr->search.action == SA_REPLACE || 
				sr->search.action == SA_REPLACEALL))
		{
			search_set_action(SA_REPLACE);
			sr->search.action = SA_REPLACE;
			search_show_replace(TRUE);
		}		
	}
	else
	{
		if (sr->search.action == SA_REPLACE || sr->search.action == SA_REPLACEALL)	
		{
			search_set_action(SA_SELECT);
			sr->search.action = SA_SELECT;
			search_show_replace(FALSE);
		}
	}
	if (sr->search.action != SA_REPLACEALL)
		modify_label_image_button(SEARCH_BUTTON, _("Search"), GTK_STOCK_FIND);
	
	if (project)
	{
		search_set_target(SR_PROJECT);
		if (!replace)
		{
			search_set_action (SA_FIND_PANE);
			search_set_direction (SD_BEGINNING);
		}
	}
	show_jump_button(FALSE);
	
	notebook = sr_get_gladewidget(SEARCH_NOTEBOOK)->widget;
	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 0);
	
	/* Show the dialog */
	if (search_entry)
	gtk_widget_grab_focus (search_entry);
	show_dialog();
}
