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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*
 * Keep all external tool data (ATPUserTool)
 * All tools belong to a list (ATPToolList) which take care of allocating 
 * memory for them. This tool list is implemented with a linked list of all
 * tools and a hash table to allow a fast access to a tool from its name.
 *
 * It is possible to read tools configuration from differents files (typically
 * a file in a system directory and another one in an user directory) and to
 * have the same tools (= same name) defined in both files. In this case
 * both are in the list and each tool have a different storage number and are
 * linked with a pointer named over.
 *
 * It is not possible to ordered tool with different storage order. But to
 * emulate this function, it is possible to create new tools with the same
 * data in the right storage location.
 *---------------------------------------------------------------------------*/

#include <config.h>

#include "tool.h"

#include "execute.h"

#include <libanjuta/anjuta-ui.h>

/*---------------------------------------------------------------------------*/

#define STRING_CHUNK_SIZE	256

/*---------------------------------------------------------------------------*/

struct _ATPUserTool
{
	gchar *name;
	gchar *command;		/* Command and parameters are in two variables */
	gchar *param;
	gchar *working_dir;
	ATPToolFlag flags;
	ATPOutputType output;
	ATPOutputType error;
	ATPInputType input;
	gchar *input_string;
	ATPToolStore storage;
	GtkWidget* menu_item;
	GtkAction *action;
	GtkActionGroup *action_group;
	guint accel_key;
	GdkModifierType accel_mods;
	gchar *icon;
	guint merge_id;
	ATPToolList *owner;	 
	ATPUserTool *over;	/* Same tool in another storage */
	ATPUserTool *next;	/* Next tool in the list */
	ATPUserTool *prev;	/* Previous tool in the list */
};

/* Tools helper functions (map enum to string) 
 *---------------------------------------------------------------------------*/

ATPEnumType output_type_list[] = {
 {ATP_TOUT_SAME, N_("Same as output")},
 {ATP_TOUT_COMMON_PANE, N_("Existing message pane")},
 {ATP_TOUT_NEW_PANE, N_("New message pane")},
 {ATP_TOUT_NEW_BUFFER, N_("New buffer")},
 {ATP_TOUT_REPLACE_BUFFER, N_("Replace buffer")},
 {ATP_TOUT_INSERT_BUFFER, N_("Insert in buffer")},
 {ATP_TOUT_APPEND_BUFFER, N_("Append to buffer")},
 {ATP_TOUT_REPLACE_SELECTION, N_("Replace selection")},
 /* Translators: Checkbox if a dialog should be shown after some operation finishes, so translate as "to popup a dialog" */
 {ATP_TOUT_POPUP_DIALOG, N_("Popup dialog")},
 {ATP_TOUT_NULL, N_("Discard output")},
 {-1, NULL}
};

ATPEnumType input_type_list[] = {
 {ATP_TIN_NONE, N_("None")},
 {ATP_TIN_BUFFER, N_("Current buffer")},
 {ATP_TIN_SELECTION, N_("Current selection")},
 {ATP_TIN_STRING, N_("String")},
 {ATP_TIN_FILE, N_("File")},
 {-1, NULL}
};

/*---------------------------------------------------------------------------*/

const ATPEnumType* atp_get_output_type_list (void)
{
	return &output_type_list[1];
}

const ATPEnumType* atp_get_error_type_list (void)
{
	return &output_type_list[0];
}

const ATPEnumType* atp_get_input_type_list (void)
{
	return &input_type_list[0];
}

/* Return a copy of the name without mnemonic (underscore) */
gchar*
atp_remove_mnemonic (const gchar* label)
{
	const char *src;
	char *dst;
	char *without;

	without = g_new (char, strlen(label) + 1);
	dst = without;
	for (src = label; *src != '\0'; ++src)
	{
		if (*src == '_')
		{
			/* Remove single underscore */
			++src;
		}
		*dst++ = *src;
	}
	*dst = *src;

	return without;
}

