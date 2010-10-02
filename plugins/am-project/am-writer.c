/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* am-writer.c
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

#include "am-writer.h"
#include "ac-scanner.h"
#include "ac-parser.h"

#include "am-project-private.h"
#include "am-node.h"

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-utils.h>

#include <string.h>

/* Types
  *---------------------------------------------------------------------------*/


/* Helper functions
 *---------------------------------------------------------------------------*/

/* Private functions
 *---------------------------------------------------------------------------*/

/* Public functions
 *---------------------------------------------------------------------------*/

AnjutaToken *
amp_project_write_config_list (AmpProject *project)
{
	AnjutaToken *pos;
	AnjutaToken *token;
	static gint output_type[] = {AC_TOKEN_AC_OUTPUT, 0};
	static gint eol_type[] = {ANJUTA_TOKEN_EOL, ANJUTA_TOKEN_SPACE, ANJUTA_TOKEN_COMMENT, 0};
	
	pos = anjuta_token_find_type (project->configure_token, 0, output_type);
	if (pos == NULL)
	{
		gint other_type[] = {AC_TOKEN_AC_INIT,
			AC_TOKEN_PKG_CHECK_MODULES,
			AC_TOKEN_AC_CONFIG_FILES, 
			AC_TOKEN_OBSOLETE_AC_OUTPUT,
			AC_TOKEN_AC_PREREQ,
			0};
			
		pos = anjuta_token_find_type (project->configure_token, ANJUTA_TOKEN_SEARCH_LAST, other_type);
		if (pos == NULL)
		{
			pos = anjuta_token_skip_comment (project->configure_token);
		}
		else
		{
			AnjutaToken* next;

			next = anjuta_token_find_type (pos, ANJUTA_TOKEN_SEARCH_NOT, eol_type);
		}
		
	}

	token = anjuta_token_insert_token_list (FALSE, pos,
	    		AC_TOKEN_AC_CONFIG_FILES, "AC_CONFIG_FILES(",
	    		ANJUTA_TOKEN_LIST, NULL,
	    		ANJUTA_TOKEN_LAST, NULL,
	    		RIGHT_PAREN, ")",
	    		NULL);
	
	return token;
}

static AnjutaToken *
amp_project_write_subdirs_list (AnjutaAmGroupNode *project)
{
	AnjutaToken *pos = NULL;
	AnjutaToken *token;
	static gint eol_type[] = {ANJUTA_TOKEN_EOL, ANJUTA_TOKEN_SPACE, ANJUTA_TOKEN_COMMENT, 0};
	
	pos = anjuta_token_find_type (pos, ANJUTA_TOKEN_SEARCH_NOT, eol_type);

	token = anjuta_token_insert_token_list (FALSE, pos,
	    		AC_TOKEN_AC_CONFIG_FILES, "AC_CONFIG_FILES(",
	    		ANJUTA_TOKEN_LIST, NULL,
	    		ANJUTA_TOKEN_LAST, NULL,
	    		RIGHT_PAREN, ")",
	    		NULL);
	
	return token;
}

AnjutaToken *
amp_project_write_config_file (AmpProject *project, AnjutaToken *list, gboolean after, AnjutaToken *sibling, const gchar *filename)
{
	AnjutaToken *token;

	token = anjuta_token_new_string (ANJUTA_TOKEN_NAME | ANJUTA_TOKEN_ADDED, filename);
	//fprintf (stdout, "Dump config list:\n");
	//anjuta_token_dump (list);
	if (after)
	{
		anjuta_token_insert_word_after (list, sibling, token);
	}
	else
	{
		anjuta_token_insert_word_before (list, sibling, token);
	}
	//fprintf (stdout, "Dump config list after insertion:\n");
	//anjuta_token_dump (list);
	
	anjuta_token_style_format (project->ac_space_list, list);
	
	//fprintf (stdout, "Dump config list after format:\n");
	//anjuta_token_dump (list);
	
	anjuta_token_file_update (project->configure_file, list);
	
	return token;
}

static AnjutaToken *
amp_project_write_target (AnjutaAmGroupNode *group, gint type, const gchar *name, gboolean after, AnjutaToken* sibling)
{
	AnjutaToken *pos;
	AnjutaToken *token;
	AnjutaToken *makefile;


	
	if (sibling == NULL)
	{
		makefile = amp_group_get_makefile_token (group);
		pos = anjuta_token_first_item (makefile);
		if (pos == NULL)
		{
			/* Empty file */
			token = anjuta_token_new_string (ANJUTA_TOKEN_COMMENT | ANJUTA_TOKEN_ADDED, "## Process this file with automake to produce Makefile.in\n");
			anjuta_token_append_child (makefile, token);
			amp_group_update_makefile (group, token);
			pos = token;
		}
		else
		{
				/* Add at the end of the file */
			while (anjuta_token_next_item (pos) != NULL)
			{
				pos = anjuta_token_next_item (pos);
			}
		}
	}
	else
	{
		pos = sibling;
	}

	token = anjuta_token_new_string (ANJUTA_TOKEN_EOL | ANJUTA_TOKEN_ADDED, "\n");
	anjuta_token_insert_after (pos, token);
	amp_group_update_makefile (group, token);
	pos = token;
	
	token = anjuta_token_insert_token_list (after, pos,
	    		ANJUTA_TOKEN_LIST, NULL,
	    		type, name,
	    		ANJUTA_TOKEN_SPACE, " ",
	    		ANJUTA_TOKEN_OPERATOR, "=",
	    		ANJUTA_TOKEN_LIST, NULL,
	            ANJUTA_TOKEN_SPACE, " ",
	    		NULL);
	token = anjuta_token_last_item (token);
	
	return token;
}

