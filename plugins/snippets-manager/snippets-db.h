/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippets-db.h
    Copyright (C) Dragos Dena 2010

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, 
	Boston, MA  02110-1301  USA
*/

#ifndef __SNIPPETS_DB_H__
#define __SNIPPETS_DB_H__


#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include "snippet.h"
#include "snippets-group.h"
#include <libanjuta/anjuta-shell.h>

G_BEGIN_DECLS

typedef struct _SnippetsDB SnippetsDB;
typedef struct _SnippetsDBPrivate SnippetsDBPrivate;
typedef struct _SnippetsDBClass SnippetsDBClass;

#define ANJUTA_TYPE_SNIPPETS_DB            (snippets_db_get_type ())
#define ANJUTA_SNIPPETS_DB(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_SNIPPETS_DB, SnippetsDB))
#define ANJUTA_SNIPPETS_DB_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_SNIPPETS_DB, SnippetsDBClass))
#define ANJUTA_IS_SNIPPETS_DB(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_SNIPPETS_DB))
#define ANJUTA_IS_SNIPPETS_DB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_SNIPPETS_DB))

/**
 * @SNIPPETS_DB_MODEL_COL_CUR_OBJECT: An #AnjutaSnippet or #AnjutaSnippetsGroup object.
 * @SNIPPETS_DB_MODEL_COL_NAME: The name of the #AnjutaSnippet or #AnjutaSnippetsGroup.
 * @SNIPPETS_DB_MODEL_COL_TRIGGER: The trigger of the #AnjutaSnippet or "".
 * @SNIPPETS_DB_MODEL_COL_LANGUAGES: The supported languages of the #AnjutaSnippet or "".
 *
 * SnippetsDB Tree Model columns.
 *
 */
enum
{
	SNIPPETS_DB_MODEL_COL_CUR_OBJECT = 0,
	SNIPPETS_DB_MODEL_COL_NAME,
	SNIPPETS_DB_MODEL_COL_TRIGGER,
	SNIPPETS_DB_MODEL_COL_LANGUAGES,
	SNIPPETS_DB_MODEL_COL_N
};

enum
{
	GLOBAL_VARS_MODEL_COL_NAME = 0,
	GLOBAL_VARS_MODEL_COL_VALUE,
	GLOBAL_VARS_MODEL_COL_IS_COMMAND,
	GLOBAL_VARS_MODEL_COL_IS_INTERNAL,
	GLOBAL_VARS_MODEL_COL_N
};

struct _SnippetsDB
{
	GObject object;

	AnjutaShell* anjuta_shell;

	gint stamp;
	
	/*< private >*/
	SnippetsDBPrivate* priv;
};

struct _SnippetsDBClass
{
	GObjectClass klass;

};

typedef enum
{
	NATIVE_FORMAT = 0,
	GEDIT_FORMAT
} FormatType;


GType                      snippets_db_get_type               (void) G_GNUC_CONST;
SnippetsDB*                snippets_db_new                    (void);

void                       snippets_db_load                   (SnippetsDB *snippets_db);
void                       snippets_db_close                  (SnippetsDB *snippets_db);

GtkTreePath *              snippets_db_get_path_at_object     (SnippetsDB *snippets_db,
                                                               GObject *obj);
void                       snippets_db_save_snippets          (SnippetsDB *snippets_db);
void                       snippets_db_save_global_vars       (SnippetsDB *snippets_db);
void                       snippets_db_debug                  (SnippetsDB *snippets_db);

/* Snippet handling methods */
gboolean                   snippets_db_add_snippet            (SnippetsDB* snippets_db,
                                                               AnjutaSnippet* added_snippet,
                                                               const gchar* group_name);
gboolean                   snippets_db_has_snippet            (SnippetsDB *snippets_db,
                                                               AnjutaSnippet *snippet);
AnjutaSnippet*             snippets_db_get_snippet            (SnippetsDB* snippets_db,
                                                               const gchar* trigger_key,
                                                               const gchar* language);
gboolean                   snippets_db_remove_snippet         (SnippetsDB* snippets_db,
                                                               const gchar* trigger_key,
                                                               const gchar* language,
                                                               gboolean remove_all_languages_support);

/* SnippetsGroup handling methods */
gboolean                   snippets_db_add_snippets_group      (SnippetsDB* snippets_db,
                                                                AnjutaSnippetsGroup* snippets_group,
                                                                gboolean overwrite_group);
AnjutaSnippetsGroup*       snippets_db_get_snippets_group      (SnippetsDB* snippets_db,
                                                                const gchar* group_name);
gboolean                   snippets_db_remove_snippets_group   (SnippetsDB* snippets_db,
                                                                const gchar* group_name);
void                       snippets_db_set_snippets_group_name (SnippetsDB *snippets_db,
                                                                const gchar *old_group_name,
                                                                const gchar *new_group_name);
gboolean                   snippets_db_has_snippets_group_name (SnippetsDB *snippets_db,
                                                                const gchar *group_name);

/* Global variables handling methods */
gboolean                   snippets_db_add_global_variable       (SnippetsDB* snippets_db,
                                                                  const gchar* variable_name,
                                                                  const gchar* variable_value,
                                                                  gboolean variable_is_command,
                                                                  gboolean overwrite);
gboolean                   snippets_db_set_global_variable_name  (SnippetsDB* snippets_db,
                                                                  const gchar* variable_old_name,
                                                                  const gchar* variable_new_name);
gboolean                   snippets_db_set_global_variable_value (SnippetsDB* snippets_db,
                                                                  const gchar* variable_name,
                                                                  const gchar* variable_new_value);
gboolean                   snippets_db_set_global_variable_type  (SnippetsDB *snippets_db,
                                                                  const gchar* variable_name,
                                                                  gboolean is_command);                                                             
gchar*                     snippets_db_get_global_variable       (SnippetsDB* snippets_db,
                                                                  const gchar* variable_name);
gchar*                     snippets_db_get_global_variable_text  (SnippetsDB* snippets_db,
                                                                  const gchar* variable_name);
gboolean                   snippets_db_remove_global_variable    (SnippetsDB* snippets_db,
                                                                  const gchar* variable_name);
gboolean                   snippets_db_has_global_variable       (SnippetsDB* snippets_db,
                                                                  const gchar* variable_name);
GtkTreeModel*              snippets_db_get_global_vars_model     (SnippetsDB* snippes_db);

G_END_DECLS

#endif /* __SNIPPETS_DB_H__ */