/* Tool object
 *
 *---------------------------------------------------------------------------*/

/* Private functions
 *---------------------------------------------------------------------------*/

/* Remove tool from list but doesn't remove from hash table */

static gboolean
atp_user_tool_remove_list (ATPUserTool* this)
{
	g_return_val_if_fail (this, FALSE);
	g_return_val_if_fail (this->owner, FALSE);

	/* Remove from storage list */
	if (this->owner->list == this)
	{
		/* First tool in the list */
		this->owner->list = this->next;
		if (this->next != NULL)
		{
			this->next->prev = NULL;
		}
	}
	else
	{
		if (this->next != NULL)
		{
			this->next->prev = this->prev;
		}
		if (this->prev != NULL)
		{
			this->prev->next = this->next;
		}
	}
	this->next = NULL;
	this->prev = NULL;

	return TRUE;
}

/* Add tool in list but doesn't add in hash table */

static gboolean
atp_user_tool_append_list (ATPUserTool *this, ATPUserTool *tool)
{
	g_return_val_if_fail (tool, FALSE);

	/* Keep storage type ordered */
	if (this == NULL)
	{
		ATPUserTool* first;
		/* Insert at the beginning of list (with right storage) */
		for (first = tool->owner->list; first != NULL; first = first->next)
		{
			if (first->storage >= tool->storage) break;
			this = first;
		}	
	}

	/* If this is NULL insert at the beginning of the list */
	if (this == NULL)
	{
		tool->next = tool->owner->list;
		if (tool->next != NULL)
			tool->next->prev = tool;
		tool->owner->list = tool;
		tool->prev = NULL;
	}
	else if ((this->storage == tool->storage) || (this->next == NULL) || (this->next->storage >= tool->storage))
	{
		/* Insert tool in list */
		tool->prev = this;
		tool->next = this->next;
		this->next = tool;
		if (tool->next)
		{
			tool->next->prev = tool;
		}
	}
	else if (this->storage < tool->storage)
	{
		ATPUserTool *prev;
		/* Insert tool at the beginning of storage list */
		atp_user_tool_append_list (NULL, tool);

		/* Create new tool with the right storage and reorder them */
		for (prev = tool;(prev = atp_user_tool_previous (prev)) != this;)
		{
			ATPUserTool *clone;

			clone = atp_user_tool_new (this->owner, prev->name, tool->storage);
			atp_user_tool_append_list (tool, clone);
		}
	}		
	else
	{
		/* Unable to handle this case */
		g_return_val_if_reached (FALSE);
		return FALSE;
	}

	return TRUE;
}

/* Remove from hash table and list */

static gboolean
atp_user_tool_remove (ATPUserTool *this)
{
	if (this->name != NULL)
	{
		/* Remove from hash table */
		ATPUserTool *first;

		first = (ATPUserTool *)g_hash_table_lookup (this->owner->hash, this->name);
		if (first == NULL)
		{
		 	/* Unable to find tool */	
			g_return_val_if_reached (FALSE);
		}
		if (first == this)
		{
			if (this->over == NULL)
			{
				g_hash_table_remove (this->owner->hash, this->name);
			}
			else
			{
				g_hash_table_replace (this->owner->hash, this->name, this->over);
			}
		}
		else
		{
			for (; first->over != this; first = first->over)
			{
				if (first == NULL)
				{
					/* Unable to find tool */
					return FALSE;
				}
			}
			first->over = this->over;
		}
	}

	/* Remove from list */
	return atp_user_tool_remove_list (this);
}

/* Replace tool name */

