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

// #include "anjuta.h"
#include "text_editor.h"
// #include "anjuta-tools.h"

#define GTK
#undef PLAT_GTK
#define PLAT_GTK 1
#include "Scintilla.h"
#include "SciLexer.h"
#include "ScintillaWidget.h"

#include "search-replace_backend.h"
#include "search-replace.h"

#define GLADE_FILE_SEARCH_REPLACE PACKAGE_DATA_DIR"/anjuta.glade"

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
void on_search_action_changed (GtkEditable *editable, gpointer user_data);
void on_search_target_changed(GtkEditable *editable, gpointer user_data);
void on_actions_no_limit_clicked(GtkButton *button, gpointer user_data);
void on_search_button_close_clicked(GtkButton *button, gpointer user_data);
void on_search_button_close_clicked(GtkButton *button, gpointer user_data);
void on_search_button_help_clicked(GtkButton *button, gpointer user_data);
void on_search_button_next_clicked(GtkButton *button, gpointer user_data);
void on_search_button_jump_clicked(GtkButton *button, gpointer user_data);
void on_search_expression_activate (GtkEditable *edit, gpointer user_data);
void on_search_button_save_clicked(GtkButton *button, gpointer user_data);

void on_search_direction_changed (GtkEditable *editable, gpointer user_data);
void on_search_full_buffer_toggled (GtkToggleButton *togglebutton, 
									gpointer user_data);
void on_search_forward_toggled (GtkToggleButton *togglebutton, 
									gpointer user_data);
void on_search_backward_toggled (GtkToggleButton *togglebutton, 
									gpointer user_data);
									
/* The GUI part starts here */

#define GLADE_FILE "anjuta.glade"
#define SEARCH_REPLACE_DIALOG "dialog.search.replace"
#define CLOSE_BUTTON "button.close"
#define STOP_BUTTON "button.stop"
#define SEARCH_BUTTON "button.next"
#define JUMP_BUTTON "button.jump"
#define SEARCH_NOTEBOOK "search.notebook"

/* Frames */
#define SEARCH_EXPR_FRAME "frame.search.expression"
#define SEARCH_TARGET_FRAME "frame.search.target"
#define SEARCH_VAR_FRAME "frame.search.var"
#define FILE_FILTER_FRAME "frame.file.filter"
#define REPLACE_FRAME "frame.replace"
#define FRAME_SEARCH_BASIC "frame.search.basic"

/* Entries */
#define SEARCH_STRING "search.string"
#define SEARCH_TARGET "search.target"
#define SEARCH_ACTION "search.action"
#define SEARCH_VAR "search.var"
#define MATCH_FILES "file.filter.match"
#define UNMATCH_FILES "file.filter.unmatch"
#define MATCH_DIRS "dir.filter.match"
#define UNMATCH_DIRS "dir.filter.unmatch"
#define REPLACE_STRING "replace.string"
#define SEARCH_DIRECTION "search.direction"
#define ACTIONS_MAX "actions.max"

/* Checkboxes */
#define SEARCH_REGEX "search.regex"
#define GREEDY "search.greedy"
#define IGNORE_CASE "search.ignore.case"
#define WHOLE_WORD "search.match.whole.word"
#define WORD_START "search.match.word.start"
#define WHOLE_LINE "search.match.whole.line"
#define IGNORE_HIDDEN_FILES "ignore.hidden.files"
#define IGNORE_BINARY_FILES "ignore.binary.files"
#define IGNORE_HIDDEN_DIRS "ignore.hidden.dirs"
#define SEARCH_RECURSIVE "search.dir.recursive"
#define REPLACE_REGEX "replace.regex"
#define ACTIONS_NO_LIMIT "actions.no_limit"
#define SEARCH_FULL_BUFFER "search.full_buffer"
#define SEARCH_FORWARD "search.forward"
#define SEARCH_BACKWARD "search.backward"

/* Labels */
#define LABEL_REPLACE "label.replace"

/* Combo boxes */
#define SEARCH_STRING_COMBO "search.string.combo"
#define SEARCH_TARGET_COMBO "search.target.combo"
#define SEARCH_ACTION_COMBO "search.action.combo"
#define SEARCH_VAR_COMBO "search.var.combo"
#define MATCH_FILES_COMBO "file.filter.match.combo"
#define UNMATCH_FILES_COMBO "file.filter.unmatch.combo"
#define MATCH_DIRS_COMBO "dir.filter.match.combo"
#define UNMATCH_DIRS_COMBO "dir.filter.unmatch.combo"
#define REPLACE_STRING_COMBO "replace.string.combo"
#define SEARCH_DIRECTION_COMBO "search.direction.combo"

/* GUI dropdown option strings */
AnjutaUtilStringMap search_direction_strings[] = {
	{SD_FORWARD, "Forward"},
	{SD_BACKWARD, "Backward"},
	{SD_BEGINNING, "Full Buffer"},
	{-1, NULL}
};

