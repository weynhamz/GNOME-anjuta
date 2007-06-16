#ifndef _SEARCH_REPLACE_H
#define _SEARCH_REPLACE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <glib.h>
#include <pcre.h>
#include <glade/glade.h>

typedef enum _GUIElementType
{
	GE_NONE,
	GE_BUTTON,
	GE_COMBO_ENTRY,
	GE_TEXT,
	GE_BOOLEAN,
	GE_COMBO
} GUIElementType;

typedef struct _GladeWidget
{
	GUIElementType type;
	char *name;
	gpointer extra;
	GtkWidget *widget;
} GladeWidget;

#define GLADE_FILE "anjuta.glade"
#define SEARCH_REPLACE_DIALOG "dialog.search.replace"

/* Enum for all useful glade widget */
typedef enum _GladeWidgetId
{
	CLOSE_BUTTON,
	STOP_BUTTON,
	SEARCH_BUTTON,
	JUMP_BUTTON,
	SEARCH_NOTEBOOK,

	/* Frames */
	SEARCH_EXPR_FRAME,
	SEARCH_TARGET_FRAME,
	SEARCH_VAR_FRAME,
	FILE_FILTER_FRAME,
	FRAME_SEARCH_BASIC,

	/* Labels */
	LABEL_REPLACE,

	/* Entries */
	SEARCH_STRING,
	SEARCH_VAR,
	MATCH_FILES,
	UNMATCH_FILES,
	MATCH_DIRS,
	UNMATCH_DIRS,
	REPLACE_STRING,
	ACTIONS_MAX,
	SETTING_PREF_ENTRY,

	/* Checkboxes */
	SEARCH_REGEX,
	GREEDY,
	IGNORE_CASE,
	WHOLE_WORD,
	WORD_START,
	WHOLE_LINE,
	IGNORE_HIDDEN_FILES,
	IGNORE_BINARY_FILES,
	IGNORE_HIDDEN_DIRS,
	SEARCH_RECURSIVE,
	REPLACE_REGEX,
	ACTIONS_NO_LIMIT,
	SEARCH_FULL_BUFFER,
	SEARCH_FORWARD,
	SEARCH_BACKWARD,
	SEARCH_BASIC,

	/* Combo boxes */
	SEARCH_STRING_COMBO,
	SEARCH_TARGET_COMBO,
	SEARCH_ACTION_COMBO,
	SEARCH_VAR_COMBO,
	MATCH_FILES_COMBO,
	UNMATCH_FILES_COMBO,
	MATCH_DIRS_COMBO,
	UNMATCH_DIRS_COMBO,
	REPLACE_STRING_COMBO,
	SEARCH_DIRECTION_COMBO,

	/* Treeview */
	SETTING_PREF_TREEVIEW
} GladeWidgetId;

void search_and_replace_init (IAnjutaDocumentManager* dm);
void search_and_replace (void);
void search_replace_next(void);
void search_replace_previous(void);
void search_replace_find_usage(const gchar *symbol);
void anjuta_search_replace_activate (gboolean replace, gboolean project);
GladeWidget *sr_get_gladewidget(GladeWidgetId id);
void search_replace_populate(void);
void search_update_dialog(void);

void search_toolbar_set_text(gchar *search_text);

#ifdef __cplusplus
}
#endif

#endif /* _SEARCH_REPLACE_H */
