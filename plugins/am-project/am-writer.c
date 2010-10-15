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
#include "am-scanner.h"

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

static AnjutaToken *
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
	
	anjuta_token_file_update (project->configure_file, list);
	
	return token;
}


/* Target objects
 *---------------------------------------------------------------------------*/

gboolean 
amp_group_create_token (AmpProject  *project, AnjutaAmGroupNode *group, GError **error)
{
	AnjutaAmGroupNode *last;
	GFile *directory;
	GFile *makefile;
	AnjutaToken *list;
	gchar *basename;
	AnjutaTokenFile* tfile;
	AnjutaAmGroupNode *sibling;
	AnjutaAmGroupNode *parent;
	gboolean after;
	gchar *name;
	
	/* Get parent target */
	parent = ANJUTA_AM_GROUP_NODE (anjuta_project_node_parent(ANJUTA_PROJECT_NODE (group)));
	name = anjuta_project_node_get_name (ANJUTA_PROJECT_NODE (group));
	directory = g_file_get_child (anjuta_project_node_get_file (ANJUTA_PROJECT_NODE (parent)), name);

	/* Find a sibling if possible */
	if (anjuta_project_node_prev_sibling (ANJUTA_PROJECT_NODE (group)) != NULL)
	{
		sibling = ANJUTA_AM_GROUP_NODE (anjuta_project_node_prev_sibling (ANJUTA_PROJECT_NODE (group)));
		after = TRUE;
	}
	else if (anjuta_project_node_next_sibling (ANJUTA_PROJECT_NODE (group)) != NULL)
	{
		sibling = ANJUTA_AM_GROUP_NODE (anjuta_project_node_next_sibling (ANJUTA_PROJECT_NODE (group)));
		after = FALSE;
	}
	else
	{
		sibling = NULL;
		after = TRUE;
	}
	
	/* Create directory */
	g_file_make_directory (directory, NULL, NULL);

	/* Create Makefile.am */
	basename = amp_group_get_makefile_name  (parent);
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
	tfile = amp_group_set_makefile (group, makefile, G_OBJECT (project));
	amp_project_add_file (project, makefile, tfile);

	if (sibling == NULL)
	{
		/* Find a sibling before */
		for (last = anjuta_project_node_prev_sibling (group); (last != NULL) && (anjuta_project_node_get_node_type (last) != ANJUTA_PROJECT_GROUP); last = anjuta_project_node_prev_sibling (last));
		if (last != NULL)
		{
			sibling = last;
			after = TRUE;
		}
		else
		{
			/* Find a sibling after */
			for (last = anjuta_project_node_next_sibling (group); (last != NULL) && (anjuta_project_node_get_node_type (last) != ANJUTA_PROJECT_GROUP); last = anjuta_project_node_next_sibling (last));
			if (last != NULL)
			{
				sibling = last;
				after = FALSE;
			}
		}
	}
	
	/* Add in configure */
	list = NULL;
	if (sibling) list = amp_group_get_first_token (sibling, AM_GROUP_TOKEN_CONFIGURE);
	if (list == NULL) list= amp_group_get_first_token (parent, AM_GROUP_TOKEN_CONFIGURE);
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
			prev = amp_group_get_first_token (sibling, AM_GROUP_TOKEN_CONFIGURE);
			/*if ((prev != NULL) && after)
			{
				prev = anjuta_token_next_word (prev);
			}*/
		}
		//prev_token = (AnjutaToken *)token_list->data;

		relative_make = g_file_get_relative_path (anjuta_project_node_get_file (project->root), makefile);
		ext = relative_make + strlen (relative_make) - 3;
		if (strcmp (ext, ".am") == 0)
		{
			*ext = '\0';
		}
		token = amp_project_write_config_file (project, list, after, prev, relative_make);
		amp_group_add_token (group, token, AM_GROUP_TOKEN_CONFIGURE);
		g_free (relative_make);
	}

	/* Add in Makefile.am */
	if (sibling == NULL)
	{
		AnjutaToken *pos;
		AnjutaToken *makefile;
		static gint eol_type[] = {ANJUTA_TOKEN_EOL, ANJUTA_TOKEN_SPACE, ANJUTA_TOKEN_COMMENT, 0};
	
		makefile = amp_group_get_makefile_token (group);
		pos = anjuta_token_find_type (makefile, ANJUTA_TOKEN_SEARCH_NOT, eol_type);
		if (pos == NULL)
		{
			pos = anjuta_token_prepend_child (makefile, anjuta_token_new_static (ANJUTA_TOKEN_SPACE, "\n"));
		}

		list = anjuta_token_insert_token_list (FALSE, pos,
		                                       ANJUTA_TOKEN_EOL, "\n",
		                                       NULL);
		amp_group_update_makefile (parent, list);	
		list = anjuta_token_insert_token_list (FALSE, pos,
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
		
		prev = amp_group_get_first_token (sibling, AM_GROUP_TOKEN_SUBDIRS);
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
			prev = amp_group_get_first_token (sibling, AM_GROUP_TOKEN_SUBDIRS);
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
		
		amp_group_update_makefile (parent, token);
		
		amp_group_add_token (group, token, AM_GROUP_TOKEN_SUBDIRS);
	}
	g_free (name);

	return TRUE;
}

