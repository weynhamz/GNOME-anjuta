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
#include "amp-node.h"
#include "amp-group.h"
#include "amp-target.h"
#include "amp-source.h"
#include "am-scanner.h"

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-utils.h>

#include <string.h>
#include <ctype.h>

/* Types
  *---------------------------------------------------------------------------*/


/* Helper functions
 *---------------------------------------------------------------------------*/

/* Private functions
 *---------------------------------------------------------------------------*/

static AnjutaToken *
anjuta_token_find_target_property_position (AmpTargetNode *target,
                                            AnjutaTokenType type)
{
	AnjutaToken *pos = NULL;
	gboolean after = FALSE;
	GList *list;
	AmpGroupNode *group;
	AnjutaToken *makefile;
	
	group = AMP_GROUP_NODE (anjuta_project_node_parent (ANJUTA_PROJECT_NODE (target)));
	
	/* Try to find a better position */

	/* 1. With the other properties of the target */
	list = amp_target_node_get_all_token (target);
	if (list != NULL)
	{
		GList *link;
		AnjutaTokenType best = 0;

		for (link = list; link != NULL; link = g_list_next (link))
		{
			AnjutaToken *token = (AnjutaToken *)link->data;
			AnjutaTokenType existing = anjuta_token_get_type (token);

			if ((existing < AM_TOKEN_FIRST_ORDERED_TARGET_MACRO) || (existing > AM_TOKEN_LAST_ORDERED_TARGET_MACRO))
			{
				token = anjuta_token_list (token);
				if (token != NULL) existing = anjuta_token_get_type (token);
			}

			if ((existing >= AM_TOKEN_FIRST_ORDERED_TARGET_MACRO) && (existing <= AM_TOKEN_LAST_ORDERED_TARGET_MACRO))
			{
				if (existing > type)
				{
					if ((best == 0) || ((existing - type) < best))
					{
						best = existing - type;
						pos = token;
						after = FALSE;
					}
				}
				else
				{
					if ((best == 0) || ((type -existing) < best))
					{
						best = type - existing;
						pos = token;
						after = TRUE;
					}
				}
			}
		}
		g_list_free (list);
	}				

	
	/* 2. With properties of sibling targets */
	if (pos == NULL)
	{
		AnjutaProjectNode *prev = ANJUTA_PROJECT_NODE (target);
		AnjutaProjectNode *next = ANJUTA_PROJECT_NODE (target);
		AmpTargetNode *sibling;
		AnjutaTokenFile *makefile;
		AnjutaToken *target_list = NULL;
		GList *link;

		link = amp_target_node_get_token (target, ANJUTA_TOKEN_ARGUMENT);
		if ((link != NULL) && (link->data != NULL))
		{
			target_list = anjuta_token_list ((AnjutaToken *)link->data);
		}
		
		makefile = amp_group_node_get_make_token_file (group);

		if (makefile != NULL)
		{
			after = TRUE;
			while ((prev != NULL) || (next != NULL))
			{
				/* Find sibling */
				if (after)
				{
					while (prev != NULL)
					{
						prev = anjuta_project_node_prev_sibling (prev);
						if (anjuta_project_node_get_node_type (prev) == ANJUTA_PROJECT_TARGET) break;
					}
					sibling = AMP_TARGET_NODE (prev);
				}
				else
				{
					while (next != NULL)
					{
						next = anjuta_project_node_next_sibling (next);
						if (anjuta_project_node_get_node_type (next) == ANJUTA_PROJECT_TARGET) break;
					}
					sibling = AMP_TARGET_NODE (next);
				}
				list = sibling == NULL ? NULL : amp_target_node_get_all_token (sibling);

				/* Check that the target is in the same list */
				if ((list != NULL) && (target_list != NULL))
				{
					AnjutaToken *token;

					link = amp_target_node_get_token (sibling, ANJUTA_TOKEN_ARGUMENT);
					if ((link != NULL) && (link->data != NULL))
					{
						token = anjuta_token_list ((AnjutaToken *)link->data);
					}

					if ((token != NULL) && (target_list != token))
					{
						/* Target is in another list, do not use it, nor following ones */
						list = NULL;
						if (after)
						{
							prev = NULL;
						}
						else
						{
							next = NULL;
						}
					}
				}
				
				if (list != NULL)
				{
					gsize best = 0;
				
					for (link = list; link != NULL; link = g_list_next (link))
					{
						AnjutaToken *token = (AnjutaToken *)link->data;
						AnjutaTokenType existing = anjuta_token_get_type (token);

						if ((existing < AM_TOKEN_FIRST_ORDERED_TARGET_MACRO) || (existing > AM_TOKEN_LAST_ORDERED_TARGET_MACRO))
						{
							token = anjuta_token_list (token);
							if (token != NULL) existing = anjuta_token_get_type (token);
						}
						
						if ((existing >= AM_TOKEN_FIRST_ORDERED_TARGET_MACRO) && (existing <= AM_TOKEN_LAST_ORDERED_TARGET_MACRO))
						{
							gsize tpos;

							tpos = anjuta_token_file_get_token_position (makefile, token);

							if ((best == 0) ||
							    (after && (tpos > best)) ||
							    (!after && (tpos < best)))
							{
								pos = token;
								best = tpos;
							}
						}
					}
					g_list_free (list);
					list = NULL;

					if (best != 0) break;
				}

				after = after ? FALSE : TRUE;
			}
		}
	}


	/* 3. After target declaration */
	if (pos == NULL)
	{
		list = amp_target_node_get_token (AMP_TARGET_NODE (target), ANJUTA_TOKEN_ARGUMENT);
		if (list != NULL)
		{
			pos = (AnjutaToken *)list->data;
			if (pos != NULL)
			{
				pos = anjuta_token_list (pos);
				if (pos != NULL)
				{
					pos = anjuta_token_list (pos);
				}
			}
		}
		after = TRUE;
	}

	/* 4. At the end of the file */
	if (pos == NULL)
	{
		makefile = amp_group_node_get_makefile_token (group);
		
		for (pos = anjuta_token_first_item (makefile); (pos != NULL) && (anjuta_token_next_item (pos) != NULL); pos = anjuta_token_next_item (pos));
		
		after = TRUE;
	}

	/* 5. Create new file */
	if (pos == NULL)
	{
		/* Empty file */
		pos = anjuta_token_new_string (ANJUTA_TOKEN_COMMENT | ANJUTA_TOKEN_ADDED, "## Process this file with automake to produce Makefile.in\n");
		anjuta_token_append_child (makefile, pos);
		amp_group_node_update_makefile (group, pos);
	}
	
	
	/* Find end of line */
	if (after)
	{
		while (pos != NULL)
		{
			if (anjuta_token_get_type (pos) == ANJUTA_TOKEN_EOL) break;
			if (anjuta_token_next (pos) == NULL)
			{
				pos = anjuta_token_insert_token_list (after, pos,
					ANJUTA_TOKEN_EOL, "\n",
					NULL);
				
				break;
			}
			pos = anjuta_token_next (pos);
		}
	}

	pos = anjuta_token_insert_token_list (after, pos,
		    ANJUTA_TOKEN_EOL, "\n",
		    NULL);
	pos = anjuta_token_insert_token_list (after, pos,
		    ANJUTA_TOKEN_EOL, "\n",
		    NULL);
	amp_group_node_update_makefile (group, pos);

	
	return pos;
}

