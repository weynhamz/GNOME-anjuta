/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2009 <jrliggett@cox.net>
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

#ifndef _GIT_STASH_SAVE_COMMAND_H_
#define _GIT_STASH_SAVE_COMMAND_H_

#include <glib-object.h>
#include "git-command.h"

G_BEGIN_DECLS

#define GIT_TYPE_STASH_SAVE_COMMAND             (git_stash_save_command_get_type ())
#define GIT_STASH_SAVE_COMMAND(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIT_TYPE_STASH_SAVE_COMMAND, GitStashSaveCommand))
#define GIT_STASH_SAVE_COMMAND_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GIT_TYPE_STASH_SAVE_COMMAND, GitStashSaveCommandClass))
#define GIT_IS_STASH_SAVE_COMMAND(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIT_TYPE_STASH_SAVE_COMMAND))
#define GIT_IS_STASH_SAVE_COMMAND_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIT_TYPE_STASH_SAVE_COMMAND))
#define GIT_STASH_SAVE_COMMAND_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GIT_TYPE_STASH_SAVE_COMMAND, GitStashSaveCommandClass))

typedef struct _GitStashSaveCommandClass GitStashSaveCommandClass;
typedef struct _GitStashSaveCommand GitStashSaveCommand;
typedef struct _GitStashSaveCommandPriv GitStashSaveCommandPriv;

struct _GitStashSaveCommandClass
{
	GitCommandClass parent_class;
};

struct _GitStashSaveCommand
{
	GitCommand parent_instance;

	GitStashSaveCommandPriv *priv;
};

GType git_stash_save_command_get_type (void) G_GNUC_CONST;
GitStashSaveCommand *git_stash_save_command_new (const gchar *working_directory, 
												 gboolean keep_index, 
												 const gchar *message);

G_END_DECLS

#endif /* _GIT_STASH_SAVE_COMMAND_H_ */
