#ifndef _SEARCH_REPLACE_H
#define _SEARCH_REPLACE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <pcre.h>
#include <glade/glade.h>

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

void search_and_replace (void);
	
void search_replace_next(void);
	
void search_replace_previous(void);

void search_replace_find_usage(gchar *symbol);

void anjuta_search_replace_activate (gboolean replace, gboolean project);
	
GladeWidget *sr_get_gladewidget(const gchar *name);

void search_replace_populate(void);

void search_update_dialog(void);


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
#define SETTING_PREF_ENTRY "setting.pref.entry"

/* Treeview */
#define SETTING_PREF_TREEVIEW "setting.pref.treeview"

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

	
#ifdef __cplusplus
}
#endif

#endif /* _SEARCH_REPLACE_H */
