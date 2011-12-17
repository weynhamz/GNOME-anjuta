/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* ac-writer.c
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

#include <string.h>

#include "ac-writer.h"
#include "ac-scanner.h"
#include "ac-parser.h"
#include "amp-node.h"
#include "amp-module.h"
#include "amp-package.h"


#include "am-project-private.h"

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-utils.h>

/* Types
  *---------------------------------------------------------------------------*/


/* Helper functions
 *---------------------------------------------------------------------------*/

/* Private functions
 *---------------------------------------------------------------------------*/

static AnjutaToken *
anjuta_token_find_position (AnjutaToken *list, gboolean after, AnjutaTokenType type, AnjutaToken *sibling)
{
	AnjutaToken *tok;
	AnjutaToken *pos = sibling;

	if (sibling == NULL)
	{
		AnjutaToken *last = NULL;
		gboolean found = FALSE;

		for (tok = list; tok != NULL; tok = anjuta_token_next (tok))
		{
			AnjutaTokenType current = anjuta_token_get_type (tok);

			if ((current >= AC_TOKEN_FIRST_ORDERED_MACRO) && (current <= AC_TOKEN_LAST_ORDERED_MACRO))
			{
				/* Find a valid position */
				if (after)
				{
					/*  1. After the last similar macro
					 *  2. After the last macro with a higher priority
					 *  3. At the end of the file
					 */
					if (current == type)
					{
						pos = tok;
						found = TRUE;
					}
					else if (!found && (current < type))
					{
						pos = tok;
					}
				}
				else
				{
					/*  1. Before the first similar macro
					 *  2. Before the first macro with an lower priority
					 *  3. At the beginning of the file
					 */
					if (current == type)
					{
						pos = tok;
						break;
					}
					else if (!found && (current > type))
					{
						pos = tok;
						found = TRUE;
					}
				}
			}
			last = tok;
		}


		if (after && (pos == NULL)) pos = last;
	}

	if (after)
	{
		for (; pos != NULL; pos = anjuta_token_next (pos))
		{
			AnjutaTokenType current = anjuta_token_get_type (pos);

			if (current == ANJUTA_TOKEN_EOL) break;
		}
	}


	return pos;
}

static AnjutaToken*
find_tokens (AnjutaToken *list, AnjutaTokenType* types)
{
	AnjutaToken *tok;

	for (tok = list; tok != NULL; tok = anjuta_token_next (tok))
	{
		AnjutaTokenType *type;
		for (type = types; *type != 0; type++)
		{
			if (anjuta_token_get_type (tok) == *type)
			{
				return tok;
			}
		}
	}

	return NULL;
}

static AnjutaToken *
skip_comment (AnjutaToken *token)
{
	if (token == NULL) return NULL;

	for (;;)
	{
		for (;;)
		{
			AnjutaToken *next = anjuta_token_next (token);

			if (next == NULL) return token;

			switch (anjuta_token_get_type (token))
			{
			case ANJUTA_TOKEN_FILE:
			case SPACE:
				token = next;
				continue;
			case COMMENT:
				token = next;
				break;
			default:
				return token;
			}
			break;
		}

		for (;;)
		{
			AnjutaToken *next = anjuta_token_next (token);

			if (next == NULL) return token;
			token = next;
			if (anjuta_token_get_type (token) == END_OF_LINE) break;
		}
	}
}

/* Find an already existing property using the same token */
static AmpProperty *
find_similar_property (AnjutaProjectNode *node, AmpProperty *property)
{
	GList *item;

	for (item = anjuta_project_node_get_properties (node); item != NULL; item = g_list_next (item))
	{
		AmpProperty *prop = (AmpProperty *)item->data;

		if ((((AmpPropertyInfo *)prop->base.info)->token_type == ((AmpPropertyInfo *)property->base.info)->token_type) && (prop->token != NULL))
		{
			return prop;
		}
	}

	return NULL;
}

/* Public functions
 *---------------------------------------------------------------------------*/

