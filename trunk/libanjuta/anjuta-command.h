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

#ifndef _ANJUTA_COMMAND_H_
#define _ANJUTA_COMMAND_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define ANJUTA_TYPE_COMMAND             (anjuta_command_get_type ())
#define ANJUTA_COMMAND(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_COMMAND, AnjutaCommand))
#define ANJUTA_COMMAND_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_COMMAND, AnjutaCommandClass))
#define ANJUTA_IS_COMMAND(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_COMMAND))
#define ANJUTA_IS_COMMAND_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_COMMAND))
#define ANJUTA_COMMAND_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_COMMAND, AnjutaCommandClass))

typedef struct _AnjutaCommandClass AnjutaCommandClass;
typedef struct _AnjutaCommand AnjutaCommand;
typedef struct _AnjutaCommandPriv AnjutaCommandPriv;

struct _AnjutaCommandClass
{
	GObjectClass parent_class;
	
	/* Virtual Methods */
	guint (*run) (AnjutaCommand *self);
	void (*start) (AnjutaCommand *self);
	void (*notify_data_arrived) (AnjutaCommand *self);
	void (*notify_complete) (AnjutaCommand *self, guint return_code);
	void (*set_error_message) (AnjutaCommand *self, gchar *error_message);
	gchar * (*get_error_message) (AnjutaCommand *self);

};

struct _AnjutaCommand
{
	GObject parent_instance;
	
	AnjutaCommandPriv *priv;
};

GType anjuta_command_get_type (void) G_GNUC_CONST;

void anjuta_command_start (AnjutaCommand *self);
void anjuta_command_notify_data_arrived (AnjutaCommand *self);
void anjuta_command_notify_complete (AnjutaCommand *self, guint return_code);

void anjuta_command_set_error_message (AnjutaCommand *self, gchar *error_message);
gchar *anjuta_command_get_error_message (AnjutaCommand *self);

G_END_DECLS

#endif /* _ANJUTA_COMMAND_H_ */
