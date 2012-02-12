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
#include "am-properties.h"
#include "am-scanner.h"

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-utils.h>

#include <string.h>
#include <ctype.h>

/* Types & Constants
  *---------------------------------------------------------------------------*/

const static gchar* AmpStandardDirectory[] = {"bindir", "sbindir", "libdir", "pkglibdir", "libexecdir", "pkglibexecdir", "datadir", "pkgdatadir", "mandir", "infodir", "docdir", NULL};

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

	group = AMP_GROUP_NODE (anjuta_project_node_parent_type (ANJUTA_PROJECT_NODE (target), ANJUTA_PROJECT_GROUP));

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
	parent = AMP_GROUP_NODE (anjuta_project_node_parent_type(ANJUTA_PROJECT_NODE (group), ANJUTA_PROJECT_GROUP));
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

	/* Get parent group */
	parent =  anjuta_project_node_parent_type (ANJUTA_PROJECT_NODE (group), ANJUTA_PROJECT_GROUP);
	if (parent == NULL) return FALSE;

	for (item = amp_group_node_get_token (group, AM_GROUP_TOKEN_SUBDIRS); item != NULL; item = g_list_next (item))
	{
		AnjutaToken *token = (AnjutaToken *)item->data;
		AnjutaTokenStyle *style;
		AnjutaToken *list;

		list = anjuta_token_list (token);

		/* Try to use the same style than the current target list */
		style = anjuta_token_style_new_from_base (project->am_space_list);
		anjuta_token_style_update (style, list);

		anjuta_token_remove_word (token);
		anjuta_token_style_format (style, list);
		anjuta_token_style_free (style);

		/* Remove whole variable if empty */
		if (anjuta_token_first_word (list) == NULL)
		{
			anjuta_token_remove_list (anjuta_token_list (list));
		}

		amp_group_node_update_makefile (AMP_GROUP_NODE (parent), list);
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
	AnjutaToken *pos = sibling;

	if (pos != NULL)
	{
		/* Find top level parent */
		do
		{
			AnjutaTokenType type = anjuta_token_get_type (pos);

			if ((type >= AM_TOKEN_FIRST_ORDERED_MACRO) && (type <= AM_TOKEN_LAST_ORDERED_MACRO)) break;
			pos = anjuta_token_list (pos);
		}
		while (pos != NULL);

		if (pos != NULL)
		{
			/* Add target just near sibling target */
			pos = anjuta_token_insert_token_list (after, pos,
				ANJUTA_TOKEN_EOL, "\n",
				NULL);
			pos = anjuta_token_insert_token_list (after, pos,
			    ANJUTA_TOKEN_EOL, "\n",
				NULL);
			amp_group_node_update_makefile (group, pos);
		}
	}

	if (pos == NULL)
	{
		/* Find ordered position in Makefile.am */
		pos = anjuta_token_find_group_property_position (group, type);
	}

	pos = anjuta_token_insert_token_list (after, pos,
	    		ANJUTA_TOKEN_LIST, NULL,
	    		type, name,
	    		ANJUTA_TOKEN_SPACE, " ",
	    		ANJUTA_TOKEN_OPERATOR, "=",
	    		ANJUTA_TOKEN_LIST, NULL,
	            ANJUTA_TOKEN_SPACE, " ",
	    		NULL);
	pos = anjuta_token_last_item (pos);
	amp_group_node_update_makefile (group, pos);

	/* Return list token */
	return pos;
}