AnjutaUtilStringMap search_target_strings[] = {
	{SR_BUFFER, "Current Buffer"},
	{SR_SELECTION,"Current Selection"},
	{SR_BLOCK, "Current Block"},
	{SR_FUNCTION, "Current Function"},
	{SR_OPEN_BUFFERS, "All Open Buffers"},
	{SR_PROJECT, "All Project Files"},
	{SR_VARIABLE, "Specify File List"},
	{SR_FILES, "Specify File Patterns"},
	{-1, NULL}
};

AnjutaUtilStringMap search_action_strings[] = {
	{SA_SELECT, "Select the first match"},
	{SA_BOOKMARK, "Bookmark all matched lines"},
	{SA_HIGHLIGHT, "Mark all matched strings"},
	{SA_FIND_PANE, "Show result in find pane"},
	{SA_REPLACE, "Replace first match"},
	{SA_REPLACEALL, "Replace all matches"},
	{-1, NULL}
};

typedef enum _GUIElementType
{
	GE_NONE,
	GE_BUTTON,
	GE_TEXT,
	GE_BOOLEAN,
	GE_OPTION,
	GE_COMBO
} GUIElementType;

typedef struct _GladeWidget
{
	GUIElementType type;
	char *name;
	gpointer extra;
	GtkWidget *widget;
} GladeWidget;

typedef struct _SearchReplaceGUI
{
	GladeXML *xml;
	GtkWidget *dialog;
	gboolean showing;
} SearchReplaceGUI;


static GladeWidget glade_widgets[] = {
	{GE_NONE, SEARCH_REPLACE_DIALOG, NULL, NULL},
	{GE_BUTTON, CLOSE_BUTTON, NULL, NULL},
	{GE_BUTTON, STOP_BUTTON, NULL, NULL},
	{GE_BUTTON, SEARCH_BUTTON, NULL, NULL},
	{GE_BUTTON, JUMP_BUTTON, NULL, NULL},
	{GE_NONE, SEARCH_EXPR_FRAME, NULL, NULL},
	{GE_NONE, SEARCH_TARGET_FRAME, NULL, NULL},
	{GE_NONE, SEARCH_VAR_FRAME, NULL, NULL},
	{GE_NONE, FILE_FILTER_FRAME, NULL, NULL},
	{GE_NONE, FRAME_SEARCH_BASIC, NULL, NULL},
	{GE_NONE, LABEL_REPLACE, NULL, NULL},
	{GE_TEXT, SEARCH_STRING, NULL, NULL},
	{GE_OPTION, SEARCH_TARGET, search_target_strings, NULL},
	{GE_OPTION, SEARCH_ACTION, search_action_strings, NULL},
	{GE_TEXT, SEARCH_VAR, NULL, NULL},
	{GE_TEXT, MATCH_FILES, NULL, NULL},
	{GE_TEXT, UNMATCH_FILES, NULL, NULL},
	{GE_TEXT, MATCH_DIRS, NULL, NULL},
	{GE_TEXT, UNMATCH_DIRS, NULL, NULL},
	{GE_TEXT, REPLACE_STRING, NULL, NULL},
	{GE_TEXT, ACTIONS_MAX, NULL, NULL},
	{GE_OPTION, SEARCH_DIRECTION, search_direction_strings, NULL},
	{GE_BOOLEAN, SEARCH_REGEX, NULL, NULL},
	{GE_BOOLEAN, GREEDY, NULL, NULL},
	{GE_BOOLEAN, IGNORE_CASE, NULL, NULL},
	{GE_BOOLEAN, WHOLE_WORD, NULL, NULL},
	{GE_BOOLEAN, WORD_START, NULL, NULL},
	{GE_BOOLEAN, WHOLE_LINE, NULL, NULL},
	{GE_BOOLEAN, IGNORE_HIDDEN_FILES, NULL, NULL},
	{GE_BOOLEAN, IGNORE_BINARY_FILES, NULL, NULL},
	{GE_BOOLEAN, IGNORE_HIDDEN_DIRS, NULL, NULL},
	{GE_BOOLEAN, SEARCH_RECURSIVE, NULL, NULL},
	{GE_BOOLEAN, REPLACE_REGEX, NULL, NULL},
	{GE_BOOLEAN, ACTIONS_NO_LIMIT, NULL, NULL},
	{GE_BOOLEAN, SEARCH_FULL_BUFFER, NULL, NULL},
	{GE_BOOLEAN, SEARCH_FORWARD, NULL, NULL},	
	{GE_BOOLEAN, SEARCH_BACKWARD, NULL, NULL},	
	{GE_COMBO, SEARCH_STRING_COMBO, NULL, NULL},
	{GE_COMBO, SEARCH_TARGET_COMBO, search_target_strings, NULL},
	{GE_COMBO, SEARCH_ACTION_COMBO, search_action_strings, NULL},
	{GE_COMBO, SEARCH_VAR_COMBO, NULL, NULL},
	{GE_COMBO, MATCH_FILES_COMBO, NULL, NULL},
	{GE_COMBO, UNMATCH_FILES_COMBO, NULL, NULL},
	{GE_COMBO, MATCH_DIRS_COMBO, NULL, NULL},
	{GE_COMBO, UNMATCH_DIRS_COMBO, NULL, NULL},
	{GE_COMBO, REPLACE_STRING_COMBO, NULL, NULL},
	{GE_COMBO, SEARCH_DIRECTION_COMBO, search_direction_strings, NULL},
	{GE_NONE, SEARCH_NOTEBOOK, NULL, NULL},
	{GE_NONE, NULL, NULL, NULL}
};

