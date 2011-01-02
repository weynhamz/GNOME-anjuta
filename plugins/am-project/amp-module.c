/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* amp-module.c
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

#include "amp-module.h"

#include "amp-node.h"
#include "am-scanner.h"
#include "am-properties.h"
#include "ac-writer.h"
#include "am-writer.h"


#include <libanjuta/interfaces/ianjuta-project.h>

#include <libanjuta/anjuta-debug.h>

#include <glib/gi18n.h>

#include <memory.h>
#include <string.h>
#include <ctype.h>

/* Types
 *---------------------------------------------------------------------------*/

struct _AmpModuleNode {
	AnjutaProjectNode base;
	AnjutaToken *module;
};


/* Module objects
 *---------------------------------------------------------------------------*/

void
amp_module_node_add_token (AmpModuleNode *module, AnjutaToken *token)
{
	gchar *name;
	
	module->module = token;
	name = anjuta_token_evaluate (anjuta_token_first_item (token));
	if (name != NULL)
	{
		g_free (module->base.name);
		module->base.name = name;
	}
}

AnjutaToken *
amp_module_node_get_token (AmpModuleNode *node)
{
	return node->module;
}

void
amp_module_node_update_node (AmpModuleNode *node, AmpModuleNode *new_node)
{
	node->module = new_node->module;
}

AmpModuleNode*
amp_module_node_new (const gchar *name, GError **error)
{
	AmpModuleNode *module = NULL;

	module = g_object_new (AMP_TYPE_MODULE_NODE, NULL);
	module->base.name = g_strdup (name);;

	return module;
}

void
amp_module_node_free (AmpModuleNode *node)
{
	g_object_unref (G_OBJECT (node));
}



/* AmpNode implementation
 *---------------------------------------------------------------------------*/

static gboolean
amp_module_node_update (AmpNode *node, AmpNode *new_node)
{
	AMP_MODULE_NODE(node)->module = AMP_MODULE_NODE(new_node)->module;

	return TRUE;
}

static gboolean
amp_module_node_write (AmpNode *node, AmpNode *amp_parent, AmpProject *project, GError **error)
{
	AnjutaProjectNode *parent = ANJUTA_PROJECT_NODE (amp_parent);

	if ((parent != NULL) && (anjuta_project_node_get_node_type (parent) == ANJUTA_PROJECT_TARGET))
	{
		AnjutaProjectNode *group = anjuta_project_node_parent (parent);
		AnjutaProjectProperty *group_cpp;
		AnjutaProjectProperty *target_cpp;
		AnjutaProjectProperty *target_lib;
		gchar *lib_flags;
		gchar *cpp_flags;
		gint type;
				
		group_cpp = amp_node_get_property_from_token (group, AM_TOKEN__CPPFLAGS, 0); 
					
		type = anjuta_project_node_get_full_type (parent) & (ANJUTA_PROJECT_ID_MASK | ANJUTA_PROJECT_TYPE_MASK);
		switch (type)
		{
		case ANJUTA_PROJECT_TARGET | ANJUTA_PROJECT_PROGRAM:
			target_lib = amp_node_get_property_from_token (parent, AM_TOKEN_TARGET_LDADD, 0);
			break;
		case ANJUTA_PROJECT_TARGET | ANJUTA_PROJECT_STATICLIB:
		case ANJUTA_PROJECT_TARGET | ANJUTA_PROJECT_SHAREDLIB:
			target_lib = amp_node_get_property_from_token (parent, AM_TOKEN_TARGET_LIBADD, 0);
			break;
		default:
			break;
		}
		target_cpp = amp_node_get_property_from_token (parent, AM_TOKEN_TARGET_CPPFLAGS, 0);

		lib_flags = g_strconcat ("$(", anjuta_project_node_get_name (ANJUTA_PROJECT_NODE (node)), "_LIBS)", NULL);
		cpp_flags = g_strconcat ("$(", anjuta_project_node_get_name (ANJUTA_PROJECT_NODE (node)), "_CFLAGS)", NULL);

		if (!amp_node_property_has_flags (group, group_cpp, cpp_flags) && !amp_node_property_has_flags (ANJUTA_PROJECT_NODE (parent), target_cpp, cpp_flags))
		{
			AnjutaProjectProperty *prop;
			prop = amp_node_property_add_flags (group, group_cpp, cpp_flags);
			amp_project_update_am_property (project, group, prop);				
		}
					
		if (!amp_node_property_has_flags (parent, target_lib, lib_flags))
		{
			AnjutaProjectProperty *prop;
			prop = amp_node_property_add_flags (parent, target_lib, lib_flags);
			amp_project_update_am_property (project, parent, prop);				
		}

		g_free (lib_flags);
		g_free (cpp_flags);

		return TRUE;
	}
	else
	{
		return amp_module_node_create_token (project, AMP_MODULE_NODE (node), error);
	}
}

