/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* am-node.c
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

#include "am-node.h"
#include "am-scanner.h"
#include "am-properties.h"


#include <libanjuta/interfaces/ianjuta-project.h>

#include <libanjuta/anjuta-debug.h>

#include <glib/gi18n.h>

#include <memory.h>
#include <string.h>
#include <ctype.h>



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


/* Variable object
 *---------------------------------------------------------------------------*/

AmpVariable*
amp_variable_new (gchar *name, AnjutaTokenType assign, AnjutaToken *value)
{
    AmpVariable *variable = NULL;

	g_return_val_if_fail (name != NULL, NULL);
	
	variable = g_slice_new0(AmpVariable); 
	variable->name = g_strdup (name);
	variable->assign = assign;
	variable->value = value;

	return variable;
}

static void
amp_variable_free (AmpVariable *variable)
{
	g_free (variable->name);
	
    g_slice_free (AmpVariable, variable);
}




/* Root objects
 *---------------------------------------------------------------------------*/

AnjutaAmRootNode*
amp_root_new (GFile *file, GError **error)
{
	AnjutaAmRootNode *root = NULL;

	root = g_object_new (ANJUTA_TYPE_AM_ROOT_NODE, NULL);
	root->base.type = ANJUTA_PROJECT_ROOT;
	root->base.native_properties = amp_get_project_property_list();
	root->base.custom_properties = NULL;
	root->base.file = g_file_dup (file);
	root->base.name = NULL;
	root->base.state = ANJUTA_PROJECT_CAN_ADD_GROUP |
						ANJUTA_PROJECT_CAN_ADD_PACKAGE,
						ANJUTA_PROJECT_CAN_SAVE;
	

	return root;
}

void
amp_root_free (AnjutaAmRootNode *node)
{
	g_object_unref (G_OBJECT (node));
}

void
amp_root_clear (AnjutaAmRootNode *node)
{
	if (node->configure_file != NULL) anjuta_token_file_free (node->configure_file);
	node->configure_file = NULL;
	if (node->configure_token) anjuta_token_free (node->configure_token);
	
	g_list_foreach (node->base.custom_properties, (GFunc)amp_property_free, NULL);
	node->base.custom_properties = NULL;
}

static void
on_root_monitor_changed (GFileMonitor *monitor,
											GFile *file,
											GFile *other_file,
											GFileMonitorEvent event_type,
											gpointer data)
{
	AnjutaAmRootNode *node = ANJUTA_AM_ROOT_NODE (data);

	switch (event_type) {
		case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
		case G_FILE_MONITOR_EVENT_CHANGED:
		case G_FILE_MONITOR_EVENT_DELETED:
			/* project can be NULL, if the node is dummy node because the
			 * original one is reloaded. */
			if (node->project != NULL) g_signal_emit_by_name (G_OBJECT (node->project), "file-changed", data);
			break;
		default:
			break;
	}
}

AnjutaTokenFile*
amp_root_set_configure (AnjutaAmRootNode *root, GFile *configure, GObject* project)
{
	if (root->configure != NULL) g_object_unref (root->configure);
	if (root->configure_file != NULL) anjuta_token_file_free (root->configure_file);
	if (configure != NULL)
	{
		root->configure_file = anjuta_token_file_new (configure);
		root->configure = g_object_ref (configure);

		root->monitor = g_file_monitor_file (configure, 
						      									G_FILE_MONITOR_NONE,
						       									NULL,
						       									NULL);
		if (root->monitor != NULL)
		{
			root->project = project;
			g_signal_connect (G_OBJECT (root->monitor),
					  "changed",
					  G_CALLBACK (on_root_monitor_changed),
					  root);
		}
	}
	else
	{
		root->configure_file = NULL;
		root->configure = NULL;
		if (root->monitor) g_object_unref (root->monitor);
		root->monitor = NULL;
	}
	
	return root->configure_file;
}

