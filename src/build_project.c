/*
    build_project.c
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
#include <errno.h>

#include <gnome.h>
#include "anjuta.h"
#include "build_file.h"
#include "launcher.h"
#include "build_project.h"

void
build_project ()
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
			anjuta_warning (_("Unable to build module. Check Settings->Commands."));
			g_free (src_dir);
			return;
		}
		chdir (src_dir);
		anjuta_set_execution_dir(src_dir);
		g_free (src_dir);
	
		if(preferences_get_int(app->preferences, BUILD_OPTION_AUTOSAVE))
		{
			anjuta_save_all_files();
		}
	
		if (launcher_execute
		    (cmd, build_mesg_arrived, build_mesg_arrived,
		     build_terminated) == FALSE)
		{
			g_free (cmd);
			return;
		}
		anjuta_update_app_status (TRUE, _("Build Project"));
		anjuta_message_manager_clear (app->messages, MESSAGE_BUILD);
		anjuta_message_manager_append (app->messages, _("Building source directory of the Project: "),
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
	else
	{
		/* Single file build */
		build_file ();
	}
}

void
build_all_project ()
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
		cmd = command_editor_get_command (app->command_editor, COMMAND_BUILD_PROJECT);
		if (cmd == NULL)
		{
			anjuta_warning (_("Unable to build Project. Check Settings->Commands."));
			return;
		}
		chdir (app->project_dbase->top_proj_dir);
		anjuta_set_execution_dir(app->project_dbase->top_proj_dir);
		
		if(preferences_get_int(app->preferences, BUILD_OPTION_AUTOSAVE))
		{
			anjuta_save_all_files();
		}
	
		if (launcher_execute
		    (cmd, build_mesg_arrived, build_mesg_arrived,
		     build_all_terminated) == FALSE)
		{
			g_free (cmd);
			return;
		}
		anjuta_update_app_status (TRUE, _("Build Project"));
		anjuta_message_manager_clear (app->messages, MESSAGE_BUILD);
		anjuta_message_manager_append (app->messages,
				 _("Building the whole Project: "),
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
build_dist_project ()
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
		cmd = command_editor_get_command (app->command_editor, COMMAND_BUILD_TARBALL);
		if (cmd == NULL)
		{
			anjuta_warning (_
					("Unable to build tarball. Check Settings->Commands."));
			return;
		}
		chdir (app->project_dbase->top_proj_dir);
		anjuta_set_execution_dir(app->project_dbase->top_proj_dir);
		
		if(preferences_get_int(app->preferences, BUILD_OPTION_AUTOSAVE))
		{
			anjuta_save_all_files();
		}

		if (launcher_execute
		    (cmd, build_mesg_arrived, build_mesg_arrived,
		     build_dist_terminated) == FALSE)
		{
			g_free (cmd);
			return;
		}
		anjuta_update_app_status (TRUE, _("Build Distribution"));
		anjuta_message_manager_clear (app->messages, MESSAGE_BUILD);
		anjuta_message_manager_append (app->messages,
				 _
				 ("Building the distribution package of the Project: "),
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
build_install_project ()
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
		cmd = command_editor_get_command (app->command_editor, COMMAND_BUILD_INSTALL);
		if (cmd == NULL)
		{
			anjuta_warning (_
					("Unable to install Project. Check Settings->Commands."));
			return;
		}
		chdir (app->project_dbase->top_proj_dir);
		anjuta_set_execution_dir(app->project_dbase->top_proj_dir);
		
		if(preferences_get_int(app->preferences, BUILD_OPTION_AUTOSAVE))
		{
			anjuta_save_all_files();
		}
	
		if (launcher_execute
		    (cmd, build_mesg_arrived, build_mesg_arrived,
		     build_install_terminated) == FALSE)
		{
			g_free (cmd);
			return;
		}
		anjuta_update_app_status (TRUE, _("Install Project"));
		anjuta_message_manager_clear (app->messages, MESSAGE_BUILD);
		anjuta_message_manager_append (app->messages, _("Installing the Project: "),
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
build_autogen_project ()
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
		chdir (app->project_dbase->top_proj_dir);
		anjuta_set_execution_dir(app->project_dbase->top_proj_dir);
		if (file_is_executable ("./autogen.sh"))
		{
			cmd = g_strdup ("./autogen.sh");
		}
		else
		{
			cmd = command_editor_get_command (app->command_editor, COMMAND_BUILD_AUTOGEN);
			if (cmd == NULL)
			{
				anjuta_warning (_
						("Unable to auto generate. Check Settings->Commands."));
				return;
			}
		}
		
		if(preferences_get_int(app->preferences, BUILD_OPTION_AUTOSAVE))
		{
			anjuta_save_all_files();
		}
	
		if (launcher_execute
		    (cmd, build_mesg_arrived, build_mesg_arrived,
		     build_autogen_terminated) == FALSE)
		{
			g_free (cmd);
			return;
		}
		anjuta_update_app_status (TRUE, _("Auto generate Project"));
		anjuta_message_manager_clear (app->messages, MESSAGE_BUILD);
		anjuta_message_manager_append (app->messages,
				 _("Auto generating the Project: "),
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
build_mesg_arrived (gchar * mesg)
{
	anjuta_message_manager_append (app->messages, mesg, MESSAGE_BUILD);
}

void
build_terminated (int status, time_t time)
{
	gchar *buff1;

	if (WEXITSTATUS (status))
	{
		anjuta_message_manager_append (app->messages,
				 _
				 ("Build completed...............Unsuccessful\n"),
				 MESSAGE_BUILD);
		if (preferences_get_int (app->preferences, DIALOG_ON_BUILD_COMPLETE))
			anjuta_warning (_("Build terminated with error(s)."));
	}
	else
	{
		anjuta_message_manager_append (app->messages,
				 _
				 ("Build completed...............Successful\n"),
				 MESSAGE_BUILD);
		if (preferences_get_int (app->preferences, DIALOG_ON_BUILD_COMPLETE))
			anjuta_status (_("Build completed ... successful."));
	}
	buff1 =
		g_strdup_printf (_("Total time taken: %d secs\n"),
				 (gint) time);
	anjuta_message_manager_append (app->messages, buff1, MESSAGE_BUILD);
	if (preferences_get_int (app->preferences, BEEP_ON_BUILD_COMPLETE))
		gdk_beep ();
	anjuta_update_app_status (TRUE, NULL);
}

void
build_all_terminated (int status, time_t time)
{
	gchar *buff1;

	if (WEXITSTATUS (status))
	{
		anjuta_message_manager_append (app->messages,
				 _
				 ("Build All completed...............Unsuccessful\n"),
				 MESSAGE_BUILD);
		if (preferences_get_int (app->preferences, DIALOG_ON_BUILD_COMPLETE))
			anjuta_warning (_("Build All completed...............Unsuccessful"));
	}
	else
	{
		anjuta_message_manager_append (app->messages,
				 _("Build All completed...............Successful\n"),
				 MESSAGE_BUILD);
		if (preferences_get_int (app->preferences, DIALOG_ON_BUILD_COMPLETE))
			anjuta_status (_("Build All completed ... successful"));
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
build_dist_terminated (int status, time_t time)
{
	gchar *buff1;

	if (WEXITSTATUS (status))
	{
		anjuta_message_manager_append (app->messages,
				 _("Build Distribution completed...............Unsuccessful\n"),
				 MESSAGE_BUILD);
		if (preferences_get_int (app->preferences, DIALOG_ON_BUILD_COMPLETE))
			anjuta_warning (_("Build Distribution completed ... unsuccessful"));
	}
	else
	{
		anjuta_message_manager_append (app->messages,
				 _("Build Distribution completed...............Successful\n"),
				 MESSAGE_BUILD);
		anjuta_message_manager_append (app->messages,
				 _("The source tarball can be found in the top level directory of the project.\n"),
				 MESSAGE_BUILD);
		if (preferences_get_int (app->preferences, DIALOG_ON_BUILD_COMPLETE))
			anjuta_status (_("Build Distribution completed ... successful"));
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
build_install_terminated (int status, time_t time)
{
	gchar *buff1;

	if (WEXITSTATUS (status))
	{
		anjuta_message_manager_append (app->messages,
				 _("Install completed...............Unsuccessful\n"),
				 MESSAGE_BUILD);
		if (preferences_get_int (app->preferences, DIALOG_ON_BUILD_COMPLETE))
			anjuta_warning (_("Install completed...............Unsuccessful"));
	}
	else
	{
		anjuta_message_manager_append (app->messages,
				 _("Install completed...............Successful\n"),
				 MESSAGE_BUILD);
		if (preferences_get_int (app->preferences, DIALOG_ON_BUILD_COMPLETE))
			anjuta_status (_("Install completed ... successful"));
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
build_autogen_terminated (int status, time_t time)
{
	gchar *buff1;

	if (WEXITSTATUS (status))
	{
		anjuta_message_manager_append (app->messages,
				 _("Auto generation completed...............Unsuccessful\n"),
				 MESSAGE_BUILD);
		if (preferences_get_int (app->preferences, DIALOG_ON_BUILD_COMPLETE))
			anjuta_warning (_("Auto generation completed ... unsuccessful"));
	}
	else
	{
		anjuta_message_manager_append (app->messages,
				 _("Auto generation completed...............Successful\nNow Configure the Project.\n"),
				 MESSAGE_BUILD);
		if (preferences_get_int (app->preferences, DIALOG_ON_BUILD_COMPLETE))
			anjuta_status (_("Auto generation completed ... successful"));
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
