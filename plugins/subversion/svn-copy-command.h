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

#ifndef _SVN_COPY_COMMAND_H_
#define _SVN_COPY_COMMAND_H_

#include <glib-object.h>
#include <svn_path.h>
#include "svn-command.h"

G_BEGIN_DECLS

#define SVN_TYPE_COPY_COMMAND             (svn_copy_command_get_type ())
#define SVN_COPY_COMMAND(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SVN_TYPE_COPY_COMMAND, SvnCopyCommand))
#define SVN_COPY_COMMAND_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), SVN_TYPE_COPY_COMMAND, SvnCopyCommandClass))
#define SVN_IS_COPY_COMMAND(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SVN_TYPE_COPY_COMMAND))
#define SVN_IS_COPY_COMMAND_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SVN_TYPE_COPY_COMMAND))
#define SVN_COPY_COMMAND_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SVN_TYPE_COPY_COMMAND, SvnCopyCommandClass))

typedef struct _SvnCopyCommandClass SvnCopyCommandClass;
typedef struct _SvnCopyCommand SvnCopyCommand;
typedef struct _SvnCopyCommandPriv SvnCopyCommandPriv;

struct _SvnCopyCommandClass
{
	SvnCommandClass parent_class;
};

struct _SvnCopyCommand
{
	SvnCommand parent_instance;
	
	SvnCopyCommandPriv *priv;
};

enum
{
	SVN_COPY_REVISION_WORKING = 0,
	SVN_COPY_REVISION_HEAD = -1
};

GType svn_copy_command_get_type (void) G_GNUC_CONST;
SvnCopyCommand *svn_copy_command_new (const gchar *source_path, glong source_revision, 
									  const gchar *dest_path, const gchar *log_message);
void svn_copy_command_destroy (SvnCopyCommand *self);

G_END_DECLS

#endif /* _SVN_COPY_COMMAND_H_ */
