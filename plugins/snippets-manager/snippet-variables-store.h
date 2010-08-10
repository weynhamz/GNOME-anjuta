/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippet-variables-store.h
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

#ifndef __SNIPPET_VARIABLES_STORE_H__
#define __SNIPPET_VARIABLES_STORE_H__

#include "snippet.h"
#include "snippets-db.h"
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _SnippetVarsStore SnippetVarsStore;
typedef struct _SnippetVarsStorePrivate SnippetVarsStorePrivate;
typedef struct _SnippetVarsStoreClass SnippetVarsStoreClass;

#define ANJUTA_TYPE_SNIPPET_VARS_STORE            (snippet_vars_store_get_type ())
#define ANJUTA_SNIPPET_VARS_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_SNIPPET_VARS_STORE, SnippetVarsStore))
#define ANJUTA_SNIPPET_VARS_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_SNIPPET_VARS_STORE, SnippetsVarsStoreClass))
#define ANJUTA_IS_SNIPPET_VARS_STORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_SNIPPET_VARS_STORE))
#define ANJUTA_IS_SNIPPET_VARS_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_SNIPPET_VARS_STORE))

typedef enum
{
	SNIPPET_VAR_TYPE_LOCAL = 0,
	SNIPPET_VAR_TYPE_GLOBAL,
	SNIPPET_VAR_TYPE_ANY
} SnippetVariableType;

/**
 * @VARS_STORE_COL_NAME: A #gchar * representing the name of the variable
 * @VARS_STORE_COL_TYPE: If the variable is global or local. See #SnippetVariableType.
 * @VARS_STORE_COL_DEFAULT_VALUE: The default value for a local or inserted global variable,
 *                                or an empty string.
 * @VARS_STORE_COL_INSTANT_VALUE: The instant value for a global variable or the default value
 *                                for a local variable.
 * @VARS_STORE_COL_IN_SNIPPET: A #gboolean. If TRUE then the variable is inserted in the snippet.
 *                             This is always TRUE for local variables.
 * @VARS_STORE_COL_UNDEFINED: If the variable type is global and the database doesn't store an entry
 *                            for it, this field is TRUE.
 */
enum
{
	VARS_STORE_COL_NAME = 0,
	VARS_STORE_COL_TYPE,
	VARS_STORE_COL_DEFAULT_VALUE,
	VARS_STORE_COL_INSTANT_VALUE,
	VARS_STORE_COL_IN_SNIPPET,
	VARS_STORE_COL_UNDEFINED,
	VARS_STORE_COL_N
};

struct _SnippetVarsStore
{
	GtkListStore parent;

	/*< private >*/
	SnippetVarsStorePrivate *priv;
};

struct _SnippetVarsStoreClass
{
	GtkListStoreClass parent_class;

};

GType               snippet_vars_store_get_type                     (void) G_GNUC_CONST;
SnippetVarsStore *  snippet_vars_store_new                          (void);

void                snippet_vars_store_load                         (SnippetVarsStore *vars_store,
                                                                     SnippetsDB *snippets_db,
                                                                     AnjutaSnippet *snippet);
void                snippet_vars_store_unload                       (SnippetVarsStore *vars_store);
void                snippet_vars_store_set_variable_name            (SnippetVarsStore *vars_store,
                                                                     const gchar *old_variable_name,
                                                                     const gchar *new_variable_name);
void                snippet_vars_store_set_variable_type            (SnippetVarsStore *vars_store,
                                                                     const gchar *variable_name,
                                                                     SnippetVariableType new_type);
void                snippet_vars_store_set_variable_default         (SnippetVarsStore *vars_store,
                                                                     const gchar *variable_name,
                                                                     const gchar *default_value);
void                snippet_vars_store_add_variable_to_snippet      (SnippetVarsStore *vars_store,
                                                                     const gchar *variable_name,
                                                                     gboolean get_global);
void                snippet_vars_store_remove_variable_from_snippet (SnippetVarsStore *vars_store,
                                                                     const gchar *variable_name);
G_END_DECLS

#endif /* __SNIPPET_VARIABLES_STORE_H__ */
