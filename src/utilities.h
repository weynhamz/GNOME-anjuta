/* 
    utilities.h
    Copyright (C) 2000  Kh. Naba Kumar Singh

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef _UTILITIES_H_
#define _UTILITIES_H_

#define FREE(x) if (x) g_free(x), x = NULL

#include <dirent.h>
#include <stdio.h>

#define COMBO_LIST_LENGTH \
		anjuta_preferences_get_int (app->preferences, MAXIMUM_COMBO_HISTORY)

#define _STR(S) ((S)?(S):"")

/* Any addition to this enum must be synced with the
function get_file_ext_type() */
enum _FileExtType
{
	FILE_TYPE_UNKNOWN,
	FILE_TYPE_DIR,
	FILE_TYPE_C,
	FILE_TYPE_CPP,
	FILE_TYPE_HEADER,
	FILE_TYPE_PASCAL,
	FILE_TYPE_RC,
	FILE_TYPE_IDL,
	FILE_TYPE_CS,
	FILE_TYPE_JAVA,
	FILE_TYPE_JS,
	FILE_TYPE_CONF,
	FILE_TYPE_HTML,
	FILE_TYPE_XML,
	FILE_TYPE_LATEX,
	FILE_TYPE_LUA,
	FILE_TYPE_PERL,
	FILE_TYPE_SH,
	FILE_TYPE_PYTHON,
	FILE_TYPE_RUBY,
	FILE_TYPE_PROPERTIES,
	FILE_TYPE_PROJECT,
	FILE_TYPE_BATCH,
	FILE_TYPE_ERRORLIST,
	FILE_TYPE_MAKEFILE,
	FILE_TYPE_IFACE,
	FILE_TYPE_DIFF,
	FILE_TYPE_ICON,
	FILE_TYPE_IMAGE,
	FILE_TYPE_ASM,
	FILE_TYPE_SCM,
	FILE_TYPE_PO,
	FILE_TYPE_SQL,
	FILE_TYPE_PLSQL,
	FILE_TYPE_VB,
	FILE_TYPE_BAAN,
	FILE_TYPE_ADA,
	FILE_TYPE_WSCRIPT,
	FILE_TYPE_LISP,
	FILE_TYPE_MATLAB,
	FILE_TYPE_END_MARK
};

typedef enum _FileExtType FileExtType;

/****************************************************************************/
/*  Functions that dynamic allocate memory. Return value(s) should be g_freed */
/****************************************************************************/

/* Removes while spaces in the text */
gchar* remove_white_spaces(gchar* text);

/* Gets the swapped (c/h) file names */
gchar* get_swapped_filename(gchar* filename);

/* Get a unique temporary filename */
/* This filename is not valid beyond the present session */
gchar* get_a_tmp_file(void);

/* Retruns the contents of the file a buffer */
gchar* get_file_as_buffer (gchar* filename);

/* Creates a space separated string from the list of strings */
gchar* string_from_glist (GList* list);

/* Returns true if the given line is parsable for */
/* file name and line number */
/* filename is set at (*filename), line number in lineno*/
/* Don't forget to g_free (*filename)*/
gboolean parse_error_line(const gchar *line, gchar **filename, int *lineno);

/***********************************************************************************/
/*  Functions that do not dynamic allocate memory. Return value should not be g_freed */
/***********************************************************************************/

/* Get a fixed font */
PangoFontDescription* get_fixed_font (void);

/* Rather tough */
gboolean read_string(FILE* stream, gchar* token, gchar **str);
gboolean read_line(FILE* stream, gchar **str);
gboolean write_string(FILE* stream, gchar* token, gchar *str);
gboolean write_line(FILE* stream, gchar *str);

/* Get the filename */
const gchar* extract_filename (const gchar* full_filename);

/* Get directory of file, return value must be g_freed */
gchar* extract_directory (const gchar* full_filename);

