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

#define SDB_MODEL_FILE_SQL " \
	SELECT \
		symbol.symbol_id, \
		symbol.name, \
		symbol.file_position, \
		symbol.scope_definition_id, \
		symbol.signature, \
		symbol.returntype, \
		file.file_path, \
		sym_access.access_name, \
		sym_type.type_type, \
		sym_type.type_name, \
		sym_kind.is_container \
	FROM symbol \
	LEFT JOIN file ON symbol.file_defined_id = file.file_id \
	LEFT JOIN sym_access ON symbol.access_kind_id = sym_access.access_kind_id \
	LEFT JOIN sym_type ON symbol.type_id = sym_type.type_id \
	LEFT JOIN sym_kind ON symbol.kind_id = sym_kind.sym_kind_id \
	WHERE \
	( \
		file.file_path = ## /* name:'filepath' type:gchararray */ \
		AND symbol.scope_id = ## /* name:'parent' type:gint */ \
		AND symbol.kind_id NOT IN \
		( \
			SELECT sym_kind_id \
			FROM sym_kind \
			WHERE sym_kind.kind_name = 'namespace' \
		) \
	) \
	OR \
	( \
		symbol.symbol_id IN \
		( \
			SELECT symbol_id \
			FROM symbol \
			LEFT JOIN file ON symbol.file_defined_id = file.file_id \
			WHERE \
				file.file_path = ## /* name:'filepath' type:gchararray */ \
				AND symbol.scope_id = ## /* name:'parent' type:gint */ \
				AND symbol.kind_id IN \
				( \
					SELECT sym_kind_id \
					FROM sym_kind \
					WHERE sym_kind.kind_name = 'namespace' \
				) \
			GROUP BY symbol.scope_definition_id \
			\
		) \
	) \
	OR \
	( \
		symbol.scope_id = ## /* name:'parent' type:gint */ \
		AND symbol.kind_id IN \
		( \
			SELECT sym_kind_id \
			FROM sym_kind \
			WHERE sym_kind.kind_name = 'class' \
		) \
		AND symbol.scope_definition_id IN \
		( \
			SELECT scope_id \
			FROM symbol \
			JOIN file ON symbol.file_defined_id = file.file_id \
			WHERE file.file_path = ## /* name:'filepath' type:gchararray */ \
			GROUP BY symbol.scope_id \
		) \
	) \
	ORDER BY symbol.name \
	LIMIT ## /* name:'limit' type:gint */ \
	OFFSET ## /* name:'offset' type:gint */ \
	"

struct _SymbolDBModelFilePriv
{
	gchar *file_path;
	guint refresh_queue_id;
	GdaStatement *stmt;
	GdaSet *params;
	GdaHolder *param_file_path, *param_parent_id, *param_limit, *param_offset;
};

enum
{
	PROP_0,
	PROP_SYMBOL_DB_FILE_PATH
};

G_DEFINE_TYPE (SymbolDBModelFile, sdb_model_file,
               SYMBOL_DB_TYPE_MODEL_PROJECT);

static void
sdb_model_file_update_sql_stmt (SymbolDBModel *model)
{
	SymbolDBEngine *dbe;
	SymbolDBModelFilePriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_MODEL_FILE (model));
	priv = SYMBOL_DB_MODEL_FILE (model)->priv;
	
	g_object_get (model, "symbol-db-engine", &dbe, NULL);
	priv->stmt = symbol_db_engine_get_statement (dbe, SDB_MODEL_FILE_SQL);
	gda_statement_get_parameters (priv->stmt, &priv->params, NULL);
	priv->param_file_path = gda_set_get_holder (priv->params, "filepath");
	priv->param_parent_id = gda_set_get_holder (priv->params, "parent");
	priv->param_limit = gda_set_get_holder (priv->params, "limit");
	priv->param_offset = gda_set_get_holder (priv->params, "offset");
}

static GdaDataModel*
sdb_model_file_get_children (SymbolDBModel *model, gint tree_level,
                             GValue column_values[], gint offset,
                             gint limit)
{
	SymbolDBEngine *dbe;
	SymbolDBModelFilePriv *priv;
	gint parent_id = 0;
	const gchar *relative_path = NULL;
	GValue ival = {0};
	GValue sval = {0};

	g_return_val_if_fail (SYMBOL_DB_IS_MODEL_FILE (model), 0);
	priv = SYMBOL_DB_MODEL_FILE (model)->priv;

	g_object_get (model, "symbol-db-engine", &dbe, NULL);
	
	/* If engine is not connected, there is nothing we can show */
	if (!dbe || !symbol_db_engine_is_connected (dbe) || !priv->file_path)
		return NULL;

	/* Determin node parent */
	switch (tree_level)
	{
	case 0:
		parent_id = 0;
		break;
	default:
		parent_id = g_value_get_int
				(&column_values[SYMBOL_DB_MODEL_PROJECT_COL_SCOPE_DEFINITION_ID]);
	}

	if (!priv->stmt)
		sdb_model_file_update_sql_stmt (model);
	
	relative_path = symbol_db_util_get_file_db_path (dbe, priv->file_path);
	
	/* Initialize parameters */
	g_value_init (&ival, G_TYPE_INT);
	g_value_init (&sval, G_TYPE_STRING);
	g_value_set_int (&ival, parent_id);
	gda_holder_set_value (priv->param_parent_id, &ival, NULL);
	g_value_set_int (&ival, limit);
	gda_holder_set_value (priv->param_limit, &ival, NULL);
	g_value_set_int (&ival, offset);
	gda_holder_set_value (priv->param_offset, &ival, NULL);
	g_value_set_static_string (&sval, relative_path);
	gda_holder_set_value (priv->param_file_path, &sval, NULL);
	g_value_reset (&sval);

	return symbol_db_engine_execute_select (dbe, priv->stmt, priv->params);
}

static gint
sdb_model_file_get_n_children (SymbolDBModel *model, gint tree_level,
                               GValue column_values[])
{
	gint n_children = 0;
	GdaDataModel *data_model;

	data_model = sdb_model_file_get_children (model, tree_level,
	                                          column_values, 0,
	                                          INT_MAX);
	if (GDA_IS_DATA_MODEL (data_model))
	{
		n_children = gda_data_model_get_n_rows (data_model);
		g_object_unref (data_model);
	}
	return n_children;
}

static gboolean
sdb_model_file_refresh_idle (gpointer object)
{
	SymbolDBModelFilePriv *priv;
	priv = SYMBOL_DB_MODEL_FILE (object)->priv;
	symbol_db_model_update (SYMBOL_DB_MODEL (object));
	priv->refresh_queue_id = 0;
	return FALSE;
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
		{
			if (!priv->refresh_queue_id)
				priv->refresh_queue_id =
					g_idle_add (sdb_model_file_refresh_idle, object);
		}
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
	if (priv->stmt)
	{
		g_object_unref (priv->stmt);
		g_object_unref (priv->params);
	}
	if (priv->refresh_queue_id)
		g_source_remove (priv->refresh_queue_id);
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
