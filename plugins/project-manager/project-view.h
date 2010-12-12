/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* project-view.h
 *
 * Copyright (C) 2000-2002  JP Rosevear
 * Copyright (C) 2002  Dave Camp
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

#ifndef _PROJECT_VIEW_H_
#define _PROJECT_VIEW_H_

#include <gtk/gtk.h>
#include <libanjuta/anjuta-project.h>
#include "tree-data.h"

G_BEGIN_DECLS

#define GBF_TYPE_PROJECT_VIEW		  (gbf_project_view_get_type ())
#define GBF_PROJECT_VIEW(obj)		  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GBF_TYPE_PROJECT_VIEW, GbfProjectView))
#define GBF_PROJECT_VIEW_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), GBF_TYPE_PROJECT_VIEW, GbfProjectViewClass))
#define GBF_IS_PROJECT_VIEW(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GBF_TYPE_PROJECT_VIEW))
#define GBF_IS_PROJECT_VIEW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((obj), GBF_TYPE_PROJECT_VIEW))

typedef struct _GbfProjectView        GbfProjectView;
typedef struct _GbfProjectViewClass   GbfProjectViewClass;


struct _GbfProjectView {
	GtkTreeView parent;

	GbfProjectModel *model;
	GtkTreeModelFilter *filter;

	GNode *expanded;
};

struct _GbfProjectViewClass {
	GtkTreeViewClass parent_class;

	void (*node_loaded) (GbfProjectView *project_view,
	                     GtkTreeIter *iter,
	                     gboolean complete,
	                     GError *error);
	
	void (* uri_activated)    (GbfProjectView *project_view,
				   const char     *uri);

	void (* target_selected)  (GbfProjectView      *project_view,
				   AnjutaProjectNode   *target);
	void (* group_selected)  (GbfProjectView       *project_view,
				   AnjutaProjectNode   *group);
};

GType                       gbf_project_view_get_type          (void);
GtkWidget                  *gbf_project_view_new               (void);

AnjutaProjectNode          *gbf_project_filter_view_find_selected     (GtkTreeView *view,
									AnjutaProjectNodeType type);
GList                      *gbf_project_filter_view_get_all_selected  (GtkTreeView *view);


AnjutaProjectNode          *gbf_project_view_find_selected     (GbfProjectView *view,
							        AnjutaProjectNodeType type);
GbfTreeData                *gbf_project_view_get_first_selected(GbfProjectView *view,
                                                                GtkTreeIter    *selected);
GList                      *gbf_project_view_get_all_selected  (GbfProjectView *view);
GList                      *gbf_project_view_get_all_selected_iter  (GbfProjectView *view);

GList			*gbf_project_view_get_shortcut_list (GbfProjectView *view);
GList			*gbf_project_view_get_expanded_list (GbfProjectView *view);

void			gbf_project_view_remove_all_shortcut (GbfProjectView* view);
void			gbf_project_view_set_shortcut_list (GbfProjectView *view,
								GList          *shortcuts);
void			gbf_project_view_set_expanded_list (GbfProjectView *view,
								GList   *expanded);

void            gbf_project_view_update_tree (GbfProjectView *view,
                                                    AnjutaProjectNode *parent,
                                                    GtkTreeIter *iter);

			                                       
AnjutaProjectNode *gbf_project_view_get_node_from_iter (GbfProjectView *view, GtkTreeIter *iter);

AnjutaProjectNode *gbf_project_view_get_node_from_file (GbfProjectView *view, AnjutaProjectNodeType type, GFile *file);

gboolean gbf_project_view_remove_data (GbfProjectView *view, GbfTreeData *data, GError **error);

void gbf_project_view_set_project (GbfProjectView *view, AnjutaPmProject *project);

gboolean gbf_project_view_find_file (GbfProjectView *view, GtkTreeIter* iter, GFile *file, GbfTreeNodeType type);

GbfProjectModel *gbf_project_view_get_model (GbfProjectView *view);



G_END_DECLS

#endif /* _PROJECT_VIEW_H_ */
