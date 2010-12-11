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

#ifndef _GIT_LOG_PANE_H_
#define _GIT_LOG_PANE_H_

#include <glib-object.h>
#include <libanjuta/anjuta-entry.h>
#include "git-pane.h"
#include "git-log-command.h"
#include "git-log-message-command.h"
#include "git-ref-command.h"
#include "giggle-graph-renderer.h"

G_BEGIN_DECLS

#define GIT_TYPE_LOG_PANE             (git_log_pane_get_type ())
#define GIT_LOG_PANE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIT_TYPE_LOG_PANE, GitLogPane))
#define GIT_LOG_PANE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GIT_TYPE_LOG_PANE, GitLogPaneClass))
#define GIT_IS_LOG_PANE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIT_TYPE_LOG_PANE))
#define GIT_IS_LOG_PANE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIT_TYPE_LOG_PANE))
#define GIT_LOG_PANE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GIT_TYPE_LOG_PANE, GitLogPaneClass))

typedef struct _GitLogPaneClass GitLogPaneClass;
typedef struct _GitLogPane GitLogPane;
typedef struct _GitLogPanePriv GitLogPanePriv;

struct _GitLogPaneClass
{
	GitPaneClass parent_class;
};

struct _GitLogPane
{
	GitPane parent_instance;

	GitLogPanePriv *priv;
};

GType git_log_pane_get_type (void) G_GNUC_CONST;
AnjutaDockPane *git_log_pane_new (Git *plugin);
void git_log_pane_set_working_directory (GitLogPane *self, 
                                         const gchar *working_directory);

G_END_DECLS

#endif /* _GIT_LOG_PANE_H_ */
