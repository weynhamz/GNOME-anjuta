/*
 * preferences.h
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef _PREFERENCES_H_
#define _PREFERENCES_H_

#include <gnome.h>
#include <glade/glade.h>
#include "properties.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _Preferences Preferences;
typedef struct _PreferencesPriv PreferencesPriv;

struct _Preferences
{
	PropsID props_build_in;
	PropsID props_global;
	PropsID props_local;
	PropsID props_session;
	PropsID props;
	
	PreferencesPriv *priv;
};

/* Preferences */
Preferences *preferences_new (void);

/* Resets the default values into the keys */
void preferences_reset_defaults (Preferences *);

/* ----- */
void preferences_hide (Preferences *);
void preferences_show (Preferences *);
void preferences_destroy (Preferences *);

/* Save and Load */
gboolean preferences_save_yourself (Preferences * p, FILE * stream);
gboolean preferences_load_yourself (Preferences * p, PropsID pr);

/* Sets the value (string) of a key */
void preferences_set (Preferences *, gchar * key, gchar * value);

/* Sets the value (int) of a key */
void preferences_set_int (Preferences *, gchar * key, gint value);

/* Gets the value (string) of a key */
/* Must free the return string */
gchar *preferences_get (Preferences * p, gchar * key);

/* Gets the value (int) of a key. If not found, 0 is returned */
gint preferences_get_int (Preferences * p, gchar * key);

/* Gets the value (int) of a key. If not found, the default_value is returned */
gint preferences_get_int_with_default (Preferences * p, gchar * key, gint default_value);

gchar *preferences_default_get (Preferences * p, gchar * key);

/* Gets the value (int) of a key */
gint preferences_default_get_int (Preferences * p, gchar * key);

/*
 * Preferences KEY definitions.
 *
 * Use the keys instead of using the strings directly.
 *
 * Call these as the second arg of the
 * functions preferences_get_*() or preferences_set_*() to
 * read or write a preference value the preferences variables.
*/

#define PROJECTS_DIRECTORY         "projects.directory"
#define TARBALLS_DIRECTORY         "tarballs.directory"
#define RPMS_DIRECTORY             "rpms.directory"
#define SRPMS_DIRECTORY            "srpms.directory"
#define MAXIMUM_RECENT_PROJECTS    "maximum.recent.projects"
#define MAXIMUM_RECENT_FILES       "maximum.recent.files"
#define MAXIMUM_COMBO_HISTORY      "maximum.combo.history"
#define DIALOG_ON_BUILD_COMPLETE   "dialog.on.build.complete"
#define BEEP_ON_BUILD_COMPLETE     "beep.on.build.complete"
#define RELOAD_LAST_PROJECT        "reload.last.project"

#define BUILD_OPTION_KEEP_GOING    "build.option.keep.going"
#define BUILD_OPTION_DEBUG         "build.option.debug"
#define BUILD_OPTION_SILENT        "build.option.silent"
#define BUILD_OPTION_WARN_UNDEF    "build.option.warn.undef"
#define BUILD_OPTION_JOBS          "build.option.jobs"
#define BUILD_OPTION_AUTOSAVE      "build.option.autosave"

