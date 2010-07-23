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

#ifndef _GIT_CREATE_TAG_PANE_H_
#define _GIT_CREATE_TAG_PANE_H_

#include <glib-object.h>
#include <libanjuta/anjuta-column-text-view.h>
#include "git-pane.h"
#include "plugin.h"
#include "git-tag-create-command.h"

G_BEGIN_DECLS

#define GIT_TYPE_CREATE_TAG_PANE             (git_create_tag_pane_get_type ())
#define GIT_CREATE_TAG_PANE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIT_TYPE_CREATE_TAG_PANE, GitCreateTagPane))
#define GIT_CREATE_TAG_PANE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GIT_TYPE_CREATE_TAG_PANE, GitCreateTagPaneClass))
#define GIT_IS_CREATE_TAG_PANE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIT_TYPE_CREATE_TAG_PANE))
#define GIT_IS_CREATE_TAG_PANE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIT_TYPE_CREATE_TAG_PANE))
#define GIT_CREATE_TAG_PANE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GIT_TYPE_CREATE_TAG_PANE, GitCreateTagPaneClass))

typedef struct _GitCreateTagPaneClass GitCreateTagPaneClass;
typedef struct _GitCreateTagPane GitCreateTagPane;
typedef struct _GitCreateTagPanePriv GitCreateTagPanePriv;

struct _GitCreateTagPaneClass
{
	GitPaneClass parent_class;
};

struct _GitCreateTagPane
{
	GitPane parent_instance;

	GitCreateTagPanePriv *priv;
};

GType git_create_tag_pane_get_type (void) G_GNUC_CONST;
AnjutaDockPane *git_create_tag_pane_new (Git *plugin);
void on_create_tag_button_clicked (GtkAction *action, Git *plugin);

G_END_DECLS

#endif /* _GIT_CREATE_TAG_PANE_H_ */
