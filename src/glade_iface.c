/*
 * glade_iface.c
 * Copyright (C) 2000  Kh. Naba Kumar Singh
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include <errno.h>
#include <sys/wait.h>

#include "anjuta.h"
#include "project_dbase.h"
#include "glade_iface.h"
#include "glades.h"
#include "CORBA-Server.h"

gboolean
glade_iface_generate_source_code(gchar* glade_file)
{
	gchar *dir;
	pid_t pid;
	int status;
	gboolean ret;

	ret = TRUE;
	g_return_val_if_fail (glade_file != NULL, FALSE);

	switch (project_dbase_get_language (app->project_dbase))
	{
		case PROJECT_PROGRAMMING_LANGUAGE_C:
			if( IsGladen() )
			{
				return gladen_write_source( glade_file );
			}
			if (anjuta_is_installed ("glade", TRUE) == FALSE)
				return FALSE;
			
			dir = g_dirname (glade_file);
			if(dir)
			{
				force_create_dir (dir);
				if (chdir (dir) < 0)
				{
					g_free (dir);
					return FALSE;
				}
			}	
			/* Cannot use gnome_execute_shell()*/
			if((pid = fork())==0)
			{
				execlp ("glade", "glade", "--write-source",  glade_file, NULL);
				g_error ("Cannot execute glade\n");
			}
			if(pid <1)
			{
				anjuta_system_error (errno, _("Cannot fork glade."));
				return FALSE;
			}
			waitpid (pid, &status, 0);
			if (WEXITSTATUS(status))
				ret = FALSE;
			g_free (dir);
			break;

		case PROJECT_PROGRAMMING_LANGUAGE_CPP:
		case PROJECT_PROGRAMMING_LANGUAGE_C_CPP:
			if (anjuta_is_installed ("glade--", TRUE) == FALSE)
				return FALSE;

			dir = g_dirname (glade_file);
			if(dir)
			{
				force_create_dir (dir);
				if (chdir (dir) < 0)
				{
					g_free (dir);
					return FALSE;
				}
			}
			g_free (dir);
			
			dir = project_dbase_get_module_name (app->project_dbase, MODULE_SOURCE);
			/* Cannot use gnome_execute_shell()*/
			if((pid = fork())==0)
			{
				execlp ("glade--", "glade--", "--noautoconf",
					"--directory", 
					dir, extract_filename(glade_file), NULL);
				g_error ("Cannot execute glade--\n");
			}
			if(pid <1)
			{
				anjuta_system_error (errno, _("Cannot fork glade--."));
				return FALSE;
			}
			waitpid (pid, &status, 0);
			if (WEXITSTATUS(status))
				ret = FALSE;
			g_free (dir);
			break;
		default:
			return FALSE;
	}
	return ret;
}

gboolean
glade_iface_start_glade_editing (gchar* glade_file)
{
	gchar *dir, *cmd;
	g_return_val_if_fail (glade_file != NULL, FALSE);

	if( IsGladen() )
		return gladen_load_project(glade_file);
	
	if (anjuta_is_installed ("glade", TRUE) == FALSE)
		return FALSE;
	
	dir = g_dirname (glade_file);
	if(dir)
	{
		force_create_dir (dir);
		if (chdir (dir) < 0)
		{
			g_free (dir);
			return FALSE;
		}
	}	
	cmd = g_strconcat ("glade ", glade_file, NULL);
	gnome_execute_shell (dir, cmd);
	g_free (dir);
	g_free (cmd);
	return TRUE;
}