#define DISABLE_SYNTAX_HILIGHTING  "disable.syntax.hilighting"
#define SAVE_AUTOMATIC         "save.automatic"
#define INDENT_AUTOMATIC       "indent.automatic"
#define USE_TABS               "use.tabs"
#define BRACES_CHECK           "braces.check"
#define DOS_EOL_CHECK          "editor.doseol"
#define WRAP_BOOKMARKS         "editor.wrapbookmarks"
#define TAB_SIZE               "tabsize"
#define INDENT_SIZE            "indent.size"
#define INDENT_OPENING         "indent.opening"
#define INDENT_CLOSING         "indent.closing"
#define AUTOSAVE_TIMER         "autosave.timer"
#define MARGIN_LINENUMBER_WIDTH   "margin.linenumber.width"
#define SAVE_SESSION_TIMER        "save.session.timer"
#define AUTOFORMAT_DISABLE        "autoformat.disable"
#define AUTOFORMAT_CUSTOM_STYLE   "autoformat.custom.style"
#define AUTOFORMAT_STYLE       "autoformat.style"
#define EDITOR_TAG_POS         "editor.tag.pos"
#define EDITOR_TAG_HIDE        "editor.tag.hide"
#define EDITOR_TABS_ORDERING   "editor.tabs.ordering"
#define STRIP_TRAILING_SPACES  "strip.trailing.spaces"
#define FOLD_ON_OPEN           "fold.on.open"
#define CARET_FORE_COLOR       "caret.fore"
#define CALLTIP_BACK_COLOR     "calltip.back"
#define SELECTION_FORE_COLOR   "selection.fore"
#define SELECTION_BACK_COLOR   "selection.back"
#define TEXT_ZOOM_FACTOR       "text.zoom.factor"
#define TRUNCAT_MESSAGES       "truncat.messages"
#define TRUNCAT_MESG_FIRST     "truncat.mesg.first"
#define TRUNCAT_MESG_LAST      "truncat.mesg.last"
#define MESSAGES_TAG_POS       "messages.tag.position"

#define MESSAGES_COLOR_ERROR     "messages.color.error"
#define MESSAGES_COLOR_WARNING   "messages.color.warning"
#define MESSAGES_COLOR_MESSAGES1 "messages.color.messages1"
#define MESSAGES_COLOR_MESSAGES2 "messages.color.messages2"
#define MESSAGES_INDICATORS_AUTOMATIC "indicators.automatic"

#define AUTOMATIC_TAGS_UPDATE   "automatic.tags.update"
#define BUILD_SYMBOL_BROWSER	 "build.symbol.browser"
#define BUILD_FILE_BROWSER	 "build.file.browser"
#define SHOW_TOOLTIPS           "show.tooltips"

#define PRINT_PAPER_SIZE        "print.paper.size"
#define PRINT_HEADER            "print.header"
#define PRINT_WRAP              "print.linewrap"
#define PRINT_LINENUM_COUNT     "print.linenumber.count"
#define PRINT_LANDSCAPE         "print.landscape"
#define PRINT_MARGIN_LEFT       "print.margin.left"
#define PRINT_MARGIN_RIGHT      "print.margin.right"
#define PRINT_MARGIN_TOP        "print.margin.top"
#define PRINT_MARGIN_BOTTOM     "print.margin.bottom"
#define PRINT_COLOR             "print.color"

#define USE_COMPONENTS          "components.enable"

#define IDENT_NAME              "ident.name"
#define IDENT_EMAIL             "ident.email"

/* Miscellaneous */
#define CHARACTER_SET "character.set"

/* Terminal preferences */
#define TERMINAL_FONT			"terminal.font"
#define TERMINAL_SCROLLSIZE		"terminal.scrollsize"
#define TERMINAL_TERM			"terminal.term"
#define TERMINAL_WORDCLASS		"terminal.wordclass"
#define TERMINAL_BLINK			"terminal.blink"
#define TERMINAL_BELL			"terminal.bell"
#define TERMINAL_SCROLL_KEY		"terminal.scroll.keystroke"
#define TERMINAL_SCROLL_OUTPUT	"terminal.scroll.output"

/*
** Provide some reasonable failsafe values for the embedded
** terminal widget - Biswa
*/
#ifndef DEFAULT_ZVT_FONT
#define DEFAULT_ZVT_FONT "-adobe-courier-medium-r-normal-*-*-120-*-*-m-*-iso8859-1"
#endif

#ifndef DEFAULT_ZVT_SCROLLSIZE
#define DEFAULT_ZVT_SCROLLSIZE 200
#define MAX_ZVT_SCROLLSIZE 100000
#endif

#ifndef DEFAULT_ZVT_TERM
#define DEFAULT_ZVT_TERM "xterm"
#endif

#ifndef DEFAULT_ZVT_WORDCLASS
#define DEFAULT_ZVT_WORDCLASS "-A-Za-z0-9/_:.,?+%="
#endif

void
ColorFromString (const gchar * val, guint8 * r, guint8 * g, guint8 * b);

#ifdef __cplusplus
};
#endif

#endif
