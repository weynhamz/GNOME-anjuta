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

static AmpProjectPropertyInfo AmpProjectProperties[] = {
	{{N_("Name:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 0},
	{{N_("Version:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 1},
	{{N_("Bug report URL:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 2},
	{{N_("Package name:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 3},
	{{N_("URL:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{NULL, ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 0}};

static GList* AmpProjectPropertyList = NULL;


static AmpGroupPropertyInfo AmpGroupProperties[] = {
	{{N_("Name:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 0},
	{{N_("Version:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 1},
	{{N_("Bug report URL:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 2},
	{{N_("Package name:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 3},
	{{N_("URL:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{NULL, ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 0}};

static GList* AmpGroupPropertyList = NULL;


static AmpTargetPropertyInfo AmpTargetProperties[] = {
	{{N_("Name:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 0},
	{{N_("Version:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 1},
	{{N_("Bug report URL:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 2},
	{{N_("Package name:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 3},
	{{N_("URL:"), ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 4},
	{{NULL, ANJUTA_PROJECT_PROPERTY_STRING, NULL, NULL}, NULL, 0}};

static GList* AmpTargetPropertyList = NULL;

/* Helper functions
 *---------------------------------------------------------------------------*/

/* Private functions
 *---------------------------------------------------------------------------*/

/* Public functions
 *---------------------------------------------------------------------------*/

GList*
amp_get_project_property_list (void)
{
	if (AmpProjectPropertyList == NULL)
	{
		AmpProjectPropertyInfo *prop;

		for (prop = AmpProjectProperties; prop->base.name != NULL; prop++)
		{
			AmpProjectPropertyList = g_list_prepend (AmpProjectPropertyList, prop);
		}
		AmpProjectPropertyList = g_list_reverse (AmpProjectPropertyList);
	}

	return AmpProjectPropertyList;
}

GList*
amp_get_group_property_list (void)
{
	if (AmpGroupPropertyList == NULL)
	{
		AmpGroupPropertyInfo *prop;

		for (prop = AmpGroupProperties; prop->base.name != NULL; prop++)
		{
			AmpGroupPropertyList = g_list_prepend (AmpGroupPropertyList, prop);
		}
		AmpGroupPropertyList = g_list_reverse (AmpGroupPropertyList);
	}

	return AmpGroupPropertyList;
}

GList*
amp_get_target_property_list (void)
{
	if (AmpTargetPropertyList == NULL)
	{
		AmpTargetPropertyInfo *prop;

		for (prop = AmpTargetProperties; prop->base.name != NULL; prop++)
		{
			AmpTargetPropertyList = g_list_prepend (AmpTargetPropertyList, prop);
		}
		AmpTargetPropertyList = g_list_reverse (AmpTargetPropertyList);
	}

	return AmpTargetPropertyList;
}

GList*
amp_get_source_property_list (void)
{
	return NULL;
}
