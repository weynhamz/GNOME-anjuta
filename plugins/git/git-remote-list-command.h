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

#ifndef _GIT_REMOTE_LIST_COMMAND_H_
#define _GIT_REMOTE_LIST_COMMAND_H_

#include <glib-object.h>
#include "git-raw-output-command.h"

G_BEGIN_DECLS

#define GIT_TYPE_REMOTE_LIST_COMMAND             (git_remote_list_command_get_type ())
#define GIT_REMOTE_LIST_COMMAND(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIT_TYPE_REMOTE_LIST_COMMAND, GitRemoteListCommand))
#define GIT_REMOTE_LIST_COMMAND_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GIT_TYPE_REMOTE_LIST_COMMAND, GitRemoteListCommandClass))
#define GIT_IS_REMOTE_LIST_COMMAND(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIT_TYPE_REMOTE_LIST_COMMAND))
#define GIT_IS_REMOTE_LIST_COMMAND_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIT_TYPE_REMOTE_LIST_COMMAND))
#define GIT_REMOTE_LIST_COMMAND_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GIT_TYPE_REMOTE_LIST_COMMAND, GitRemoteListCommandClass))

typedef struct _GitRemoteListCommandClass GitRemoteListCommandClass;
typedef struct _GitRemoteListCommand GitRemoteListCommand;

struct _GitRemoteListCommandClass
{
	GitRawOutputCommandClass parent_class;
};

struct _GitRemoteListCommand
{
	GitRawOutputCommand parent_instance;
};

GType git_remote_list_command_get_type (void) G_GNUC_CONST;
GitRemoteListCommand *git_remote_list_command_new (const gchar *working_directory);

G_END_DECLS

#endif /* _GIT_REMOTE_LIST_COMMAND_H_ */
