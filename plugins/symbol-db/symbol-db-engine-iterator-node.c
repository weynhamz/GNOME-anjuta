/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Massimo Cora' 2007-2008 <maxcvs@email.it>
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
	gchar *project_directory;
	gchar *uri;
	
	/* we'll store returned gchar* (build filepaths) here so that user can use them
	 * as far as SymbolEngineIteratorNode exists making sure that the pointers
	 * do not break in memory.
	 */
	GList *file_paths;
};

static GObjectClass* parent_class = NULL;


SymbolDBEngineIteratorNode *
symbol_db_engine_iterator_node_new (const GdaDataModelIter *data)
{	
	SymbolDBEngineIteratorNode *s;
	s = g_object_new (SYMBOL_TYPE_DB_ENGINE_ITERATOR_NODE, NULL);
	s->priv->data_iter = (GdaDataModelIter *)data;
	s->priv->uri = NULL;
	s->priv->project_directory = NULL;
	s->priv->file_paths = NULL;
	
	return s;
}

static void
sdb_engine_iterator_node_instance_init (SymbolDBEngineIteratorNode *object)
{
	SymbolDBEngineIteratorNode *sdbin;
	
	sdbin = SYMBOL_DB_ENGINE_ITERATOR_NODE (object);
	
	sdbin->priv = g_new0 (SymbolDBEngineIteratorNodePriv, 1);
	sdbin->priv->sym_type_conversion_hash = NULL;
	sdbin->priv->uri = NULL;
	sdbin->priv->project_directory = NULL;
	sdbin->priv->file_paths = NULL;
}

static void
sdb_engine_iterator_node_finalize (GObject *object)
{
	SymbolDBEngineIteratorNode *dbin;
	SymbolDBEngineIteratorNodePriv *priv;
	
	dbin = SYMBOL_DB_ENGINE_ITERATOR_NODE (object);	
	priv = dbin->priv;
	g_free (priv->uri);
	g_free (priv->project_directory);

	/* free the paths. From this moment the pointers eventually returned become invalid */
	if (priv->file_paths)
		g_list_foreach (priv->file_paths, (GFunc)g_free, NULL);
	
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
	
	g_return_val_if_fail (dbin != NULL, -1);
	priv = dbin->priv;
	
	value = gda_data_model_iter_get_value_at (priv->data_iter, 0);
	
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
	
	g_return_val_if_fail (dbin != NULL, NULL);
	priv = dbin->priv;

	value = gda_data_model_iter_get_value_at (priv->data_iter, 1);

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
	
	g_return_val_if_fail (dbin != NULL, -1);
	priv = dbin->priv;
	
	value = gda_data_model_iter_get_value_at (priv->data_iter, 2);
	
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
	
	g_return_val_if_fail (dbin != NULL, FALSE);
	priv = dbin->priv;
	
	value = gda_data_model_iter_get_value_at (priv->data_iter, 3);
	
	if (value != NULL && G_VALUE_HOLDS_INT (value))
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
	
	g_return_val_if_fail (dbin != NULL, NULL);
	priv = dbin->priv;
	
	value = gda_data_model_iter_get_value_at (priv->data_iter, 4);
	
	return value != NULL && G_VALUE_HOLDS_STRING (value)
		? g_value_get_string (value) : NULL;
}