/***********************************************************/

static GladeWidget *sr_get_gladewidget(const gchar *name);
static void search_set_action(SearchAction action);
static void search_set_target(SearchRangeType target);
static void search_set_direction(SearchDirection dir);
static void populate_value(const char *name, gpointer val_ptr);
static void reset_flags(void);
static void reset_flags_and_search_button (void);
static void search_start_over (SearchDirection direction);
static void search_end_alert (gchar *string);
static void max_results_alert (void);
static void nb_results_alert (gint nb);
static void search_show_replace(gboolean hide);
static void modify_label_image_button(gchar *button_name, gchar *name, char *stock_image);
static void search_replace_populate(void);
static void show_jump_button (gboolean show);
static gboolean create_dialog(void);
static void show_dialog(void);
static gboolean word_in_list(GList *list, gchar *word);
static GList* list_max_items(GList *list, guint nb_max);
static void search_update_combos (void);
static void replace_update_combos (void);
static void search_update_dialog(void);
static void search_make_sensitive(gboolean sensitive);
static void search_direction_changed(SearchDirection dir);
static void search_set_direction(SearchDirection dir);
static void search_set_toggle_direction(SearchDirection dir);
static void search_disconnect_set_toggle_connect(const gchar *name, 
	GtkSignalFunc function, gboolean active);
static void search_replace_next_previous(SearchDirection dir);
static void search_set_combo(gchar *name_combo, gchar *name_entry, 
	GtkSignalFunc function, gint command);
static gint search_get_item_combo(GtkEditable *editable, AnjutaUtilStringMap *map);
static gint search_get_item_combo_name(gchar *name, AnjutaUtilStringMap *map);


static SearchReplaceGUI *sg = NULL;

static SearchReplace *sr = NULL;

gboolean flag_select = FALSE;
gboolean interactive = FALSE;
gboolean end_activity = FALSE;


/***********************************************************/

void
search_and_replace_init (AnjutaDocman *dm)
{
	sr = create_search_replace_instance (dm);
}

