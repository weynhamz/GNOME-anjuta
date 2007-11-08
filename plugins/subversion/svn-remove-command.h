/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2007 <jrliggett@cox.net>
 *
 * Portions based on the original Subversion plugin 
 * Copyright (C) Johannes Schmid 2005 
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

#ifndef _SVN_REMOVE_COMMAND_H_
#define _SVN_REMOVE_COMMAND_H_

#include <glib-object.h>
#include "svn-command.h"

G_BEGIN_DECLS

#define SVN_TYPE_REMOVE_COMMAND             (svn_remove_command_get_type ())
#define SVN_REMOVE_COMMAND(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SVN_TYPE_REMOVE_COMMAND, SvnRemoveCommand))
#define SVN_REMOVE_COMMAND_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), SVN_TYPE_REMOVE_COMMAND, SvnRemoveCommandClass))
#define SVN_IS_REMOVE_COMMAND(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SVN_TYPE_REMOVE_COMMAND))
#define SVN_IS_REMOVE_COMMAND_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SVN_TYPE_REMOVE_COMMAND))
#define SVN_REMOVE_COMMAND_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SVN_TYPE_REMOVE_COMMAND, SvnRemoveCommandClass))

typedef struct _SvnRemoveCommandClass SvnRemoveCommandClass;
typedef struct _SvnRemoveCommand SvnRemoveCommand;
typedef struct _SvnRemoveCommandPriv SvnRemoveCommandPriv;

struct _SvnRemoveCommandClass
{
	SvnCommandClass parent_class;
};

struct _SvnRemoveCommand
{
	SvnCommand parent_instance;
	
	SvnRemoveCommandPriv *priv;
};

GType svn_remove_command_get_type (void) G_GNUC_CONST;
SvnRemoveCommand * svn_remove_command_new (gchar *path, gchar *log_message, 
										   gboolean force);
void svn_remove_command_destroy (SvnRemoveCommand *self);
gchar *svn_remove_command_get_path (SvnRemoveCommand *self);

G_END_DECLS

#endif /* _SVN_REMOVE_COMMAND_H_ */
