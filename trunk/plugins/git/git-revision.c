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

#include "git-revision.h"

struct _GitRevisionPriv
{
	gchar *sha;
	gchar *author;
	gchar *date;
	gchar *short_log;
	GList *children;
	gboolean has_parents;
};

G_DEFINE_TYPE (GitRevision, git_revision, G_TYPE_OBJECT);

static void
git_revision_init (GitRevision *self)
{
	self->priv = g_new0 (GitRevisionPriv, 1);
}

static void
git_revision_finalize (GObject *object)
{
	GitRevision *self;
	GList *current_child;
	
	self = GIT_REVISION (object);
	
	g_free (self->priv->sha);
	g_free (self->priv->author);
	g_free (self->priv->date);
	g_free (self->priv->short_log);
	
	current_child = self->priv->children;
	
	while (current_child)
	{
		g_object_unref (current_child->data);
		current_child = g_list_next (current_child);
	}
	
	g_list_free (self->priv->children);
	g_free (self->priv);

	G_OBJECT_CLASS (git_revision_parent_class)->finalize (object);
}

static void
git_revision_class_init (GitRevisionClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = git_revision_finalize;
}

GitRevision *
git_revision_new (void)
{
	return g_object_new (GIT_TYPE_REVISION, NULL);
}

void
git_revision_set_sha (GitRevision *self, const gchar *sha)
{
	g_free (self->priv->sha);
	self->priv->sha = g_strdup (sha);
}

gchar *
git_revision_get_sha (GitRevision *self)
{
	return g_strdup (self->priv->sha);
}

gchar *
git_revision_get_short_sha (GitRevision *self)
{
	return g_strndup (self->priv->sha, 7);
}

void
git_revision_set_author (GitRevision *self, const gchar *author)
{
	g_free (self->priv->author);
	self->priv->author = g_strdup (author);
}

gchar *
git_revision_get_author (GitRevision *self)
{
	return g_strdup (self->priv->author);
}

void
git_revision_set_short_log (GitRevision *self, const gchar *short_log)
{
	g_free (self->priv->short_log);
	self->priv->short_log = g_strdup (short_log);
	
	g_strchug (self->priv->short_log);
}

gchar *
git_revision_get_short_log (GitRevision *self)
{
	return g_strdup (self->priv->short_log);
}

static const gchar *
git_revision_get_time_format (struct tm *revision_time)
{
	struct tm *tm;
	time_t t1, t2;

	t1 = mktime ((struct tm *) revision_time);

	/* check whether it's ahead in time */
	time (&t2);
	if (t1 > t2) 
	{
		return "%c";
	}

	/* check whether it's as fresh as today's bread */
	t2 = time (NULL);
	tm = localtime (&t2);
	tm->tm_sec = tm->tm_min = tm->tm_hour = 0;
	t2 = mktime (tm);

	if (t1 > t2) 
	{
		/* TRANSLATORS: it's a strftime format string */
		return "%I:%M %p";
	}

	/* check whether it's older than a week */
	t2 = time (NULL);
	tm = localtime (&t2);
	tm->tm_sec = tm->tm_min = tm->tm_hour = 0;
	t2 = mktime (tm);

	t2 -= 60 * 60 * 24 * 6; /* substract 1 week */

	if (t1 > t2) 
	{
		/* TRANSLATORS: it's a strftime format string */
		return "%a %I:%M %p";
	}

	/* check whether it's more recent than the new year hangover */
	t2 = time (NULL);
	tm = localtime (&t2);
	tm->tm_sec = tm->tm_min = tm->tm_hour = tm->tm_mon = 0;
	tm->tm_mday = 1;
	t2 = mktime (tm);

	if (t1 > t2) 
	{
		/* TRANSLATORS: it's a strftime format string */
		return "%b %d %I:%M %p";
	}

	/* it's older */
	/* TRANSLATORS: it's a strftime format string */
	return "%b %d %Y";
}

void
git_revision_set_date (GitRevision *self, time_t unix_time)
{
	struct tm time;
	const gchar *format;
	gchar buffer[256];
	
	localtime_r (&unix_time, &time);
	format = git_revision_get_time_format (&time);
	strftime (buffer, sizeof (buffer), format, &time);
	
	g_free (self->priv->date);
	self->priv->date = g_strdup (buffer);
}

gchar *
git_revision_get_formatted_date (GitRevision *self)
{
	return g_strdup (self->priv->date);
}

void
git_revision_add_child (GitRevision *self, GitRevision *child)
{
	self->priv->children = g_list_prepend (self->priv->children,
										  g_object_ref (child));
	git_revision_set_has_parents (child, TRUE);
}

GList *
git_revision_get_children (GitRevision *self)
{
	return self->priv->children;
}

void
git_revision_set_has_parents (GitRevision *self, gboolean has_parents)
{
	self->priv->has_parents = has_parents;
}

gboolean
git_revision_has_parents (GitRevision *self)
{
	return self->priv->has_parents;
}
