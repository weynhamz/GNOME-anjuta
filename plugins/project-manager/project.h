/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/* project.h
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _PROJECT_H_
#define _PROJECT_H_

#include <glib.h>
#include <glib-object.h>
#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/anjuta-project.h>
#include <libanjuta/interfaces/ianjuta-project.h>

G_BEGIN_DECLS

typedef struct _ProjectManagerProject ProjectManagerProject;

ProjectManagerProject* pm_project_new (AnjutaPlugin *plugin);
void pm_project_free (ProjectManagerProject* project);

gboolean pm_project_load (ProjectManagerProject *project, GFile *file, GError **error);
gboolean pm_project_unload (ProjectManagerProject *project, GError **error);
gboolean pm_project_refresh (ProjectManagerProject *project, GError **error);
GtkWidget *pm_project_configure (ProjectManagerProject *project, AnjutaProjectNode *node);
gboolean pm_project_remove (ProjectManagerProject *project, AnjutaProjectNode *node, GError **error);
IAnjutaProjectCapabilities pm_project_get_capabilities (ProjectManagerProject *project);
GList *pm_project_get_target_types (ProjectManagerProject *project);
GList *pm_project_get_packages (ProjectManagerProject *project);
AnjutaProjectNode *pm_project_add_group (ProjectManagerProject *project, AnjutaProjectNode *group, const gchar *name, GError **error);
AnjutaProjectNode *pm_project_add_target (ProjectManagerProject *project, AnjutaProjectNode *group, const gchar *name, AnjutaProjectTargetType type, GError **error);
AnjutaProjectNode *pm_project_add_source (ProjectManagerProject *project, AnjutaProjectNode *target, GFile *file, GError **error);
AnjutaProjectNode *pm_project_get_root (ProjectManagerProject *project);

gboolean pm_project_is_open (ProjectManagerProject *project);

IAnjutaProject *pm_project_get_project (ProjectManagerProject *project);

G_END_DECLS

#endif /* _PROJECT_H_ */
