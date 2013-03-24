/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * git-shell-test
 * Copyright (C) James Liggett 2010 <jrliggett@cox.net>
 * 
 * git-shell-test is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * git-shell-test is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _GIT_BRANCHES_PANE_H_
#define _GIT_BRANCHES_PANE_H_

#include <glib-object.h>
#include "git-pane.h"
#include "git-branch-checkout-command.h"

G_BEGIN_DECLS

#define GIT_TYPE_BRANCHES_PANE             (git_branches_pane_get_type ())
#define GIT_BRANCHES_PANE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIT_TYPE_BRANCHES_PANE, GitBranchesPane))
#define GIT_BRANCHES_PANE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GIT_TYPE_BRANCHES_PANE, GitBranchesPaneClass))
#define GIT_IS_BRANCHES_PANE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIT_TYPE_BRANCHES_PANE))
#define GIT_IS_BRANCHES_PANE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIT_TYPE_BRANCHES_PANE))
#define GIT_BRANCHES_PANE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GIT_TYPE_BRANCHES_PANE, GitBranchesPaneClass))

typedef struct _GitBranchesPaneClass GitBranchesPaneClass;
typedef struct _GitBranchesPane GitBranchesPane;
typedef struct _GitBranchesPanePriv GitBranchesPanePriv;

struct _GitBranchesPaneClass
{
	GitPaneClass parent_class;
};

struct _GitBranchesPane
{
	GitPane parent_instance;
	GitBranchesPanePriv *priv;
};

GType git_branches_pane_get_type (void) G_GNUC_CONST;
AnjutaDockPane *git_branches_pane_new (Git *plugin);
GList *git_branches_pane_get_selected_local_branches (GitBranchesPane *self);
GList *git_branches_pane_get_selected_remote_branches (GitBranchesPane *self);
gsize git_branches_pane_count_selected_items (GitBranchesPane *self);
gchar *git_branches_pane_get_selected_branch (GitBranchesPane *self);
void git_branches_pane_set_select_column_visible (GitBranchesPane *self, 
                                                  gboolean visible);

G_END_DECLS

#endif /* _GIT_BRANCHES_PANE_H_ */
