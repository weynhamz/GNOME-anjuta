/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    tool.c
    Copyright (C) 2005 Sebastien Granjoux

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
 * External tool data
 * 
 *---------------------------------------------------------------------------*/

#include <config.h>

#include "tool.h"
#include "execute.h"

/*---------------------------------------------------------------------------*/

#define STRING_CHUNK_SIZE	256

/*---------------------------------------------------------------------------*/

struct _ATPUserTool
{
	gchar *name;
	gchar *command;
	gchar *param;
	gchar *working_dir;
	gboolean enabled;
	ATPToolStore storage;
	GtkMenuItem* menu;
	guint position;
	ATPToolList *owner;
	ATPUserTool *over;
	ATPUserTool *next;
	ATPUserTool *prev;
	#if 0
	gboolean file_level;
	gboolean project_level;
	gboolean detached;
	gboolean run_in_terminal;
	gboolean user_params;
	gboolean autosave;
	gchar *location;
	gchar *icon;
	gchar *shortcut;
	int input_type; /* AnToolInputType */
	gchar *input;
	ATPOutput output; /* MESSAGE_* or AN_TBUF_* */
	ATPOutput error; /* MESSAGE_* or AN_TBUF_* */
	GtkWidget *menu_item;
	#endif
};

/* Tool list object
 *---------------------------------------------------------------------------*/

ATPToolList *
atp_tool_list_initialize (ATPToolList* this, ATPPlugin* plugin, GtkMenu* menu)
{
	this->menu = menu;
	this->plugin = plugin;
	/*memset (this->list, 0, sizeof(this->list)); */
	this->list = NULL;
	this->hash = g_hash_table_new (g_str_hash, g_str_equal);
	this->string_pool = g_string_chunk_new (STRING_CHUNK_SIZE);
	this->data_pool = g_mem_chunk_new ("tool pool", sizeof (ATPUserTool), STRING_CHUNK_SIZE * sizeof (ATPUserTool) / 4, G_ALLOC_AND_FREE);
	return this;
}

void atp_tool_list_destroy (ATPToolList* this)
{
	g_hash_table_destroy (this->hash);
	g_string_chunk_free (this->string_pool);
	g_mem_chunk_destroy (this->data_pool);
}

#if 0
static ATPUserTool *
atp_tool_list_get_tool (const ATPToolList *this, const gchar *name)
{
	/* ATPUserTool *tool; */

	return (ATPUserTool *)g_hash_table_lookup (this->hash, name);
}

static ATPUserTool *
atp_tool_list_get_tool_in (const ATPToolList *this, const gchar *name, ATPToolStore storage)
{
	ATPUserTool *tool;
	
	g_return_val_if_fail (name, NULL);

	tool = (ATPUserTool *)g_hash_table_lookup (this->hash, name);

	/* Search tool in the list */
	for(; tool != NULL;  tool = tool->over)
	{
		if (storage == tool->storage) break;
	}

	return tool;
}
#endif

static ATPUserTool *
atp_tool_list_last (const ATPToolList *this)
{
	ATPUserTool* tool;

	tool = this->list;
	if (tool != NULL)
	{
		for (; tool->next != NULL; tool = tool->next);
	}

	return tool;
}

/* Create a tool with corresponding name and storage
 *  but does NOT add it in one of the storage list */

static ATPUserTool *
atp_tool_list_new_tool (ATPToolList *this, const gchar *name, ATPToolStore storage)
{
	ATPUserTool *tool;

	g_return_val_if_fail (this, NULL);

	if (name)
	{
		/* Search tool in hash table */
		tool = (ATPUserTool *) g_hash_table_lookup (this->hash, name);
		if (tool != NULL)
		{
			/* Search tool in the list */
			for(;;  tool = tool->over)
			{
				if (storage == tool->storage)
				{
					/* Tool already exist */

					return NULL;
				}
				else if (storage > tool->storage)
				{
					ATPUserTool *over;

					/* Add tool before, using previous values as default */
					over = g_chunk_new0(ATPUserTool, this->data_pool);
					memcpy(over, tool, sizeof (ATPUserTool));
					tool->over = over;
					break;
				}
				else if (tool->over == NULL)
				{
					/* Add tool after */
					tool->over = g_chunk_new0(ATPUserTool, this->data_pool);
					tool->over->name = tool->name;
					tool = tool->over;
					break;
				}
			}
		}
		else
		{
			/* Create new tool */
			tool = g_chunk_new0(ATPUserTool, this->data_pool);
			tool->name = g_string_chunk_insert_const (this->string_pool, name);
			g_hash_table_insert (this->hash, tool->name, tool);
		}
	}
	else
	{
		/* Create stand alone tool */
		tool = g_chunk_new0(ATPUserTool, this->data_pool);
	}
		
	/* Set default values */
	tool->storage = storage;
	tool->owner = this;
	#if 0
	tool->enabled = TRUE;
	tool->output = ATP_MESSAGE_STDOUT;
	tool->error = ATP_MESSAGE_STDERR;
	#endif

	return tool;
}



