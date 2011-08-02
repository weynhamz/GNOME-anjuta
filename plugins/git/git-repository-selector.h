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

#ifndef _GIT_REPOSITORY_SELECTOR_H_
#define _GIT_REPOSITORY_SELECTOR_H_

#include <glib-object.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GIT_TYPE_REPOSITORY_SELECTOR             (git_repository_selector_get_type ())
#define GIT_REPOSITORY_SELECTOR(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIT_TYPE_REPOSITORY_SELECTOR, GitRepositorySelector))
#define GIT_REPOSITORY_SELECTOR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GIT_TYPE_REPOSITORY_SELECTOR, GitRepositorySelectorClass))
#define GIT_IS_REPOSITORY_SELECTOR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIT_TYPE_REPOSITORY_SELECTOR))
#define GIT_IS_REPOSITORY_SELECTOR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIT_TYPE_REPOSITORY_SELECTOR))
#define GIT_REPOSITORY_SELECTOR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GIT_TYPE_REPOSITORY_SELECTOR, GitRepositorySelectorClass))

typedef struct _GitRepositorySelectorClass GitRepositorySelectorClass;
typedef struct _GitRepositorySelector GitRepositorySelector;
typedef struct _GitRepositorySelectorPriv GitRepositorySelectorPriv;

struct _GitRepositorySelectorClass
{
	GtkBoxClass parent_class;
};

struct _GitRepositorySelector
{
	GtkBox parent_instance;

	GitRepositorySelectorPriv *priv;
};

typedef enum
{
	GIT_REPOSITORY_SELECTOR_REMOTE,
	GIT_REPOSITORY_SELECTOR_URL
} GitRepositorySelectorMode;


GType git_repository_selector_get_type (void) G_GNUC_CONST;
GtkWidget *git_repository_selector_new (void);
GitRepositorySelectorMode git_repository_selector_get_mode (GitRepositorySelector *self);
void git_repository_selector_set_remote (GitRepositorySelector *self, 
                                         const gchar *remote);
gchar *git_repository_selector_get_repository (GitRepositorySelector *self);

G_END_DECLS

#endif /* _GIT_REMOTE_SELECTOR_H_ */
