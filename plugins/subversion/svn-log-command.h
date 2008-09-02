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

#ifndef _SVN_LOG_COMMAND_H_
#define _SVN_LOG_COMMAND_H_

#include <glib-object.h>
#include <svn_time.h>
#include "svn-command.h"
#include "svn-log-entry.h"

G_BEGIN_DECLS

#define SVN_TYPE_LOG_COMMAND             (svn_log_command_get_type ())
#define SVN_LOG_COMMAND(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SVN_TYPE_LOG_COMMAND, SvnLogCommand))
#define SVN_LOG_COMMAND_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), SVN_TYPE_LOG_COMMAND, SvnLogCommandClass))
#define SVN_IS_LOG_COMMAND(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SVN_TYPE_LOG_COMMAND))
#define SVN_IS_LOG_COMMAND_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SVN_TYPE_LOG_COMMAND))
#define SVN_LOG_COMMAND_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SVN_TYPE_LOG_COMMAND, SvnLogCommandClass))

typedef struct _SvnLogCommandClass SvnLogCommandClass;
typedef struct _SvnLogCommand SvnLogCommand;
typedef struct _SvnLogCommandPriv SvnLogCommandPriv;

struct _SvnLogCommandClass
{
	SvnCommandClass parent_class;
};

struct _SvnLogCommand
{
	SvnCommand parent_instance;
	
	SvnLogCommandPriv *priv;
};

GType svn_log_command_get_type (void) G_GNUC_CONST;
SvnLogCommand *svn_log_command_new (const gchar *path);
void svn_log_command_destroy (SvnLogCommand *self);
GQueue *svn_log_command_get_entry_queue (SvnLogCommand *self);

G_END_DECLS

#endif /* _SVN_LOG_COMMAND_H_ */
