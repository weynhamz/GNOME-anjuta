/*
 * compatibility.c
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

#include <stdio.h>
#include <dirent.h>
#include <gnome.h>

#include "anjuta.h"
#include "utilities.h"
#include "compatibility_0.h"

static gboolean
compatibility_0_compiler_options_load (PropsID props, FILE* s)
{
	gchar *buff;
	gint length, i;
	GList *list;

	g_return_val_if_fail (s != NULL, FALSE);

	if (fscanf (s, "Include Paths Length = %d\n", &length) < 1)
		return FALSE;
	list = NULL;
	for (i = 0; i < length; i++)
	{
		if (!read_string (s, "Include_Path", &buff))
		{
			glist_strings_free (list);
			return FALSE;
		}
		list = g_list_append (list, buff);
	}
	buff = string_from_glist (list);
	glist_strings_free  (list);
	prop_set_with_key (props, "compiler.options.include.paths", buff);
	g_free (buff);

	if (fscanf (s, "Library Paths Length = %d\n", &length) < 1)
		return FALSE;
	list = NULL;
	for (i = 0; i < length; i++)
	{
		if (!read_string (s, "Library_Path", &buff))
		{
			glist_strings_free (list);
			return FALSE;
		}
		list = g_list_append (list, buff);
	}
	buff = string_from_glist (list);
	glist_strings_free  (list);
	prop_set_with_key (props, "compiler.options.library.paths", buff);
	g_free (buff);

	if (fscanf (s, "Libraries Length = %d\n", &length) < 1)
		return FALSE;
	list = NULL;
	for (i = 0; i < length; i++)
	{
		if (!read_string (s, "Library", &buff))
		{
			glist_strings_free (list);
			return FALSE;
		}
		list = g_list_append (list, buff);
	}
	buff = string_from_glist (list);
	glist_strings_free  (list);
	prop_set_with_key (props, "compiler.options.libraries", buff);
	g_free (buff);

	if (fscanf (s, "Defines Length = %d\n", &length) < 1)
		return FALSE;
	list = NULL;
	for (i = 0; i < length; i++)
	{
		if (!read_string (s, "Define", &buff))
		{
			glist_strings_free (list);
			return FALSE;
		}
		list = g_list_append (list, buff);
	}
	buff = string_from_glist (list);
	glist_strings_free  (list);
	prop_set_with_key (props, "compiler.options.defines", buff);
	g_free (buff);

	list = NULL;
	for (i = 0; i < 26; i++)
	{
		gint state;

		if (fscanf (s, "Button State: %d\n", &state) < 1)
		{
			glist_strings_free (list);
			return FALSE;
		}
		if (state)
			list = g_list_append (list, g_strdup("1"));
		else
			list = g_list_append (list, g_strdup("0"));
	}
	buff = string_from_glist (list);
	glist_strings_free  (list);
	/* Rather complex to convert this. Disabled for the time being. */
	/*	prop_set_with_key (props, "compiler.options.warning.buttons", buff); */
	g_free (buff);

	if (read_string (s, "Other_Options", &buff) == FALSE)
		return FALSE;
	prop_set_with_key (props, "compiler.options.other.c.options", buff);
	g_free (buff);
	return TRUE;
}

static gboolean
compatibility_0_src_paths_load (PropsID props, FILE* s)
{
	gchar *buff;
	gint length, i;
	GList* list;
	
	g_return_val_if_fail (s != NULL, FALSE);

	if (fscanf (s, "Source Paths Length = %d\n", &length) < 1)
		return FALSE;
	list = NULL;
	for (i = 0; i < length; i++)
	{
		if (!read_string (s, "Src_Path", &buff))
		{
			glist_strings_free (list);
			return FALSE;
		}
		list = g_list_append (list, buff);
	}
	buff = string_from_glist (list);
	glist_strings_free  (list);
	prop_set_with_key (props, "project.source.paths", buff);
	g_free (buff);
	return TRUE;
}

