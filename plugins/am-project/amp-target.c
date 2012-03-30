/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* amp-target.c
 *
 * Copyright (C) 2010  Sébastien Granjoux
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

#include "amp-target.h"

#include "amp-node.h"
#include "am-scanner.h"
#include "am-properties.h"
#include "am-writer.h"
#include "amp-group.h"

#include <libanjuta/interfaces/ianjuta-project.h>

#include <libanjuta/anjuta-debug.h>

#include <glib/gi18n.h>

#include <memory.h>
#include <string.h>
#include <ctype.h>


/* Types
 *---------------------------------------------------------------------------*/

struct _AmpTargetNode {
	AnjutaProjectNode base;
	gchar *install;
	gint flags;
	GList* tokens;
};






/* Tagged token list
 *
 * This structure is used to keep a list of useful tokens in each
 * node. It is a two levels list. The level lists all kinds of token
 * and has a pointer of another list of token of  this kind.
 *---------------------------------------------------------------------------*/

typedef struct _TaggedTokenItem {
	AmTokenType type;
	GList *tokens;
} TaggedTokenItem;


static TaggedTokenItem *
tagged_token_item_new (AmTokenType type)
{
    TaggedTokenItem *item;

	item = g_slice_new0(TaggedTokenItem);

	item->type = type;

	return item;
}

static void
tagged_token_item_free (TaggedTokenItem* item)
{
	g_list_free (item->tokens);
    g_slice_free (TaggedTokenItem, item);
}

static gint
tagged_token_item_compare (gconstpointer a, gconstpointer b)
{
	return ((TaggedTokenItem *)a)->type - (GPOINTER_TO_INT(b));
}

static GList*
tagged_token_list_insert (GList *list, AmTokenType type, AnjutaToken *token)
{
	GList *existing;

	existing = g_list_find_custom (list, GINT_TO_POINTER (type), tagged_token_item_compare);
	if (existing == NULL)
	{
		/* Add a new item */
		TaggedTokenItem *item;

		item = tagged_token_item_new (type);
		list = g_list_prepend (list, item);
		existing = list;
	}

	((TaggedTokenItem *)(existing->data))->tokens = g_list_prepend (((TaggedTokenItem *)(existing->data))->tokens, token);

	return list;
}

static GList*
tagged_token_list_get (GList *list, AmTokenType type)
{
	GList *existing;
	GList *tokens = NULL;

	existing = g_list_find_custom (list, GINT_TO_POINTER (type), tagged_token_item_compare);
	if (existing != NULL)
	{
		tokens = ((TaggedTokenItem *)(existing->data))->tokens;
	}

	return tokens;
}

/* The returned list must be freed */
static GList*
tagged_token_list_get_all (GList *list)
{
	GList *tokens = NULL;

	for (; list != NULL; list = g_list_next (list))
	{
		tokens = g_list_concat (tokens, g_list_copy (((TaggedTokenItem *)list->data)->tokens));
	}

	return tokens;
}

static AnjutaTokenType
tagged_token_list_next (GList *list, AmTokenType type)
{
	AnjutaTokenType best = 0;

	for (list = g_list_first (list); list != NULL; list = g_list_next (list))
	{
		TaggedTokenItem *item = (TaggedTokenItem *)list->data;

		if ((item->type > type) && ((best == 0) || (item->type < best)))
		{
			best = item->type;
		}
	}

	return best;
}

static GList*
tagged_token_list_free (GList *list)
{
	g_list_foreach (list, (GFunc)tagged_token_item_free, NULL);
	g_list_free (list);

	return NULL;
}

/* Public functions
 *---------------------------------------------------------------------------*/


/* Public functions
 *---------------------------------------------------------------------------*/

void
amp_target_node_set_type (AmpTargetNode *target, AmTokenType type)
{
	target->base.type = ANJUTA_PROJECT_TARGET | type;
	target->base.properties_info = amp_get_target_property_list(type);
}

void
amp_target_node_add_token (AmpTargetNode *target, AmTokenType type, AnjutaToken *token)
{
	target->tokens = tagged_token_list_insert (target->tokens, type, token);
}

void
amp_target_node_remove_token (AmpTargetNode *target, AnjutaToken *token)
{
	GList *list;

	for (list = target->tokens; list != NULL; list = g_list_next (list))
	{
		TaggedTokenItem *tagged = (TaggedTokenItem *)list->data;

		tagged->tokens = g_list_remove (tagged->tokens, token);
	}
}

GList *
amp_target_node_get_token (AmpTargetNode *target, AmTokenType type)
{
	return tagged_token_list_get	(target->tokens, type);
}

GList*
amp_target_node_get_all_token (AmpTargetNode *target)
{
	return tagged_token_list_get_all (target->tokens);
}

AnjutaTokenType
amp_target_node_get_first_token_type (AmpTargetNode *target)
{
	return tagged_token_list_next (target->tokens, 0);
}

AnjutaTokenType
amp_target_node_get_next_token_type (AmpTargetNode *target, AnjutaTokenType type)
{
	return tagged_token_list_next (target->tokens, type);
}

