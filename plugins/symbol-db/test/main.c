/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
 * Copyright (C) Massimo Cora' 2007 <pescio@darkslack.pescionet>
 * 
 * main.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * main.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with main.c.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */
#include <libgda/libgda.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <unistd.h>

#include "../symbol-db-engine.h"
#include "../symbol-db-view.h"
#include "../symbol-db-engine-iterator.h"
#include "../symbol-db-engine-iterator-node.h"
#include "../readtags.h"

static GdaConnection *db_connection = NULL;
static GdaClient *gda_client = NULL;
static gchar *data_source = NULL;
static gchar *dsn_name;

#define ANJUTA_DB_FILE	".anjuta_sym_db"


static void
dump_iterator (SymbolDBEngineIterator *iterator)
{
	if (iterator != NULL) 
	{
		do {
			SymbolDBEngineIteratorNode *iter_node;

			iter_node = SYMBOL_DB_ENGINE_ITERATOR_NODE (iterator);
			const gchar *sname = symbol_db_engine_iterator_node_get_symbol_name (iter_node);
			gint file_pos = symbol_db_engine_iterator_node_get_symbol_file_pos (iter_node);
			
			g_message ("kind...");
			const gchar* kind = symbol_db_engine_iterator_node_get_symbol_extra_string (iter_node,
																						SYMINFO_KIND);
			g_message ("ok");
			
			if (kind == NULL)
				g_message ("kind null");
			g_message ("GOT! : %s %d %s", sname, file_pos, kind);
		} while (symbol_db_engine_iterator_move_next (iterator) == TRUE);		
		g_object_unref (iterator);
	}
}

static void
remove_file (SymbolDBEngine *dbe) 
{			 
	symbol_db_engine_remove_file(dbe, "foo_project", 
				"/home/pescio/svnroot/anjuta/plugins/symbol-db/readtags.c");			 
}

static void
close_project (SymbolDBEngine *dbe) 
{
	g_message ("closing project...");
	symbol_db_engine_close_project (dbe, "foo_project");
	g_message ("...closed");
}


static void 
update_project (SymbolDBEngine *dbe) 
{				 
	symbol_db_engine_update_project_symbols(dbe, "foo_project"/*, TRUE*/);	
}
				 
static void
add_new_files (SymbolDBEngine *dbe) 
{
	GPtrArray *files_array;
	
	files_array = g_ptr_array_new();	
	g_ptr_array_add (files_array, g_strdup("/home/pescio/svnroot/libgda/providers/sqlite/sqlite-src/sqlite3.c"));	
	symbol_db_engine_add_new_files (dbe, "foo_project", files_array, "C", TRUE);
}

static void
get_parents (SymbolDBEngine *dbe)
{
	gint i;
	GPtrArray *scope_path;	
	scope_path = g_ptr_array_new();	
	g_ptr_array_add (scope_path, g_strdup("namespace"));
	g_ptr_array_add (scope_path, g_strdup("Third"));
	g_ptr_array_add (scope_path, g_strdup("namespace"));
	g_ptr_array_add (scope_path, g_strdup("Fourth"));
	g_ptr_array_add (scope_path, NULL);
	
	
	SymbolDBEngineIterator *iterator =
			symbol_db_engine_get_class_parents (dbe, "Fourth_2_class", scope_path);
	
	g_message ("Fourth_2_class parents:");
	
	dump_iterator (iterator);
		
	g_message ("YourClass parents:");
	iterator =
			symbol_db_engine_get_class_parents (dbe, "YourClass", NULL);
	
	dump_iterator (iterator);
}

static void
get_scope_members (SymbolDBEngine *dbe)
{
	gint i;
	GPtrArray *scope_path;	
	scope_path = g_ptr_array_new();	
	g_ptr_array_add (scope_path, g_strdup("namespace"));
	g_ptr_array_add (scope_path, g_strdup("First"));
	g_ptr_array_add (scope_path, g_strdup("namespace"));
	g_ptr_array_add (scope_path, g_strdup("Second"));
//	g_ptr_array_add (scope_path, g_strdup("class"));
//	g_ptr_array_add (scope_path, g_strdup("Second_1_class"));
	g_ptr_array_add (scope_path, NULL);
		
	SymbolDBEngineIterator *iterator =
		symbol_db_engine_get_scope_members (dbe, scope_path,
											SYMINFO_SIMPLE | 
											SYMINFO_FILE_PATH |
											SYMINFO_FILE_IGNORE | 
											SYMINFO_LANGUAGE | 
											SYMINFO_TYPE);
	
	dump_iterator (iterator);
}

static void
get_current_scope (SymbolDBEngine *dbe)
{
	gint i;
	SymbolDBEngineIterator *iterator = 
		symbol_db_engine_get_current_scope (dbe, 
										"/plugins/symbol-browser/test-class.cpp", 
										35);
	if (iterator == NULL)
		g_message ("iterator is NULL");
	
	dump_iterator (iterator);
}