gboolean
amp_root_update_configure (AnjutaAmRootNode *root, AnjutaToken *token)
{
	return anjuta_token_file_update (root->configure_file, token);
}

AnjutaToken*
amp_root_get_configure_token (AnjutaAmRootNode *root)
{
	return root->configure_token;
}

void
amp_root_update_monitor (AnjutaAmRootNode *root)
{
	if (root->monitor != NULL) g_object_unref (root->monitor);

	if (root->configure != NULL)
	{
		root->monitor = g_file_monitor_file (root->configure,
						      						G_FILE_MONITOR_NONE,
						       						NULL,
						       						NULL);
		if (root->monitor != NULL)
		{
			g_signal_connect (G_OBJECT (root->monitor),
					  "changed",
					  G_CALLBACK (on_root_monitor_changed),
					  root);
		}
	}
}

/* GObjet implementation
 *---------------------------------------------------------------------------*/

typedef struct _AnjutaAmRootNodeClass AnjutaAmRootNodeClass;

struct _AnjutaAmRootNodeClass {
	AnjutaProjectNodeClass parent_class;
};

G_DEFINE_TYPE (AnjutaAmRootNode, anjuta_am_root_node, ANJUTA_TYPE_PROJECT_NODE);

static void
anjuta_am_root_node_init (AnjutaAmRootNode *node)
{
	node->configure_file = NULL;
	node->configure_token = NULL;
}

static void
anjuta_am_root_node_dispose (GObject *object)
{
	AnjutaAmRootNode *root = ANJUTA_AM_ROOT_NODE (object);

	if (root->monitor) g_object_unref (root->monitor);
	root->monitor = NULL;
	
	G_OBJECT_CLASS (anjuta_am_root_node_parent_class)->dispose (object);
}

static void
anjuta_am_root_node_finalize (GObject *object)
{
	AnjutaAmRootNode *root = ANJUTA_AM_ROOT_NODE (object);

	amp_root_clear (root);
	
	G_OBJECT_CLASS (anjuta_am_root_node_parent_class)->finalize (object);
}

static void
anjuta_am_root_node_class_init (AnjutaAmRootNodeClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	
	object_class->finalize = anjuta_am_root_node_finalize;
	object_class->dispose = anjuta_am_root_node_dispose;
}




/* Module objects
 *---------------------------------------------------------------------------*/

void
amp_module_add_token (AnjutaAmModuleNode *module, AnjutaToken *token)
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
amp_module_get_token (AnjutaAmModuleNode *node)
{
	return node->module;
}

AnjutaAmModuleNode*
amp_module_new (const gchar *name, GError **error)
{
	AnjutaAmModuleNode *module = NULL;

	module = g_object_new (ANJUTA_TYPE_AM_MODULE_NODE, NULL);
	module->base.type = ANJUTA_PROJECT_MODULE;
	module->base.native_properties = amp_get_module_property_list();
	module->base.custom_properties = NULL;
	module->base.file = NULL;
	module->base.name = g_strdup (name);;
	module->base.state = ANJUTA_PROJECT_CAN_ADD_PACKAGE |
						ANJUTA_PROJECT_CAN_REMOVE;
	module->module = NULL;

	return module;
}

void
amp_module_free (AnjutaAmModuleNode *node)
{
	g_object_unref (G_OBJECT (node));
}


/* GObjet implementation
 *---------------------------------------------------------------------------*/

typedef struct _AnjutaAmModuleNodeClass AnjutaAmModuleNodeClass;

struct _AnjutaAmModuleNodeClass {
	AnjutaProjectNodeClass parent_class;
};

G_DEFINE_TYPE (AnjutaAmModuleNode, anjuta_am_module_node, ANJUTA_TYPE_PROJECT_NODE);

static void
anjuta_am_module_node_init (AnjutaAmModuleNode *node)
{
}

static void
anjuta_am_module_node_finalize (GObject *object)
{
	AnjutaAmModuleNode *module = ANJUTA_AM_MODULE_NODE (object);

	g_list_foreach (module->base.custom_properties, (GFunc)amp_property_free, NULL);
	
	G_OBJECT_CLASS (anjuta_am_module_node_parent_class)->finalize (object);
}

