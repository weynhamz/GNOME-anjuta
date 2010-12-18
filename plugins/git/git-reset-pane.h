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

#ifndef _GIT_RESET_PANE_H_
#define _GIT_RESET_PANE_H_

#include <glib-object.h>
#include <libanjuta/anjuta-entry.h>
#include "git-pane.h"
#include "plugin.h"
#include "git-reset-tree-command.h"

G_BEGIN_DECLS

#define GIT_TYPE_RESET_PANE             (git_reset_pane_get_type ())
#define GIT_RESET_PANE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIT_TYPE_RESET_PANE, GitResetPane))
#define GIT_RESET_PANE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GIT_TYPE_RESET_PANE, GitResetPaneClass))
#define GIT_IS_RESET_PANE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIT_TYPE_RESET_PANE))
#define GIT_IS_RESET_PANE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIT_TYPE_RESET_PANE))
#define GIT_RESET_PANE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GIT_TYPE_RESET_PANE, GitResetPaneClass))

typedef struct _GitResetPaneClass GitResetPaneClass;
typedef struct _GitResetPane GitResetPane;
typedef struct _GitResetPanePriv GitResetPanePriv;

struct _GitResetPaneClass
{
	GitPaneClass parent_class;
};

struct _GitResetPane
{
	GitPane parent_instance;

	GitResetPanePriv *priv;
};

GType git_reset_pane_get_type (void) G_GNUC_CONST;
AnjutaDockPane *git_reset_pane_new (Git *plugin);
void on_reset_button_clicked (GtkAction *action, Git *plugin);

G_END_DECLS

#endif /* _GIT_RESET_PANE_H_ */