static gboolean
atp_user_tool_replace_name (ATPUserTool *this, const gchar *name)
{
	if ((name != NULL) && (g_hash_table_lookup (this->owner->hash, name) != NULL))
	{
		/* Name already exist */
		return FALSE;
	}

	if (this->name != NULL)
	{
		ATPUserTool *first;

		first = (ATPUserTool *) g_hash_table_lookup (this->owner->hash, this->name);

		if (first->over == NULL)
		{
			g_return_val_if_fail (first == this, FALSE);

			g_hash_table_remove (this->owner->hash, this->name);
		}
		else
		{
			/* Remove tool from override list */
			if (first == this)
			{
				g_hash_table_replace (this->owner->hash, this->name, this->over);
				this->over = NULL;
			}
			else
			{
				ATPUserTool *tool;
				
				for (tool = first; tool->over != this; tool = tool->over)
				{
					/* Tool not in list */
					g_return_val_if_fail (tool->over != NULL, FALSE);
				}
				tool->over = this->over;
			}
		}
	}

	/* Add in list */
	this->name = name == NULL ? NULL : g_string_chunk_insert_const (this->owner->string_pool, name);
	if (name != NULL)
	{
		/* Add name in hash table */
		g_hash_table_insert (this->owner->hash, this->name, this);
	}

	return TRUE;
}

/* Creation and Destruction
 *---------------------------------------------------------------------------*/

/* Create a tool with corresponding name and storage
 *  but does NOT add it in the list */

ATPUserTool *
atp_user_tool_new (ATPToolList *list, const gchar *name, ATPToolStore storage)
{
	ATPUserTool *first;
	ATPUserTool *tool;

	g_return_val_if_fail (list, NULL);

	if (name)
	{
		/* Search tool in hash table */
		first = (ATPUserTool *) g_hash_table_lookup (list->hash, name);
		if (first != NULL)
		{
			/* Search tool in the list */
			for(tool = first;;  tool = tool->over)
			{
				if (tool->storage == storage)
				{
					/* Tool already exist */

					return NULL;
				}
				else if (tool->storage > storage)
				{
					/* Add tool before */
					g_return_val_if_fail (tool == first, NULL);

					tool = g_slice_new0(ATPUserTool);
					tool->over = first;
					tool->flags = ATP_TOOL_ENABLE;
					tool->name = first->name;
					g_hash_table_replace (list->hash, tool->name, tool);
					break;
				}
				else if ((tool->over == NULL) || (tool->over->storage > storage)) 
				{
					/* Add tool after, using previous values as default */
					first = g_slice_new(ATPUserTool);
					memcpy(first, tool, sizeof (ATPUserTool));
					first->over = tool->over;
					tool->over = first;
					tool->menu_item = NULL;
					tool = tool->over;
					break;
				}
			}
		}
		else
		{
			/* Create new tool */
			tool = g_slice_new0(ATPUserTool);
			tool->flags = ATP_TOOL_ENABLE;
			tool->name = g_string_chunk_insert_const (list->string_pool, name);
			g_hash_table_insert (list->hash, tool->name, tool);
		}
	}
	else
	{
		/* Create stand alone tool */
		tool = g_slice_new0(ATPUserTool);
		tool->flags = ATP_TOOL_ENABLE;
	}
		
	/* Set default values */
	tool->storage = storage;
	tool->owner = list;

	return tool;
}

void
atp_user_tool_free (ATPUserTool *this)
{
	g_return_if_fail (this->owner);

	atp_user_tool_remove (this);
	atp_user_tool_deactivate (this, this->owner->ui);

	g_slice_free (ATPUserTool, this);
}


/* Access tool data
 *---------------------------------------------------------------------------*/