static void
anjuta_am_module_node_class_init (AnjutaAmModuleNodeClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	
	object_class->finalize = anjuta_am_module_node_finalize;
}




/* Package objects
 *---------------------------------------------------------------------------*/

AnjutaAmPackageNode*
amp_package_new (const gchar *name, GError **error)
{
	AnjutaAmPackageNode *node = NULL;

	node = g_object_new (ANJUTA_TYPE_AM_PACKAGE_NODE, NULL);
	node->base.type = ANJUTA_PROJECT_PACKAGE;
	node->base.native_properties = amp_get_package_property_list();
	node->base.custom_properties = NULL;
	node->base.file = NULL;
	node->base.name = g_strdup (name);
	node->base.state =  ANJUTA_PROJECT_CAN_REMOVE;
	node->version = NULL;

	return node;
}

void
amp_package_free (AnjutaAmPackageNode *node)
{
	g_object_unref (G_OBJECT (node));
}

void
amp_package_set_version (AnjutaAmPackageNode *node, const gchar *compare, const gchar *version)
{
	g_return_if_fail (node != NULL);
	g_return_if_fail ((version == NULL) || (compare != NULL));

	g_free (node->version);
	node->version = version != NULL ? g_strconcat (compare, version, NULL) : NULL;
}

AnjutaToken *
amp_package_get_token (AnjutaAmPackageNode *node)
{
	return node->token;
}

void
amp_package_add_token (AnjutaAmPackageNode *node, AnjutaToken *token)
{
	node->token = token;
}


/* GObjet implementation
 *---------------------------------------------------------------------------*/

typedef struct _AnjutaAmPackageNodeClass AnjutaAmPackageNodeClass;

struct _AnjutaAmPackageNodeClass {
	AnjutaProjectNodeClass parent_class;
};

G_DEFINE_TYPE (AnjutaAmPackageNode, anjuta_am_package_node, ANJUTA_TYPE_PROJECT_NODE);

static void
anjuta_am_package_node_init (AnjutaAmPackageNode *node)
{
}

static void
anjuta_am_package_node_finalize (GObject *object)
{
	AnjutaAmPackageNode *node = ANJUTA_AM_PACKAGE_NODE (object);

	g_list_foreach (node->base.custom_properties, (GFunc)amp_property_free, NULL);
	
	G_OBJECT_CLASS (anjuta_am_package_node_parent_class)->finalize (object);
}

static void
anjuta_am_package_node_class_init (AnjutaAmPackageNodeClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	
	object_class->finalize = anjuta_am_package_node_finalize;
}




/* Group objects
 *---------------------------------------------------------------------------*/


void
amp_group_add_token (AnjutaAmGroupNode *group, AnjutaToken *token, AmpGroupTokenCategory category)
{
	group->tokens[category] = g_list_prepend (group->tokens[category], token);
}

GList *
amp_group_get_token (AnjutaAmGroupNode *group, AmpGroupTokenCategory category)
{
	return group->tokens[category];
}

AnjutaToken*
amp_group_get_first_token (AnjutaAmGroupNode *group, AmpGroupTokenCategory category)
{
	GList *list;
	
	list = amp_group_get_token (group, category);
	if (list == NULL) return NULL;

	return (AnjutaToken *)list->data;
}

void
amp_group_set_dist_only (AnjutaAmGroupNode *group, gboolean dist_only)
{
 	group->dist_only = dist_only;
}

static void
on_group_monitor_changed (GFileMonitor *monitor,
											GFile *file,
											GFile *other_file,
											GFileMonitorEvent event_type,
											gpointer data)
{
	AnjutaAmGroupNode *node = ANJUTA_AM_GROUP_NODE (data);

	switch (event_type) {
		case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
		case G_FILE_MONITOR_EVENT_CHANGED:
		case G_FILE_MONITOR_EVENT_DELETED:
			/* project can be NULL, if the node is dummy node because the
			 * original one is reloaded. */
			if (node->project != NULL) g_signal_emit_by_name (G_OBJECT (node->project), "file-changed", data);
			break;
		default:
			break;
	}
}

