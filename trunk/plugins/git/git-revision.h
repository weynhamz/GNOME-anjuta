/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * git-command-test
 * 
 * git-command-test is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * git-command-test is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _GIT_REVISION_H_
#define _GIT_REVISION_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define GIT_TYPE_REVISION             (git_revision_get_type ())
#define GIT_REVISION(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIT_TYPE_REVISION, GitRevision))
#define GIT_REVISION_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GIT_TYPE_REVISION, GitRevisionClass))
#define GIT_IS_REVISION(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIT_TYPE_REVISION))
#define GIT_IS_REVISION_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIT_TYPE_REVISION))
#define GIT_REVISION_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GIT_TYPE_REVISION, GitRevisionClass))

typedef struct _GitRevisionClass GitRevisionClass;
typedef struct _GitRevision GitRevision;
typedef struct _GitRevisionPriv GitRevisionPriv;

struct _GitRevisionClass
{
	GObjectClass parent_class;
};

struct _GitRevision
{
	GObject parent_instance;
	
	GitRevisionPriv *priv;
};

GType git_revision_get_type (void) G_GNUC_CONST;
GitRevision *git_revision_new (void);
void git_revision_set_sha (GitRevision *self, const gchar *sha);
gchar *git_revision_get_sha (GitRevision *self);
gchar *git_revision_get_short_sha (GitRevision *self);
void git_revision_set_author (GitRevision *self, const gchar *author);
gchar *git_revision_get_author (GitRevision *self);
void git_revision_set_short_log (GitRevision *self, const gchar *short_log);
gchar *git_revision_get_short_log (GitRevision *self);
void git_revision_set_date (GitRevision *self, time_t unix_time);
gchar *git_revision_get_formatted_date (GitRevision *self);
void git_revision_add_child (GitRevision *self, GitRevision *child);
GList *git_revision_get_children (GitRevision *self);
void git_revision_set_has_parents (GitRevision *self, gboolean has_parents);
gboolean git_revision_has_parents (GitRevision *self);

G_END_DECLS

#endif /* _GIT_REVISION_H_ */
