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
#include "properties.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _PreferencesWidgets PreferencesWidgets;
typedef struct _Preferences Preferences;
typedef struct _StyleData StyleData;

struct _StyleData
{
	gchar *item;
	gchar *font;
	gint size;
	gboolean bold, italics, underlined;
	gchar *fore, *back;
	gboolean eolfilled;

	gboolean font_use_default;
	gboolean attrib_use_default;
	gboolean fore_use_default;
	gboolean back_use_default;
};

struct _PreferencesWidgets
{
	GtkWidget *window;
	GtkWidget *notebook;

	/*
	 * * Page 0 
	 */
	GtkWidget *prj_dir_entry;
	GtkWidget *tarballs_dir_entry;
	GtkWidget *rpms_dir_entry;
	GtkWidget *srpms_dir_entry;
	GtkWidget *recent_prj_spin;
	GtkWidget *recent_files_spin;
	GtkWidget *combo_history_spin;
	GtkWidget *beep_check;
	GtkWidget *dialog_check;

	/*
	 * * Page 1 
	 */
	GtkWidget *build_keep_going_check;
	GtkWidget *build_silent_check;
	GtkWidget *build_debug_check;
	GtkWidget *build_warn_undef_check;
	GtkWidget *build_jobs_spin;

	/*
	 * * page2 
	 */
	GtkWidget *hilite_item_combo;
	GtkWidget *font_picker;
	GtkWidget *font_size_spin;
	GtkWidget *font_bold_check, *font_italics_check,
		*font_underlined_check;
	GtkWidget *fore_colorpicker;
	GtkWidget *back_colorpicker;
	GtkWidget *font_use_default_check;
	GtkWidget *font_attrib_use_default_check;
	GtkWidget *fore_color_use_default_check;
	GtkWidget *back_color_use_default_check;
	GtkWidget *caret_fore_color;
	GtkWidget *selection_fore_color;
	GtkWidget *selection_back_color;
	GtkWidget *calltip_back_color;

	/*
	 * * Page 3 
	 */
	GtkWidget *strip_spaces_check;
	GtkWidget *fold_on_open_check;

	GtkWidget *disable_hilite_check;
	GtkWidget *auto_save_check;
	GtkWidget *auto_indent_check;
	GtkWidget *use_tabs_check;
	GtkWidget *braces_check_check;
	GtkWidget *dos_eol_check;

	GtkWidget *tab_size_spin;
	GtkWidget *autosave_timer_spin;
	GtkWidget *autoindent_size_spin;
	GtkWidget *linenum_margin_width_spin;
	GtkWidget *session_timer_spin;

	/*
	 * * Page 4 
	 */
	GtkWidget *paperselector;
	GtkWidget *pr_command_combo;
	GtkWidget *pr_command_entry;

	/*
	 * * Page 5 
	 */
	GtkWidget *format_disable_check;
	GtkWidget *format_style_combo;
	GtkWidget *custom_style_entry;
	GtkWidget *format_frame1;
	GtkWidget *format_frame2;

	/*
	 * * Page 6 
	 */
	GtkWidget *truncat_mesg_check;
	GtkWidget *mesg_first_spin;
	GtkWidget *mesg_last_spin;
	GtkWidget *tag_pos_radio[4];
	GtkWidget *no_tag_check;
	GtkWidget *tags_update_check;
	/* Page Comps */
	GtkWidget *use_components;
};

struct _Preferences
{
	PreferencesWidgets widgets;

	PropsID props_build_in;
	PropsID props_global;
	PropsID props_local;
	PropsID props_session;
	PropsID props;

/*
 * Private 
 */
	StyleData *default_style;
	StyleData *current_style;

	gboolean is_showing;
	gint win_pos_x, win_pos_y;
};

/* Style data to be used in style editor */
StyleData *style_data_new (void);
StyleData *style_data_new_parse (gchar * style_string);
void style_data_destroy (StyleData * sdata);

/* Get the string version of the style */
gchar *style_data_get_string (StyleData * sdata);

/* Int and Boolean data can be set directly */
/* String data should use these functions */
void style_data_set_font (StyleData * sdata, gchar * font);
void style_data_set_fore (StyleData * sdata, gchar * fore);
void style_data_set_back (StyleData * sdata, gchar * fore);
void style_data_set_item (StyleData * sdata, gchar * fore);

/* Preferences */
Preferences *preferences_new (void);

/* Syncs the key values and the widgets */
void preferences_sync (Preferences * p);

/* Resets the default values into the keys */
void preferences_reset_defaults (Preferences *);