static void
get_global_members (SymbolDBEngine *dbe)
{
/*/	
	SymbolDBEngineIterator *iterator = 
		symbol_db_engine_get_global_members (dbe, 
										"namespace", 
										SYMINFO_SIMPLE, SYMINFO_TYPE);
/*/
	SymbolDBEngineIterator *iterator = 
		symbol_db_engine_get_global_members (dbe, 
										"namespace", 
										TRUE,
										-1,
										-1,
										SYMINFO_SIMPLE | 
										SYMINFO_KIND |
										SYMINFO_ACCESS );
//*/	
	if (iterator == NULL)
		g_message ("iterator is NULL");
	else
		dump_iterator (iterator);
}


void
get_file_symbols (SymbolDBEngine *dbe)
{
	gint i;
	
	SymbolDBEngineIterator *iterator = 
		symbol_db_engine_get_file_symbols (dbe, 
										"/home/pescio/svnroot/anjuta/plugins/symbol-db/readtags.c", 
										SYMINFO_SIMPLE |
										SYMINFO_KIND);
//*/	
	if (iterator == NULL)
		g_message ("iterator is NULL");
	
	dump_iterator (iterator);
}

static void
get_info_by_id (SymbolDBEngine *dbe)
{
	gint i;
	
	SymbolDBEngineIterator *iterator = 
			symbol_db_engine_get_symbol_info_by_id (dbe, 
									205, SYMINFO_SIMPLE | SYMINFO_KIND);
//*/	
	if (iterator == NULL)
		g_message ("iterator is NULL");
	
	dump_iterator (iterator);
}


static void
update_buffers (SymbolDBEngine * dbe)
{

	gchar * foo_buffer = 
		"#include <stdio.h>\n"
		"\n"
		"void my_lucky_func (int argc, char** argv) {\n"
		"int a;\n"
		"}\n"
		"\n";

	GPtrArray *real_files_list;
	GPtrArray *text_buffers;
	GPtrArray *buffer_sizes;
	
	real_files_list = g_ptr_array_new();	
	g_ptr_array_add (real_files_list, 
					 g_strdup("/home/pescio/svnroot/anjuta/plugins/symbol-browser/test-class.cpp"));
//					 g_strdup("/plugins/symbol-browser/test-class.cpp"));					 
	
	text_buffers = g_ptr_array_new();	
	g_ptr_array_add (text_buffers, g_strdup (foo_buffer));
	
	buffer_sizes = g_ptr_array_new();	
	g_ptr_array_add (buffer_sizes, (gpointer)strlen (foo_buffer));
		
	
	symbol_db_engine_update_buffer_symbols (dbe, "foo_project",
										real_files_list,
										text_buffers,
										buffer_sizes);

}


#if 0
#include <libgda/libgda.h>
#include <stdio.h>

int 
main(int argc, char ** argv)
{
	gda_init ("bug488860", "0.1", argc, argv);
	
	GdaQuery *query;
	GdaParameterList *plist;
	GError *error = NULL;

	 
	
	 query = gda_query_new_from_sql (NULL, "SELECT scope_id FROM scope WHERE scope_name = ## /* name:'scope' "
	 "type:gchararray */ AND type_id = (SELECT type_id FROM sym_type WHERE type = ## /* name:'type' "
	 "type:gchararray */ AND type_name = ## /* name:'typename' "
	 "type:gchararray */)", &error);
	
/*	query = gda_query_new_from_sql (NULL, "SELECT symbol_id FROM symbol WHERE symbol.file_defined_id = (SELECT "
					"file_defined_id FROM symbol WHERE symbol = ##thesym)", &error);
*/
	if (error)
		g_print ("Parser ERROR: %s\n", error->message);

	plist = gda_query_get_parameter_list (query);
	if (plist) {
		g_print ("-Parameter(s):\n");
		GSList *params;
		for (params = plist->parameters; params; params = params->next) {
			GdaParameter *parameter = GDA_PARAMETER (params->data);
			g_print ("   - name:%s type:%s\n",
				 gda_object_get_name (GDA_OBJECT (parameter)),
				 g_type_name (gda_parameter_get_g_type (parameter)));
		}

		if (g_slist_length (plist->parameters) == 1) {
			gchar *sql;
			gda_parameter_set_value_str (GDA_PARAMETER (plist->parameters->data), "my_symbol");
			sql = gda_renderer_render_as_sql (GDA_RENDERER (query), plist, NULL, 0, NULL);
			g_print ("SQL: %s\n", sql);
			g_free (sql);
		}
		g_object_unref (plist);
	}
	else
		g_print ("-No params!-\n");
	g_object_unref (query);
	
	return EXIT_SUCCESS;
}

#endif