gboolean
amp_project_update_ac_property (AmpProject *project, AnjutaProjectProperty *property)
{
	AnjutaToken *token;
	AnjutaToken *arg;
	AnjutaToken *args;
	AmpProperty *prop;
	AmpPropertyInfo *info;

	if (g_strcmp0 (((AmpPropertyInfo *)property->info)->value, property->value) == 0)
	{
		/* Remove property */
		info = (AmpPropertyInfo *)property->info;
		prop = (AmpProperty *)property;

		if (info->position == -1)
		{
			token = prop->token;

			anjuta_token_remove_list (anjuta_token_list (token));
		}

		anjuta_project_node_remove_property (ANJUTA_PROJECT_NODE (project), property);
	}
	else
	{
		info = (AmpPropertyInfo *)property->info;
		prop = find_similar_property (ANJUTA_PROJECT_NODE (project), (AmpProperty *)property);
		args = prop != NULL ? prop->token : NULL;

		prop = (AmpProperty *)property;

		if (args == NULL)
		{
			AnjutaToken *group;
			AnjutaToken *configure;
			const char *suffix;

			configure = amp_project_get_configure_token (project);
			token = anjuta_token_find_position (configure, TRUE, info->token_type, NULL);
			if (token == NULL)
			{
				token = skip_comment (configure);
				if (token == NULL)
				{
					token = anjuta_token_append_child (configure, anjuta_token_new_string (COMMENT | ANJUTA_TOKEN_ADDED, "#"));
					token = anjuta_token_insert_after (token, anjuta_token_new_string (SPACE | ANJUTA_TOKEN_ADDED, " Created by Anjuta project manager"));
					token = anjuta_token_insert_after (token, anjuta_token_new_string (END_OF_LINE | ANJUTA_TOKEN_ADDED, "\n"));
					token = anjuta_token_insert_after (token, anjuta_token_new_string (END_OF_LINE | ANJUTA_TOKEN_ADDED, "\n"));
				}
			}

			suffix = info->suffix;
			token = anjuta_token_insert_after (token, anjuta_token_new_string (AC_TOKEN_AC_INIT | ANJUTA_TOKEN_ADDED, suffix));
			if (suffix[strlen(suffix) - 1] == '(')
			{
				group = anjuta_token_insert_after (token, anjuta_token_new_static (ANJUTA_TOKEN_LIST | ANJUTA_TOKEN_ADDED, NULL));
				args = group;
				token = anjuta_token_insert_after (group, anjuta_token_new_static (ANJUTA_TOKEN_LAST | ANJUTA_TOKEN_ADDED, NULL));
				anjuta_token_merge (group, token);
			}
			anjuta_token_insert_after (token, anjuta_token_new_string (END_OF_LINE | ANJUTA_TOKEN_ADDED, "\n"));
		}
		if (args != NULL)
		{
			guint pos;

			token = anjuta_token_new_string (ANJUTA_TOKEN_NAME | ANJUTA_TOKEN_ADDED, prop->base.value);
			arg = anjuta_token_insert_before (token, anjuta_token_new_static (ANJUTA_TOKEN_ITEM | ANJUTA_TOKEN_ADDED, NULL));
			anjuta_token_merge (arg, token);

			pos = info->position;
			if (pos == -1) pos = 0;
			anjuta_token_replace_nth_word (args, pos, arg);
			anjuta_token_style_format (project->arg_list, args);
		}
	}

	amp_project_update_configure (project, token);

	return TRUE;
}

/* Module objects
 *---------------------------------------------------------------------------*/

static AnjutaToken *
amp_project_write_module_list (AmpProject *project, const gchar *name, gboolean after, AnjutaToken* sibling)
{
	AnjutaToken *pos;
	AnjutaToken *token;
	AnjutaToken *configure;

	configure = amp_project_get_configure_token (project);

	pos = anjuta_token_find_position (configure, after, AC_TOKEN_PKG_CHECK_MODULES, sibling);

	pos = anjuta_token_insert_token_list (after, pos,
		    ANJUTA_TOKEN_EOL, "\n",
		    NULL);

	pos = anjuta_token_insert_token_list (after, pos,
		    ANJUTA_TOKEN_EOL, "\n",
		    NULL);
	amp_project_update_configure (project, pos);

	token = anjuta_token_insert_token_list (FALSE, pos,
	    		AC_TOKEN_AC_CONFIG_FILES, "PKG_CHECK_MODULES(",
	    		ANJUTA_TOKEN_LIST, NULL,
	    		ANJUTA_TOKEN_NAME, name,
	    		ANJUTA_TOKEN_COMMA, ",",
	    		ANJUTA_TOKEN_LAST, NULL,
	    		RIGHT_PAREN, ")",
	    		NULL);

	return token;
}


gboolean
amp_module_node_create_token (AmpProject  *project, AmpModuleNode *module, GError **error)
{
	gboolean after;
	AnjutaToken *token;
	AnjutaToken *prev;
	AnjutaToken *next;
	AnjutaProjectNode *sibling;

	/* Add in configure.ac */
	/* Find a sibling if possible */
	prev = NULL;
	after = TRUE;
	for (sibling = anjuta_project_node_prev_sibling (ANJUTA_PROJECT_NODE (module)); sibling != NULL; sibling = anjuta_project_node_prev_sibling (sibling))
	{
		if (anjuta_project_node_get_node_type (sibling) == ANJUTA_PROJECT_MODULE)
		{
			prev = amp_module_node_get_token (AMP_MODULE_NODE (sibling));
			if (prev != NULL)
			{
				prev = anjuta_token_list (prev);
				break;
			}
		}
	}
	if (prev == NULL)
	{
		after = FALSE;
		for (sibling = anjuta_project_node_next_sibling (ANJUTA_PROJECT_NODE (module)); sibling != NULL; sibling = anjuta_project_node_next_sibling (sibling))
		{
			if (anjuta_project_node_get_node_type (sibling) == ANJUTA_PROJECT_MODULE)
			{
				prev = amp_module_node_get_token (AMP_MODULE_NODE (sibling));
				if (prev != NULL)
				{
					prev = anjuta_token_list (prev);
					break;
				}
			}
		}
	}

	token = amp_project_write_module_list (project, anjuta_project_node_get_name (ANJUTA_PROJECT_NODE (module)), after, prev);
	next = anjuta_token_next (token);
	next = anjuta_token_next (next);
	next = anjuta_token_next (next);
	amp_module_node_add_token (module, next);

	amp_project_update_configure (project, token);

	return TRUE;
}

