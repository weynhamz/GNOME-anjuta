/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    header.c
    Copyright (C) 2004 Sebastien Granjoux

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

/*
 * header data read in .wiz file, used in the first page (project selection)
 *
 *---------------------------------------------------------------------------*/

#include <config.h>
#include <sys/wait.h>
#include <unistd.h>

#include "header.h"

#include <glib.h>
#include <libanjuta/anjuta-utils.h>

/*---------------------------------------------------------------------------*/

struct _NPWHeader {
	gchar* name;
	gchar* description;
	gchar* iconfile;
	gchar* category;
	gchar* filename;
	GList* required_programs;
	GList* required_packages;
};

/* Header
 *---------------------------------------------------------------------------*/

NPWHeader*
npw_header_new (void)
{
	NPWHeader* self;

	self = g_slice_new0(NPWHeader);

	return self;
}

void
npw_header_free (NPWHeader* self)
{
	g_free (self->name);
	g_free (self->description);
	g_free (self->iconfile);
	g_free (self->category);
	g_free (self->filename);
	g_list_free (self->required_programs);
	g_list_free (self->required_packages);
}

void
npw_header_set_name (NPWHeader* self, const gchar* name)
{
	self->name = g_strdup (name);
}

const gchar*
npw_header_get_name (const NPWHeader* self)
{
	return self->name;
}

void
npw_header_set_filename (NPWHeader* self, const gchar* filename)
{
	self->filename = g_strdup (filename);
}

const gchar*
npw_header_get_filename (const NPWHeader* self)
{
	return self->filename;
}

void
npw_header_set_category (NPWHeader* self, const gchar* category)
{
	self->category =g_strdup (category);
}

const gchar*
npw_header_get_category (const NPWHeader* self)
{
	return self->category;
}

void
npw_header_set_description (NPWHeader* self, const gchar* description)
{
	self->description = g_strdup (description);
}

const gchar*
npw_header_get_description (const NPWHeader* self)
{
	return self->description;
}

void
npw_header_set_iconfile (NPWHeader* self, const gchar* iconfile)
{
	self->iconfile = g_strdup (iconfile);
}

const gchar*
npw_header_get_iconfile (const NPWHeader* self)
{
	return self->iconfile;
}

void
npw_header_add_required_program (NPWHeader* self, const gchar* program)
{
	self->required_programs =
		g_list_prepend (self->required_programs,
						g_strdup (program));
	
}

void
npw_header_add_required_package (NPWHeader* self, const gchar* package)
{
	self->required_packages =
		g_list_prepend (self->required_packages,
						g_strdup(package));
}

GList*
npw_header_check_required_programs (NPWHeader* self)
{
	GList *node = NULL;
	GList *failed_programs = NULL;
	for (node = self->required_programs; node; node = g_list_next (node))
	{
		if (!anjuta_util_prog_is_installed (node->data, FALSE))
		{
			failed_programs = g_list_prepend (failed_programs, node->data);
		}
	}
	return failed_programs;
}

static gboolean
package_is_installed (const gchar *package)
{
	int status;
	int exit_status;
	pid_t pid;
	if ((pid = fork()) == 0)
	{
		execlp ("pkg-config", "pkg-config", "--exists", package, NULL);
	}
	waitpid (pid, &status, 0);
	exit_status = WEXITSTATUS (status);
	return (exit_status == 0);
}

GList*
npw_header_check_required_packages (NPWHeader* self)
{
	GList *node = NULL;
	GList *failed_packages = NULL;
	for (node = self->required_packages; node; node = g_list_next (node))
	{
		if (!package_is_installed (node->data))
		{
			failed_packages = g_list_prepend (failed_packages, node->data);
		}
	}
	return failed_packages;
}

/* Header list
 *---------------------------------------------------------------------------*/

GList*
npw_header_list_new (void)
{
	GList* list;

	list = NULL;
	
	return list;
}

static void
remove_category (GList *list)
{
	g_list_foreach (list, (GFunc)npw_header_free, NULL);
	g_list_free (list);
}

void
npw_header_list_free (GList* list)
{
	g_list_foreach (list, (GFunc)remove_category, NULL);
	g_list_free (list);
}

static gint
compare_header_name (NPWHeader *a, NPWHeader *b)
{
	return g_ascii_strcasecmp (npw_header_get_name (a), npw_header_get_name (b));
}

GList *
npw_header_list_insert_header (GList *list, NPWHeader *header)
{
	GList *node;
	GList *template_list;
	
	for (node = g_list_first (list); node != NULL; node = g_list_next (node))
	{
		NPWHeader* first;
		gint res;
		
		template_list = (GList *)node->data;	
		first = (NPWHeader *)template_list->data;
		res = g_ascii_strcasecmp (npw_header_get_category (first), npw_header_get_category (header));
		if (res == 0)
		{
			node->data = g_list_insert_sorted (template_list, header, (GCompareFunc) compare_header_name);
			return list;
		}
		else if (res > 0)
		{
			break;
		}
	}

	template_list = g_list_prepend (NULL, header);

	return g_list_insert_before (list, node, template_list);
}

NPWHeader*
npw_header_list_find_header (GList *list, NPWHeader *header)
{
	GList *node;
	GList *template_list;
	
	for (node = g_list_first (list); node != NULL; node = g_list_next (node))
	{
		NPWHeader* first;
		gint res;
		
		template_list = (GList *)node->data;	
		first = (NPWHeader *)template_list->data;
		res = g_ascii_strcasecmp (npw_header_get_category (first), npw_header_get_category (header));
		if (res == 0)
		{
			GList *find;
			
			find = g_list_find_custom (template_list, header, (GCompareFunc) compare_header_name);
			if (find != NULL)
			{
				return (NPWHeader *)find->data;
			}

			break;
		}
		else if (res > 0)
		{
			break;
		}
	}

	return NULL;
}
