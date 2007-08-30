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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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

	if (build_execute_command (cmd) == FALSE)
	{
		g_free (cmd);
		return;
	}
	anjuta_update_app_status (TRUE, _("Build"));
	an_message_manager_clear (app->messages, MESSAGE_BUILD);
	buff = g_strdup_printf (_("Building file: %s...\n"), te->filename);
	an_message_manager_append (app->messages, buff, MESSAGE_BUILD);
	an_message_manager_append (app->messages, cmd, MESSAGE_BUILD);
	an_message_manager_append (app->messages, "\n", MESSAGE_BUILD);
	an_message_manager_show (app->messages, MESSAGE_BUILD);
	g_free (cmd);
	g_free (buff);
}

/* Launching the command */
static void
on_build_mesg_arrived (AnjutaLauncher *launcher,
					   AnjutaLauncherOutputType output_type,
					   const gchar * mesg, gpointer data)
{
	an_message_manager_append (app->messages, mesg, MESSAGE_BUILD);
}

static void
on_build_terminated (AnjutaLauncher *launcher,
					 gint child_pid, gint status, gulong time_taken,
					 gpointer data)
{
	gchar *buff1;

	g_signal_handlers_disconnect_by_func (launcher,
										  G_CALLBACK (on_build_terminated),
										  data);
	buff1 = g_strdup_printf (_("Total time taken: %lu secs\n"), time_taken);
	if (status)
	{
		an_message_manager_append (app->messages,
								   _("Completed... unsuccessful\n"),
								   MESSAGE_BUILD);
		if (anjuta_preferences_get_int (ANJUTA_PREFERENCES (app->preferences),
										DIALOG_ON_BUILD_COMPLETE))
			anjuta_warning (_("Completed... unsuccessful"));
	}
	else
	{
		an_message_manager_append (app->messages,
								   _("Completed... successful\n"),
								   MESSAGE_BUILD);
		if (anjuta_preferences_get_int (ANJUTA_PREFERENCES (app->preferences),
										DIALOG_ON_BUILD_COMPLETE))
			anjuta_status (_("Completed... successful"));
	}
	an_message_manager_append (app->messages, buff1, MESSAGE_BUILD);
	if (anjuta_preferences_get_int (ANJUTA_PREFERENCES (app->preferences),
									BEEP_ON_BUILD_COMPLETE))
		gdk_beep ();
	g_free (buff1);
	anjuta_update_app_status (TRUE, NULL);
}

gboolean
build_execute_command (const gchar *command)
{
	g_signal_connect (G_OBJECT (app->launcher), "child-exited",
					  G_CALLBACK (on_build_terminated), NULL);
	return anjuta_launcher_execute (app->launcher, command,
									on_build_mesg_arrived, NULL);
}