AnjutaTokenFile*
amp_group_set_makefile (AnjutaAmGroupNode *group, GFile *makefile, GObject* project)
{
	if (group->makefile != NULL) g_object_unref (group->makefile);
	if (group->tfile != NULL) anjuta_token_file_free (group->tfile);
	if (makefile != NULL)
	{
		AnjutaToken *token;
		AmpAmScanner *scanner;
		
		group->makefile = g_object_ref (makefile);
		group->tfile = anjuta_token_file_new (makefile);

		token = anjuta_token_file_load (group->tfile, NULL);
		amp_project_add_file (AMP_PROJECT (project), makefile, group->tfile);
			
		scanner = amp_am_scanner_new (AMP_PROJECT (project), group);
		group->make_token = amp_am_scanner_parse_token (scanner, anjuta_token_new_static (ANJUTA_TOKEN_FILE, NULL), token, makefile, NULL);
		amp_am_scanner_free (scanner);

		group->monitor = g_file_monitor_file (makefile, 
						      									G_FILE_MONITOR_NONE,
						       									NULL,
						       									NULL);
		if (group->monitor != NULL)
		{
			group->project = project;
			g_signal_connect (G_OBJECT (group->monitor),
					  "changed",
					  G_CALLBACK (on_group_monitor_changed),
					  group);
		}
	}
	else
	{
		group->makefile = NULL;
		group->tfile = NULL;
		group->make_token = NULL;
		if (group->monitor) g_object_unref (group->monitor);
		group->monitor = NULL;
	}

	return group->tfile;
}

AnjutaToken*
amp_group_get_makefile_token (AnjutaAmGroupNode *group)
{
	return group->make_token;
}

gboolean
amp_group_update_makefile (AnjutaAmGroupNode *group, AnjutaToken *token)
{
	return anjuta_token_file_update (group->tfile, token);
}

gchar *
amp_group_get_makefile_name (AnjutaAmGroupNode *group)
{
	gchar *basename = NULL;
	
	if (group->makefile != NULL) 
	{
		basename = g_file_get_basename (group->makefile);
	}

	return basename;
}

void
amp_group_update_monitor (AnjutaAmGroupNode *group)
{
	if (group->monitor != NULL) g_object_unref (group->monitor);

	if (group->makefile != NULL)
	{
		group->monitor = g_file_monitor_file (group->makefile, 
						      									G_FILE_MONITOR_NONE,
						       									NULL,
						       									NULL);
		if (group->monitor != NULL)
		{
			g_signal_connect (G_OBJECT (group->monitor),
					  "changed",
					  G_CALLBACK (on_group_monitor_changed),
					  group);
		}
	}
}

AnjutaAmGroupNode*
amp_group_new (GFile *file, gboolean dist_only, GError **error)
{
	AnjutaAmGroupNode *node = NULL;
	gchar *name;

	/* Validate group name */
	name = g_file_get_basename (file);
	if (!name || strlen (name) <= 0)
	{
		g_free (name);
		error_set (error, IANJUTA_PROJECT_ERROR_VALIDATION_FAILED,
			   _("Please specify group name"));
		return NULL;
	}
	{
		gboolean failed = FALSE;
		const gchar *ptr = name;
		while (*ptr) {
			if (!isalnum (*ptr) && *ptr != '.' && *ptr != '-' &&
			    *ptr != '_')
				failed = TRUE;
			ptr++;
		}
		if (failed) {
			g_free (name);
			error_set (error, IANJUTA_PROJECT_ERROR_VALIDATION_FAILED,
				   _("Group name can only contain alphanumeric, '_', '-' or '.' characters"));
			return NULL;
		}
	}
	g_free (name);
	
	node = g_object_new (ANJUTA_TYPE_AM_GROUP_NODE, NULL);
	node->base.type = ANJUTA_PROJECT_GROUP;
	node->base.native_properties = amp_get_group_property_list();
	node->base.custom_properties = NULL;
	node->base.file = g_object_ref (file);
	node->base.name = NULL;
	node->base.state = ANJUTA_PROJECT_CAN_ADD_GROUP |
						ANJUTA_PROJECT_CAN_ADD_TARGET |
						ANJUTA_PROJECT_CAN_ADD_SOURCE |
						ANJUTA_PROJECT_CAN_REMOVE |
						ANJUTA_PROJECT_CAN_SAVE;
	node->dist_only = dist_only;
	node->variables = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, (GDestroyNotify)amp_variable_free);

    return node;	
}