static AnjutaToken *
anjuta_token_find_group_property_position (AmpGroupNode *group,
                                            AnjutaTokenType type)
{
	AnjutaToken *pos = NULL;
	gboolean after = FALSE;
	GList *list;
	AnjutaToken *makefile;

	
	/* Try to find a better position */

	/* 1. With the other properties of the group */
	list = amp_group_node_get_all_token (group);
	if (list != NULL)
	{
		GList *link;
		AnjutaTokenType best = 0;

		for (link = list; link != NULL; link = g_list_next (link))
		{
			AnjutaToken *token = (AnjutaToken *)link->data;
			AnjutaTokenType existing = anjuta_token_get_type (token);

			if ((existing < AM_TOKEN_FIRST_ORDERED_TARGET_MACRO) || (existing > AM_TOKEN_LAST_ORDERED_TARGET_MACRO))
			{
				token = anjuta_token_list (token);
				if (token != NULL) existing = anjuta_token_get_type (token);
			}

			if ((existing >= AM_TOKEN_FIRST_ORDERED_TARGET_MACRO) && (existing <= AM_TOKEN_LAST_ORDERED_TARGET_MACRO))
			{
				if (existing > type)
				{
					if ((best == 0) || ((existing - type) < best))
					{
						best = existing - type;
						pos = token;
						after = FALSE;
					}
				}
				else
				{
					if ((best == 0) || ((type -existing) < best))
					{
						best = type - existing;
						pos = token;
						after = TRUE;
					}
				}
			}
		}
		g_list_free (list);
	}				

	/* 2. At the end of the file */
	if (pos == NULL)
	{
		makefile = amp_group_node_get_makefile_token (group);
		anjuta_token_dump (makefile);
		
		for (pos = anjuta_token_first_item (makefile); (pos != NULL) && (anjuta_token_next_item (pos) != NULL); pos = anjuta_token_next_item (pos));
		
		after = TRUE;
	}

	/* 3. Create new file */
	if (pos == NULL)
	{
		/* Empty file */
		pos = anjuta_token_new_string (ANJUTA_TOKEN_COMMENT | ANJUTA_TOKEN_ADDED, "## Process this file with automake to produce Makefile.in\n");
		anjuta_token_append_child (makefile, pos);
		amp_group_node_update_makefile (group, pos);
	}

	/* Find end of line */
	if (after)
	{
		while (pos != NULL)
		{
			if (anjuta_token_get_type (pos) == ANJUTA_TOKEN_EOL) break;
			if (anjuta_token_next (pos) == NULL)
			{
				pos = anjuta_token_insert_token_list (after, pos,
					ANJUTA_TOKEN_EOL, "\n",
					NULL);
				
				break;
			}
			pos = anjuta_token_next (pos);
		}
	}

	pos = anjuta_token_insert_token_list (after, pos,
		    ANJUTA_TOKEN_EOL, "\n",
		    NULL);
	pos = anjuta_token_insert_token_list (after, pos,
		    ANJUTA_TOKEN_EOL, "\n",
		    NULL);
	amp_group_node_update_makefile (group, pos);

	
	return pos;
}


