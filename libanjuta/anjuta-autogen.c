/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
   anjuta-autogen.c
    Copyright (C) 2004 Sebastien Granjoux

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/**
 * SECTION:anjuta-autogen
 * @title: AnjutaAutogen
 * @short_description: Template engine using GNU autogen program.
 * @see_also: #AnjutaLauncher
 * @stability: Unstable
 * @include: libanjuta/anjuta-autogen.h
 *
 * GNU autogen is a program generating a text file from a template and a
 * definition file. The template contains fixed text and variables those will
 * be replaced under the control of the definition file.
 *
 * By example from the following definition file
 * <programlisting>
 * AutoGen Definitions .;
 * list = { list_element = alpha;
 *          list_info    = "some alpha stuff"; };
 * list = { list_info    = "more beta stuff";
 *          list_element = beta; };
 * list = { list_element = omega;
 *          list_info    = "final omega stuff"; }
 * </programlisting>
 * And the following template
 * <programlisting>
 * [+ AutoGen5 template +]
 * typedef enum {[+
 *    FOR list "," +]
 *         IDX_[+ (string-upcase! (get "list_element")) +][+
 *    ENDFOR list +] }  list_enum;
 * </programlisting>
 * Autogen generates
 * <programlisting>
 * typedef enum {
 *         IDX_ALPHA,
 *         IDX_BETA,
 *         IDX_OMEGA }  list_enum;
 * </programlisting>
 *
 * The template file can be quite complex, you can read autogen documentation
 * <ulink url="http://www.gnu.org/software/autogen">here</ulink>.
 *
 * The #AnjutaAutogen object takes care of writing the definition file from
 * a hash table and call autogen. The output can be written in a file or passed
 * to a callback function. Autogen is executed asynchronously, so there is
 * another callback function called when the processing is completed.
 */

#include <config.h>

#include <libanjuta/anjuta-launcher.h>
#include <libanjuta/anjuta-autogen.h>

#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <glib.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gstdio.h>

/*---------------------------------------------------------------------------*/

#define TMP_DEF_FILENAME "NPWDEFXXXXXX"
#define TMP_TPL_FILENAME "NPWTPLXXXXXX"

#define FILE_BUFFER_SIZE	4096

#define TEMPLATES_DIR PACKAGE_DATA_DIR"/templates"

/*---------------------------------------------------------------------------*/

struct _AnjutaAutogen
{
	GObject parent;

	gchar* deffilename;		/* name of generated definition file */
	const gchar* tplfilename;	/* name of template (input) file */
	gchar* temptplfilename;		/* name of generated template if the
					 * previous file doesn't contains
					 * autogen marker */

	GList *library_paths;		/* List of paths for searching include files */
					/* Output file name and handle used
					 * when autogen output is written
					 * in a file */
	const gchar* outfilename;
	FILE* output;
	gboolean empty;
					/* Call back function and data used
					 * when autogen output something */
	AnjutaAutogenOutputFunc outfunc;
	gpointer outdata;
	GDestroyNotify destroy;

					/* Call back function and data used
					 * when autogen terminate */
	AnjutaAutogenFunc endfunc;
	gpointer enddata;

	AnjutaLauncher* launcher;
	gboolean busy;			/* For debugging */
};

struct _AnjutaAutogenClass
{
	GObjectClass parent;
};


/*---------------------------------------------------------------------------*/

/* Helper functions
 *---------------------------------------------------------------------------*/

/**
 * anjuta_check_autogen:
 *
 * Check if autogen version 5 is installed.
 *
 * Return value: %TRUE if autogen is installed.
 */

