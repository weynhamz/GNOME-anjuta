/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/* 
    anjuta-utils.h
    Copyright (C) 2003 Naba Kumar  <naba@gnome.org>

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
#ifndef _ANJUTA_UTILS_H_
#define _ANJUTA_UTILS_H_

#include <glib.h>
#include <gtk/gtk.h>

gboolean anjuta_util_copy_file (gchar * src, gchar * dest, gboolean show_error);

void anjuta_util_color_from_string (const gchar * val, guint8 * r,
									guint8 * g, guint8 * b);

gchar* anjuta_util_string_from_color (guint8 r, guint8 g, guint8 b);

GtkWidget* anjuta_util_button_new_with_stock_image (const gchar* text,
													const gchar* stock_id);

GtkWidget* anjuta_util_dialog_add_button (GtkDialog *dialog, const gchar* text,
										  const gchar* stock_id,
										  gint response_id);

void anjuta_util_dialog_error (GtkWindow *parent, gchar * mesg, ...);
void anjuta_util_dialog_warning (GtkWindow *parent, gchar * mesg, ...);
void anjuta_util_dialog_info (GtkWindow *parent, gchar * mesg, ...);
void anjuta_util_dialog_error_system (GtkWindow* parent, gint errnum,
									  gchar * mesg, ... );
gboolean anjuta_util_dialog_boolean_question (GtkWindow *parent,
											  gchar * mesg, ...);

gboolean anjuta_util_prog_is_installed (gchar * prog, gboolean show);

gchar* anjuta_util_get_a_tmp_file (void);

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

/**********************************************************/
/* Both the returned glist and the data should be g_freed */
/* Call g_list_strings_free() to do that.                 */
/**********************************************************/
GList* anjuta_util_glist_from_data (guint props, const gchar* id);
GList* anjuta_util_glist_from_string (const gchar* id);
GList* anjuta_util_glist_strings_dup (GList * list);

/* Dedup a list of paths - duplicates are removed from the tail.
** Useful for deduping Recent Files and Recent Projects */
GList* anjuta_util_glist_path_dedup(GList *list);

/* Temporarily copied here */

#define ANJUTA_TYPE_BEGIN(class_name, prefix, parent_type) \
GType                                                     \
prefix##_get_type (void)                                  \
{                                                         \
  static GType type = 0;                                  \
  if (!type)                                              \
    {                                                     \
 static const GTypeInfo type_info =                       \
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
                                          #class_name,    \
                                          &type_info, 0);
#define ANJUTA_TYPE_END                                   \
     }                                                    \
  return type;                                            \
}

#define ANJUTA_INTERFACE(prefix,interface_type)                 \
{                                                               \
GInterfaceInfo iface_info = { (GInterfaceInitFunc)prefix##_iface_init, NULL, NULL };\
g_type_add_interface_static (type, interface_type,              \
                             &iface_info);                      \
}

#endif
