/* Symbol db performance stress test */


#include <../symbol-db-engine.h>
#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs.h>

#define BASE_PATH "/home/pescio/svnroot/anjuta/plugins/symbol-db"

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
	gchar* uri = gnome_vfs_get_uri_from_local_path (dir);
	
	if (gnome_vfs_directory_list_load (&list, uri, GNOME_VFS_FILE_INFO_GET_MIME_TYPE)
			!= GNOME_VFS_OK)
		return files;
	
	for (node = list; node != NULL; node = g_list_next (node))
	{
		GnomeVFSFileInfo* info = node->data;
		
		if (!info->mime_type)
			continue;
		if (g_str_equal (info->mime_type, "text/x-csrc") ||
				g_str_equal (info->mime_type, "text/x-chdr"))
		{
			g_message ("File: %s", info->name);
			g_ptr_array_add (files, g_build_filename (dir, info->name, NULL));
		}
	}
	
	g_free (uri);
	return files;
}	
	
static void 
on_scan_end (SymbolDBEngine* engine, gpointer user_data)
{
	g_message ("on_scan_end  ()");
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
	gnome_vfs_init();
	
	if (argc != 2)
	{
		g_message ("Usage: benchmark <source_directory>");
		return 1;
	}
	root_dir = argv[1];
	
	GMutex *mutex = g_mutex_new ();
    engine = symbol_db_engine_new (mutex);
    
  
	if (!symbol_db_engine_open_db (engine, root_dir, root_dir))
	{
		g_message ("Could not open database: %s", root_dir);
		return 1;
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
