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
#include "message-manager.h"
#include "compile.h"
#include "build_file.h"

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
			anjuta_warning (_("Unable to build module. Check Settings->Commands."));
			g_free (src_dir);
			return;
		}
		chdir (src_dir);
		anjuta_set_execution_dir(src_dir);
		g_free (src_dir);
	
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
		an_message_manager_append (app->messages, "...\n", MESSAGE_BUILD);
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

#endif

void
compile_file (gboolean use_make)
{
	gchar *dirname, *cmd, *buff;
	TextEditor *te;

	te = anjuta_get_current_text_editor ();
	g_return_if_fail (te != NULL);
	
	if (te->full_filename == NULL)
	{
		anjuta_warning (_("This file has not been saved. Save it first and then compile."));
		return;
	}
	if (text_editor_is_saved(te) == FALSE)
	{
		if (text_editor_save_file (te, TRUE) == FALSE)
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
		anjuta_warning (_("The object file is up-to-date, there is no need to compile it again."));
		g_free (filename);
		return;
	}
	g_free (filename);
#endif

	anjuta_set_file_properties (te->full_filename);
	cmd = command_editor_get_command_file (app->command_editor,
		use_make?COMMAND_MAKE_FILE:COMMAND_COMPILE_FILE, te->filename);
	if (cmd == NULL)
	{
		anjuta_warning (_("No compile command has been specified for this type of file."));
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
	anjuta_update_app_status (TRUE, _("Compile"));
	an_message_manager_clear (app->messages, MESSAGE_BUILD);
	buff = g_strdup_printf (_("Compiling file: %s...\n"), te->filename);
	an_message_manager_append (app->messages, buff, MESSAGE_BUILD);
	an_message_manager_append (app->messages, cmd, MESSAGE_BUILD);
	an_message_manager_append (app->messages, "\n", MESSAGE_BUILD);
	an_message_manager_show (app->messages, MESSAGE_BUILD);
	g_free (buff);
	g_free (cmd);
}
