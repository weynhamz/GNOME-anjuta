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

#include <libanjuta/interfaces/ianjuta-symbol.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-debug.h>

#include "symbol-db-engine-iterator-node.h"
#include "symbol-db-engine.h"
#include "symbol-db-view.h"

struct _SymbolDBEngineIteratorNodePriv
{
	GdaDataModelIter *data_iter;	
	const GHashTable *sym_type_conversion_hash;
};

static GObjectClass* parent_class = NULL;


SymbolDBEngineIteratorNode *
symbol_db_engine_iterator_node_new (const GdaDataModelIter *data)
{	
/*	DEBUG_PRINT ("sdb_engine_iterator_node_new ()");*/
	SymbolDBEngineIteratorNode *s;
	s = g_object_new (SYMBOL_TYPE_DB_ENGINE_ITERATOR_NODE, NULL);
	s->priv->data_iter = (GdaDataModelIter *)data;
	
	return s;
}

static void
sdb_engine_iterator_node_instance_init (SymbolDBEngineIteratorNode *object)
{
	SymbolDBEngineIteratorNode *sdbin;
/*	DEBUG_PRINT ("sdb_engine_iterator_node_instance_init ()");*/
	
	sdbin = SYMBOL_DB_ENGINE_ITERATOR_NODE (object);
	
	sdbin->priv = g_new0 (SymbolDBEngineIteratorNodePriv, 1);
	sdbin->priv->sym_type_conversion_hash = NULL;
}

static void
sdb_engine_iterator_node_finalize (GObject *object)
{
	/* TODO: Add deinitalization code here */
	SymbolDBEngineIteratorNode *dbin;
	SymbolDBEngineIteratorNodePriv *priv;
	
/*	DEBUG_PRINT ("sdb_engine_iterator_node_finalize ()");*/
	
	dbin = SYMBOL_DB_ENGINE_ITERATOR_NODE (object);	
	priv = dbin->priv;

	g_free (priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
sdb_engine_iterator_node_class_init (SymbolDBEngineIteratorNodeClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));

	object_class->finalize = sdb_engine_iterator_node_finalize;
}

/** 
 * Symbol name must be always on column 0
 */
gint
symbol_db_engine_iterator_node_get_symbol_id (SymbolDBEngineIteratorNode *dbin)
{
	SymbolDBEngineIteratorNodePriv *priv;
	const GValue* value;
	GdaParameter *par;
	
	g_return_val_if_fail (dbin != NULL, -1);
	priv = dbin->priv;
	
	par = gda_data_model_iter_get_param_for_column (priv->data_iter, 0);
	value = gda_parameter_get_value (par);	
	
	return value != NULL && G_VALUE_HOLDS_INT (value)
		? g_value_get_int (value) : -1;
}

/**
 * Symbol name must be always on column 1
 */
const gchar* 
symbol_db_engine_iterator_node_get_symbol_name (SymbolDBEngineIteratorNode *dbin)
{
	SymbolDBEngineIteratorNodePriv *priv;
	const GValue* value;
	GdaParameter *par;
	
	g_return_val_if_fail (dbin != NULL, NULL);
	priv = dbin->priv;
	
	par = gda_data_model_iter_get_param_for_column (priv->data_iter, 1);
	value = gda_parameter_get_value (par);	
	
	return value != NULL && G_VALUE_HOLDS_STRING (value)
		? g_value_get_string (value) : NULL;
}

/**
 * File pos must be always on column 2
 */
gint
symbol_db_engine_iterator_node_get_symbol_file_pos (SymbolDBEngineIteratorNode *dbin)
{
	SymbolDBEngineIteratorNodePriv *priv;
	const GValue* value;
	GdaParameter *par;
	
	g_return_val_if_fail (dbin != NULL, -1);
	priv = dbin->priv;
	
	par = gda_data_model_iter_get_param_for_column (priv->data_iter, 2);
	value = gda_parameter_get_value (par);	
	
	return value != NULL && G_VALUE_HOLDS_INT (value)
		? g_value_get_int (value) : -1;
}

/**
 * 'Is File Scope' must be always on column 3
 */
gboolean
symbol_db_engine_iterator_node_get_symbol_is_file_scope (SymbolDBEngineIteratorNode *dbin)
{
	SymbolDBEngineIteratorNodePriv *priv;
	const GValue* value;
	
	GdaParameter *par;
	
	g_return_val_if_fail (dbin != NULL, FALSE);
	priv = dbin->priv;
	
	par = gda_data_model_iter_get_param_for_column (priv->data_iter, 3);
	value = gda_parameter_get_value (par);	
	
	if (value != NULL)
		return g_value_get_int (value) == 1 ? TRUE : FALSE;
	
	return FALSE;
}

