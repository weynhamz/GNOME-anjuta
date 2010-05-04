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

#ifndef _ANJUTA_COMMAND_QUEUE_H_
#define _ANJUTA_COMMAND_QUEUE_H_

#include <glib-object.h>
#include "anjuta-command.h"

G_BEGIN_DECLS

#define ANJUTA_TYPE_COMMAND_QUEUE             (anjuta_command_queue_get_type ())
#define ANJUTA_COMMAND_QUEUE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_COMMAND_QUEUE, AnjutaCommandQueue))
#define ANJUTA_COMMAND_QUEUE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_COMMAND_QUEUE, AnjutaCommandQueueClass))
#define ANJUTA_IS_COMMAND_QUEUE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_COMMAND_QUEUE))
#define ANJUTA_IS_COMMAND_QUEUE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_COMMAND_QUEUE))
#define ANJUTA_COMMAND_QUEUE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_COMMAND_QUEUE, AnjutaCommandQueueClass))

typedef struct _AnjutaCommandQueueClass AnjutaCommandQueueClass;
typedef struct _AnjutaCommandQueue AnjutaCommandQueue;
typedef struct _AnjutaCommandQueuePriv AnjutaCommandQueuePriv;

struct _AnjutaCommandQueueClass
{
	GObjectClass parent_class;

	/* Signals */
	void (*finished) (AnjutaCommandQueue *queue);
};

struct _AnjutaCommandQueue
{
	GObject parent_instance;

	AnjutaCommandQueuePriv *priv;
};

typedef enum
{
	ANJUTA_COMMAND_QUEUE_EXECUTE_AUTOMATIC,
	ANJUTA_COMMAND_QUEUE_EXECUTE_MANUAL
} AnjutaCommandQueueExecuteMode;

GType anjuta_command_queue_get_type (void) G_GNUC_CONST;
AnjutaCommandQueue * anjuta_command_queue_new (AnjutaCommandQueueExecuteMode mode);
void anjuta_command_queue_push (AnjutaCommandQueue *self, 
                                AnjutaCommand *command);
gboolean anjuta_command_queue_start (AnjutaCommandQueue *self);

G_END_DECLS

#endif /* _ANJUTA_COMMAND_QUEUE_H_ */
