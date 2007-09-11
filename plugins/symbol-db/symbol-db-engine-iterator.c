/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Massimo Cora' 2007 <maxcvs@email.it>
 * 
 * anjuta is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * anjuta is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with anjuta.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include <libanjuta/anjuta-debug.h>
#include "symbol-db-engine-iterator.h"
#include "symbol-db-engine.h"

struct _SymbolDBEngineIteratorPriv
{
	GdaDataModel *data_model;
	GdaDataModelIter *data_iter;
	
	gint curr_row;
	gint total_rows;
};

static GObjectClass* parent_class = NULL;

static void
sdb_engine_iterator_init (SymbolDBEngineIterator *object)
{
	SymbolDBEngineIterator *sdbi;
	
	sdbi = SYMBOL_DB_ENGINE_ITERATOR (object);
	sdbi->priv = g_new0 (SymbolDBEngineIteratorPriv, 1);

	/* initialize some priv data */
	sdbi->priv->data_model = NULL;
	sdbi->priv->data_iter = NULL;
	sdbi->priv->curr_row = 0;
	sdbi->priv->total_rows = 0;	
}

static void
sdb_engine_iterator_finalize (GObject *object)
{
	/* TODO: Add deinitalization code here */
	SymbolDBEngineIterator *dbi;
	SymbolDBEngineIteratorPriv *priv;
	
	dbi = SYMBOL_DB_ENGINE_ITERATOR (object);	
	priv = dbi->priv;

	DEBUG_PRINT ("finalizing sdb_iterator");
	
	if (priv->data_model)
		g_object_unref (priv->data_model);
		
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
sdb_engine_iterator_class_init (SymbolDBEngineIteratorClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));

	object_class->finalize = sdb_engine_iterator_finalize;
}

GType
symbol_db_engine_iterator_get_type (void)
{
	static GType our_type = 0;

	if(our_type == 0)
	{
		static const GTypeInfo our_info =
		{
			sizeof (SymbolDBEngineIteratorClass), /* class_size */
			(GBaseInitFunc) NULL, /* base_init */
			(GBaseFinalizeFunc) NULL, /* base_finalize */
			(GClassInitFunc) sdb_engine_iterator_class_init, /* class_init */
			(GClassFinalizeFunc) NULL, /* class_finalize */
			NULL /* class_data */,
			sizeof (SymbolDBEngineIterator), /* instance_size */
			0, /* n_preallocs */
			(GInstanceInitFunc) sdb_engine_iterator_init, /* instance_init */
			NULL /* value_table */
		};

		our_type = g_type_register_static (G_TYPE_OBJECT, "SymbolDBEngineIterator",
		                                   &our_info, 0);
	}

	return our_type;
}

SymbolDBEngineIterator *
symbol_db_engine_iterator_new (GdaDataModel *model)
{
	SymbolDBEngineIterator *dbi;
	SymbolDBEngineIteratorPriv *priv;
	
	g_return_val_if_fail (model != NULL, NULL);
	
	dbi = g_object_new (SYMBOL_TYPE_DB_ENGINE_ITERATOR, NULL);
	
	priv = dbi->priv;
	
	priv->data_model = model;
	priv->data_iter = gda_data_model_iter_new (model);
	priv->total_rows = gda_data_model_get_n_rows (model);
	
	return dbi;
}

gboolean
symbol_db_engine_iterator_move_next (SymbolDBEngineIterator *dbi)
{
	SymbolDBEngineIteratorPriv *priv;
	
	g_return_val_if_fail (dbi != NULL, FALSE);
	priv = dbi->priv;

	if (priv->curr_row + 1 < priv->total_rows)
		priv->curr_row++;
	else 
		return FALSE;

	return gda_data_model_iter_move_next (priv->data_iter);
}

gboolean
symbol_db_engine_iterator_move_prev (SymbolDBEngineIterator *dbi)
{
	SymbolDBEngineIteratorPriv *priv;
	
	g_return_val_if_fail (dbi != NULL, FALSE);
	priv = dbi->priv;

	if (priv->curr_row > 0)
		priv->curr_row--;
	else
		return FALSE;
	
	return gda_data_model_iter_move_prev (priv->data_iter);
}

/* -1 on error */
gint 
symbol_db_engine_iterator_get_n_items (SymbolDBEngineIterator *dbi)
{
	SymbolDBEngineIteratorPriv *priv;
	
	g_return_val_if_fail (dbi != NULL, -1);
	priv = dbi->priv;
	
	return priv->total_rows;
}

/* 
 * Symbol name must be always on column 0
 */
gint
symbol_db_engine_iterator_get_symbol_id (SymbolDBEngineIterator *dbi)
{
	SymbolDBEngineIteratorPriv *priv;
	const GValue* value;
	
	g_return_val_if_fail (dbi != NULL, -1);
	priv = dbi->priv;
	
	value = gda_data_model_get_value_at (priv->data_model, 0, priv->curr_row);
	
	return value != NULL && G_VALUE_HOLDS_INT (value)
		? g_value_get_int (value) : -1;
}

/* 
 * Symbol name must be always on column 1
 */
