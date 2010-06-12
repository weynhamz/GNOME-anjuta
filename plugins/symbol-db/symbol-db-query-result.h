/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
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

#ifndef _SYMBOL_DB_QUERY_RESULT_H_
#define _SYMBOL_DB_QUERY_RESULT_H_

#include <glib.h>
#include <glib-object.h>
#include <libgda/gda-data-model.h>

G_BEGIN_DECLS

#define SYMBOL_DB_TYPE_QUERY_RESULT             (sdb_query_result_get_type ())
#define SYMBOL_DB_QUERY_RESULT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SYMBOL_DB_TYPE_QUERY_RESULT, SymbolDBQueryResult))
#define SYMBOL_DB_QUERY_RESULT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), SYMBOL_DB_TYPE_QUERY_RESULT, SymbolDBQueryResultClass))
#define SYMBOL_DB_IS_QUERY_RESULT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SYMBOL_DB_TYPE_QUERY_RESULT))
#define SYMBOL_DB_IS_QUERY_RESULT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SYMBOL_DB_TYPE_QUERY_RESULT))
#define SYMBOL_DB_QUERY_RESULT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SYMBOL_DB_TYPE_QUERY_RESULT, SymbolDBQueryResultClass))
#define SYMBOL_DB_QUERY_RESULT_ERROR            (symbol_db_query_result_error_quark ())

typedef struct _SymbolDBQueryResultClass SymbolDBQueryResultClass;
typedef struct _SymbolDBQueryResult SymbolDBQueryResult;
typedef struct _SymbolDBQueryResultPriv SymbolDBQueryResultPriv;

typedef enum {
	SYMBOL_DB_QUERY_RESULT_ERROR_INVALID_FIELD,
	SYMBOL_DB_QUERY_RESULT_ERROR_FIELD_NOT_PRESENT
} SymbolDBQueryResultError;

struct _SymbolDBQueryResultClass
{
	GObjectClass parent_class;
};

struct _SymbolDBQueryResult
{
	GObject parent_instance;
	SymbolDBQueryResultPriv *priv;
};

GQuark symbol_db_query_result_error_quark (void);
GType sdb_query_result_get_type (void) G_GNUC_CONST;
SymbolDBQueryResult* symbol_db_query_result_new (GdaDataModel *data_model,
                                                 IAnjutaSymbolField *fields_order,
                                                 const GHashTable *sym_type_conversion_hash,
                                                 const gchar *project_root_dir);

gboolean symbol_db_query_result_is_empty (SymbolDBQueryResult *result);

G_END_DECLS

#endif /* _SYMBOL_DB_QUERY_RESULT_H_ */
