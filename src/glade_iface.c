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

#define _GNU_SOURCE
#include <stdio.h>

#include "anjuta.h"
#include "project_dbase.h"
#include "glade_iface.h"

/* This function detects if the given glade file is for
   glade-2 (GNOME2) or glade (GNOME1) */
static gboolean
glade_is_glade_2 (const gchar *glade_file)
{
	FILE *fp;
	gchar *line = NULL;
	gboolean ret = FALSE;
	size_t size = 0;
	
	fp = fopen (glade_file, "r");
	g_return_val_if_fail (fp != NULL, FALSE);
	
	getline (&line, &size, fp);
	fclose (fp);
	g_return_val_if_fail (line != NULL, FALSE);
	if (strstr (line, "standalone"))
	{
		ret = TRUE;
	};
	g_free (line);
	
	return ret;
}

gboolean
glade_iface_generate_source_code(gchar* glade_file)
{
	gchar *dir;
	pid_t pid;
	int status;
	gboolean ret;
	gboolean glade_2;
	gchar * no_glade_msg;
	
	no_glade_msg = _("Warning: Can not generate c/c++ source code.\n"
	"Warning: It is because either glade (for gtk/gnome projects) or glademm (for "
	"gtkmm/gnomemm projects) is not installed.\n"
	"Warning: Your generated project will still be fine, though.\n");
	
	ret = TRUE;
	g_return_val_if_fail (glade_file != NULL, FALSE);

	glade_2 = glade_is_glade_2 (glade_file);

	if (!glade_2)
	{	
		switch (project_dbase_get_language (app->project_dbase))
		{
			case PROJECT_PROGRAMMING_LANGUAGE_C:
				if (anjuta_is_installed ("glade", FALSE) == FALSE)
				{
					an_message_manager_append (app->messages, no_glade_msg, MESSAGE_BUILD);
					return FALSE;
				}
				
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
				if((pid = fork ()) == 0)
				{
					execlp ("glade", "glade", "--write-source",  glade_file, NULL);
					g_warning (_("Cannot execute command: \"%s\""), "glade");
					_exit(1);
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
				if (anjuta_is_installed ("glade--", FALSE) == FALSE)
				{
					an_message_manager_append (app->messages, no_glade_msg, MESSAGE_BUILD);
					return FALSE;
				}
	
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
					g_warning (_("Cannot execute command: \"%s\""), "glade--");
					_exit(1);
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
	} 
	else 
	{
		switch (project_dbase_get_language (app->project_dbase))
		{
			case PROJECT_PROGRAMMING_LANGUAGE_C:
				if (anjuta_is_installed ("glade-2", FALSE) == FALSE)
				{
					an_message_manager_append (app->messages, no_glade_msg, MESSAGE_BUILD);
					return FALSE;
				}
				
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
				if((pid = fork ()) == 0)
				{
					execlp ("glade-2", "glade-2", "--write-source",  glade_file, NULL);
					g_warning (_("Cannot execute command: \"%s\""), "glade-2");
					_exit(1);
				}
				if(pid <1)
				{
					anjuta_system_error (errno, _("Cannot fork glade-2."));
					return FALSE;
				}
				waitpid (pid, &status, 0);
				if (WEXITSTATUS(status))
					ret = FALSE;
				g_free (dir);
				break;
	
			case PROJECT_PROGRAMMING_LANGUAGE_CPP:
			case PROJECT_PROGRAMMING_LANGUAGE_C_CPP:
				if (anjuta_is_installed ("glade--", FALSE) == FALSE)
				{
					an_message_manager_append (app->messages, no_glade_msg, MESSAGE_BUILD);
					return FALSE;
				}
	
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
					g_warning (_("Cannot execute command: \"%s\""), "glade--");
					_exit(1);
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
	}
	return ret;
}

gboolean
glade_iface_start_glade_editing (gchar* glade_file)
{
	gchar *dir;
	gboolean glade_2;
	pid_t pid;
	
	g_return_val_if_fail (glade_file != NULL, FALSE);
	glade_2 = glade_is_glade_2 (glade_file);

	if (glade_2)
	{
		if (anjuta_is_installed ("glade-2", TRUE) == FALSE)
			return FALSE;
	}
	else
	{
		if (anjuta_is_installed ("glade", TRUE) == FALSE)
			return FALSE;
	}
	dir = g_dirname (glade_file);
	/* gnome_execute_shell (dir, cmd) is evil. It overwrites SIGCHLD */;
	pid = fork();
	if (pid == 0)
	{
		if(dir)
		{
			force_create_dir (dir);
			chdir (dir);
		}
		if (glade_2)
		{
			execlp ("glade-2", "glade-2", glade_file, NULL);
			g_warning (_("Cannot execute command: \"%s\""), "glade-2");
		}
		else
		{
			execlp ("glade", "glade", glade_file, NULL);
			g_warning (_("Cannot execute command: \"%s\""), "glade");
		}
		_exit(1);
	}
	g_free (dir);
	return TRUE;
}
