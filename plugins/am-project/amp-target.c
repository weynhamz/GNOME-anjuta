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



/* Helper functions
 *---------------------------------------------------------------------------*/

static void
error_set (GError **error, gint code, const gchar *message)
{
        if (error != NULL) {
                if (*error != NULL) {
                        gchar *tmp;

                        /* error already created, just change the code
                         * and prepend the string */
                        (*error)->code = code;
                        tmp = (*error)->message;
                        (*error)->message = g_strconcat (message, "\n\n", tmp, NULL);
                        g_free (tmp);

                } else {
                        *error = g_error_new_literal (IANJUTA_PROJECT_ERROR,
                                                      code,
                                                      message);
                }
        }
}


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




/* Target object
 *---------------------------------------------------------------------------*/

void
amp_target_node_set_type (AmpTargetNode *target, AmTokenType type)
{
	target->base.type = ANJUTA_PROJECT_TARGET | type;
	target->base.native_properties = amp_get_target_property_list(type);
}

void
amp_target_node_add_token (AmpTargetNode *target, AmTokenType type, AnjutaToken *token)
{
	target->tokens = tagged_token_list_insert (target->tokens, type, token);
}

GList *
amp_target_node_get_token (AmpTargetNode *target, AmTokenType type)
{
	return tagged_token_list_get	(target->tokens, type);
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

AmpTargetNode*
amp_target_node_new (const gchar *name, AnjutaProjectNodeType type, const gchar *install, gint flags, GError **error)
{
	AmpTargetNode *node = NULL;
	const gchar *basename;

	/* Validate target name */
	if (!name || strlen (name) <= 0)
	{
		error_set (error, IANJUTA_PROJECT_ERROR_VALIDATION_FAILED,
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
			error_set (error, IANJUTA_PROJECT_ERROR_VALIDATION_FAILED,
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
			error_set (error, IANJUTA_PROJECT_ERROR_VALIDATION_FAILED,
				   _("Shared library target name must be of the form 'libxxx.la'"));
			return NULL;
		}
	}
	else if ((type & ANJUTA_PROJECT_ID_MASK) == ANJUTA_PROJECT_STATICLIB) {
		if (strlen (basename) < 6 ||
		    strncmp (basename, "lib", strlen("lib")) != 0 ||
		    strcmp (&basename[strlen(basename) - 2], ".a") != 0) {
			error_set (error, IANJUTA_PROJECT_ERROR_VALIDATION_FAILED,
				   _("Static library target name must be of the form 'libxxx.a'"));
			return NULL;
		}
	}
	
	node = g_object_new (AMP_TYPE_TARGET_NODE, NULL);
	amp_target_node_set_type (node, type);
	node->base.name = g_strdup (name);
	node->install = g_strdup (install);
	node->flags = flags;
	
	return node;
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
amp_target_node_erase (AmpNode *node, AmpNode *parent, AmpProject *project, GError **error)
{
	return amp_target_node_delete_token (project, AMP_TARGET_NODE (node), error);
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

	g_list_foreach (node->base.custom_properties, (GFunc)amp_property_free, NULL);
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