void
amp_group_free (AnjutaAmGroupNode *node)
{
	g_object_unref (G_OBJECT (node));
}


/* GObjet implementation
 *---------------------------------------------------------------------------*/


typedef struct _AnjutaAmGroupNodeClass AnjutaAmGroupNodeClass;

struct _AnjutaAmGroupNodeClass {
	AnjutaProjectNodeClass parent_class;
};

G_DEFINE_TYPE (AnjutaAmGroupNode, anjuta_am_group_node, ANJUTA_TYPE_PROJECT_NODE);

static void
anjuta_am_group_node_init (AnjutaAmGroupNode *node)
{
	node->makefile = NULL;
	node->variables = NULL;
	node->monitor = NULL;
	memset (node->tokens, 0, sizeof (node->tokens));
}

static void
anjuta_am_group_node_dispose (GObject *object)
{
	AnjutaAmGroupNode *node = ANJUTA_AM_GROUP_NODE (object);

	if (node->monitor) g_object_unref (node->monitor);
	node->monitor = NULL;
	
	G_OBJECT_CLASS (anjuta_am_group_node_parent_class)->dispose (object);
}

static void
anjuta_am_group_node_finalize (GObject *object)
{
	AnjutaAmGroupNode *node = ANJUTA_AM_GROUP_NODE (object);
	gint i;
	
	g_list_foreach (node->base.custom_properties, (GFunc)amp_property_free, NULL);
	if (node->tfile) anjuta_token_file_free (node->tfile);
	if (node->makefile) g_object_unref (node->makefile);

	for (i = 0; i < AM_GROUP_TOKEN_LAST; i++)
	{
		if (node->tokens[i] != NULL) g_list_free (node->tokens[i]);
	}
	if (node->variables) g_hash_table_destroy (node->variables);

	if (node->monitor) g_object_unref (node->monitor);
	node->monitor = NULL;
	
	G_OBJECT_CLASS (anjuta_am_group_node_parent_class)->finalize (object);
}

static void
anjuta_am_group_node_class_init (AnjutaAmGroupNodeClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	
	object_class->finalize = anjuta_am_group_node_finalize;
	object_class->dispose = anjuta_am_group_node_dispose;
}




/* Target object
 *---------------------------------------------------------------------------*/



void
amp_target_add_token (AnjutaAmTargetNode *target, AmTokenType type, AnjutaToken *token)
{
	target->tokens = tagged_token_list_insert (target->tokens, type, token);
}

GList *
amp_target_get_token (AnjutaAmTargetNode *target, AmTokenType type)
{
	return tagged_token_list_get	(target->tokens, type);
}

AnjutaTokenType
amp_target_get_first_token_type (AnjutaAmTargetNode *target)
{
	return tagged_token_list_next (target->tokens, 0);
}

AnjutaTokenType
amp_target_get_next_token_type (AnjutaAmTargetNode *target, AnjutaTokenType type)
{
	return tagged_token_list_next (target->tokens, type);
}