static ATPUserTool* 
atp_tool_list_remove (ATPToolList *this, ATPUserTool* tool)
{
	ATPUserTool* first;

	g_return_val_if_fail (tool, NULL);
	g_return_val_if_fail (tool->owner == this, NULL);

	/* Remove from storage list */
	first = (ATPUserTool *) g_hash_table_lookup (this->hash, tool->name);
	if (first == tool)
	{
		if (this->list == tool)
		{
			this->list = tool->next;
			tool->next->prev = NULL;
		}
		else
		{
			if (tool->next != NULL)
			{
				tool->next->prev = tool->prev;
			}
			if (tool->prev != NULL)
			{
				tool->prev->next = tool->next;
			}
		}
	}
	tool->next = NULL;
	tool->prev = NULL;

	return tool;
}

ATPUserTool*
atp_tool_list_move (ATPToolList *this, ATPUserTool *tool, ATPUserTool *position)
{
	ATPUserTool* first;

	g_return_val_if_fail (tool, NULL);
	g_return_val_if_fail (tool->owner == this, NULL);

	if (tool->name != NULL)
	{
		/* Remove tool from list if necessary */
		first = (ATPUserTool *) g_hash_table_lookup (this->hash, tool->name);
		if (first == tool)
		{
			/* Remove tool from list */
			if (tool->prev != NULL)
			{
				tool->prev->next = tool->next;
			}
			if (tool->next != NULL)
			{
				tool->next->prev = tool->prev;
			}
			if (this->list == tool)
			{
				this->list = tool->next;
			}
		}
	}

	/* By default move at the end */
	if (position == NULL) position = atp_tool_list_last (this);

	if (position == NULL)
	{
		tool->prev = NULL;
		tool->next = NULL;
		this->list = tool;
	}
	else
	{
		tool->prev = position;
		tool->next = position->next;
		position->next = tool;
		if (tool->next)
		{
			tool->next->prev = tool;
		}
	}

	return tool;
}

static gboolean
atp_tool_list_unregister (ATPToolList *this, ATPUserTool *tool)
{
	/* Remove from hash table and override list */
	if (tool->name != NULL)
	{
		ATPUserTool *first;

		first = (ATPUserTool *)g_hash_table_lookup (this->hash, tool->name);
		if (first == NULL) return TRUE;
		if (first == tool)
		{
			if (first->over == NULL)
			{
				g_hash_table_remove (this->hash, tool->name);
			}
			else
			{
				g_hash_table_replace (this->hash, tool->name, first->over);
			}
		}
		else
		{
			for (; first->over != tool; first = first->over)
			{
				if (first == NULL)
				{
					/* Unable to find tool */
					return FALSE;
				}
			}
			first->over = tool->over;
		}
	}

	return TRUE;
}

static gboolean
atp_tool_list_register (ATPToolList *this, ATPUserTool *tool)
{
	g_return_val_if_fail (tool, FALSE);
	g_return_val_if_fail (tool->owner == this, FALSE);

	if (tool->name != NULL)
	{
		ATPUserTool *first;

		/* Add tool in hash table */
		first = (ATPUserTool *) g_hash_table_lookup (this->hash, tool->name);
		if (first != NULL)
		{
			if (first->storage == tool->storage)
			{
				/* Ok if tool is already here */
				g_return_val_if_fail (first != tool, FALSE);
			}
			else if (first->storage < tool->storage)
			{
				/* Add before */
				g_hash_table_replace (this->hash, tool->name, tool);
			}
			else
			{
				for (; first->over != NULL; first = first->over)
				{
					if (first->storage <= tool->storage) break;
				}
				if (first->storage == tool->storage)
				{
					/* Ok if tool is already here */
					g_return_val_if_fail (first != tool, FALSE);
				}
				else
				{
					/* Add after */
					tool->over = first->over;
					first->over = tool;
				}
			}
		}
		else
		{
			/* Add new */
			g_hash_table_insert (this->hash, tool->name, tool);
		}
	}

	return TRUE;
}

ATPUserTool *
atp_tool_list_append_new (ATPToolList *this, const gchar *name, ATPToolStore storage)
{
	ATPUserTool *tool;

	g_return_val_if_fail (this, NULL);

	/* Create tool */
	tool = atp_tool_list_new_tool (this, name, storage);

	/* Add it in one of the storage list if necessary */
	if (tool)
	{
		atp_tool_list_move (this, tool, NULL);
	}

	return tool;
}

ATPUserTool* atp_tool_list_first (ATPToolList *this)
{
	ATPUserTool *tool;
	guint storage;

	for (storage = 0; storage < ATP_LAST_TSTORE; storage++)
	{
		for (tool = this->list; tool != NULL; tool = tool->next)
		{
			/* Skip tool not registered */
			if ((tool->name != NULL) && (g_hash_table_lookup (this->hash, tool->name) == tool))
			{
				return tool;
			}
		}
	}

	return NULL;
}

gboolean atp_tool_list_activate (ATPToolList *this)
{
	ATPUserTool *next;

	for (next = this->list; next != NULL; next = atp_user_tool_next (next))
	{
		atp_user_tool_activate (next, this->menu);
	}

	return TRUE;
}

