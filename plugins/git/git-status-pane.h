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

#ifndef _GIT_STATUS_PANE_H_
#define _GIT_STATUS_PANE_H_

#include <glib-object.h>
#include "git-pane.h"
#include "git-status-command.h"
#include "git-add-command.h"

G_BEGIN_DECLS

#define GIT_TYPE_STATUS_PANE             (git_status_pane_get_type ())
#define GIT_STATUS_PANE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIT_TYPE_STATUS_PANE, GitStatusPane))
#define GIT_STATUS_PANE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GIT_TYPE_STATUS_PANE, GitStatusPaneClass))
#define GIT_IS_STATUS_PANE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIT_TYPE_STATUS_PANE))
#define GIT_IS_STATUS_PANE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIT_TYPE_STATUS_PANE))
#define GIT_STATUS_PANE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GIT_TYPE_STATUS_PANE, GitStatusPaneClass))

typedef struct _GitStatusPaneClass GitStatusPaneClass;
typedef struct _GitStatusPane GitStatusPane;
typedef struct _GitStatusPanePriv GitStatusPanePriv;

struct _GitStatusPaneClass
{
	GitPaneClass parent_class;
};

struct _GitStatusPane
{
	GitPane parent_instance;

	GitStatusPanePriv *priv;
};

GType git_status_pane_get_type (void) G_GNUC_CONST;
AnjutaDockPane *git_status_pane_new (Git *plugin);
GList *git_status_pane_get_checked_commit_items (GitStatusPane *self,
                                                 AnjutaVcsStatus status_codes);
GList *git_status_pane_get_checked_not_updated_items (GitStatusPane *self,
                                                      AnjutaVcsStatus status_codes);
GList *git_status_pane_get_all_checked_items (GitStatusPane *self,
                                              AnjutaVcsStatus status_codes);
gchar *git_status_pane_get_selected_commit_path (GitStatusPane *self);
gchar *git_status_pane_get_selected_not_updated_path (GitStatusPane *self);

G_END_DECLS

#endif /* _GIT_STATUS_PANE_H_ */
