/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * symbol-db-model-file.c
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

#include "symbol-db-engine.h"
#include "symbol-db-model-file.h"

struct _SymbolDBModelFilePriv
{
	gchar *file_path;
};

enum
{
	PROP_0,
	PROP_SYMBOL_DB_FILE_PATH
};

G_DEFINE_TYPE (SymbolDBModelFile, sdb_model_file,
               SYMBOL_DB_TYPE_MODEL_PROJECT);

static gint
sdb_model_file_get_n_children (SymbolDBModel *model, gint tree_level,
                               GValue column_values[])
{
	gint n_children;
	SymbolDBEngine *dbe;
	SymbolDBModelFilePriv *priv;
	SymbolDBEngineIterator *iter = NULL;
	
	g_return_val_if_fail (SYMBOL_DB_IS_MODEL_FILE (model), 0);
	priv = SYMBOL_DB_MODEL_FILE (model)->priv;

	g_object_get (model, "symbol-db-engine", &dbe, NULL);
	
	/* If engine is not connected, there is nothing we can show */
	if (!dbe || !symbol_db_engine_is_connected (dbe))
		return 0;
	
	switch (tree_level)
	{
	case 0:
		if (priv->file_path)
		{
			iter = symbol_db_engine_get_file_symbols
				(dbe, priv->file_path, -1, -1, SYMINFO_SIMPLE);
			if (iter)
			{
				n_children = symbol_db_engine_iterator_get_n_items (iter);
				g_object_unref (iter);
				return n_children;
			}
		}
		return 0;
	default:
		return SYMBOL_DB_MODEL_CLASS (sdb_model_file_parent_class)->
				get_n_children (model, tree_level, column_values);
	}
}

static GdaDataModel*
sdb_model_file_get_children (SymbolDBModel *model, gint tree_level,
                             GValue column_values[], gint offset,
                             gint limit)
{
	SymbolDBEngine *dbe;
	SymbolDBModelFilePriv *priv;
	SymbolDBEngineIterator *iter = NULL;

	g_return_val_if_fail (SYMBOL_DB_IS_MODEL_FILE (model), 0);
	priv = SYMBOL_DB_MODEL_FILE (model)->priv;

	g_object_get (model, "symbol-db-engine", &dbe, NULL);
	
	/* If engine is not connected, there is nothing we can show */
	if (!dbe || !symbol_db_engine_is_connected (dbe))
		return NULL;
	
	switch (tree_level)
	{
	case 0:
		if (priv->file_path)
		{
			iter = symbol_db_engine_get_file_symbols
				(dbe, priv->file_path, limit, offset,
				 SYMINFO_SIMPLE | SYMINFO_ACCESS | SYMINFO_TYPE |
				 SYMINFO_KIND | SYMINFO_FILE_PATH);
			if (iter)
			{
				GdaDataModel *data_model;
				data_model =
					GDA_DATA_MODEL (symbol_db_engine_iterator_get_datamodel (iter));
				g_object_ref (data_model);
				g_object_unref (iter);
				return data_model;
			}
		}
		return NULL;
	default:
		return SYMBOL_DB_MODEL_CLASS (sdb_model_file_parent_class)->
				get_children (model, tree_level, column_values, offset, limit);
	}
}

static void
sdb_model_file_set_property (GObject *object, guint prop_id,
                             const GValue *value, GParamSpec *pspec)
{
	gchar *old_file_path;
	SymbolDBModelFilePriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_MODEL_FILE (object));
	priv = SYMBOL_DB_MODEL_FILE (object)->priv;
	
	switch (prop_id)
	{
	case PROP_SYMBOL_DB_FILE_PATH:
		old_file_path = priv->file_path;
		priv->file_path = g_value_dup_string (value);
		if (g_strcmp0 (old_file_path, priv->file_path) != 0)
		    symbol_db_model_update (SYMBOL_DB_MODEL (object));
		g_free (old_file_path);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
sdb_model_file_get_property (GObject *object, guint prop_id,
                             GValue *value, GParamSpec *pspec)
{
	SymbolDBModelFilePriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_MODEL_FILE (object));
	priv = SYMBOL_DB_MODEL_FILE (object)->priv;
	
	switch (prop_id)
	{
	case PROP_SYMBOL_DB_FILE_PATH:
		g_value_set_string (value, priv->file_path);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
sdb_model_file_finalize (GObject *object)
{
	SymbolDBModelFilePriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_MODEL_FILE (object));
	priv = SYMBOL_DB_MODEL_FILE (object)->priv;
	g_free (priv->file_path);

	g_free (priv);
	
	G_OBJECT_CLASS (sdb_model_file_parent_class)->finalize (object);
}

static void
sdb_model_file_init (SymbolDBModelFile *object)
{
	SymbolDBModelFilePriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_MODEL_FILE (object));

	priv = g_new0 (SymbolDBModelFilePriv, 1);
	object->priv = priv;
	
	priv->file_path = NULL;
}

static void
sdb_model_file_class_init (SymbolDBModelFileClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	SymbolDBModelClass* model_class = SYMBOL_DB_MODEL_CLASS (klass);
	
	object_class->finalize = sdb_model_file_finalize;
	object_class->set_property = sdb_model_file_set_property;
	object_class->get_property = sdb_model_file_get_property;

	model_class->get_n_children = sdb_model_file_get_n_children;
	model_class->get_children =  sdb_model_file_get_children;
	
	g_object_class_install_property
		(object_class, PROP_SYMBOL_DB_FILE_PATH,
		 g_param_spec_string ("file-path",
		                      "File Path",
		                      "Absolute file path for which symbols are shown",
		                      NULL, G_PARAM_READABLE | G_PARAM_WRITABLE));
}

GtkTreeModel*
symbol_db_model_file_new (SymbolDBEngine* dbe)
{
	return GTK_TREE_MODEL (g_object_new (SYMBOL_DB_TYPE_MODEL_FILE,
	                                     "symbol-db-engine", dbe, NULL));
}
