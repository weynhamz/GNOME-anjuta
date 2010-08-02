#include <libgda/libgda.h>
#include <gtk/gtk.h>
#include <sql-parser/gda-sql-parser.h>

#define HASH_VALUES_FILE "../data/hash_values.log"
#define DB_FILE "example_db"

#define SDB_PARAM_SET_STRING(gda_param, str_value) \
	g_value_init (&v, G_TYPE_STRING); \
	g_value_set_string (&v, (str_value)); \
	gda_holder_set_value ((gda_param), &v, NULL); \
	g_value_unset (&v);


GdaConnection *open_connection (void);
void display_products_contents (GdaConnection *cnc);
void create_table (GdaConnection *cnc);
void insert_data (GdaConnection *cnc);
void update_data (GdaConnection *cnc);
void delete_data (GdaConnection *cnc);
const GdaStatement *get_insert_statement_by_query_id (GdaSet **plist);
const GdaStatement *get_select_statement_by_query_id (GdaConnection * cnc, GdaSet **plist);

void run_sql_non_select (GdaConnection *cnc, const gchar *sql);

GdaSqlParser *sql_parser;
GQueue *values_queue;


static GdaDataModel *
execute_select_sql (GdaConnection *cnc, const gchar *sql)
{
	GdaStatement *stmt;
	GdaDataModel *res;
	const gchar *remain;
	GError *err = NULL;
	
	stmt = gda_sql_parser_parse_string (sql_parser, sql, &remain, NULL);	

	if (stmt == NULL)
		return NULL;
	
	res = gda_connection_statement_execute_select (cnc, 
												   (GdaStatement*)stmt, NULL, &err);
	if (!res) 
		g_message ("Could not execute query: %s\n", sql);

	if (err)
	{
		g_message ("error: %s", err->message);
	}

	g_object_unref (stmt);
	
	return res;
}


const GdaStatement *
get_insert_statement_by_query_id (GdaSet **plist)
{
	GdaStatement *stmt;
	gchar *sql = "INSERT INTO sym_type (type_type, type_name) VALUES (## /* name:'type' "
 				"type:gchararray */, ## /* name:'typename' type:gchararray */)";

	/* create a new GdaStatement */
	stmt = gda_sql_parser_parse_string (sql_parser, sql, NULL, NULL);

	if (gda_statement_get_parameters ((GdaStatement*)stmt, 
									  plist, NULL) == FALSE)
	{
		g_warning ("Error on getting parameters");
	}

	return stmt;
}

const GdaStatement *
get_select_statement_by_query_id (GdaConnection * cnc, GdaSet **plist)
{
	GdaStatement *stmt;
	gchar *sql = "SELECT count(*) FROM SYM_TYPE";
	
	GdaSqlParser *parser = gda_connection_create_parser (cnc);
	
	/* create a new GdaStatement */
	stmt = gda_sql_parser_parse_string (parser, sql, NULL, 
										 NULL);

	if (gda_statement_get_parameters ((GdaStatement*)stmt, 
									  plist, NULL) == FALSE)
	{
		g_warning ("Error on getting parameters");
	}

	return stmt;	
}

static void 
load_queue_values ()
{
	gchar line[80];
	values_queue = g_queue_new ();

	FILE *file = fopen (HASH_VALUES_FILE, "r");

	while( fgets(line,sizeof(line),file) )
	{
		/*g_message ("got %s", line);*/
		g_queue_push_tail (values_queue, g_strdup (line));
	}	
	
	fclose (file);
}

static void
delete_previous_db ()
{
	GFile *file;

	g_message ("deleting file "DB_FILE"...");
	
	file = g_file_new_for_path (DB_FILE);

	g_file_delete (file, NULL, NULL);

	g_object_unref (file);

	g_message ("..OK");
}

int
main (int argc, char *argv[])
{
	if ( !g_thread_supported() )
		g_thread_init( NULL );

	g_type_init();
	gtk_init(&argc, &argv);
	
	gda_init ();
    GdaConnection *cnc;

	delete_previous_db ();
	
	/* open connections */
	cnc = open_connection ();
	create_table (cnc);
	
	
	/* load        $ wc -l hash_values.log 
	 * 20959 hash_values.log
	 * into our queue.
	 */
	load_queue_values ();

	insert_data (cnc);	

	
    gda_connection_close (cnc);
	g_object_unref (cnc);

    return 0;
}

/*
 * Open a connection to the example.db file
 */
