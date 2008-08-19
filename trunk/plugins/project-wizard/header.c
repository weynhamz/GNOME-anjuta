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

#include <glib/gdir.h>
#include <libanjuta/anjuta-utils.h>

/*---------------------------------------------------------------------------*/

#define STRING_CHUNK_SIZE	256

/*---------------------------------------------------------------------------*/

struct _NPWHeaderList
{
	GNode* list;
	GStringChunk* string_pool;
	GMemChunk* data_pool;
};

struct _NPWHeader {
	gchar* name;
	gchar* description;
	gchar* iconfile;
	gchar* category;
	gchar* filename;
	GList* required_programs;
	GList* required_packages;
	NPWHeaderList* owner;
	GNode* node;
};

/* Header
 *---------------------------------------------------------------------------*/

NPWHeader*
npw_header_new (NPWHeaderList* owner)
{
	NPWHeader* this;

	g_return_val_if_fail (owner, NULL);

	this = g_chunk_new0(NPWHeader, owner->data_pool);
	this->owner = owner;
	this->node = g_node_append_data (owner->list, this);

	return this;
}

void
npw_header_free (NPWHeader* this)
{
	GNode* node;

	/* Memory allocated in string pool and project pool is not free */
	node = g_node_find (this->owner->list, G_IN_ORDER, G_TRAVERSE_ALL, this);
	if (node != NULL)
		g_node_destroy (node);
	if (this->required_programs)
		g_list_free (this->required_programs);
	if (this->required_packages)
		g_list_free (this->required_packages);
}

void
npw_header_set_name (NPWHeader* this, const gchar* name)
{
	this->name = g_string_chunk_insert (this->owner->string_pool, name);
}

const gchar*
npw_header_get_name (const NPWHeader* this)
{
	return this->name;
}

void
npw_header_set_filename (NPWHeader* this, const gchar* filename)
{
	this->filename = g_string_chunk_insert (this->owner->string_pool, filename);
}

const gchar*
npw_header_get_filename (const NPWHeader* this)
{
	return this->filename;
}

void
npw_header_set_category (NPWHeader* this, const gchar* category)
{
	npw_header_list_organize(this->owner, category, this);
	this->category = ((NPWHeader *)this->node->parent->data)->category;
}

const gchar*
npw_header_get_category (const NPWHeader* this)
{
	return this->category;
}

void
npw_header_set_description (NPWHeader* this, const gchar* description)
{
	this->description = g_string_chunk_insert (this->owner->string_pool, description);
}

const gchar*
npw_header_get_description (const NPWHeader* this)
{
	return this->description;
}

void
npw_header_set_iconfile (NPWHeader* this, const gchar* iconfile)
{
	this->iconfile = g_string_chunk_insert (this->owner->string_pool, iconfile);
}

const gchar*
npw_header_get_iconfile (const NPWHeader* this)
{
	return this->iconfile;
}

void
npw_header_add_required_program (NPWHeader* this, const gchar* program)
{
	this->required_programs =
		g_list_prepend (this->required_programs,
						g_string_chunk_insert (this->owner->string_pool,
											   program));
	
}

void
npw_header_add_required_package (NPWHeader* this, const gchar* package)
{
	this->required_packages =
		g_list_prepend (this->required_packages,
						g_string_chunk_insert (this->owner->string_pool,
											   package));
}