gboolean
anjuta_check_autogen (void)
{
	gchar* args[] = {"autogen", "-v", NULL};
	gchar* output;

	if (g_spawn_sync (NULL, args, NULL, G_SPAWN_SEARCH_PATH | G_SPAWN_STDERR_TO_DEV_NULL,
		NULL, NULL, &output, NULL, NULL, NULL))
	{
		GRegex *re;
		GMatchInfo *minfo;
		gint ver[3] = {0, 0, 0};

		/* Check autogen 5 version string
		 * Examples:
		 * autogen - The Automated Program Generator - Ver. 5.5.7
		 * autogen (GNU AutoGen) - The Automated Program Generator - Ver. 5.11
		 * autogen (GNU AutoGen) 5.11.9
		 */
		re = g_regex_new ("autogen.* (\\d+)\\.(\\d+)(?:\\.(\\d+))?", 0, 0, NULL);
		g_regex_match (re, output, 0, &minfo);
		if (g_match_info_matches (minfo)) {
			gchar **match_strings;

			match_strings = g_match_info_fetch_all (minfo);
			ver[0] = g_ascii_strtoll (match_strings[1], NULL, 10);
			ver[1] = g_ascii_strtoll (match_strings[2], NULL, 10);
			if (match_strings[3] != NULL) ver[2] = g_ascii_strtoll (match_strings[3], NULL, 10);

			g_strfreev (match_strings);
		}
		g_match_info_free (minfo);
		g_regex_unref (re);

		return ver[0] == 5;
	}

	return FALSE;
}



/* Write definitions
 *---------------------------------------------------------------------------*/

static void
cb_autogen_write_key (const gchar* name, const gchar *value, gpointer user_data)
{
	FILE* def = (FILE *)user_data;

	if (value != NULL)
	{
		if(*value == '{') /* Seems to be a list, so do not quote */
		{
			fprintf(def, "%s = %s;\n", name, value);
		}
		else
		{
			gchar *esc_value = g_strescape (value, NULL);
			fprintf (def, "%s = \"%s\";\n", name, esc_value);
			g_free (esc_value);
		}
	}
}

/**
 * anjuta_autogen_write_definition_file
 * @this: A #AnjutaAutogen object
 * @values: (element-type utf8 utf8): A hash table containing all definitions
 * @error: Error propagation and reporting
 *
 * Write the autogen definition file. The definition file defined variables
 * those will be used, typically replaced, in the template files.
 *
 * The hash table keys are the names of the variables. The name can include an
 * index in square bracket, by example "members[0]". All values are strings but
 * but they could include children using braces, by example
 * "{count=2; list="aa bb"}".
 *
 * The file is created in a temporary directory and removed when the object
 * is destroyed.
 *
 * Returns: %TRUE if the file has been written without error,
 */

gboolean
anjuta_autogen_write_definition_file (AnjutaAutogen* this, GHashTable* values, GError **error)
{
	FILE* def;

	/* Autogen should not be running */
	if (this->busy)
	{
		g_set_error_literal (error, G_FILE_ERROR,
		                     G_FILE_ERROR_AGAIN,
		                     _("Autogen is busy"));

		return FALSE;
	}

	def = fopen (this->deffilename, "wt");
	if (def == NULL)
	{
		g_set_error(
		            error,
		            G_FILE_ERROR,
		            g_file_error_from_errno(errno),
		            _("Could not write definition file \"%s\": %s"),
		            this->deffilename,
					g_strerror(errno)
		            );

		return FALSE;
	}

	/* Generate definition data for autogen */
	fputs ("AutoGen Definitions .;\n",def);
	g_hash_table_foreach (values, (GHFunc)cb_autogen_write_key, def);

	fclose (def);

	return TRUE;
}

/* Set library path
 *---------------------------------------------------------------------------*/

/**
 * anjuta_autogen_set_library_path
 * @this: A #AnjutaAutogen object
 * @directory: A path containing autogen library.
 *
 * Add a new directory in the list of autogen libraries path.
 *
 * Autogen can include files. These included file will be searched by default
 * in the same directory than the template file. This functions allows you to
 * add other directories.
 */

