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

#ifndef _ANJUTA_SYNC_COMMAND_H_
#define _ANJUTA_SYNC_COMMAND_H_

#include <glib-object.h>
#include "anjuta-command.h"

G_BEGIN_DECLS

#define ANJUTA_TYPE_SYNC_COMMAND             (anjuta_sync_command_get_type ())
#define ANJUTA_SYNC_COMMAND(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_SYNC_COMMAND, SyncCommand))
#define ANJUTA_SYNC_COMMAND_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_SYNC_COMMAND, SyncCommandClass))
#define ANJUTA_IS_SYNC_COMMAND(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_SYNC_COMMAND))
#define ANJUTA_IS_SYNC_COMMAND_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_SYNC_COMMAND))
#define ANJUTA_SYNC_COMMAND_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_SYNC_COMMAND, SyncCommandClass))

typedef struct _AnjutaSyncCommandClass AnjutaSyncCommandClass;
typedef struct _AnjutaSyncCommand AnjutaSyncCommand;

struct _AnjutaSyncCommandClass
{
	AnjutaCommandClass parent_class;
};

struct _AnjutaSyncCommand
{
	AnjutaCommand parent_instance;
};

GType anjuta_sync_command_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* _ANJUTA_SYNC_COMMAND_H_ */
