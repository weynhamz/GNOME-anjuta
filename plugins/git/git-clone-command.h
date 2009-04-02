/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Carl-Anton Ingmarsson 2009 <ca.ingmarsson@gmail.com>
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

#ifndef _GIT_CLONE_COMMAND_H_
#define _GIT_CLONE_COMMAND_H_

#include <glib-object.h>
#include "git-command.h"

G_BEGIN_DECLS

#define GIT_TYPE_CLONE_COMMAND             (git_clone_command_get_type ())
#define GIT_CLONE_COMMAND(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIT_TYPE_CLONE_COMMAND, GitCloneCommand))
#define GIT_CLONE_COMMAND_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GIT_TYPE_CLONE_COMMAND, GitCloneCommandClass))
#define GIT_IS_CLONE_COMMAND(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIT_TYPE_CLONE_COMMAND))
#define GIT_IS_CLONE_COMMAND_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIT_TYPE_CLONE_COMMAND))
#define GIT_CLONE_COMMAND_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GIT_TYPE_CLONE_COMMAND, GitCloneCommandClass))

typedef struct _GitCloneCommandClass GitCloneCommandClass;
typedef struct _GitCloneCommand GitCloneCommand;

struct _GitCloneCommandClass
{
	GitCommandClass parent_class;
};

struct _GitCloneCommand
{
	GitCommand parent_instance;
};

GType git_clone_command_get_type (void) G_GNUC_CONST;

GitCloneCommand *git_clone_command_new (const gchar *uri, const gchar *working_directory,
                                        const gchar *repository_name);

G_END_DECLS

#endif /* _GIT_CLONE_COMMAND_H_ */
