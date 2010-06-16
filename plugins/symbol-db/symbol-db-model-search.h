/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * symbol-db-model-file.h
 * Copyright (C) Naba Kumar 2010 <naba@gnome.org>
 * 
 * anjuta is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SYMBOL_DB_MODEL_SEARCH_H_
#define _SYMBOL_DB_MODEL_SEARCH_H_

#include <glib-object.h>
#include "symbol-db-model-project.h"

G_BEGIN_DECLS

#define SYMBOL_DB_TYPE_MODEL_SEARCH             (sdb_model_search_get_type ())
#define SYMBOL_DB_MODEL_SEARCH(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SYMBOL_DB_TYPE_MODEL_SEARCH, SymbolDBModelSearch))
#define SYMBOL_DB_MODEL_SEARCH_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), SYMBOL_DB_TYPE_MODEL_SEARCH, SymbolDBModelSearchClass))
#define SYMBOL_DB_IS_MODEL_SEARCH(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SYMBOL_DB_TYPE_MODEL_SEARCH))
#define SYMBOL_DB_IS_MODEL_SEARCH_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SYMBOL_DB_TYPE_MODEL_SEARCH))
#define SYMBOL_DB_MODEL_SEARCH_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SYMBOL_DB_TYPE_MODEL_SEARCH, SymbolDBModelSearchClass))

typedef struct _SymbolDBModelSearchClass SymbolDBModelSearchClass;
typedef struct _SymbolDBModelSearch SymbolDBModelSearch;
typedef struct _SymbolDBModelSearchPriv SymbolDBModelSearchPriv;

struct _SymbolDBModelSearchClass
{
	SymbolDBModelProjectClass parent_class;
};

struct _SymbolDBModelSearch
{
	SymbolDBModelProject parent_instance;

	SymbolDBModelSearchPriv *priv;
};

GType sdb_model_search_get_type (void) G_GNUC_CONST;
GtkTreeModel* symbol_db_model_search_new (SymbolDBEngine* dbe);

G_END_DECLS

#endif /* _SYMBOL_DB_MODEL_SEARCH_H_ */
