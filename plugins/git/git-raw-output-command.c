/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2008 <jrliggett@cox.net>
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

#include "git-raw-output-command.h"

struct _GitRawOutputCommandPriv
{
	GQueue *output_queue;
};

G_DEFINE_TYPE (GitRawOutputCommand, git_raw_output_command, GIT_TYPE_COMMAND);

static void
git_raw_output_command_init (GitRawOutputCommand *self)
{
	self->priv = g_new0 (GitRawOutputCommandPriv, 1);
	self->priv->output_queue = g_queue_new ();
}

static void
git_raw_output_command_finalize (GObject *object)
{
	GitRawOutputCommand *self;
	GList *current_output;
	
	self = GIT_RAW_OUTPUT_COMMAND (object);
	
	current_output = self->priv->output_queue->head;
	
	while (current_output)
	{
		g_free (current_output->data);
		current_output = g_list_next (current_output);
	}
	
	g_queue_free (self->priv->output_queue);
	g_free (self->priv);

	G_OBJECT_CLASS (git_raw_output_command_parent_class)->finalize (object);
}

static void
git_raw_output_command_handle_output (GitCommand *git_command, 
									 const gchar *output)
{
	GitRawOutputCommand *self;
	
	self = GIT_RAW_OUTPUT_COMMAND (git_command);
	
	g_queue_push_tail (self->priv->output_queue, g_strdup (output));
	anjuta_command_notify_data_arrived (ANJUTA_COMMAND (git_command));
}

static void
git_raw_output_command_class_init (GitRawOutputCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GitCommandClass* parent_class = GIT_COMMAND_CLASS (klass);

	object_class->finalize = git_raw_output_command_finalize;
	parent_class->output_handler = git_raw_output_command_handle_output;
}

GQueue *
git_raw_output_command_get_output (GitRawOutputCommand *self)
{
	return self->priv->output_queue;
}
