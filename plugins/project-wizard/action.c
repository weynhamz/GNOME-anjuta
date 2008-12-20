/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    action.c
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

/*
 * Actions data read in .wiz file
 * 
 * It's just a list containing all data
 *---------------------------------------------------------------------------*/

#include <config.h>

#include "action.h"

#include <glib.h>

/*---------------------------------------------------------------------------*/

#define STRING_CHUNK_SIZE	256

/*---------------------------------------------------------------------------*/

/* List containing all actions. It includes chunk for strings and actions
 * object, so memory allocation is handled by the list itself */

struct _NPWActionList
{
	GList* list;
	GStringChunk* string_pool;
	GMemChunk* data_pool;
};

/* Action object, it includes a pointer on its owner to allocate new data */

struct _NPWAction {
	NPWActionType type;
	gchar* command;
	NPWActionList* owner;
	GList* node;
};

/* Action object
 *
 *---------------------------------------------------------------------------*/

NPWAction*
npw_action_new (NPWActionList* owner, NPWActionType type)
{
	NPWAction* this;

	g_return_val_if_fail (owner != NULL, NULL);	
	
	this = g_chunk_new0(NPWAction, owner->data_pool);
	this->type = type;
	this->owner = owner;
	owner->list = g_list_append (owner->list, this);
	this->node = g_list_last (owner->list);
	
	return this;
}

void
npw_action_free (NPWAction* this)
{
	/* Memory allocated in string pool and project pool is not free */
}

NPWActionType
npw_action_get_type (const NPWAction* this)
{
	return this->type;
}

void
npw_action_set_command (NPWAction* this, const gchar* command)
{
	this->command = g_string_chunk_insert (this->owner->string_pool, command);
}

const gchar*
npw_action_get_command (const NPWAction* this)
{
	return this->command;
}

void
npw_action_set_file (NPWAction* this, const gchar* file)
{
	npw_action_set_command (this, file);
}

const gchar*
npw_action_get_file (const NPWAction* this)
{
	return npw_action_get_command (this);
}

const NPWAction*
npw_action_next (const NPWAction* this)
{
	GList* node = this->node->next;

	return node == NULL ? NULL : (NPWAction *)node->data;
}


/* Action list object
 *---------------------------------------------------------------------------*/

NPWActionList*
npw_action_list_new (void)
{
	NPWActionList* this;

	this = g_new (NPWActionList, 1);
	this->string_pool = g_string_chunk_new (STRING_CHUNK_SIZE);
	this->data_pool = g_mem_chunk_new ("action pool", sizeof (NPWAction), STRING_CHUNK_SIZE * sizeof (NPWAction) / 4, G_ALLOC_ONLY);
	this->list = NULL;

	return this;
}

void
npw_action_list_free (NPWActionList* this)
{
	g_return_if_fail (this != NULL);

	g_string_chunk_free (this->string_pool);
	g_mem_chunk_destroy (this->data_pool);
	g_list_free (this->list);
	g_free (this);
}

const NPWAction*
npw_action_list_first (const NPWActionList* this)
{
	GList* node = g_list_first (this->list);

	return node == NULL ? NULL : (NPWAction *)node->data;
}
