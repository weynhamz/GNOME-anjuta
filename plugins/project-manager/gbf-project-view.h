/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gbf-project-view.h
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

#ifndef _GBF_PROJECT_TREE_H_
#define _GBF_PROJECT_TREE_H_

#include <gtk/gtk.h>
#include "gbf-tree-data.h"

G_BEGIN_DECLS

#define GBF_TYPE_PROJECT_VIEW		  (gbf_project_view_get_type ())
#define GBF_PROJECT_VIEW(obj)		  (GTK_CHECK_CAST ((obj), GBF_TYPE_PROJECT_VIEW, GbfProjectView))
#define GBF_PROJECT_VIEW_CLASS(klass)	  (GTK_CHECK_CLASS_CAST ((klass), GBF_TYPE_PROJECT_VIEW, GbfProjectViewClass))
#define GBF_IS_PROJECT_VIEW(obj)	  (GTK_CHECK_TYPE ((obj), GBF_TYPE_PROJECT_VIEW))
#define GBF_IS_PROJECT_VIEW_CLASS(klass)  (GTK_CHECK_CLASS_TYPE ((obj), GBF_TYPE_PROJECT_VIEW))

typedef struct _GbfProjectView        GbfProjectView;
typedef struct _GbfProjectViewPrivate GbfProjectViewPrivate;
typedef struct _GbfProjectViewClass   GbfProjectViewClass;


struct _GbfProjectView {
	GtkTreeView parent;

	GbfProjectViewPrivate *priv;
};

struct _GbfProjectViewClass {
	GtkTreeViewClass parent_class;

	void (* uri_activated)    (GbfProjectView *project_view,
				   const char     *uri);

	void (* target_selected)  (GbfProjectView *project_view,
				   const gchar    *target_id);
	void (* group_selected)  (GbfProjectView *project_view,
				   const gchar    *group_id);
};

GType                       gbf_project_view_get_type         (void);
GtkWidget                  *gbf_project_view_new              (void);

GbfTreeData                *gbf_project_view_find_selected    (GbfProjectView *view,
							       GbfTreeNodeType type);

G_END_DECLS

#endif /* _GBF_PROJECT_VIEW_H_ */