gboolean
amp_module_node_delete_token (AmpProject  *project, AmpModuleNode *module, GError **error)
{
	AnjutaToken *token;

	token = amp_module_node_get_token (module);
	if (token != NULL)
	{
		AnjutaToken *eol;

		token = anjuta_token_list (token);
		anjuta_token_set_flags (token, ANJUTA_TOKEN_REMOVED);
		eol = anjuta_token_next_item (token);
		if (anjuta_token_get_type (eol) == ANJUTA_TOKEN_EOL)
		{
			anjuta_token_set_flags (eol, ANJUTA_TOKEN_REMOVED);
		}
		eol = anjuta_token_next_item (eol);
		if (anjuta_token_get_type (eol) == ANJUTA_TOKEN_EOL)
		{
			anjuta_token_set_flags (eol, ANJUTA_TOKEN_REMOVED);
		}

		amp_project_update_configure (project, token);
	}

	return TRUE;
}

/* Package objects
 *---------------------------------------------------------------------------*/

gboolean
amp_package_node_create_token (AmpProject  *project, AmpPackageNode *package, GError **error)
{
	AmpModuleNode *module;
	AnjutaProjectNode *sibling;
	gboolean after;
	AnjutaToken *token;
	AnjutaToken *prev;
	AnjutaToken *args;


	/* Get parent module */
	module = AMP_MODULE_NODE (anjuta_project_node_parent_type (ANJUTA_PROJECT_NODE (package), ANJUTA_PROJECT_MODULE));
	if (module == NULL) return FALSE;


	/* Add in configure.ac */
	/* Find a sibling if possible */
	if ((sibling = anjuta_project_node_prev_sibling (ANJUTA_PROJECT_NODE (package))) != NULL)
	{
		prev = amp_package_node_get_token (AMP_PACKAGE_NODE (sibling));
		after = TRUE;
		args = anjuta_token_list (prev);
	}
	else if ((sibling = anjuta_project_node_next_sibling (ANJUTA_PROJECT_NODE (package))) != NULL)
	{
		prev = amp_package_node_get_token (AMP_PACKAGE_NODE (sibling));
		after = FALSE;
		args = anjuta_token_list (prev);
	}
	else
	{
		prev = NULL;
		args = NULL;
	}

	/* Check if a valid source variable is already defined */
	if (args == NULL)
	{
		args = amp_module_node_get_token (module);
	}

	if (args != NULL)
	{
		AnjutaTokenStyle *style;
		const gchar *name;

		name = anjuta_project_node_get_name (ANJUTA_PROJECT_NODE (package));
		style = anjuta_token_style_new_from_base (project->ac_space_list);
		//anjuta_token_style_update (style, args);

		token = anjuta_token_new_string (ANJUTA_TOKEN_NAME | ANJUTA_TOKEN_ADDED, name);

		if (after)
		{
			anjuta_token_insert_word_after (args, prev, token);
		}
		else
		{
			anjuta_token_insert_word_before (args, prev, token);
		}

		/* Try to use the same style than the current target list */
		anjuta_token_style_format (style, args);
		anjuta_token_style_free (style);

		amp_project_update_configure (project, token);

		amp_package_node_add_token (package, token);
	}

	return TRUE;
}

gboolean
amp_package_node_delete_token (AmpProject  *project, AmpPackageNode *package, GError **error)
{
	AnjutaProjectNode *module;
	AnjutaToken *token;

	/* Get parent module */
	module = anjuta_project_node_parent_type (ANJUTA_PROJECT_NODE (package), ANJUTA_PROJECT_MODULE);
	if (module == NULL)
	{
		return FALSE;
	}

	token = amp_package_node_get_token (package);
	if (token != NULL)
	{
		AnjutaToken *args;
		AnjutaTokenStyle *style;

		args = anjuta_token_list (token);

		/* Try to use the same style than the current target list */
		style = anjuta_token_style_new_from_base (project->ac_space_list);
		anjuta_token_style_update (style, args);

		anjuta_token_remove_word (token);

		anjuta_token_style_format (style, args);
		anjuta_token_style_free (style);

		amp_project_update_configure (project, args);
	}

	return TRUE;
}
