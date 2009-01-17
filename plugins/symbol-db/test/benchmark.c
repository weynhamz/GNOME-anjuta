/* Symbol db performance stress test */


#include <../symbol-db-engine.h>
#include <gtk/gtk.h>

static void on_single_file_scan_end (SymbolDBEngine* engine, GPtrArray* files)
{
	static int i = 0;
	g_message ("Finished [%d]: %s", i, (gchar*)g_ptr_array_index (files, i++));
}

static GPtrArray* get_files (const gchar* dir)
{
	GList* list = NULL;
	GList* node;
	GPtrArray* files = g_ptr_array_new();
	GFile *file;
	GFileEnumerator *enumerator;
	GFileInfo* info;
	GError *error = NULL;
	
	file = g_file_new_for_commandline_arg (dir);
	enumerator = g_file_enumerate_children (file, 
			G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ","
			G_FILE_ATTRIBUTE_STANDARD_NAME,
			G_FILE_QUERY_INFO_NONE,
			NULL, &error);

	if (!enumerator)
	{
		g_warning ("Could not enumerate: %s %s\n", 
				g_file_get_path (file),
				error->message);
		g_error_free (error);
		g_object_unref (file);
		return files;
	}

	for (info = g_file_enumerator_next_file (enumerator, NULL, NULL); info != NULL; 
			info = g_file_enumerator_next_file (enumerator, NULL, NULL))
	{
		const gchar *mime_type = g_file_info_get_content_type (info);
		if (!mime_type)
			continue;
		if (g_str_equal (mime_type, "text/x-csrc") ||
				g_str_equal (mime_type, "text/x-chdr"))
		{
			g_message ("File: %s", g_file_info_get_name (info));
			g_ptr_array_add (files, g_build_filename (dir, g_file_info_get_name (info), NULL));
		}
	}
	
	return files;
}	
	
static void 
on_scan_end (SymbolDBEngine* engine, gpointer user_data)
{
	g_message ("on_scan_end  ()");
//	g_object_unref (engine);
//  	exit(0);
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
	gnome_vfs_init();
	
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
			
	files = get_files (root_dir);
	for (i = 0; i < files->len; i++)
		g_ptr_array_add (languages, "C");
	
	g_signal_connect (engine, "scan-end", G_CALLBACK (on_scan_end), NULL);
	g_signal_connect (G_OBJECT (engine), "single-file-scan-end",
		  G_CALLBACK (on_single_file_scan_end), files);
	
	symbol_db_engine_add_new_files (engine, root_dir, files, languages, TRUE);
	
	gtk_main();
	
	return 0;
}
