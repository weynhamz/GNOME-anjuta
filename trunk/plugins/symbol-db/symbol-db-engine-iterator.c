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

#include <libanjuta/interfaces/ianjuta-iterable.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-debug.h>
#include "symbol-db-engine-iterator-node.h"
#include "symbol-db-engine-iterator.h"
#include "symbol-db-engine.h"

struct _SymbolDBEngineIteratorPriv
{
	GdaDataModel *data_model;
	GdaDataModelIter *data_iter;
	
	gint total_rows;
};

static SymbolDBEngineIteratorNodeClass* parent_class = NULL;

static void
sdb_engine_iterator_instance_init (SymbolDBEngineIterator *object)
{
	SymbolDBEngineIterator *sdbi;
	
	sdbi = SYMBOL_DB_ENGINE_ITERATOR (object);
	sdbi->priv = g_new0 (SymbolDBEngineIteratorPriv, 1);

	/* initialize some priv data */
	sdbi->priv->data_model = NULL;
	sdbi->priv->data_iter = NULL;
	sdbi->priv->total_rows = -1;	
}

static void
sdb_engine_iterator_finalize (GObject *object)
{
	SymbolDBEngineIterator *dbi;
	SymbolDBEngineIteratorPriv *priv;
	
	dbi = SYMBOL_DB_ENGINE_ITERATOR (object);	
	priv = dbi->priv;

	if (priv->data_model)
		g_object_unref (priv->data_model);
		
	g_free (priv);
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
sdb_engine_iterator_class_init (SymbolDBEngineIteratorClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	parent_class = g_type_class_ref (SYMBOL_TYPE_DB_ENGINE_ITERATOR_NODE);
	
	object_class->finalize = sdb_engine_iterator_finalize;
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
	
	/* because gda_data_model_get_n_rows () could be cpu-intensive, we'll 
	 * proxy this value, e.g. it's calculated if it is really needed */
	priv->total_rows = -1; 

	/* to avoid calling everytime this function when we want to use the IteratorNode,
	 * just do it now.
	 */
	symbol_db_engine_iterator_first (dbi);
	
	/* set the data_iter no the base class */
	symbol_db_engine_iterator_node_set_data (SYMBOL_DB_ENGINE_ITERATOR_NODE (dbi),
											 priv->data_iter);
	
	ianjuta_iterable_first (IANJUTA_ITERABLE (dbi), NULL);
	return dbi;
}

/**
 * Set the iterator at the first node.
 */
gboolean
symbol_db_engine_iterator_first (SymbolDBEngineIterator *dbi)
{
	SymbolDBEngineIteratorPriv *priv;
	
	g_return_val_if_fail (dbi != NULL, FALSE);
	priv = dbi->priv;
	
	return gda_data_model_iter_set_at_row (priv->data_iter, 0);
}


gboolean
symbol_db_engine_iterator_move_next (SymbolDBEngineIterator *dbi)
{
	SymbolDBEngineIteratorPriv *priv;
	
	g_return_val_if_fail (dbi != NULL, FALSE);
	priv = dbi->priv;

	return gda_data_model_iter_move_next (priv->data_iter);
}

gboolean
symbol_db_engine_iterator_move_prev (SymbolDBEngineIterator *dbi)
{
	SymbolDBEngineIteratorPriv *priv;
	
	g_return_val_if_fail (dbi != NULL, FALSE);
	priv = dbi->priv;
	
	return gda_data_model_iter_move_prev (priv->data_iter);
}

/**
 * @return -1 on error
 */
gint 
symbol_db_engine_iterator_get_n_items (SymbolDBEngineIterator *dbi)
{
	SymbolDBEngineIteratorPriv *priv;
	
	g_return_val_if_fail (dbi != NULL, -1);
	priv = dbi->priv;
	
	if (priv->total_rows < 0) 
	{
		/* try to get the number of rows */
		priv->total_rows = gda_data_model_get_n_rows (priv->data_model);
	}		
	
	return priv->total_rows;
}

/**
 * Set the iterator at the last node.
 */
gboolean
symbol_db_engine_iterator_last (SymbolDBEngineIterator *dbi)
{
	SymbolDBEngineIteratorPriv *priv;
	
	g_return_val_if_fail (dbi != NULL, FALSE);
	priv = dbi->priv;
	
	return gda_data_model_iter_set_at_row (priv->data_iter, 
					symbol_db_engine_iterator_get_n_items (dbi));
}

gboolean
symbol_db_engine_iterator_set_position (SymbolDBEngineIterator *dbi, gint pos)
{
	SymbolDBEngineIteratorPriv *priv;
	
	g_return_val_if_fail (dbi != NULL, FALSE);
	priv = dbi->priv;
	
	return gda_data_model_iter_set_at_row (priv->data_iter, pos);
}

gint
symbol_db_engine_iterator_get_position (SymbolDBEngineIterator *dbi)
{
	SymbolDBEngineIteratorPriv *priv;
	
	g_return_val_if_fail (dbi != NULL, FALSE);
	priv = dbi->priv;
	
	return gda_data_model_iter_get_row (priv->data_iter);
}


void
symbol_db_engine_iterator_foreach (SymbolDBEngineIterator *dbi, GFunc callback, 
								   gpointer user_data)
{
	SymbolDBEngineIteratorPriv *priv;
	
	g_return_if_fail (dbi != NULL);
	priv = dbi->priv;

	gint saved_pos = symbol_db_engine_iterator_get_position (dbi);
	
	symbol_db_engine_iterator_last (dbi);
	
	while (!symbol_db_engine_iterator_move_next (dbi))
		callback (dbi, user_data);
	symbol_db_engine_iterator_set_position (dbi, saved_pos);
}

/* IAnjutaIterable implementation */

static gboolean
isymbol_iter_first (IAnjutaIterable *iterable, GError **err)
{
	SymbolDBEngineIterator *dbi = SYMBOL_DB_ENGINE_ITERATOR (iterable);
	SymbolDBEngineIteratorPriv *priv;
	
	g_return_val_if_fail (dbi != NULL, FALSE);
	priv = dbi->priv;


	if (symbol_db_engine_iterator_first (dbi) == FALSE) 
	{
		symbol_db_engine_iterator_node_set_data (SYMBOL_DB_ENGINE_ITERATOR_NODE (iterable),
										 NULL);
		
		return FALSE;
	}
	symbol_db_engine_iterator_node_set_data (SYMBOL_DB_ENGINE_ITERATOR_NODE (iterable),
										 priv->data_iter);
	return TRUE;
}

static gboolean
isymbol_iter_next (IAnjutaIterable *iterable, GError **err)
{
	SymbolDBEngineIterator *dbi = SYMBOL_DB_ENGINE_ITERATOR (iterable);
	SymbolDBEngineIteratorPriv *priv;
	
	g_return_val_if_fail (dbi != NULL, FALSE);
	priv = dbi->priv;

	if (symbol_db_engine_iterator_move_next (dbi) == FALSE)
	{
		symbol_db_engine_iterator_node_set_data (SYMBOL_DB_ENGINE_ITERATOR_NODE (iterable),
										 NULL);
		
		return FALSE;
	}
	symbol_db_engine_iterator_node_set_data (SYMBOL_DB_ENGINE_ITERATOR_NODE (iterable),
										 priv->data_iter);
	
	return TRUE;
}

static gboolean
isymbol_iter_previous (IAnjutaIterable *iterable, GError **err)
{
	SymbolDBEngineIterator *dbi = SYMBOL_DB_ENGINE_ITERATOR (iterable);
	SymbolDBEngineIteratorPriv *priv;
	
	g_return_val_if_fail (dbi != NULL, FALSE);
	priv = dbi->priv;

	if (symbol_db_engine_iterator_move_prev (dbi) == FALSE)
	{
		symbol_db_engine_iterator_node_set_data (SYMBOL_DB_ENGINE_ITERATOR_NODE (iterable),
										 NULL);
		
		return FALSE;
	}
	symbol_db_engine_iterator_node_set_data (SYMBOL_DB_ENGINE_ITERATOR_NODE (iterable),
										 priv->data_iter);
	
	return TRUE;
}

static gboolean
isymbol_iter_last (IAnjutaIterable *iterable, GError **err)
{
	SymbolDBEngineIterator *dbi = SYMBOL_DB_ENGINE_ITERATOR (iterable);
	SymbolDBEngineIteratorPriv *priv;
	
	g_return_val_if_fail (dbi != NULL, FALSE);
	priv = dbi->priv;

	if (symbol_db_engine_iterator_last (dbi) == FALSE)
	{
		symbol_db_engine_iterator_node_set_data (SYMBOL_DB_ENGINE_ITERATOR_NODE (iterable),
										 NULL);
		
		return FALSE;
	}
	symbol_db_engine_iterator_node_set_data (SYMBOL_DB_ENGINE_ITERATOR_NODE (iterable),
										 priv->data_iter);
	
	return TRUE;
}

static void
isymbol_iter_foreach (IAnjutaIterable *iterable, GFunc callback,
					  gpointer user_data, GError **err)
{
	SymbolDBEngineIterator *dbi = SYMBOL_DB_ENGINE_ITERATOR (iterable);
	SymbolDBEngineIteratorPriv *priv;
	
	g_return_if_fail (dbi != NULL);
	priv = dbi->priv;

	symbol_db_engine_iterator_foreach (dbi, callback, user_data);
}

static gboolean
isymbol_iter_set_position (IAnjutaIterable *iterable,
						   gint position, GError **err)
{
	SymbolDBEngineIterator *dbi = SYMBOL_DB_ENGINE_ITERATOR (iterable);
	SymbolDBEngineIteratorPriv *priv;
	
	g_return_val_if_fail (dbi != NULL, FALSE);
	priv = dbi->priv;

	if (symbol_db_engine_iterator_set_position (dbi, position) == FALSE)
	{
		symbol_db_engine_iterator_node_set_data (SYMBOL_DB_ENGINE_ITERATOR_NODE (iterable),
										 NULL);
		
		return FALSE;
	}
	symbol_db_engine_iterator_node_set_data (SYMBOL_DB_ENGINE_ITERATOR_NODE (iterable),
										 priv->data_iter);
	
	return TRUE;	
}

static gint
isymbol_iter_get_position (IAnjutaIterable *iterable, GError **err)
{
	SymbolDBEngineIterator *dbi = SYMBOL_DB_ENGINE_ITERATOR (iterable);
	SymbolDBEngineIteratorPriv *priv;
	
	g_return_val_if_fail (dbi != NULL, FALSE);
	priv = dbi->priv;

	return symbol_db_engine_iterator_get_position (dbi);
}

static gint
isymbol_iter_get_length (IAnjutaIterable *iterable, GError **err)
{
	SymbolDBEngineIterator *dbi = SYMBOL_DB_ENGINE_ITERATOR (iterable);
	SymbolDBEngineIteratorPriv *priv;
	
	g_return_val_if_fail (dbi != NULL, FALSE);
	priv = dbi->priv;

	return symbol_db_engine_iterator_get_n_items (dbi);
}

static IAnjutaIterable *
isymbol_iter_clone (IAnjutaIterable *iter, GError **e)
{
	DEBUG_PRINT ("isymbol_iter_clone  () UNIMPLEMENTED");
	return NULL;
}

static void
isymbol_iter_assign (IAnjutaIterable *iter, IAnjutaIterable *src_iter, GError **e)
{
	DEBUG_PRINT ("isymbol_iter_assign () UNIMPLEMENTED");
}


static void
isymbol_iter_iface_init (IAnjutaIterableIface *iface, GError **err)
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



ANJUTA_TYPE_BEGIN (SymbolDBEngineIterator, sdb_engine_iterator, SYMBOL_TYPE_DB_ENGINE_ITERATOR_NODE);
ANJUTA_TYPE_ADD_INTERFACE (isymbol_iter, IANJUTA_TYPE_ITERABLE);
ANJUTA_TYPE_END;
