/*  -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4; coding: utf-8 -*-
 *
 * Copyright (C) 2003 Gustavo Giráldez
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Gustavo Giráldez <gustavo.giraldez@gmx.net>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdarg.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#include "project-util.h"

GList *
gbf_project_util_node_all (AnjutaProjectNode *parent, AnjutaProjectNodeType type)
{
    AnjutaProjectNode *node;
    GList *list = NULL;
    gint type_id;
    gint type_flag;
    gint type_type;

    type_type = type & ANJUTA_PROJECT_TYPE_MASK;
    type_flag = type & ANJUTA_PROJECT_FLAG_MASK;
    type_id = type & ANJUTA_PROJECT_ID_MASK;

    for (node = anjuta_project_node_first_child (parent); node != NULL; node = anjuta_project_node_next_sibling (node))
    {
        GList *child_list;

        if ((type_type == 0) || (anjuta_project_node_get_node_type (node) == type_type))
        {
            gint type;

            type = anjuta_project_node_get_full_type (node);
            if (((type_id == 0) || (type_id == (type & ANJUTA_PROJECT_ID_MASK))) &&
                ((type_flag == 0) || ((type & type_flag) != 0)))
            {
                list = g_list_prepend (list, node);
            }
        }

        child_list = gbf_project_util_node_all (node, type);
        child_list = g_list_reverse (child_list);
        list = g_list_concat (child_list, list);
    }

    list = g_list_reverse (list);

    return list;
}

GList *
gbf_project_util_replace_by_file (GList* list)
{
        GList* link;

	for (link = g_list_first (list); link != NULL; link = g_list_next (link))
	{
                AnjutaProjectNode *node = (AnjutaProjectNode *)link->data;

                link->data = g_object_ref (anjuta_project_node_get_file (node));
	}

        return list;
}