static gboolean
amp_module_node_erase (AmpNode *node, AmpNode *amp_parent, AmpProject *project, GError **error)
{
	AnjutaProjectNode *parent = ANJUTA_PROJECT_NODE (amp_parent);

	if ((parent != NULL) && (anjuta_project_node_get_node_type (parent) == ANJUTA_PROJECT_TARGET))
	{
		AnjutaProjectNode *group = anjuta_project_node_parent (parent);
		AnjutaProjectProperty *prop;
		AnjutaProjectProperty *group_cpp;
		AnjutaProjectProperty *target_cpp;
		AnjutaProjectProperty *target_lib;
		gchar *lib_flags;
		gchar *cpp_flags;
		gint type;

		lib_flags = g_strconcat ("$(", anjuta_project_node_get_name (ANJUTA_PROJECT_NODE (node)), "_LIBS)", NULL);
		cpp_flags = g_strconcat ("$(", anjuta_project_node_get_name (ANJUTA_PROJECT_NODE (node)), "_CFLAGS)", NULL);
				
		group_cpp = amp_node_get_property_from_token (group, AM_TOKEN__CPPFLAGS, 0); 
		if (amp_node_property_has_flags (group, group_cpp, cpp_flags))
		{
			/* Remove flags in group variable if not more target has this module */
			gboolean used = FALSE;
			AnjutaProjectNode *target;
					
			for (target = anjuta_project_node_first_child (ANJUTA_PROJECT_NODE (group)); target != NULL; target = anjuta_project_node_next_sibling (target))
			{
				if (anjuta_project_node_get_node_type (target) == ANJUTA_PROJECT_TARGET)
				{
					AnjutaProjectNode *module;

					for (module = anjuta_project_node_first_child (target); module != NULL; module = anjuta_project_node_next_sibling (module))
					{
						if ((anjuta_project_node_get_node_type (module) == ANJUTA_PROJECT_MODULE) &&
						    (module != ANJUTA_PROJECT_NODE (node)) &&
						    (strcmp (anjuta_project_node_get_name (module), anjuta_project_node_get_name (ANJUTA_PROJECT_NODE (node))) == 0))
						{
							used = TRUE;
							break;
						}
					}
				}
				if (used) break;
			}

			if (!used)
			{
				AnjutaProjectProperty *prop;

				prop = amp_node_property_remove_flags (group, group_cpp, cpp_flags);
				if (prop != NULL) amp_project_update_am_property (project, group, prop);
			}
		}
					
		type = anjuta_project_node_get_full_type (ANJUTA_PROJECT_NODE (parent)) & (ANJUTA_PROJECT_ID_MASK | ANJUTA_PROJECT_TYPE_MASK);
		switch (type)
		{
		case ANJUTA_PROJECT_TARGET | ANJUTA_PROJECT_PROGRAM:
			target_lib = amp_node_get_property_from_token (parent, AM_TOKEN_TARGET_LDADD, 0);
			break;
		case ANJUTA_PROJECT_TARGET | ANJUTA_PROJECT_STATICLIB:
		case ANJUTA_PROJECT_TARGET | ANJUTA_PROJECT_SHAREDLIB:
			target_lib = amp_node_get_property_from_token (parent, AM_TOKEN_TARGET_LIBADD, 0);
			break;
		default:
			target_lib = NULL;
			break;
		}
		target_cpp = amp_node_get_property_from_token (parent, AM_TOKEN_TARGET_CPPFLAGS, 0);

		prop = amp_node_property_remove_flags (parent, target_cpp, cpp_flags);
		if (prop != NULL) amp_project_update_am_property (project, parent, prop);
		prop = amp_node_property_remove_flags (parent, target_lib, lib_flags);
		if (prop != NULL) amp_project_update_am_property (project, parent, prop);

		g_free (lib_flags);
		g_free (cpp_flags);

		return TRUE;
	}
	else
	{
		return amp_module_node_delete_token (project, AMP_MODULE_NODE (node), error);
	}
}



/* GObjet implementation
 *---------------------------------------------------------------------------*/

typedef struct _AmpModuleNodeClass AmpModuleNodeClass;

struct _AmpModuleNodeClass {
	AmpNodeClass parent_class;
};

G_DEFINE_DYNAMIC_TYPE (AmpModuleNode, amp_module_node, AMP_TYPE_NODE);

static void
amp_module_node_init (AmpModuleNode *node)
{
	node->base.type = ANJUTA_PROJECT_MODULE;
	node->base.native_properties = amp_get_module_property_list();
	node->base.state = ANJUTA_PROJECT_CAN_ADD_PACKAGE |
						ANJUTA_PROJECT_CAN_REMOVE;
	node->module = NULL;
}

static void
amp_module_node_finalize (GObject *object)
{
	AmpModuleNode *module = AMP_MODULE_NODE (object);

	g_list_foreach (module->base.custom_properties, (GFunc)amp_property_free, NULL);
	
	G_OBJECT_CLASS (amp_module_node_parent_class)->finalize (object);
}

static void
amp_module_node_class_init (AmpModuleNodeClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AmpNodeClass* node_class;
	
	object_class->finalize = amp_module_node_finalize;

	node_class = AMP_NODE_CLASS (klass);
	node_class->update = amp_module_node_update;
	node_class->erase = amp_module_node_erase;
	node_class->write = amp_module_node_write;
}

static void
amp_module_node_class_finalize (AmpModuleNodeClass *klass)
{
}

void
amp_module_node_register (GTypeModule *module)
{
	amp_module_node_register_type (module);
}