AnjutaAmTargetNode*
amp_target_new (const gchar *name, AnjutaProjectNodeType type, const gchar *install, gint flags, GError **error)
{
	AnjutaAmTargetNode *node = NULL;
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
	
	node = g_object_new (ANJUTA_TYPE_AM_TARGET_NODE, NULL);
	node->base.type = ANJUTA_PROJECT_TARGET | type;
	node->base.native_properties = amp_get_target_property_list(type);
	node->base.custom_properties = NULL;
	node->base.name = g_strdup (name);
	node->base.file = NULL;
	node->base.state = ANJUTA_PROJECT_CAN_ADD_SOURCE |
						ANJUTA_PROJECT_CAN_REMOVE;
	node->install = g_strdup (install);
	node->flags = flags;
	node->tokens = NULL;
	
	return node;
}

void
amp_target_free (AnjutaAmTargetNode *node)
{
	g_object_unref (G_OBJECT (node));
}


/* GObjet implementation
 *---------------------------------------------------------------------------*/

typedef struct _AnjutaAmTargetNodeClass AnjutaAmTargetNodeClass;

struct _AnjutaAmTargetNodeClass {
	AnjutaProjectNodeClass parent_class;
};

G_DEFINE_TYPE (AnjutaAmTargetNode, anjuta_am_target_node, ANJUTA_TYPE_PROJECT_NODE);

static void
anjuta_am_target_node_init (AnjutaAmTargetNode *node)
{
}

static void
anjuta_am_target_node_finalize (GObject *object)
{
	AnjutaAmTargetNode *node = ANJUTA_AM_TARGET_NODE (object);

	g_list_foreach (node->base.custom_properties, (GFunc)amp_property_free, NULL);
	tagged_token_list_free (node->tokens);
	node->tokens = NULL;
	
	G_OBJECT_CLASS (anjuta_am_target_node_parent_class)->finalize (object);
}

static void
anjuta_am_target_node_class_init (AnjutaAmTargetNodeClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	
	object_class->finalize = anjuta_am_target_node_finalize;
}




/* Source object
 *---------------------------------------------------------------------------*/

AnjutaToken *
amp_source_get_token (AnjutaAmSourceNode *node)
{
	return node->token;
}

void
amp_source_add_token (AnjutaAmSourceNode *node, AnjutaToken *token)
{
	node->token = token;
}

AnjutaProjectNode*
amp_source_new (GFile *file, GError **error)
{
	AnjutaAmSourceNode *node = NULL;

	node = g_object_new (ANJUTA_TYPE_AM_SOURCE_NODE, NULL);
	node->base.type = ANJUTA_PROJECT_SOURCE;
	node->base.native_properties = amp_get_source_property_list();
	node->base.custom_properties = NULL;
	node->base.name = NULL;
	node->base.file = g_object_ref (file);
	node->base.state = ANJUTA_PROJECT_CAN_REMOVE;
	node->token = NULL;

	return ANJUTA_PROJECT_NODE (node);
}

void
amp_source_free (AnjutaAmSourceNode *node)
{
	g_object_unref (G_OBJECT (node));
}


/* GObjet implementation
 *---------------------------------------------------------------------------*/

typedef struct _AnjutaAmSourceNodeClass AnjutaAmSourceNodeClass;

struct _AnjutaAmSourceNodeClass {
	AnjutaProjectNodeClass parent_class;
};

G_DEFINE_TYPE (AnjutaAmSourceNode, anjuta_am_source_node, ANJUTA_TYPE_PROJECT_NODE);

static void
anjuta_am_source_node_init (AnjutaAmSourceNode *node)
{
}

static void
anjuta_am_source_node_finalize (GObject *object)
{
	AnjutaAmSourceNode *node = ANJUTA_AM_SOURCE_NODE (object);

	g_list_foreach (node->base.custom_properties, (GFunc)amp_property_free, NULL);
	G_OBJECT_CLASS (anjuta_am_source_node_parent_class)->finalize (object);
}

static void
anjuta_am_source_node_class_init (AnjutaAmSourceNodeClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	
	object_class->finalize = anjuta_am_source_node_finalize;
}
