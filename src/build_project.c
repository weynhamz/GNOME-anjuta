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
#include "clean_project.h"

/* Private */
static void install_as_root (GtkWidget* button, gpointer data);
static void install_as_user (GtkWidget* button, gpointer data);

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
	
	/* perform a clean, if required before the build */
	if (app->project_dbase->clean_before_build == TRUE)
	{
		clean_project (build_project);
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
	
		if(anjuta_preferences_get_int(ANJUTA_PREFERENCES (app->preferences),
									  BUILD_OPTION_AUTOSAVE))
		{
			anjuta_save_all_files();
		}
	
		if (build_execute_command (cmd) == FALSE)
		{
			g_free (cmd);
			return;
		}
		anjuta_update_app_status (TRUE, _("Build Project"));
		an_message_manager_clear (app->messages, MESSAGE_BUILD);
		an_message_manager_append (app->messages, _("Building source directory of the Project: "),
				 MESSAGE_BUILD);
		prj_name = project_dbase_get_proj_name (app->project_dbase);
		an_message_manager_append (app->messages, prj_name, MESSAGE_BUILD);
		an_message_manager_append (app->messages, " ...\n", MESSAGE_BUILD);
		an_message_manager_append (app->messages, cmd, MESSAGE_BUILD);
		an_message_manager_append (app->messages, "\n", MESSAGE_BUILD);
		an_message_manager_show (app->messages, MESSAGE_BUILD);
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

	/* perform a clean, if required before the build */
	if (app->project_dbase->clean_before_build == TRUE)
	{
		clean_project (build_all_project);
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
		
		if(anjuta_preferences_get_int (ANJUTA_PREFERENCES (app->preferences),
									   BUILD_OPTION_AUTOSAVE))
			anjuta_save_all_files();
	
		if (build_execute_command (cmd) == FALSE)
		{
			g_free (cmd);
			return;
		}
		anjuta_update_app_status (TRUE, _("Build Project"));
		an_message_manager_clear (app->messages, MESSAGE_BUILD);
		an_message_manager_append (app->messages,
				 _("Building the whole Project: "),
				 MESSAGE_BUILD);
		prj_name = project_dbase_get_proj_name (app->project_dbase);
		an_message_manager_append (app->messages, prj_name, MESSAGE_BUILD);
		an_message_manager_append (app->messages, " ...\n", MESSAGE_BUILD);
		an_message_manager_append (app->messages, cmd, MESSAGE_BUILD);
		an_message_manager_append (app->messages, "\n", MESSAGE_BUILD);
		an_message_manager_show (app->messages, MESSAGE_BUILD);
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
		
		if(anjuta_preferences_get_int (ANJUTA_PREFERENCES (app->preferences),
									   BUILD_OPTION_AUTOSAVE))
			anjuta_save_all_files();

		if (build_execute_command (cmd) == FALSE)
		{
			g_free (cmd);
			return;
		}
		anjuta_update_app_status (TRUE, _("Build Distribution"));
		an_message_manager_clear (app->messages, MESSAGE_BUILD);
		an_message_manager_append (app->messages,
				 _
				 ("Building the distribution package of the Project: "),
				 MESSAGE_BUILD);
		prj_name = project_dbase_get_proj_name (app->project_dbase);
		an_message_manager_append (app->messages, prj_name, MESSAGE_BUILD);
		an_message_manager_append (app->messages, " ...\n", MESSAGE_BUILD);
		an_message_manager_append (app->messages, cmd, MESSAGE_BUILD);
		an_message_manager_append (app->messages, "\n", MESSAGE_BUILD);
		an_message_manager_show (app->messages, MESSAGE_BUILD);
		g_free (cmd);
		g_free (prj_name);
	}
}

