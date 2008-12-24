/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    action.h
    Copyright (C) 2004 Sebastien Granjoux

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef __ACTION_H__
#define __ACTION_H__

#include <glib.h>

typedef struct _NPWAction NPWAction;

typedef enum {
	NPW_RUN_ACTION,
	NPW_OPEN_ACTION
} NPWActionType;

NPWAction* npw_action_new_file (const gchar *file);
NPWAction* npw_action_new_command (const gchar *command);

void npw_action_free (NPWAction* action);

NPWActionType npw_action_get_type (const NPWAction* action);
const gchar* npw_action_get_command (const NPWAction* action);
const gchar* npw_action_get_file (const NPWAction* action);

#endif
