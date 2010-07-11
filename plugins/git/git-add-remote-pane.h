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

#ifndef _GIT_ADD_REMOTE_PANE_H_
#define _GIT_ADD_REMOTE_PANE_H_

#include <glib-object.h>
#include "git-pane.h"
#include "plugin.h"
#include "git-remote-add-command.h"

G_BEGIN_DECLS

#define GIT_TYPE_ADD_REMOTE_PANE             (git_add_remote_pane_get_type ())
#define GIT_ADD_REMOTE_PANE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIT_TYPE_ADD_REMOTE_PANE, GitAddRemotePane))
#define GIT_ADD_REMOTE_PANE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GIT_TYPE_ADD_REMOTE_PANE, GitAddRemotePaneClass))
#define GIT_IS_ADD_REMOTE_PANE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIT_TYPE_ADD_REMOTE_PANE))
#define GIT_IS_ADD_REMOTE_PANE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIT_TYPE_ADD_REMOTE_PANE))
#define GIT_ADD_REMOTE_PANE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GIT_TYPE_ADD_REMOTE_PANE, GitAddRemotePaneClass))

typedef struct _GitAddRemotePaneClass GitAddRemotePaneClass;
typedef struct _GitAddRemotePane GitAddRemotePane;
typedef struct _GitAddRemotePanePriv GitAddRemotePanePriv;

struct _GitAddRemotePaneClass
{
	GitPaneClass parent_class;
};

struct _GitAddRemotePane
{
	GitPane parent_instance;

	GitAddRemotePanePriv *priv;
};

GType git_add_remote_pane_get_type (void) G_GNUC_CONST;
AnjutaDockPane *git_add_remote_pane_new (Git *plugin);
void on_add_remote_button_clicked (GtkAction *action, Git *plugin);

G_END_DECLS

#endif /* _GIT_ADD_REMOTE_PANE_H_ */