const gchar*
symbol_db_engine_iterator_node_get_symbol_extra_string (SymbolDBEngineIteratorNode *dbin,
												   SymExtraInfo sym_info)
{
	SymbolDBEngineIteratorNodePriv *priv;
	const GValue *value = NULL;
	gchar *filepath = NULL;
	
	g_return_val_if_fail (dbin != NULL, NULL);
	priv = dbin->priv;
	
	if (sym_info & SYMINFO_FILE_PATH)
	{
		value = gda_data_model_iter_get_value_for_field (priv->data_iter, 
														 "db_file_path");
		/* build the absolute file path */
		if (value != NULL && G_VALUE_HOLDS_STRING (value) && 
			priv->project_directory != NULL)
		{
			filepath = g_strconcat (priv->project_directory, 
									g_value_get_string (value),
									NULL);
		}
	}	
	else if (sym_info & SYMINFO_LANGUAGE)
	{
		value = gda_data_model_iter_get_value_for_field (priv->data_iter, 
														 "language_name");		
	}
	else if (sym_info & SYMINFO_IMPLEMENTATION)
	{
		value = gda_data_model_iter_get_value_for_field (priv->data_iter, 
														 "implementation_name");
	}
	else if (sym_info & SYMINFO_ACCESS)
	{
		value = gda_data_model_iter_get_value_for_field (priv->data_iter, 
														 "access_name");
	}
	else if (sym_info & SYMINFO_KIND)
	{
		value = gda_data_model_iter_get_value_for_field (priv->data_iter, 
														 "kind_name");
	}
	else if (sym_info & SYMINFO_TYPE)
	{
		value = gda_data_model_iter_get_value_for_field (priv->data_iter, 
														 "type_type");
	}
	else if (sym_info & SYMINFO_TYPE_NAME)
	{
		value = gda_data_model_iter_get_value_for_field (priv->data_iter, 
														 "type_name");
	}
	else if (sym_info & SYMINFO_PROJECT_NAME)
	{
		value = gda_data_model_iter_get_value_for_field (priv->data_iter, 
														 "project_name");
	}	
	else if (sym_info & SYMINFO_FILE_IGNORE)
	{
		value = gda_data_model_iter_get_value_for_field (priv->data_iter, 
														 "file_ignore_type");
	}
	else if (sym_info & SYMINFO_FILE_INCLUDE)
	{
		value = gda_data_model_iter_get_value_for_field (priv->data_iter, 
														 "file_include_type");
	}

	if (filepath != NULL) 
	{
		/* add a reference to GList */
		priv->file_paths = g_list_prepend (priv->file_paths, filepath);
		return filepath;
	}
	
	return value != NULL && G_VALUE_HOLDS_STRING (value)
		? g_value_get_string (value) : NULL;
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
	SymbolDBEngineIteratorNodePriv *priv;
	g_return_if_fail (dbin != NULL);
	g_return_if_fail (data != NULL);
	
	priv = dbin->priv;
	priv->data_iter = GDA_DATA_MODEL_ITER (data);
	if (priv->uri != NULL)
	{
		g_free (priv->uri);
		priv->uri = NULL;
	}
}

void
symbol_db_engine_iterator_node_set_prj_directory (SymbolDBEngineIteratorNode *dbin,
										const gchar *prj_directory)
{
	SymbolDBEngineIteratorNodePriv *priv;
	g_return_if_fail (dbin != NULL);
	
	priv = dbin->priv;
	/* save a dup of the passed prj_directory: we'll need it in case user requests
	 * a file path: db'll return only a relative path.
	 */
	priv->project_directory = g_strdup (prj_directory);

}

/* IAnjutaSymbol implementation */
static const gchar*
isymbol_get_name (IAnjutaSymbol *isymbol, GError **err)
{
	SymbolDBEngineIteratorNode *s;

	g_return_val_if_fail (SYMBOL_IS_DB_ENGINE_ITERATOR (isymbol), NULL);
	s = SYMBOL_DB_ENGINE_ITERATOR_NODE (isymbol);
	
	return symbol_db_engine_iterator_node_get_symbol_name (s);
}

static const gchar*
isymbol_get_args (IAnjutaSymbol *isymbol, GError **err)
{
	SymbolDBEngineIteratorNode *s;

	g_return_val_if_fail (SYMBOL_IS_DB_ENGINE_ITERATOR (isymbol),  NULL);
	s = SYMBOL_DB_ENGINE_ITERATOR_NODE (isymbol);
	return symbol_db_engine_iterator_node_get_symbol_signature (s);
}