void
anjuta_autogen_set_library_path (AnjutaAutogen* this, const gchar *directory)
{
	g_return_if_fail (directory != NULL);

	this->library_paths = g_list_prepend (this->library_paths, g_strdup (directory));
}

/**
 * anjuta_autogen_clear_library_path
 * @this: A #AnjutaAutogen object
 *
 * Remove all library pathes.
 */

void
anjuta_autogen_clear_library_path (AnjutaAutogen* this)
{
	g_list_foreach (this->library_paths, (GFunc)g_free, NULL);
	g_list_free (this->library_paths);
	this->library_paths = NULL;
}

/**
 * anjuta_autogen_get_library_paths
 * @this: A #AnjutaAutogen object
 *
 * Get the list of all directories searched for files included in the autogen
 * templates.
 *
 * Returns: (element-type gchar *) (transfer none): A list of directories.
 * The content and the list itself are owned by the #AnjutaAutogen object and
 * should not be modified or freed.
 */

GList *
anjuta_autogen_get_library_paths (AnjutaAutogen* this)
{
	return this->library_paths;
}

/* Set input and output
 *---------------------------------------------------------------------------*/

/**
 * anjuta_autogen_set_input_file
 * @this: A #AnjutaAutogen object
 * @filename: name of the input template file
 * @start_marker: (allow-none): start marker string
 * @end_marker: (allow-none): end marker string
 *
 * Read an autogen template file, optionally adding autogen markers.
 *
 * To be recognized as an autogen template, the first line has to contain:
 *	- the start marker
 *	- "autogen5 template"
 *	- the end marker
 *
 * These markers are a custom sequence of up to 7 characters delimiting
 * the start and the end of autogen variables and macros.
 *
 * This function can add this line using the value of @start_marker and
 * @end_marker. If this line is already present in the file,
 * @start_marker and @end_marker must be %NULL.
 *
 * Returns: %TRUE if the file has been read without error.
 */

gboolean
anjuta_autogen_set_input_file (AnjutaAutogen* this, const gchar* filename, const gchar* start_marker, const gchar* end_marker)
{
	FILE* tpl;
	FILE* src;
	gboolean ok;
	gchar* buffer;
	guint len;

	/* Autogen should not be running */
	g_return_val_if_fail (this->busy == FALSE, FALSE);

	/* We need to specify start and end marker or nothing */
	g_return_val_if_fail ((start_marker && end_marker) || (!start_marker && !end_marker), FALSE);

	/* Remove previous temporary file if exist */
	if (this->temptplfilename != NULL)
	{
		remove (this->temptplfilename);
		this->temptplfilename = NULL;
	}
	g_free (this->tplfilename);

	if ((start_marker == NULL) && (end_marker == NULL))
	{
		/* input file is really an autogen file, nothig do to */
		this->tplfilename = g_strdup (filename);

		return TRUE;
	}

	/* Autogen definition is missing, we need to create a temporary file
	 * with them */

	/* Create temporary file */
	this->temptplfilename = g_build_filename (g_get_tmp_dir (), TMP_TPL_FILENAME, NULL);
	mktemp (this->temptplfilename);
	this->tplfilename = this->temptplfilename;
	tpl = fopen (this->tplfilename, "wt");
	if (tpl == NULL) return FALSE;

	/* Add autogen definition */
	fputs (start_marker, tpl);
	fputs (" autogen5 template ", tpl);
	fputs (end_marker, tpl);
	fputc ('\n', tpl);

	/* Copy source file into this new file */
	src = fopen (filename, "rb");
	if (src == NULL) return FALSE;

	buffer = g_new (gchar, FILE_BUFFER_SIZE);

	ok = TRUE;
	for (;!feof (src);)
	{
		len = fread (buffer, 1, FILE_BUFFER_SIZE, src);
		if ((len != FILE_BUFFER_SIZE) && !feof (src))
		{
			ok = FALSE;
			break;
		}

		if (len != fwrite (buffer, 1, len, tpl))
		{
			ok = FALSE;
			break;
		}
	}

	g_free (buffer);
	fclose (src);
	fclose (tpl);

	return ok;
}