#if 0
int main(int argc, char** argv)
{
	SymbolDBEngine *dbe;
	tagFile *tag_file;
	tagFileInfo tag_file_info;
	tagEntry tag_entry;
	gint i;
	
	gnome_vfs_init ();
    g_type_init();
	gda_init ("Test db", "0.1", argc, argv);
	dbe = symbol_db_engine_new ();
	
	gchar *prj_dir = "/home/pescio/Projects/entwickler-0.1";

	g_message ("opening database");
	if (symbol_db_engine_open_db (dbe, prj_dir) == FALSE)
		g_message ("error in opening db");
	
//	g_message ("adding new workspace");
//	if (symbol_db_engine_add_new_workspace (dbe, "foo_workspace") == FALSE)
//		g_message ("error adding workspace");
	
//	g_message ("adding new project");
//	if (symbol_db_engine_add_new_project (dbe, NULL, "foo_project") == FALSE)
//		g_message ("error in adding project");
	
	g_message ("opening project");
	if (symbol_db_engine_open_project (dbe, "/home/pescio/Projects/entwickler-0.1") == FALSE)
		g_message ("error in opening project");	
	

	
//	g_message ("adding files...");
//	add_new_files  (dbe);

	/* 
	** Message: elapsed: 319.713238 for (4008) [0.079769 per symbol]
	
	adding index on file (file_path)
	** Message: elapsed: 206.735366 for (4008) [0.051581 per symbol] (db already present)
	** Message: elapsed: 276.093110 for (4008) [0.068886 per symbol]
	
	adding index on sym_type (type)
	** Message: elapsed: 228.382055 for (4008) [0.056982 per symbol]
	
	adding index on sym_type (type, type_name)
	** Message: elapsed: 286.737892 for (4008) [0.071541 per symbol]
	
	adding index scope_uniq_idx_2
	** Message: elapsed: 273.554569 for (4008) [0.068252 per symbol]

	check these:
	v PREP_QUERY_GET_FILE_ID_BY_UNIQUE_NAME
	v PREP_QUERY_GET_SYMBOL_ID_BY_UNIQUE_INDEX_KEY
	v PREP_QUERY_UPDATE_SYMBOL_ALL
	
	v PREP_QUERY_GET_SYM_TYPE_ID
	v PREP_QUERY_SYM_TYPE_NEW
	v PREP_QUERY_GET_SYM_KIND_BY_UNIQUE_NAME
	v PREP_QUERY_SYM_KIND_NEW
	
	
	*/
//	g_message ("updating project");
//	update_project	(dbe);
	
//	get_current_scope (dbe);
	
//	g_message ("getting scope members");
//	get_scope_members (dbe);
	
	g_message ("getting get_global_members");
	get_global_members (dbe);

//	g_message ("getting parents");
//	get_parents (dbe);
	
//	g_message ("getting file's symbols");
//	get_file_symbols (dbe);

//	g_message ("getting symbol info by id");
//	get_info_by_id (dbe);
	
//	g_message ("updating buffers");
//	update_buffers (dbe);
	
	g_message ("go on with main loop");
	GMainLoop *main_loop;	
	main_loop = g_main_loop_new( NULL, FALSE );
	
	g_main_loop_run( main_loop );

	return 0;
}
#endif

#if 0
#include <glib.h>
#include <gtk/gtk.h>

GQueue* messages;
GMutex* mutex;

static void thread(gpointer data)
{
  GQueue* queue = data;
  while (1)
  {
    g_mutex_lock (mutex);
    g_queue_push_head (queue, "Not so interesting message\n");
    g_mutex_unlock (mutex);
  } 
}

static gboolean print_message (gpointer data)
{
  if (g_mutex_trylock (mutex))
  {
    const gchar* message = g_queue_pop_head (messages);
    if (message)
      g_printf (message);
    g_mutex_unlock (mutex);
  }
  return TRUE;
}

int main(int argc, char** argv)
{
  g_thread_init(NULL);
  gtk_init (&argc, &argv);
  
  messages = g_queue_new ();
  mutex = g_mutex_new ();
  GThread* my_thread = g_thread_create (thread, messages, FALSE, NULL);
  
  g_idle_add (print_message, NULL);
  
  gtk_main();
}
#endif

#if 1

#include <libgda/libgda.h>
#include <stdio.h>
#include <glib.h>

static GAsyncQueue* signals_queue = NULL;
static gint counter = 0;
static GMutex * mutex;

static void
bastard_thread (gpointer bdata)
{
	g_mutex_lock (mutex);
	gint i;
	for (i=0; i < 10; i++)
	{
		counter ++;
	
		g_message ("thread pushed %d", counter);
		g_async_queue_push (signals_queue, counter);
		g_message ("thread queue length %d", g_async_queue_length (signals_queue));
	}
	g_mutex_unlock (mutex);
}

static gboolean
idle_signals (gpointer data)
{
	gpointer tmp;
	
	tmp = g_async_queue_try_pop (signals_queue);
	if (tmp != 0) {
		g_message ("idle got %d", (gint)tmp);
		g_message ("idle queue length %d", g_async_queue_length (signals_queue));
	}
	return TRUE;
}

int 
main(int argc, char ** argv)
{
	GMainLoop *main_loop;	
	gint i;
	
	g_thread_init (NULL);
	gtk_init (&argc, &argv);
	signals_queue = g_async_queue_new ();

	mutex = g_mutex_new ();
	
		
	g_idle_add (idle_signals, NULL);

	for (i= 0; i < 10; i++)
	{
		g_thread_create (bastard_thread, NULL, FALSE, NULL);
	}
	
	main_loop = g_main_loop_new( NULL, FALSE );	
	g_main_loop_run( main_loop );

	return 0;
}




#endif

