#include <glib-object.h>
#include <glib.h>
#include <gio/gio.h>
#include <stdio.h>
#include <string.h>
#include <sqlite3.h>

#define HASH_VALUES_FILE "../data/hash_values.log"
#define DB_FILE "example_db.db"

GQueue *values_queue;

static void
create_table (sqlite3 *db)
{
	gchar *sql = "CREATE TABLE sym_type (type_id integer PRIMARY KEY AUTOINCREMENT,"
                   "type_type text not null,"
                   "type_name text not null,"
                   "unique (type_type, type_name))"	;
	
	sqlite3_exec(db, sql, NULL, 0, NULL);
	
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

static gint
open_connection (sqlite3 **db)
{
	gint rc = 0;
  	sqlite3_open (DB_FILE, &(*db));
	
  	if (rc)
	{
    	g_message ("Can't open database: %s\n", sqlite3_errmsg(*db));
    	sqlite3_close(*db);
    	return -1;
  	}
	return 0;
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
insert_data (sqlite3 *db)
{
	sqlite3_stmt *stmt;
	gint i;
	gdouble elapsed_DEBUG;
	GTimer *sym_timer_DEBUG  = g_timer_new ();	
	
	gchar *sql_str = "INSERT INTO sym_type (type_type, type_name) VALUES (?, ?)";

	gint queue_length = g_queue_get_length (values_queue);

	g_message ("begin transaction...");
	sqlite3_exec(db, "BEGIN", 0, 0, 0);
	g_message ("..OK");
	
	g_message ("populating transaction..");
	for (i = 0; i < queue_length; i++)
	{
		gchar * value = g_queue_pop_head (values_queue);	
		gchar **tokens = g_strsplit (value, "|", 2);

		if ( sqlite3_prepare(db, 
				sql_str,  // stmt
				-1, // If than zero, then stmt is read up to the first nul terminator
				&stmt,
				0  /* Pointer to unused portion of stmt */
		    ) != SQLITE_OK)
		{
			printf("\nCould not prepare statement.");
			return;
		}
		
		if (sqlite3_bind_text(
			stmt,
			1,  /* Index of wildcard */
			tokens[0],
			strlen (tokens[0]),
			SQLITE_STATIC) != SQLITE_OK) 
		{
			printf("\nCould not bind int.\n");
			return;
		}

		if (sqlite3_bind_text(
			stmt,
			2,  /* Index of wildcard */
			tokens[1],
			strlen (tokens[1]),
			SQLITE_STATIC) != SQLITE_OK) 
		{
			printf("\nCould not bind int.\n");
			return;
		}
		
		if (sqlite3_step(stmt) != SQLITE_DONE) {
			printf("\nCould not step (execute) stmt.\n");
			return;
		}

		g_strfreev(tokens);
		
		sqlite3_reset(stmt);		
	}
	elapsed_DEBUG = g_timer_elapsed (sym_timer_DEBUG, NULL);
	g_message ("..OK (elapsed %f)", elapsed_DEBUG);
	
	g_message ("committing...");
	
	sqlite3_exec(db, "COMMIT", 0, 0, 0);
	
	elapsed_DEBUG = g_timer_elapsed (sym_timer_DEBUG, NULL);
	g_message ("..OK (elapsed %f)", elapsed_DEBUG);
}

gint 
main(gint argc, gchar **argv)
{
	sqlite3 *db = NULL;


	if ( !g_thread_supported() )
		g_thread_init( NULL );

	g_type_init();

	
	delete_previous_db ();

	if (open_connection (&db) < 0)
		return -1;

	create_table (db);

	/* load        $ wc -l hash_values.log 
	 * 20959 hash_values.log
	 * into our queue.
	 */
	load_queue_values ();


	insert_data (db);	
	
	
  	sqlite3_close(db);
  	return 0;
}