/* Adds the given string in the list, if it does not already exist. */
/* The added string will come at the top of the list */
/* The list will be then truncated to (length) items only */
GList* update_string_list(GList *p_list, const gchar *p_str, gint length);

/* Gets the extension of the file */
gchar* get_file_extension(gchar* file);

/* Gets the file extension type */
FileExtType get_file_ext_type(gchar* file);

/*
void fast_widget_repaint(GtkWidget* w);
*/

/* Comapares the two data */
/* Don't use this func. Define your own */
gint compare_string_func(gconstpointer a, gconstpointer b);

/* Checks if the file is of the given type */
gboolean file_is_regular(const gchar * fn);
gboolean file_is_directory(const gchar * fn);
gboolean file_is_link(const gchar * fn);
gboolean file_is_char_device(const gchar * fn);
gboolean file_is_block_device(const gchar * fn);
gboolean file_is_fifo(const gchar * fn);
gboolean file_is_socket(const gchar * fn);
gboolean file_is_readable(const gchar * fn);
gboolean file_is_readonly(const gchar * fn);
gboolean file_is_readwrite(const gchar * fn);
gboolean file_is_writable(const gchar * fn);
gboolean file_is_executable(const gchar * fn);
gboolean file_is_suid(const gchar * fn);
gboolean file_is_sgid(const gchar * fn);
gboolean file_is_sticky(const gchar * fn);

/* copies src to dest */
/* if (show_error) is true, then a dialog box will be shown for the error */
gboolean copy_file(gchar* src, gchar* dest, gboolean show_error);

/* bytewise compares two files and returns true for same or false for */
/* different or error */
/* if (show_error) is true, then a dialog box will be shown for the error */
gboolean compare_files(gchar* file1, gchar* file2, gboolean show_error);

/* (src) will be renamed, ie moved, to (dest) if they are not same */
/* Used to avoid unnecessary change in modified time of the dest file */
/* Which may result in recompilation, etc */
gboolean move_file_if_not_same (gchar* src, gchar* dest);

/* Check if two files/dirs/etc are the same one */
gboolean is_file_same(gchar *a, gchar *b);

/* Update, flush and paint all gtk widgets */
void update_gtk(void);

/* Sets a text (chars) in the entry (w) */
/* Text will be selected if use_selection is true */
void entry_set_text_n_select(GtkWidget* w, gchar* chars, gboolean use_selection);

/* Creates a dir, if it already doesnot exists */
/* All the required parent dirs are also created if they don't exist */
gboolean force_create_dir(const gchar* d);

/* Checks if the (parent) is parent widget of the (child)*/
gboolean widget_is_child(GtkWidget *parent, GtkWidget* child);

/* Fast string assignment to avoid memory leaks */
/* Assigns a new string (value), if not NULL, with g_strdup() to (*string) */
/* The previous string, if not NULL, is g_freed */
void string_assign (gchar** string, const gchar* value);

/* Returns true if the e->d_name is a regular file */
/* Used as the callback for scandir() */
int select_only_file (const struct dirent *e);

/***********************************************/
/*  Functions that operate on list of strings. */
/***********************************************/
void glist_strings_free(GList* list);
void glist_strings_prefix (GList * list, const gchar *prefix);
void glist_strings_sufix (GList * list, const gchar *sufix);
GList* glist_strings_sort (GList * list);

/**********************************************************/
/* Both the returned glist and the data should be g_freed */
/* Call g_list_strings_free() to do that.                 */
/**********************************************************/
GList* glist_from_data (guint props, const gchar* id);
GList* glist_from_string (const gchar* id);
GList* glist_strings_dup (GList * list);

/* Returns the alphabatically sorted list for files in dir */
/* select is the func that approves (returning 1)each file */
GList* scan_files_in_dir (const char *dir, int (*select)(const struct dirent *));

/*******************************************************************/
/* In this case only GList must be freed and not the data          */
/* Because output data are the input data. Only GList is allocated */
/*******************************************************************/
GList* remove_blank_lines(GList* lines); 

