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
#include <stdlib.h>

#include "header.h"

#include <glib.h>
#include <glib/gi18n.h>

#include <libanjuta/anjuta-utils.h>

/*---------------------------------------------------------------------------*/

struct _NPWHeader {
	gchar* name;
	gint name_lang;
	gchar* description;
	gint description_lang;
	gchar* iconfile;
	gchar* category;
	guint  order;
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
	if (self == NULL) return;

	g_free (self->name);
	g_free (self->description);
	g_free (self->iconfile);
	g_free (self->category);
	g_free (self->filename);
	g_list_free (self->required_programs);
	g_list_free (self->required_packages);
	g_slice_free (NPWHeader, self);
}

void
npw_header_set_name (NPWHeader* self, const gchar* name, gint lang)
{
	if (lang >= self->name_lang)
	{
		g_free (self->name);
		self->name = g_strdup (name);
		self->name_lang = lang;
	}
}

const gchar*
npw_header_get_name (const NPWHeader* self)
{
	return self->name_lang == 0 ? _(self->name) : self->name;
}

void
npw_header_set_filename (NPWHeader* self, const gchar* filename)
{
	g_free (self->filename);
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
	g_free (self->category);
	self->category =g_strdup (category);
}

const gchar*
npw_header_get_category (const NPWHeader* self)
{
	return self->category;
}

void
npw_header_set_description (NPWHeader* self, const gchar* description, gint lang)
{
	if (lang >= self->description_lang)
	{
		g_free (self->description);
		self->description = g_strdup (description);
		self->description_lang = lang;
	}
}

const gchar*
npw_header_get_description (const NPWHeader* self)
{
	return self->description_lang == 0 ? _(self->description) : self->description;
}

void
npw_header_set_iconfile (NPWHeader* self, const gchar* iconfile)
{
	g_free (self->iconfile);
	self->iconfile = g_strdup (iconfile);
}

const gchar*
npw_header_get_iconfile (const NPWHeader* self)
{
	return self->iconfile;
}

void
npw_header_set_order (NPWHeader* self, const gchar* order)
{
	self->order = strtoul (order, NULL, 10);
}

const guint
npw_header_get_order (const NPWHeader* self)
{
	return self->order;
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
			const gchar *const prog = (const gchar *) node->data;
			failed_programs = g_list_prepend (failed_programs,
											  g_strdup (prog));
		}
	}
	return failed_programs;
}

GList*
npw_header_check_required_packages (NPWHeader* self)
{
	GList *node = NULL;
	GList *failed_packages = NULL;
	for (node = self->required_packages; node; node = g_list_next (node))
	{
		if (!anjuta_util_package_is_installed (node->data, FALSE))
		{
			const gchar *const pkg = (const gchar *) node->data;
			failed_packages = g_list_prepend (failed_packages,
											  g_strdup (pkg));
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
	if (npw_header_get_order (a) == npw_header_get_order (b))
	{
		return g_utf8_collate (npw_header_get_name (a), npw_header_get_name (b));
	}
	else if (npw_header_get_order (a) == 0)
	{
		return 1;
	}
	else if (npw_header_get_order (b) == 0)
	{
		return -1;
	}
	else
	{
		return npw_header_get_order (a) - npw_header_get_order (b);
	}
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