static AnjutaToken *
amp_target_add_in_list (AmpProject *project, AnjutaToken *list, AnjutaProjectNode *target, gboolean after, AnjutaToken* sibling)
{
	AnjutaTokenStyle *style;
	AnjutaToken *token;
	AmpGroupNode *parent;

	g_return_val_if_fail (list != NULL, NULL);

	/* Get parent target */
	parent = AMP_GROUP_NODE (anjuta_project_node_parent_type (target, ANJUTA_PROJECT_GROUP));

	style = anjuta_token_style_new_from_base (project->am_space_list);
	anjuta_token_style_update (style, list);

	token = anjuta_token_new_string (ANJUTA_TOKEN_ARGUMENT | ANJUTA_TOKEN_ADDED, anjuta_project_node_get_name (target));
	if (after)
	{
		anjuta_token_insert_word_after (list, sibling, token);
	}
	else
	{
		anjuta_token_insert_word_before (list, sibling, token);
	}

	/* Try to use the same style than the current target list */
	anjuta_token_style_format (style, list);
	anjuta_token_style_free (style);

	amp_group_node_update_makefile (parent, token);

	amp_target_node_add_token (AMP_TARGET_NODE (target), ANJUTA_TOKEN_ARGUMENT, token);

	return token;
}


gboolean
amp_target_node_create_token (AmpProject  *project, AmpTargetNode *target, GError **error)
{
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
	parent = AMP_GROUP_NODE (anjuta_project_node_parent_type (ANJUTA_PROJECT_NODE (target), ANJUTA_PROJECT_GROUP));

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
	targetname = g_strconcat (info->install, "_", info->prefix, NULL);

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
			gchar *value = anjuta_token_evaluate (anjuta_token_first_word ((AnjutaToken *)last->data));

			if ((value != NULL) && (strcmp (targetname, value) == 0))
			{
				g_free (value);
				args = anjuta_token_last_item ((AnjutaToken *)last->data);
				break;
			}
			g_free (value);
		}
	}


	if (args == NULL)
	{
		args = amp_project_write_target (parent, info->token, targetname, FALSE, NULL);
	}
	g_free (targetname);

	switch (anjuta_project_node_get_full_type (ANJUTA_PROJECT_NODE (target)) & ANJUTA_PROJECT_ID_MASK)
	{
	case ANJUTA_PROJECT_SHAREDLIB:
	case ANJUTA_PROJECT_STATICLIB:
	case ANJUTA_PROJECT_LT_MODULE:
	case ANJUTA_PROJECT_PROGRAM:
		amp_target_add_in_list (project, args, ANJUTA_PROJECT_NODE (target), after, prev);
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
amp_target_node_delete_token (AmpProject  *project, AmpTargetNode *target, GList *list, GError **error)
{
	GList *item;
	GList *removed_dir = NULL;
	AmpGroupNode *parent;

	/* Get parent group */
	parent = AMP_GROUP_NODE (anjuta_project_node_parent_type (ANJUTA_PROJECT_NODE (target), ANJUTA_PROJECT_GROUP));

	/* Remove all associated token */
	for (item = list; item != NULL;	 item = g_list_next (item))
	{
		AnjutaToken *token = (AnjutaToken *)item->data;
		AnjutaTokenStyle *style;
		AnjutaToken *list;

		switch (anjuta_token_get_type (token))
		{
		case ANJUTA_TOKEN_ARGUMENT:

			list = anjuta_token_list (token);

			/* Try to use the same style than the current target list */
			style = anjuta_token_style_new_from_base (project->am_space_list);
			anjuta_token_style_update (style, list);

			anjuta_token_remove_word (token);
			anjuta_token_style_format (style, list);
			anjuta_token_style_free (style);

			/* Remove whole variable if empty */
			if (anjuta_token_first_word (list) == NULL)
			{
				AnjutaToken *variable = anjuta_token_list (list);
				gchar *value;
				gint flags;
				gchar *install = NULL;

				value = anjuta_token_evaluate (anjuta_token_first_word (variable));
				split_automake_variable (value, &flags, &install, NULL);

				if (install != NULL)
				{
					/* Mark all removed directory, normally only one */
					removed_dir = g_list_prepend (removed_dir, g_strdup (install));
				}
				g_free (value);
				anjuta_token_remove_list (variable);
			}

			amp_group_node_update_makefile (parent, list);

			break;
		case AM_TOKEN__SOURCES:
		case AM_TOKEN__DATA:
		case AM_TOKEN__HEADERS:
		case AM_TOKEN__LISP:
		case AM_TOKEN__MANS:
		case AM_TOKEN__PYTHON:
		case AM_TOKEN__JAVA:
		case AM_TOKEN__SCRIPTS:
		case AM_TOKEN__TEXINFOS:
        case AM_TOKEN_TARGET_LDFLAGS:
        case AM_TOKEN_TARGET_CPPFLAGS:
        case AM_TOKEN_TARGET_CFLAGS:
        case AM_TOKEN_TARGET_CXXFLAGS:
        case AM_TOKEN_TARGET_JAVACFLAGS:
        case AM_TOKEN_TARGET_VALAFLAGS:
        case AM_TOKEN_TARGET_FCFLAGS:
        case AM_TOKEN_TARGET_OBJCFLAGS:
        case AM_TOKEN_TARGET_LFLAGS:
        case AM_TOKEN_TARGET_YFLAGS:
        case AM_TOKEN_TARGET_DEPENDENCIES:
        case AM_TOKEN_TARGET_LIBADD:
        case AM_TOKEN_TARGET_LDADD:
			anjuta_token_remove_list (token);
			amp_group_node_update_makefile (parent, token);
			break;
		};
		amp_target_node_remove_token (target, token);
	}

	/* Check if we need to remove dir variable */
	for (item = removed_dir; item != NULL; item = g_list_next(item))
	{
		gchar* dir = (gchar *)item->data;
		GList *list;

		/* Check if dir is used in another target */
		for (list = amp_group_node_get_token (parent, AM_GROUP_TARGET); list != NULL; list = g_list_next (list))
		{
			AnjutaToken *target_list = (AnjutaToken *)list->data;
			gchar *value;
			gint flags;
			gchar *install = NULL;
			gboolean same;

			value = anjuta_token_evaluate (anjuta_token_first_word (target_list));
			/* value can be NULL if we have a list can has just been removed */
			if (value != NULL) split_automake_variable (value, &flags, &install, NULL);

			same = g_strcmp0 (install, dir) == 0;
			g_free (value);

			if (same)
			{
				/* directory use elsewhere */

				g_free (dir);
				dir = NULL;
				break;
			}
		}

		if (dir != NULL)
		{
			/* Directory is not used anymore, remove variable */
			gchar* install = g_strconcat (dir, "dir", NULL);

			for (list = anjuta_project_node_get_properties (ANJUTA_PROJECT_NODE(parent)); list != NULL; list = g_list_next (list))
			{
				AmpProperty *prop = (AmpProperty *)list->data;

				if ((((AmpPropertyInfo *)prop->base.info)->token_type == AM_TOKEN_DIR) &&
				    (g_strcmp0(prop->base.name, install) == 0))
				{
					AnjutaProjectProperty *new_prop;

					new_prop = amp_node_map_property_set (ANJUTA_PROJECT_NODE (parent), prop->base.info->id, prop->base.name, NULL);
					amp_project_update_am_property (project, ANJUTA_PROJECT_NODE (parent), new_prop);
				}
			}
			g_free (install);
			g_free (dir);
		}
	}
	g_list_free (removed_dir);


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
	target = AMP_TARGET_NODE (anjuta_project_node_parent_type (ANJUTA_PROJECT_NODE (source), ANJUTA_PROJECT_TARGET));
	if (target == NULL) return FALSE;

	group = AMP_GROUP_NODE (anjuta_project_node_parent_type (ANJUTA_PROJECT_NODE (target), ANJUTA_PROJECT_GROUP));
	relative_name = get_relative_path (anjuta_project_node_get_file (ANJUTA_PROJECT_NODE (group)), anjuta_project_node_get_file (ANJUTA_PROJECT_NODE (source)));

	/* Add in Makefile.am */
	/* Find a sibling if possible */
	after = TRUE;
	for (sibling = anjuta_project_node_prev_sibling (ANJUTA_PROJECT_NODE (source)); sibling != NULL; sibling = anjuta_project_node_prev_sibling (sibling))
	{
		if (anjuta_project_node_get_node_type (sibling) == ANJUTA_PROJECT_SOURCE)
		{
			break;
		}
		else if (anjuta_project_node_get_node_type (sibling) == ANJUTA_PROJECT_OBJECT)
		{
			sibling = anjuta_project_node_first_child (sibling);
			break;
		}
	}
	if (sibling == NULL)
	{
		after = FALSE;
		for (sibling = anjuta_project_node_next_sibling (ANJUTA_PROJECT_NODE (source)); sibling != NULL; sibling = anjuta_project_node_next_sibling (sibling))
		{
			if (anjuta_project_node_get_node_type (sibling) == ANJUTA_PROJECT_SOURCE)
			{
				break;
			}
			else if (anjuta_project_node_get_node_type (sibling) == ANJUTA_PROJECT_OBJECT)
			{
				sibling = anjuta_project_node_first_child (sibling);
				break;
 			}
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
			args = anjuta_token_last_item ((AnjutaToken *)last->data);
			break;
		}
		if (last == NULL)
		{
			for (last = amp_target_node_get_token (target, AM_TOKEN__DATA); last != NULL; last = g_list_next (last))
			{
				args = anjuta_token_last_item ((AnjutaToken *)last->data);
				break;
			}
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
		if (var == NULL)
			var = anjuta_token_find_target_property_position (target, AM_TOKEN__DATA);

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
	group = anjuta_project_node_parent_type (ANJUTA_PROJECT_NODE (source), ANJUTA_PROJECT_GROUP);
	if (group == NULL) return FALSE;

	token = amp_source_node_get_token (source);
	if (token != NULL)
	{
		AnjutaTokenStyle *style;
		AnjutaToken *list;

		list = anjuta_token_list (token);

		/* Try to use the same style than the current target list */
		style = anjuta_token_style_new_from_base (project->am_space_list);
		anjuta_token_style_update (style, list);

		anjuta_token_remove_word (token);
		anjuta_token_style_format (style, list);
		anjuta_token_style_free (style);

		/* Remove whole variable if empty */
		if (anjuta_token_first_word (list) == NULL)
		{
			anjuta_token_remove_list (anjuta_token_list (list));
		}

		amp_group_node_update_makefile (AMP_GROUP_NODE (group), list);
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
		anjuta_token_remove_list (anjuta_token_list (token));

		updated = TRUE;
	}

	return token;
}

static AnjutaToken *
amp_project_write_property_list (AmpGroupNode *group, AnjutaProjectNode *node, AmpPropertyInfo *info)
{
	AnjutaToken *pos;
	gchar *name;

	if (anjuta_project_node_get_node_type (node) == ANJUTA_PROJECT_GROUP)
	{
		/* Group property */
		name = g_strdup (info->suffix);

		pos = anjuta_token_find_group_property_position (AMP_GROUP_NODE (node), info->token_type);
	}
	else
	{
		/* Target property */
		gchar *canon_name;

		canon_name = canonicalize_automake_variable (anjuta_project_node_get_name (ANJUTA_PROJECT_NODE (node)));
		name = g_strconcat (canon_name, info->suffix, NULL);
		g_free (canon_name);

		pos = anjuta_token_find_target_property_position (AMP_TARGET_NODE (node), info->token_type);
	}

	pos = anjuta_token_insert_token_list (FALSE, pos,
	    		info->token_type, NULL,
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

static gint
compare_property_position (gconstpointer a, gconstpointer b)
{
	return ((const AmpPropertyInfo *)a)->position - ((const AmpPropertyInfo *)b)->position;
}

static AnjutaToken *
amp_property_rename_target (AmpProject *project, AnjutaProjectNode *node)
{
	AnjutaProjectNode *group;
	GList *infos;
	GList *item;
	GString *new_name;
	AmpNodeInfo *info;
	GList *list;
	AnjutaToken *update = NULL;
	AnjutaToken *existing_target_list;
	gboolean after;
	gchar *target_dir;

	g_return_val_if_fail (anjuta_project_node_get_node_type (node) == ANJUTA_PROJECT_TARGET, NULL);

	group = anjuta_project_node_parent_type (node, ANJUTA_PROJECT_GROUP);

	/* Find all program properties */
	infos = NULL;
	for (item = anjuta_project_node_get_properties_info (node); item != NULL; item = g_list_next (item))
	{
		AmpPropertyInfo *info = (AmpPropertyInfo *)item->data;

		if (info->token_type == AM_TOKEN__PROGRAMS)
		{
			infos = g_list_insert_sorted (infos, info, compare_property_position);
		}
	}

	/* Create new name */
	new_name = g_string_new (NULL);
	for (item = infos; item != NULL; item = g_list_next (item))
	{
		AmpPropertyInfo *info = (AmpPropertyInfo *)item->data;
		AmpProperty *prop;

		/* Check if property is enabled by another property */
		if (info->link != NULL)
		{
			AnjutaProjectProperty *en_prop;

			en_prop = anjuta_project_node_get_property (node, info->link->id);

			if ((en_prop->value != NULL) && (*en_prop->value == '1')) continue;
		}

		prop = (AmpProperty *)anjuta_project_node_get_property (node, info->base.id);
		if ((prop == (AmpProperty *)info->base.default_value) || (g_strcmp0 (prop->base.value, info->base.default_value->value) == 0))
		{
			/* Default value, add only string properties */
			if (info->base.type == ANJUTA_PROJECT_PROPERTY_STRING)
			{
				g_string_append (new_name, info->suffix);
				g_string_append_c (new_name, '_');
			}
		}
		else
		{
			switch (info->base.type)
			{
			case ANJUTA_PROJECT_PROPERTY_STRING:
				if ((info->flags & AM_PROPERTY_DIRECTORY) &&
				    (strlen (prop->base.value) > 4) &&
				    (strcmp (prop->base.value + strlen (prop->base.value) - 3, "dir") == 0))
				{
					/* Remove "dir" suffix */
					g_string_append_len (new_name, prop->base.value, strlen (prop->base.value) - 3);
				}
				else
				{
					g_string_append (new_name, prop->base.value);
				}
				g_string_append_c (new_name, '_');
				break;
			case ANJUTA_PROJECT_PROPERTY_BOOLEAN:
				if ((prop->base.value != NULL) && (g_strcmp0 (prop->base.value, info->base.default_value->value) != 0))
				{
					g_string_append (new_name, info->suffix);
				}
				break;
			default:
				break;
			}
		}
	}

	info = (AmpNodeInfo *)amp_project_get_type_info (project, anjuta_project_node_get_full_type (node));
	g_string_append (new_name, info->prefix);


    // Check if the target already exist.
	after = TRUE;
	for (item = amp_group_node_get_token (AMP_GROUP_NODE (group), AM_GROUP_TARGET); item != NULL; item = g_list_next (item))
	{
		existing_target_list = (AnjutaToken *)item->data;
		gchar *target_name = anjuta_token_evaluate (anjuta_token_first_word (existing_target_list));
		gboolean same;

		same = strcmp (target_name,  new_name->str) == 0;
		g_free (target_name);

		if (after)
		{
			GList *list;
			GList *item;

			list = amp_target_node_get_token (AMP_TARGET_NODE (node), ANJUTA_TOKEN_ARGUMENT);
			for (item = g_list_first (list); item != NULL; item = g_list_next (item))
			{
				AnjutaToken *arg = (AnjutaToken *)item->data;
				AnjutaToken *target_list;

				if (arg != NULL)
				{
					target_list = anjuta_token_list (arg);
					if (anjuta_token_list (target_list) == existing_target_list)
					{
						/* token in group_node are stored in reverse order */
						after = FALSE;
						break;
					}
				}
			}
		}

		if (same)
		{
			existing_target_list = anjuta_token_last_item (existing_target_list);
			break;
		}
		existing_target_list = NULL;
	}

	if (existing_target_list != NULL)
	{
		GList *token_list;

		/* Get old tokens */
		token_list = g_list_copy (amp_target_node_get_token (AMP_TARGET_NODE (node), ANJUTA_TOKEN_ARGUMENT));

		/* Add target in already existing list */
		amp_target_add_in_list (project, existing_target_list, node, after, NULL);

		/* Remove old token */
		amp_target_node_delete_token (project, AMP_TARGET_NODE (node), token_list, NULL);
		g_list_free (token_list);
	}
	else
	{
		list = amp_target_node_get_token (AMP_TARGET_NODE (node), ANJUTA_TOKEN_ARGUMENT);
		for (item = g_list_first (list); item != NULL; item = g_list_next (item))
		{
			AnjutaToken *arg = (AnjutaToken *)item->data;
			AnjutaToken *target_list;

			if (arg == NULL) continue;

			target_list = anjuta_token_list (arg);

			if (anjuta_token_nth_word (target_list, 1) == NULL)
			{
				/* Only one target in list, just replace list name */
				AnjutaToken *target_variable = anjuta_token_list (target_list);

				if (target_variable != NULL)
				{
					AnjutaToken *old_token;

					old_token = anjuta_token_first_word (target_variable);
					if (old_token != NULL)
					{
						AnjutaToken *token;

						token = anjuta_token_new_string (ANJUTA_TOKEN_ADDED, new_name->str);
						update = anjuta_token_insert_word_after (target_variable, old_token, token);
						anjuta_token_remove_word (old_token);
						update = target_variable;
					}
				}
			}
			else
			{
				gchar *old_target;
				AmpNodeInfo *info;
				gboolean after = TRUE;
				AnjutaToken *sibling = NULL;
				AnjutaTokenStyle *style;
				AnjutaToken *token;

				old_target = anjuta_token_evaluate (arg);

				/* Find sibling target */
				if (anjuta_token_first_word (target_list) == arg)
				{
					sibling = anjuta_token_next_word (arg);
					after = FALSE;
				}
				else
				{
					for (sibling = anjuta_token_first_word (target_list); sibling != NULL; sibling = anjuta_token_next_word (sibling))
					{
						if (anjuta_token_next_word (sibling) == arg) break;
					}
					after = TRUE;
				}

				/* More than one target, remove target in list */
				arg = anjuta_token_remove_word (arg);
				if (arg != NULL) amp_group_node_update_makefile (AMP_GROUP_NODE (group), arg);


				/* Add target in new list */
				style = anjuta_token_style_new_from_base (project->am_space_list);
				anjuta_token_style_update (style, target_list);

				info = (AmpNodeInfo *)amp_project_get_type_info (project, anjuta_project_node_get_full_type (node));
				target_list = amp_project_write_target (AMP_GROUP_NODE (group), info->token, new_name->str, after, sibling);

				token = anjuta_token_new_string (ANJUTA_TOKEN_ARGUMENT | ANJUTA_TOKEN_ADDED, old_target);
				anjuta_token_insert_word_after (target_list, NULL, token);

				/* Try to use the same style than the current target list */
				anjuta_token_style_format (style, target_list);
				anjuta_token_style_free (style);

				amp_group_node_update_makefile (AMP_GROUP_NODE (group), token);
				amp_target_node_add_token (AMP_TARGET_NODE (node), ANJUTA_TOKEN_ARGUMENT, token);

				g_free (old_target);

				update = anjuta_token_list (target_list);
			}
		}
	}


	/* Add directory variable if needed */
	target_dir = NULL;
	for (item = anjuta_project_node_get_properties (node); item != NULL; item = g_list_next (item))
	{
		AmpProperty *prop = (AmpProperty *)item->data;

		if ((((AmpPropertyInfo *)prop->base.info)->token_type == AM_TOKEN__PROGRAMS) && (((AmpPropertyInfo *)prop->base.info)->flags & AM_PROPERTY_DIRECTORY))
		{
			target_dir = prop->base.value;
			if ((strlen (target_dir) <= 3) || (strcmp (target_dir + strlen(target_dir) - 3, "dir") != 0))
			{
				target_dir = g_strconcat (target_dir, "dir", NULL);
				g_free (prop->base.value);
				prop->base.value = target_dir;
			}
			break;
		}
	}

	/* If it is a standard directory do not add a variable*/
	if (target_dir != NULL)
	{
		const gchar **std_dir;

		for (std_dir = AmpStandardDirectory; *std_dir != NULL; std_dir++)
		{
			if (strcmp(*std_dir, target_dir) == 0)
			{
				target_dir = NULL;
				break;
			}
		}
	}

	if (target_dir != NULL)
	{
		for (item = anjuta_project_node_get_properties (group); item != NULL; item = g_list_next (item))
		{
			AmpProperty *prop = (AmpProperty *)item->data;

			if ((((AmpPropertyInfo *)prop->base.info)->token_type == AM_TOKEN_DIR) && (g_strcmp0 (prop->base.name, target_dir) == 0))
			{
				/* Find already existing directory variable */
				target_dir = NULL;
				break;
			}
		}
	}

	if ((update != NULL) && (target_dir != NULL))
	{
		update = anjuta_token_insert_token_list (FALSE, update,
					AM_TOKEN_DIR, NULL,
    				ANJUTA_TOKEN_NAME, target_dir,
    				ANJUTA_TOKEN_SPACE, " ",
    				ANJUTA_TOKEN_OPERATOR, "=",
        			ANJUTA_TOKEN_SPACE, " ",
    				ANJUTA_TOKEN_LIST, NULL,
        			ANJUTA_TOKEN_SPACE, " ",
					ANJUTA_TOKEN_EOL, "\n",
    				NULL);
	}

	g_string_free (new_name, TRUE);


	return update;
}

gboolean amp_project_update_am_property (AmpProject *project, AnjutaProjectNode *node, AnjutaProjectProperty *property)
{
	AnjutaProjectNode *group;
	AnjutaToken *args;

	/* Find group  of the property */
	if (anjuta_project_node_get_node_type (node) == ANJUTA_PROJECT_GROUP)
	{
		group = node;
	}
	else
	{
		group = anjuta_project_node_parent_type (node, ANJUTA_PROJECT_GROUP);
	}

	if (property->value == NULL)
	{
		/* Remove property */
		if (((AmpPropertyInfo *)property->info)->token_type == AM_TOKEN__PROGRAMS)
		{
			/* Properties added in the target name */
			args = amp_property_rename_target (project, node);
		}
		else
		{
			/* Other properties having their own variable */
			args = amp_property_delete_token (project, ((AmpProperty *)property)->token);
		}

		anjuta_project_node_remove_property (node, property);
	}
	else
	{
		if (((AmpPropertyInfo *)property->info)->token_type == AM_TOKEN__PROGRAMS)
		{
			/* Properties added in the target name */
			args = amp_property_rename_target (project, node);
		}
		else
		{
			/* Other properties having their own variable */
			GString *new_value;
			AnjutaToken *arg;
			AnjutaToken *token;
			const gchar *value;
			AnjutaTokenStyle *style;

			args = ((AmpProperty *)property)->token;

			/* Try to use the same style than the current target list */
			style = anjuta_token_style_new_from_base (project->am_space_list);
			anjuta_token_style_update (style, args);

			if (args == NULL)
			{
				args = amp_project_write_property_list (AMP_GROUP_NODE (group), node, (AmpPropertyInfo *)property->info);
				((AmpProperty *)property)->token = args;
			}

			switch (property->info->type)
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

				g_free (property->value);
				property->value = g_string_free (new_value, FALSE);

				break;
			case ANJUTA_PROJECT_PROPERTY_MAP:

				token =  anjuta_token_new_string (ANJUTA_TOKEN_NAME | ANJUTA_TOKEN_ADDED, property->value);
				anjuta_token_insert_word_after (args, NULL, token);

				for (token = anjuta_token_next_word (token); token != NULL; token = anjuta_token_next_word (token))
				{
					anjuta_token_remove_word (token);
				}
				break;
			default:
				break;
			}
		}
	}

	if (args != NULL) amp_group_node_update_makefile (AMP_GROUP_NODE (group), args);

	return args != NULL ? TRUE : FALSE;
}
