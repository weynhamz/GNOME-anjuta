/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Massimo Cora' 2009 <maxcvs@email.it> 
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


//#include <libanjuta/interfaces/ianjuta-symbol-manager.h>
#include "../symbol-db-engine.h"
#include <gtk/gtk.h>

#include <errno.h>
#include <stdlib.h>

#include "engine-parser.h"


#define SAMPLE_DB_ABS_PATH "/home/pescio/gitroot/anjuta/plugins/symbol-db/cxxparser/sample-db/"
#define ANJUTA_TAGS "anjuta-tags"



/* source_file must be provided without extension */
#define INIT_C_TEST(source_file,callback) { \
	gchar *associated_source_file = SAMPLE_DB_ABS_PATH""source_file".c";	\
	gchar *associated_db = source_file;	\
	gchar *root_dir = SAMPLE_DB_ABS_PATH;	\
	SymbolDBEngine *dbe = symbol_db_engine_new_full (ANJUTA_TAGS, associated_db);	\
	symbol_db_engine_open_db (dbe, root_dir, root_dir);	\
	symbol_db_engine_add_new_project (dbe, NULL, root_dir);	\
	g_signal_connect (dbe, "scan-end", G_CALLBACK (callback), NULL);	\
	\
	GPtrArray *files_array = g_ptr_array_new ();	\
	g_ptr_array_add (files_array, associated_source_file);	\
	GPtrArray *source_array = g_ptr_array_new ();	\
	g_ptr_array_add (source_array, "C");	\
	\
	if (symbol_db_engine_add_new_files_full (dbe, root_dir, files_array, source_array, TRUE) < 0)	\
		g_warning ("Error on scanning");	\
	\
	engine_parser_init (dbe);	\
}


/******************************************************************************/
static void 
on_test_complex_struct_scan_end (SymbolDBEngine* dbe, gpointer user_data)
{	
	gchar *associated_source_file = SAMPLE_DB_ABS_PATH"test-complex-struct.c";	
	gchar *file_content;
	g_file_get_contents (associated_source_file, &file_content, NULL, NULL);

	engine_parser_process_expression ("((foo*)var)->asd_struct->", file_content, 
	    associated_source_file, 18);

	g_free (file_content);
}

static void
test_complex_struct ()
{		
	INIT_C_TEST("test-complex-struct", on_test_complex_struct_scan_end);
}

/******************************************************************************/
static void 
on_test_cast_simple_struct_scan_end (SymbolDBEngine* dbe, gpointer user_data)
{	
	g_message ("dbe %p user data is %p", dbe, user_data);
	gchar *associated_source_file = SAMPLE_DB_ABS_PATH"test-cast-simple-struct.c";	
	gchar *file_content;
	g_file_get_contents (associated_source_file, &file_content, NULL, NULL);

	engine_parser_process_expression ("((foo)var).", file_content, 
	    associated_source_file, 15);

	g_free (file_content);
}

static void
test_cast_simple_struct ()
{		
	INIT_C_TEST("test-cast-simple-struct", on_test_cast_simple_struct_scan_end);
}

/******************************************************************************/
static void 
on_test_simple_struct_scan_end (SymbolDBEngine* dbe, gpointer user_data)
{	
	gchar *associated_source_file = SAMPLE_DB_ABS_PATH"test-simple-struct.c";	
	gchar *file_content;
	g_file_get_contents (associated_source_file, &file_content, NULL, NULL);

	engine_parser_process_expression ("var.", file_content, associated_source_file, 9);

	g_free (file_content);
}

static void
test_simple_struct ()
{		
	INIT_C_TEST("test-simple-struct", on_test_simple_struct_scan_end);
}


/**
 * This main simulate an anjuta glib/gtk process. We'll then call some functions
 * of the C++ parser to retrieve the type of an expression.
 */
int	main (int argc, char *argv[])
{
	GMainLoop * main_loop;
  	//gtk_init(&argc, &argv);

	if ( !g_thread_supported() )
  		g_thread_init( NULL );	
  	
	g_type_init();
	gda_init ();	

	main_loop = g_main_loop_new (NULL, FALSE);
	
 	g_test_init (&argc, &argv, NULL);

//	g_test_add_func ("/simple_c/test-simple-struct", test_simple_struct);
//	g_test_add_func ("/simple_c/test-cast-simple-struct", test_cast_simple_struct);
	g_test_add_func ("/complex_c/test-complex-struct", test_complex_struct);

	g_test_run ();
	g_message ("test run finished");

	
	g_main_loop_run (main_loop);
	return 0;
} 	