void
search_and_replace (void)
{
	GList *entries;
	GList *tmp;
	char buf[BUFSIZ];
	SearchEntry *se;
	FileBuffer *fb;
	static MatchInfo *mi;
	Search *s;
	gint offset;
	static gint os=0;
	gchar *match_line;
	gint nb_results;
	static long start_sel = 0;
	static long end_sel = 0;
	static gchar *ch= NULL;
	
	g_return_if_fail(sr);
	s = &(sr->search);

	end_activity = FALSE;
	search_make_sensitive(FALSE);
	
	entries = create_search_entries(s);
	
	search_update_combos (); //
	if (s->action == SA_REPLACE || s->action == SA_REPLACEALL)
		replace_update_combos ();

	if (SA_FIND_PANE == s->action)
	{
		// FIXME: message-pane
		// an_message_manager_clear(app->messages, MESSAGE_FIND);
		// an_message_manager_show(app->messages, MESSAGE_FIND);
	}

	nb_results = 0;
	for (tmp = entries; tmp && (nb_results <= s->expr.actions_max); 
		 tmp = g_list_next(tmp))
	{
		if (end_activity)
			break;
		while(gtk_events_pending())
			gtk_main_iteration();
		
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
				
				
				switch (s->action)
				{
					case SA_HIGHLIGHT: /* FIXME */
					case SA_BOOKMARK:
						if (NULL == fb->te)
						{
							// FIXME: fb->te = anjuta_goto_file_line(fb->path, mi->line);
							fb->te = anjuta_docman_goto_file_line (sr->docman, 
											fb->path, mi->line);
						}
						if (fb->te)
						{
							scintilla_send_message(SCINTILLA(
							  fb->te->scintilla), SCI_MARKERADD
							  , mi->line, 0);
						}
						break;
						
					case SA_SELECT:
						if (NULL == fb->te)
							// FIXME: fb->te = anjuta_goto_file_line(fb->path, mi->line+1);
							fb->te = anjuta_docman_goto_file_line (sr->docman, 
											fb->path, mi->line+1);
						scintilla_send_message(SCINTILLA(
						  fb->te->scintilla), SCI_SETSEL, mi->pos
						  , (mi->pos + mi->len));
						break;
						
					case SA_FIND_PANE: 
						match_line = file_match_line_from_pos(fb, mi->pos);
						snprintf(buf, BUFSIZ, "%s:%ld:%s\n", fb->path
						  , mi->line + 1, match_line);
						g_free(match_line);
						// FIXME: an_message_manager_append(app->messages, buf
						//  , MESSAGE_FIND);
						break;
					
					case SA_REPLACE:
						if (!interactive)
						{
							if (NULL == fb->te)
								// FIXME: fb->te = anjuta_goto_file_line(fb->path, mi->line+1);
								fb->te = anjuta_docman_goto_file_line (sr->docman, 
											fb->path, mi->line+1);
							scintilla_send_message(SCINTILLA(
						  		fb->te->scintilla), SCI_SETSEL,
								mi->pos - offset, mi->pos - offset + mi->len);
							interactive = TRUE;
							os = offset;
							modify_label_image_button(SEARCH_BUTTON, _("Replace"), 
								                      GTK_STOCK_FIND_AND_REPLACE);
							show_jump_button(TRUE);
							if (sr->replace.regex && sr->search.expr.regex)
							{
								if (ch)
								{
									g_free (ch);
								}
								ch = g_strdup(regex_backref(mi, fb));
							}
							break;
						}

						if (ch && sr->replace.regex && sr->search.expr.regex)
						{
							sr->replace.repl_str = g_strdup(ch);
							g_free (ch);
						}
						
//						if (fb->te == NULL)
//							fb->te = anjuta_append_text_editor(se->path);
						scintilla_send_message(SCINTILLA(fb->te->scintilla), 
							SCI_SETSEL, mi->pos - os, mi->pos + mi->len - os);
						scintilla_send_message(SCINTILLA(fb->te->scintilla),
							SCI_REPLACESEL, 0, (long) (sr->replace).repl_str); 
					
						if (se->direction != SD_BACKWARD)						
							offset += mi->len - strlen(sr->replace.repl_str);
						
						interactive = FALSE;
						break;
					case SA_REPLACEALL:
						if ((sr->replace.regex) && (sr->search.expr.regex))
							sr->replace.repl_str = g_strdup(regex_backref(mi, fb));
//						if (fb->te == NULL)
//							fb->te = anjuta_append_text_editor(se->path);
						scintilla_send_message(SCINTILLA(fb->te->scintilla), 
							SCI_SETSEL, mi->pos - offset, mi->pos + mi->len - offset);
						scintilla_send_message(SCINTILLA(fb->te->scintilla),
							SCI_REPLACESEL, 0, (long) (sr->replace).repl_str); 
						if (se->direction != SD_BACKWARD)						
							offset += mi->len - strlen(sr->replace.repl_str);
						break;
					default:
						// FIXME: anjuta_not_implemented(__FILE__, __LINE__);
						break;
				}  // switch

				if (se->direction != SD_BACKWARD)
					start_sel = mi->pos + mi->len - offset; 
				else
					start_sel = mi->pos - offset; 
				
				if (SA_REPLACE != s->action || (SA_REPLACE == s->action && !interactive))
					match_info_free(mi);
				
				if (SA_SELECT == s->action || ((SA_REPLACE == s->action || 
					SA_REPLACEALL == s->action)&& interactive))
					break;
			} // while
			
		}  // if (fb)
		
		file_buffer_free(fb);
		g_free(se);
		if (SA_SELECT == s->action && nb_results > 0)
			break;
	}  // for
	
	if (s->range.type == SR_BLOCK || s->range.type == SR_FUNCTION || 
		s->range.type == SR_SELECTION)
			flag_select = TRUE;
	
	g_list_free(entries);
	
	if (nb_results == 0)
		search_end_alert(sr->search.expr.search_str);
	else if (nb_results > sr->search.expr.actions_max)
		max_results_alert();
	else if (s->action == SA_REPLACEALL)
		nb_results_alert(nb_results);
	
	if ((s->range.direction == SD_BEGINNING) &&
		((s->action == SA_SELECT) || (s->action == SA_REPLACE)) )
	{
		search_set_direction(SD_FORWARD);
		search_set_toggle_direction(SD_FORWARD);
	}
	
	search_make_sensitive(TRUE);
	return;
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
				save_type == SR_VARIABLE || save_type == SR_FILES)
			sr->search.range.direction = SR_BUFFER;
		sr->search.action = SA_SELECT;
		
		search_and_replace();
		
		sr->search.action = save_action;
		sr->search.range.type = save_type;
		sr->search.range.direction = save_direction;
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

