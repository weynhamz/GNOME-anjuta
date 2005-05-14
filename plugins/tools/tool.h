/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    tool.h
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

#ifndef __TOOL_H__
#define __TOOL_H__

#include "plugin.h"

#include <gtk/gtk.h>

#include <glib.h>

/* Defines how output (stdout and stderr) should be handled */
typedef enum _ATPOutputType
{
	ATP_TOUT_UNKNOWN = -1,
	ATP_TOUT_SAME = 0,
	ATP_TOUT_COMMON_PANE,
	ATP_TOUT_NEW_PANE,
	ATP_TOUT_NEW_BUFFER,
	ATP_TOUT_REPLACE_BUFFER,
	ATP_TOUT_INSERT_BUFFER,
	ATP_TOUT_APPEND_BUFFER,
	ATP_TOUT_REPLACE_SELECTION,
	ATP_TOUT_POPUP_DIALOG,
	ATP_TOUT_NULL,
	ATP_OUTPUT_TYPE_COUNT
} ATPOutputType;

/* Defines how input should be handled */
typedef enum _ATPInputType
{
	ATP_TIN_UNKNOWN = -1,
	ATP_TIN_NONE = 0,
	ATP_TIN_BUFFER,
	ATP_TIN_SELECTION,
	ATP_TIN_STRING,
	ATP_TIN_FILE,
	ATP_INPUT_TYPE_COUNT
} ATPInputType;

/* Simple type to associate an identifier and a string */
typedef struct _ATPEnumType
{
	gint id;
	const gchar* name;
} ATPEnumType;

/* Defines how and where tool information will be stored. */
typedef enum _ATPToolStore
{
	ATP_TSTORE_GLOBAL = 0,
	ATP_TSTORE_LOCAL,
	ATP_TSTORE_PROJECT,
	ATP_LAST_TSTORE
} ATPToolStore;

/* Additional tool information */
typedef enum _ATPToolFlag
{
	/* Select the operation needed on the flag */
	ATP_CLEAR = 0,
	ATP_SET = 1,
	ATP_TOGGLE = 2,
	ATP_OPERATION = 3,
	/* Tool flags */
	ATP_TOOL_ENABLE = 1 << 2,
	ATP_TOOL_AUTOSAVE = 1 << 3,
	ATP_TOOL_TERMINAL = 1 << 4
} ATPToolFlag;

typedef struct _ATPUserTool ATPUserTool;
typedef struct _ATPToolList ATPToolList;

struct _ATPToolList
{
	GHashTable* hash;
	GStringChunk* string_pool ;
	GMemChunk* data_pool;
	AnjutaUI* ui;
	ATPUserTool* list;
	ATPPlugin* plugin;
};

/*---------------------------------------------------------------------------*/

const ATPEnumType* atp_get_output_type_list (void);
const ATPEnumType* atp_get_error_type_list (void);
const ATPEnumType* atp_get_input_type_list (void);

/*---------------------------------------------------------------------------*/

ATPUserTool* atp_user_tool_new (ATPToolList *list, const gchar* name, ATPToolStore storage);
void atp_user_tool_free(ATPUserTool *tool);

ATPUserTool *atp_user_tool_append_new (ATPUserTool* this, const gchar *name, ATPToolStore storage);
ATPUserTool *atp_user_tool_clone_new (ATPUserTool* this, ATPToolStore storage);

gboolean atp_user_tool_activate (ATPUserTool* this, GtkMenu* submenu, GtkAccelGroup* group);
void atp_user_tool_deactivate (ATPUserTool* this);

gboolean atp_user_tool_move_after (ATPUserTool* this, ATPUserTool* position);

gboolean atp_user_tool_set_name (ATPUserTool *this, const gchar *value);
const gchar* atp_user_tool_get_name (const ATPUserTool* this);

void atp_user_tool_set_command (ATPUserTool* this, const gchar* value);
const gchar* atp_user_tool_get_command (const ATPUserTool* this);

void atp_user_tool_set_param (ATPUserTool* this, const gchar* value);
const gchar* atp_user_tool_get_param (const ATPUserTool* this);

void atp_user_tool_set_working_dir (ATPUserTool* this, const gchar* value);
const gchar* atp_user_tool_get_working_dir (const ATPUserTool* this);

void atp_user_tool_set_flag (ATPUserTool *this, ATPToolFlag flag);
gboolean atp_user_tool_get_flag (const ATPUserTool *this, ATPToolFlag flag);

void atp_user_tool_set_output (ATPUserTool *this, ATPOutputType type);
ATPOutputType atp_user_tool_get_output (const ATPUserTool *this);

void atp_user_tool_set_error (ATPUserTool *this, ATPOutputType type);
ATPOutputType atp_user_tool_get_error (const ATPUserTool *this);

void atp_user_tool_set_input (ATPUserTool *this, ATPInputType type, const gchar* value);
ATPInputType atp_user_tool_get_input (const ATPUserTool *this);
const gchar* atp_user_tool_get_input_string (const ATPUserTool *this);

void atp_user_tool_set_accelerator (ATPUserTool *this, guint key, GdkModifierType mods);
gboolean atp_user_tool_get_accelerator (const ATPUserTool *this, guint *key, GdkModifierType *mods);

void atp_user_tool_set_icon (ATPUserTool *this, const gchar* value);
const gchar* atp_user_tool_get_icon (const ATPUserTool *this);

ATPToolStore atp_user_tool_get_storage (const ATPUserTool *this);

ATPPlugin* atp_user_tool_get_plugin (ATPUserTool* this);

ATPUserTool *atp_user_tool_next (ATPUserTool* this);
ATPUserTool *atp_user_tool_next_in_same_storage (ATPUserTool* this);
ATPUserTool *atp_user_tool_previous (ATPUserTool* this);
ATPUserTool *atp_user_tool_override (const ATPUserTool* this);

/*---------------------------------------------------------------------------*/

ATPToolList *atp_tool_list_construct (ATPToolList *this, ATPPlugin* plugin);
void atp_tool_list_destroy (ATPToolList *this);

ATPUserTool* atp_tool_list_first (ATPToolList *this);
ATPUserTool* atp_tool_list_first_in_storage (ATPToolList *this, ATPToolStore storage);
ATPUserTool* atp_tool_list_last (ATPToolList *this);

gboolean atp_tool_list_activate (ATPToolList *this);
ATPUserTool *atp_tool_list_append_new (ATPToolList* this, const gchar *name, ATPToolStore storage);


#endif
