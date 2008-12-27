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

#include <libanjuta/anjuta-debug.h>

#include "symbol-db-engine-queries.h"
#include "symbol-db-engine-priv.h"

/*
 * extern declarations 
 */
extern inline const GdaStatement *
sdb_engine_get_statement_by_query_id (SymbolDBEngine * dbe, static_query_type query_id);

extern inline const GdaSet *
sdb_engine_get_query_parameters_list (SymbolDBEngine *dbe, static_query_type query_id);

extern inline const DynChildQueryNode *
sdb_engine_get_dyn_query_node_by_id (SymbolDBEngine *dbe, dyn_query_type query_id,
									 SymExtraInfo sym_info, gsize other_parameters);

extern inline const DynChildQueryNode *
sdb_engine_insert_dyn_query_node_by_id (SymbolDBEngine *dbe, dyn_query_type query_id,
									 	SymExtraInfo sym_info, gsize other_parameters,
										const gchar *sql);

/*
 * implementation starts here 
 */

gint 
symbol_db_glist_compare_func (gconstpointer a, gconstpointer b)
{
	return g_strcmp0 ((const gchar*)a, (const gchar*)b);
}

gint
symbol_db_gtree_compare_func (gconstpointer a, gconstpointer b, gpointer user_data)
{
	return GPOINTER_TO_INT(a) - GPOINTER_TO_INT(b);
}

gboolean
symbol_db_engine_is_locked (SymbolDBEngine * dbe)
{
	SymbolDBEnginePriv *priv;

	g_return_val_if_fail (dbe != NULL, FALSE);
	
	priv = dbe->priv;
	return priv->scanning_status;
}

gchar*
symbol_db_engine_get_full_local_path (SymbolDBEngine *dbe, const gchar* file)
{
	SymbolDBEnginePriv *priv;
	gchar *full_path;
	
	g_return_val_if_fail (dbe != NULL, NULL);
	
	priv = dbe->priv;
	full_path = g_strdup_printf ("%s%s", priv->project_directory, file);
	return full_path;	
}

gchar*
symbol_db_engine_get_file_db_path (SymbolDBEngine *dbe, const gchar* full_local_file_path)
{
	SymbolDBEnginePriv *priv;
	gchar *relative_path;
	const gchar *tmp;
	g_return_val_if_fail (dbe != NULL, NULL);
	g_return_val_if_fail (full_local_file_path != NULL, NULL);
		
	priv = dbe->priv;
	
	if (priv->db_directory == NULL)
		return NULL;

	if (strlen (priv->project_directory) >= strlen (full_local_file_path)) 
	{
		return NULL;
	}

	tmp = full_local_file_path + strlen (priv->project_directory);
	relative_path = strdup (tmp);

	return relative_path;
}

GPtrArray *
symbol_db_engine_get_files_with_zero_symbols (SymbolDBEngine *dbe)
{
	/*select * from file where file_id not in (select file_defined_id from symbol);*/
	SymbolDBEnginePriv *priv;
	GdaDataModel *data_model;
	GPtrArray *files_to_scan;
	const GdaStatement *stmt;
	gint i, num_rows = 0;

	g_return_val_if_fail (dbe != NULL, NULL);
	priv = dbe->priv;
	
	if (priv->mutex)
		g_mutex_lock (priv->mutex);	
	
	if ((stmt = sdb_engine_get_statement_by_query_id (dbe,
								 PREP_QUERY_GET_ALL_FROM_FILE_WHERE_NOT_IN_SYMBOLS))
		== NULL)
	{
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);
		return NULL;
	}

	data_model = gda_connection_statement_execute_select (priv->db_connection, 
														  (GdaStatement*)stmt, 
														  NULL, NULL);
	
	if (!GDA_IS_DATA_MODEL (data_model) ||
		(num_rows = gda_data_model_get_n_rows (GDA_DATA_MODEL (data_model))) <= 0)
	{
		if (data_model != NULL)
			g_object_unref (data_model);
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);
		return NULL;
	}	
		
	/* initialize the array */
	files_to_scan = g_ptr_array_new ();

	/* we can now scan each filename entry to check the last modification time. */
	for (i = 0; i < num_rows; i++)
	{
		const GValue *value;
		const gchar *file_name;
		gchar *file_abs_path = NULL;

		if ((value =
			 gda_data_model_get_value_at (data_model, 
						gda_data_model_get_column_index(data_model, "db_file_path"),
										  i, NULL)) == NULL)
		{
			continue;
		}

		/* build abs path. */
		file_name = g_value_get_string (value);
		file_abs_path = symbol_db_engine_get_full_local_path (dbe, file_name);
		g_ptr_array_add (files_to_scan, file_abs_path);
	}

	g_object_unref (data_model);
	
	if (priv->mutex)
		g_mutex_unlock (priv->mutex);
	
	return files_to_scan;
}
