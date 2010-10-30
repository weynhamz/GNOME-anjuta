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

#include "ac-writer.h"
#include "ac-scanner.h"
#include "ac-parser.h"
#include "am-node.h"

#include "am-project-private.h"

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-utils.h>

/* Types
  *---------------------------------------------------------------------------*/


/* Helper functions
 *---------------------------------------------------------------------------*/

/* Private functions
 *---------------------------------------------------------------------------*/

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
find_next_eol (AnjutaToken *token)
{
	if (token == NULL) return NULL;
	
	for (;;)
	{
		AnjutaToken *next = anjuta_token_next (token);

		if (next == NULL) return token;
		token = next;
		if (anjuta_token_get_type (token) == EOL) return token;
	}
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
			if (anjuta_token_get_type (token) == EOL) break;
		}
	}
}

/* Public functions
 *---------------------------------------------------------------------------*/

gboolean
amp_project_update_ac_property (AmpProject *project, AnjutaProjectProperty *property)
{
	AnjutaToken *token;
	AnjutaToken *arg;
	guint pos;
	const gchar *value;

	pos = ((AmpProperty *)property)->position;
	value = ((AmpProperty *)property)->base.value;
	
	if (project->ac_init == NULL)
	{
		gint types[] = {AC_TOKEN_AC_PREREQ, 0};
		AnjutaToken *group;
		AnjutaToken *configure;

		configure = amp_root_get_configure_token (ANJUTA_AM_ROOT_NODE (project->root));
		token = find_tokens (configure, types);
		if (token == NULL)
		{
			token = skip_comment (configure);
			if (token == NULL)
			{
				token = anjuta_token_append_child (configure, anjuta_token_new_string (COMMENT | ANJUTA_TOKEN_ADDED, "#"));
				token = anjuta_token_insert_after (token, anjuta_token_new_string (SPACE | ANJUTA_TOKEN_ADDED, " Created by Anjuta project manager"));
				token = anjuta_token_insert_after (token, anjuta_token_new_string (EOL | ANJUTA_TOKEN_ADDED, "\n"));
				token = anjuta_token_insert_after (token, anjuta_token_new_string (EOL | ANJUTA_TOKEN_ADDED, "\n"));
			}
		}
		
		token = anjuta_token_insert_before (token, anjuta_token_new_string (AC_TOKEN_AC_INIT | ANJUTA_TOKEN_ADDED, "AC_INIT("));
		project->ac_init = token;
		group = anjuta_token_insert_after (token, anjuta_token_new_static (ANJUTA_TOKEN_LIST | ANJUTA_TOKEN_ADDED, NULL));
		project->args = group;
		token = anjuta_token_insert_after (group, anjuta_token_new_static (ANJUTA_TOKEN_LAST | ANJUTA_TOKEN_ADDED, NULL));
		anjuta_token_merge (group, token);
		anjuta_token_insert_after (token, anjuta_token_new_string (EOL | ANJUTA_TOKEN_ADDED, "\n"));
		//fprintf(stdout, "whole file\n");
		//anjuta_token_dump (project->configure_token);
	}
	//fprintf(stdout, "ac_init before replace\n");
	//anjuta_token_dump (project->args);
	token = anjuta_token_new_string (ANJUTA_TOKEN_NAME | ANJUTA_TOKEN_ADDED, value);
	arg = anjuta_token_insert_before (token, anjuta_token_new_static (ANJUTA_TOKEN_ITEM | ANJUTA_TOKEN_ADDED, NULL));
	anjuta_token_merge (arg, token);
	anjuta_token_replace_nth_word (project->args, pos, arg);
	//fprintf(stdout, "ac_init after replace\n");
	//anjuta_token_dump (project->args);
	//fprintf(stdout, "ac_init after replace link\n");
	//anjuta_token_dump_link (project->args);
	anjuta_token_style_format (project->arg_list, project->args);
	//fprintf(stdout, "ac_init after update link\n");
	//anjuta_token_dump (project->args);
	amp_root_update_configure (ANJUTA_AM_ROOT_NODE (project->root), token);
	
	return TRUE;
}

/* Module objects
 *---------------------------------------------------------------------------*/

