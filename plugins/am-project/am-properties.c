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

#include <glib/gi18n.h>


/* Types
  *---------------------------------------------------------------------------*/

/* Constants
  *---------------------------------------------------------------------------*/

static AmpPropertyInfo AmpProjectProperties[] = {
	{{N_("Name:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 0},
	{{N_("Version:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 1},
	{{N_("Bug report URL:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 2},
	{{N_("Package name:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 3},
	{{N_("URL:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{NULL, ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 0}};

static GList* AmpProjectPropertyList = NULL;


static AmpPropertyInfo AmpGroupProperties[] = {
	{{N_("Linker flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("C preprocessor flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("C compiler flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("C++ compiler flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("Java Compiler flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("Fortan compiler flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("Objective C compiler flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("Lex/Flex flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("Yacc/Bison flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("Ratfor compiler flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{NULL, ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 0}};

static GList* AmpGroupPropertyList = NULL;


static AmpPropertyInfo AmpTargetProperties[] = {
	{{N_("Do not install:"), ANJUTA_PROJECT_PROPERTY_BOOLEAN, NULL, NULL}, NULL, 3},
	{{N_("Installation directory:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("Linker flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("C preprocessor flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("C compiler flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("C++ compiler flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("Java Compiler flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("Fortan compiler flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("Objective C compiler flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("Lex/Flex flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("Yacc/Bison flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("Ratfor compiler flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("Additional dependencies:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("Include in distribution:"), ANJUTA_PROJECT_PROPERTY_BOOLEAN, NULL, NULL}, NULL, 2},
	{{N_("Build for check only:"), ANJUTA_PROJECT_PROPERTY_BOOLEAN, NULL, NULL}, NULL, 3},
	{{N_("Do not use prefix:"), ANJUTA_PROJECT_PROPERTY_BOOLEAN, NULL, NULL}, NULL, 1},
	{{N_("Keep target path:"), ANJUTA_PROJECT_PROPERTY_BOOLEAN, NULL, NULL}, NULL, 0},
	{{NULL, ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 0}};

static GList* AmpTargetPropertyList = NULL;

static AmpPropertyInfo AmpManTargetProperties[] = {
	{{N_("Installation directory:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("Linker flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("C preprocessor flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("C compiler flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("C++ compiler flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("Java Compiler flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("Fortan compiler flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("Objective C compiler flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("Lex/Flex flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("Yacc/Bison flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("Ratfor compiler flags:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("Additional dependencies:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{N_("Include in distribution:"), ANJUTA_PROJECT_PROPERTY_BOOLEAN, NULL, NULL}, NULL, 2},
	{{N_("Keep target path:"), ANJUTA_PROJECT_PROPERTY_BOOLEAN, NULL, NULL}, NULL, 0},
	{{N_("Manual section:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{NULL, ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 0}};

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
