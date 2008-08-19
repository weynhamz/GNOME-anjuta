/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-git
 * Copyright (C) James Liggett 2008 <jrliggett@cox.net>
 * 
 * anjuta-git is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * anjuta-git is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with anjuta-git.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef _GIT_IGNORE_COMMAND_H_
#define _GIT_IGNORE_COMMAND_H_

#include <glib-object.h>
#include <gio/gio.h>
#include "git-file-command.h"

G_BEGIN_DECLS

#define GIT_TYPE_IGNORE_COMMAND             (git_ignore_command_get_type ())
#define GIT_IGNORE_COMMAND(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIT_TYPE_IGNORE_COMMAND, GitIgnoreCommand))
#define GIT_IGNORE_COMMAND_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GIT_TYPE_IGNORE_COMMAND, GitIgnoreCommandClass))
#define GIT_IS_IGNORE_COMMAND(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIT_TYPE_IGNORE_COMMAND))
#define GIT_IS_IGNORE_COMMAND_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIT_TYPE_IGNORE_COMMAND))
#define GIT_IGNORE_COMMAND_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GIT_TYPE_IGNORE_COMMAND, GitIgnoreCommandClass))

typedef struct _GitIgnoreCommandClass GitIgnoreCommandClass;
typedef struct _GitIgnoreCommand GitIgnoreCommand;
typedef struct _GitIgnoreCommandPriv GitIgnoreCommandPriv;

struct _GitIgnoreCommandClass
{
	GitFileCommandClass parent_class;
};

struct _GitIgnoreCommand
{
	GitFileCommand parent_instance;
	
	GitIgnoreCommandPriv *priv;
};

GType git_ignore_command_get_type (void) G_GNUC_CONST;
GitIgnoreCommand *git_ignore_command_new_path (const gchar *working_directory, 
											   const gchar *path);
GitIgnoreCommand *git_ignore_command_new_list (const gchar *working_directory,
											   GList *path_list);

G_END_DECLS

#endif /* _GIT_IGNORE_COMMAND_H_ */
