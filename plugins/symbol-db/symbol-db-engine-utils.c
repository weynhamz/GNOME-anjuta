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

#include <libanjuta/resources.h>
#include "symbol-db-engine-utils.h"


static GHashTable *pixbufs_hash = NULL;

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

gchar*
symbol_db_util_get_full_local_path (SymbolDBEngine *dbe, const gchar* file)
{
	SymbolDBEnginePriv *priv;
	gchar *full_path;
	
	g_return_val_if_fail (dbe != NULL, NULL);
	
	priv = dbe->priv;
	full_path = g_strdup_printf ("%s%s", priv->project_directory, file);
	return full_path;	
}

gchar*
symbol_db_util_get_file_db_path (SymbolDBEngine *dbe, const gchar* full_local_file_path)
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
symbol_db_util_get_files_with_zero_symbols (SymbolDBEngine *dbe)
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
		file_abs_path = symbol_db_util_get_full_local_path (dbe, file_name);
		g_ptr_array_add (files_to_scan, file_abs_path);
	}

	g_object_unref (data_model);
	
	if (priv->mutex)
		g_mutex_unlock (priv->mutex);
	
	return files_to_scan;
}

#define CREATE_SYM_ICON(N, F) \
	pix_file = anjuta_res_get_pixmap_file (F); \
	g_hash_table_insert (pixbufs_hash, \
					   N, \
					   gdk_pixbuf_new_from_file (pix_file, NULL)); \
	g_free (pix_file);

static void
sdb_util_load_symbol_pixbufs ()
{
	gchar *pix_file;
	
	if (pixbufs_hash != NULL) 
	{
		/* we already have loaded it */
		return;
	}

	pixbufs_hash = g_hash_table_new (g_str_hash, g_str_equal);

	CREATE_SYM_ICON ("class",             "element-class-16.png");	
	CREATE_SYM_ICON ("enum",     	  	  "element-enumeration-16.png");		
	CREATE_SYM_ICON ("enumerator",     	  "element-enumeration-16.png");	
	CREATE_SYM_ICON ("function",          "element-method-16.png");	
	CREATE_SYM_ICON ("method",            "element-method-16.png");	
	CREATE_SYM_ICON ("interface",         "element-interface-16.png");	
	CREATE_SYM_ICON ("macro",             "element-event-16.png");	
	CREATE_SYM_ICON ("namespace",         "element-namespace-16.png");
	CREATE_SYM_ICON ("none",              "element-literal-16.png");
	CREATE_SYM_ICON ("struct",            "element-structure-16.png");
	CREATE_SYM_ICON ("typedef",           "element-literal-16.png");
	CREATE_SYM_ICON ("union",             "element-structure-16.png");
	CREATE_SYM_ICON ("variable",          "element-literal-16.png");
	CREATE_SYM_ICON ("prototype",         "element-interface-16.png");	
	
	CREATE_SYM_ICON ("privateclass",      "element-class-16.png");
	CREATE_SYM_ICON ("privateenum",   	  "element-enumeration-16.png");
	CREATE_SYM_ICON ("privatefield",   	  "element-event-16.png");
	CREATE_SYM_ICON ("privatefunction",   "element-method-16.png");
	CREATE_SYM_ICON ("privateinterface",  "element-interface-16.png");	
	CREATE_SYM_ICON ("privatemember",     "element-property-16.png");	
	CREATE_SYM_ICON ("privatemethod",     "element-method-16.png");
	CREATE_SYM_ICON ("privateproperty",   "element-property-16.png");
	CREATE_SYM_ICON ("privatestruct",     "element-structure-16.png");
	CREATE_SYM_ICON ("privateprototype",  "element-interface-16.png");

	CREATE_SYM_ICON ("protectedclass",    "element-class-16.png");	
	CREATE_SYM_ICON ("protectedenum",     "element-enumeration-16.png");
	CREATE_SYM_ICON ("protectedfield",    "element-event-16.png");	
	CREATE_SYM_ICON ("protectedmember",   "element-property-16.png");
	CREATE_SYM_ICON ("protectedmethod",   "element-method-16.png");
	CREATE_SYM_ICON ("protectedproperty", "element-property-16.png");
	CREATE_SYM_ICON ("protectedprototype","element-interface-16.png");
	
	CREATE_SYM_ICON ("publicclass",    	  "element-class-16.png");	
	CREATE_SYM_ICON ("publicenum",    	  "element-enumeration-16.png");	
	CREATE_SYM_ICON ("publicfunction",    "element-method-16.png");
	CREATE_SYM_ICON ("publicmember",      "element-method-16.png");
	CREATE_SYM_ICON ("publicproperty",    "element-property-16.png");
	CREATE_SYM_ICON ("publicstruct",      "element-structure-16.png");
	CREATE_SYM_ICON ("publicprototype",   "element-interface-16.png");
	
	/* special icon */
	CREATE_SYM_ICON ("othersvars",   "element-event-16.png");
	CREATE_SYM_ICON ("globalglobal", "element-event-16.png");
}