static GladeWidget *
sr_get_gladewidget(const gchar *name)
{
	GladeWidget *gw = NULL;
	int i;
	
	for (i=0; NULL != glade_widgets[i].name; ++i)
	{
		if (0 == strcmp(glade_widgets[i].name, name))
		{
			gw = & (glade_widgets[i]);
			break;
		}
	}
	return gw;
}

static void
search_set_combo(gchar *name_combo, gchar *name_entry, GtkSignalFunc function,
                 gint command)
{
	GtkCombo *combo;
	GtkWidget *entry;
	
	combo = GTK_COMBO(sr_get_gladewidget(name_combo)->widget);
	entry = sr_get_gladewidget(name_entry)->widget;
	gtk_signal_disconnect_by_func(GTK_OBJECT(entry),(GtkSignalFunc)
	                              function , NULL);
	
	gtk_list_select_item(GTK_LIST(combo->list), command);
	gtk_signal_connect(GTK_OBJECT(entry),"changed", (GtkSignalFunc)
	                   function, NULL);
}

static void
search_set_action(SearchAction action)
{
	search_set_combo(SEARCH_ACTION_COMBO, SEARCH_ACTION, (GtkSignalFunc)
		on_search_action_changed, action);
}

static void
search_set_target(SearchRangeType target)
{
	search_set_combo(SEARCH_TARGET_COMBO, SEARCH_TARGET, (GtkSignalFunc)
		on_search_target_changed, target);
}

static void
search_set_direction(SearchDirection dir)
{
	search_set_combo(SEARCH_DIRECTION_COMBO, SEARCH_DIRECTION, (GtkSignalFunc)
		on_search_direction_changed, dir);
}

static gint
search_get_item_combo(GtkEditable *editable, AnjutaUtilStringMap *map)
{
	gchar *s;
	gint item;
	
	s = gtk_editable_get_chars(GTK_EDITABLE(editable), 0, -1);
	item = anjuta_util_type_from_string(map, s);
	g_free(s);
	return item;
}

static gint
search_get_item_combo_name(gchar *name, AnjutaUtilStringMap *map)
{
	GtkWidget *editable = sr_get_gladewidget(name)->widget;
	return search_get_item_combo(GTK_EDITABLE(editable), map);
}

static void
search_direction_changed(SearchDirection dir)
{
	SearchEntryType tgt;
	SearchAction act;
	
	tgt = search_get_item_combo_name(SEARCH_TARGET, search_target_strings);
	if (dir != SD_BEGINNING)
	{
		if (tgt == SR_OPEN_BUFFERS || tgt == SR_PROJECT || tgt == SR_VARIABLE
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
			act = search_get_item_combo_name(SEARCH_ACTION, search_action_strings);
			if (act == SA_SELECT)
				search_set_action(SA_BOOKMARK);
			if (act == SA_REPLACE)
				search_set_action(SA_REPLACEALL);				
		}
	}
}

