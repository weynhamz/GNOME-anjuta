/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* amp-group.c
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

#include "amp-group.h"

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

struct _AmpGroupNode {
	AnjutaProjectNode base;
	gboolean dist_only;										/* TRUE if the group is distributed but not built */
	GFile *makefile;												/* GFile corresponding to group makefile */
	AnjutaTokenFile *tfile;										/* Corresponding Makefile */
	GList *tokens[AM_GROUP_TOKEN_LAST];			/* List of token used by this group */
	AnjutaToken *make_token;
	GHashTable *variables;
	GFileMonitor *monitor;									/* File monitor */
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
                                                      
/* Private functions
 *---------------------------------------------------------------------------*/

/* Find if pkg-config modules are used in group targets */
static gboolean
project_load_group_module (AmpProject *project, AmpGroupNode *group)
{
	AnjutaProjectNode *target;
	AnjutaProjectProperty *prop;
	gchar **group_cpp = NULL;

	prop = amp_node_get_property_from_token (ANJUTA_PROJECT_NODE (group), AM_TOKEN__CPPFLAGS, 0);
	if (prop && (prop->value != NULL)) group_cpp = g_strsplit_set (prop->value, " \t", 0);
	
	/* Check all targets */
	for (target = anjuta_project_node_first_child (ANJUTA_PROJECT_NODE (group)); target != NULL; target = anjuta_project_node_next_sibling (target))
	{
		gint type = anjuta_project_node_get_full_type (target) & (ANJUTA_PROJECT_ID_MASK | ANJUTA_PROJECT_TYPE_MASK);
		gchar **target_lib = NULL;
		gchar **target_cpp = NULL;

		prop = NULL;
		switch (type)
		{
		case ANJUTA_PROJECT_TARGET | ANJUTA_PROJECT_PROGRAM:
			prop = amp_node_get_property_from_token (target, AM_TOKEN_TARGET_LDADD, 0);
			break;
		case ANJUTA_PROJECT_TARGET | ANJUTA_PROJECT_STATICLIB:
		case ANJUTA_PROJECT_TARGET | ANJUTA_PROJECT_SHAREDLIB:
			prop = amp_node_get_property_from_token (target, AM_TOKEN_TARGET_LIBADD, 0);
			break;
		default:
			break;
		}
		if (prop && (prop->value != NULL)) target_lib = g_strsplit_set (prop->value, " \t", 0);

		/* Check if targets use libraries */
		if (target_lib != NULL)
		{
			AnjutaProjectNode *module;

			prop = amp_node_get_property_from_token (target, AM_TOKEN_TARGET_CPPFLAGS, 0);
			if (prop && (prop->value != NULL)) target_cpp = g_strsplit_set (prop->value, " \t", 0);

			for (module = anjuta_project_node_first_child (ANJUTA_PROJECT_NODE (project)); module != NULL; module = anjuta_project_node_next_sibling (module))
			{
				if (anjuta_project_node_get_node_type (module) == ANJUTA_PROJECT_MODULE)
				{
					const gchar *name = anjuta_project_node_get_name (module);
					gchar *lib_flags = g_strconcat ("$(", name, "_LIBS)", NULL);
					gchar **flags;

					for (flags = target_lib; *flags != NULL; flags++)
					{

						if (strcmp (*flags, lib_flags) == 0)
						{
							gchar *cpp_flags = g_strconcat ("$(", name, "_CFLAGS)", NULL);
							gchar **cflags;
							gboolean found = FALSE;

							if (group_cpp != NULL)
							{
								for (cflags = group_cpp; *cflags != NULL; cflags++)
								{
									if (strcmp (*cflags, cpp_flags) == 0)
									{
										found = TRUE;
										break;
									}
								}
							}
							if ((target_cpp != NULL) && !found)
							{
								for (cflags = target_cpp; *cflags != NULL; cflags++)
								{
									if (strcmp (*cflags, cpp_flags) == 0)
									{
										found = TRUE;
										break;
									}
								}
							}
							if (found)
							{
								/* Add new module */
								AnjutaProjectNode *new_module;

								new_module = amp_node_new (target, ANJUTA_PROJECT_MODULE, NULL, name, NULL);
								anjuta_project_node_append (target, new_module);
							}
							g_free (cpp_flags);
						}
					}
					g_free (lib_flags);
				}
			}		
			g_strfreev (target_cpp);
			g_strfreev (target_lib);
		}
	}
	g_strfreev (group_cpp);

	return TRUE;
}


extern const gchar *valid_am_makefiles[];

static AmpGroupNode*
project_load_makefile (AmpProject *project, AmpGroupNode *group)
{
	const gchar **filename;
	AnjutaTokenFile *tfile;
	GFile *makefile = NULL;
	GFile *file = anjuta_project_node_get_file ((AnjutaProjectNode *)group);

	/* Find makefile name
	 * It has to be in the config_files list with .am extension */
	for (filename = valid_am_makefiles; *filename != NULL; filename++)
	{
		makefile = g_file_get_child (file, *filename);
		if (file_type (file, *filename) == G_FILE_TYPE_REGULAR)
		{
			gchar *final_filename = g_strdup (*filename);
			gchar *ptr;
			GFile *final_file;
			AnjutaToken *token;

			ptr = g_strrstr (final_filename,".am");
			if (ptr != NULL) *ptr = '\0';
			final_file = g_file_get_child (file, final_filename);
			g_free (final_filename);

			token = amp_project_get_config_token (project, final_file);
			g_object_unref (final_file);
			if (token != NULL)
			{
				amp_group_node_add_token (group, token, AM_GROUP_TOKEN_CONFIGURE);
				break;
			}
		}
		g_object_unref (makefile);
	}

	if (*filename == NULL)
	{
		/* Unable to find automake file */
		return group;
	}

	/* Parse makefile.am */
	DEBUG_PRINT ("Parse: %s", g_file_get_uri (makefile));
	tfile = amp_group_node_set_makefile (group, makefile, project);

	project_load_group_module (project, group);
	
	return group;
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



/* Group objects
 *---------------------------------------------------------------------------*/


void
amp_group_node_add_token (AmpGroupNode *group, AnjutaToken *token, AmpGroupNodeTokenCategory category)
{
	group->tokens[category] = g_list_prepend (group->tokens[category], token);
}

GList *
amp_group_node_get_token (AmpGroupNode *group, AmpGroupNodeTokenCategory category)
{
	return group->tokens[category];
}

GList * 
amp_group_node_get_all_token (AmpGroupNode *group)
{
	gint i;
	GList *tokens = NULL;

	for (i = 0; i < AM_GROUP_TOKEN_LAST; i++)
	{
		tokens = g_list_concat (tokens, g_list_copy (group->tokens[i]));
	}
	
	return tokens;
}

AnjutaToken*
amp_group_node_get_first_token (AmpGroupNode *group, AmpGroupNodeTokenCategory category)
{
	GList *list;
	
	list = amp_group_node_get_token (group, category);
	if (list == NULL) return NULL;

	return (AnjutaToken *)list->data;
}

void
amp_group_node_set_dist_only (AmpGroupNode *group, gboolean dist_only)
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
	AnjutaProjectNode *node = ANJUTA_PROJECT_NODE (data);
	AnjutaProjectNode *root;

	switch (event_type) {
		case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
		case G_FILE_MONITOR_EVENT_CHANGED:
		case G_FILE_MONITOR_EVENT_DELETED:
			/* project can be NULL, if the node is dummy node because the
			 * original one is reloaded. */
			root = anjuta_project_node_root (node);
			if (root != NULL) g_signal_emit_by_name (G_OBJECT (root), "file-changed", data);
			break;
		default:
			break;
	}
}

AnjutaTokenFile*
amp_group_node_set_makefile (AmpGroupNode *group, GFile *makefile, AmpProject *project)
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
		amp_project_add_file (project, makefile, group->tfile);
			