GList*
npw_header_check_required_programs (NPWHeader* this)
{
	GList *node = NULL;
	GList *failed_programs = NULL;
	for (node = this->required_programs; node; node = g_list_next (node))
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
npw_header_check_required_packages (NPWHeader* this)
{
	GList *node = NULL;
	GList *failed_packages = NULL;
	for (node = this->required_packages; node; node = g_list_next (node))
	{
		if (!package_is_installed (node->data))
		{
			failed_packages = g_list_prepend (failed_packages, node->data);
		}
	}
	return failed_packages;
}

gboolean
npw_header_is_leaf(const NPWHeader* this)
{
	return G_NODE_IS_LEAF(this->node);
}


/* Header list
 *---------------------------------------------------------------------------*/

static GNode*
npw_header_list_find_parent(NPWHeaderList* this, const gchar* category, gboolean create)
{
	GNode* node;
	gint order;

	for (node = g_node_first_child(this->list); node != NULL; node = g_node_next_sibling(node))
	{
		const char* s;

		s = ((NPWHeader *)node->data)->category;
		if (s != NULL)
		{
			order = g_ascii_strcasecmp (s, category);
			if (order == 0)
			{
				/* Find right category */
				break;
			}
			else if (order > 0)
			{
				if (create == TRUE)
				{
					/* Category doesn't exist create a new node */
					NPWHeader* new_parent;

					new_parent = npw_header_new (this);
					new_parent->category = g_string_chunk_insert (this->string_pool, category);
					g_node_unlink (new_parent->node);
					g_node_insert_before (this->list, node, new_parent->node);
					node = new_parent->node;
				}
				else
				{
					node = NULL;
				}
				break;
			}
		}
	}

	if (create && (node == NULL))
	{
		NPWHeader* new_parent;

		new_parent = npw_header_new (this);
		new_parent->category = g_string_chunk_insert (this->string_pool, category);
		node = new_parent->node;
	}

	return node;
}

typedef struct _HeaderListForeachProjectData
{
	NPWHeaderForeachFunc func;
	gpointer data;
} HeaderListForeachProjectData;

static void
cb_header_list_foreach_project (GNode* node, gpointer data)
{
	HeaderListForeachProjectData* d = (HeaderListForeachProjectData *)data;

	(d->func)((NPWHeader*)node->data, d->data);
}

static gboolean
npw_header_list_foreach_node (GNode* list, NPWHeaderForeachFunc func, gpointer data, GTraverseFlags flag)
{
	HeaderListForeachProjectData d;

	if (g_node_first_child (list) == NULL) return FALSE;

	d.func = func;
	d.data = data;
				     
	g_node_children_foreach (list, flag, cb_header_list_foreach_project, &d);

	return TRUE;
}
	
/*---------------------------------------------------------------------------*/

NPWHeaderList*
npw_header_list_new (void)
{
	NPWHeaderList* this;

	this = g_new (NPWHeaderList, 1);
	this->string_pool = g_string_chunk_new (STRING_CHUNK_SIZE);
	this->data_pool = g_mem_chunk_new ("project pool", sizeof (NPWHeader), STRING_CHUNK_SIZE * sizeof (NPWHeader) / 4, G_ALLOC_ONLY);
	this->list = g_node_new (NULL);

	return this;
}

void
npw_header_list_free (NPWHeaderList* this)
{
	g_return_if_fail (this != NULL);

	g_string_chunk_free (this->string_pool);
	g_mem_chunk_destroy (this->data_pool);
	g_node_destroy (this->list);
	g_free (this);
}

void
npw_header_list_organize(NPWHeaderList* this, const gchar* category, NPWHeader* header)
{
	GNode* parent;
	GNode* node;
	const gchar* name;

	/* node without a category stay on top */
	if ((category == NULL) || (*category == '\0')) return;

	/* Detach the node */
	g_node_unlink(header->node);

	/* Find parent */
	parent = npw_header_list_find_parent(this, category, TRUE);

	/* Insert node as a child in alphabetic order*/
	name = npw_header_get_name (header);
	for (node = g_node_first_child(parent); node != NULL; node = g_node_next_sibling(node))
	{
		if (g_ascii_strcasecmp (npw_header_get_name ((NPWHeader *)node->data), name) > 0)
		{
			g_node_insert_before (parent, node, header->node);
			return;
		}
	}
	g_node_insert(parent, -1, header->node);
}

gboolean
npw_header_list_foreach_project (const NPWHeaderList* this, NPWHeaderForeachFunc func, gpointer data)
{
	return npw_header_list_foreach_node (this->list, func, data, G_TRAVERSE_LEAFS);
}
	
gboolean
npw_header_list_foreach_project_in (const NPWHeaderList* this, const gchar* category, NPWHeaderForeachFunc func, gpointer data)
{
	GNode* node;
	
	/* FIXME: const modified has been removed because the find function
	 * with TRUE as the third argument could create a parent if it
	 * doesn't exist. It could be better to split this function in two */
	node = npw_header_list_find_parent((NPWHeaderList*)this, category, FALSE);
	if (node == NULL) return FALSE;

	return npw_header_list_foreach_node (node, func, data, G_TRAVERSE_LEAFS);
}

gboolean
npw_header_list_foreach_category (const NPWHeaderList* this, NPWHeaderForeachFunc func, gpointer data)
{
	return npw_header_list_foreach_node (this->list, func, data, G_TRAVERSE_NON_LEAFS);
}
