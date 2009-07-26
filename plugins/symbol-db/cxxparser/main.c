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

static gchar *
load_file(const gchar *fileName)
{
	FILE *fp;
	glong len;
	gchar *buf = NULL;

	fp = fopen(fileName, "rb");
	if (!fp) {
		printf("failed to open file 'test.h': %s\n", strerror(errno));
		return NULL;
	}

	//read the whole file
	fseek(fp, 0, SEEK_END); 		//go to end
	len = ftell(fp); 				//get position at end (length)
	fseek(fp, 0, SEEK_SET); 		//go to begining
	buf = (gchar *)malloc(len+1); 	//malloc buffer

	//read into buffer
	glong bytes = fread(buf, sizeof(gchar), len, fp);
	printf("read: %ld\n", bytes);
	if (bytes != len) {
		fclose(fp);
		printf("failed to read from file 'test.h': %s\n", strerror(errno));
		return NULL;
	}

	buf[len] = 0;	// make it null terminated string
	fclose(fp);
	return buf;
}


#define SAMPLE_DB_ABS_PATH "/home/pescio/gitroot/anjuta/plugins/symbol-db/cxxparser/sample-db/"
#define ANJUTA_TAGS "/home/pescio/svnroot/svninstalled/usr/bin/anjuta-tags"

static void 
on_test_simple_struct_scan_end (SymbolDBEngine* dbe, gpointer user_data)
{	
	gchar *associated_source_file = SAMPLE_DB_ABS_PATH"test-simple-struct.c";	
	gchar *file_content = load_file (associated_source_file);

	g_message ("above text: %s", file_content);

	
	engine_parser_process_expression ("var.", file_content, associated_source_file, 9);

//	g_free (file_content);
}

static void
test_simple_struct ()
{		
	gchar *associated_source_file = SAMPLE_DB_ABS_PATH"test-simple-struct.c";	
	gchar *associated_db_file = "test-simple-struct";
	gchar *root_dir = SAMPLE_DB_ABS_PATH;
	SymbolDBEngine *dbe = symbol_db_engine_new_full (ANJUTA_TAGS, 
	    associated_db_file);	
	symbol_db_engine_open_db (dbe, root_dir, root_dir);
	symbol_db_engine_add_new_project (dbe, NULL, root_dir);

	g_signal_connect (dbe, "scan-end", 
	    G_CALLBACK (on_test_simple_struct_scan_end), NULL);
	
	GPtrArray *files_array = g_ptr_array_new ();
	g_ptr_array_add (files_array, associated_source_file);
	
	GPtrArray *source_array = g_ptr_array_new ();
	g_ptr_array_add (source_array, "C");
	
	if (symbol_db_engine_add_new_files (dbe, root_dir, files_array, source_array, TRUE) < 0)
		g_warning ("Error on scanning");


	engine_parser_init (dbe);
}

/**
 * This main simulate an anjuta glib/gtk process. We'll then call some functions
 * of the C++ parser to retrieve the type of an expression.
 */
int	main (int argc, char *argv[])
{
  	gtk_init(&argc, &argv);
  	g_thread_init (NULL);
	gda_init ();	
 	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/simple_c/test-simple-struct", test_simple_struct);
	

	g_test_run();
	gtk_main ();
	return 0;
} 	
#if 0
	
	// FIXME: an instance of symbolmanager should be passed instead of a dbe one.
	engine_parser_init (dbe);
	
	//engine_parser_DEBUG_print_tokens (buf);

//	char *test_str = "str.";
	char *test_str = "Std::String *asd";
	
//	char *test_str = "(wxString*)str.";
//	char *test_str = "((Std::string*)eran)->";
//	char *test_str = "((TypeDefFoo)((Std::string*)eran))->func_returning_klass ().";
	//char *test_str = "((A*)((B*)(foo)))->"; // should return A* as m_name. Check here..
//	char *test_str = "*this->"; // should return A* as m_name. Check here. 

//	printf ("print tokens.....\n");
//	engine_parser_DEBUG_print_tokens (test_str);

	printf ("process expression..... %s\n", test_str);
	engine_parser_process_expression (test_str);

/*
	engine_parser_test_optimize_scope (buf);
	engine_parser_get_local_variables (buf);
*/	
	//gtk_main();
#endif	
