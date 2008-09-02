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

#include "anjuta-sync-command.h"

/**
 * SECTION: anjuta-sync-command
 * @short_description: #AnjutaCommand subclass that serves as the base for 
 *					   commands that run synchronously.
 * @include: libanjuta/anjuta-sync-command.h
 *
 * #AnjutaSyncCommand allows plugins to abstract the work of tasks that do not
 * need to be run in another thread. This class can provide a base for 
 * abstratraction between client code and asynchronous facilities such as
 * #AnjutaLauncher or GIO, and is especially useful when complicated tasks
 * are being performed. 
 *
 * #AnjutaSyncCommand simply calls ::run directly from ::start, and emits the 
 * command-finished signal as soon as it returns. 
 *
 * For an example of how #AnjutaSyncCommand is used, see the Git plugin.
 */

G_DEFINE_TYPE (AnjutaSyncCommand, anjuta_sync_command, ANJUTA_TYPE_COMMAND);

static void
anjuta_sync_command_init (AnjutaSyncCommand *self)
{

}

static void
anjuta_sync_command_finalize (GObject *object)
{
	G_OBJECT_CLASS (anjuta_sync_command_parent_class)->finalize (object);
}

static void
start_command (AnjutaCommand *command)
{
	guint return_code;
	
	return_code = ANJUTA_COMMAND_GET_CLASS (command)->run (command);
	anjuta_command_notify_complete (command, return_code);
}

static void
notify_data_arrived (AnjutaCommand *command)
{
	g_signal_emit_by_name (command, "data-arrived");
}

static void
notify_complete (AnjutaCommand *command, guint return_code)
{
	g_signal_emit_by_name (command, "command-finished", 
						   return_code);
}

static void
notify_progress (AnjutaCommand *command, gfloat progress)
{
	g_signal_emit_by_name (command, "progress", progress);
}

static void
anjuta_sync_command_class_init (AnjutaSyncCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaCommandClass* parent_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = anjuta_sync_command_finalize;
	
	parent_class->start = start_command;
	parent_class->notify_data_arrived = notify_data_arrived;
	parent_class->notify_complete = notify_complete;
	parent_class->notify_progress = notify_progress;
}
