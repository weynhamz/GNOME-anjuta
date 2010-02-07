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

#if 0
gboolean
amp_project_update_property (AmpProject *project, AmpPropertyType type)
{
	AnjutaToken *token;
	AnjutaToken *arg;
	guint pos;
	const gchar *value;

	g_return_val_if_fail (project->property != NULL, FALSE);

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
	
	if (project->property->ac_init == NULL)
	{
		gint types[] = {AC_TOKEN_AC_PREREQ, 0};
		AnjutaToken *group;

		token = find_tokens (project->configure_token, types);
		if (token == NULL)
		{
			token = skip_comment (project->configure_token);
			if (token == NULL)
			{
				token = anjuta_token_append_child (project->configure_token, anjuta_token_new_string (COMMENT | ANJUTA_TOKEN_ADDED, "#"));
				token = anjuta_token_insert_after (token, anjuta_token_new_string (SPACE | ANJUTA_TOKEN_ADDED, " Created by Anjuta project manager"));
				token = anjuta_token_insert_after (token, anjuta_token_new_string (EOL | ANJUTA_TOKEN_ADDED, "\n"));
				token = anjuta_token_insert_after (token, anjuta_token_new_string (EOL | ANJUTA_TOKEN_ADDED, "\n"));
			}
		}
		
		token = anjuta_token_insert_before (token, anjuta_token_new_string (AC_TOKEN_AC_INIT | ANJUTA_TOKEN_ADDED, "AC_INIT("));
		project->property->ac_init = token;
		group = anjuta_token_insert_after (token, anjuta_token_new_static (ANJUTA_TOKEN_LIST | ANJUTA_TOKEN_ADDED, NULL));
		project->property->args = group;
		token = anjuta_token_insert_after (group, anjuta_token_new_static (ANJUTA_TOKEN_LAST | ANJUTA_TOKEN_ADDED, NULL));
		anjuta_token_merge (group, token);
		anjuta_token_insert_after (token, anjuta_token_new_string (EOL | ANJUTA_TOKEN_ADDED, "\n"));
		fprintf(stdout, "whole file\n");
		anjuta_token_dump (project->configure_token);
	}
	fprintf(stdout, "ac_init before replace\n");
	anjuta_token_dump (project->property->args);
	token = anjuta_token_new_string (ANJUTA_TOKEN_NAME | ANJUTA_TOKEN_ADDED, value);
	arg = anjuta_token_insert_before (token, anjuta_token_new_static (ANJUTA_TOKEN_ITEM | ANJUTA_TOKEN_ADDED, NULL));
	anjuta_token_merge (arg, token);
	anjuta_token_replace_nth_word (project->property->args, pos, arg);
	fprintf(stdout, "ac_init after replace\n");
	anjuta_token_dump (project->property->args);
	fprintf(stdout, "ac_init after replace link\n");
	anjuta_token_dump_link (project->property->args);
	anjuta_token_style_format (project->arg_list, project->property->args);
	fprintf(stdout, "ac_init after update link\n");
	anjuta_token_dump (project->property->args);
	anjuta_token_file_update (project->configure_file, token);
	
	return TRUE;
}
#endif