gboolean
atp_user_tool_set_name (ATPUserTool *this, const gchar *value)
{
	if ((value != this->name) && ((value == NULL) || (this->name == NULL) || (strcmp (value, this->name) != 0)))
	{
		return atp_user_tool_replace_name (this, value);
	}

	return TRUE;
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
atp_user_tool_set_flag (ATPUserTool* this, ATPToolFlag flag)
{
	switch (flag & ATP_OPERATION)
	{
	case ATP_SET:
		this->flags |= flag;
		break;
	case ATP_CLEAR:
		this->flags &= ~flag;
		break;
	case ATP_TOGGLE:
		this->flags ^= flag;
		break;
	default:
		g_return_if_reached();
	}
	
	if ((flag & ATP_TOOL_ENABLE) && (this->menu_item != NULL))
	{
		/* Enable or disable menu item */
		gtk_widget_set_sensitive (this->menu_item, this->flags & ATP_TOOL_ENABLE);
	}
}

gboolean
atp_user_tool_get_flag (const ATPUserTool* this, ATPToolFlag flag)
{
	return this->flags & flag ? TRUE : FALSE;
}

void
atp_user_tool_set_output (ATPUserTool *this, ATPOutputType output)
{
	this->output = output;
}

ATPOutputType
atp_user_tool_get_output (const ATPUserTool *this)
{
	return this->output;
}

void
atp_user_tool_set_error (ATPUserTool *this, ATPOutputType error)
{
	this->error = error;
}

ATPOutputType
atp_user_tool_get_error (const ATPUserTool *this )
{
	return this->error;
}

void
atp_user_tool_set_input (ATPUserTool *this, ATPInputType type, const gchar* value)
{
	this->input = type;
	this->input_string = value == NULL ? NULL : g_string_chunk_insert_const (this->owner->string_pool, value);
}

ATPInputType
atp_user_tool_get_input (const ATPUserTool *this )
{
	return this->input;
}

const gchar* 
atp_user_tool_get_input_string (const ATPUserTool *this )
{
	return this->input_string;
}

void
atp_user_tool_set_accelerator (ATPUserTool *this, guint key, GdkModifierType mods)
{
	this->accel_key = key;
	this->accel_mods = mods;
}

gboolean
atp_user_tool_get_accelerator (const ATPUserTool *this, guint *key, GdkModifierType *mods)
{
	*key = this->accel_key;
	*mods = this->accel_mods;

	return this->accel_key != 0;
}

void atp_user_tool_set_icon (ATPUserTool *this, const gchar* value)
{
	this->icon = value == NULL ? NULL : g_string_chunk_insert_const (this->owner->string_pool, value);
}

const gchar* atp_user_tool_get_icon (const ATPUserTool *this)
{
	return this->icon;
}

ATPToolStore atp_user_tool_get_storage (const ATPUserTool *this)
{
	return this->storage;
}

ATPPlugin*
atp_user_tool_get_plugin (ATPUserTool* this)
{
	return this->owner->plugin;
}

gboolean
atp_user_tool_is_valid (const ATPUserTool* this)
{
	return (this->command != NULL);
}

/* Additional tool functions
 *---------------------------------------------------------------------------*/

ATPUserTool*
atp_user_tool_next (ATPUserTool *this)
{
	while ((this = this->next))
	{
		/* Skip unnamed and overridden tool */
		if ((this->name != NULL) && (this->over == NULL)) break;
	}

	return this;
}

ATPUserTool*
atp_user_tool_next_in_same_storage (ATPUserTool *this)
{
	ATPToolStore storage;

	storage = this->storage;
	while ((this = this->next))
	{
		/* Skip unnamed */
		if (this->storage != storage) return NULL;
		if (this->name != NULL) break;
	}

	return this;
}

ATPUserTool*
atp_user_tool_previous (ATPUserTool *this)
{
	while ((this = this->prev))
	{
		/* Skip unnamed and overridden tool */
		if ((this->name != NULL) && (this->over == NULL)) break;
	}

	return this;
}

ATPUserTool*
atp_user_tool_override (const ATPUserTool *this)
{
	ATPUserTool* tool;

	for (tool = g_hash_table_lookup (this->owner->hash, this->name);
		 tool != NULL; tool = tool->over)
	{
		if (tool->over == this) return tool;
	}

	return NULL;
}

ATPUserTool*
atp_user_tool_append_new (ATPUserTool *this, const gchar *name, ATPToolStore storage)
{
	ATPUserTool *tool;

	g_return_val_if_fail (this, NULL);

	/* Create tool */
	tool = atp_user_tool_new (this->owner, name, storage);
	if (tool)
	{
		atp_user_tool_append_list (this, tool);
	}

	return tool;
}

ATPUserTool*
atp_user_tool_clone_new (ATPUserTool *this, ATPToolStore storage)
{
	ATPUserTool *tool;

	g_return_val_if_fail (this, NULL);

	/* Create tool */
	tool = atp_user_tool_new (this->owner, this->name, storage);
	if (tool)
	{
		ATPUserTool *prev;

		prev = atp_user_tool_previous (this);
		atp_user_tool_append_list (prev, tool);
	}

	return tool;
}

gboolean
atp_user_tool_move_after (ATPUserTool *this, ATPUserTool *position)
{
	g_return_val_if_fail (this, FALSE);

	if (!atp_user_tool_remove_list (this)) return FALSE;
	return atp_user_tool_append_list (position, this);
}

void
atp_user_tool_deactivate (ATPUserTool* this, AnjutaUI *ui)
{
	if (this->merge_id != 0)	
	{
		gtk_ui_manager_remove_ui (GTK_UI_MANAGER (ui), this->merge_id);
		gtk_ui_manager_remove_action_group (GTK_UI_MANAGER (ui), this->action_group);
	}
}

gboolean
atp_user_tool_activate (ATPUserTool *this, GtkAccelGroup *group, AnjutaUI *ui)
{
	gchar *menuitem_path;	

	/* Remove previous menu */
	atp_user_tool_deactivate (this, ui);

	/* Create new menu item */
	this->action = gtk_action_new (this->name, this->name, this->name, NULL);
	this->action_group = gtk_action_group_new ("ActionGroupTools");

	if (this->accel_key != 0)
	{
		gchar *accelerator;		
		
		accelerator = gtk_accelerator_name (this->accel_key, this->accel_mods);
		gtk_action_group_add_action_with_accel (this->action_group, this->action, accelerator);
	}	
	else
	{
		gtk_action_group_add_action (this->action_group, this->action);
	}

	gtk_ui_manager_insert_action_group (GTK_UI_MANAGER (ui), this->action_group, 0);

	this->merge_id = gtk_ui_manager_new_merge_id (GTK_UI_MANAGER (ui));
	gtk_ui_manager_add_ui (GTK_UI_MANAGER (ui), 
							this->merge_id,
							MENU_PLACEHOLDER,
							this->name,
							this->name,
							GTK_UI_MANAGER_MENUITEM,
							FALSE);

	menuitem_path = g_strconcat (MENU_PLACEHOLDER, "/", this->name, NULL);
	this->menu_item = gtk_ui_manager_get_widget (GTK_UI_MANAGER (ui), menuitem_path);
	gtk_action_set_sensitive (this->action, this->flags & ATP_TOOL_ENABLE);

	/* Add icon */
	if ((this->menu_item != NULL) && (this->icon != NULL))
	{
		GdkPixbuf *pixbuf;
		GdkPixbuf *scaled_pixbuf;
		gint height, width;
			
		gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (this->menu_item), GTK_ICON_SIZE_MENU, &width, &height);

		pixbuf = gdk_pixbuf_new_from_file (this->icon, NULL);
		if (pixbuf)
		{
			GtkWidget* icon;

			scaled_pixbuf = gdk_pixbuf_scale_simple (pixbuf, width, height, GDK_INTERP_BILINEAR);
			icon = gtk_image_new_from_pixbuf (scaled_pixbuf);
			gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (this->menu_item), icon);
                	g_object_unref (pixbuf);
			g_object_unref (scaled_pixbuf);
		}
	}

	g_signal_connect (G_OBJECT (this->action), "activate", G_CALLBACK (atp_user_tool_execute), this);

	return TRUE;
}