		scanner = amp_am_scanner_new (project, group);
		group->make_token = amp_am_scanner_parse_token (scanner, anjuta_token_new_static (ANJUTA_TOKEN_FILE, NULL), token, makefile, NULL);
		amp_am_scanner_free (scanner);

		group->monitor = g_file_monitor_file (makefile, 
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
amp_group_node_get_makefile_token (AmpGroupNode *group)
{
	return group->make_token;
}

AnjutaTokenFile *
amp_group_node_get_make_token_file (AmpGroupNode *group)
{
	return group->tfile;
}

gboolean
amp_group_node_update_makefile (AmpGroupNode *group, AnjutaToken *token)
{
	return anjuta_token_file_update (group->tfile, token);
}

gchar *
amp_group_node_get_makefile_name (AmpGroupNode *group)
{
	gchar *basename = NULL;
	
	if (group->makefile != NULL) 
	{
		basename = g_file_get_basename (group->makefile);
	}

	return basename;
}

void
amp_group_node_update_node (AmpGroupNode *group, AmpGroupNode *new_group)
{
	gint i;
	GHashTable *hash;

	if (group->monitor != NULL)
	{
		g_object_unref (group->monitor);
		group->monitor = NULL;
	}
	if (group->makefile != NULL)	
	{
		g_object_unref (group->makefile);
		group->monitor = NULL;
	}
	if (group->tfile) anjuta_token_file_free (group->tfile);
	for (i = 0; i < AM_GROUP_TOKEN_LAST; i++)
	{
		if (group->tokens[i] != NULL) g_list_free (group->tokens[i]);
	}
	if (group->variables) g_hash_table_remove_all (group->variables);

	group->dist_only = new_group->dist_only;
	group->makefile = new_group->makefile;
	new_group->makefile = NULL;
	group->tfile = new_group->tfile;
	new_group->tfile = NULL;
	memcpy (group->tokens, new_group->tokens, sizeof (group->tokens));
	memset (new_group->tokens, 0, sizeof (new_group->tokens));
	hash = group->variables;
	group->variables = new_group->variables;
	new_group->variables = hash;
	
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

void
amp_group_node_update_variable (AmpGroupNode *group, AnjutaToken *variable)
{
	AnjutaToken *arg;
	char *name = NULL;
	AnjutaToken *value = NULL;
	AmpVariable *var;

	arg = anjuta_token_first_item (variable);
	name = g_strstrip (anjuta_token_evaluate (arg));
	value = anjuta_token_last_item (variable);

	var = (AmpVariable *)g_hash_table_lookup (group->variables, name);
	if (var != NULL)
	{
		var->value = value;
	}
	else
	{
		var = amp_variable_new (name, 0, value);
		g_hash_table_insert (group->variables, var->name, var);
	}

	if (name) g_free (name);
}

AnjutaToken*
amp_group_node_get_variable_token (AmpGroupNode *group, AnjutaToken *variable)
{
	guint length;
	const gchar *string;
	gchar *name;
	AmpVariable *var;
		
	length = anjuta_token_get_length (variable);
	string = anjuta_token_get_string (variable);
	if (string[1] == '(')
	{
		name = g_strndup (string + 2, length - 3);
	}
	else
	{
		name = g_strndup (string + 1, 1);
	}
	var = g_hash_table_lookup (group->variables, name);
	g_free (name);

	return var != NULL ? var->value : NULL;
}

gboolean
amp_group_node_set_file (AmpGroupNode *group, GFile *new_file)
{
	g_object_unref (group->base.file);
	group->base.file = g_object_ref (new_file);

	return TRUE;
}

AmpGroupNode*
amp_group_node_new (GFile *file, gboolean dist_only, GError **error)
{
	AmpGroupNode *node = NULL;
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
			if (!isalnum (*ptr) && (strchr ("#$:%+,-.=@^_`~", *ptr) == NULL))
				failed = TRUE;
			ptr++;
		}
		if (failed) {
			g_free (name);
			error_set (error, IANJUTA_PROJECT_ERROR_VALIDATION_FAILED,
			           _("Group name can only contain alphanumeric or \"#$:%+,-.=@^_`~\" characters"));
			return NULL;
		}
	}
	g_free (name);
	
	node = g_object_new (AMP_TYPE_GROUP_NODE, NULL);
	node->base.file = g_object_ref (file);
	node->dist_only = dist_only;
	memset (node->tokens, 0, sizeof (node->tokens));

    return node;	
}

void
amp_group_node_free (AmpGroupNode *node)
{
	g_object_unref (G_OBJECT (node));
}

/* AmpNode implementation
 *---------------------------------------------------------------------------*/

static gboolean
amp_group_node_load (AmpNode *group, AmpNode *parent, AmpProject *project, GError **error)
{
	if (project_load_makefile (project, AMP_GROUP_NODE (group)) == NULL)
	{
		g_set_error (error, IANJUTA_PROJECT_ERROR, 
					IANJUTA_PROJECT_ERROR_DOESNT_EXIST,
			_("Project doesn't exist or invalid path"));

		return FALSE;
	}

	return TRUE;
}

static gboolean
amp_group_node_save (AmpNode *group, AmpNode *parent, AmpProject *project, GError **error)
{
	AnjutaTokenFile *tfile;
	AnjutaProjectNode *child;

	/* Save group */
	tfile = AMP_GROUP_NODE (group)->tfile;
	if (tfile == NULL) return FALSE;
	if (anjuta_token_file_is_dirty (tfile))
	{
		if (!anjuta_token_file_save (tfile, error)) return FALSE;
	}

	/* Save all children */
	for (child = anjuta_project_node_first_child (ANJUTA_PROJECT_NODE (group)); child != NULL; child = anjuta_project_node_next_sibling (child))
	{
		if (!amp_node_save (AMP_NODE (child), group, project, error)) return FALSE;
	}

	return TRUE;
}

static gboolean
amp_group_node_update (AmpNode *node, AmpNode *new_node)
{
	amp_group_node_update_node (AMP_GROUP_NODE (node), AMP_GROUP_NODE (new_node));

	return TRUE;
}

static gboolean
amp_group_node_write (AmpNode *node, AmpNode *parent, AmpProject *project, GError **error)
{
	return amp_group_node_create_token (project, AMP_GROUP_NODE (node), error);
}

static gboolean
amp_group_node_erase (AmpNode *node, AmpNode *parent, AmpProject *project, GError **error)
{
	return amp_group_node_delete_token (project, AMP_GROUP_NODE (node), error);
}



/* GObjet implementation
 *---------------------------------------------------------------------------*/


typedef struct _AmpGroupNodeClass AmpGroupNodeClass;

struct _AmpGroupNodeClass {
	AmpNodeClass parent_class;
};

G_DEFINE_DYNAMIC_TYPE (AmpGroupNode, amp_group_node, AMP_TYPE_NODE);

static void
amp_group_node_init (AmpGroupNode *node)
{
	node->base.type = ANJUTA_PROJECT_GROUP;
	node->base.native_properties = amp_get_group_property_list();
	node->base.state = ANJUTA_PROJECT_CAN_ADD_GROUP |
						ANJUTA_PROJECT_CAN_ADD_TARGET |
						ANJUTA_PROJECT_CAN_ADD_SOURCE |
						ANJUTA_PROJECT_CAN_REMOVE |
						ANJUTA_PROJECT_CAN_SAVE;
	node->dist_only = FALSE;
	node->variables = NULL;
	node->makefile = NULL;
	node->variables = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, (GDestroyNotify)amp_variable_free);
	node->monitor = NULL;
	memset (node->tokens, 0, sizeof (node->tokens));
}

