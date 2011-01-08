/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* am-package.c
 *
 * Copyright (C) 2009  Sébastien Granjoux
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "amp-package.h"

#include "amp-node.h"
#include "am-scanner.h"
#include "am-properties.h"
#include "ac-writer.h"


#include <libanjuta/interfaces/ianjuta-project.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-pkg-config.h>

#include <libanjuta/anjuta-debug.h>

#include <glib/gi18n.h>

#include <memory.h>
#include <string.h>
#include <ctype.h>

/* Types
 *---------------------------------------------------------------------------*/

struct _AmpPackageNode {
	AnjutaProjectNode base;
	gchar *version;
	AnjutaToken *token;
};


/* Helper functions
 *---------------------------------------------------------------------------*/

static void
list_all_children (GList **children, GFile *dir)
{
	GFileEnumerator *list;
					
	list = g_file_enumerate_children (dir,
	    G_FILE_ATTRIBUTE_STANDARD_NAME,
	    G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
	    NULL,
	    NULL);

	if (list != NULL)
	{
		GFileInfo *info;
		
		while ((info = g_file_enumerator_next_file (list, NULL, NULL)) != NULL)
		{
			const gchar *name;
			GFile *file;

			name = g_file_info_get_name (info);
			file = g_file_get_child (dir, name);
			g_object_unref (info);

			if (g_file_query_file_type (file, G_FILE_QUERY_INFO_NONE, NULL) == G_FILE_TYPE_DIRECTORY)
			{
				list_all_children (children, file);
				g_object_unref (file);
			}
			else
			{
				*children = g_list_prepend (*children, file);
			}
		}
		g_file_enumerator_close (list, NULL, NULL);
		g_object_unref (list);
	}
}



/* Package objects
 *---------------------------------------------------------------------------*/

AmpPackageNode*
amp_package_node_new (const gchar *name, GError **error)
{
	AmpPackageNode *node = NULL;

	node = g_object_new (AMP_TYPE_PACKAGE_NODE, NULL);
	node->base.name = g_strdup (name);

	return node;
}

void
amp_package_node_free (AmpPackageNode *node)
{
	g_object_unref (G_OBJECT (node));
}

void
amp_package_node_set_version (AmpPackageNode *node, const gchar *compare, const gchar *version)
{
	g_return_if_fail (node != NULL);
	g_return_if_fail ((version == NULL) || (compare != NULL));

	g_free (node->version);
	node->version = version != NULL ? g_strconcat (compare, version, NULL) : NULL;
}

AnjutaToken *
amp_package_node_get_token (AmpPackageNode *node)
{
	return node->token;
}

void
amp_package_node_add_token (AmpPackageNode *node, AnjutaToken *token)
{
	node->token = token;
}

void
amp_package_node_update_node (AmpPackageNode *node, AmpPackageNode *new_node)
{
	g_return_if_fail (new_node != NULL);	

	node->token = new_node->token;
	g_free (node->version);
	node->version = new_node->version;
	new_node->version = NULL;
}

/* AmpNode implementation
 *---------------------------------------------------------------------------*/

static gboolean
amp_package_node_load (AmpNode *node, AmpNode *parent, AmpProject *project, GError **error)
{
	GList* deps;
	GList* dep;
	GList* include_dirs = NULL;
	
	deps = anjuta_pkg_config_list_dependencies (anjuta_project_node_get_name (ANJUTA_PROJECT_NODE (node)),
	                                            error);
	for (dep = deps; dep != NULL; dep = g_list_next (dep))
	{
		/* Create a package node for the depedencies */
		AnjutaProjectNode *pkg;

		pkg = amp_node_new (ANJUTA_PROJECT_NODE (parent), ANJUTA_PROJECT_PACKAGE, NULL, dep->data, NULL);
		anjuta_project_node_append (ANJUTA_PROJECT_NODE (node), pkg);
	}
	anjuta_util_glist_strings_free (deps);

	if (*error != NULL)
	{
		g_warning ("Error getting dependencies: %s", (*error)->message);
		g_error_free (*error);
		*error = NULL;
	}
	
	if ((include_dirs = anjuta_pkg_config_get_directories (anjuta_project_node_get_name (ANJUTA_PROJECT_NODE (node)),
	                                                       TRUE, error)))
	{
		GList* include_dir;
		
		for (include_dir = include_dirs; include_dir != NULL; include_dir = g_list_next (include_dir))
		{
			GList* children = NULL;
			GList* file = NULL;
			GFile* dir = g_file_new_for_path (include_dir->data);

			list_all_children (&children, dir);
			for (file = g_list_first (children); file != NULL; file = g_list_next (file))
			{
				/* Create a source for files */
				AnjutaProjectNode *source;

				source = amp_node_new (ANJUTA_PROJECT_NODE (parent), ANJUTA_PROJECT_SOURCE, (GFile *)file->data, NULL, NULL);
				anjuta_project_node_append (ANJUTA_PROJECT_NODE (node), source);
				g_object_unref ((GObject *)file->data);
			}
			g_list_free (children);
			g_object_unref (dir);
		}
	}
	anjuta_util_glist_strings_free (include_dirs);
	
	return TRUE;
}

static gboolean
amp_package_node_update (AmpNode *node, AmpNode *new_node)
{
	amp_package_node_update_node (AMP_PACKAGE_NODE (node), AMP_PACKAGE_NODE (new_node));

	return TRUE;
}

static gboolean
amp_package_node_write (AmpNode *node, AmpNode *parent, AmpProject *project, GError **error)
{
	return amp_package_node_create_token (project, AMP_PACKAGE_NODE (node), error);
}

static gboolean
amp_package_node_erase (AmpNode *node, AmpNode *parent, AmpProject *project, GError **error)
{
			return amp_package_node_delete_token (project, AMP_PACKAGE_NODE (node), error);
}


/* GObjet implementation
 *---------------------------------------------------------------------------*/

typedef struct _AmpPackageNodeClass AmpPackageNodeClass;

struct _AmpPackageNodeClass {
	AmpNodeClass parent_class;
};

G_DEFINE_DYNAMIC_TYPE (AmpPackageNode, amp_package_node, AMP_TYPE_NODE);

static void
amp_package_node_init (AmpPackageNode *node)
{
	node->base.type = ANJUTA_PROJECT_PACKAGE;
	node->base.native_properties = amp_get_package_property_list();
	node->base.state =  ANJUTA_PROJECT_CAN_REMOVE;
	node->version = NULL;
}

static void
amp_package_node_finalize (GObject *object)
{
	AmpPackageNode *node = AMP_PACKAGE_NODE (object);

	g_list_foreach (node->base.custom_properties, (GFunc)amp_property_free, NULL);
	
	G_OBJECT_CLASS (amp_package_node_parent_class)->finalize (object);
}

static void
amp_package_node_class_init (AmpPackageNodeClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AmpNodeClass* node_class;
	
	object_class->finalize = amp_package_node_finalize;
	
	node_class = AMP_NODE_CLASS (klass);
	node_class->load = amp_package_node_load;
	node_class->update = amp_package_node_update;
	node_class->erase = amp_package_node_erase;
	node_class->write = amp_package_node_write;
}

static void
amp_package_node_class_finalize (AmpPackageNodeClass *klass)
{
}

void
amp_package_node_register (GTypeModule *module)
{
	amp_package_node_register_type (module);
}
