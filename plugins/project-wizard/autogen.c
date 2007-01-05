/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    autogen.c
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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
 * Execute autogen program
 *
 * This takes cares of everything needed to run autogen:
 * 	- generate the definition files from a list of values
 * 	- add autogen marker in template file if necessary
 * 	- check autogen version
 * 	- run autogen with AnjutaLauncher object
 * Autogen output could be written in a file or pass to callback function.
 *---------------------------------------------------------------------------*/

#include <config.h>

#include "autogen.h"

#include <libanjuta/anjuta-launcher.h>

#include <glib/gstdio.h>
#include <glib/gfileutils.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gstdio.h>

/*---------------------------------------------------------------------------*/

#define TMP_DEF_FILENAME "NPWDEFXXXXXX"
#define TMP_TPL_FILENAME "NPWTPLXXXXXX"

#define FILE_BUFFER_SIZE	4096

/*---------------------------------------------------------------------------*/

struct _NPWAutogen
{
	gchar* deffilename;		/* name of generated definition file */
	const gchar* tplfilename;	/* name of template (input) file */
	gchar* temptplfilename;		/* name of generated template if the
					 * previous file doesn't contains
					 * autogen marker */

					/* Output file name and handle used
					 * when autogen output is written
					 * in a file */
	const gchar* outfilename;
	FILE* output;
	gboolean empty;
					/* Call back function and data used
					 * when autogen output something */
	NPWAutogenOutputFunc outfunc;
	gpointer outdata;
					/* Call back function and data used
					 * when autogen terminate */
	NPWAutogenFunc endfunc;
	gpointer enddata;

	AnjutaLauncher* launcher;
	gboolean busy;			/* For debugging */
};

/*---------------------------------------------------------------------------*/

/* Helper functions
 *---------------------------------------------------------------------------*/

/* Check if autogen version 5 is present */

gboolean
npw_check_autogen (void)
{
	gchar* args[] = {"autogen", "-v", NULL};
	gchar* output;

	if (g_spawn_sync (NULL, args, NULL, G_SPAWN_SEARCH_PATH | G_SPAWN_STDERR_TO_DEV_NULL,
		NULL, NULL, &output, NULL, NULL, NULL))
	{
		gint ver[3];
		gchar* ptr;

		/* Check autogen */
		if (strstr(output, "The Automated Program Generator") == NULL) return FALSE;

		/* Get version number */
		ptr = strstr(output, "Ver. ");
	       	if (ptr == NULL) return FALSE;
		ptr += 5;
		sscanf(ptr,"%d.%d.%d", &ver[0], &ver[1], &ver[2]);

		return (ver[0] == 5);
	}

	return FALSE;
}

/* Write definitions
 *---------------------------------------------------------------------------*/

static void
cb_autogen_write_definition (const gchar* name, const gchar* value, NPWValueTag tag, gpointer user_data)
{
	FILE* def = (FILE *)user_data;

	if ((tag & NPW_VALID_VALUE) && (value != NULL))
	{
		if(value[0] == '{') /* Seems to be a list, so do not quote */
			fprintf(def, "%s = %s;\n", name, value);
		else
			fprintf (def, "%s = \"%s\";\n", name, value);
	}
}

gboolean
npw_autogen_write_definition_file (NPWAutogen* this, NPWValueHeap* values)
{
	FILE* def;

	/* Autogen should not be running */
	g_return_val_if_fail (this->busy == FALSE, FALSE);

	def = fopen (this->deffilename, "wt");
	if (def == NULL) return FALSE;

	/* Generate definition data for autogen */
	fputs ("AutoGen Definitions .;\n",def);
	npw_value_heap_foreach_value (values, cb_autogen_write_definition, def);

	fclose (def);

	return TRUE;
}

/* Set input and output
 *---------------------------------------------------------------------------*/

gboolean
npw_autogen_set_input_file (NPWAutogen* this, const gchar* filename, const gchar* start_marker, const gchar* end_marker)
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
		g_free (this->temptplfilename);
		this->temptplfilename = NULL;
	}

	if ((start_marker == NULL) && (end_marker == NULL))
	{
		/* input file is really an autogen file, nothig do to */
		this->tplfilename = filename;

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

gboolean
npw_autogen_set_output_file (NPWAutogen* this, const gchar* filename)
{
	/* Autogen should not be running */
	g_return_val_if_fail (this->busy == FALSE, FALSE);

	this->outfilename = filename;
	this->outfunc = NULL;

	return TRUE;
}

gboolean
npw_autogen_set_output_callback (NPWAutogen* this, NPWAutogenOutputFunc func, gpointer user_data)
{
	/* Autogen should not be running */
	g_return_val_if_fail (this->busy == FALSE, FALSE);

	this->outfunc = func;
	this->outdata = user_data;
	this->outfilename = NULL;

	return TRUE;
}

/* Execute autogen
 *---------------------------------------------------------------------------*/

static void
on_autogen_output (AnjutaLauncher* launcher, AnjutaLauncherOutputType type, const gchar* output, gpointer user_data)
{
	NPWAutogen* this = (NPWAutogen*)user_data;

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
on_autogen_terminated (AnjutaLauncher* launcher, gint pid, gint status, gulong time, NPWAutogen* this)
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

	if (this->endfunc)
	{
		(this->endfunc)(this, this->enddata);
	}
}

gboolean
npw_autogen_execute (NPWAutogen* this, NPWAutogenFunc func, gpointer data, GError** error) 
{
	gchar* args[] = {"autogen", "-T", NULL, NULL, NULL};

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
	args[2] = (gchar *)this->tplfilename;
	args[3] = (gchar *)this->deffilename;

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
				"Could not open file \"%s\": %s",
				this->outfilename,
				g_strerror(errno)
			);

			return FALSE;
		}
		this->empty = TRUE;
	}
	
	this->busy = TRUE;
	if (!anjuta_launcher_execute_v (this->launcher, args, on_autogen_output, this))
	{
		return FALSE;
	}
	/* Keep output as it is */
	anjuta_launcher_set_encoding (this->launcher, NULL);

	return TRUE;
}

/* Creation and Destruction
 *---------------------------------------------------------------------------*/

NPWAutogen* npw_autogen_new (void)
{
	NPWAutogen* this;

	this = g_new0(NPWAutogen, 1);

	this->launcher = anjuta_launcher_new ();
	g_signal_connect (G_OBJECT (this->launcher), "child-exited", G_CALLBACK (on_autogen_terminated), this);

	/* Create a temporary file for definitions */
	this->deffilename = g_build_filename (g_get_tmp_dir (), TMP_DEF_FILENAME, NULL);
	mktemp (this->deffilename);

	return this;
}

void npw_autogen_free (NPWAutogen* this)
{
	g_return_if_fail(this != NULL);

	if (this->output != NULL)
	{
		/* output is not used if a callback function is used */
		fclose (this->output);
	}

	if (this->temptplfilename != NULL)
	{
		/* temptplfilename is not used if input file already
		 * contains autogen marker */
		remove (this->temptplfilename);
		g_free (this->temptplfilename);
	}


	/* deffilename should always be here (created in new) */
	g_return_if_fail (this->deffilename);
	remove (this->deffilename);
	g_free (this->deffilename);

	g_signal_handlers_disconnect_by_func (G_OBJECT (this->launcher), G_CALLBACK (on_autogen_terminated), this);
	g_object_unref (this->launcher);

	g_free (this);
}




