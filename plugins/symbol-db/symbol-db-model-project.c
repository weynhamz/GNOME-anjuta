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

#define SDB_MODEL_PROJECT_SQL " \
	SELECT \
		symbol.symbol_id, \
		symbol.name, \
		symbol.file_position, \
		symbol.is_file_scope, \
		symbol.scope_definition_id, \
		symbol.signature, \
		symbol.returntype, \
		file.file_path, \
		sym_access.access_name, \
		sym_type.type_type, \
		sym_type.type_name, \
		sym_kind.kind_name \
	FROM symbol \
	LEFT JOIN file ON symbol.file_defined_id = file.file_id \
	LEFT JOIN sym_access ON symbol.access_kind_id = sym_access.access_kind_id \
	LEFT JOIN sym_type ON symbol.type_id = sym_type.type_id \
	LEFT JOIN sym_kind ON symbol.kind_id = sym_kind.sym_kind_id \
	WHERE \
	( \
		symbol.scope_id = ## /* name:'parent' type:gint */ \
		AND sym_kind.kind_name != 'namespace' \
	) \
	OR \
	( \
		symbol.symbol_id IN \
		( \
			SELECT symbol_id \
			FROM symbol \
			LEFT JOIN file ON symbol.file_defined_id = file.file_id \
			LEFT JOIN sym_kind ON symbol.kind_id = sym_kind.sym_kind_id \
			WHERE \
				symbol.scope_id = ## /* name:'parent' type:gint */ \
				AND sym_kind.kind_name = 'namespace' \
			GROUP BY symbol.scope_definition_id \
			\
		) \
	) \
	ORDER BY symbol.name \
	LIMIT ## /* name:'limit' type:gint */ \
	OFFSET ## /* name:'offset' type:gint */ \
	"

struct _SymbolDBModelProjectPriv
{
	SymbolDBEngine* dbe;
	GdaStatement *stmt;
	GdaSet *params;
	GdaHolder *param_parent_id, *param_limit, *param_offset;
};

enum {
	DATA_COL_SYMBOL_ID,
	DATA_COL_SYMBOL_NAME,
	DATA_COL_SYMBOL_FILE_LINE,
	DATA_COL_SYMBOL_FILE_SCOPE,
	DATA_COL_SYMBOL_SCOPE_DEFINITION_ID,
	DATA_COL_SYMBOL_ARGS,
	DATA_COL_SYMBOL_RETURNTYPE,
	DATA_COL_SYMBOL_FILE_PATH,
	DATA_COL_SYMBOL_ACCESS,
	DATA_COL_SYMBOL_TYPE,
	DATA_COL_SYMBOL_TYPE_NAME,
	DATA_COL_SYMBOL_KIND_NAME,
	DATA_N_COLS
};

enum
{
	PROP_0,
	PROP_SYMBOL_DB_ENGINE
};

G_DEFINE_TYPE (SymbolDBModelProject, sdb_model_project,
               SYMBOL_DB_TYPE_MODEL);

static void
sdb_model_project_update_sql_stmt (SymbolDBModel *model)
{
	SymbolDBModelProjectPriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_MODEL_PROJECT (model));
	priv = SYMBOL_DB_MODEL_PROJECT (model)->priv;
	
	priv->stmt = symbol_db_engine_get_statement (priv->dbe, SDB_MODEL_PROJECT_SQL);
	gda_statement_get_parameters (priv->stmt, &priv->params, NULL);
	priv->param_parent_id = gda_set_get_holder (priv->params, "parent");
	priv->param_limit = gda_set_get_holder (priv->params, "limit");
	priv->param_offset = gda_set_get_holder (priv->params, "offset");
}

static GdaDataModel*
sdb_model_project_get_children (SymbolDBModel *model, gint tree_level,
                                     GValue column_values[], gint offset,
                                     gint limit)
{
	SymbolDBModelProjectPriv *priv;
	gint parent_id;
	GValue ival = {0};

	g_return_val_if_fail (SYMBOL_DB_IS_MODEL_PROJECT (model), 0);
	priv = SYMBOL_DB_MODEL_PROJECT (model)->priv;
	
	/* If engine is not connected, there is nothing we can show */
	if (!priv->dbe || !symbol_db_engine_is_connected (priv->dbe))
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
		sdb_model_project_update_sql_stmt (model);
	
	/* Initialize parameters */
	g_value_init (&ival, G_TYPE_INT);
	g_value_set_int (&ival, parent_id);
	gda_holder_set_value (priv->param_parent_id, &ival, NULL);
	g_value_set_int (&ival, limit);
	gda_holder_set_value (priv->param_limit, &ival, NULL);
	g_value_set_int (&ival, offset);
	gda_holder_set_value (priv->param_offset, &ival, NULL);

	return symbol_db_engine_execute_select (priv->dbe, priv->stmt, priv->params);
}