gboolean 
amp_target_create_token (AmpProject  *project, AnjutaAmTargetNode *target, GError **error)
{
	AnjutaToken* token;
	AnjutaToken *args;
	AnjutaToken *var;
	AnjutaToken *prev;
	AmpNodeInfo *info;
	gchar *targetname;
	gchar *name;
	GList *last;
	AnjutaAmTargetNode *sibling;
	AnjutaAmGroupNode *parent;
	gboolean after;

	/* Get parent target */
	parent = ANJUTA_AM_GROUP_NODE (anjuta_project_node_parent (ANJUTA_PROJECT_NODE (target)));
	
	info = (AmpNodeInfo *)amp_project_get_type_info (project, anjuta_project_node_get_full_type (ANJUTA_PROJECT_NODE (target)));
	name = anjuta_project_node_get_name (ANJUTA_PROJECT_NODE (target));

	/* Find a sibling if possible */
	if (target->base.prev != NULL)
	{
		sibling = ANJUTA_AM_TARGET_NODE (target->base.prev);
		after = TRUE;
	}
	else if (target->base.next != NULL)
	{
		sibling = ANJUTA_AM_TARGET_NODE (target->base.next);
		after = FALSE;
	}
	else
	{
		sibling = NULL;
		after = TRUE;
	}
	
	/* Add in Makefile.am */
	targetname = g_strconcat (info->install, info->prefix, NULL);

	// Get token corresponding to sibling and check if the target are compatible
	args = NULL;
	var = NULL;
	prev = NULL;
	if (sibling != NULL)
	{
		last = amp_target_get_token (sibling);

		if (last != NULL) 
		{
			AnjutaToken *token = (AnjutaToken *)last->data;

			/* Check that the sibling is of the same kind */
			token = anjuta_token_list (token);
			if (token != NULL)
			{
				token = anjuta_token_list (token);
				var = token;
				if (token != NULL)
				{
					token = anjuta_token_first_item (token);
					if (token != NULL)
					{
						gchar *value;
						
						value = anjuta_token_evaluate (token);

						if ((value != NULL) && (strcmp (targetname, value) == 0))
						{
							g_free (value);
							prev = (AnjutaToken *)last->data;
							args = anjuta_token_list (prev);
						}
					}
				}
			}
		}
	}


	/* Check if a valid target variable is already defined */
	if (args == NULL)
	{
		for (last = amp_group_get_token (parent, AM_GROUP_TARGET); last != NULL; last = g_list_next (last))
		{
			gchar *value = anjuta_token_evaluate ((AnjutaToken *)last->data);

			if ((value != NULL) && (strcmp (targetname, value) == 0))
			{
				g_free (value);
				args = anjuta_token_last_item (anjuta_token_list ((AnjutaToken *)last->data));
				break;
			}
			g_free (value);
		}
	}

	
	if (args == NULL)
	{
		args = amp_project_write_target (parent, info->token, targetname, after, var);
	}
	g_free (targetname);

	if (args != NULL)
	{
		AnjutaTokenStyle *style;

		style = anjuta_token_style_new_from_base (project->am_space_list);
		anjuta_token_style_update (style, args);
		
		token = anjuta_token_new_string (ANJUTA_TOKEN_ARGUMENT | ANJUTA_TOKEN_ADDED, name);
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
		
		amp_group_update_makefile (parent, token);
		
		amp_target_add_token (target, token);
	}
	g_free (name);

	return TRUE;
}

gboolean 
amp_target_delete_token (AmpProject  *project, AnjutaAmTargetNode *target, GError **error)
{
	GList *item;
	AnjutaAmGroupNode *parent;

	/* Get parent target */
	parent = ANJUTA_AM_GROUP_NODE (anjuta_project_node_parent (ANJUTA_PROJECT_NODE (target)));

	for (item = amp_target_get_token (target); item != NULL; item = g_list_next (item))
	{
		AnjutaToken *token = (AnjutaToken *)item->data;
		AnjutaToken *args;
		AnjutaTokenStyle *style;

		args = anjuta_token_list (token);

		/* Try to use the same style than the current target list */
		style = anjuta_token_style_new_from_base (project->am_space_list);
		anjuta_token_style_update (style, args);
		
		anjuta_token_remove_word (token);
		
		anjuta_token_style_format (style, args);
		anjuta_token_style_free (style);

		amp_group_update_makefile (parent, args);
	}

	return TRUE;
}

AnjutaToken *
amp_project_write_source_list (AnjutaToken *makefile, const gchar *name, gboolean after, AnjutaToken* sibling)
{
	AnjutaToken *pos;
	AnjutaToken *token;
	static gint eol_type[] = {ANJUTA_TOKEN_EOL, 0};
	
	if (sibling == NULL)
	{
		pos = anjuta_token_first_item (makefile);
		
		/* Add at the end of the file */
		while (anjuta_token_next_item (pos) != NULL)
		{
			pos = anjuta_token_next_item (pos);
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
		}
	}
	
	token = anjuta_token_insert_token_list (after, pos,
	    		ANJUTA_TOKEN_LIST, NULL,
	    		ANJUTA_TOKEN_NAME, name,
	    		ANJUTA_TOKEN_SPACE, " ",
	    		ANJUTA_TOKEN_OPERATOR, "=",
	    		ANJUTA_TOKEN_LIST, NULL,
	    		ANJUTA_TOKEN_START, NULL,
	    		NULL);

	return anjuta_token_last_item (token);
}