static void
populate_value(const char *name, gpointer val_ptr)
{
	AnjutaUtilStringMap *map;
	GladeWidget *gw;
	char *s;

	g_return_if_fail(name && val_ptr);
	
	gw = sr_get_gladewidget(name);
	g_return_if_fail(gw);
	switch(gw->type)
	{
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
		case GE_OPTION:
			map = (AnjutaUtilStringMap *) gw->extra;
			g_return_if_fail(map);
			s = gtk_editable_get_chars(GTK_EDITABLE(gw->widget), 0, -1);
			*((int *) val_ptr) = anjuta_util_type_from_string(map, s);
			g_free (s);
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
	TextEditor *te = anjuta_docman_get_current_editor(sr->docman);
	long length;
	
	if (te)
	{
		length = aneditor_command(te->editor_id, ANE_GETLENGTH, 0, 0);
	
		if (direction != SD_BACKWARD)
			/* search from doc start */
			text_editor_goto_point (te, 0);
		else
			/* search from doc end */
			text_editor_goto_point (te, length);
	}
}

static void 
search_end_alert(gchar *string)
{
	GtkWidget *dialog;
	gchar buff[256];

	if (sr->search.range.direction != SD_BEGINNING && !flag_select)
	{
		// Ask if user wants to wrap around the doc
		// Dialog to be made HIG compliant.
		sprintf (buff,
			_("The match \"%s\" was not found. Wrap search around the document?"),
			string);
		dialog = gtk_message_dialog_new (GTK_WINDOW (sg->dialog),
										 GTK_DIALOG_DESTROY_WITH_PARENT,
										 GTK_MESSAGE_QUESTION,
									 	 GTK_BUTTONS_YES_NO,
										 buff);
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
		sprintf (buff,
			_("The match \"%s\" was not found."),
			string);
		dialog = gtk_message_dialog_new(GTK_WINDOW (sg->dialog),
					GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
		        	buff);
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
	static char *hide_widgets[] = {
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
modify_label_image_button(gchar *button_name, gchar *name, char *stock_image)
{
	GList *list, *l;
	GtkHBox *hbox;
	GtkWidget *alignment;
	// GtkWidget *label;
	// GtkWidget *image;
	GtkWidget *button = sr_get_gladewidget(button_name)->widget;
	
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

static void
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
	populate_value(SEARCH_TARGET, &(sr->search.range.type));
	populate_value(SEARCH_DIRECTION, &(sr->search.range.direction));
	populate_value(ACTIONS_NO_LIMIT, &(sr->search.expr.no_limit));

	if (sr->search.range.direction == SD_BEGINNING)
		sr->search.range.whole = TRUE;
	else
		sr->search.range.whole = FALSE;
	
	if (sr->search.expr.no_limit)
		sr->search.expr.actions_max = G_MAXINT;	
	else
	{
		populate_value(ACTIONS_MAX, &(max));
		sr->search.expr.actions_max = atoi(max);
		if (sr->search.expr.actions_max == 0)
			sr->search.expr.actions_max = 100;
		g_free(max);
	}

	switch (sr->search.range.type)
	{
		case SR_FUNCTION:
		case SR_BLOCK:
			if (flag_select)
				sr->search.range.type = SR_SELECTION;
			break;
		case SR_VARIABLE:
			populate_value(SEARCH_VAR, &(sr->search.range.var));
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
	populate_value(SEARCH_ACTION, &(sr->search.action));
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


static gboolean
create_dialog(void)
{
	char glade_file[PATH_MAX];
	GladeWidget *w;
	GList *combo_strings;
	int i;

	g_return_val_if_fail(NULL != sr, FALSE);
	sg = g_new0(SearchReplaceGUI, 1);
	snprintf(glade_file, PATH_MAX, "%s/%s", PACKAGE_DATA_DIR, GLADE_FILE);
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
	
	for (i=0; NULL != glade_widgets[i].name; ++i)
	{
		w = &(glade_widgets[i]);
		w->widget = glade_xml_get_widget(sg->xml, w->name);
		gtk_widget_ref(w->widget);
		if (GE_COMBO == w->type && NULL != w->extra)
		{
			combo_strings = anjuta_util_glist_from_map((AnjutaUtilStringMap *) w->extra);
			gtk_combo_set_popdown_strings(GTK_COMBO(w->widget), combo_strings);
			g_list_free(combo_strings);
		}
	}
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
		list = g_list_remove(list, last->data);
		g_free(last->data);
	}
	return list;
}

#define MAX_ITEMS_SEARCH_COMBO 16

//  FIXME  free GList sr->search.expr_history ?????

static void
search_update_combos(void)
{
	GtkWidget *search_entry = NULL;
	gchar *search_word = NULL;
	TextEditor *te = anjuta_docman_get_current_editor (sr->docman);

	search_entry = sr_get_gladewidget(SEARCH_STRING)->widget;
	if (search_entry && te)
	{
		search_word = g_strdup(gtk_entry_get_text((GtkEntry *) search_entry));
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
				gtk_combo_set_popdown_strings((GtkCombo *) search_list,
					sr->search.expr_history);
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
	TextEditor *te = anjuta_docman_get_current_editor (sr->docman);

	replace_entry = sr_get_gladewidget(REPLACE_STRING)->widget;
	if (replace_entry && te)
	{
		replace_word = g_strdup(gtk_entry_get_text((GtkEntry *) replace_entry));
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
				gtk_combo_set_popdown_strings((GtkCombo *) replace_list,
					sr->replace.expr_history);
			}
		}
	}
}

static void 
search_update_dialog(void)
{
	GtkWidget *widget;
	Search *s;

	s = &(sr->search);
	widget = sr_get_gladewidget(SEARCH_REGEX)->widget;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), s->expr.regex);

	widget = sr_get_gladewidget(IGNORE_CASE)->widget;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), s->expr.ignore_case);
	
	widget = sr_get_gladewidget(WHOLE_WORD)->widget;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), s->expr.whole_word);

	widget = sr_get_gladewidget(GREEDY)->widget;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
	widget = sr_get_gladewidget(WHOLE_LINE)->widget;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
	widget = sr_get_gladewidget(WORD_START)->widget;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);

	widget = sr_get_gladewidget(REPLACE_REGEX)->widget;	
	gtk_widget_set_sensitive(widget, sr->search.expr.regex);
	
	widget = sr_get_gladewidget(SEARCH_STRING)->widget;
	if (s->expr.search_str)
		gtk_entry_set_text(GTK_ENTRY(widget), s->expr.search_str);
	
	widget = sr_get_gladewidget(SEARCH_DIRECTION_COMBO)->widget;
	if (s->range.whole)
		gtk_list_select_item (GTK_LIST(GTK_COMBO(widget)->list), SD_BEGINNING);
	else
	{
		if (s->range.direction == SD_FORWARD)
			gtk_list_select_item (GTK_LIST(GTK_COMBO(widget)->list), SD_FORWARD);
		if (s->range.direction == SD_BACKWARD)
			gtk_list_select_item (GTK_LIST(GTK_COMBO(widget)->list), SD_BACKWARD);
	}
	
	widget = sr_get_gladewidget(SEARCH_ACTION_COMBO)->widget;
	gtk_list_select_item (GTK_LIST(GTK_COMBO(widget)->list), s->action);
	widget = sr_get_gladewidget(SEARCH_TARGET_COMBO)->widget;
	gtk_list_select_item (GTK_LIST(GTK_COMBO(widget)->list), s->range.type);

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
	// gchar *str;
	
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
search_disconnect_set_toggle_connect(const gchar *name, GtkSignalFunc function, 
	                                 gboolean active)
{
	GtkWidget *button;
	
	button = sr_get_gladewidget(name)->widget;
	gtk_signal_disconnect_by_func(GTK_OBJECT(button), function, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), active);
	gtk_signal_connect(GTK_OBJECT(button), "toggled", function, NULL);
}