/**
 * Signature must be always on column 4
 */
const gchar* 
symbol_db_engine_iterator_node_get_symbol_signature (SymbolDBEngineIteratorNode *dbin)
{
	SymbolDBEngineIteratorNodePriv *priv;
	const GValue* value;
	GdaParameter *par;
	
	g_return_val_if_fail (dbin != NULL, NULL);
	priv = dbin->priv;
	
	par = gda_data_model_iter_get_param_for_column (priv->data_iter, 4);
	value = gda_parameter_get_value (par);	
	
	return value != NULL && G_VALUE_HOLDS_STRING (value)
		? g_value_get_string (value) : NULL;
}

const gchar*
symbol_db_engine_iterator_node_get_symbol_extra_string (SymbolDBEngineIteratorNode *dbin,
												   gint sym_info)
{
	SymbolDBEngineIteratorNodePriv *priv;
	GdaParameter *par = NULL;
	GdaParameter *res = NULL;
	gint column = -1;
	
	g_return_val_if_fail (dbin != NULL, NULL);
	priv = dbin->priv;
#if 0	
	DEBUG_PRINT ("symbol_db_engine_iterator_node_get_symbol_extra_string () for %d", 
				 sym_info);

	if (GDA_PARAMETER_LIST(priv->data_iter)) {
		g_print ("-Parameter(s):\n");
		GSList *params;
		for (params = GDA_PARAMETER_LIST(priv->data_iter)->parameters; params; 
			 params = params->next) {
			GdaParameter *parameter = GDA_PARAMETER (params->data);
			g_print ("   - name:%s type:%s\n",
				 gda_object_get_name (GDA_OBJECT (parameter)),
				 g_type_name (gda_parameter_get_g_type (parameter)));
		}
	}
#endif	
	if (sym_info & SYMINFO_FILE_PATH)
	{
		par = gda_parameter_list_find_param (GDA_PARAMETER_LIST(priv->data_iter),
											 "file_path");
	}	
	else if (sym_info & SYMINFO_LANGUAGE)
	{
		par = gda_parameter_list_find_param (GDA_PARAMETER_LIST(priv->data_iter),
											 "language_name");
	}
	else if (sym_info & SYMINFO_IMPLEMENTATION)
	{
		par = gda_parameter_list_find_param (GDA_PARAMETER_LIST(priv->data_iter),
											 "implementation_name");
	}
	else if (sym_info & SYMINFO_ACCESS)
	{
		par = gda_parameter_list_find_param (GDA_PARAMETER_LIST(priv->data_iter),
											 "access_name");
	}
	else if (sym_info & SYMINFO_KIND)
	{
		par = gda_parameter_list_find_param (GDA_PARAMETER_LIST(priv->data_iter),
											 "kind_name");
	}
	else if (sym_info & SYMINFO_TYPE)
	{
		par = gda_parameter_list_find_param (GDA_PARAMETER_LIST(priv->data_iter),
											 "type");
	}
	else if (sym_info & SYMINFO_TYPE_NAME)
	{
		par = gda_parameter_list_find_param (GDA_PARAMETER_LIST(priv->data_iter),
											 "type_name");
	}
	else if (sym_info & SYMINFO_PROJECT_NAME)
	{
		par = gda_parameter_list_find_param (GDA_PARAMETER_LIST(priv->data_iter),
											 "project_name");
	}	
	else if (sym_info & SYMINFO_FILE_IGNORE)
	{
		par = gda_parameter_list_find_param (GDA_PARAMETER_LIST(priv->data_iter),
											 "file_ignore_type");
	}
	else if (sym_info & SYMINFO_FILE_INCLUDE)
	{
		par = gda_parameter_list_find_param (GDA_PARAMETER_LIST(priv->data_iter),
											 "file_include_type");
	}

	if (par != NULL) 
	{
		column = gda_data_model_iter_get_column_for_param (priv->data_iter, par);
		res = gda_data_model_iter_get_param_for_column (priv->data_iter, column);
		return gda_parameter_get_value_str (res);
	}
	else
		return NULL;	
}