static void
amp_group_node_dispose (GObject *object)
{
	AmpGroupNode *node = AMP_GROUP_NODE (object);

	if (node->monitor) g_object_unref (node->monitor);
	node->monitor = NULL;
	
	G_OBJECT_CLASS (amp_group_node_parent_class)->dispose (object);
}

static void
amp_group_node_finalize (GObject *object)
{
	AmpGroupNode *node = AMP_GROUP_NODE (object);
	gint i;
	
	g_list_foreach (node->base.custom_properties, (GFunc)amp_property_free, NULL);
	if (node->tfile) anjuta_token_file_free (node->tfile);
	if (node->makefile) g_object_unref (node->makefile);

	for (i = 0; i < AM_GROUP_TOKEN_LAST; i++)
	{
		if (node->tokens[i] != NULL) g_list_free (node->tokens[i]);
	}
	if (node->variables) g_hash_table_destroy (node->variables);

	G_OBJECT_CLASS (amp_group_node_parent_class)->finalize (object);
}

static void
amp_group_node_class_init (AmpGroupNodeClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AmpNodeClass* node_class;
	
	object_class->finalize = amp_group_node_finalize;
	object_class->dispose = amp_group_node_dispose;

	node_class = AMP_NODE_CLASS (klass);
	node_class->load = amp_group_node_load;
	node_class->save = amp_group_node_save;
	node_class->update = amp_group_node_update;
	node_class->write = amp_group_node_write;
	node_class->erase = amp_group_node_erase;
}

static void
amp_group_node_class_finalize (AmpGroupNodeClass *klass)
{
}

void
amp_group_node_register (GTypeModule *module)
{
	amp_group_node_register_type (module);
}
