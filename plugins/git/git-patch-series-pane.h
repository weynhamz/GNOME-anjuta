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

#ifndef _GIT_PATCH_SERIES_PANE_H_
#define _GIT_PATCH_SERIES_PANE_H_

#include <glib-object.h>
#include <libanjuta/anjuta-entry.h>
#include "git-pane.h"
#include "plugin.h"
#include "git-format-patch-command.h"

G_BEGIN_DECLS

#define GIT_TYPE_PATCH_SERIES_PANE             (git_patch_series_pane_get_type ())
#define GIT_PATCH_SERIES_PANE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIT_TYPE_PATCH_SERIES_PANE, GitPatchSeriesPane))
#define GIT_PATCH_SERIES_PANE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GIT_TYPE_PATCH_SERIES_PANE, GitPatchSeriesPaneClass))
#define GIT_IS_PATCH_SERIES_PANE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIT_TYPE_PATCH_SERIES_PANE))
#define GIT_IS_PATCH_SERIES_PANE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIT_TYPE_PATCH_SERIES_PANE))
#define GIT_PATCH_SERIES_PANE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GIT_TYPE_PATCH_SERIES_PANE, GitPatchSeriesPaneClass))

typedef struct _GitPatchSeriesPaneClass GitPatchSeriesPaneClass;
typedef struct _GitPatchSeriesPane GitPatchSeriesPane;
typedef struct _GitPatchSeriesPanePriv GitPatchSeriesPanePriv;

struct _GitPatchSeriesPaneClass
{
	GitPaneClass parent_class;
};

struct _GitPatchSeriesPane
{
	GitPane parent_instance;

	GitPatchSeriesPanePriv *priv;
};

GType git_patch_series_pane_get_type (void) G_GNUC_CONST;
AnjutaDockPane *git_patch_series_pane_new (Git *plugin);
void on_patch_series_button_clicked (GtkAction *action, Git *plugin);

G_END_DECLS

#endif /* _GIT_RESET_PANE_H_ */