void
symbol_db_engine_iterator_node_set_conversion_hash (SymbolDBEngineIteratorNode *dbin,
										 const GHashTable *sym_type_conversion_hash)
{
	g_return_if_fail (dbin != NULL);
	g_return_if_fail (SYMBOL_IS_DB_ENGINE_ITERATOR_NODE (dbin));
	
	SymbolDBEngineIteratorNodePriv *priv;
	
	priv = dbin->priv;
	priv->sym_type_conversion_hash = sym_type_conversion_hash;	
}

void
symbol_db_engine_iterator_node_set_data (SymbolDBEngineIteratorNode *dbin,
										 const GdaDataModelIter *data)
{
	g_return_if_fail (dbin != NULL);
	g_return_if_fail (SYMBOL_IS_DB_ENGINE_ITERATOR_NODE (dbin));
	
	SymbolDBEngineIteratorNodePriv *priv;
	
	priv = dbin->priv;
	priv->data_iter = GDA_DATA_MODEL_ITER (data);
}


/* IAnjutaSymbol implementation */
static IAnjutaSymbolType
isymbol_type (IAnjutaSymbol *isymbol, GError **err)
{
	SymbolDBEngineIteratorNode *s;
	SymbolDBEngineIteratorNodePriv *priv;
	gpointer tag_found;
	const gchar* find_str;
	
	g_return_val_if_fail (SYMBOL_IS_DB_ENGINE_ITERATOR_NODE (isymbol), 
						  IANJUTA_SYMBOL_TYPE_UNDEF);
	
	s = SYMBOL_DB_ENGINE_ITERATOR_NODE (isymbol);
	priv = s->priv;
	
	find_str = symbol_db_engine_iterator_node_get_symbol_extra_string (s,
												   SYMINFO_TYPE);
	if (find_str == NULL) {
		DEBUG_PRINT ("isymbol_type (): find_str is NULL");
		return IANJUTA_SYMBOL_TYPE_UNDEF;												   
	}
												   
	tag_found = g_hash_table_lookup (priv->sym_type_conversion_hash, find_str);
	
	if (tag_found == NULL) {
		DEBUG_PRINT ("isymbol_type (): nothing matching ");
		return IANJUTA_SYMBOL_TYPE_UNDEF;
	}
	
	return (IAnjutaSymbolType)tag_found;
}


static const gchar*
isymbol_type_str (IAnjutaSymbol *isymbol, GError **err)
{
	SymbolDBEngineIteratorNode *s;

	g_return_val_if_fail (SYMBOL_IS_DB_ENGINE_ITERATOR (isymbol),  NULL);
	s = SYMBOL_DB_ENGINE_ITERATOR_NODE (isymbol);
	return symbol_db_engine_iterator_node_get_symbol_extra_string (s, 
								SYMINFO_TYPE);
}

static const gchar*
isymbol_type_name (IAnjutaSymbol *isymbol, GError **err)
{
	SymbolDBEngineIteratorNode *s;

	g_return_val_if_fail (SYMBOL_IS_DB_ENGINE_ITERATOR (isymbol),  NULL);
	s = SYMBOL_DB_ENGINE_ITERATOR_NODE (isymbol);
	return symbol_db_engine_iterator_node_get_symbol_extra_string (s, 
								SYMINFO_TYPE_NAME);
}


static const gchar*
isymbol_name (IAnjutaSymbol *isymbol, GError **err)
{
	SymbolDBEngineIteratorNode *s;

	g_return_val_if_fail (SYMBOL_IS_DB_ENGINE_ITERATOR (isymbol), NULL);
	s = SYMBOL_DB_ENGINE_ITERATOR_NODE (isymbol);
	
	return symbol_db_engine_iterator_node_get_symbol_name (s);
}

static const gchar*
isymbol_args (IAnjutaSymbol *isymbol, GError **err)
{
	SymbolDBEngineIteratorNode *s;

	g_return_val_if_fail (SYMBOL_IS_DB_ENGINE_ITERATOR (isymbol),  NULL);
	s = SYMBOL_DB_ENGINE_ITERATOR_NODE (isymbol);
	return symbol_db_engine_iterator_node_get_symbol_signature (s);
}

static const gchar*
isymbol_scope (IAnjutaSymbol *isymbol, GError **err)
{
	SymbolDBEngineIteratorNode *s;

	g_return_val_if_fail (SYMBOL_IS_DB_ENGINE_ITERATOR (isymbol),  NULL);
	s = SYMBOL_DB_ENGINE_ITERATOR_NODE (isymbol);
//	g_return_val_if_fail (s->priv->tm_tag != NULL, NULL);
//	return s->priv->tm_tag->atts.entry.scope;
	return NULL;
}

