/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-utils.h
 * Copyright (C) Naba Kumar  <naba@gnome.org>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef _ANJUTA_UTILS_H_
#define _ANJUTA_UTILS_H_

#include <glib.h>
#include <gtk/gtk.h>
#include <sys/types.h>
#include <gio/gio.h>

#include <libanjuta/anjuta-preferences.h>

G_BEGIN_DECLS

gboolean anjuta_util_copy_file (const gchar * src, const gchar * dest, gboolean show_error);

gboolean anjuta_util_diff(const gchar* uri, const gchar* text);

void anjuta_util_color_from_string (const gchar * val, guint16 * r,
									guint16 * g, guint16 * b);

gchar* anjuta_util_string_from_color (guint16 r, guint16 g, guint16 b);

GdkColor* anjuta_util_convert_color(AnjutaPreferences* prefs, const gchar* pref_name);

GtkWidget* anjuta_util_button_new_with_stock_image (const gchar* text,
													const gchar* stock_id);

GtkWidget* anjuta_util_dialog_add_button (GtkDialog *dialog, const gchar* text,
										  const gchar* stock_id,
										  gint response_id);

void anjuta_util_dialog_error (GtkWindow *parent, const gchar * mesg, ...);
void anjuta_util_dialog_warning (GtkWindow *parent, const gchar * mesg, ...);
void anjuta_util_dialog_info (GtkWindow *parent, const gchar * mesg, ...);
void anjuta_util_dialog_error_system (GtkWindow* parent, gint errnum,
									  const gchar * mesg, ... );
gboolean anjuta_util_dialog_boolean_question (GtkWindow *parent,
											  const gchar * mesg, ...);
gboolean anjuta_util_dialog_input (GtkWindow *parent, const gchar *label,
								   const gchar *default_value, gchar **value);

gboolean anjuta_util_prog_is_installed (const gchar * prog, gboolean show);

gchar* anjuta_util_get_a_tmp_file (void);

gchar* anjuta_util_convert_to_utf8 (const gchar *str);

GList* anjuta_util_parse_args_from_string (const gchar* string);

/***********************************************/
/* String integer mapping utility functions    */
/***********************************************/
typedef struct _AnjutaUtilStringMap
{
	int type;
	char *name;
} AnjutaUtilStringMap;

int anjuta_util_type_from_string(AnjutaUtilStringMap *map, const char *str);
const char *anjuta_util_string_from_type(AnjutaUtilStringMap *map, int type);
GList *anjuta_util_glist_from_map(AnjutaUtilStringMap *map);

/***********************************************/
/*  Functions that operate on list of strings. */
/***********************************************/
void anjuta_util_glist_strings_free(GList* list);
void anjuta_util_glist_strings_prefix (GList * list, const gchar *prefix);
void anjuta_util_glist_strings_sufix (GList * list, const gchar *sufix);
GList* anjuta_util_glist_strings_sort (GList * list);
gchar* anjuta_util_glist_strings_join (GList * list, gchar *delimiter);

/**********************************************************/
/* Both the returned glist and the data should be g_freed */
/* Call g_list_strings_free() to do that.                 */
/**********************************************************/
GList* anjuta_util_glist_from_string (const gchar* id);
GList* anjuta_util_glist_strings_dup (GList * list);

/* Dedup a list of paths - duplicates are removed from the tail.
** Useful for deduping Recent Files and Recent Projects */
GList* anjuta_util_glist_path_dedup(GList *list);

/* Adds the given string in the list, if it does not already exist. */
/* The added string will come at the top of the list */
/* The list will be then truncated to (length) items only */
GList * anjuta_util_update_string_list (GList *p_list, const gchar *p_str,
										gint length);

gboolean anjuta_util_create_dir (const gchar * d);
char * anjuta_util_user_shell (void);
pid_t anjuta_util_execute_shell (const gchar *dir, const gchar *command);

gchar* anjuta_util_escape_quotes(const gchar* str);

gchar* anjuta_util_get_real_path (const gchar *path);

gchar* anjuta_util_uri_get_dirname (const gchar *uri);
gchar* anjuta_util_replace_home_dir_with_tilde (const gchar *uri);
gchar* anjuta_util_shell_expand (const gchar *string);
gchar* anjuta_util_str_middle_truncate (const gchar *string,
										 guint        truncate_length);

gboolean anjuta_util_is_project_file (const gchar *filename);
gchar* anjuta_util_get_file_mime_type (GFile *file);
gchar* anjuta_util_get_local_path_from_uri (const gchar *uri);

void anjuta_util_help_display (GtkWidget   *parent,
							   const gchar *doc_id,
							   const gchar *file_name);

/* XDG BaseDir specifcation functions */
GFile* anjuta_util_get_user_data_file (const gchar* path, ...);
GFile* anjuta_util_get_user_cache_file (const gchar* path, ...);
GFile* anjuta_util_get_user_config_file (const gchar* path, ...);
gchar* anjuta_util_get_user_data_file_path (const gchar* path, ...);
gchar* anjuta_util_get_user_cache_file_path (const gchar* path, ...);
gchar* anjuta_util_get_user_config_file_path (const gchar* path, ...);

/* Function for converting GFile objects to string paths 
 * Free the returned list with anjuta_util_glist_strings_free. */
GList *anjuta_util_convert_gfile_list_to_path_list (GList *list);
GList *anjuta_util_convert_gfile_list_to_relative_path_list (GList *list, 
															 const gchar *parent);

/* Temporarily copied here */

#define ANJUTA_TYPE_BEGIN(class_name, prefix, parent_type) \
GType                                                     \
prefix##_get_type (void)                                  \
{                                                         \
  static GType type = 0;                                  \
  if (!type)                                              \
    {                                                     \
        static const GTypeInfo type_info =                \
        {                                                 \
          sizeof (class_name##Class),                     \
          (GBaseInitFunc) NULL,                           \
          (GBaseFinalizeFunc) NULL,                       \
          (GClassInitFunc) prefix##_class_init,           \
          (GClassFinalizeFunc) NULL,                      \
          NULL,                                           \
          sizeof (class_name),                            \
          0, /* n_preallocs */                            \
          (GInstanceInitFunc) prefix##_instance_init,     \
        };                                                \
                                                          \
        type = g_type_register_static (parent_type,       \
                                       #class_name,       \
                                       &type_info, 0);
#define ANJUTA_TYPE_END                                   \
     }                                                    \
  return type;                                            \
}

#define ANJUTA_TYPE_ADD_INTERFACE(prefix,interface_type)  \
    {                                                     \
        GInterfaceInfo iface_info = {                     \
            (GInterfaceInitFunc)prefix##_iface_init,      \
            NULL,                                         \
            NULL                                          \
        };                                                \
        g_type_add_interface_static (type,                \
                                     interface_type,      \
                                     &iface_info);        \
    }

#define ANJUTA_TYPE_BOILERPLATE(class_name, prefix, parent_type) \
ANJUTA_TYPE_BEGIN(class_name, prefix, parent_type);              \
ANJUTA_TYPE_END

G_END_DECLS

#endif
