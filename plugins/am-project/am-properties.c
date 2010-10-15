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
#include "ac-writer.h"

#include <glib/gi18n.h>


/* Types
  *---------------------------------------------------------------------------*/

/* Constants
  *---------------------------------------------------------------------------*/

static AmpProperty AmpProjectProperties[] = {
	{{N_("Name:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AC_TOKEN_AC_INIT, 0, NULL},
	{{N_("Version:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AC_TOKEN_AC_INIT, 1, NULL},
	{{N_("Bug report URL:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AC_TOKEN_AC_INIT, 2, NULL},
	{{N_("Package name:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AC_TOKEN_AC_INIT, 3, NULL},
	{{N_("URL:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AC_TOKEN_AC_INIT, 4, NULL},
	{{NULL, ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AC_TOKEN_AC_INIT, 5, NULL}};

static GList* AmpProjectPropertyList = NULL;


static AmpProperty AmpGroupProperties[] = {
	{{N_("Linker flags:"), ANJUTA_PROJECT_PROPERTY_LIST, NULL, NULL}, AM_TOKEN__LDFLAGS, 0, NULL},
	{{N_("C preprocessor flags:"), ANJUTA_PROJECT_PROPERTY_LIST, NULL, NULL}, AM_TOKEN__CPPFLAGS, 0, NULL},
	{{N_("C compiler flags:"), ANJUTA_PROJECT_PROPERTY_LIST, NULL, NULL}, AM_TOKEN__CFLAGS, 0, NULL},
	{{N_("C++ compiler flags:"), ANJUTA_PROJECT_PROPERTY_LIST, NULL, NULL}, AM_TOKEN__CXXFLAGS, 0, NULL},
	{{N_("Java Compiler flags:"), ANJUTA_PROJECT_PROPERTY_LIST, NULL, NULL}, AM_TOKEN__JAVACFLAGS, 0, NULL},
	{{N_("Fortan compiler flags:"), ANJUTA_PROJECT_PROPERTY_LIST, NULL, NULL}, AM_TOKEN__FCFLAGS, 0, NULL},
	{{N_("Objective C compiler flags:"), ANJUTA_PROJECT_PROPERTY_LIST, NULL, NULL}, AM_TOKEN__OBJCFLAGS, 0, NULL},
	{{N_("Lex/Flex flags:"), ANJUTA_PROJECT_PROPERTY_LIST, NULL, NULL}, AM_TOKEN__LFLAGS, 0, NULL},
	{{N_("Yacc/Bison flags:"), ANJUTA_PROJECT_PROPERTY_LIST, NULL, NULL},AM_TOKEN__YFLAGS, 0, NULL},
	{{N_("Install directories:"), ANJUTA_PROJECT_PROPERTY_MAP, NULL, NULL}, AM_TOKEN_DIR, 0, NULL},
	{{NULL, ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, 0, 0, NULL}};

static GList* AmpGroupPropertyList = NULL;


static AmpProperty AmpTargetProperties[] = {
	{{N_("Do not install:"), ANJUTA_PROJECT_PROPERTY_BOOLEAN, NULL, NULL}, AM_TOKEN__PROGRAMS, 3, NULL},
	{{N_("Installation directory:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AM_TOKEN__PROGRAMS, 6, NULL},
	{{N_("Linker flags:"), ANJUTA_PROJECT_PROPERTY_LIST, NULL, NULL}, AM_TOKEN_TARGET_LDFLAGS, 0, NULL},
	{{N_("Additional libraries:"), ANJUTA_PROJECT_PROPERTY_LIST, NULL, NULL}, AM_TOKEN_TARGET_LIBADD, 0, NULL},
	{{N_("Additional objects:"), ANJUTA_PROJECT_PROPERTY_LIST, NULL, NULL}, AM_TOKEN_TARGET_LDADD, 0, NULL},
	{{N_("C preprocessor flags:"), ANJUTA_PROJECT_PROPERTY_LIST, NULL, NULL}, AM_TOKEN_TARGET_CPPFLAGS, 0, NULL},
	{{N_("C compiler flags:"), ANJUTA_PROJECT_PROPERTY_LIST, NULL, NULL}, AM_TOKEN_TARGET_CFLAGS, 0, NULL},
	{{N_("C++ compiler flags:"), ANJUTA_PROJECT_PROPERTY_LIST, NULL, NULL}, AM_TOKEN_TARGET_CXXFLAGS, 0, NULL},
	{{N_("Java Compiler flags:"), ANJUTA_PROJECT_PROPERTY_LIST, NULL, NULL}, AM_TOKEN_TARGET_JAVACFLAGS, 0, NULL},
	{{N_("Fortan compiler flags:"), ANJUTA_PROJECT_PROPERTY_LIST, NULL, NULL}, AM_TOKEN_TARGET_FCFLAGS, 0, NULL},
	{{N_("Objective C compiler flags:"), ANJUTA_PROJECT_PROPERTY_LIST, NULL, NULL}, AM_TOKEN_TARGET_OBJCFLAGS, 0, NULL},
	{{N_("Lex/Flex flags:"), ANJUTA_PROJECT_PROPERTY_LIST, NULL, NULL}, AM_TOKEN_TARGET_LFLAGS, 0, NULL},
	{{N_("Yacc/Bison flags:"), ANJUTA_PROJECT_PROPERTY_LIST, NULL, NULL}, AM_TOKEN_TARGET_YFLAGS, 0, NULL},
	{{N_("Additional dependencies:"), ANJUTA_PROJECT_PROPERTY_LIST, NULL, NULL}, AM_TOKEN_TARGET_DEPENDENCIES, 0, NULL},
	{{N_("Include in distribution:"), ANJUTA_PROJECT_PROPERTY_BOOLEAN, NULL, NULL}, AM_TOKEN__PROGRAMS, 2, NULL},
	{{N_("Build for check only:"), ANJUTA_PROJECT_PROPERTY_BOOLEAN, NULL, NULL}, AM_TOKEN__PROGRAMS, 4, NULL},
	{{N_("Do not use prefix:"), ANJUTA_PROJECT_PROPERTY_BOOLEAN, NULL, NULL}, AM_TOKEN__PROGRAMS, 1, NULL},
	{{N_("Keep target path:"), ANJUTA_PROJECT_PROPERTY_BOOLEAN, NULL, NULL}, AM_TOKEN__PROGRAMS, 0, NULL},
	{{NULL, ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, 0, 0, NULL}};

static GList* AmpTargetPropertyList = NULL;

static AmpProperty AmpManTargetProperties[] = {
	{{N_("Additional dependencies:"), ANJUTA_PROJECT_PROPERTY_LIST, NULL, NULL}, 0, 0, NULL},
	{{N_("Do not use prefix:"), ANJUTA_PROJECT_PROPERTY_BOOLEAN, NULL, NULL}, AM_TOKEN__PROGRAMS, 1, NULL},
	{{N_("Manual section:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, AM_TOKEN__PROGRAMS, 5, NULL},
	{{NULL, ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, 0, 0, NULL}};

static GList* AmpManTargetPropertyList = NULL;

/* Helper functions
 *---------------------------------------------------------------------------*/

/* Private functions
 *---------------------------------------------------------------------------*/

static GList *
amp_create_property_list (GList **list, AmpProperty *properties)
{
	if (*list == NULL)
	{
		AmpProperty *prop;

		for (prop = properties; prop->base.name != NULL; prop++)
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

AnjutaProjectProperty *
amp_property_new (const gchar *name, AnjutaTokenType type, gint position, const gchar *value, AnjutaToken *token)
{
	AmpProperty*prop;

	prop = g_slice_new0(AmpProperty);
	prop->base.name = g_strdup (name);
	prop->base.value = g_strdup (value);
	prop->token = token;
	prop->token_type = type;
	prop->position = position;

	return (AnjutaProjectProperty *)prop;
}

void
amp_property_free (AnjutaProjectProperty *prop)
{
	if (prop->native == NULL) return;
		
	if ((prop->name != NULL) && (prop->name != prop->native->name))
	{
		g_free (prop->name);
	}
	if ((prop->value != NULL) && (prop->value != prop->native->value))
	{
		g_free (prop->value);
	}
	g_slice_free (AmpProperty, (AmpProperty *)prop);
}

/* Set property list
 *---------------------------------------------------------------------------*/

gboolean
amp_node_property_load (AnjutaProjectNode *node, gint token_type, gint position, const gchar *value, AnjutaToken *token)
{
	GList *item;
	gboolean set = FALSE;
	
	for (item = anjuta_project_node_get_native_properties (node); item != NULL; item = g_list_next (item))
	{
		AmpProperty *prop = (AmpProperty *)item->data;

		if ((prop->token_type == token_type) && (prop->position == position))
		{
			AnjutaProjectProperty *new_prop;

			new_prop = anjuta_project_node_get_property (node, (AnjutaProjectProperty *)prop);
			if (new_prop == NULL)
			{
				new_prop = amp_property_new (NULL, token_type, position, NULL, token);
				node->custom_properties = g_list_prepend (node->custom_properties, new_prop);
			}
	
			if ((new_prop->value != NULL) && (new_prop->value != prop->base.value)) g_free (new_prop->value);
			new_prop->value = g_strdup (value);
			set = TRUE;
		}
	}
	
	return set;
}

gboolean
amp_node_property_add (AnjutaProjectNode *node, AnjutaProjectProperty *new_prop)
{
	GList *item;
	gboolean set = FALSE;

	for (item = anjuta_project_node_get_native_properties (node); item != NULL; item = g_list_next (item))
	{
		AmpProperty *prop = (AmpProperty *)item->data;
		AnjutaToken *arg;
		GString *list;

		if ((prop->token_type == ((AmpProperty *)new_prop)->token_type) && (prop->position == ((AmpProperty *)new_prop)->position))
		{
			if (prop->base.type != ANJUTA_PROJECT_PROPERTY_MAP)
			{
				/* Replace property */
				AnjutaProjectProperty *old_prop;

				old_prop = anjuta_project_node_remove_property (node, (AnjutaProjectProperty *)prop);
				if (old_prop != NULL)
				{
					amp_property_free (old_prop);
				}
			}
			anjuta_project_node_insert_property (node, (AnjutaProjectProperty *)prop, new_prop);

			switch (prop->base.type)
			{
			case  ANJUTA_PROJECT_PROPERTY_LIST:
				/* Re-evaluate token to remove useless space between item */
				
				list = g_string_new (new_prop->value);
				g_string_assign (list, "");
				for (arg = anjuta_token_first_word (((AmpProperty *)new_prop)->token); arg != NULL; arg = anjuta_token_next_word (arg))
				{
					gchar *value = anjuta_token_evaluate (arg);

					if (value != NULL)
					{
						if (list->len != 0) g_string_append_c (list, ' ');
						g_string_append (list, value);
					}
				}
				g_free (new_prop->value);
				new_prop->value = g_string_free (list, FALSE);
				break;
			case ANJUTA_PROJECT_PROPERTY_MAP:
			case ANJUTA_PROJECT_PROPERTY_STRING:
				/* Strip leading and trailing space */
				new_prop->value = g_strstrip (new_prop->value);
				break;
			default:
				break;
			}
			
			set = TRUE;
			break;
		}
	}

	if (!set) amp_property_free (new_prop);
	
	return set;
}

AnjutaProjectProperty *
amp_node_property_set (AnjutaProjectNode *node, AnjutaProjectProperty *prop, const gchar* value)
{
	AnjutaProjectProperty *new_prop;
		
	new_prop = anjuta_project_node_get_property (node, prop);
	if ((new_prop != NULL) && (new_prop->native != NULL))
	{
		if ((new_prop->value != NULL) && (new_prop->value != new_prop->native->value)) g_free (new_prop->value);
		new_prop->value = g_strdup (value);
	}
	else
	{
		new_prop = amp_property_new (NULL, ((AmpProperty *)prop)->token_type, ((AmpProperty *)prop)->position, value, NULL);
		anjuta_project_node_insert_property (node, prop, new_prop);
	}

	return new_prop;
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
amp_get_target_property_list (AnjutaProjectNodeType type)
{
	switch (type & ANJUTA_PROJECT_ID_MASK)
	{
	case ANJUTA_PROJECT_MAN:
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

GList*
amp_get_module_property_list (void)
{
	return NULL;
}

GList*
amp_get_package_property_list (void)
{
	return NULL;
}