const gchar* 
symbol_db_engine_iterator_get_symbol_name (SymbolDBEngineIterator *dbi)
{
	SymbolDBEngineIteratorPriv *priv;
	const GValue* value;
	
	g_return_val_if_fail (dbi != NULL, NULL);
	priv = dbi->priv;
	
	value = gda_data_model_get_value_at (priv->data_model, 1, priv->curr_row);
	
	return value != NULL && G_VALUE_HOLDS_STRING (value)
		? g_value_get_string (value) : NULL;
}

gint
symbol_db_engine_iterator_get_symbol_file_pos (SymbolDBEngineIterator *dbi)
{
	SymbolDBEngineIteratorPriv *priv;
	const GValue* value;
	
	g_return_val_if_fail (dbi != NULL, -1);
	priv = dbi->priv;
	
	value = gda_data_model_get_value_at (priv->data_model, 2, priv->curr_row);
	
	return value != NULL && G_VALUE_HOLDS_INT (value)
		? g_value_get_int (value) : -1;
}

gboolean
symbol_db_engine_iterator_get_symbol_is_file_scope (SymbolDBEngineIterator *dbi)
{
	SymbolDBEngineIteratorPriv *priv;
	const GValue* value;
	
	g_return_val_if_fail (dbi != NULL, -1);
	priv = dbi->priv;
	
	value = gda_data_model_get_value_at (priv->data_model, 3, priv->curr_row);
	
	if (value != NULL)
		return g_value_get_int (value) == 1 ? TRUE : FALSE;
	
	return FALSE;
}

const gchar* 
symbol_db_engine_iterator_get_symbol_signature (SymbolDBEngineIterator *dbi)
{
	SymbolDBEngineIteratorPriv *priv;
	const GValue* value;
	
	g_return_val_if_fail (dbi != NULL, NULL);
	priv = dbi->priv;
	
	value = gda_data_model_get_value_at (priv->data_model, 4, priv->curr_row);
	
	return value != NULL && G_VALUE_HOLDS_STRING (value)
		? g_value_get_string (value) : NULL;
}

const gchar*
symbol_db_engine_iterator_get_symbol_extra_string (SymbolDBEngineIterator *dbi,
												   gint sym_info)
{
	SymbolDBEngineIteratorPriv *priv;
	const GValue* value;
	
	g_return_val_if_fail (dbi != NULL, NULL);
	priv = dbi->priv;

	if (sym_info & SYMINFO_FILE_PATH)
	{
		value = gda_data_model_get_value_at_col_name (priv->data_model,
													 "file_path",
													 priv->curr_row);
		return value != NULL && G_VALUE_HOLDS_STRING (value)
			? g_value_get_string (value) : NULL;
	}
			
	if (sym_info & SYMINFO_LANGUAGE)
	{
		value = gda_data_model_get_value_at_col_name (priv->data_model,
													 "language_name",
													 priv->curr_row);
		return value != NULL && G_VALUE_HOLDS_STRING (value)
			? g_value_get_string (value) : NULL;		
	}
	
	if (sym_info & SYMINFO_IMPLEMENTATION)
	{
		value = gda_data_model_get_value_at_col_name (priv->data_model,
													 "implementation_name",
													 priv->curr_row);
		return value != NULL && G_VALUE_HOLDS_STRING (value)
			? g_value_get_string (value) : NULL;
	}
	
	if (sym_info & SYMINFO_ACCESS)
	{
		value = gda_data_model_get_value_at_col_name (priv->data_model,
													 "access_name",
													 priv->curr_row);
		return value != NULL && G_VALUE_HOLDS_STRING (value)
			? g_value_get_string (value) : NULL;
	}
	
	if (sym_info & SYMINFO_KIND)
	{
		value = gda_data_model_get_value_at_col_name (priv->data_model,
													 "kind_name",
													 priv->curr_row);
		return value != NULL && G_VALUE_HOLDS_STRING (value)
			? g_value_get_string (value) : NULL;
	}
	
	if (sym_info & SYMINFO_TYPE)
	{
		value =	gda_data_model_get_value_at_col_name (priv->data_model,
													 "type",
													 priv->curr_row);
		return value != NULL && G_VALUE_HOLDS_STRING (value)
			? g_value_get_string (value) : NULL;
	}
	
	if (sym_info & SYMINFO_TYPE_NAME)
	{
		value = gda_data_model_get_value_at_col_name (priv->data_model,
													 "type_name",
													 priv->curr_row);
		return value != NULL && G_VALUE_HOLDS_STRING (value)
			? g_value_get_string (value) : NULL;
	}

	if (sym_info & SYMINFO_PROJECT_NAME)
	{
		value = gda_data_model_get_value_at_col_name (priv->data_model,
													 "project_name",
													 priv->curr_row);
		return value != NULL && G_VALUE_HOLDS_STRING (value)
			? g_value_get_string (value) : NULL;
	}	

	if (sym_info & SYMINFO_FILE_IGNORE)
	{
		value =	gda_data_model_get_value_at_col_name (priv->data_model,
													 "file_ignore_type",
													 priv->curr_row);
		return value != NULL && G_VALUE_HOLDS_STRING (value)
			? g_value_get_string (value) : NULL;
	}

	if (sym_info & SYMINFO_FILE_INCLUDE)
	{
		value = gda_data_model_get_value_at_col_name (priv->data_model,
													 "file_include_type",
													 priv->curr_row);
		return value != NULL && G_VALUE_HOLDS_STRING (value)
			? g_value_get_string (value) : NULL;
	}

	/* not found */
	return NULL;
}




