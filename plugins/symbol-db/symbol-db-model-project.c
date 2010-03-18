/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * symbol-db-model-project.c
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
#include "symbol-db-model-project.h"

typedef struct _SymbolDBModelProjectPriv SymbolDBModelProjectPriv;
struct _SymbolDBModelProjectPriv
{
	SymbolDBEngine* dbe;
};

#define GET_PRIV(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
						SYMBOL_DB_TYPE_MODEL_PROJECT, \
						SymbolDBModelProjectPriv))
enum {
	DATA_COL_SYMBOL_ID,
	DATA_COL_SYMBOL_NAME,
	DATA_COL_SYMBOL_FILE_LINE,
	DATA_COL_SYMBOL_FILE_SCOPE,
	DATA_COL_SYMBOL_ARGS,
	DATA_COL_SYMBOL_RETURNTYPE,
	DATA_COL_SYMBOL_FILE_PATH,
	DATA_COL_SYMBOL_ACCESS,
	DATA_COL_SYMBOL_KIND_NAME,
	DATA_COL_SYMBOL_TYPE,
	DATA_COL_SYMBOL_TYPE_NAME,
	DATA_N_COLS
};

enum
{
	PROP_0,
	PROP_SYMBOL_DB_ENGINE
};

G_DEFINE_TYPE (SymbolDBModelProject, symbol_db_model_project,
               SYMBOL_DB_TYPE_MODEL);

static gint
symbol_db_model_project_get_n_children (SymbolDBModel *model, gint tree_level,
                                       GValue column_values[])
{
	SymbolDBModelProjectPriv *priv;
	SymbolDBEngineIterator *iter;
	gint n_children;
	gint symbol_id;
	
	g_return_val_if_fail (SYMBOL_DB_IS_MODEL_PROJECT (model), 0);
	priv = GET_PRIV (model);

	/* If engine is not connected, there is nothing we can show */
	if (!priv->dbe || !symbol_db_engine_is_connected (priv->dbe))
		return 0;
		
	switch (tree_level)
	{
		case 0:
			iter = symbol_db_engine_get_global_members_filtered
				(priv->dbe, SYMTYPE_CLASS | SYMTYPE_ENUM | SYMTYPE_STRUCT|
				 SYMTYPE_UNION, TRUE, TRUE, -1, -1,
				 SYMINFO_SIMPLE);
			break;
		case 1:
			symbol_id = g_value_get_int (&column_values[DATA_COL_SYMBOL_ID]);
			iter = symbol_db_engine_get_scope_members_by_symbol_id
				(priv->dbe, symbol_id, -1, -1, SYMINFO_SIMPLE);
			break;
		default:
			return 0; /* FIXME */
	}
	if (!iter)
		return 0;
	n_children = symbol_db_engine_iterator_get_n_items (iter);
	g_object_unref (iter);
	return n_children;
}

static GdaDataModel*
symbol_db_model_project_get_children (SymbolDBModel *model, gint tree_level,
                                     GValue column_values[], gint offset,
                                     gint limit)
{
	SymbolDBModelProjectPriv *priv;
	SymbolDBEngineIterator *iter;
	GdaDataModel *data_model;
	gint symbol_id;

	g_return_val_if_fail (SYMBOL_DB_IS_MODEL_PROJECT (model), 0);
	priv = GET_PRIV (model);
	
	/* If engine is not connected, there is nothing we can show */
	if (!priv->dbe || !symbol_db_engine_is_connected (priv->dbe))
		return NULL;
	
	switch (tree_level)
	{
		case 0:
			iter = symbol_db_engine_get_global_members_filtered
				(priv->dbe, SYMTYPE_CLASS | SYMTYPE_ENUM | SYMTYPE_STRUCT |
				 SYMTYPE_UNION, TRUE, TRUE, limit, offset,
				 SYMINFO_SIMPLE | SYMINFO_ACCESS | SYMINFO_TYPE |
				 SYMINFO_KIND | SYMINFO_FILE_PATH);
			break;
		case 1:
			symbol_id = g_value_get_int (&column_values[DATA_COL_SYMBOL_ID]);
			iter = symbol_db_engine_get_scope_members_by_symbol_id
				(priv->dbe, symbol_id, limit, offset, SYMINFO_SIMPLE |
				 SYMINFO_KIND | SYMINFO_ACCESS | SYMINFO_TYPE |
				 SYMINFO_FILE_PATH);
			break;
		default:
			return NULL;
	}
	if (iter)
	{
		data_model = (GdaDataModel*)symbol_db_engine_iterator_get_datamodel (iter);
		g_object_ref (data_model);
		g_object_unref (iter);
		return data_model;
	}
	return FALSE;
}

static gboolean
symbol_db_model_project_get_query_value (SymbolDBModel *model,
                                        GdaDataModel *data_model,
                                        GdaDataModelIter *iter,
                                        gint column,
                                        GValue *value)
{
	const GdkPixbuf *pixbuf;
	const GValue *ret_value;
	const gchar *type = NULL;
	const gchar *access = NULL;

	switch (column)
	{
	case SYMBOL_DB_MODEL_PROJECT_COL_PIXBUF:
		ret_value = gda_data_model_iter_get_value_for_field (iter,
		                                                     "type_type");
		if (ret_value && G_VALUE_HOLDS_STRING (ret_value))
				type = g_value_get_string (ret_value);
		ret_value = gda_data_model_iter_get_value_for_field (iter,
		                                                     "access_name");
		if (ret_value && G_VALUE_HOLDS_STRING (ret_value))
				access = g_value_get_string (ret_value);

		pixbuf = symbol_db_util_get_pixbuf (type, access);
		g_value_set_object (value, G_OBJECT (pixbuf));
		return TRUE;
		break;
	default:
		return SYMBOL_DB_MODEL_CLASS (symbol_db_model_project_parent_class)->
				get_query_value (model, data_model, iter, column, value);
	}
}

