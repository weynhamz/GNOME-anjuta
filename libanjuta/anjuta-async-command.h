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

#ifndef _ANJUTA_ASYNC_COMMAND_H_
#define _ANJUTA_ASYNC_COMMAND_H_

#include <glib-object.h>
#include "anjuta-command.h"

G_BEGIN_DECLS

#define ANJUTA_TYPE_ASYNC_COMMAND             (anjuta_async_command_get_type ())
#define ANJUTA_ASYNC_COMMAND(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_ASYNC_COMMAND, AnjutaAsyncCommand))
#define ANJUTA_ASYNC_COMMAND_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_ASYNC_COMMAND, AnjutaAsyncCommandClass))
#define IS_ANJUTA_ASYNC_COMMAND(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_ASYNC_COMMAND))
#define IS_ANJUTA_ASYNC_COMMAND_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_ASYNC_COMMAND))
#define ANJUTA_ASYNC_COMMAND_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_ASYNC_COMMAND, AnjutaAsyncCommandClass))

typedef struct _AnjutaAsyncCommandClass AnjutaAsyncCommandClass;
typedef struct _AnjutaAsyncCommand AnjutaAsyncCommand;
typedef struct _AnjutaAsyncCommandPriv AnjutaAsyncCommandPriv;

struct _AnjutaAsyncCommandClass
{
	AnjutaCommandClass parent_class;
};

struct _AnjutaAsyncCommand
{
	AnjutaCommand parent_instance;
	
	AnjutaAsyncCommandPriv *priv;
};

GType anjuta_async_command_get_type (void) G_GNUC_CONST;

void anjuta_async_command_set_error_message (AnjutaCommand *command, 
											 gchar *error_message);
gchar *anjuta_async_command_get_error_message (AnjutaCommand *command);

void anjuta_async_command_lock (AnjutaAsyncCommand *self);
void anjuta_async_command_unlock (AnjutaAsyncCommand *self);

G_END_DECLS

#endif /* _ANJUTA_ASYNC_COMMAND_H_ */