/**
 * anjuta_autogen_set_output_file
 * @this: A #AnjutaAutogen object
 * @filename: name of the generated file
 *
 * Define the name of the generated file.
 *
 * Returns: %TRUE if the file has been set without error.
 */

gboolean
anjuta_autogen_set_output_file (AnjutaAutogen* this, const gchar* filename)
{
	/* Autogen should not be running */
	g_return_val_if_fail (this->busy == FALSE, FALSE);

	g_free (this->outfilename);
	this->outfilename = g_strdup (filename);
	this->outfunc = NULL;

	return TRUE;
}

/**
 * anjuta_autogen_set_output_callback
 * @this: A #AnjutaAutogen object
 * @func: (scope notified) (allow-none): Function call each time we get new data from autogen
 * @user_data: (allow-none): User data to pass to @func, or %NULL
 * @destroy: Function call when the process is complete to free user data
 *
 * Define that autogen output should be send to a function as soon as it arrives.
 *
 * Returns: %TRUE if there is no error.
 */

gboolean
anjuta_autogen_set_output_callback (AnjutaAutogen* this, AnjutaAutogenOutputFunc func, gpointer user_data, GDestroyNotify destroy)
{
	/* Autogen should not be running */
	g_return_val_if_fail (this->busy == FALSE, FALSE);

	this->outfunc = func;
	this->outdata = user_data;
	this->destroy = destroy;
	this->outfilename = NULL;

	return TRUE;
}

/* Execute autogen
 *---------------------------------------------------------------------------*/

static void
on_autogen_output (AnjutaLauncher* launcher, AnjutaLauncherOutputType type, const gchar* output, gpointer user_data)
{
	AnjutaAutogen* this = (AnjutaAutogen*)user_data;

	if (this->outfilename != NULL)
	{
		/* Write output in a file */
		if (this->output != NULL)
		{
			fputs (output, this->output);
			this->empty = FALSE;
		}
	}
	if (this->outfunc != NULL)
	{
		/* Call a callback function */
		(this->outfunc)(output, this->outdata);
	}
}

static void
on_autogen_terminated (AnjutaLauncher* launcher, gint pid, gint status, gulong time, AnjutaAutogen* this)
{
	this->busy = FALSE;
	if (this->output != NULL)
	{
		fclose (this->output);
		this->output = NULL;
		/* Delete empty file */
		if (this->empty == TRUE)
		{
			g_remove (this->outfilename);
		}
	}

	if (this->destroy)
	{
		(this->destroy)(this->outdata);
	}
	if (this->endfunc)
	{
		(this->endfunc)(this, this->enddata);
	}
}

/**
 * anjuta_autogen_execute
 * @this: A #AnjutaAutogen object
 * @func: (scope async) (allow-none): A function called when autogen is terminated
 * @data: (allow-none): User data to pass to @func, or %NULL
 * @error: (allow-none): Error propagation and reporting
 *
 * Asynchronously execute autogen to generate the output, calling @func when the
 * process is completed.
 *
 * Returns: %TRUE if the file has been processed without error.
 */

