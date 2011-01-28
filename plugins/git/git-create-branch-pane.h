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

#ifndef _GIT_CREATE_BRANCH_PANE_H_
#define _GIT_CREATE_BRANCH_PANE_H_

#include <glib-object.h>
#include <libanjuta/anjuta-entry.h>
#include "git-pane.h"
#include "plugin.h"
#include "git-branch-create-command.h"

G_BEGIN_DECLS

#define GIT_TYPE_CREATE_BRANCH_PANE             (git_create_branch_pane_get_type ())
#define GIT_CREATE_BRANCH_PANE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIT_TYPE_CREATE_BRANCH_PANE, GitCreateBranchPane))
#define GIT_CREATE_BRANCH_PANE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GIT_TYPE_CREATE_BRANCH_PANE, GitCreateBranchPaneClass))
#define GIT_IS_CREATE_BRANCH_PANE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIT_TYPE_CREATE_BRANCH_PANE))
#define GIT_IS_CREATE_BRANCH_PANE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIT_TYPE_CREATE_BRANCH_PANE))
#define GIT_CREATE_BRANCH_PANE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GIT_TYPE_CREATE_BRANCH_PANE, GitCreateBranchPaneClass))

typedef struct _GitCreateBranchPaneClass GitCreateBranchPaneClass;
typedef struct _GitCreateBranchPane GitCreateBranchPane;
typedef struct _GitCreateBranchPanePriv GitCreateBranchPanePriv;

struct _GitCreateBranchPaneClass
{
	GitPaneClass parent_class;
};

struct _GitCreateBranchPane
{
	GitPane parent_instance;

	GitCreateBranchPanePriv *priv;
};

GType git_create_branch_pane_get_type (void) G_GNUC_CONST;
AnjutaDockPane * git_create_branch_pane_new (Git *plugin);
void on_create_branch_button_clicked (GtkAction *action, Git *plugin);

G_END_DECLS

#endif /* _GIT_CREATE_BRANCH_PANE_H_ */
