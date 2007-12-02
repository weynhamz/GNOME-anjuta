/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2007 <jrliggett@cox.net>
 * 
 * anjuta is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * anjuta is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with anjuta.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include "svn-log-command.h"

struct _SvnLogCommandPriv
{
	gchar *path;
	GQueue *log_entry_queue;
};

G_DEFINE_TYPE (SvnLogCommand, svn_log_command, SVN_TYPE_COMMAND);

static void
svn_log_command_init (SvnLogCommand *self)
{
	self->priv = g_new0 (SvnLogCommandPriv, 1);
	self->priv->log_entry_queue = g_queue_new ();
}

static void
svn_log_command_finalize (GObject *object)
{
	SvnLogCommand *self;
	GList *current_log_entry;
	
	self = SVN_LOG_COMMAND (object);
	
	g_free (self->priv->path);
	
	current_log_entry = self->priv->log_entry_queue->head;
	
	while (current_log_entry)
	{
		svn_log_entry_destroy (current_log_entry->data);
		current_log_entry = g_list_next (current_log_entry);
	}
	
	g_queue_free (self->priv->log_entry_queue);
	g_free (self->priv);

	G_OBJECT_CLASS (svn_log_command_parent_class)->finalize (object);
}

static svn_error_t *
log_callback (void *baton,
			  apr_hash_t *changed_paths,
			  svn_revnum_t revision,
			  const char *author,
			  const char *date,
			  const char *message,
			  apr_pool_t *pool)
{
	SvnLogCommand *self;
	SvnLogEntry *log_entry;
	gchar *entry_author;
	const gchar *human_date;
	gchar *entry_date;
	apr_time_t entry_time;
	gchar *entry_message;
	
	self = SVN_LOG_COMMAND (baton);
	
	/* Libsvn docs say author, date, and message might be NULL. */
	if (author)
		entry_author = g_strdup (author);
	else 
		entry_author = g_strdup ("(none)");
	
	if (date && date[0])
	{
		svn_time_from_cstring (&entry_time, date,
							   svn_command_get_pool (SVN_COMMAND (self)));
		human_date = svn_time_to_human_cstring (entry_time,
												svn_command_get_pool (SVN_COMMAND (self)));
		entry_date = g_strdup (human_date);
	}
	else
		entry_date = g_strdup ("(none)");
	
	if (message)
		entry_message = g_strdup (message);
	else
		entry_message = g_strdup ("empty log message"); /* Emulate ViewCVS */
	
	log_entry = svn_log_entry_new (entry_author, entry_date, revision, 
								   entry_message);
	
	anjuta_async_command_lock (ANJUTA_ASYNC_COMMAND (self));
	g_queue_push_head (self->priv->log_entry_queue, log_entry);
	anjuta_async_command_unlock (ANJUTA_ASYNC_COMMAND (self));
	
	anjuta_command_notify_data_arrived (ANJUTA_COMMAND (self));
	
	return SVN_NO_ERROR;
}

static guint
svn_log_command_run (AnjutaCommand *command)
{
	SvnLogCommand *self;
	SvnCommand *svn_command;
	apr_array_header_t *log_path;
	svn_opt_revision_t peg_revision;
	svn_opt_revision_t start_revision;
	svn_opt_revision_t end_revision;
	svn_error_t *error;
	
	self = SVN_LOG_COMMAND (command);
	svn_command = SVN_COMMAND (command);
	log_path = apr_array_make (svn_command_get_pool (SVN_COMMAND (command)),
													 1, sizeof (char *));
	/* I just copied this so don't blame me... */
	(*((const char **) apr_array_push (log_path))) = self->priv->path;
	peg_revision.kind = svn_opt_revision_unspecified;
	start_revision.kind = svn_opt_revision_number;
	start_revision.value.number = 1;  /* Initial revision */
	end_revision.kind = svn_opt_revision_head;
	
	error = svn_client_log3 (log_path,
							 &peg_revision,
							 &start_revision,
							 &end_revision,
							 0,
							 FALSE,
							 FALSE,
							 log_callback,
							 self,
							 svn_command_get_client_context (svn_command),
							 svn_command_get_pool (svn_command));
	
	if (error)
	{
		svn_command_set_error (svn_command, error);
		return 1;
	}
	
	return 0;
}

static void
svn_log_command_class_init (SvnLogCommandClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	AnjutaCommandClass *command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = svn_log_command_finalize;
	command_class->run = svn_log_command_run;
}

SvnLogCommand *
svn_log_command_new (gchar *path)
{
	SvnLogCommand *self;
	
	self = g_object_new (SVN_TYPE_LOG_COMMAND, NULL);
	self->priv->path = g_strdup (path);
	
	return self;
}

void
svn_log_command_destroy (SvnLogCommand *self)
{
	g_object_unref (self);
}

GQueue *
svn_log_command_get_entry_queue (SvnLogCommand *self)
{
	return self->priv->log_entry_queue;
}
