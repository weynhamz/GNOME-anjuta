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

#ifndef _GIT_PULL_COMMAND_H_
#define _GIT_PULL_COMMAND_H_

#include <glib-object.h>
#include "git-command.h"

G_BEGIN_DECLS

#define GIT_TYPE_PULL_COMMAND             (git_pull_command_get_type ())
#define GIT_PULL_COMMAND(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIT_TYPE_PULL_COMMAND, GitPullCommand))
#define GIT_PULL_COMMAND_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GIT_TYPE_PULL_COMMAND, GitPullCommandClass))
#define GIT_IS_PULL_COMMAND(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIT_TYPE_PULL_COMMAND))
#define GIT_IS_PULL_COMMAND_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIT_TYPE_PULL_COMMAND))
#define GIT_PULL_COMMAND_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GIT_TYPE_PULL_COMMAND, GitPullCommandClass))

typedef struct _GitPullCommandClass GitPullCommandClass;
typedef struct _GitPullCommand GitPullCommand;
typedef struct _GitPullCommandPriv GitPullCommandPriv;

struct _GitPullCommandClass
{
	GitCommandClass parent_class;
};

struct _GitPullCommand
{
	GitCommand parent_instance;
	
	GitPullCommandPriv *priv;
};

GType git_pull_command_get_type (void) G_GNUC_CONST;
GitPullCommand *git_pull_command_new (const gchar *working_directory,
									  const gchar *url, 
									  gboolean no_commit, gboolean squash, 
									  gboolean commit_fast_forward,
									  gboolean append_fetch_data, 
									  gboolean force, gboolean no_follow_tags);

G_END_DECLS

#endif /* _GIT_PULL_COMMAND_H_ */
