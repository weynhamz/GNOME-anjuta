/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2010 <jrliggett@cox.net>
 * 
 * anjuta is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _GIT_PUSH_PANE_H_
#define _GIT_PUSH_PANE_H_

#include <glib-object.h>
#include "git-pane.h"
#include "git-repository-selector.h"
#include "git-remotes-pane.h"
#include "git-branch-list-command.h"
#include "git-tag-list-command.h"
#include "git-push-command.h"

G_BEGIN_DECLS

#define GIT_TYPE_PUSH_PANE             (git_push_pane_get_type ())
#define GIT_PUSH_PANE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIT_TYPE_PUSH_PANE, GitPushPane))
#define GIT_PUSH_PANE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GIT_TYPE_PUSH_PANE, GitPushPaneClass))
#define GIT_IS_PUSH_PANE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIT_TYPE_PUSH_PANE))
#define GIT_IS_PUSH_PANE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIT_TYPE_PUSH_PANE))
#define GIT_PUSH_PANE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GIT_TYPE_PUSH_PANE, GitPushPaneClass))

typedef struct _GitPushPaneClass GitPushPaneClass;
typedef struct _GitPushPane GitPushPane;
typedef struct _GitPushPanePriv GitPushPanePriv;

struct _GitPushPaneClass
{
	GitPaneClass parent_class;
};

struct _GitPushPane
{
	GitPane parent_instance;

	GitPushPanePriv *priv;
};

GType git_push_pane_get_type (void) G_GNUC_CONST;
AnjutaDockPane *git_push_pane_new (Git *plugin);

void on_push_button_clicked (GtkAction *action, Git *plugin);

G_END_DECLS

#endif /* _GIT_PUSH_PANE_H_ */