void
on_search_match_whole_word_toggled (GtkToggleButton *togglebutton, 
									gpointer user_data)
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton)))
	{
		search_disconnect_set_toggle_connect(WHOLE_LINE, (GtkSignalFunc) 
				on_search_match_whole_line_toggled, FALSE);
		search_disconnect_set_toggle_connect(WORD_START, (GtkSignalFunc) 
				on_search_match_word_start_toggled, FALSE);
	}
}

void
on_search_match_whole_line_toggled (GtkToggleButton *togglebutton, 
									gpointer user_data)
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton)))
	{
		search_disconnect_set_toggle_connect(WHOLE_WORD, (GtkSignalFunc) 
				on_search_match_whole_word_toggled, FALSE);
		search_disconnect_set_toggle_connect(WORD_START, (GtkSignalFunc) 
				on_search_match_word_start_toggled, FALSE);
	}
}

void
on_search_match_word_start_toggled (GtkToggleButton *togglebutton, 
									gpointer user_data)
{	
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton)))
	{
		search_disconnect_set_toggle_connect(WHOLE_WORD, (GtkSignalFunc) 
				on_search_match_whole_word_toggled, FALSE);
		search_disconnect_set_toggle_connect(WHOLE_LINE, (GtkSignalFunc) 
				on_search_match_whole_line_toggled, FALSE);
	}
}

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

void
on_search_regex_toggled (GtkToggleButton *togglebutton, gpointer user_data)
{
	static char *dependent_widgets[] = {
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
		search_direction_changed(SD_FORWARD);
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
			search_disconnect_set_toggle_connect(SEARCH_FORWARD, (GtkSignalFunc) 
				on_search_forward_toggled, TRUE);
			break;
		case SD_BACKWARD :
			search_disconnect_set_toggle_connect(SEARCH_BACKWARD, (GtkSignalFunc) 
				on_search_backward_toggled, TRUE);
			break;
		case SD_BEGINNING :
			search_disconnect_set_toggle_connect(SEARCH_FULL_BUFFER, (GtkSignalFunc) 
				on_search_full_buffer_toggled, TRUE);
			break;
	}
}

void
on_search_direction_changed (GtkEditable *editable, gpointer user_data)
{
	SearchDirection dir;

	dir = search_get_item_combo(editable, search_direction_strings);
	search_set_toggle_direction(dir);
	search_direction_changed(dir);
}