gboolean
anjuta_autogen_execute (AnjutaAutogen* this, AnjutaAutogenFunc func, gpointer data, GError** error)
{
	gchar** args;
	guint arg;
	GList *path;

	/* Autogen should not be running */
	g_return_val_if_fail (this->busy == FALSE, FALSE);
	g_return_val_if_fail (this, FALSE);
	g_return_val_if_fail (this->launcher, FALSE);

	/* Set output end callback */
	if (func != NULL)
	{
		this->endfunc = func;
		this->enddata = data;
	}
	else
	{
		this->endfunc = NULL;
	}
	args = g_new (gchar *, 5 + g_list_length (this->library_paths) * 2);
	args[0] = "autogen";
	arg = 1;
	for (path = g_list_first (this->library_paths); path != NULL; path = g_list_next (path))
	{
		args[arg++] = "-L";
		args[arg++] = (gchar *)(path->data);
	}
	args[arg++] = "-T";
	args[arg++] = (gchar *)this->tplfilename;
	args[arg++] = (gchar *)this->deffilename;
	args[arg] = NULL;

	/* Check if output file can be written */
	if (this->outfilename != NULL)
	{
		/* Open file if it's not already done */
		this->output = fopen (this->outfilename, "wt");
		if (this->output == NULL)
		{
			g_set_error(
				error,
				G_FILE_ERROR,
				g_file_error_from_errno(errno),
				_("Could not open file \"%s\": %s"),
				this->outfilename,
				g_strerror(errno)
			);
			g_free (args);

			return FALSE;
		}
		this->empty = TRUE;
	}

	/* The template and definition file are in UTF-8 so the output too */
	anjuta_launcher_set_encoding (this->launcher, "UTF-8");

	this->busy = TRUE;
	if (!anjuta_launcher_execute_v (this->launcher, NULL, args, NULL, on_autogen_output, this))
	{
		g_free (args);

		return FALSE;
	}
	g_free (args);

	return TRUE;
}

/* Implement GObject
 *---------------------------------------------------------------------------*/

G_DEFINE_TYPE (AnjutaAutogen, anjuta_autogen, G_TYPE_OBJECT);

static void
anjuta_autogen_init (AnjutaAutogen *this)
{
	this->launcher = anjuta_launcher_new ();
	g_signal_connect (G_OBJECT (this->launcher), "child-exited", G_CALLBACK (on_autogen_terminated), this);

	/* Create a temporary file for definitions */
	this->deffilename = g_build_filename (g_get_tmp_dir (), TMP_DEF_FILENAME, NULL);
	mktemp (this->deffilename);

	this->library_paths = g_list_prepend (NULL, g_strdup (TEMPLATES_DIR));
}

static void
anjuta_autogen_dispose (GObject *object)
{
	AnjutaAutogen *this = ANJUTA_AUTOGEN (object);

	if (this->output != NULL)
	{
		/* output is not used if a callback function is used */
		fclose (this->output);
		this->output = NULL;
	}

	if (this->outfilename != NULL)
	{
		g_free (this->outfilename);
		this->outfilename = NULL;
	}

	if (this->tplfilename != NULL)
	{
		g_free (this->tplfilename);
		this->tplfilename = NULL;
	}

	if (this->temptplfilename != NULL)
	{
		/* temptplfilename is not used if input file already
		 * contains autogen marker */
		remove (this->temptplfilename);
		this->temptplfilename = NULL;
	}

	g_list_foreach (this->library_paths, (GFunc)g_free, NULL);
	g_list_free (this->library_paths);
	this->library_paths = NULL;

	if (this->deffilename != NULL)
	{
		remove (this->deffilename);
		g_free (this->deffilename);
		this->deffilename = NULL;
	}

	if (this->launcher != NULL)
	{
		g_signal_handlers_disconnect_by_func (G_OBJECT (this->launcher), G_CALLBACK (on_autogen_terminated), this);
		g_object_unref (this->launcher);
		this->launcher = NULL;
	}

	G_OBJECT_CLASS (anjuta_autogen_parent_class)->dispose (object);
}

static void
anjuta_autogen_class_init (AnjutaAutogenClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = anjuta_autogen_dispose;
}


/* Creation and Destruction
 *---------------------------------------------------------------------------*/

/**
 * anjuta_autogen_new
 *
 * Create a new autogen object.
 *
 * Returns: (transfer full): A new #AnjutaAutogen object. Free it using g_object_unref.
 */

AnjutaAutogen* anjuta_autogen_new (void)
{
	return g_object_new (ANJUTA_TYPE_AUTOGEN, NULL);
}



