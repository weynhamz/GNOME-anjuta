/*
    clean_project.c
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
#include <time.h>

#include <gnome.h>
#include "anjuta.h"
#include "build_file.h"
#include "launcher.h"
#include "clean_project.h"
#include "build_project.h"


void
clean_project ()
{
	gchar *cmd, *prj_name;

	if(app->project_dbase->is_saved == FALSE 
		&& app->project_dbase->project_is_open==TRUE)
	{
		gboolean ret;
		ret = project_dbase_save_project(app->project_dbase);
		if (ret == FALSE)
			return;
	}

	if (app->project_dbase->project_is_open)
	{
		cmd =
			command_editor_get_command (app->command_editor,
						    COMMAND_BUILD_CLEAN);
		if (cmd == NULL)
		{
			anjuta_warning (_
					("Unable to clean Project. Check Settings->Commands."));
			return;
		}
		chdir (app->project_dbase->top_proj_dir);
		if (launcher_execute (cmd, clean_mesg_arrived,
				      clean_mesg_arrived,
				      clean_terminated) == FALSE)
		{
			g_free (cmd);
			return;
		}
		anjuta_update_app_status (TRUE, _("Clean"));
		anjuta_message_manager_clear (app->messages, MESSAGE_BUILD);
		anjuta_message_manager_append (app->messages,
				 _("Cleaning the source directory of the Project: "),
				 MESSAGE_BUILD);
		prj_name = project_dbase_get_proj_name (app->project_dbase);
		anjuta_message_manager_append (app->messages, prj_name, MESSAGE_BUILD);
		anjuta_message_manager_append (app->messages, " ...\n", MESSAGE_BUILD);
		anjuta_message_manager_append (app->messages, cmd, MESSAGE_BUILD);
		anjuta_message_manager_append (app->messages, "\n", MESSAGE_BUILD);
		anjuta_message_manager_append (app->messages, "", MESSAGE_BUILD); // Maybe something is missing here
		g_free (cmd);
		g_free (prj_name);
	}
}

void
clean_mesg_arrived (gchar * mesg)
{
	anjuta_message_manager_append (app->messages, mesg, MESSAGE_BUILD);
}

void
clean_terminated (int status, time_t time)
{
	gchar *buff1;

	if (WEXITSTATUS (status))
	{
		anjuta_message_manager_append (app->messages,
				 _
				 ("Cleaning completed...............Unsuccessful\n"),
				 MESSAGE_BUILD);
		anjuta_warning (_("Cleaning completed ... unsuccessful"));
	}
	else
	{
		anjuta_message_manager_append (app->messages,
				 _
				 ("Cleaning completed...............Successful\n"),
				 MESSAGE_BUILD);
		anjuta_status (_("Cleaning completed ... successful"));
	}
	buff1 =
		g_strdup_printf (_("Total time taken: %d secs\n"),
				 (gint) time);
	anjuta_message_manager_append (app->messages, buff1, MESSAGE_BUILD);
	if (preferences_get_int (app->preferences, BEEP_ON_BUILD_COMPLETE))
		gdk_beep ();
	g_free (buff1);
	anjuta_update_app_status (TRUE, NULL);
}

void
clean_all_project ()
{
	gchar *cmd, *prj_name;

	if(app->project_dbase->is_saved == FALSE 
		&& app->project_dbase->project_is_open==TRUE)
	{
		gboolean ret;
		ret = project_dbase_save_project(app->project_dbase);
		if (ret == FALSE)
			return;
	}

	if (app->project_dbase->project_is_open)
	{
		cmd =
			command_editor_get_command (app->command_editor,
						    COMMAND_BUILD_CLEAN_ALL);
		if (cmd == NULL)
		{
			anjuta_warning (_
					("Unable to Clean All for the Project. Check Settings->Commands."));
			return;
		}
		chdir (app->project_dbase->top_proj_dir);
		if (launcher_execute (cmd, clean_mesg_arrived,
				      clean_mesg_arrived,
				      clean_all_terminated) == FALSE)
		{
			g_free (cmd);
			return;
		}
		anjuta_update_app_status (TRUE, _("Clean All"));
		anjuta_message_manager_clear (app->messages, MESSAGE_BUILD);
		anjuta_message_manager_append (app->messages,
				 _("Cleaning whole of the Project: "),
				 MESSAGE_BUILD);
		prj_name = project_dbase_get_proj_name (app->project_dbase);
		anjuta_message_manager_append (app->messages, prj_name, MESSAGE_BUILD);
		anjuta_message_manager_append (app->messages, " ...\n", MESSAGE_BUILD);
		anjuta_message_manager_append (app->messages, cmd, MESSAGE_BUILD);
		anjuta_message_manager_append (app->messages, "\n", MESSAGE_BUILD);
		anjuta_message_manager_show (app->messages, MESSAGE_BUILD);
		g_free (cmd);
		g_free (prj_name);
	}
}

void
clean_all_terminated (int status, time_t time)
{
	gchar *buff1;

	if (WEXITSTATUS (status))
	{
		anjuta_message_manager_append (app->messages,
				 _
				 ("Clean All completed...............Unsuccessful\n"),
				 MESSAGE_BUILD);
		anjuta_warning (_("Clean All completed ... unsuccessful"));
	}
	else
	{
		anjuta_message_manager_append (app->messages,
				 _
				 ("Clean All completed...............Successful\n"),
				 MESSAGE_BUILD);
		anjuta_status (_("Clean All completed ... successful"));
	}
	buff1 =
		g_strdup_printf (_("Total time taken: %d secs\n"),
				 (gint) time);
	anjuta_message_manager_append (app->messages, buff1, MESSAGE_BUILD);
	if (preferences_get_int (app->preferences, BEEP_ON_BUILD_COMPLETE))
		gdk_beep ();
	g_free (buff1);
	anjuta_update_app_status (TRUE, NULL);
}
