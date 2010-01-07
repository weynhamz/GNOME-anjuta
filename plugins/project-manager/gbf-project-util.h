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

#ifndef __GBF_PROJECT_UTIL_H__
#define __GBF_PROJECT_UTIL_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <libanjuta/anjuta-project.h>
#include "gbf-project-model.h"

G_BEGIN_DECLS

AnjutaProjectGroup* gbf_project_util_new_group  (GbfProjectModel   *model,
				                GtkWindow          *parent,
				                GbfTreeData        *default_group,
				                const gchar        *default_group_name_to_add);

AnjutaProjectTarget* gbf_project_util_new_target (GbfProjectModel  *model,
				                GtkWindow          *parent,
				                GbfTreeData        *default_group,
				                const gchar        *default_target_name_to_add);

AnjutaProjectSource* gbf_project_util_add_source (GbfProjectModel   *model,
				                GtkWindow           *parent,
				                GbfTreeData         *default_target,
				                const gchar         *default_uri_to_add);

GList* gbf_project_util_add_source_multi (GbfProjectModel   *model,
				        GtkWindow           *parent,
        		                GbfTreeData         *default_target,
				        GList               *uris_to_add);
				    
GList * gbf_project_util_all_child (AnjutaProjectNode *parent,
                                        AnjutaProjectNodeType type);

GList * gbf_project_util_node_all (AnjutaProjectNode *parent,
                                        AnjutaProjectNodeType type);

GList * gbf_project_util_replace_by_uri (GList* list);
				    
G_END_DECLS

#endif /* __GBF_PROJECT_UTIL_H__ */