static void
on_symbol_db_project_dbe_unref (SymbolDBModelProject *model)
{
	SymbolDBModelProjectPriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_MODEL_PROJECT (model));
	priv = GET_PRIV (model);
	priv->dbe = NULL;
	symbol_db_model_update (SYMBOL_DB_MODEL (model));
}

static void
symbol_db_model_project_set_property (GObject *object, guint prop_id,
                                     const GValue *value, GParamSpec *pspec)
{
	SymbolDBModelProjectPriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_MODEL_PROJECT (object));
	priv = GET_PRIV (object);
	
	switch (prop_id)
	{
	case PROP_SYMBOL_DB_ENGINE:
		if (priv->dbe)
		{
			g_object_weak_unref (G_OBJECT (priv->dbe),
			                    (GWeakNotify)on_symbol_db_project_dbe_unref,
			                     object);
			g_signal_handlers_disconnect_by_func (priv->dbe,
				              G_CALLBACK (symbol_db_model_update),
				              object);
		}
		priv->dbe = g_value_dup_object (value);
		g_object_weak_ref (G_OBJECT (priv->dbe),
			                    (GWeakNotify)on_symbol_db_project_dbe_unref,
			                     object);
		g_signal_connect_swapped (priv->dbe, "db-connected",
		                          G_CALLBACK (symbol_db_model_update),
		                          object);
		g_signal_connect_swapped (priv->dbe, "db-disconnected",
		                          G_CALLBACK (symbol_db_model_update),
		                          object);
		g_signal_connect_swapped (priv->dbe, "scan-end",
		                          G_CALLBACK (symbol_db_model_update), object);
		
		symbol_db_model_update (SYMBOL_DB_MODEL (object));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
symbol_db_model_project_get_property (GObject *object, guint prop_id,
                                     GValue *value, GParamSpec *pspec)
{
	SymbolDBModelProjectPriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_MODEL_PROJECT (object));
	priv = GET_PRIV (object);
	
	switch (prop_id)
	{
	case PROP_SYMBOL_DB_ENGINE:
		g_value_set_object (value, priv->dbe);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
symbol_db_model_project_finalize (GObject *object)
{
	SymbolDBModelProjectPriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_MODEL_PROJECT (object));
	priv = GET_PRIV (object);
	
	if (priv->dbe)
	{
		g_object_weak_unref (G_OBJECT (priv->dbe),
		                     (GWeakNotify)on_symbol_db_project_dbe_unref,
		                     object);
		g_signal_handlers_disconnect_by_func (priv->dbe,
		                  G_CALLBACK (symbol_db_model_update),
		                  object);
	}
	G_OBJECT_CLASS (symbol_db_model_project_parent_class)->finalize (object);
}

static void
symbol_db_model_project_init (SymbolDBModelProject *object)
{
	SymbolDBModelProjectPriv *priv;
	
	GType types[] = {
		G_TYPE_INT,
		GDK_TYPE_PIXBUF,
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_INT,
	};

	gint data_cols[] = {
		DATA_COL_SYMBOL_ID,
		-1,
		DATA_COL_SYMBOL_NAME,
		DATA_COL_SYMBOL_FILE_PATH,
		DATA_COL_SYMBOL_FILE_LINE,
	};
	
	g_return_if_fail (SYMBOL_DB_IS_MODEL_PROJECT (object));
	priv = GET_PRIV (object);
	
	priv->dbe = NULL;

	g_assert ((sizeof (types) / sizeof (GType)) == (sizeof (data_cols) / sizeof (gint)));
	symbol_db_model_set_columns (SYMBOL_DB_MODEL (object),
	                             (sizeof (types) / sizeof (GType)),
	                             types, data_cols);
}

static void
symbol_db_model_project_class_init (SymbolDBModelProjectClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	SymbolDBModelClass* parent_class = SYMBOL_DB_MODEL_CLASS (klass);

	g_type_class_add_private (klass, sizeof (SymbolDBModelProjectPriv));

	object_class->finalize = symbol_db_model_project_finalize;
	object_class->set_property = symbol_db_model_project_set_property;
	object_class->get_property = symbol_db_model_project_get_property;

	parent_class->get_query_value = symbol_db_model_project_get_query_value;
	parent_class->get_n_children = symbol_db_model_project_get_n_children;
	parent_class->get_children =  symbol_db_model_project_get_children;
	
	g_object_class_install_property
		(object_class, PROP_SYMBOL_DB_ENGINE,
		 g_param_spec_object ("symbol-db-engine",
		                      "Symbol DB Engine",
		                      "Symbol DB Engine instance used to make queries",
		                      SYMBOL_TYPE_DB_ENGINE,
		                      G_PARAM_READABLE |
		                      G_PARAM_WRITABLE |
		                      G_PARAM_CONSTRUCT_ONLY));
}

GtkTreeModel*
symbol_db_model_project_new (SymbolDBEngine* dbe)
{
	return GTK_TREE_MODEL (g_object_new (SYMBOL_DB_TYPE_MODEL_PROJECT,
	                                     "symbol-db-engine", dbe, NULL));
}
