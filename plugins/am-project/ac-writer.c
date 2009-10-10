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

#include "am-project-private.h"

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-utils.h>

/* Types
  *---------------------------------------------------------------------------*/


/* Helper functions
 *---------------------------------------------------------------------------*/

/* Private functions
 *---------------------------------------------------------------------------*/

static gboolean
remove_list_item (AnjutaToken *token, AnjutaTokenStyle *user_style)
{
	AnjutaTokenStyle *style;
	AnjutaToken *space;

	DEBUG_PRINT ("remove list item");

	style = user_style != NULL ? user_style : anjuta_token_style_new (NULL," ","\\n",NULL,0);
	anjuta_token_style_update (style, anjuta_token_parent (token));
	
	anjuta_token_remove (token);
	space = anjuta_token_next_sibling (token);
	if (space && (anjuta_token_get_type (space) == ANJUTA_TOKEN_SPACE) && (anjuta_token_next (space) != NULL))
	{
		/* Remove following space */
		anjuta_token_remove (space);
	}
	else
	{
		space = anjuta_token_previous_sibling (token);
		if (space && (anjuta_token_get_type (space) == ANJUTA_TOKEN_SPACE) && (anjuta_token_previous (space) != NULL))
		{
			anjuta_token_remove (space);
		}
	}
	
	anjuta_token_style_format (style, anjuta_token_parent (token));
	if (user_style == NULL) anjuta_token_style_free (style);
	
	return TRUE;
}

static gboolean
add_list_item (AnjutaToken *list, AnjutaToken *token, AnjutaTokenStyle *user_style)
{
	AnjutaTokenStyle *style;
	AnjutaToken *space;

	style = user_style != NULL ? user_style : anjuta_token_style_new (NULL," ","\\n",NULL,0);
	anjuta_token_style_update (style, anjuta_token_parent (list));
	
	space = anjuta_token_new_static (ANJUTA_TOKEN_SPACE | ANJUTA_TOKEN_ADDED, " ");
	space = anjuta_token_insert_after (list, space);
	anjuta_token_insert_after (space, token);

	anjuta_token_style_format (style, anjuta_token_parent (list));
	if (user_style == NULL) anjuta_token_style_free (style);
	
	return TRUE;
}

/* Public functions
 *---------------------------------------------------------------------------*/

gboolean
amp_project_update_property (AmpProject *project, AmpPropertyType type)
{
	AnjutaToken *token;
	guint pos;
	const gchar *value;
	
	if (project->property == NULL)
	{
		return FALSE;
	}
	gchar *name;
	gchar *version;
	gchar *bug_report;
	gchar *tarname;
	gchar *url;

	switch (type)
	{
		case AMP_PROPERTY_NAME:
			pos = 0;
			value = project->property->name;
			break;
		case AMP_PROPERTY_VERSION:
			pos = 1;
			value = project->property->version;
			break;
		case AMP_PROPERTY_BUG_REPORT:
			pos = 2;
			value = project->property->bug_report;
			break;
		case AMP_PROPERTY_TARNAME:
			pos = 3;
			value = project->property->tarname;
			break;
		case AMP_PROPERTY_URL:
			pos = 4;
			value = project->property->url;
			break;
	}
	
	token = anjuta_token_new_string (ANJUTA_TOKEN_NAME | ANJUTA_TOKEN_ADDED, value);
	anjuta_token_list_replace_nth (project->property->ac_init, pos, token);
	anjuta_token_style_format (project->arg_list, project->property->ac_init);
	
	return TRUE;
}

