/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* am-properties.c
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

#include "am-properties.h"

#include "am-project-private.h"

#include "ac-scanner.h"
#include "am-scanner.h"

#include <glib/gi18n.h>


/* Types
  *---------------------------------------------------------------------------*/

/* Constants
  *---------------------------------------------------------------------------*/

static AmpPropertyInfo AmpProjectProperties[] = {
	{{N_("Name:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AC_TOKEN_AC_INIT, 0, NULL},
	{{N_("Version:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AC_TOKEN_AC_INIT, 1, NULL},
	{{N_("Bug report URL:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AC_TOKEN_AC_INIT, 2, NULL},
	{{N_("Package name:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AC_TOKEN_AC_INIT, 3, NULL},
	{{N_("URL:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AC_TOKEN_AC_INIT, 4, NULL},
	{{NULL, ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AC_TOKEN_AC_INIT, 5, NULL}};

static GList* AmpProjectPropertyList = NULL;


static AmpPropertyInfo AmpGroupProperties[] = {
	{{N_("Linker flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AM_TOKEN__LDFLAGS, 0, NULL},
	{{N_("C preprocessor flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AM_TOKEN__CPPFLAGS, 0, NULL},
	{{N_("C compiler flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AM_TOKEN__CFLAGS, 0, NULL},
	{{N_("C++ compiler flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AM_TOKEN__CXXFLAGS, 0, NULL},
	{{N_("Java Compiler flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AM_TOKEN__JAVACFLAGS, 0, NULL},
	{{N_("Fortan compiler flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AM_TOKEN__FCFLAGS, 0, NULL},
	{{N_("Objective C compiler flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AM_TOKEN__OBJCFLAGS, 0, NULL},
	{{N_("Lex/Flex flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AM_TOKEN__LFLAGS, 0, NULL},
	{{N_("Yacc/Bison flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL},AM_TOKEN__YFLAGS, 0, NULL},
	{{N_("Install directories:"), ANJUTA_PROJECT_PROPERTY_MAP, NULL, NULL}, AM_TOKEN_DIR, 0, NULL},
	{{NULL, ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, 0, 0, NULL}};

static GList* AmpGroupPropertyList = NULL;


static AmpPropertyInfo AmpTargetProperties[] = {
	{{N_("Do not install:"), ANJUTA_PROJECT_PROPERTY_BOOLEAN, NULL, NULL}, AM_TOKEN__PROGRAMS, 3, NULL},
	{{N_("Installation directory:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AM_TOKEN__PROGRAMS, 6, NULL},
	{{N_("Linker flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AM_TOKEN_TARGET_LDFLAGS, 0, NULL},
	{{N_("C preprocessor flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AM_TOKEN_TARGET_CPPFLAGS, 0, NULL},
	{{N_("C compiler flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AM_TOKEN_TARGET_CFLAGS, 0, NULL},
	{{N_("C++ compiler flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AM_TOKEN_TARGET_CXXFLAGS, 0, NULL},
	{{N_("Java Compiler flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AM_TOKEN_TARGET_JAVACFLAGS, 0, NULL},
	{{N_("Fortan compiler flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AM_TOKEN_TARGET_FCFLAGS, 0, NULL},
	{{N_("Objective C compiler flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AM_TOKEN_TARGET_OBJCFLAGS, 0, NULL},
	{{N_("Lex/Flex flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AM_TOKEN_TARGET_LFLAGS, 0, NULL},
	{{N_("Yacc/Bison flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AM_TOKEN_TARGET_YFLAGS, 0, NULL},
	{{N_("Additional dependencies:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AM_TOKEN_TARGET_DEPENDENCIES, 0, NULL},
	{{N_("Include in distribution:"), ANJUTA_PROJECT_PROPERTY_BOOLEAN, NULL, NULL}, AM_TOKEN__PROGRAMS, 2, NULL},
	{{N_("Build for check only:"), ANJUTA_PROJECT_PROPERTY_BOOLEAN, NULL, NULL}, AM_TOKEN__PROGRAMS, 4, NULL},
	{{N_("Do not use prefix:"), ANJUTA_PROJECT_PROPERTY_BOOLEAN, NULL, NULL}, AM_TOKEN__PROGRAMS, 1, NULL},
	{{N_("Keep target path:"), ANJUTA_PROJECT_PROPERTY_BOOLEAN, NULL, NULL}, AM_TOKEN__PROGRAMS, 0, NULL},
	{{NULL, ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, 0, 0, NULL}};

static GList* AmpTargetPropertyList = NULL;

static AmpPropertyInfo AmpManTargetProperties[] = {
	{{N_("Additional dependencies:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, 0, 0, NULL},
	{{N_("Do not use prefix:"), ANJUTA_PROJECT_PROPERTY_BOOLEAN, NULL, NULL}, AM_TOKEN__PROGRAMS, 1, NULL},
	{{N_("Manual section:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AM_TOKEN__PROGRAMS, 5, NULL},
	{{NULL, ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, 0, 0, NULL}};

static GList* AmpManTargetPropertyList = NULL;

/* Helper functions
 *---------------------------------------------------------------------------*/

/* Private functions
 *---------------------------------------------------------------------------*/

static GList *
amp_create_property_list (GList **list, AmpPropertyInfo *info)
{
	if (*list == NULL)
	{
		AmpPropertyInfo *prop;

		for (prop = info; prop->base.name != NULL; prop++)
		{
			*list = g_list_prepend (*list, prop);
		}
		*list = g_list_reverse (*list);
	}

	return *list;
}

/* Public functions
 *---------------------------------------------------------------------------*/


/* Properties objects
 *---------------------------------------------------------------------------*/

AnjutaProjectPropertyInfo *
amp_property_new (AnjutaTokenType type, gint position, const gchar *value, AnjutaToken *token)
{
	AmpPropertyInfo *prop;

	prop = g_slice_new0(AmpPropertyInfo);
	prop->base.value = g_strdup (value);
	prop->token = token;
	prop->token_type = type;
	prop->position = position;

	return (AnjutaProjectPropertyInfo *)prop;
}

void
amp_property_free (AnjutaProjectPropertyInfo *prop)
{
	if (prop->override != NULL)
	{
		if ((prop->value != NULL) && (prop->value != ((AmpPropertyInfo *)(prop->override->data))->base.value))
		{
			g_free (prop->value);
		}
		g_slice_free (AmpPropertyInfo, (AmpPropertyInfo *)prop);
	}
}

/* Set property list
 *---------------------------------------------------------------------------*/

gboolean
amp_node_property_set (AnjutaProjectNode *node, gint token_type, gint position, const gchar *value, AnjutaToken *token)
{
	AnjutaProjectPropertyList **properties = &(ANJUTA_PROJECT_NODE_DATA(node)->properties);
	AnjutaProjectPropertyItem *list;
	gboolean set = FALSE;
	
	for (list = anjuta_project_property_first (*properties); list != NULL; list = anjuta_project_property_next (list))
	{
		AmpPropertyInfo *info = (AmpPropertyInfo *)anjuta_project_property_get_info (list);

		if ((info->token_type == token_type) && (info->position == position))
		{
			AnjutaProjectPropertyInfo *prop;

			prop = anjuta_project_property_lookup (*properties, list);
			if (prop == NULL)
			{
				prop = (AnjutaProjectPropertyInfo *)amp_property_new (token_type, position, NULL, token);
				
				*properties = anjuta_project_property_insert (*properties, list, prop);
			}
	
			if ((prop->value != NULL) && (prop->value != info->base.value)) g_free (prop->value);
			prop->value = g_strdup (value);
			set = TRUE;
		}
	}
	
	return set;
}

gboolean
amp_node_property_add (AnjutaProjectNode *node, AmpPropertyInfo *prop)
{
	AnjutaProjectPropertyList **properties = &(ANJUTA_PROJECT_NODE_DATA(node)->properties);
	AnjutaProjectPropertyItem *list;
	gboolean set = FALSE;

	g_message ("properties %p", *properties);
	for (list = anjuta_project_property_first (*properties); list != NULL; list = anjuta_project_property_next (list))
	{
		AmpPropertyInfo *info = (AmpPropertyInfo *)anjuta_project_property_get_info (list);

		if ((info->token_type == prop->token_type) && (info->position == prop->position))
		{
			AnjutaProjectPropertyInfo *old_prop;

			old_prop = anjuta_project_property_lookup (*properties, list);
			if (old_prop != NULL)
			{
				*properties = anjuta_project_property_remove (*properties, list);
				g_message ("remove properties %p", *properties);
				amp_property_free (old_prop);
			}
			*properties = anjuta_project_property_insert (*properties, list, (AnjutaProjectPropertyInfo *)prop);
			set = TRUE;
			break;
		}
	}

	if (!set) amp_property_free ((AnjutaProjectPropertyInfo *)prop);
	
	return set;
}

/* Get property list
 *---------------------------------------------------------------------------*/

GList*
amp_get_project_property_list (void)
{
	return amp_create_property_list (&AmpProjectPropertyList, AmpProjectProperties);
}

GList*
amp_get_group_property_list (void)
{
	return amp_create_property_list (&AmpGroupPropertyList, AmpGroupProperties);
}

GList*
amp_get_target_property_list (AnjutaProjectTargetType type)
{
	switch (type->base)
	{
	case ANJUTA_TARGET_MAN:
		return amp_create_property_list (&AmpManTargetPropertyList, AmpManTargetProperties);
	default:
		return amp_create_property_list (&AmpTargetPropertyList, AmpTargetProperties);
	}
}

GList*
amp_get_source_property_list (void)
{
	return NULL;
}
