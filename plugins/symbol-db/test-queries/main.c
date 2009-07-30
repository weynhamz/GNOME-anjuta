#include "../symbol-db-engine.h"
#include <libanjuta/anjuta-debug.h>
#include <gtk/gtk.h>

static void on_single_file_scan_end (SymbolDBEngine* engine, GPtrArray* files)
{
	static int i = 0;
	g_message ("Finished [%d]: %s", i, (gchar*)g_ptr_array_index (files, i));
	i++;
}

static void
find_symbol_by_name_pattern_filtered (SymbolDBEngine *dbe)
{	
	SymbolDBEngineIterator *iter;
	DEBUG_PRINT ("");	
		
	iter = symbol_db_engine_find_symbol_by_name_pattern_filtered (dbe, 
	    "TwoC", 
	    TRUE,
	    SYMTYPE_MAX,
	    TRUE,
	    SYMSEARCH_FILESCOPE_IGNORE,
	    NULL,
	    -1,
	    -1,
	    SYMINFO_SIMPLE);

	if (iter == NULL)
	{
		g_warning ("Iterator null");
		return;
	}

	do {
		SymbolDBEngineIteratorNode *node;

		node = SYMBOL_DB_ENGINE_ITERATOR_NODE (iter);

		DEBUG_PRINT ("node name %s", 
		    symbol_db_engine_iterator_node_get_symbol_name (node));
		
	} while (symbol_db_engine_iterator_move_next (iter) == TRUE);
}

static void
get_scope_members_by_path (SymbolDBEngine* dbe)
{
	GPtrArray *array;
	SymbolDBEngineIterator *iter;

	DEBUG_PRINT ("");
	
	array = g_ptr_array_new ();
	g_ptr_array_add (array, "namespace");	
//	g_ptr_array_add (array, "NSOne");
	g_ptr_array_add (array, "NSFour");
	g_ptr_array_add (array, NULL);
	
	iter = symbol_db_engine_get_scope_members_by_path (dbe, array, SYMINFO_SIMPLE);

	if (iter == NULL)
	{
		g_warning ("Iterator null");
		return;
	}

	do {
		SymbolDBEngineIteratorNode *node;

		node = SYMBOL_DB_ENGINE_ITERATOR_NODE (iter);

		DEBUG_PRINT ("node name %s", 
		    symbol_db_engine_iterator_node_get_symbol_name (node));
		
	} while (symbol_db_engine_iterator_move_next (iter) == TRUE);
	

	g_ptr_array_free (array, TRUE);
}

static void
do_test_queries (SymbolDBEngine* dbe)
{
    
//	get_scope_members_by_path (dbe);

	find_symbol_by_name_pattern_filtered (dbe);
}

static void 
on_scan_end (SymbolDBEngine* dbe, gpointer user_data)
{
	DEBUG_PRINT ("on_scan_end  ()");

	do_test_queries (dbe);
	
	symbol_db_engine_close_db (dbe);
	g_object_unref (dbe);
  	exit(0);
}

int main (int argc, char** argv)
{
  	SymbolDBEngine* engine;
	GHashTable *mimes;
  	GPtrArray* files;
	GPtrArray* languages = g_ptr_array_new();
	gchar* root_dir;
	GFile *g_dir;
	int i;
	
  	gtk_init(&argc, &argv);
  	g_thread_init (NULL);
	gda_init ();

	g_dir = g_file_new_for_path ("sample-db");
	if (g_dir == NULL)
	{
		g_warning ("sample-db doesn't exist");
		return -1;
	}
	
	root_dir = g_file_get_path (g_dir);
	
    engine = symbol_db_engine_new_full ("anjuta-tags", "sample-db");

	if (!symbol_db_engine_open_db (engine, root_dir, root_dir))
	{
		g_message ("Could not open database: %s", root_dir);
		return -1;
	}

	symbol_db_engine_add_new_project (engine, NULL, root_dir);

	mimes = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (mimes, "text/x-csrc", "text/x-csrc");
	g_hash_table_insert (mimes, "text/x-chdr", "text/x-chdr");
	g_hash_table_insert (mimes, "text/x-c++src", "text/x-c++src");
	g_hash_table_insert (mimes, "text/x-c+++hdr", "text/x-c++hdr");
	
	files = symbol_db_util_get_source_files_by_mime (root_dir, mimes);
	g_hash_table_destroy (mimes);
	
	for (i = 0; i < files->len; i++) 
	{
		g_message ("(%d) %s", i, (gchar*)g_ptr_array_index (files, i));
		g_ptr_array_add (languages, "C");
	}
	
	g_signal_connect (engine, "scan-end", G_CALLBACK (on_scan_end), NULL);
	g_signal_connect (G_OBJECT (engine), "single-file-scan-end",
		  G_CALLBACK (on_single_file_scan_end), files);
	
	symbol_db_engine_add_new_files_full (engine, root_dir, files, languages, TRUE);	

	g_free (root_dir);
	g_object_unref (g_dir);
	
	gtk_main();
	return 0;
}