/* Tool list object containing all tools
 *---------------------------------------------------------------------------*/

ATPToolList *
atp_tool_list_construct (ATPToolList* this, ATPPlugin* plugin)
{
	this->plugin = plugin;
	this->ui = anjuta_shell_get_ui (ANJUTA_PLUGIN(plugin)->shell, NULL);
	this->list = NULL;
	this->hash = g_hash_table_new (g_str_hash, g_str_equal);
	this->string_pool = g_string_chunk_new (STRING_CHUNK_SIZE);

	return this;
}

static void
on_remove_tool (gpointer key, gpointer value, gpointer user_data)
{
	ATPUserTool* tool = value;
	
	do
	{
		ATPUserTool* next = tool->over;
		
		g_slice_free (ATPUserTool, tool);
		tool = next;
	} while (tool != NULL);
}

void atp_tool_list_destroy (ATPToolList* this)
{
	g_hash_table_foreach(this->hash, (GHFunc)on_remove_tool, NULL);
	g_hash_table_destroy (this->hash);
	g_string_chunk_free (this->string_pool);
}

/*---------------------------------------------------------------------------*/

ATPUserTool *
atp_tool_list_last (ATPToolList *this)
{
	ATPUserTool *tool;
	ATPUserTool *last;

	last = NULL;
	for (tool = this->list; tool != NULL; tool = tool->next)
	{
		/* Skip tool not registered */
		if (tool->name != NULL)
		{
			last = tool;
		}
	}

	return last;
}

