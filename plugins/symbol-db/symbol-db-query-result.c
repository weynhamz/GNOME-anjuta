/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Massimo Cora' 2007-2008 <maxcvs@email.it>
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

#include <libanjuta/interfaces/ianjuta-symbol.h>
#include <libanjuta/interfaces/ianjuta-iterable.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-debug.h>

#include <libgda/libgda.h>
#include "symbol-db-query-result.h"
#include "symbol-db-engine-utils.h"

#define SYMBOL_DB_QUERY_RESULT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
	SYMBOL_DB_TYPE_QUERY_RESULT, SymbolDBQueryResultPriv))

enum {
	PROP_SDB_0,
	PROP_SDB_COLUMNS,
	PROP_SDB_DATA_MODEL,
	PROP_SDB_DATA_ITER,
	PROP_SDB_SYM_TYPE_CONVERSION_HASH,
	PROP_SDB_PROJECT_ROOT
};

struct _SymbolDBQueryResultPriv
{
	gint *col_map;
	GdaDataModel *data_model;
	GdaDataModelIter *iter;
	const GHashTable *sym_type_conversion_hash;
	gchar *project_root;
	gboolean result_is_empty;
};

static void isymbol_iface_init (IAnjutaSymbolIface *iface);
static void isymbol_iter_iface_init (IAnjutaIterableIface *iface);

G_DEFINE_TYPE_WITH_CODE (SymbolDBQueryResult, sdb_query_result, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (IANJUTA_TYPE_SYMBOL,
                                                isymbol_iface_init)
                         G_IMPLEMENT_INTERFACE (IANJUTA_TYPE_ITERABLE,
                                                isymbol_iter_iface_init));
GQuark
symbol_db_query_result_error_quark (void)
{
	return g_quark_from_static_string ("symbol-db-query-return-error-quark");
}

static gboolean
sdb_query_result_validate_field (SymbolDBQueryResult *result,
                                 IAnjutaSymbolField field,
                                 GError **err)
{
	g_return_val_if_fail (err == NULL || *err == NULL, FALSE);
	
	if (field >= IANJUTA_SYMBOL_FIELD_END)
	{
		g_set_error (err, SYMBOL_DB_QUERY_RESULT_ERROR,
		             SYMBOL_DB_QUERY_RESULT_ERROR_INVALID_FIELD,
		             "Invalid symbol query field '%d'. It should be less than '%d'",
		             field, IANJUTA_SYMBOL_FIELD_END);
		g_warning ("Invalid symbol query field '%d'. It should be less than '%d'",
		           field, IANJUTA_SYMBOL_FIELD_END);
		return FALSE;
	}
	
	if (result->priv->col_map [field] == -1)
	{
		g_set_error (err, SYMBOL_DB_QUERY_RESULT_ERROR,
		             SYMBOL_DB_QUERY_RESULT_ERROR_FIELD_NOT_PRESENT,
		             "Symbol field '%d' is present in the query. Make sure to "
		             "include the during query creation.", field);
		g_warning ("Symbol field '%d' is present in the query. Make sure to "
		             "include the during query creation.", field);
		return FALSE;
	}
	return TRUE;
}

static void
sdb_query_result_init (SymbolDBQueryResult *result)
{
	gint i;
	
	result->priv = SYMBOL_DB_QUERY_RESULT_GET_PRIVATE (result);
	result->priv->col_map = g_new (gint, IANJUTA_SYMBOL_FIELD_END);
	for (i = 0; i < IANJUTA_SYMBOL_FIELD_END; i++)
		result->priv->col_map[i] = -1;
	result->priv->result_is_empty = TRUE;
}

static void
sdb_query_result_dispose (GObject *object)
{
	SymbolDBQueryResult *result = SYMBOL_DB_QUERY_RESULT (object);
	if (result->priv->data_model)
	{
		g_object_unref (result->priv->data_model);
		result->priv->data_model = NULL;
	}
	if (result->priv->iter)
	{
		g_object_unref (result->priv->iter);
		result->priv->iter = NULL;
	}
	G_OBJECT_CLASS (sdb_query_result_parent_class)->dispose (object);
}

static void
sdb_query_result_finalize (GObject *object)
{
	SymbolDBQueryResult *result = SYMBOL_DB_QUERY_RESULT (object);

	g_free (result->priv->project_root);
	g_free (result->priv->col_map);
	G_OBJECT_CLASS (sdb_query_result_parent_class)->finalize (object);
}

