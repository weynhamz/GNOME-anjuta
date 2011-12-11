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

#ifndef _GIT_TAGS_PANE_H_
#define _GIT_TAGS_PANE_H_

#include <glib-object.h>
#include "git-pane.h"

G_BEGIN_DECLS

#define GIT_TYPE_TAGS_PANE             (git_tags_pane_get_type ())
#define GIT_TAGS_PANE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIT_TYPE_TAGS_PANE, GitTagsPane))
#define GIT_TAGS_PANE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GIT_TYPE_TAGS_PANE, GitTagsPaneClass))
#define GIT_IS_TAGS_PANE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIT_TYPE_TAGS_PANE))
#define GIT_IS_TAGS_PANE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIT_TYPE_TAGS_PANE))
#define GIT_TAGS_PANE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GIT_TYPE_TAGS_PANE, GitTagsPaneClass))

typedef struct _GitTagsPaneClass GitTagsPaneClass;
typedef struct _GitTagsPane GitTagsPane;
typedef struct _GitTagsPanePriv GitTagsPanePriv;

struct _GitTagsPaneClass
{
	GitPaneClass parent_class;
};

struct _GitTagsPane
{
	GitPane parent_instance;
	
	GitTagsPanePriv *priv;
};

GType git_tags_pane_get_type (void) G_GNUC_CONST;
AnjutaDockPane *git_tags_pane_new (Git *plugin);
GList *git_tags_pane_get_selected_tags (GitTagsPane *self);
void git_tags_pane_update_ui (GitTagsPane *self);

G_END_DECLS

#endif /* _GIT_TAGS_PANE_H_ */
