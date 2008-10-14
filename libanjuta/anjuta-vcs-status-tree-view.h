/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2007 <jrliggett@cox.net>
 * 
 * anjuta is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * anjuta is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with anjuta.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef _ANJUTA_VCS_STATUS_TREE_VIEW_H_
#define _ANJUTA_VCS_STATUS_TREE_VIEW_H_

#include <glib-object.h>
#include <gtk/gtk.h>
#include "anjuta-enum-types.h"
#include <libanjuta/anjuta-vcs-status.h>

G_BEGIN_DECLS

#define ANJUTA_VCS_TYPE_STATUS_TREE_VIEW             (anjuta_vcs_status_tree_view_get_type ())
#define ANJUTA_VCS_STATUS_TREE_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_VCS_TYPE_STATUS_TREE_VIEW, AnjutaVcsStatusTreeView))
#define ANJUTA_VCS_STATUS_TREE_VIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_VCS_TYPE_STATUS_TREE_VIEW, AnjutaVcsStatusTreeViewClass))
#define ANJUTA_VCS_IS_STATUS_TREE_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_VCS_TYPE_STATUS_TREE_VIEW))
#define ANJUTA_VCS_IS_STATUS_TREE_VIEW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_VCS_TYPE_STATUS_TREE_VIEW))
#define ANJUTA_VCS_STATUS_TREE_VIEW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_VCS_TYPE_STATUS_TREE_VIEW, AnjutaVcsStatusTreeViewClass))

/* Show default status codes: Modified, Added, Deleted, and Conflicted */
#define ANJUTA_VCS_DEFAULT_STATUS_CODES (ANJUTA_VCS_STATUS_MODIFIED | \
										 ANJUTA_VCS_STATUS_ADDED |    \
										 ANJUTA_VCS_STATUS_DELETED |  \
										 ANJUTA_VCS_STATUS_CONFLICTED)

typedef struct _AnjutaVcsStatusTreeViewClass AnjutaVcsStatusTreeViewClass;
typedef struct _AnjutaVcsStatusTreeView AnjutaVcsStatusTreeView;
typedef struct _AnjutaVcsStatusTreeViewPriv AnjutaVcsStatusTreeViewPriv;

struct _AnjutaVcsStatusTreeViewClass
{
	GtkTreeViewClass parent_class;
};

struct _AnjutaVcsStatusTreeView
{
	GtkTreeView parent_instance;
	
	AnjutaVcsStatusTreeViewPriv *priv;
};

GType anjuta_vcs_status_tree_view_get_type (void) G_GNUC_CONST;
GtkWidget *anjuta_vcs_status_tree_view_new (void);
void anjuta_vcs_status_tree_view_destroy (AnjutaVcsStatusTreeView *self);
void anjuta_vcs_status_tree_view_add (AnjutaVcsStatusTreeView *self, 
									  gchar *path, 
									  AnjutaVcsStatus status, 
									  gboolean selected);
void anjuta_vcs_status_tree_view_select_all (AnjutaVcsStatusTreeView *self);
void anjuta_vcs_status_tree_view_unselect_all (AnjutaVcsStatusTreeView *self);
GList *anjuta_vcs_status_tree_view_get_selected (AnjutaVcsStatusTreeView *self);

G_END_DECLS

#endif /* _ANJUTA_VCS_STATUS_TREE_VIEW_H_ */