static void
sdb_query_result_set_property (GObject *object, guint prop_id,
                               const GValue *value, GParamSpec *pspec)
{
	gint i;
	GdaDataModel *data_model;
	IAnjutaSymbolField *cols_order;
	SymbolDBQueryResultPriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_QUERY_RESULT (object));
	priv = SYMBOL_DB_QUERY_RESULT (object)->priv;
	
	switch (prop_id)
	{
	case PROP_SDB_COLUMNS:
		for (i = 0; i < IANJUTA_SYMBOL_FIELD_END; i++)
			priv->col_map[i] = -1;
		i = 0;
		cols_order = g_value_get_pointer (value);
		while (!(*cols_order == IANJUTA_SYMBOL_FIELD_END))
		{
			priv->col_map[*cols_order] = i;
			i++;
			cols_order++;
		}
		break;
	case PROP_SDB_DATA_MODEL:
		priv->result_is_empty = TRUE;
		data_model = GDA_DATA_MODEL (g_value_get_object (value));
		if (priv->data_model) g_object_unref (priv->data_model);
		priv->data_model = data_model;
		if (priv->iter)
			g_object_unref (priv->iter);
		priv->iter = gda_data_model_create_iter (data_model);
		if (gda_data_model_iter_move_at_row (priv->iter, 0))
			priv->result_is_empty = FALSE;
		break;
	case PROP_SDB_SYM_TYPE_CONVERSION_HASH:
		priv->sym_type_conversion_hash = g_value_get_pointer (value);
		break;
	case PROP_SDB_PROJECT_ROOT:
		g_free (priv->project_root);
		priv->project_root = g_value_dup_string (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
sdb_query_result_get_property (GObject *object, guint prop_id,
                               GValue *value, GParamSpec *pspec)
{
	SymbolDBQueryResultPriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_QUERY_RESULT (object));
	priv = SYMBOL_DB_QUERY_RESULT (object)->priv;
	
	switch (prop_id)
	{
	case PROP_SDB_DATA_ITER:
		g_value_set_object (value, priv->iter);
		break;
	case PROP_SDB_SYM_TYPE_CONVERSION_HASH:
		g_value_set_pointer (value, (gpointer) priv->sym_type_conversion_hash);
		break;
	case PROP_SDB_PROJECT_ROOT:
		g_value_set_string (value, priv->project_root);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
sdb_query_result_class_init (SymbolDBQueryResultClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (SymbolDBQueryResultPriv));
	object_class->set_property = sdb_query_result_set_property;
	object_class->get_property = sdb_query_result_get_property;
	object_class->finalize = sdb_query_result_finalize;
	object_class->dispose = sdb_query_result_dispose;

	g_object_class_install_property
		(object_class, PROP_SDB_COLUMNS,
		 g_param_spec_pointer ("fields-order",
		                      "Fields order",
		                      "List of data fields in the order found in data model terminated by end field",
		                      G_PARAM_WRITABLE |
		                      G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property
		(object_class, PROP_SDB_DATA_MODEL,
		 g_param_spec_object ("data-model",
		                      "a GdaDataModel",
		                      "GdaDataModel of the result set",
		                      GDA_TYPE_DATA_MODEL,
		                      G_PARAM_WRITABLE |
		                      G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property
		(object_class, PROP_SDB_DATA_ITER,
		 g_param_spec_object ("data-iter",
		                      "a GdaDataModelIter",
		                      "GdaDataModelIter on the result set",
		                      GDA_TYPE_DATA_MODEL_ITER,
		                      G_PARAM_READABLE));
	g_object_class_install_property
		(object_class, PROP_SDB_SYM_TYPE_CONVERSION_HASH,
		 g_param_spec_pointer ("sym-type-conversion-hash",
		                       "Symbol type conversoin hash table",
		                       "A hash table to convert string form of symbol type to enum value (fixme)",
		                       G_PARAM_READABLE | G_PARAM_WRITABLE |
		                       G_PARAM_CONSTRUCT));
	g_object_class_install_property
		(object_class, PROP_SDB_PROJECT_ROOT,
		 g_param_spec_string ("project-root",
		                      "Project root directory",
		                      "The project root directory",
		                      NULL,
		                      G_PARAM_READABLE | G_PARAM_WRITABLE |
		                      G_PARAM_CONSTRUCT));
}

/* IAnjutaSymbol implementation */
static gboolean
isymbol_get_boolean (IAnjutaSymbol *isymbol, IAnjutaSymbolField field,
                     GError **err)
{
	SymbolDBQueryResult *result;
	const GValue *val;
	gint col;

	g_return_val_if_fail (SYMBOL_DB_IS_QUERY_RESULT (isymbol), FALSE);

	result = SYMBOL_DB_QUERY_RESULT (isymbol);
	if (!sdb_query_result_validate_field (result, field, err)) return FALSE;
	
	col = result->priv->col_map[field];
	val = gda_data_model_iter_get_value_at (result->priv->iter, col);
	return g_value_get_boolean (val);
}

static gboolean
isymbol_get_int (IAnjutaSymbol *isymbol, IAnjutaSymbolField field,
                 GError **err)
{
	SymbolDBQueryResult *result;
	const GValue *val;
	gint col;
	
	g_return_val_if_fail (SYMBOL_DB_IS_QUERY_RESULT (isymbol), -1);

	result = SYMBOL_DB_QUERY_RESULT (isymbol);
	if (!sdb_query_result_validate_field (result, field, err)) return FALSE;

	col = result->priv->col_map[field];
	val = gda_data_model_iter_get_value_at (result->priv->iter, col);
	if (!val) return 0;
	if (field == IANJUTA_SYMBOL_FIELD_TYPE)
	{
		const gchar* type_str = g_value_get_string (val);
		gint type_val = 
			(gint)g_hash_table_lookup ((GHashTable*)result->priv->sym_type_conversion_hash, 
			                           type_str);
		return type_val;
	}
	return g_value_get_int (val);
}

static const gchar*
isymbol_get_string (IAnjutaSymbol *isymbol, IAnjutaSymbolField field,
                    GError **err)
{
	SymbolDBQueryResult *result;
	const GValue *val;
	gint col;

	g_return_val_if_fail (SYMBOL_DB_IS_QUERY_RESULT (isymbol), NULL);

	result = SYMBOL_DB_QUERY_RESULT (isymbol);
	if (!sdb_query_result_validate_field (result, field, err)) return FALSE;

	col = result->priv->col_map[field];
	val = gda_data_model_iter_get_value_at (result->priv->iter, col);
	if (!val) return NULL;
	if (!G_VALUE_HOLDS_STRING (val)) return NULL;
	return g_value_get_string (val);
}

static IAnjutaSymbolType 
isymbol_get_sym_type (IAnjutaSymbol *isymbol, GError **err)
{
	return (IAnjutaSymbolType)isymbol_get_int (isymbol, IANJUTA_SYMBOL_FIELD_TYPE, err);
}

static GFile*
isymbol_get_file (IAnjutaSymbol *isymbol, GError **err)
{
	SymbolDBQueryResult *result;
	const gchar* file_path;
	gchar *abs_file_path;
	GFile *file;
	
	g_return_val_if_fail (SYMBOL_DB_IS_QUERY_RESULT (isymbol), NULL);

	result = SYMBOL_DB_QUERY_RESULT (isymbol);
	
	file_path = isymbol_get_string (isymbol, IANJUTA_SYMBOL_FIELD_FILE_PATH, err);
	if (!file_path)
		return NULL;
	abs_file_path = g_build_filename (result->priv->project_root, file_path, NULL);
	file = g_file_new_for_path (abs_file_path);
	g_free (abs_file_path);
	return file;
}

static const GdkPixbuf*
isymbol_get_icon (IAnjutaSymbol *isymbol, GError **err)
{
	return symbol_db_util_get_pixbuf
		(isymbol_get_string (isymbol, IANJUTA_SYMBOL_FIELD_TYPE, NULL),
		 isymbol_get_string (isymbol, IANJUTA_SYMBOL_FIELD_ACCESS, NULL));
}

static void
isymbol_iface_init (IAnjutaSymbolIface *iface)
{
	iface->get_boolean = isymbol_get_boolean;
	iface->get_int = isymbol_get_int;
	iface->get_string = isymbol_get_string;
	iface->get_sym_type = isymbol_get_sym_type;
	iface->get_file = isymbol_get_file;
	iface->get_icon = isymbol_get_icon;
}

/* IAnjutaIterable implementation */

static gboolean
isymbol_iter_first (IAnjutaIterable *iterable, GError **err)
{
	SymbolDBQueryResult *result;

	g_return_val_if_fail (SYMBOL_DB_IS_QUERY_RESULT (iterable), FALSE);
	result = SYMBOL_DB_QUERY_RESULT (iterable);	
	return gda_data_model_iter_move_to_row (result->priv->iter, 0);
}

static gboolean
isymbol_iter_next (IAnjutaIterable *iterable, GError **err)
{
	SymbolDBQueryResult *result;

	g_return_val_if_fail (SYMBOL_DB_IS_QUERY_RESULT (iterable), FALSE);
	result = SYMBOL_DB_QUERY_RESULT (iterable);	
	return gda_data_model_iter_move_next (result->priv->iter);
}

static gboolean
isymbol_iter_previous (IAnjutaIterable *iterable, GError **err)
{
	SymbolDBQueryResult *result;

	g_return_val_if_fail (SYMBOL_DB_IS_QUERY_RESULT (iterable), FALSE);
	result = SYMBOL_DB_QUERY_RESULT (iterable);	
	return gda_data_model_iter_move_prev (result->priv->iter);
}

static gboolean
isymbol_iter_last (IAnjutaIterable *iterable, GError **err)
{
	GdaDataModel *data_model;
	gint len;
	SymbolDBQueryResult *result;

	g_return_val_if_fail (SYMBOL_DB_IS_QUERY_RESULT (iterable), FALSE);
	result = SYMBOL_DB_QUERY_RESULT (iterable);	
	g_object_get (G_OBJECT (result->priv->iter), "data-model", &data_model, NULL);
	len = gda_data_model_get_n_rows (data_model);
	g_object_unref (data_model);
	if (len <= 0)
		return FALSE;
	return gda_data_model_iter_move_to_row (result->priv->iter, len - 1);
}

static void
isymbol_iter_foreach (IAnjutaIterable *iterable, GFunc callback,
					  gpointer user_data, GError **err)
{
	gint current;
	SymbolDBQueryResult *result;

	g_return_if_fail (SYMBOL_DB_IS_QUERY_RESULT (iterable));
	result = SYMBOL_DB_QUERY_RESULT (iterable);
	current = gda_data_model_iter_get_row (result->priv->iter);
	if (!gda_data_model_iter_move_to_row (result->priv->iter, 0))
		return;
	do
	{
		callback (iterable, user_data);
	}
	while (gda_data_model_iter_move_next (result->priv->iter));
	gda_data_model_iter_move_to_row (result->priv->iter, current);
}

static gboolean
isymbol_iter_set_position (IAnjutaIterable *iterable,
						   gint position, GError **err)
{
	SymbolDBQueryResult *result;

	g_return_val_if_fail (SYMBOL_DB_IS_QUERY_RESULT (iterable), FALSE);
	result = SYMBOL_DB_QUERY_RESULT (iterable);	
	return gda_data_model_iter_move_to_row (result->priv->iter, position);
}

static gint
isymbol_iter_get_position (IAnjutaIterable *iterable, GError **err)
{
	SymbolDBQueryResult *result;

	g_return_val_if_fail (SYMBOL_DB_IS_QUERY_RESULT (iterable), FALSE);
	result = SYMBOL_DB_QUERY_RESULT (iterable);	
	return gda_data_model_iter_get_row (result->priv->iter);
}

static gint
isymbol_iter_get_length (IAnjutaIterable *iterable, GError **err)
{
	SymbolDBQueryResult *result;

	g_return_val_if_fail (SYMBOL_DB_IS_QUERY_RESULT (iterable), FALSE);
	result = SYMBOL_DB_QUERY_RESULT (iterable);	
	return gda_data_model_get_n_rows (result->priv->data_model);
}

static IAnjutaIterable *
isymbol_iter_clone (IAnjutaIterable *iter, GError **e)
{
	DEBUG_PRINT ("%s", "isymbol_iter_clone  () UNIMPLEMENTED");
	return NULL;
}

static void
isymbol_iter_assign (IAnjutaIterable *iter, IAnjutaIterable *src_iter, GError **e)
{
	DEBUG_PRINT ("%s", "isymbol_iter_assign () UNIMPLEMENTED");
}

static void
isymbol_iter_iface_init (IAnjutaIterableIface *iface)
{
	iface->first = isymbol_iter_first;
	iface->next = isymbol_iter_next;
	iface->previous = isymbol_iter_previous;
	iface->last = isymbol_iter_last;
	iface->foreach = isymbol_iter_foreach;
	iface->set_position = isymbol_iter_set_position;
	iface->get_position = isymbol_iter_get_position;
	iface->get_length = isymbol_iter_get_length;
	iface->clone = isymbol_iter_clone;
	iface->assign = isymbol_iter_assign;	
}

SymbolDBQueryResult*
symbol_db_query_result_new (GdaDataModel *data_model,
                            IAnjutaSymbolField *fields_order,
                            const GHashTable *sym_type_conversion_hash,
                            const gchar *project_root_dir)
{
	return g_object_new (SYMBOL_DB_TYPE_QUERY_RESULT,
	                     "data-model", data_model,
	                     "fields-order", fields_order,
	                     "sym-type-conversion-hash", sym_type_conversion_hash,
	                     "project-root", project_root_dir,
	                     NULL);
}

gboolean
symbol_db_query_result_is_empty (SymbolDBQueryResult *result)
{
	return result->priv->result_is_empty;
}