/* ----- */
void preferences_hide (Preferences *);
void preferences_show (Preferences *);
void preferences_destroy (Preferences *);
void create_preferences_gui (Preferences *);

void
on_hilite_style_entry_changed (GtkEditable * editable, gpointer user_data);

/* Save and Load */
gboolean preferences_save_yourself (Preferences * p, FILE * stream);
gboolean preferences_load_yourself (Preferences * p, PropsID pr);

/* Sets the value (string) of a key */
void preferences_set (Preferences *, gchar * key, gchar * value);

/* Sets the value (int) of a key */
void preferences_set_int (Preferences *, gchar * key, gint value);

/* Gets the indent options */
/* Must free the return string */
gchar *preferences_get_format_opts (Preferences * p);

/* Gets the value (string) of a key */
/* Must free the return string */
gchar *preferences_get (Preferences * p, gchar * key);

/* Gets the value (int) of a key */
gint preferences_get_int (Preferences * p, gchar * key);

gchar *preferences_default_get (Preferences * p, gchar * key);

/* Gets the value (int) of a key */
gint preferences_default_get_int (Preferences * p, gchar * key);

/* Sets the make options in the prop variable $(anjuta.make.options) */
void preferences_set_build_options(Preferences* p);

/*
 * Preferences KEY definitions.
 *
 * Use the keys instead of using the strings directly.
 *
 * Call these as the second arg of the
 * functions preferences_get(), preferences_get_int(),
 * preferences_set() and preferences_set_int() to
 * manupulate the preferences variables.
*/

#define PROJECTS_DIRECTORY "projects.directory"
#define TARBALLS_DIRECTORY "tarballs.directory"
#define RPMS_DIRECTORY "rpms.directory"
#define SRPMS_DIRECTORY "srpms.directory"
#define MAXIMUM_RECENT_PROJECTS "maximum.recent.projects"
#define MAXIMUM_RECENT_FILES "maximum.recent.files"
#define MAXIMUM_COMBO_HISTORY "maximum.combo.history"
#define DIALOG_ON_BUILD_COMPLETE "dialog.on.build.complete"
#define BEEP_ON_BUILD_COMPLETE "beep.on.build.complete"

#define BUILD_OPTION_KEEP_GOING "build.option.keep.going"
#define BUILD_OPTION_DEBUG "build.option.debug"
#define BUILD_OPTION_SILENT "build.option.silent"
#define BUILD_OPTION_WARN_UNDEF "build.option.warn.undef"
#define BUILD_OPTION_JOBS "build.option.jobs"

#define DISABLE_SYNTAX_HILIGHTING "disable.syntax.hilighting"
#define SAVE_AUTOMATIC "save.automatic"
#define INDENT_AUTOMATIC "indent.automatic"
#define USE_TABS "use.tabs"
#define BRACES_CHECK "braces.check"
#define DOS_EOL_CHECK "editor.doseol"
#define TAB_SIZE "tabsize"
#define INDENT_SIZE "indent.size"
#define AUTOSAVE_TIMER "autosave.timer"
#define MARGIN_LINENUMBER_WIDTH "margin.linenumber.width"
#define SAVE_SESSION_TIMER "save.session.timer"
#define AUTOFORMAT_DISABLE "autoformat.disable"
#define AUTOFORMAT_CUSTOM_STYLE "autoformat.custom.style"
#define AUTOFORMAT_STYLE "autoformat.style"
#define EDITOR_TAG_POS "editor.tag.pos"
#define EDITOR_TAG_HIDE "editor.tag.hide"
#define STRIP_TRAILING_SPACES "strip.trailing.spaces"
#define FOLD_ON_OPEN "fold.on.open"
#define CARET_FORE_COLOR "caret.fore"
#define CALLTIP_BACK_COLOR "calltip.back"
#define SELECTION_FORE_COLOR "selection.fore"
#define SELECTION_BACK_COLOR "selection.back"

#define TRUNCAT_MESSAGES "truncat.messages"
#define TRUNCAT_MESG_FIRST "truncat.mesg.first"
#define TRUNCAT_MESG_LAST "truncat.mesg.last"

#define AUTOMATIC_TAGS_UPDATE "automatic.tags.update"

#define COMMAND_PRINT "command.print"
#define USE_COMPONENTS "components.enable"

/* Miscellaneous */
#define CHARACTER_SET "character.set"

void
ColorFromString (const gchar * val, guint8 * r, guint8 * g, guint8 * b);

#ifdef __cplusplus
};
#endif

#endif
