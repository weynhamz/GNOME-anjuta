/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * symbol-db-model-search.c
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

#include <libanjuta/anjuta-debug.h>
#include "symbol-db-engine.h"
#include "symbol-db-model-search.h"

#define SDB_MODEL_SEARCH_SQL " \
	SELECT \
		symbol.symbol_id, \
		symbol.name, \
		symbol.file_position, \
		symbol.scope_definition_id, \
		symbol.signature, \
		symbol.returntype, \
		symbol.type_type, \
		symbol.type_name, \
		file.file_path, \
		sym_access.access_name, \
		sym_kind.is_container \
	FROM symbol \
	LEFT JOIN file ON symbol.file_defined_id = file.file_id \
	LEFT JOIN sym_access ON symbol.access_kind_id = sym_access.access_kind_id \
	LEFT JOIN sym_kind ON symbol.kind_id = sym_kind.sym_kind_id \
	WHERE symbol.name LIKE ## /* name:'pattern' type:gchararray */ \
	ORDER BY symbol.name \
	LIMIT ## /* name:'limit' type:gint */ \
	OFFSET ## /* name:'offset' type:gint */ \
	"

struct _SymbolDBModelSearchPriv
{
	gchar *search_pattern;
	guint refresh_queue_id;
	GdaStatement *stmt;
	GdaSet *params;
	GdaHolder *param_pattern, *param_limit, *param_offset;
};

enum
{
	PROP_0,
	PROP_SEARCH_PATTERN
};

G_DEFINE_TYPE (SymbolDBModelSearch, sdb_model_search,
               SYMBOL_DB_TYPE_MODEL_PROJECT);

static void
sdb_model_search_update_sql_stmt (SymbolDBModel *model)
{
	SymbolDBEngine *dbe;
	SymbolDBModelSearchPriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_MODEL_SEARCH (model));
	priv = SYMBOL_DB_MODEL_SEARCH (model)->priv;
	
	g_object_get (model, "symbol-db-engine", &dbe, NULL);
	priv->stmt = symbol_db_engine_get_statement (dbe, SDB_MODEL_SEARCH_SQL);
	gda_statement_get_parameters (priv->stmt, &priv->params, NULL);
	priv->param_pattern = gda_set_get_holder (priv->params, "pattern");
	priv->param_limit = gda_set_get_holder (priv->params, "limit");
	priv->param_offset = gda_set_get_holder (priv->params, "offset");
}

static GdaDataModel*
sdb_model_search_get_children (SymbolDBModel *model, gint tree_level,
                             GValue column_values[], gint offset,
                             gint limit)
{
	SymbolDBEngine *dbe;
	SymbolDBModelSearchPriv *priv;
	GValue ival = {0};
	GValue sval = {0};

	g_return_val_if_fail (SYMBOL_DB_IS_MODEL_SEARCH (model), 0);
	priv = SYMBOL_DB_MODEL_SEARCH (model)->priv;

	if (tree_level > 0)
		return NULL; /* It's a flat list */

	if (priv->search_pattern == NULL || strlen (priv->search_pattern) == 2)
		return NULL;
	
	g_object_get (model, "symbol-db-engine", &dbe, NULL);
	
	/* If engine is not connected, there is nothing we can show */
	if (!dbe || !symbol_db_engine_is_connected (dbe) || !priv->search_pattern)
		return NULL;

	if (!priv->stmt)
		sdb_model_search_update_sql_stmt (model);
	
	/* Initialize parameters */
	g_value_init (&ival, G_TYPE_INT);
	g_value_init (&sval, G_TYPE_STRING);
	g_value_set_int (&ival, limit);
	gda_holder_set_value (priv->param_limit, &ival, NULL);
	g_value_set_int (&ival, offset);
	gda_holder_set_value (priv->param_offset, &ival, NULL);
	g_value_set_static_string (&sval, priv->search_pattern);
	gda_holder_set_value (priv->param_pattern, &sval, NULL);
	g_value_reset (&sval);

	return symbol_db_engine_execute_select (dbe, priv->stmt, priv->params);
}