/* Create a new hbox with an image and a label packed into it
        * and return the box. */
GtkWidget*
create_xpm_label_box( GtkWidget *parent,
                                 const gchar     *xpm_filename, gboolean gnome_pixmap,
                                 const gchar     *label_text );
/* Excluding the final 0 */
gint calc_string_len( const gchar *szStr );
gint calc_gnum_len( void /*const gint iVal*/ );

/* Allocates a struct of pointers if sep = 0 use ',' */
gchar **string_parse_separator( const gint nItems, gchar *szStrIn, const gchar chSep /*= ','*/ );
#define	PARSE_STR(nItems,szStr)	string_parse_separator( nItems, szStr, ',' );
gchar* GetStrCod( const gchar *szIn );

/* Write in file....*/
gchar *WriteBufUL( gchar* szDst, const gulong ulVal);
gchar *WriteBufI( gchar* szDst, const gint iVal );
gchar *WriteBufB( gchar* szDst, const gboolean bVal );
gchar *WriteBufS( gchar* szDst, const gchar* szVal );

void
free_string_list ( GList * pList );

/********************************************************
 * Escapes single and double quotes in the given string *
 * Return string must be g_freed()                      *
 ********************************************************/
gchar* anjuta_util_escape_quotes(const gchar* str);

/********************************************************
 * Use this function instead of kill() to send a signal *
 * to the process which runs in separate process group, *
 * because normally kill() wouldn't work                *
 * Returns: status of the kill                          *
 ********************************************************/
gint  anjuta_util_kill(pid_t process_id, const gchar* signal_name);

/********************************************************
 * This function parses a string and returns a GList of *
 * program args, just like shell parses the program args*
 ********************************************************/
GList* anjuta_util_parse_args_from_string (const gchar* string);

/* Gets the relative filename w.r.t the given directory */
gchar *get_relative_file_name(gchar *dir, gchar *file);

/* Checks if the file exists in the given directory or any of it's subdirectories */
gboolean is_file_in_dir(const gchar *file, const gchar *dir);

GList *glist_path_dedup(GList *list);

/* Check which gnome-terminal is installed
 Returns: 0 -- No gnome-terminal
 Returns: 1 -- Gnome1 gnome-terminal
 Returns: 2 -- Gnome2 gnome-terminal */
gint anjuta_util_check_gnome_terminal (void);

/* String integer mapping utility functions */
typedef struct _StringMap
{
	int type;
	char *name;
} StringMap;

int type_from_string(StringMap *map, const char *str);
const char *string_from_type(StringMap *map, int type);
GList *glist_from_map(StringMap *map);

void anjuta_util_color_from_string (const gchar * val,
									guint8 * r, guint8 * g, guint8 * b);

gchar *anjuta_util_string_from_color (guint8 r, guint8 g, guint8 b);

/* Return must be freed. Works even if str is already UTF8 string */
gchar *anjuta_util_convert_to_utf8 (const gchar *str);

GtkWidget * anjuta_util_toolbar_append_button (GtkWidget *toolbar,
											   const gchar *iconfile,
											   const gchar *label,
											   const gchar *tooltip,
											   GtkSignalFunc callback,
											   gpointer user_data);
GtkWidget *
anjuta_util_toolbar_append_stock (GtkWidget *toolbar, const gchar *stock_icon,
								  const gchar *tooltip, GtkSignalFunc callback, 
								  gpointer user_data);

GtkWidget*
anjuta_dialog_add_button (GtkDialog *dialog, const gchar* text,
						  const gchar* stock_id, gint response_id);
GtkWidget* 
anjuta_button_new_with_stock_image (const gchar* text, const gchar* stock_id);

/* Equivalent to gnome_execute_shell(), but does not overwrite
 * SIGCHLD handler */
pid_t anjuta_execute_shell (const gchar *dir, const gchar *command);

#endif
