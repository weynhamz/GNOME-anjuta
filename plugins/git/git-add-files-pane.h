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

#ifndef _GIT_ADD_FILES_PANE_H_
#define _GIT_ADD_FILES_PANE_H_

#include <glib-object.h>
#include <libanjuta/anjuta-file-list.h>
#include "git-pane.h"
#include "git-add-command.h"

G_BEGIN_DECLS

#define GIT_TYPE_ADD_FILES_PANE             (git_add_files_pane_get_type ())
#define GIT_ADD_FILES_PANE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIT_TYPE_ADD_FILES_PANE, GitAddFilesPane))
#define GIT_ADD_FILES_PANE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GIT_TYPE_ADD_FILES_PANE, GitAddFilesPaneClass))
#define GIT_IS_ADD_FILES_PANE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIT_TYPE_ADD_FILES_PANE))
#define GIT_IS_ADD_FILES_PANE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIT_TYPE_ADD_FILES_PANE))
#define GIT_ADD_FILES_PANE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GIT_TYPE_ADD_FILES_PANE, GitAddFilesPaneClass))

typedef struct _GitAddFilesPaneClass GitAddFilesPaneClass;
typedef struct _GitAddFilesPane GitAddFilesPane;
typedef struct _GitAddFilesPanePriv GitAddFilesPanePriv;

struct _GitAddFilesPaneClass
{
	GitPaneClass parent_class;
};

struct _GitAddFilesPane
{
	GitPane parent_instance;

	GitAddFilesPanePriv *priv;
};

GType git_add_files_pane_get_type (void) G_GNUC_CONST;
AnjutaDockPane * git_add_files_pane_new (Git *plugin);

void on_add_button_clicked (GtkAction *action, Git *plugin);

G_END_DECLS

#endif /* _GIT_ADD_FILES_PANE_H_ */