void
build_install_project ()
{
	if(app->project_dbase->is_saved == FALSE 
		&& app->project_dbase->project_is_open==TRUE)
	{
		gboolean ret;
		ret = project_dbase_save_project(app->project_dbase);
		if (ret == FALSE)
			return;
	}

	if (app->project_dbase->project_is_open) {
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new (GTK_WINDOW (app->widgets.window),
						 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_QUESTION,
						 GTK_BUTTONS_YES_NO,
						 _("Do you prefer installing as root ?"));

		if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
			install_as_root (NULL, NULL);
		else
			install_as_user (NULL, NULL);
		gtk_widget_destroy(dialog);
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
		
		if (anjuta_preferences_get_int (ANJUTA_PREFERENCES (app->preferences),
										BUILD_OPTION_AUTOSAVE))
			anjuta_save_all_files();
	
		if (build_execute_command (cmd) == FALSE)
		{
			g_free (cmd);
			return;
		}
		anjuta_update_app_status (TRUE, _("Auto generate Project"));
		an_message_manager_clear (app->messages, MESSAGE_BUILD);
		an_message_manager_append (app->messages,
				 _("Auto generating the Project: "),
				 MESSAGE_BUILD);
		prj_name = project_dbase_get_proj_name (app->project_dbase);
		an_message_manager_append (app->messages, prj_name, MESSAGE_BUILD);
		an_message_manager_append (app->messages, " ...\n", MESSAGE_BUILD);
		an_message_manager_append (app->messages, cmd, MESSAGE_BUILD);
		an_message_manager_append (app->messages, "\n", MESSAGE_BUILD);
		an_message_manager_show (app->messages, MESSAGE_BUILD);
		g_free (cmd);
		g_free (prj_name);
	}
}

static void
install_as_root (GtkWidget* button, gpointer data)
{
	gchar *cmd, *build_cmd, *prj_name;
	build_cmd = command_editor_get_command (app->command_editor,
											COMMAND_BUILD_INSTALL);
	if (build_cmd == NULL)
	{
		anjuta_warning (_
				("Unable to install Project. Check Settings->Commands."));
		return;
	}
	cmd = g_strconcat ("su --command='", build_cmd, "'", NULL);
	g_free(build_cmd);
	
	chdir (app->project_dbase->top_proj_dir);
	anjuta_set_execution_dir(app->project_dbase->top_proj_dir);
	
	if(anjuta_preferences_get_int (ANJUTA_PREFERENCES (app->preferences),
								   BUILD_OPTION_AUTOSAVE))
	{
		anjuta_save_all_files();
	}

	anjuta_update_app_status (TRUE, _("Install Project"));
	an_message_manager_clear (app->messages, MESSAGE_BUILD);
	an_message_manager_append (app->messages, _("Installing the Project: "),
							   MESSAGE_BUILD);
	prj_name = project_dbase_get_proj_name (app->project_dbase);
	an_message_manager_append (app->messages, prj_name, MESSAGE_BUILD);
	an_message_manager_append (app->messages, " ...\n", MESSAGE_BUILD);
	an_message_manager_append (app->messages, cmd, MESSAGE_BUILD);
	an_message_manager_append (app->messages, "\n", MESSAGE_BUILD);
	an_message_manager_show (app->messages, MESSAGE_BUILD);
	g_free (prj_name);
	build_execute_command (cmd);
	g_free (cmd);
}

static void
install_as_user (GtkWidget* button, gpointer data)
{
	gchar *cmd, *prj_name;
	cmd = command_editor_get_command (app->command_editor, COMMAND_BUILD_INSTALL);
	if (cmd == NULL)
	{
		anjuta_warning (_
				("Unable to install Project. Check Settings->Commands."));
		return;
	}
	chdir (app->project_dbase->top_proj_dir);
	anjuta_set_execution_dir(app->project_dbase->top_proj_dir);
	
	if(anjuta_preferences_get_int (ANJUTA_PREFERENCES (app->preferences),
								   BUILD_OPTION_AUTOSAVE))
	{
		anjuta_save_all_files();
	}

	anjuta_update_app_status (TRUE, _("Install Project"));
	an_message_manager_clear (app->messages, MESSAGE_BUILD);
	an_message_manager_append (app->messages, _("Installing the Project: "),
			 MESSAGE_BUILD);
	prj_name = project_dbase_get_proj_name (app->project_dbase);
	an_message_manager_append (app->messages, prj_name, MESSAGE_BUILD);
	an_message_manager_append (app->messages, " ...\n", MESSAGE_BUILD);
	an_message_manager_append (app->messages, cmd, MESSAGE_BUILD);
	an_message_manager_append (app->messages, "\n", MESSAGE_BUILD);
	an_message_manager_show (app->messages, MESSAGE_BUILD);
	g_free (prj_name);
	build_execute_command (cmd);
	g_free (cmd);
}
