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

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-utils.h>

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

AnjutaToken *
amp_project_write_subdirs_list (AmpGroup *project)
{
	AnjutaToken *pos;
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
	fprintf (stdout, "Dump config list:\n");
	anjuta_token_dump (list);
	if (after)
	{
		anjuta_token_insert_word_after (list, sibling, token);
	}
	else
	{
		anjuta_token_insert_word_before (list, sibling, token);
	}
	fprintf (stdout, "Dump config list after insertion:\n");
	anjuta_token_dump (list);
	
	anjuta_token_style_format (project->ac_space_list, list);
	
	fprintf (stdout, "Dump config list after format:\n");
	anjuta_token_dump (list);
	
	anjuta_token_file_update (project->configure_file, list);
	
	return token;
}

AnjutaToken *
amp_project_write_target (AnjutaToken *makefile, gint type, const gchar *name, gboolean after, AnjutaToken* sibling)
{
	AnjutaToken *pos;
	AnjutaToken *token;
	
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

	token = anjuta_token_insert_token_list (after, pos,
	    		ANJUTA_TOKEN_LIST, NULL,
	    		type, name,
	    		ANJUTA_TOKEN_SPACE, " ",
	    		ANJUTA_TOKEN_OPERATOR, "=",
	    		ANJUTA_TOKEN_LIST, NULL,
	    		ANJUTA_TOKEN_START, NULL,
	    		NULL);

	return anjuta_token_last_item (token);
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
