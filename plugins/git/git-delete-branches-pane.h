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

#ifndef _GIT_DELETE_BRANCHES_PANE_H_
#define _GIT_DELETE_BRANCHES_PANE_H_

#include <glib-object.h>
#include <libanjuta/anjuta-command-queue.h>
#include "git-pane.h"
#include "git-branches-pane.h"
#include "git-branch-delete-command.h"

G_BEGIN_DECLS

#define GIT_TYPE_DELETE_BRANCHES_PANE             (git_delete_branches_pane_get_type ())
#define GIT_DELETE_BRANCHES_PANE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIT_TYPE_DELETE_BRANCHES_PANE, GitDeleteBranchesPane))
#define GIT_DELETE_BRANCHES_PANE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GIT_TYPE_DELETE_BRANCHES_PANE, GitDeleteBranchesPaneClass))
#define GIT_IS_DELETE_BRANCHES_PANE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIT_TYPE_DELETE_BRANCHES_PANE))
#define GIT_IS_DELETE_BRANCHES_PANE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIT_TYPE_DELETE_BRANCHES_PANE))
#define GIT_DELETE_BRANCHES_PANE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GIT_TYPE_DELETE_BRANCHES_PANE, GitDeleteBranchesPaneClass))

typedef struct _GitDeleteBranchesPaneClass GitDeleteBranchesPaneClass;
typedef struct _GitDeleteBranchesPane GitDeleteBranchesPane;
typedef struct _GitDeleteBranchesPanePriv GitDeleteBranchesPanePriv;

struct _GitDeleteBranchesPaneClass
{
	GitPaneClass parent_class;
};

struct _GitDeleteBranchesPane
{
	GitPane parent_instance;

	GitDeleteBranchesPanePriv *priv;
};

GType git_delete_branches_pane_get_type (void) G_GNUC_CONST;
AnjutaDockPane * git_delete_branches_pane_new (Git *plugin);
void on_delete_branches_button_clicked (GtkAction *action, 
                                        Git *plugin);

G_END_DECLS

#endif /* _GIT_DELETE_BRANCHES_PANE_H_ */