static const gchar*
isymbol_get_extra_info_string (IAnjutaSymbol *isymbol, IAnjutaSymbolField sym_info,
							   GError **err)
{
	SymbolDBEngineIteratorNode *s;

	g_return_val_if_fail (SYMBOL_IS_DB_ENGINE_ITERATOR (isymbol), NULL);
	s = SYMBOL_DB_ENGINE_ITERATOR_NODE (isymbol);
	return symbol_db_engine_iterator_node_get_symbol_extra_string (s, sym_info);
} 

static GFile*
isymbol_get_file (IAnjutaSymbol *isymbol, GError **err)
{
	SymbolDBEngineIteratorNode *s;

	g_return_val_if_fail (SYMBOL_IS_DB_ENGINE_ITERATOR (isymbol),  NULL);
	s = SYMBOL_DB_ENGINE_ITERATOR_NODE (isymbol);
	if (s->priv->uri == NULL)
	{
		const gchar* file_path;
		GFile* file;
		file_path = symbol_db_engine_iterator_node_get_symbol_extra_string (s, 
													SYMINFO_FILE_PATH);
		file = g_file_new_for_path (file_path);
		s->priv->uri = g_file_get_uri (file);
		g_object_unref (file);
	}
	return g_file_new_for_uri (s->priv->uri);
}

static gulong
isymbol_get_line (IAnjutaSymbol *isymbol, GError **err)
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
isymbol_get_icon (IAnjutaSymbol *isymbol, GError **err)
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

static IAnjutaSymbolType 
isymbol_get_sym_type (IAnjutaSymbol *isymbol, GError **err)
{
	SymbolDBEngineIteratorNode *s;
	SymbolDBEngineIteratorNodePriv *priv;
	const gchar* type_str;
	gint type;
	
	g_return_val_if_fail (SYMBOL_IS_DB_ENGINE_ITERATOR (isymbol), 
						  IANJUTA_SYMBOL_TYPE_UNDEF);
	s = SYMBOL_DB_ENGINE_ITERATOR_NODE (isymbol);
	
	priv = s->priv;
	
	type_str = symbol_db_engine_iterator_node_get_symbol_extra_string (s, 
												   IANJUTA_SYMBOL_FIELD_TYPE);
	if (type_str == NULL)
		return IANJUTA_SYMBOL_TYPE_UNDEF;

	type = (IAnjutaSymbolType)g_hash_table_lookup ((GHashTable*)priv->sym_type_conversion_hash, 
									  type_str);

	return type;
}

static gint
isymbol_get_id (IAnjutaSymbol *isymbol, GError **err)
{
	SymbolDBEngineIteratorNode *s;

	g_return_val_if_fail (SYMBOL_IS_DB_ENGINE_ITERATOR (isymbol), 0);
	s = SYMBOL_DB_ENGINE_ITERATOR_NODE (isymbol);
	return symbol_db_engine_iterator_node_get_symbol_id (s);
}

static void
isymbol_iface_init (IAnjutaSymbolIface *iface)
{
	iface->get_id = isymbol_get_id;
	iface->get_file = isymbol_get_file;
	iface->get_name = isymbol_get_name;	
	iface->get_line = isymbol_get_line;
	iface->get_sym_type = isymbol_get_sym_type;
	iface->is_local = isymbol_is_local;
	iface->get_args = isymbol_get_args;
	iface->get_extra_info_string = isymbol_get_extra_info_string;
	iface->get_icon = isymbol_get_icon;
}

ANJUTA_TYPE_BEGIN (SymbolDBEngineIteratorNode, sdb_engine_iterator_node, G_TYPE_OBJECT);
ANJUTA_TYPE_ADD_INTERFACE (isymbol, IANJUTA_TYPE_SYMBOL);
ANJUTA_TYPE_END;