void
on_search_action_changed (GtkEditable *editable, gpointer user_data)
{
	// GtkWidget *replace_frame = sr_get_gladewidget(REPLACE_FRAME)->widget;
	SearchAction act;
	SearchRangeType rt;
	
	reset_flags();
	act = search_get_item_combo(editable, search_action_strings);
	rt = search_get_item_combo_name(SEARCH_TARGET, search_target_strings);
	switch(act)
	{
		case SA_SELECT:
			search_show_replace(FALSE);
			modify_label_image_button(SEARCH_BUTTON, _("Search"), GTK_STOCK_FIND);
			if (rt == SR_OPEN_BUFFERS || rt == SR_PROJECT || rt == SR_VARIABLE ||
					rt == SR_FILES)
				search_set_target(SR_BUFFER);
			break;
		case SA_REPLACE:
			search_show_replace(TRUE);
			modify_label_image_button(SEARCH_BUTTON, _("Search"), GTK_STOCK_FIND);
			if (rt == SR_OPEN_BUFFERS || rt == SR_PROJECT || rt == SR_VARIABLE ||
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
on_search_target_changed(GtkEditable *editable, gpointer user_data)
{
	SearchRangeType tgt;
	SearchDirection dir;
	SearchAction act;
	GtkWidget *search_var_frame = sr_get_gladewidget(SEARCH_VAR_FRAME)->widget;
	GtkWidget *file_filter_frame = sr_get_gladewidget(FILE_FILTER_FRAME)->widget;

	tgt = search_get_item_combo(editable, search_target_strings);
	switch(tgt)
	{
		case SR_FILES:
			gtk_widget_hide(search_var_frame);
			gtk_widget_show(file_filter_frame);
			break;
		case SR_VARIABLE:
			gtk_widget_show(search_var_frame);
			gtk_widget_hide(file_filter_frame);
			break;
		default:
			gtk_widget_hide(search_var_frame);
			gtk_widget_hide(file_filter_frame);
			break;
	}
	
	dir = search_get_item_combo_name(SEARCH_DIRECTION, search_direction_strings);	
	
	if (tgt == SR_SELECTION || tgt == SR_BLOCK || tgt == SR_FUNCTION)
	{
		
		if (dir == SD_BEGINNING)
		{
			search_set_direction(SD_FORWARD);
			search_set_toggle_direction(SD_FORWARD);
		}
	}
	if (tgt == SR_OPEN_BUFFERS || tgt == SR_PROJECT || tgt == SR_VARIABLE ||
			tgt == SR_FILES)
	{
		search_set_direction(SD_BEGINNING);
		search_set_toggle_direction(SD_BEGINNING);

		act = search_get_item_combo_name(SEARCH_ACTION, search_action_strings);	
		if (act != SA_REPLACE && act != SA_REPLACEALL)
		{
			if (tgt == SR_OPEN_BUFFERS)
				search_set_action(SA_BOOKMARK);
			else
				search_set_action(SA_FIND_PANE);
		}
		else
			search_set_action(SA_REPLACEALL);	
	}
	reset_flags_and_search_button();
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
	clear_pcre(); //	
	search_replace_populate();

	search_and_replace();
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
	gtk_combo_disable_activate((GtkCombo*)combo);
	reset_flags_and_search_button();
}


void
on_search_full_buffer_toggled (GtkToggleButton *togglebutton, 
									gpointer user_data)
{
	if (gtk_toggle_button_get_active(togglebutton))
	{
		search_set_direction(SD_BEGINNING);
		search_direction_changed(SD_BEGINNING);
	}
}

void
on_search_forward_toggled (GtkToggleButton *togglebutton, 
									gpointer user_data)
{
	// GtkWidget *widget;
	if (gtk_toggle_button_get_active(togglebutton))
	{
		search_set_direction(SD_FORWARD);
		search_direction_changed(SD_FORWARD);
	}
}

void
on_search_backward_toggled (GtkToggleButton *togglebutton, 
									gpointer user_data)
{
	// GtkWidget *widget;
	
	if (gtk_toggle_button_get_active(togglebutton))
	{
		search_set_direction(SD_BACKWARD);
		search_direction_changed(SD_BACKWARD);
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
		search_set_toggle_direction(SD_FORWARD);

		act = search_get_item_combo_name(SEARCH_ACTION, search_action_strings);
		if (act == SA_REPLACE || act == SA_REPLACEALL)
			search_set_action(SA_REPLACE);
		else		
			search_set_action(SA_SELECT);
	}
	else
		gtk_widget_hide(frame_basic);
}
/***********************************************************************/

#define MAX_LENGTH_SEARCH 64

void 
anjuta_search_replace_activate (gboolean replace, gboolean project)
{
	GtkWidget *search_entry = NULL;
	gchar *current_word = NULL;
	TextEditor *te;  
	GtkWidget *notebook;
	
	if (NULL == sg)
	{
		if (! create_dialog())
			return;
    }
	
	te = anjuta_docman_get_current_editor(sr->docman);
	search_update_dialog();

	search_replace_populate();
	reset_flags_and_search_button();
	/* Set properties */
	search_entry = sr_get_gladewidget(SEARCH_STRING)->widget;
	if (te && search_entry && sr->search.range.type != SR_SELECTION)
	{
		current_word = text_editor_get_current_word(te);
		if (current_word && strlen(current_word) > 0 )
		{
			if (strlen(current_word) > MAX_LENGTH_SEARCH)
				current_word = g_strndup (current_word, MAX_LENGTH_SEARCH);
			gtk_entry_set_text((GtkEntry *) search_entry, current_word);
			g_free(current_word);
		}	
	}
		
	search_show_replace(replace);
	if (replace)
		search_set_action(SA_REPLACE);
	else
		search_set_action(SA_SELECT);
	
	if (project)
		search_set_target(SR_PROJECT);
	else
		search_set_target(SR_BUFFER);
	
	show_jump_button(FALSE);
	
	notebook = sr_get_gladewidget(SEARCH_NOTEBOOK)->widget;
	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 0);
	
	/* Show the dialog */
	gtk_widget_grab_focus (search_entry);
	show_dialog();
}