void
amp_target_node_update_node (AmpTargetNode *node, AmpTargetNode *new_node)
{
	g_free (node->install);
	g_list_free (node->tokens);

	node->install = new_node->install;
	new_node->install = NULL;
	node->flags = new_node->flags;
	node->tokens = new_node->tokens;
	new_node->tokens = NULL;
}

/* The target has changed which could change its children */
void
amp_target_changed (AmpTargetNode *node)
{
	GList *item;
	gboolean custom = FALSE;

	for (item = ANJUTA_PROJECT_NODE (node)->properties; item != NULL; item = g_list_next (item))
	{
		AmpProperty *prop = (AmpProperty *)item->data;

		custom = ((AmpPropertyInfo *)prop->base.info)->flags & AM_PROPERTY_PREFIX_OBJECT;
		if (custom) break;
	}

	if (custom)
	{
		/* Update object name if the target has some custom properties */
		AnjutaProjectNode *child;

		for (child = anjuta_project_node_first_child (ANJUTA_PROJECT_NODE (node)); child != NULL; child = anjuta_project_node_next_sibling (child))
		{
			if (anjuta_project_node_get_node_type (child) == ANJUTA_PROJECT_OBJECT)
			{
				if (child->file != NULL)
				{
					AnjutaProjectNode *source = anjuta_project_node_first_child (child);

					if (source != NULL)
					{
						gchar *obj_name;
						const gchar *obj_ext;

						if (child->name != NULL)
						{
							g_free (child->name);
							child->name = NULL;
						}
						obj_name = g_file_get_basename (child->file);
						obj_ext = strrchr (obj_name, '.');
						if ((obj_ext != NULL)  && (obj_ext != obj_name))
						{
							GFile *src_dir;
							gchar *src_name;
							gchar *src_ext;
							gchar *new_name;

							src_dir = g_file_get_parent (source->file);
							src_name = g_file_get_basename (source->file);
							src_ext = strrchr (src_name, '.');
							if ((src_ext != NULL) && (src_ext != src_name)) *src_ext = '\0';
							new_name = g_strconcat (node->base.name, "-", src_name, obj_ext, NULL);

							g_object_unref (child->file);
							child->file = g_file_get_child (src_dir, new_name);

							g_free (new_name);
							g_free (src_name);
							g_object_unref (src_dir);
						}
						g_free (obj_name);
					}
				}
			}
		}
	}
}

AmpTargetNode*
amp_target_node_new (const gchar *name, AnjutaProjectNodeType type, const gchar *install, gint flags)
{
	AmpTargetNode *node;

	node = g_object_new (AMP_TYPE_TARGET_NODE, NULL);
	amp_target_node_set_type (node, type);
	node->base.name = g_strdup (name);
	node->install = g_strdup (install);
	node->flags = flags;

	amp_node_property_add_mandatory (ANJUTA_PROJECT_NODE (node));

	return node;
}

AmpTargetNode*
amp_target_node_new_valid (const gchar *name, AnjutaProjectNodeType type, const gchar *install, gint flags, AnjutaProjectNode *parent, GError **error)
{
	const gchar *basename;

	/* Check parent if present */
	if (parent != NULL)
	{
		if ((anjuta_project_node_get_node_type (parent) == ANJUTA_PROJECT_GROUP) &&
		    (amp_group_node_get_makefile_token (AMP_GROUP_NODE (parent)) == NULL))
		{
			amp_set_error (error, IANJUTA_PROJECT_ERROR_VALIDATION_FAILED,
		               _("Target parent is not a valid group"));

			return NULL;
		}
	}

	/* Validate target name */
	if (!name || strlen (name) <= 0)
	{
		amp_set_error (error, IANJUTA_PROJECT_ERROR_VALIDATION_FAILED,
		               _("Please specify target name"));
		return NULL;
	}
	{
		gboolean failed = FALSE;
		const gchar *ptr = name;
		while (*ptr) {
			if (!isalnum (*ptr) && *ptr != '.' && *ptr != '-' &&
			    *ptr != '_' && *ptr != '/')
				failed = TRUE;
			ptr++;
		}
		if (failed) {
			amp_set_error (error, IANJUTA_PROJECT_ERROR_VALIDATION_FAILED,
			               _("Target name can only contain alphanumeric, '_', '-', '/' or '.' characters"));
			return NULL;
		}
	}

	/* Skip eventual directory name */
	basename = strrchr (name, '/');
	basename = basename == NULL ? name : basename + 1;


	if ((type & ANJUTA_PROJECT_ID_MASK) == ANJUTA_PROJECT_SHAREDLIB) {
		if (strlen (basename) < 7 ||
		    strncmp (basename, "lib", strlen("lib")) != 0 ||
		    strcmp (&basename[strlen(basename) - 3], ".la") != 0) {
			amp_set_error (error, IANJUTA_PROJECT_ERROR_VALIDATION_FAILED,
			               _("Shared library target name must be of the form 'libxxx.la'"));
			return NULL;
		}
	}
	else if ((type & ANJUTA_PROJECT_ID_MASK) == ANJUTA_PROJECT_STATICLIB) {
		if (strlen (basename) < 6 ||
		    strncmp (basename, "lib", strlen("lib")) != 0 ||
		    strcmp (&basename[strlen(basename) - 2], ".a") != 0) {
			amp_set_error (error, IANJUTA_PROJECT_ERROR_VALIDATION_FAILED,
			               _("Static library target name must be of the form 'libxxx.a'"));
			return NULL;
		}
	}
	else if ((type & ANJUTA_PROJECT_ID_MASK) == ANJUTA_PROJECT_LT_MODULE) {
		if (strlen (basename) < 4 ||
		    strcmp (&basename[strlen(basename) - 3], ".la") != 0) {
			amp_set_error (error, IANJUTA_PROJECT_ERROR_VALIDATION_FAILED,
			               _("Module target name must be of the form 'xxx.la'"));
			return NULL;
		}
	}

	return amp_target_node_new (name, type, install, flags);
}