static AnjutaToken *
amp_project_write_module_list (AnjutaAmRootNode *root, const gchar *name, gboolean after, AnjutaToken* sibling)
{
	AnjutaToken *pos;
	AnjutaToken *token;
	static gint pkg_type[] = {AC_TOKEN_PKG_CHECK_MODULES, 0};
	static gint eol_type[] = {ANJUTA_TOKEN_EOL, ANJUTA_TOKEN_SPACE, ANJUTA_TOKEN_COMMENT, 0};
	AnjutaToken *configure;

	configure = amp_root_get_configure_token (root);
	
	if (sibling == NULL)
	{
		pos = anjuta_token_find_type (configure, 0, pkg_type);
		if (pos == NULL)
		{
			gint other_type[] = {AC_TOKEN_AC_INIT,
				AC_TOKEN_PKG_CHECK_MODULES,
				AC_TOKEN_AC_PREREQ,
				0};
			
			pos = anjuta_token_find_type (configure, ANJUTA_TOKEN_SEARCH_LAST, other_type);
			if (pos == NULL)
			{
				pos = anjuta_token_skip_comment (configure);
			}
			else
			{
				AnjutaToken* next;

				next = anjuta_token_find_type (pos, ANJUTA_TOKEN_SEARCH_NOT, eol_type);
			}
		}
		
	}
	else
	{
		pos = sibling;
	}

	if (after && (pos != NULL))
	{
		token = anjuta_token_find_type (pos, 0, eol_type);
		if (token != NULL)
		{
			pos = token;
		}
		else
		{
			pos = anjuta_token_insert_token_list (after, pos,
			    ANJUTA_TOKEN_EOL, "\n",
			    NULL);
			amp_root_update_configure (root, pos);
		}
	}
	
	pos = anjuta_token_insert_token_list (after, pos,
		    ANJUTA_TOKEN_EOL, "\n",
		    NULL);
	amp_root_update_configure (root, pos);

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
amp_module_create_token (AmpProject  *project, AnjutaAmModuleNode *module, GError **error)
{
	AnjutaAmRootNode *root;
	gboolean after;
	AnjutaToken *token;
	AnjutaToken *prev;
	AnjutaProjectNode *sibling;
	gchar *relative_name;

	/* Get parent target */
	root = ANJUTA_AM_ROOT_NODE (anjuta_project_node_parent (ANJUTA_PROJECT_NODE (module)));
	if ((root == NULL) || (anjuta_project_node_get_node_type (root) != ANJUTA_PROJECT_ROOT)) return FALSE;

	
	/* Add in configure.ac */
	/* Find a sibling if possible */
	prev = NULL;
	after = TRUE;
	for (sibling = anjuta_project_node_prev_sibling (ANJUTA_PROJECT_NODE (module)); sibling != NULL; sibling = anjuta_project_node_prev_sibling (sibling))
	{
		if (anjuta_project_node_get_node_type (sibling) == ANJUTA_PROJECT_MODULE)
		{
			prev = amp_module_get_token (ANJUTA_AM_MODULE_NODE (sibling));
			if (prev != NULL) break;
		}
	}
	if (prev == NULL)
	{
		after = FALSE;
		for (sibling = anjuta_project_node_next_sibling (ANJUTA_PROJECT_NODE (module)); sibling != NULL; sibling = anjuta_project_node_next_sibling (sibling))
		{
			if (anjuta_project_node_get_node_type (sibling) == ANJUTA_PROJECT_MODULE)
			{
				prev = amp_module_get_token (ANJUTA_AM_MODULE_NODE (sibling));
				if (prev != NULL) break;
			}
		}
	}

	token = amp_project_write_module_list (root, anjuta_project_node_get_name (ANJUTA_PROJECT_NODE (module)), after, prev);
	amp_root_update_configure (root, token);

	return TRUE;
}

gboolean 
amp_module_delete_token (AmpProject  *project, AnjutaAmModuleNode *module, GError **error)
{
	AnjutaProjectNode *root;
	AnjutaToken *token;

	/* Get root node */
	root = anjuta_project_node_parent (ANJUTA_PROJECT_NODE (module));
	if ((root == NULL) || (anjuta_project_node_get_node_type (root) != ANJUTA_PROJECT_ROOT))
	{
		return FALSE;
	}

	token = amp_module_get_token (module);
	g_message ("amp_module_delete_token %p", token);
	if (token != NULL)
	{
		token = anjuta_token_list (token);
		anjuta_token_set_flags (token, ANJUTA_TOKEN_REMOVED);

		amp_root_update_configure (ANJUTA_AM_ROOT_NODE (root), token);
	}

	return TRUE;
}
