/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * git-command-test
 * Copyright (C) James Liggett 2008 <jrliggett@cox.net>
 * 
 * git-command-test is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * git-command-test is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with git-command-test.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef _GIT_LOG_MESSAGE_COMMAND_H_
#define _GIT_LOG_MESSAGE_COMMAND_H_

#include <glib-object.h>
#include <stdlib.h>
#include "git-command.h"
#include "git-revision.h"

G_BEGIN_DECLS

#define GIT_TYPE_LOG_MESSAGE_COMMAND             (git_log_message_command_get_type ())
#define GIT_LOG_MESSAGE_COMMAND(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIT_TYPE_LOG_MESSAGE_COMMAND, GitLogMessageCommand))
#define GIT_LOG_MESSAGE_COMMAND_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GIT_TYPE_LOG_MESSAGE_COMMAND, GitLogMessageCommandClass))
#define GIT_IS_LOG_MESSAGE_COMMAND(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIT_TYPE_LOG_MESSAGE_COMMAND))
#define GIT_IS_LOG_MESSAGE_COMMAND_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIT_TYPE_LOG_MESSAGE_COMMAND))
#define GIT_LOG_MESSAGE_COMMAND_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GIT_TYPE_LOG_MESSAGE_COMMAND, GitLogMessageCommandClass))

typedef struct _GitLogMessageCommandClass GitLogMessageCommandClass;
typedef struct _GitLogMessageCommand GitLogMessageCommand;
typedef struct _GitLogMessageCommandPriv GitLogMessageCommandPriv;

struct _GitLogMessageCommandClass
{
	GitCommandClass parent_class;
};

struct _GitLogMessageCommand
{
	GitCommand parent_instance;
	
	GitLogMessageCommandPriv *priv;
};

GType git_log_message_command_get_type (void) G_GNUC_CONST;
GitLogMessageCommand *git_log_message_command_new (const gchar *working_directory,
												   const gchar *sha);
gchar *git_log_message_command_get_message (GitLogMessageCommand *self);

G_END_DECLS

#endif /* _GIT_LOG_MESSAGE_COMMAND_H_ */