/* Public functions
 *---------------------------------------------------------------------------*/

static AnjutaToken *
amp_project_write_config_list (AmpProject *project)
{
	AnjutaToken *pos;
	AnjutaToken *token;
	static gint output_type[] = {AC_TOKEN_AC_OUTPUT, 0};
	static gint eol_type[] = {ANJUTA_TOKEN_EOL, ANJUTA_TOKEN_SPACE, ANJUTA_TOKEN_COMMENT, 0};
	AnjutaToken *configure;
	
	configure = amp_project_get_configure_token (project);
	pos = anjuta_token_find_type (configure, 0, output_type);
	if (pos == NULL)
	{
		gint other_type[] = {AC_TOKEN_AC_INIT,
			AC_TOKEN_PKG_CHECK_MODULES,
			AC_TOKEN_AC_CONFIG_FILES, 
			AC_TOKEN_OBSOLETE_AC_OUTPUT,
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

	token = anjuta_token_insert_token_list (FALSE, pos,
	    		AC_TOKEN_AC_CONFIG_FILES, "AC_CONFIG_FILES(",
	    		ANJUTA_TOKEN_LIST, NULL,
	    		ANJUTA_TOKEN_LAST, NULL,
	    		RIGHT_PAREN, ")",
	    		NULL);
	
	return token;
}

static AnjutaToken *
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

	amp_project_update_configure (project, list);
	
	return token;
}


/* Target objects
 *---------------------------------------------------------------------------*/

gboolean 
amp_group_node_create_token (AmpProject  *project, AmpGroupNode *group, GError **error)
{
	GFile *directory;
	GFile *makefile;
	AnjutaToken *list;
	gchar *basename;
	AnjutaTokenFile* tfile;
	AnjutaProjectNode *sibling;
	AmpGroupNode *parent;
	gboolean after;
	const gchar *name;
	
	/* Get parent target */
	parent = AMP_GROUP_NODE (anjuta_project_node_parent(ANJUTA_PROJECT_NODE (group)));
	name = anjuta_project_node_get_name (ANJUTA_PROJECT_NODE (group));
	directory = g_file_get_child (anjuta_project_node_get_file (ANJUTA_PROJECT_NODE (parent)), name);

	/* Find a sibling if possible */
	after = TRUE;
	for (sibling = anjuta_project_node_prev_sibling (ANJUTA_PROJECT_NODE (group)); sibling != NULL; sibling = anjuta_project_node_prev_sibling (sibling))
	{
		if (anjuta_project_node_get_node_type (sibling) == ANJUTA_PROJECT_GROUP) break;
	}
	if (sibling == NULL)
	{
		after = FALSE;
		for (sibling = anjuta_project_node_next_sibling (ANJUTA_PROJECT_NODE (group)); sibling != NULL; sibling = anjuta_project_node_next_sibling (sibling))
		{
			if (anjuta_project_node_get_node_type (sibling) == ANJUTA_PROJECT_GROUP) break;
		}
	}
	if (sibling == NULL) after = TRUE;
	
	/* Create directory */
	g_file_make_directory (directory, NULL, NULL);

	/* Create Makefile.am */
	basename = amp_group_node_get_makefile_name  (parent);
	if (basename != NULL)
	{
		makefile = g_file_get_child (directory, basename);
		g_free (basename);
	}
	else
	{
		makefile = g_file_get_child (directory, "Makefile.am");
	}
	g_file_replace_contents (makefile, "", 0, NULL, FALSE, G_FILE_CREATE_NONE, NULL, NULL, NULL);

	
	/* Add in configure */
	list = NULL;
	if (sibling) list = amp_group_node_get_first_token (AMP_GROUP_NODE (sibling), AM_GROUP_TOKEN_CONFIGURE);
	if (list == NULL) list= amp_group_node_get_first_token (parent, AM_GROUP_TOKEN_CONFIGURE);
	if (list != NULL) list = anjuta_token_list (list);
	if (list == NULL)
	{
		list = amp_project_write_config_list (project);
		list = anjuta_token_next (list);
	}
	if (list != NULL)
	{
		gchar *relative_make;
		gchar *ext;
		AnjutaToken *prev = NULL;
		AnjutaToken *token;

		if (sibling)
		{
			prev = amp_group_node_get_first_token (AMP_GROUP_NODE (sibling), AM_GROUP_TOKEN_CONFIGURE);
			/*if ((prev != NULL) && after)
			{
				prev = anjuta_token_next_word (prev);
			}*/
		}
		//prev_token = (AnjutaToken *)token_list->data;

		relative_make = g_file_get_relative_path (anjuta_project_node_get_file (ANJUTA_PROJECT_NODE (project)), makefile);
		ext = relative_make + strlen (relative_make) - 3;
		if (strcmp (ext, ".am") == 0)
		{
			*ext = '\0';
		}
		token = amp_project_write_config_file (project, list, after, prev, relative_make);
		amp_group_node_add_token (AMP_GROUP_NODE (group), token, AM_GROUP_TOKEN_CONFIGURE);
		g_free (relative_make);
	}

	/* Add in Makefile.am */
	if (sibling == NULL)
	{
		list = anjuta_token_find_group_property_position (parent, AM_TOKEN_SUBDIRS);

		list = anjuta_token_insert_token_list (FALSE, list,
	    		AM_TOKEN_SUBDIRS, "SUBDIRS",
		    	ANJUTA_TOKEN_SPACE, " ",
		    	ANJUTA_TOKEN_OPERATOR, "=",
	    		ANJUTA_TOKEN_LIST, NULL,
	    		ANJUTA_TOKEN_LAST, NULL,
	    		NULL);
		list = anjuta_token_next (anjuta_token_next ( anjuta_token_next (list)));
	}
	else
	{
		AnjutaToken *prev;
		
		prev = amp_group_node_get_first_token (AMP_GROUP_NODE (sibling), AM_GROUP_TOKEN_SUBDIRS);
		list = anjuta_token_list (prev);
	}

	if (list != NULL)
	{
		AnjutaToken *token;
		AnjutaToken *prev;
		AnjutaTokenStyle *style;

		style = anjuta_token_style_new_from_base (project->am_space_list);
		anjuta_token_style_update (style, list);

		if (sibling)
		{
			prev = amp_group_node_get_first_token (AMP_GROUP_NODE (sibling), AM_GROUP_TOKEN_SUBDIRS);
		}
		
		token = anjuta_token_new_string (ANJUTA_TOKEN_NAME | ANJUTA_TOKEN_ADDED, name);
		if (after)
		{
			anjuta_token_insert_word_after (list, prev, token);
		}
		else
		{
			anjuta_token_insert_word_before (list, prev, token);
		}

		/* Try to use the same style than the current group list */
		anjuta_token_style_format (style, list);
		anjuta_token_style_free (style);
		
		amp_group_node_update_makefile (parent, token);

		amp_group_node_add_token (group, token, AM_GROUP_TOKEN_SUBDIRS);
	}

	tfile = amp_group_node_set_makefile (group, makefile, project);
	amp_project_add_file (project, makefile, tfile);
	
	return TRUE;
}

gboolean 
amp_group_node_delete_token (AmpProject  *project, AmpGroupNode *group, GError **error)
{
	GList *item;
	AnjutaProjectNode *parent;

	/* Get parent target */
	parent =  anjuta_project_node_parent (ANJUTA_PROJECT_NODE (group));
	if (anjuta_project_node_get_node_type  (parent) != ANJUTA_PROJECT_GROUP) return FALSE;

	for (item = amp_group_node_get_token (group, AM_GROUP_TOKEN_SUBDIRS); item != NULL; item = g_list_next (item))
	{
		AnjutaToken *token = (AnjutaToken *)item->data;
		AnjutaToken *args;
		AnjutaTokenStyle *style;
		AnjutaToken *list;

		args = anjuta_token_list (token);

		/* Try to use the same style than the current target list */
		style = anjuta_token_style_new_from_base (project->am_space_list);
		anjuta_token_style_update (style, args);
		
		anjuta_token_remove_word (token);
		anjuta_token_style_format (style, args);
		anjuta_token_style_free (style);

		/* Remove whole variable if empty */
		list = anjuta_token_list (token);
		if (anjuta_token_first_word (list) == NULL)
		{
			anjuta_token_remove_list (list);
		}
		
		amp_group_node_update_makefile (AMP_GROUP_NODE (parent), args);
	}

	/* Remove from configure file */
	for (item = amp_group_node_get_token (group, AM_GROUP_TOKEN_CONFIGURE); item != NULL; item = g_list_next (item))
	{
		AnjutaToken *token = (AnjutaToken *)item->data;
		AnjutaToken *args;
		AnjutaTokenStyle *style;

		args = anjuta_token_list (token);

		/* Try to use the same style than the current group list */
		style = anjuta_token_style_new_from_base (project->ac_space_list);
		anjuta_token_style_update (style, args);
		
		anjuta_token_remove_word (token);
		
		anjuta_token_style_format (style, args);
		anjuta_token_style_free (style);

		amp_project_update_configure (project, args);
	}	
	
	return TRUE;
}


/* Target objects
 *---------------------------------------------------------------------------*/

static AnjutaToken *
amp_project_write_target (AmpGroupNode *group, gint type, const gchar *name, gboolean after, AnjutaToken* sibling)
{
	AnjutaToken *pos;

	pos = anjuta_token_find_group_property_position (group, type);

	pos = anjuta_token_insert_token_list (FALSE, pos,
	    		ANJUTA_TOKEN_LIST, NULL,
	    		type, name,
	    		ANJUTA_TOKEN_SPACE, " ",
	    		ANJUTA_TOKEN_OPERATOR, "=",
	    		ANJUTA_TOKEN_LIST, NULL,
	            ANJUTA_TOKEN_SPACE, " ",
	    		NULL);
	pos = anjuta_token_last_item (pos);
	amp_group_node_update_makefile (group, pos);
	
	return pos;
}

gboolean 
amp_target_node_create_token (AmpProject  *project, AmpTargetNode *target, GError **error)
{
	AnjutaToken* token;
	AnjutaToken *args;
	AnjutaToken *var;
	AnjutaToken *prev;
	AmpNodeInfo *info;
	gchar *targetname;
	const gchar *name;
	GList *last;
	AnjutaProjectNode *sibling;
	AmpGroupNode *parent;
	gboolean after;

	/* Get parent target */
	parent = AMP_GROUP_NODE (anjuta_project_node_parent (ANJUTA_PROJECT_NODE (target)));
	
	info = (AmpNodeInfo *)amp_project_get_type_info (project, anjuta_project_node_get_full_type (ANJUTA_PROJECT_NODE (target)));
	name = anjuta_project_node_get_name (ANJUTA_PROJECT_NODE (target));

	/* Find a sibling if possible */
	after = TRUE;
	for (sibling = anjuta_project_node_prev_sibling (ANJUTA_PROJECT_NODE (target)); sibling != NULL; sibling = anjuta_project_node_prev_sibling (sibling))
	{
		if (anjuta_project_node_get_node_type (sibling) == ANJUTA_PROJECT_TARGET) break;
	}
	if (sibling == NULL)
	{
		after = FALSE;
		for (sibling = anjuta_project_node_next_sibling (ANJUTA_PROJECT_NODE (target)); sibling != NULL; sibling = anjuta_project_node_next_sibling (sibling))
		{
			if (anjuta_project_node_get_node_type (sibling) == ANJUTA_PROJECT_TARGET) break;
		}
	}
	if (sibling == NULL) after = TRUE;
	
	/* Add in Makefile.am */
	targetname = g_strconcat (info->install, info->prefix, NULL);

	// Get token corresponding to sibling and check if the target are compatible
	args = NULL;
	var = NULL;
	prev = NULL;
	if (sibling != NULL)
	{
		last = amp_target_node_get_token (AMP_TARGET_NODE (sibling), ANJUTA_TOKEN_ARGUMENT);

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
		for (last = amp_group_node_get_token (parent, AM_GROUP_TARGET); last != NULL; last = g_list_next (last))
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

	switch (anjuta_project_node_get_full_type (ANJUTA_PROJECT_NODE (target)) & ANJUTA_PROJECT_ID_MASK)
	{
	case ANJUTA_PROJECT_SHAREDLIB:
	case ANJUTA_PROJECT_STATICLIB:
	case ANJUTA_PROJECT_PROGRAM:
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
		
			amp_group_node_update_makefile (parent, token);
		
			amp_target_node_add_token (target, ANJUTA_TOKEN_ARGUMENT, token);
		}
		break;
	default:
		if (args != NULL)
		{
			amp_target_node_add_token (target, AM_TOKEN__SOURCES, args);
		}
		break;
	}

	return TRUE;
}

gboolean 
amp_target_node_delete_token (AmpProject  *project, AmpTargetNode *target, GError **error)
{
	GList *item;
	AmpGroupNode *parent;

	/* Get parent target */
	parent = AMP_GROUP_NODE (anjuta_project_node_parent (ANJUTA_PROJECT_NODE (target)));

	/* Remove list for data target */
	for (item = amp_target_node_get_token (target, AM_TOKEN__DATA); item != NULL; item = g_list_next (item))
	{
		AnjutaToken *token = (AnjutaToken *)item->data;
		
		anjuta_token_remove_list (token);
		amp_group_node_update_makefile (parent, token);
	}

	for (item = amp_target_node_get_token (target, ANJUTA_TOKEN_ARGUMENT); item != NULL; item = g_list_next (item))
	{
		AnjutaToken *token = (AnjutaToken *)item->data;
		AnjutaToken *args;
		AnjutaToken *list;
		AnjutaTokenStyle *style;

		args = anjuta_token_list (token);

		/* Try to use the same style than the current target list */
		style = anjuta_token_style_new_from_base (project->am_space_list);
		anjuta_token_style_update (style, args);
		
		anjuta_token_remove_word (token);
		anjuta_token_style_format (style, args);
		anjuta_token_style_free (style);

		/* Remove whole variable if empty */
		list = anjuta_token_list (token);
		if (anjuta_token_first_word (list) == NULL)
		{
			anjuta_token_remove_list (list);
		}

		amp_group_node_update_makefile (parent, args);
	}

	return TRUE;
}



/* Source objects
 *---------------------------------------------------------------------------*/


/* Source objects
 *---------------------------------------------------------------------------*/

gboolean 
amp_source_node_create_token (AmpProject  *project, AmpSourceNode *source, GError **error)
{
	AmpGroupNode *group;
	AmpTargetNode *target;
	AnjutaProjectNode *sibling;
	gboolean after;
	AnjutaToken *token;
	AnjutaToken *prev;
	AnjutaToken *args;
	gchar *relative_name;

	/* Get parent target */
	target = AMP_TARGET_NODE (anjuta_project_node_parent (ANJUTA_PROJECT_NODE (source)));
	if ((target == NULL) || (anjuta_project_node_get_node_type (ANJUTA_PROJECT_NODE (target)) != ANJUTA_PROJECT_TARGET)) return FALSE;
	
	group = AMP_GROUP_NODE (anjuta_project_node_parent (ANJUTA_PROJECT_NODE (target)));
	relative_name = get_relative_path (anjuta_project_node_get_file (ANJUTA_PROJECT_NODE (group)), anjuta_project_node_get_file (ANJUTA_PROJECT_NODE (source)));
	
	/* Add in Makefile.am */
	/* Find a sibling if possible */
	after = TRUE;
	for (sibling = anjuta_project_node_prev_sibling (ANJUTA_PROJECT_NODE (source)); sibling != NULL; sibling = anjuta_project_node_prev_sibling (sibling))
	{
		if (anjuta_project_node_get_node_type (sibling) == ANJUTA_PROJECT_SOURCE) break;
	}
	if (sibling == NULL)
	{
		after = FALSE;
		for (sibling = anjuta_project_node_next_sibling (ANJUTA_PROJECT_NODE (source)); sibling != NULL; sibling = anjuta_project_node_next_sibling (sibling))
		{
			if (anjuta_project_node_get_node_type (sibling) == ANJUTA_PROJECT_SOURCE) break;
		}
	}
	if (sibling == NULL)
	{
		after = TRUE;
		prev = NULL;
		args = NULL;
	}
	else
	{
		prev = amp_source_node_get_token (AMP_SOURCE_NODE (sibling));
		args = anjuta_token_list (prev);
	}

	/* Check if a valid source variable is already defined */
	if (args == NULL)
	{
		GList *last;
		for (last = amp_target_node_get_token (target, AM_TOKEN__SOURCES); last != NULL; last = g_list_next (last))
		{
			args = anjuta_token_last_item (anjuta_token_list ((AnjutaToken *)last->data));
			break;
		}
	}
	
	if (args == NULL)
	{
		gchar *target_var;
		gchar *canon_name;
		AnjutaToken *var;
		
		canon_name = canonicalize_automake_variable (anjuta_project_node_get_name (ANJUTA_PROJECT_NODE (target)));
		target_var = g_strconcat (canon_name,  "_SOURCES", NULL);

		var = anjuta_token_find_target_property_position (target, AM_TOKEN__SOURCES);

		args = anjuta_token_insert_token_list (FALSE, var,
					ANJUTA_TOKEN_LIST, NULL,
	    			ANJUTA_TOKEN_NAME, target_var,
		    		ANJUTA_TOKEN_SPACE, " ",
					ANJUTA_TOKEN_OPERATOR, "=",
	    			ANJUTA_TOKEN_LIST, NULL,
		            ANJUTA_TOKEN_SPACE, " ",
					NULL);

		args = anjuta_token_last_item (args);
		g_free (target_var);
	}
	
	if (args != NULL)
	{
		AnjutaTokenStyle *style;

		style = anjuta_token_style_new_from_base (project->am_space_list);
		anjuta_token_style_update (style, args);

		token = anjuta_token_new_string (ANJUTA_TOKEN_NAME | ANJUTA_TOKEN_ADDED, relative_name);
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

		amp_group_node_update_makefile (group, token);
		
		amp_source_node_add_token (source, token);
	}

	return TRUE;
}

gboolean 
amp_source_node_delete_token (AmpProject  *project, AmpSourceNode *source, GError **error)
{
	AnjutaProjectNode *group;
	AnjutaToken *token;

	/* Get parent group */
	group = anjuta_project_node_parent (ANJUTA_PROJECT_NODE (source));
	if (anjuta_project_node_get_node_type (group) == ANJUTA_PROJECT_TARGET)
	{
		group = anjuta_project_node_parent (ANJUTA_PROJECT_NODE (group));
	}
	if (group == NULL) return FALSE;

	token = amp_source_node_get_token (source);
	if (token != NULL)
	{
		AnjutaToken *args;
		AnjutaTokenStyle *style;
		AnjutaToken *list;

		args = anjuta_token_list (token);

		/* Try to use the same style than the current target list */
		style = anjuta_token_style_new_from_base (project->am_space_list);
		anjuta_token_style_update (style, args);

		anjuta_token_remove_word (token);
		anjuta_token_style_format (style, args);
		anjuta_token_style_free (style);

		/* Remove whole variable if empty */
		list = anjuta_token_list (token);
		if (anjuta_token_first_word (list) == NULL)
		{
			anjuta_token_remove_list (list);
		}
		
		amp_group_node_update_makefile (AMP_GROUP_NODE (group), args);
	}

	return TRUE;
}


/* Properties
 *---------------------------------------------------------------------------*/

static AnjutaToken*
amp_property_delete_token (AmpProject  *project, AnjutaToken *token)
{
	gboolean updated = FALSE;

	if (token != NULL)
	{
		anjuta_token_remove_list (token);
		
		updated = TRUE;
	}

	return token;
}

static AnjutaToken *
amp_project_write_property_list (AmpGroupNode *group, AnjutaProjectNode *node, AmpProperty *property)
{
	AnjutaToken *pos;
	gchar *name;
	
	if (anjuta_project_node_get_node_type (node) == ANJUTA_PROJECT_GROUP)
	{
		/* Group property */
		name = g_strdup (property->suffix);

		pos = anjuta_token_find_group_property_position (AMP_GROUP_NODE (node), property->token_type);
	}
	else
	{
		/* Target property */
		gchar *canon_name;
		
		canon_name = canonicalize_automake_variable (anjuta_project_node_get_name (ANJUTA_PROJECT_NODE (node)));
		name = g_strconcat (canon_name, property->suffix, NULL);
		g_free (canon_name);
	
		pos = anjuta_token_find_target_property_position (AMP_TARGET_NODE (node), property->token_type);
	}
	
	pos = anjuta_token_insert_token_list (FALSE, pos,
	    		property->token_type, NULL,
	    		ANJUTA_TOKEN_NAME, name,
	    		ANJUTA_TOKEN_SPACE, " ",
	    		ANJUTA_TOKEN_OPERATOR, "=",
	            ANJUTA_TOKEN_SPACE, " ",
	    		ANJUTA_TOKEN_LIST, NULL,
	            ANJUTA_TOKEN_SPACE, " ",
	    		NULL);
	
	g_free (name);

	return anjuta_token_last_item (pos);
}

gboolean amp_project_update_am_property (AmpProject *project, AnjutaProjectNode *node, AnjutaProjectProperty *property)
{
	AnjutaProjectNode *group;
	AnjutaToken *args;

	g_return_val_if_fail (property->native != NULL, FALSE);
	
	/* Find group  of the property */
	if (anjuta_project_node_get_node_type (node) == ANJUTA_PROJECT_GROUP)
	{
		group = node;
	}
	else
	{
		group = anjuta_project_node_parent (node);
	}

	if (((property->native->value == NULL) && ((property->value == NULL) || (*property->value == '\0'))) ||
	    (g_strcmp0 (property->native->value, property->value) == 0))
	{
		/* Remove property */
		args = amp_property_delete_token (project, ((AmpProperty *)property)->token);
		
		anjuta_project_node_remove_property (node, property);
	}
	else
	{
		GString *new_value;
		AnjutaToken *arg;
		const gchar *value;
		AnjutaTokenStyle *style;

		args = ((AmpProperty *)property)->token;
		
		/* Try to use the same style than the current target list */
		style = anjuta_token_style_new_from_base (project->am_space_list);
		anjuta_token_style_update (style, args);

		if (args== NULL)
		{
			args = amp_project_write_property_list (AMP_GROUP_NODE (group), node, (AmpProperty *)property->native);
			((AmpProperty *)property)->token = args;
		}

		switch (property->native->type)
		{
		case ANJUTA_PROJECT_PROPERTY_LIST:
			new_value = g_string_new (property->value);
			g_string_assign (new_value, "");
			value = property->value;

			for (arg = anjuta_token_first_word (args); arg != NULL;)
			{
				gchar *arg_value = anjuta_token_evaluate (arg);

				while (isspace (*value)) value++;

				if (*value == '\0')
				{
					AnjutaToken *next;
					
					next = anjuta_token_next_word (arg);
					anjuta_token_remove_word (arg);
					arg = next;
				}
				else
				{
					const gchar *end;
					gchar *name;
					
					for (end = value; !isspace (*end) && (*end != '\0'); end++);
					name = g_strndup (value, end - value);

					if (strcmp (arg_value, name) != 0)
					{
						/* New argument in property list */
						AnjutaToken *token;
						
						token = anjuta_token_new_string (ANJUTA_TOKEN_NAME | ANJUTA_TOKEN_ADDED, name);
						anjuta_token_insert_word_before (args, arg, token);
					}
					else
					{
						arg = anjuta_token_next_word (arg);
					}
					value = end;
					
					if (arg_value != NULL)
					{
						if (new_value->len != 0) g_string_append_c (new_value, ' ');
						g_string_append (new_value, name);
					}
				}
				g_free (arg_value);
			}

			while (*value != '\0')
			{
				AnjutaToken *token;
				const gchar *end;
				gchar *name;
				
				while (isspace (*value)) value++;
				if (*value == '\0') break;

				for (end = value; !isspace (*end) && (*end != '\0'); end++);

				name = g_strndup (value, end - value);
				token = anjuta_token_new_string (ANJUTA_TOKEN_NAME | ANJUTA_TOKEN_ADDED, name);

				anjuta_token_insert_word_before (args, NULL, token);
				
				if (new_value->len != 0) g_string_append_c (new_value, ' ');
				g_string_append (new_value, name);
				
				g_free (name);
				value = end;
			}

			anjuta_token_style_format (style, args);
			anjuta_token_style_free (style);

			if (property->value != property->native->value) g_free (property->value);
			property->value = g_string_free (new_value, FALSE);

			break;
		default:				
			break;
		}
	}

	if (args != NULL) amp_group_node_update_makefile (AMP_GROUP_NODE (group), args);
	
	return args != NULL ? TRUE : FALSE;
}
