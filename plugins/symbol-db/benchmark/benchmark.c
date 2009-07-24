/* Symbol db performance stress test */


#include "../symbol-db-engine.h"
#include <gtk/gtk.h>

static void on_single_file_scan_end (SymbolDBEngine* engine, GPtrArray* files)
{
	static int i = 0;
	g_message ("Finished [%d]: %s", i, (gchar*)g_ptr_array_index (files, i));
	i++;
}
	
static void 
on_scan_end (SymbolDBEngine* engine, gpointer user_data)
{
	g_message ("on_scan_end  ()");
	symbol_db_engine_close_db (engine);
	g_object_unref (engine);
  	exit(0);
}

int main (int argc, char** argv)
{
  	SymbolDBEngine* engine;
  	GPtrArray* files;
	GPtrArray* languages = g_ptr_array_new();
	const gchar* root_dir;
	int i;
	
  	gtk_init(&argc, &argv);
  	g_thread_init (NULL);
	gda_init ();
	
	if (argc != 2)
	{
		g_message ("Usage: benchmark <source_directory>");
		return 1;
	}
	root_dir = argv[1];
	
    engine = symbol_db_engine_new ("/usr/bin/ctags");
  
	if (!symbol_db_engine_open_db (engine, root_dir, root_dir))
	{
		g_message ("Could not open database: %s", root_dir);
		return -1;
	}

	symbol_db_engine_add_new_project (engine, NULL, root_dir);
			
	files = symbol_db_util_get_c_source_files (root_dir);
	for (i = 0; i < files->len; i++)
		g_ptr_array_add (languages, "C");
	
	g_signal_connect (engine, "scan-end", G_CALLBACK (on_scan_end), NULL);
	g_signal_connect (G_OBJECT (engine), "single-file-scan-end",
		  G_CALLBACK (on_single_file_scan_end), files);
	
	symbol_db_engine_add_new_files (engine, root_dir, files, languages, TRUE);	
	
	gtk_main();
	
	return 0;
}
