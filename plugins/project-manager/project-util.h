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

#ifndef __PROJECT_UTIL_H__
#define __PROJECT_UTIL_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <libanjuta/anjuta-project.h>
#include "project.h"

G_BEGIN_DECLS

GList * gbf_project_util_all_child (AnjutaProjectNode *parent,
                                        AnjutaProjectNodeType type);

GList * gbf_project_util_node_all (AnjutaProjectNode *parent,
                                        AnjutaProjectNodeType type);

GList * gbf_project_util_replace_by_file (GList* list);
				    
G_END_DECLS

#endif /* __PROJECT_UTIL_H__ */