static gboolean
sdb_model_project_get_has_child (SymbolDBModel *model, gint tree_level,
                                 GValue column_values[])
{
	return (g_value_get_int
	        (&column_values[SYMBOL_DB_MODEL_PROJECT_COL_SCOPE_DEFINITION_ID]) > 0);
}

static gint
sdb_model_project_get_n_children (SymbolDBModel *model, gint tree_level,
                                       GValue column_values[])
{
	gint n_children = 0;
	GdaDataModel *data_model;

	data_model = sdb_model_project_get_children (model, tree_level,
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
sdb_model_project_get_query_value (SymbolDBModel *model,
                                        GdaDataModel *data_model,
                                        GdaDataModelIter *iter,
                                        gint column,
                                        GValue *value)
{
	const GdkPixbuf *pixbuf;
	const GValue *ret_value;
	const gchar *name = NULL;
	const gchar *type = NULL;
	const gchar *access = NULL;
	const gchar *args = NULL;
	GString *label;
	
	switch (column)
	{
	case SYMBOL_DB_MODEL_PROJECT_COL_PIXBUF:
		ret_value = gda_data_model_iter_get_value_at (iter,
		                                              DATA_COL_SYMBOL_TYPE);
		if (ret_value && G_VALUE_HOLDS_STRING (ret_value))
				type = g_value_get_string (ret_value);
		ret_value = gda_data_model_iter_get_value_at (iter,
		                                              DATA_COL_SYMBOL_ACCESS);
		if (ret_value && G_VALUE_HOLDS_STRING (ret_value))
				access = g_value_get_string (ret_value);

		pixbuf = symbol_db_util_get_pixbuf (type, access);
		g_value_set_object (value, G_OBJECT (pixbuf));
		return TRUE;
		break;
	case SYMBOL_DB_MODEL_PROJECT_COL_LABEL:
		label = g_string_new_len (NULL, 256);
		ret_value = gda_data_model_iter_get_value_at (iter,
		                                              DATA_COL_SYMBOL_NAME);
		if (ret_value && G_VALUE_HOLDS_STRING (ret_value))
		{
			name = g_value_get_string (ret_value);
			g_string_assign (label, name);
		}
		ret_value = gda_data_model_iter_get_value_at (iter,
		                                              DATA_COL_SYMBOL_ARGS);
		if (ret_value && G_VALUE_HOLDS_STRING (ret_value))
				args = g_value_get_string (ret_value);
		/* */
		if (args)
		{
			if (strlen (args) == 2)
				g_string_append (label, "()");
			else if (strlen (args) > 2)
				g_string_append (label, "(...)");
			ret_value =
				gda_data_model_iter_get_value_at (iter,
				                                  DATA_COL_SYMBOL_RETURNTYPE);
			if (ret_value && G_VALUE_HOLDS_STRING (ret_value))
			{
				gchar *escaped =
					g_markup_escape_text (g_value_get_string (ret_value), -1);
				g_string_append (label, "<span style=\"italic\"> : ");
				g_string_append (label, escaped);
				g_string_append (label, "</span>");
				g_free (escaped);
			}
		}
		else
		{
			ret_value =
				gda_data_model_iter_get_value_at (iter,
				                                  DATA_COL_SYMBOL_TYPE_NAME);
			if (ret_value && G_VALUE_HOLDS_STRING (ret_value) &&
			    g_strcmp0 (g_value_get_string (ret_value), name) != 0)
			{
				gchar *escaped =
					g_markup_escape_text (g_value_get_string (ret_value), -1);
				g_string_append (label, "<span style=\"italic\"> : ");
				g_string_append (label, escaped);
				g_string_append (label, "</span>");
				g_free (escaped);
			}
		}
		g_value_set_string (value, label->str);
		g_string_free (label, TRUE);
		return TRUE;
		break;
	case SYMBOL_DB_MODEL_PROJECT_COL_ARGS:
		ret_value = gda_data_model_iter_get_value_at (iter,
		                                              DATA_COL_SYMBOL_ARGS);
		if (ret_value && G_VALUE_HOLDS_STRING (ret_value)
		    && strlen (g_value_get_string (ret_value)) > 2)	
		{
			gchar *escaped =
				g_markup_escape_text (g_value_get_string (ret_value), -1);
			g_value_set_string (value, escaped);
			g_free (escaped);
		}
		return TRUE;
		break;
	default:
		return SYMBOL_DB_MODEL_CLASS (sdb_model_project_parent_class)->
				get_query_value (model, data_model, iter, column, value);
	}
}

static void
on_sdb_project_dbe_unref (SymbolDBModelProject *model)
{
	SymbolDBModelProjectPriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_MODEL_PROJECT (model));
	priv = SYMBOL_DB_MODEL_PROJECT (model)->priv;
	priv->dbe = NULL;
	symbol_db_model_update (SYMBOL_DB_MODEL (model));
}

static void
sdb_model_project_set_property (GObject *object, guint prop_id,
                                     const GValue *value, GParamSpec *pspec)
{
	SymbolDBModelProjectPriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_MODEL_PROJECT (object));
	priv = SYMBOL_DB_MODEL_PROJECT (object)->priv;
	
	switch (prop_id)
	{
	case PROP_SYMBOL_DB_ENGINE:
		if (priv->dbe)
		{
			g_object_weak_unref (G_OBJECT (priv->dbe),
			                    (GWeakNotify)on_sdb_project_dbe_unref,
			                     object);
			g_signal_handlers_disconnect_by_func (priv->dbe,
				              G_CALLBACK (symbol_db_model_update),
				              object);
			g_signal_handlers_disconnect_by_func (priv->dbe,
				              G_CALLBACK (symbol_db_model_freeze),
				              object);
			g_signal_handlers_disconnect_by_func (priv->dbe,
				              G_CALLBACK (symbol_db_model_thaw),
				              object);
		}
		priv->dbe = g_value_dup_object (value);
		g_object_weak_ref (G_OBJECT (priv->dbe),
			                    (GWeakNotify)on_sdb_project_dbe_unref,
			                     object);
		g_signal_connect_swapped (priv->dbe, "db-connected",
		                          G_CALLBACK (symbol_db_model_update),
		                          object);
		g_signal_connect_swapped (priv->dbe, "db-disconnected",
		                          G_CALLBACK (symbol_db_model_update),
		                          object);
		g_signal_connect_swapped (priv->dbe, "scan-begin",
		                          G_CALLBACK (symbol_db_model_freeze), object);
		g_signal_connect_swapped (priv->dbe, "scan-end",
		                          G_CALLBACK (symbol_db_model_thaw), object);
		
		symbol_db_model_update (SYMBOL_DB_MODEL (object));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
sdb_model_project_get_property (GObject *object, guint prop_id,
                                     GValue *value, GParamSpec *pspec)
{
	SymbolDBModelProjectPriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_MODEL_PROJECT (object));
	priv = SYMBOL_DB_MODEL_PROJECT (object)->priv;
	
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
sdb_model_project_finalize (GObject *object)
{
	SymbolDBModelProjectPriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_MODEL_PROJECT (object));
	priv = SYMBOL_DB_MODEL_PROJECT (object)->priv;
	
	if (priv->dbe)
	{
		g_object_weak_unref (G_OBJECT (priv->dbe),
		                     (GWeakNotify)on_sdb_project_dbe_unref,
		                     object);
		g_signal_handlers_disconnect_by_func (priv->dbe,
		                  G_CALLBACK (symbol_db_model_update),
		                  object);
		g_signal_handlers_disconnect_by_func (priv->dbe,
		                  G_CALLBACK (symbol_db_model_freeze),
		                  object);
		g_signal_handlers_disconnect_by_func (priv->dbe,
		                  G_CALLBACK (symbol_db_model_thaw),
		                  object);
	}

	if (priv->stmt)
	{
		g_object_unref (priv->stmt);
		g_object_unref (priv->params);
	}
	
	g_free (priv);
	
	G_OBJECT_CLASS (sdb_model_project_parent_class)->finalize (object);
}

static void
sdb_model_project_init (SymbolDBModelProject *object)
{
	SymbolDBModelProjectPriv *priv;
	
	GType types[] = {
		G_TYPE_INT,
		GDK_TYPE_PIXBUF,
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_INT,
		G_TYPE_STRING,
		G_TYPE_INT
	};

	gint data_cols[] = {
		DATA_COL_SYMBOL_ID,
		-1,
		-1,
		DATA_COL_SYMBOL_FILE_PATH,
		DATA_COL_SYMBOL_FILE_LINE,
		DATA_COL_SYMBOL_ARGS,
		DATA_COL_SYMBOL_SCOPE_DEFINITION_ID
	};
	
	g_return_if_fail (SYMBOL_DB_IS_MODEL_PROJECT (object));

	priv = g_new0 (SymbolDBModelProjectPriv, 1);
	object->priv = priv;
	
	priv->dbe = NULL;

	g_assert ((sizeof (types) / sizeof (GType)) == (sizeof (data_cols) / sizeof (gint)));
	symbol_db_model_set_columns (SYMBOL_DB_MODEL (object),
	                             (sizeof (types) / sizeof (GType)),
	                             types, data_cols);
}

static void
sdb_model_project_class_init (SymbolDBModelProjectClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	SymbolDBModelClass* parent_class = SYMBOL_DB_MODEL_CLASS (klass);

	g_type_class_add_private (klass, sizeof (SymbolDBModelProjectPriv));

	object_class->finalize = sdb_model_project_finalize;
	object_class->set_property = sdb_model_project_set_property;
	object_class->get_property = sdb_model_project_get_property;

	parent_class->get_query_value = sdb_model_project_get_query_value;
	parent_class->get_has_child = sdb_model_project_get_has_child;
	parent_class->get_n_children = sdb_model_project_get_n_children;
	parent_class->get_children =  sdb_model_project_get_children;
	
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