/*ATPUserTool* atp_tool_list_first_in (ATPToolList *this, ATPToolStore storage)
{
	ATPUserTool *next;

	for (next = this->list[storage]; next != NULL; next = next->next)
	{
		if (next->name != NULL) break;
	}

	return next;
}*/

/* Tool object
 *
 *---------------------------------------------------------------------------*/

void
atp_user_tool_free (ATPUserTool *this)
{
	g_return_if_fail (this->owner);

	atp_tool_list_remove (this->owner, this);
	atp_tool_list_unregister (this->owner, this);

	if (this->menu) gtk_widget_destroy (GTK_WIDGET(this->menu));
	g_chunk_free (this, this->owner->data_pool);
}

gboolean
atp_user_tool_set_name (ATPUserTool *this, const gchar *value)
{
	atp_tool_list_unregister (this->owner, this);
	this->name = value == NULL ? NULL : g_string_chunk_insert_const (this->owner->string_pool, value);
	return atp_tool_list_register (this->owner, this);
}

const gchar*
atp_user_tool_get_name (const ATPUserTool* this)
{
	return this->name;
}

void
atp_user_tool_set_command (ATPUserTool* this, const gchar* value)
{
	this->command = value == NULL ? NULL : g_string_chunk_insert_const (this->owner->string_pool, value);
}

const gchar*
atp_user_tool_get_command (const ATPUserTool* this)
{
	return this->command;
}

void
atp_user_tool_set_param (ATPUserTool* this, const gchar* value)
{
	this->param = value == NULL ? NULL : g_string_chunk_insert_const (this->owner->string_pool, value);
}

const gchar*
atp_user_tool_get_param (const ATPUserTool* this)
{
	return this->param;
}

void
atp_user_tool_set_working_dir (ATPUserTool* this, const gchar* value)
{
	this->working_dir = value == NULL ? NULL : g_string_chunk_insert_const (this->owner->string_pool, value);
}

const gchar*
atp_user_tool_get_working_dir (const ATPUserTool* this)
{
	return this->working_dir;
}

void
atp_user_tool_set_enable (ATPUserTool* this, gboolean value)
{
	this->enabled = value;
}

gboolean
atp_user_tool_get_enable (const ATPUserTool* this)
{
	return this->enabled;
}

ATPPlugin*
atp_user_tool_get_plugin (ATPUserTool* this)
{
	return this->owner->plugin;
}

ATPUserTool*
atp_user_tool_append_new (ATPUserTool *this, const gchar *name, ATPToolStore storage)
{
	ATPUserTool *tool;

	g_return_val_if_fail (this, NULL);

	/* Create tool */
	tool = atp_tool_list_new_tool (this->owner, name, storage);

	/* Add it in one of the storage list if necessary */
	if (tool)
	{
		atp_tool_list_move (tool->owner, tool, this);
	}

	return tool;
}

ATPUserTool*
atp_user_tool_next (const ATPUserTool *this)
{
	ATPUserTool *next;

	for (;;)
	{
		next = this->next;
		if (next == NULL)
		{
			return NULL;
		}
		/* Skip tool not registered */
		if ((next->name != NULL) && (g_hash_table_lookup (this->owner->hash, next->name) == next))
		{
			return next;
		}
		this = next;
	}
}

ATPUserTool*
atp_user_tool_previous (const ATPUserTool *this)
{
	ATPUserTool *prev;

	for (;;)
	{
		prev = this->prev;
		if (prev == NULL)
		{
			return NULL;
		}
		/* Skip tool not registered */
		if ((prev->name != NULL) && (g_hash_table_lookup (this->owner->hash, prev->name) == prev))
		{
			return prev;
		}
		this = prev;
	}
}

ATPUserTool*
atp_user_tool_in (ATPUserTool *this, ATPToolStore storage)
{
	ATPUserTool* tool;

	for (tool = this; (tool != NULL) && (tool->storage >= storage); tool = tool->over)
	{
		if (tool->storage == storage) return tool;
	}

	return NULL;
}

gboolean
atp_user_tool_activate (ATPUserTool *this, GtkMenu* submenu)
{
	if (this->menu != NULL)
	{
		gtk_widget_destroy (GTK_WIDGET(this->menu));
	}

	this->menu = gtk_menu_item_new_with_mnemonic (this->name);
	//gtk_widget_ref (this->menu);
	g_signal_connect (G_OBJECT (this->menu), "activate", G_CALLBACK (atp_user_tool_execute), this);
	gtk_menu_shell_append (GTK_MENU_SHELL (submenu), GTK_WIDGET (this->menu));
	gtk_widget_show(GTK_WIDGET (this->menu));

	return TRUE;
}

#if 0
ATPUserTool*
atp_user_tool_next_in (ATPUserTool *this)
{
	ATPUserTool* next;

	for (next = this->next; next != NULL; next = next->next)
	{
		/* Skip tool with empty name */
		if (next->name != NULL) break;
	}

	return next;
}
#endif
