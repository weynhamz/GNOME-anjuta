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

#ifndef _SVN_MERGE_COMMAND_H_
#define _SVN_MERGE_COMMAND_H_

#include <glib-object.h>
#include "svn-command.h"

G_BEGIN_DECLS

#define SVN_TYPE_MERGE_COMMAND             (svn_merge_command_get_type ())
#define SVN_MERGE_COMMAND(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SVN_TYPE_MERGE_COMMAND, SvnMergeCommand))
#define SVN_MERGE_COMMAND_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), SVN_TYPE_MERGE_COMMAND, SvnMergeCommandClass))
#define SVN_IS_MERGE_COMMAND(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SVN_TYPE_MERGE_COMMAND))
#define SVN_IS_MERGE_COMMAND_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SVN_TYPE_MERGE_COMMAND))
#define SVN_MERGE_COMMAND_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SVN_TYPE_MERGE_COMMAND, SvnMergeCommandClass))

typedef struct _SvnMergeCommandClass SvnMergeCommandClass;
typedef struct _SvnMergeCommand SvnMergeCommand;
typedef struct _SvnMergeCommandPriv SvnMergeCommandPriv;

struct _SvnMergeCommandClass
{
	SvnCommandClass parent_class;
};

struct _SvnMergeCommand
{
	SvnCommand parent_instance;
	
	SvnMergeCommandPriv *priv;
};

enum
{
	SVN_MERGE_REVISION_HEAD = -1
};

GType svn_merge_command_get_type (void) G_GNUC_CONST;
SvnMergeCommand *svn_merge_command_new (gchar *path1, 
										gchar *path2, 
									    glong start_revision, 
										glong end_revision, 
										gchar *target_path, 
										gboolean recursive, 
									    gboolean ignore_ancestry, 
										gboolean force, 
									    gboolean dry_run);

void svn_merge_command_destroy (SvnMergeCommand *self);

G_END_DECLS

#endif /* _SVN_MERGE_COMMAND_H_ */