gboolean compatibility_0_load_project (ProjectDBase * p, FILE * fp)
{
	gchar buff[256], *temp, *key;
	gint tmp, i;
	GList *files;

	if (fscanf (fp, "Project Type: %d \n", &tmp) < 1)
		return FALSE;
	/* blank = 0, console = 1,gnome_blank = 2, gnome_full = 3 */
	switch (tmp)
	{
		case 0:
		case 1:
			prop_set_with_key (p->props, "project.type", "GENERIC");
			prop_set_with_key (p->props, "compiler.options.supports", "");
			break;
		case 2:
		case 3:
			prop_set_with_key (p->props, "project.type", "GNOME");
			prop_set_with_key (p->props, "compiler.options.supports", "GNOME");
			break;
		default:
			return FALSE;
	}
	prop_set_with_key (p->props, "project.target.type", "EXECUTABLE");

	if (fscanf (fp, "Project Name: %s \n", buff) < 1)
		return FALSE;
	prop_set_with_key (p->props, "project.name", buff);
	prop_set_with_key (p->props, "project.menu.entry", buff);

	if (fscanf (fp, "Program Name: %s \n", buff) < 1)
		return FALSE;
	prop_set_with_key (p->props, "project.source.target", buff);

	if (fscanf (fp, "Version: %s \n", buff) < 1)
		return FALSE;
	prop_set_with_key (p->props, "project.version", buff);

	fscanf (fp, "Author: ");
	read_line (fp, &temp);
	prop_set_with_key (p->props, "project.author", temp);
	g_free (temp);

	if (fscanf (fp, "Source Dir: %s \n", buff) < 1)
		return FALSE;
	key =
		g_strconcat ("module.", module_map[MODULE_SOURCE], ".name",
			     NULL);
	prop_set_with_key (p->props, key, buff);
	g_free (key);

	if (fscanf (fp, "Pixmap Dir: %s \n", buff) < 1)
		return FALSE;
	key =
		g_strconcat ("module.", module_map[MODULE_PIXMAP], ".name",
			     NULL);
	prop_set_with_key (p->props, key, buff);
	g_free (key);

	if (fscanf (fp, "Document Dir: %s \n", buff) < 1)
		return FALSE;
	key = g_strconcat ("module.", module_map[MODULE_DOC], ".name", NULL);
	prop_set_with_key (p->props, key, buff);
	g_free (key);

	/* Discard */
	if (!read_string (fp, "Includes", &temp))
		return FALSE;
	g_free (temp);

	/* Discard */
	if (!read_string (fp, "Libraries", &temp))
		return FALSE;
	g_free (temp);

	if (fscanf (fp, "Project Has Gui: %d\n", &tmp) < 1)
		return FALSE;
	if (tmp)
		prop_set_with_key (p->props, "project.has.gui", "1");
	else
		prop_set_with_key (p->props, "project.has.gui", "0");

	if (fscanf (fp, "Gettext Support: %d\n", &tmp) < 1)
		return FALSE;
	if (tmp)
		prop_set_with_key (p->props, "project.has.gettext", "1");
	else
		prop_set_with_key (p->props, "project.has.gettext", "0");

	if (fscanf (fp, "Manage Makefiles: %d\n", &tmp) < 1)
		return FALSE;
	if (tmp == TRUE) /* Does anjuta manage the build files? */
	{
		/* if yes, enable overwrite for all but configure.in and top make file*/
		project_config_set_disable_overwrite_all (p->project_config, FALSE);
		project_config_set_disable_overwrite (p->project_config, BUILD_FILE_CONFIGURE_IN, TRUE);
		project_config_set_disable_overwrite (p->project_config, BUILD_FILE_TOP_MAKEFILE_AM, TRUE);
	}
	else
	{
		/* if yes, disable overwrite for all but configure.in and top make file*/
		project_config_set_disable_overwrite_all (p->project_config, TRUE);
	}

	/* Discard */
	if (fscanf (fp, "Set Cflags: %d\n", &tmp) < 1)
		return FALSE;

	if (compatibility_0_compiler_options_load (p->props, fp) < 1)
		return FALSE;

	if (compatibility_0_src_paths_load (p->props, fp) < 1)
		return FALSE;

	files = NULL;
	if (fscanf (fp, "No of Source Files: %d\n", &tmp) < 1)
		return FALSE;
	for (i = 0; i < tmp; i++)
	{
		if (fscanf (fp, "Src: %s \n", buff) < 1)
			return FALSE;
		files = g_list_append (files, g_strdup (buff));
	}
	temp = string_from_glist (files);
	if (temp)
	{
		key =
			g_strconcat ("module.", module_map[MODULE_SOURCE],
				     ".files", NULL);
		prop_set_with_key (p->props, key, temp);
		g_free (key);
		g_free (temp);
	}
	glist_strings_free (files);

	files = NULL;
	if (fscanf (fp, "No of Document Files: %d \n", &tmp) < 1)
		return FALSE;
	for (i = 0; i < tmp; i++)
	{
		if (fscanf (fp, "Doc: %s \n", buff) < 1)
			return FALSE;
		files = g_list_append (files, g_strdup (buff));
	}
	temp = string_from_glist (files);
	if (temp)
	{
		key =
			g_strconcat ("module.", module_map[MODULE_DOC],
				     ".files", NULL);
		prop_set_with_key (p->props, key, temp);
		g_free (key);
		g_free (temp);
	}
	glist_strings_free (files);

	files = project_dbase_scan_files_in_module (p, MODULE_PIXMAP, FALSE);
	temp = string_from_glist (files);
	if (temp)
	{
		key =
			g_strconcat ("module.", module_map[MODULE_PIXMAP],
				     ".files", NULL);
		prop_set_with_key (p->props, key, temp);
		g_free (key);
		g_free (temp);
	}
	glist_strings_free (files);

	return TRUE;
}