const GdkPixbuf* 
symbol_db_util_get_pixbuf  (const gchar *node_type, const gchar *node_access)
{
	gchar *search_node;
	GdkPixbuf *pix;
	if (!pixbufs_hash)
	{
		sdb_util_load_symbol_pixbufs ();
	}
	
	/*DEBUG_PRINT ("symbol_db_view_get_pixbuf: node_type %s node_access %s",
				 node_type, node_access);*/
	
	g_return_val_if_fail (node_type != NULL, NULL);

	/* is there a better/quicker method to retrieve pixbufs? */
	if (node_access != NULL)
	{
		search_node = g_strdup_printf ("%s%s", node_access, node_type);
	}
	else 
	{ 
		/* we will not free search_node gchar, so casting here is ok. */
		search_node = (gchar*)node_type;
	}
	pix = GDK_PIXBUF (g_hash_table_lookup (pixbufs_hash, search_node));
	
	if (node_access)
	{
		g_free (search_node);
	}
	
	if (pix == NULL)
	{
		DEBUG_PRINT ("symbol_db_view_get_pixbuf (): no pixbuf for %s %s",					 
					 node_type, node_access);
	}
	
	return pix;
}

gboolean
symbol_db_util_is_pattern_exact_match (const gchar *pattern)
{
	gint i;
	g_return_val_if_fail (pattern != NULL, FALSE);
	gint str_len = strlen (pattern);
	gboolean found_sequence = FALSE;
	gint count = 0;
	
	for (i = 0; i < str_len; i++)
	{
		gchar c = pattern[i];
		gint j = i;
		
		while (c == '%')
		{
			found_sequence = TRUE;
			count++;
			/* grab the next one */
			if (j + 1 < str_len)
			{				
				c = pattern[j+1];
				j++;
			}
			else 
			{
				break;
			}			
		}
		
		if (found_sequence)
			break;
	}

	return (count % 2 == 1) ? FALSE : TRUE;
}

GPtrArray *
symbol_db_util_fill_type_array (IAnjutaSymbolType match_types)
{
	GPtrArray *filter_array;
	filter_array = g_ptr_array_new ();

	if (match_types & IANJUTA_SYMBOL_TYPE_CLASS)
	{
		g_ptr_array_add (filter_array, g_strdup ("class"));
	}

	if (match_types & IANJUTA_SYMBOL_TYPE_ENUM)
	{
		g_ptr_array_add (filter_array, g_strdup ("enum"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_ENUMERATOR)
	{
		g_ptr_array_add (filter_array, g_strdup ("enumerator"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_FIELD)
	{
		g_ptr_array_add (filter_array, g_strdup ("field"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_FUNCTION)
	{
		g_ptr_array_add (filter_array, g_strdup ("function"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_INTERFACE)
	{
		g_ptr_array_add (filter_array, g_strdup ("interface"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_MEMBER)
	{
		g_ptr_array_add (filter_array, g_strdup ("member"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_METHOD)
	{
		g_ptr_array_add (filter_array, g_strdup ("method"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_NAMESPACE)
	{
		g_ptr_array_add (filter_array, g_strdup ("namespace"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_PACKAGE)
	{
		g_ptr_array_add (filter_array, g_strdup ("package"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_PROTOTYPE)
	{
		g_ptr_array_add (filter_array, g_strdup ("prototype"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_STRUCT)
	{
		g_ptr_array_add (filter_array, g_strdup ("struct"));
	}

	if (match_types & IANJUTA_SYMBOL_TYPE_TYPEDEF)
	{
		g_ptr_array_add (filter_array, g_strdup ("typedef"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_STRUCT)
	{
		g_ptr_array_add (filter_array, g_strdup ("struct"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_UNION)
	{
		g_ptr_array_add (filter_array, g_strdup ("union"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_VARIABLE)
	{
		g_ptr_array_add (filter_array, g_strdup ("variable"));
	}
				
	if (match_types & IANJUTA_SYMBOL_TYPE_EXTERNVAR)
	{
		g_ptr_array_add (filter_array, g_strdup ("externvar"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_MACRO)
	{
		g_ptr_array_add (filter_array, g_strdup ("macro"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_MACRO_WITH_ARG)
	{
		g_ptr_array_add (filter_array, g_strdup ("macro_with_arg"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_FILE)
	{
		g_ptr_array_add (filter_array, g_strdup ("file"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_VARIABLE)
	{
		g_ptr_array_add (filter_array, g_strdup ("variable"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_OTHER)
	{
		g_ptr_array_add (filter_array, g_strdup ("other"));
	}

	return filter_array;
}

const GHashTable*
symbol_db_util_get_sym_type_conversion_hash (SymbolDBEngine *dbe)
{
	SymbolDBEnginePriv *priv;
	g_return_val_if_fail (dbe != NULL, NULL);
	
	priv = dbe->priv;
		
	return priv->sym_type_conversion_hash;
}
