/*
    compile.c
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
#include "messages.h"
#include "compile.h"

#if 0

void
compile_file_with_make ()
{
	gchar* src_dir;

	if(app->project_dbase->is_saved == FALSE 
		&& app->project_dbase->project_is_open==TRUE)
	{
		gboolean ret;
		ret = project_dbase_save_project(app->project_dbase);
		if (ret == FALSE)
			return;
	}

	src_dir = project_dbase_get_module_dir (app->project_dbase, MODULE_SOURCE);
	if (src_dir)
	{
		gchar *prj_name, *cmd;
	
		cmd = command_editor_get_command (app->command_editor, COMMAND_BUILD_MODULE);
		if (cmd == NULL)
		{
			anjuta_warning (_("Unable to build module. Check Preferences->Commands."));
			g_free (src_dir);
			return;
		}
		chdir (src_dir);
		anjuta_set_execution_dir(src_dir);
		g_free (src_dir);
	
		if (launcher_execute
		    (cmd, build_mesg_arrived, build_mesg_arrived,
		     build_terminated) == FALSE)
		{
			g_free (cmd);
			return;
		}
		anjuta_update_app_status (TRUE, _("Build Project"));
		messages_clear (app->messages, MESSAGE_BUILD);
		messages_append (app->messages, _("Building source directory of the project: "),
				 MESSAGE_BUILD);
		prj_name = project_dbase_get_proj_name (app->project_dbase);
		messages_append (app->messages, prj_name, MESSAGE_BUILD);
		messages_append (app->messages, " ...\n", MESSAGE_BUILD);
		messages_append (app->messages, cmd, MESSAGE_BUILD);
		messages_append (app->messages, "\n", MESSAGE_BUILD);
		messages_show (app->messages, MESSAGE_BUILD);
		g_free (cmd);
		g_free (prj_name);
	}
	else
	{
		/* Single file build */
		build_file ();
	}
}

#endif

void
compile_file ()
{
	gchar *dirname, *cmd, *buff;
	TextEditor *te;

	te = anjuta_get_current_text_editor ();
	g_return_if_fail (te != NULL);
	
	if (te->full_filename == NULL)
	{
		anjuta_warning (_("This file is not saved. Save it first and then compile."));
		return;
	}
	if (text_editor_is_saved(te) == FALSE)
	{
		if (text_editor_save_file (te) == FALSE)
			return;
	}

#if 0 /* Skipping dependency check for file */
	filename = g_strdup (te->full_filename);
	ext = get_file_extension (filename);
	if (ext)
	{
		*(ext) = 'o';
		*(++ext) = '\0';
	}
	stat (te->full_filename, &st1);
	flg = stat (filename, &st2);
	if ((flg == 0) && (st2.st_mtime > st1.st_mtime))
	{
		anjuta_warning (_("The object file is up to date. No need to compile again."));
		g_free (filename);
		return;
	}
	g_free (filename);
#endif

	anjuta_set_file_properties (te->full_filename);
	cmd =
		command_editor_get_command_file (app->command_editor,
						 COMMAND_COMPILE_FILE,
						 te->filename);
	if (cmd == NULL)
	{
		anjuta_warning (_("I do not really know how to compile this file."));
		return;
	}
	dirname = g_dirname (te->full_filename);
	chdir (dirname);
	anjuta_set_execution_dir(dirname);
	g_free (dirname);

	if (launcher_execute (cmd, compile_mesg_arrived, compile_mesg_arrived, compile_terminated) == FALSE)
	{
		g_free (cmd);
		return;
	}
	anjuta_update_app_status (TRUE, _("Compile"));
	messages_clear (app->messages, MESSAGE_BUILD);
	buff = g_strdup_printf (_("Compiling file: %s ...\n"), te->filename);
	messages_append (app->messages, buff, MESSAGE_BUILD);
	messages_append (app->messages, cmd, MESSAGE_BUILD);
	messages_append (app->messages, "\n", MESSAGE_BUILD);
	messages_show (app->messages, MESSAGE_BUILD);
	g_free (buff);
	g_free (cmd);
}

void
compile_mesg_arrived (gchar * mesg)
{
	messages_append (app->messages, mesg, MESSAGE_BUILD);
}

void
compile_terminated (int status, time_t time)
{
	gchar *buff1;

	if (WEXITSTATUS (status))
	{
		messages_append (app->messages,
				 _("Compile completed ... unsuccessful"),
				 MESSAGE_BUILD);
		messages_append (app->messages, "\n", MESSAGE_BUILD);
		anjuta_warning (_("Compile completed ... unsuccessful"));
	}
	else
	{
		messages_append (app->messages,
				 _("Compile completed ... successful"),
				 MESSAGE_BUILD);
		messages_append (app->messages, "\n", MESSAGE_BUILD);
		anjuta_status (_("Compile completed ... successful"));
	}
	buff1 =
		g_strdup_printf (_("Total time taken: %d secs\n"),
				 (int) time);
	messages_append (app->messages, buff1, MESSAGE_BUILD);
	if (preferences_get_int (app->preferences, BEEP_ON_BUILD_COMPLETE))
		gdk_beep ();
	g_free (buff1);
	anjuta_update_app_status (TRUE, NULL);
}