static const gchar*
isymbol_inheritance (IAnjutaSymbol *isymbol, GError **err)
{
	SymbolDBEngineIteratorNode *s;

	g_return_val_if_fail (SYMBOL_IS_DB_ENGINE_ITERATOR (isymbol),  NULL);
	s = SYMBOL_DB_ENGINE_ITERATOR_NODE (isymbol);
//	g_return_val_if_fail (s->priv->tm_tag != NULL, NULL);
//	return s->priv->tm_tag->atts.entry.inheritance;
//	return 
	return NULL;
}

static const gchar*
isymbol_access (IAnjutaSymbol *isymbol, GError **err)
{
	SymbolDBEngineIteratorNode *s;

	g_return_val_if_fail (SYMBOL_IS_DB_ENGINE_ITERATOR (isymbol), NULL);
	s = SYMBOL_DB_ENGINE_ITERATOR_NODE (isymbol);
	return symbol_db_engine_iterator_node_get_symbol_extra_string (s, 
																SYMINFO_ACCESS);
}

static const gchar*
isymbol_impl (IAnjutaSymbol *isymbol, GError **err)
{
	SymbolDBEngineIteratorNode *s;

	g_return_val_if_fail (SYMBOL_IS_DB_ENGINE_ITERATOR (isymbol), NULL);
	s = SYMBOL_DB_ENGINE_ITERATOR_NODE (isymbol);
	return symbol_db_engine_iterator_node_get_symbol_extra_string (s, 
													SYMINFO_IMPLEMENTATION);
}

static const gchar*
isymbol_file (IAnjutaSymbol *isymbol, GError **err)
{
	SymbolDBEngineIteratorNode *s;

	g_return_val_if_fail (SYMBOL_IS_DB_ENGINE_ITERATOR (isymbol),  NULL);
	s = SYMBOL_DB_ENGINE_ITERATOR_NODE (isymbol);
	return symbol_db_engine_iterator_node_get_symbol_extra_string (s, 
													SYMINFO_FILE_PATH);
}

static gulong
isymbol_line (IAnjutaSymbol *isymbol, GError **err)
{
	SymbolDBEngineIteratorNode *s;

	g_return_val_if_fail (SYMBOL_IS_DB_ENGINE_ITERATOR (isymbol), 0);
	s = SYMBOL_DB_ENGINE_ITERATOR_NODE (isymbol);
	return symbol_db_engine_iterator_node_get_symbol_file_pos (s);
}

static gboolean
isymbol_is_local (IAnjutaSymbol *isymbol, GError **err)
{
	SymbolDBEngineIteratorNode *s;

	g_return_val_if_fail (SYMBOL_IS_DB_ENGINE_ITERATOR (isymbol), FALSE);
	s = SYMBOL_DB_ENGINE_ITERATOR_NODE (isymbol);
	return symbol_db_engine_iterator_node_get_symbol_is_file_scope (s);
}

static const GdkPixbuf*
isymbol_icon (IAnjutaSymbol *isymbol, GError **err)
{
	SymbolDBEngineIteratorNode *s;
	
	g_return_val_if_fail (SYMBOL_IS_DB_ENGINE_ITERATOR (isymbol), FALSE);
	s = SYMBOL_DB_ENGINE_ITERATOR_NODE (isymbol);
	
	return symbol_db_view_get_pixbuf (
				symbol_db_engine_iterator_node_get_symbol_extra_string (s, 
								SYMINFO_TYPE), 
				symbol_db_engine_iterator_node_get_symbol_extra_string (s, 
								SYMINFO_ACCESS));
}

static void
isymbol_iface_init (IAnjutaSymbolIface *iface)
{
	iface->type = isymbol_type;
	iface->type_str = isymbol_type_str;	
	iface->type_name = isymbol_type_name;
	iface->name = isymbol_name;
	iface->args = isymbol_args;
	iface->scope = isymbol_scope;
	iface->inheritance = isymbol_inheritance;	
	iface->access = isymbol_access;
	iface->impl = isymbol_impl;
	iface->file = isymbol_file;
	iface->line = isymbol_line;
	iface->is_local = isymbol_is_local;
	iface->icon = isymbol_icon;
}

ANJUTA_TYPE_BEGIN (SymbolDBEngineIteratorNode, sdb_engine_iterator_node, G_TYPE_OBJECT);
ANJUTA_TYPE_ADD_INTERFACE (isymbol, IANJUTA_TYPE_SYMBOL);
ANJUTA_TYPE_END;

