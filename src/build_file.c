/*
    build_file.c
    Copyright (C) 2000  Kh. Naba Kumar Singh

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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

#include <gnome.h>
#include "anjuta.h"
#include "launcher.h"
#include "message-manager.h"
#include "compile.h"
#include "build_file.h"

static void build_file_terminated (int status, time_t time);
static void build_file_mesg_arrived (gchar * mesg);

void
build_file ()
{
	gchar *dirname, *cmd, *buff;
	TextEditor *te;

	te = anjuta_get_current_text_editor ();
	if (!te)
		return;
	if (te->full_filename == NULL)
	{
		anjuta_warning (_("This file has not been saved. Save it first and then build."));
		return;
	}
	if (text_editor_is_saved (te) == FALSE)
	{
		if (text_editor_save_file (te, TRUE) == FALSE)
			return;
	}
	
#if 0 /* Skipping dependency check for file */
	if (TRUE)
	{
		gchar *ext, *filename;
		struct stat st1, st2;
		gint flg;

		filename = g_strdup (te->full_filename);
		ext = get_file_extension (filename);
		if (ext)
			*(--ext) = '\0';
		stat (te->full_filename, &st1);
		flg = stat (filename, &st2);
		if ((flg == 0) && (st2.st_mtime > st1.st_mtime))
		{
			anjuta_warning (_
					("The executable is up-to-date, there is no need to build it again."));
			g_free (filename);
			return;
		}
		g_free (filename);
	}
#endif
	
	anjuta_set_file_properties (te->full_filename);
	cmd =
		command_editor_get_command_file (app->command_editor,
						 COMMAND_BUILD_FILE,
						 te->filename);
	if (cmd == NULL) {
		anjuta_warning (_
				("No build command has been specified for this type of file."));
		return;
	}

	dirname = extract_directory (te->full_filename);
	chdir (dirname);
	anjuta_set_execution_dir(dirname);
	g_free (dirname);

	if (launcher_execute
	    (cmd, build_file_mesg_arrived, build_file_mesg_arrived,
	     build_file_terminated) == FALSE)
	{
		g_free (cmd);
		return;
	}
	anjuta_update_app_status (TRUE, _("Build"));
	an_message_manager_clear (app->messages, MESSAGE_BUILD);
	buff = g_strdup_printf (_("Building file: %s ...\n"), te->filename);
	an_message_manager_append (app->messages, buff, MESSAGE_BUILD);
	an_message_manager_append (app->messages, cmd, MESSAGE_BUILD);
	an_message_manager_append (app->messages, "\n", MESSAGE_BUILD);
	an_message_manager_show (app->messages, MESSAGE_BUILD);
	g_free (cmd);
	g_free (buff);
}

static void
build_file_mesg_arrived (gchar * mesg)
{
	an_message_manager_append (app->messages, mesg, MESSAGE_BUILD);
}

static void
build_file_terminated (int status, time_t time)
{
	gchar *buff1;

	buff1 =
		g_strdup_printf (_("Total time taken: %d secs\n"),
				 (int) time);
	if (status)
	{
		an_message_manager_append (app->messages,
				 _("Build completed ... unsuccessful\n"),
				 MESSAGE_BUILD);
		if (anjuta_preferences_get_int (ANJUTA_PREFERENCES (app->preferences),
										DIALOG_ON_BUILD_COMPLETE))
			anjuta_warning (_("Build completed ... unsuccessful"));
	}
	else
	{
		an_message_manager_append (app->messages,
				 _("Build completed ... successful\n"),
				 MESSAGE_BUILD);
		if (anjuta_preferences_get_int (ANJUTA_PREFERENCES (app->preferences),
										DIALOG_ON_BUILD_COMPLETE))
			anjuta_status (_("Build completed ... successful"));
	}
	an_message_manager_append (app->messages, buff1, MESSAGE_BUILD);
	if (anjuta_preferences_get_int (ANJUTA_PREFERENCES (app->preferences),
									BEEP_ON_BUILD_COMPLETE))
		gdk_beep ();
	g_free (buff1);
	anjuta_update_app_status (TRUE, NULL);
}
