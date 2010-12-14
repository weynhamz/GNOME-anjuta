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

#ifndef _GIT_LOG_DATA_COMMAND_H_
#define _GIT_LOG_DATA_COMMAND_H_

#include <glib-object.h>
#include <stdlib.h>
#include <libanjuta/anjuta-async-command.h>
#include "git-revision.h"

G_BEGIN_DECLS

#define GIT_TYPE_LOG_DATA_COMMAND             (git_log_data_command_get_type ())
#define GIT_LOG_DATA_COMMAND(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIT_TYPE_LOG_DATA_COMMAND, GitLogDataCommand))
#define GIT_LOG_DATA_COMMAND_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GIT_TYPE_LOG_DATA_COMMAND, GitLogDataCommandClass))
#define GIT_IS_LOG_DATA_COMMAND(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIT_TYPE_LOG_DATA_COMMAND))
#define GIT_IS_LOG_DATA_COMMAND_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIT_TYPE_LOG_DATA_COMMAND))
#define GIT_LOG_DATA_COMMAND_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GIT_TYPE_LOG_DATA_COMMAND, GitLogDataCommandClass))

typedef struct _GitLogDataCommandClass GitLogDataCommandClass;
typedef struct _GitLogDataCommand GitLogDataCommand;
typedef struct _GitLogDataCommandPriv GitLogDataCommandPriv;

struct _GitLogDataCommandClass
{
	AnjutaAsyncCommandClass parent_class;
};

struct _GitLogDataCommand
{
	AnjutaAsyncCommand parent_instance;

	GitLogDataCommandPriv *priv;
};

GType git_log_data_command_get_type (void) G_GNUC_CONST;
GitLogDataCommand *git_log_data_command_new (void);
GQueue *git_log_data_command_get_output (GitLogDataCommand *self);
void git_log_data_command_push_line (GitLogDataCommand *self, 
                                     const gchar *line);

G_END_DECLS

#endif /* _GIT_LOG_DATA_COMMAND_H_ */