GdaConnection *
open_connection ()
{
	GdaConnection *cnc;
    GError *error = NULL;

	/* open connection */
    cnc = gda_connection_open_from_string ("SQLite", "DB_DIR=.;DB_NAME="DB_FILE, NULL,
					       GDA_CONNECTION_OPTIONS_THREAD_SAFE,
					       &error);
    if (!cnc) {
    	g_print ("Could not open connection to SQLite database in example_db.db file: %s\n",
                         error && error->message ? error->message : "No detail");
        exit (1);
    }

	/* create an SQL parser */
	sql_parser = gda_connection_create_parser (cnc);
	if (!sql_parser) /* @cnc doe snot provide its own parser => use default one */
		sql_parser = gda_sql_parser_new ();
	/* attach the parser object to the connection */
	g_object_set_data_full (G_OBJECT (cnc), "parser", sql_parser, g_object_unref);

    return cnc;
}

/*
 * Create a "products" table
 */
void
create_table (GdaConnection *cnc)
{
	run_sql_non_select (cnc, "DROP table IF EXISTS sym_type");
    run_sql_non_select (cnc, "CREATE TABLE sym_type (type_id integer PRIMARY KEY AUTOINCREMENT,"
                   "type_type text not null,"
                   "type_name text not null,"
                   "unique (type_type, type_name))");
}

/*
 * Insert some data
 *
 * Even though it is possible to use SQL text which includes the values to insert into the
 * table, it's better to use variables (place holders), or as is done here, convenience functions
 * to avoid SQL injection problems.
 */
void
insert_data (GdaConnection *cnc)
{	
	GdaSet *plist = NULL;
	const GdaStatement *stmt;
	GdaHolder *param;
	gint i;
	gdouble elapsed_DEBUG;
	GTimer *sym_timer_DEBUG  = g_timer_new ();	

	g_message ("begin transaction...");
	gda_connection_begin_transaction (cnc, "symtype", 
	    GDA_TRANSACTION_ISOLATION_READ_UNCOMMITTED, NULL);
	g_message ("..OK");

	gint queue_length = g_queue_get_length (values_queue);

	if ((stmt = get_insert_statement_by_query_id (&plist))
		== NULL)
	{
		g_warning ("query is null");
		return;
	}
	
	g_message ("populating transaction..");
	for (i = 0; i < queue_length; i++)
	{
		gchar * value = g_queue_pop_head (values_queue);	
		gchar **tokens = g_strsplit (value, "|", 2);
		GdaSet *last_inserted = NULL;
		GValue v = {0};		
		
		/* type parameter */
		if ((param = gda_set_get_holder ((GdaSet*)plist, "type")) == NULL)
		{
			g_warning ("param type is NULL from pquery!");
			return;
		}
		
		SDB_PARAM_SET_STRING (param, tokens[0]);

		/* type_name parameter */
		if ((param = gda_set_get_holder ((GdaSet*)plist, "typename")) == NULL)
		{
			g_warning ("param typename is NULL from pquery!");
			return;
		}
		
		SDB_PARAM_SET_STRING (param, tokens[1]);

		/* execute the query with parametes just set */
		gda_connection_statement_execute_non_select (cnc, 
														 (GdaStatement*)stmt, 
														 (GdaSet*)plist, &last_inserted,
														 NULL);

		g_strfreev(tokens);
		/* no need to free value, it'll be freed when associated value 
		 * on hashtable'll be freed
		 */

	}
	
	elapsed_DEBUG = g_timer_elapsed (sym_timer_DEBUG, NULL);
	g_message ("..OK (elapsed %f)", elapsed_DEBUG);

	
	g_message ("committing...");
	gda_connection_commit_transaction (cnc, "symtype", NULL);

	elapsed_DEBUG = g_timer_elapsed (sym_timer_DEBUG, NULL);
	g_message ("..OK (elapsed %f)", elapsed_DEBUG);	
}



/*
 * run a non SELECT command and stops if an error occurs
 */
void
run_sql_non_select (GdaConnection *cnc, const gchar *sql)
{
    GdaStatement *stmt;
    GError *error = NULL;
    gint nrows;
	const gchar *remain;
	GdaSqlParser *parser;

	parser = g_object_get_data (G_OBJECT (cnc), "parser");
	stmt = gda_sql_parser_parse_string (parser, sql, &remain, &error);
	if (remain) 
		g_print ("REMAINS: %s\n", remain);

        nrows = gda_connection_statement_execute_non_select (cnc, stmt, NULL, NULL, &error);
        if (nrows == -1)
                g_error ("NON SELECT error: %s\n", error && error->message ? error->message : "no detail");
	g_object_unref (stmt);
}