gboolean 
amp_group_delete_token (AmpProject  *project, AnjutaAmGroupNode *group, GError **error)
{
	GList *item;
	AnjutaProjectNode *parent;
	AnjutaProjectNode *root;

	/* Get parent target */
	parent =  anjuta_project_node_parent (ANJUTA_PROJECT_NODE (group));
	if (anjuta_project_node_get_node_type  (parent) != ANJUTA_PROJECT_GROUP) return FALSE;

	for (item = amp_group_get_token (group, AM_GROUP_TOKEN_SUBDIRS); item != NULL; item = g_list_next (item))
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

		amp_group_update_makefile (ANJUTA_AM_GROUP_NODE (parent), args);
	}

	/* Remove from configure file */
	root = anjuta_project_node_root (ANJUTA_PROJECT_NODE (group));

	for (item = amp_group_get_token (group, AM_GROUP_TOKEN_CONFIGURE); item != NULL; item = g_list_next (item))
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

		amp_root_update_configure (ANJUTA_AM_ROOT_NODE (root), args);
	}	
	
	return TRUE;
}


/* Target objects
 *---------------------------------------------------------------------------*/

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
		last = amp_target_get_token (sibling, ANJUTA_TOKEN_ARGUMENT);

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
		
		amp_target_add_token (target, ANJUTA_TOKEN_ARGUMENT, token);
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

	for (item = amp_target_get_token (target, ANJUTA_TOKEN_ARGUMENT); item != NULL; item = g_list_next (item))
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

static AnjutaToken *
amp_project_write_source_list (AnjutaAmGroupNode *group, const gchar *name, gboolean after, AnjutaToken* sibling)
{
	AnjutaToken *pos;
	AnjutaToken *token;
	AnjutaToken *makefile;
	static gint eol_type[] = {ANJUTA_TOKEN_EOL, 0};
	
	if (sibling == NULL)
	{
		makefile = amp_group_get_makefile_token (group);
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
			amp_group_update_makefile (group, pos);
		}
	}
	
	pos = anjuta_token_insert_token_list (after, pos,
		    ANJUTA_TOKEN_EOL, "\n",
		    NULL);
	amp_group_update_makefile (group, pos);
	
	token = anjuta_token_insert_token_list (after, pos,
	    		ANJUTA_TOKEN_LIST, NULL,
	    		ANJUTA_TOKEN_NAME, name,
	    		ANJUTA_TOKEN_SPACE, " ",
	    		ANJUTA_TOKEN_OPERATOR, "=",
	    		ANJUTA_TOKEN_LIST, NULL,
	            ANJUTA_TOKEN_SPACE, " ",
	    		NULL);

	return anjuta_token_last_item (token);
}


/* Source objects
 *---------------------------------------------------------------------------*/

gboolean 
amp_source_create_token (AmpProject  *project, AnjutaAmSourceNode *source, GError **error)
{
	AnjutaAmGroupNode *group;
	AnjutaAmTargetNode *target;
	gboolean after;
	AnjutaToken *token;
	AnjutaToken *prev;
	AnjutaToken *args;
	gchar *relative_name;

	/* Get parent target */
	target = ANJUTA_AM_TARGET_NODE (anjuta_project_node_parent (ANJUTA_PROJECT_NODE (source)));
	if ((target == NULL) || (anjuta_project_node_get_node_type (target) != ANJUTA_PROJECT_TARGET)) return FALSE;
	
	group = ANJUTA_AM_GROUP_NODE (anjuta_project_node_parent (ANJUTA_PROJECT_NODE (target)));
	relative_name = g_file_get_relative_path (anjuta_project_node_get_file (group), anjuta_project_node_get_file (source));

	/* Add in Makefile.am */
	/* Find a sibling if possible */
	if (source->base.prev != NULL)
	{
		prev =  ANJUTA_AM_SOURCE_NODE (source->base.prev)->token;
		after = TRUE;
		args = anjuta_token_list (prev);
	}
	else if (source->base.next != NULL)
	{
		prev = ANJUTA_AM_SOURCE_NODE (source->base.next)->token;
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
		GList *last;
		for (last = amp_target_get_token (target, AM_TOKEN__SOURCES); last != NULL; last = g_list_next (last))
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
		GList *list;
		
		canon_name = canonicalize_automake_variable (ANJUTA_AM_TARGET_NODE (target)->base.name);
		target_var = g_strconcat (canon_name,  "_SOURCES", NULL);

		/* Search where the target is declared */
		var = NULL;
		list = amp_target_get_token (target, ANJUTA_TOKEN_ARGUMENT);
		if (list != NULL)
		{
			var = (AnjutaToken *)list->data;
			if (var != NULL)
			{
				var = anjuta_token_list (var);
				if (var != NULL)
				{
					var = anjuta_token_list (var);
				}
			}
		}
		
		args = amp_project_write_source_list (group, target_var, after, var);
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
		
		amp_group_update_makefile (group, token);
		
		amp_source_add_token (source, token);
	}

	return TRUE;
}

gboolean 
amp_source_delete_token (AmpProject  *project, AnjutaAmSourceNode *source, GError **error)
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

	token = amp_source_get_token (source);
	if (token != NULL)
	{
		AnjutaToken *args;
		AnjutaTokenStyle *style;

		args = anjuta_token_list (token);

		/* Try to use the same style than the current target list */
		style = anjuta_token_style_new_from_base (project->am_space_list);
		anjuta_token_style_update (style, args);
		
		anjuta_token_remove_word (token);
		
		anjuta_token_style_format (style, args);
		anjuta_token_style_free (style);

		amp_group_update_makefile (ANJUTA_AM_GROUP_NODE (group), args);
	}

	return TRUE;
}