static ATPUserTool *
atp_tool_list_last_in_storage (const ATPToolList *this, ATPToolStore storage)
{
	ATPUserTool *tool;
	ATPUserTool *last;

	last = NULL;
	for (tool = this->list; tool != NULL; tool = tool->next)
	{
		if (tool->storage > storage) break;
		/* Skip tool not registered */
		if (tool->name != NULL)
		{
			last = tool;
		}
	}

	return last;
}

ATPUserTool*
atp_tool_list_first (ATPToolList *this)
{
	ATPUserTool *tool;

	for (tool = this->list; tool != NULL; tool = tool->next)
	{
		/* Skip unnamed and overridden tool*/
		if ((tool->name != NULL) && (tool->over == NULL))
		{
			return tool;
		}
	}

	return NULL;
}

ATPUserTool*
atp_tool_list_first_in_storage (ATPToolList *this, ATPToolStore storage)
{
	ATPUserTool *tool;

	for (tool = this->list; tool != NULL; tool = tool->next)
	{
		/* Skip unamed tool */
		if ((tool->name != NULL) && (tool->storage == storage))
		{
			return tool;
		}
	}

	return NULL;
}

ATPUserTool *
atp_tool_list_append_new (ATPToolList *this, const gchar *name, ATPToolStore storage)
{
	ATPUserTool *tool;

	g_return_val_if_fail (this, NULL);

	/* Create tool */
	tool = atp_user_tool_new (this, name, storage);

	/* Add it in one of the storage list if necessary */
	if (tool)
	{
		atp_user_tool_append_list (atp_tool_list_last_in_storage (this, storage), tool);
	}

	return tool;
}

gboolean 
atp_tool_list_activate (ATPToolList *this)
{
	ATPUserTool *next;
	GtkAccelGroup* group;

	group = anjuta_ui_get_accel_group (this->ui);

	for (next = atp_tool_list_first (this); next != NULL; next = atp_user_tool_next (next))
	{
		atp_user_tool_activate (next, group, this->ui);
	}

	return TRUE;
}

gboolean 
atp_tool_list_deactivate (ATPToolList *this)
{
	ATPUserTool *next;

	for (next = atp_tool_list_first (this); next != NULL; next = atp_user_tool_next (next))
	{
		atp_user_tool_deactivate (next, next->owner->ui);
	}

	return TRUE;
}


