/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    tar.c
    Copyright (C) 2010 Sebastien Granjoux

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

#include <config.h>

#include "tar.h"

#include <libanjuta/interfaces/ianjuta-wizard.h>

/*---------------------------------------------------------------------------*/

#define TMP_DEF_FILENAME "NPWDEFXXXXXX"
#define TMP_TPL_FILENAME "NPWTPLXXXXXX"

#define FILE_BUFFER_SIZE	4096

/* Type
 *---------------------------------------------------------------------------*/

typedef struct _NPWTarPacket NPWTarPacket;

struct _NPWTarPacket
{
	gint stdout;
	gint stderr;
	gpointer callback;
	gpointer data;
	GFile *tarfile;
	GFile *destination;
};
 
/* Helper functions
 *---------------------------------------------------------------------------*/

/* Tar Packet object
 *---------------------------------------------------------------------------*/

static void
npw_tar_packet_free (NPWTarPacket *pack)
{
	g_object_unref (pack->tarfile);
	if (pack->destination) g_object_unref (pack->destination);
	g_free (pack);
}
 
/* Public functions
 *---------------------------------------------------------------------------*/

static void
on_tar_listed (GPid pid,
				gint status,
				gpointer data)
{
	NPWTarPacket *pack = (NPWTarPacket *)data;

	if (pack->callback != NULL)
	{
		GIOChannel *output;
		GList *list;
		GString *line;
		GIOStatus status;
	
		list = NULL;
		line = g_string_new (NULL);
		output = g_io_channel_unix_new (pack->stdout);
		do
		{
			gsize terminator;
		
			status = g_io_channel_read_line_string (output, line, &terminator, NULL);
			if (status == G_IO_STATUS_NORMAL)
			{
				g_string_truncate (line, terminator);
				
				list = g_list_prepend (list, g_strdup (line->str));
				continue;
			}
		}
		while (status == G_IO_STATUS_AGAIN);
		g_io_channel_close (output);
		g_string_free (line, TRUE);
		
		list = g_list_reverse (list);
		((NPWTarListFunc)pack->callback) (pack->tarfile, list, pack->data, NULL);
		
		g_list_foreach (list, (GFunc)g_free, NULL);
		g_list_free (list);
	}

	g_spawn_close_pid(pid);
} 

static void
on_tar_completed (GPid pid,
				gint status,
				gpointer data)
{
	NPWTarPacket *pack = (NPWTarPacket *)data;

	if (pack->callback != NULL)
	{
		GError *error = NULL;
		if (status != 0)
		{
			GIOChannel *stderr;
			gchar *message;
			gsize length;
			
			stderr = g_io_channel_unix_new (pack->stderr);
			g_io_channel_read_to_end (stderr, &message, &length, &error);
			if (error != NULL)
			{
				error = g_error_new_literal (IANJUTA_WIZARD_ERROR, 0, message);
			}
			g_io_channel_close (stderr);
		}
			
		((NPWTarCompleteFunc)pack->callback) (pack->destination, pack->tarfile, pack->data, error);
		g_clear_error (&error);
	}

	g_spawn_close_pid(pid);
}
 
/* Public functions
 *---------------------------------------------------------------------------*/

gboolean
npw_tar_list (GFile *tarfile, NPWTarListFunc list, gpointer data, GError **error)
{
	gchar *argv [] = {"tar", "--force-local", "--no-wildcards", "-tzf", NULL, NULL};
	gchar *prog;
	gchar *filename;
	GPid pid;
	gboolean ok;
	NPWTarPacket *pack;
	
	/* Use gtar if available */
	prog = g_find_program_in_path ("gtar");
	filename = g_file_get_path (tarfile);
	argv[4] = filename;

	/* Execute tar command asynchronously */
	pack = g_new0(NPWTarPacket, 1);
	pack->callback = (gpointer)list;
	pack->data = data;
	pack->tarfile = g_object_ref (tarfile);
	ok = g_spawn_async_with_pipes (NULL, argv, NULL, 
				G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH | G_SPAWN_STDERR_TO_DEV_NULL,
				NULL, NULL,
				&pid,
				NULL,
				&pack->stdout,
				NULL,
				error);
	
	if (ok)
	{
		g_child_watch_add_full (G_PRIORITY_HIGH_IDLE, pid, on_tar_listed, pack, (GDestroyNotify)npw_tar_packet_free);
	}
	
	g_free (filename);
	g_free (prog);
	
	return ok;
}
  
gboolean
npw_tar_extract (GFile *destination, GFile *tarfile, NPWTarCompleteFunc complete, gpointer data, GError **error)
{
	gchar *argv [] = {"tar", "--force-local", "--no-wildcards", "-C", NULL, "-xzf", NULL, NULL};
	gchar *prog;
	gchar *filename;
	gchar *dirname;
	GPid pid;
	gboolean ok;
	NPWTarPacket *pack;
	
	/* Use gtar if available */
	prog = g_find_program_in_path ("gtar");
	dirname = g_file_get_path (destination);
	argv[4] = dirname;
	filename = g_file_get_path (tarfile);
	argv[6] = filename;

	/* Execute tar command asynchronously */
	pack = g_new0(NPWTarPacket, 1);
	pack->callback = (gpointer)complete;
	pack->data = data;
	pack->tarfile = g_object_ref (tarfile);
	pack->destination = g_object_ref (destination);
	ok = g_spawn_async_with_pipes (NULL, argv, NULL, 
				G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH,
				NULL, NULL,
				&pid,
				NULL,
				NULL,
				&pack->stderr,
				error);
	
	if (ok)
	{
		g_child_watch_add_full (G_PRIORITY_HIGH_IDLE, pid, on_tar_completed, pack, (GDestroyNotify)npw_tar_packet_free);
	}
	
	g_free (filename);
	g_free (dirname);
	g_free (prog);
	
	return ok;
}