static gint
sdb_model_search_get_n_children (SymbolDBModel *model, gint tree_level,
                               GValue column_values[])
{
	gint n_children = 0;
	GdaDataModel *data_model;

	if (tree_level > 0)
		return 0; /* It's a flat list */
	
	data_model = sdb_model_search_get_children (model, tree_level,
	                                          column_values, 0,
	                                          500);
	if (GDA_IS_DATA_MODEL (data_model))
	{
		n_children = gda_data_model_get_n_rows (data_model);
		g_object_unref (data_model);
	}
	return n_children;
}

static gboolean
sdb_model_search_get_has_child (SymbolDBModel *model, gint tree_level,
                                GValue column_values[])
{
	return FALSE;
}

static gboolean
sdb_model_search_refresh_idle (gpointer object)
{
	SymbolDBModelSearchPriv *priv;
	priv = SYMBOL_DB_MODEL_SEARCH (object)->priv;
	symbol_db_model_update (SYMBOL_DB_MODEL (object));
	priv->refresh_queue_id = 0;
	return FALSE;
}

static void
sdb_model_search_set_property (GObject *object, guint prop_id,
                             const GValue *value, GParamSpec *pspec)
{
	gchar *old_pattern;
	SymbolDBModelSearchPriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_MODEL_SEARCH (object));
	priv = SYMBOL_DB_MODEL_SEARCH (object)->priv;
	
	switch (prop_id)
	{
	case PROP_SEARCH_PATTERN:
		old_pattern = priv->search_pattern;
		priv->search_pattern = g_strdup_printf ("%%%s%%",
		                                        g_value_get_string (value));
		if (g_strcmp0 (old_pattern, priv->search_pattern) != 0)
		{
			if (priv->refresh_queue_id)
				g_source_remove (priv->refresh_queue_id);
			priv->refresh_queue_id =
					g_idle_add (sdb_model_search_refresh_idle, object);
		}
		g_free (old_pattern);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
sdb_model_search_get_property (GObject *object, guint prop_id,
                             GValue *value, GParamSpec *pspec)
{
	SymbolDBModelSearchPriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_MODEL_SEARCH (object));
	priv = SYMBOL_DB_MODEL_SEARCH (object)->priv;
	
	switch (prop_id)
	{
	case PROP_SEARCH_PATTERN:
		g_value_set_string (value, priv->search_pattern);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
sdb_model_search_finalize (GObject *object)
{
	SymbolDBModelSearchPriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_MODEL_SEARCH (object));
	priv = SYMBOL_DB_MODEL_SEARCH (object)->priv;
	g_free (priv->search_pattern);
	if (priv->stmt)
	{
		g_object_unref (priv->stmt);
		g_object_unref (priv->params);
	}
	if (priv->refresh_queue_id)
		g_source_remove (priv->refresh_queue_id);
	g_free (priv);
	
	G_OBJECT_CLASS (sdb_model_search_parent_class)->finalize (object);
}

static void
sdb_model_search_init (SymbolDBModelSearch *object)
{
	SymbolDBModelSearchPriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_MODEL_SEARCH (object));

	priv = g_new0 (SymbolDBModelSearchPriv, 1);
	object->priv = priv;
}

static void
sdb_model_search_class_init (SymbolDBModelSearchClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	SymbolDBModelClass* model_class = SYMBOL_DB_MODEL_CLASS (klass);
	
	object_class->finalize = sdb_model_search_finalize;
	object_class->set_property = sdb_model_search_set_property;
	object_class->get_property = sdb_model_search_get_property;

	model_class->get_n_children = sdb_model_search_get_n_children;
	model_class->get_children =  sdb_model_search_get_children;
	model_class->get_has_child = sdb_model_search_get_has_child;
	
	g_object_class_install_property
		(object_class, PROP_SEARCH_PATTERN,
		 g_param_spec_string ("search-pattern",
		                      "Search Pattern",
		                      "Search pattern to match",
		                      NULL, G_PARAM_READABLE | G_PARAM_WRITABLE));
}

GtkTreeModel*
symbol_db_model_search_new (SymbolDBEngine* dbe)
{
	return GTK_TREE_MODEL (g_object_new (SYMBOL_DB_TYPE_MODEL_SEARCH,
	                                     "symbol-db-engine", dbe, NULL));
}