void
amp_target_node_free (AmpTargetNode *node)
{
	g_object_unref (G_OBJECT (node));
}



/* AmpNode implementation
 *---------------------------------------------------------------------------*/

static gboolean
amp_target_node_update (AmpNode *node, AmpNode *new_node)
{
	amp_target_node_update_node (AMP_TARGET_NODE (node), AMP_TARGET_NODE (new_node));

	return TRUE;
}

static gboolean
amp_target_node_write (AmpNode *node, AmpNode *parent, AmpProject *project, GError **error)
{
	return amp_target_node_create_token (project, AMP_TARGET_NODE (node), error);
}

static gboolean
amp_target_node_erase (AmpNode *target, AmpNode *parent, AmpProject *project, GError **error)
{
	gboolean ok;
	GList * token_list;

	token_list = amp_target_node_get_all_token (AMP_TARGET_NODE (target));
	ok = amp_target_node_delete_token (project, AMP_TARGET_NODE (target), token_list, error);
	g_list_free (token_list);

	/* Remove installation directory variable if the removed target was the
	 * only one using it */
	if (ok)
	{
		AnjutaProjectNode *node;
		const gchar *installdir;
		AnjutaProjectProperty *prop;
		gboolean used = FALSE;

		prop = amp_node_get_property_from_token (ANJUTA_PROJECT_NODE (target), AM_TOKEN__PROGRAMS, 6);
		installdir = prop->value;

		for (node = anjuta_project_node_first_child (ANJUTA_PROJECT_NODE (parent)); node != NULL; node = anjuta_project_node_next_sibling (node))
		{
			if (node != ANJUTA_PROJECT_NODE (target))
			{
				prop = amp_node_get_property_from_token (node, AM_TOKEN__PROGRAMS, 6);
				if ((prop != NULL) && (g_strcmp0 (installdir, prop->value) == 0))
				{
					used = TRUE;
					break;
				}
			}
		}

		if (!used)
		{
			GList *item;

			for (item = anjuta_project_node_get_properties (ANJUTA_PROJECT_NODE (parent)); item != NULL; item = g_list_next (item))
			{
				AmpProperty *prop = (AmpProperty *)item->data;

				if ((((AmpPropertyInfo *)prop->base.info)->token_type == AM_TOKEN_DIR) && (g_strcmp0 (prop->base.name, installdir) == 0))
				{
					/* Remove directory variable */
					anjuta_token_remove_list (anjuta_token_list (prop->token));
					amp_group_node_update_makefile (AMP_GROUP_NODE (parent), prop->token);
					break;
				}
			}
		}
	};

	return ok;
}



/* GObjet implementation
 *---------------------------------------------------------------------------*/

typedef struct _AmpTargetNodeClass AmpTargetNodeClass;

struct _AmpTargetNodeClass {
	AmpNodeClass parent_class;
};

G_DEFINE_DYNAMIC_TYPE (AmpTargetNode, amp_target_node, AMP_TYPE_NODE);

static void
amp_target_node_init (AmpTargetNode *node)
{
	node->base.type = ANJUTA_PROJECT_TARGET;
	node->base.state = ANJUTA_PROJECT_CAN_ADD_SOURCE |
						ANJUTA_PROJECT_CAN_ADD_MODULE |
						ANJUTA_PROJECT_CAN_REMOVE;
	node->install = NULL;
	node->flags = 0;
	node->tokens = NULL;
}

static void
amp_target_node_finalize (GObject *object)
{
	AmpTargetNode *node = AMP_TARGET_NODE (object);

	tagged_token_list_free (node->tokens);
	node->tokens = NULL;

	G_OBJECT_CLASS (amp_target_node_parent_class)->finalize (object);
}

static void
amp_target_node_class_init (AmpTargetNodeClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AmpNodeClass* node_class;

	object_class->finalize = amp_target_node_finalize;

	node_class = AMP_NODE_CLASS (klass);
	node_class->update = amp_target_node_update;
	node_class->write = amp_target_node_write;
	node_class->erase = amp_target_node_erase;
}

static void
amp_target_node_class_finalize (AmpTargetNodeClass *klass)
{
}

void
amp_target_node_register (GTypeModule *module)
{
	amp_target_node_register_type (module);
}
