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

#include <glib.h>
#include <gtk/gtk.h>

#include "plugin.h"

/** Defines how output should be handled */
typedef enum _ATPOutputType
{
	ATP_UNKNOWN = -1,
	ATP_SAME = 0,
	ATP_COMMON_MESSAGE,
	ATP_PRIVATE_MESSAGE,
	ATP_NEW_BUFFER,
	ATP_REPLACE_BUFFER,
	ATP_INSERT_BUFFER,
	ATP_APPEND_BUFFER,
	ATP_POPUP_DIALOG,
	ATP_NULL,
	ATP_OUTPUT_TYPE_COUNT
#if 0
	ATP_TBUF_NEW, /* Create a new buffer and put output into it */
	ATP_TBUF_REPLACE, /* Replace exisitng buffer content with output */
	ATP_TBUF_INSERT, /* Insert at cursor position in the current buffer */
	ATP_TBUF_APPEND, /* Append output to the end of the buffer */
	ATP_TBUF_REPLACESEL, /* Replace the current selection with the output */
	ATP_TBUF_POPUP /* Show result in a popup window */
#endif
} ATPOutputType;

#if 0
/** Defines what to supply to the standard input of the tool  */
typedef enum _ATPToolInputType
{
	ATP_TINP_NONE = 0, /* No input */
	ATP_TINP_BUFFER, /* Contents of current buffer */
	ATP_TINP_STRING /* User defined string (variables will be expanded) */
} ATPToolInputType;

typedef enum _ATPOutput
{
	ATP_MESSAGE_STDERR,
	ATP_MESSAGE_STDOUT
} ATPOutput;
#endif

/** Defines how and where tool information will be stored. */
typedef enum _ATPToolStore
{
	ATP_TSTORE_GLOBAL = 0,
	ATP_TSTORE_LOCAL,
	ATP_TSTORE_PROJECT,
	ATP_LAST_TSTORE
} ATPToolStore;

typedef enum _ATPToolFlag
{
	ATP_CLEAR = 0,
	ATP_SET = 1,
	ATP_TOGGLE = 2,
	ATP_OPERATION = 3,
	ATP_TOOL_ENABLE = 1 << 2,
	ATP_TOOL_AUTOSAVE = 1 << 3,
	ATP_TOOL_TERMINAL = 1 << 4
} ATPToolFlag;

typedef struct _ATPUserTool ATPUserTool;
typedef struct _ATPToolList ATPToolList;
typedef guint ATPToolListPos;

struct _ATPToolList
{
	GHashTable* hash;
	GStringChunk* string_pool ;
	GMemChunk* data_pool;
	GtkMenu* menu;
	ATPUserTool* list;
	ATPPlugin* plugin;
};

void atp_user_tool_free(ATPUserTool *tool);

const char* atp_get_string_from_output_type (ATPOutputType type);
ATPOutputType atp_get_output_type_from_string (const gchar* type);

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

ATPPlugin* atp_user_tool_get_plugin (ATPUserTool* this);

gboolean atp_user_tool_activate (ATPUserTool* this, GtkMenu* submenu);

ATPUserTool *atp_user_tool_append_new (ATPUserTool* this, const gchar *name, ATPToolStore storage);

ATPUserTool *atp_user_tool_next (const ATPUserTool* this);
ATPUserTool *atp_user_tool_previous (const ATPUserTool* this);
ATPUserTool *atp_user_tool_in (ATPUserTool* this, ATPToolStore storage);



ATPToolList *atp_tool_list_construct (ATPToolList *this, ATPPlugin* plugin, GtkMenu* menu);
void atp_tool_list_destroy (ATPToolList *this);

ATPUserTool* atp_tool_list_append_new (ATPToolList *this, const gchar *name, ATPToolStore storage);

ATPUserTool* atp_tool_list_first (ATPToolList *this);
ATPUserTool* atp_tool_list_move (ATPToolList *this, ATPUserTool *tool, ATPUserTool *position);
gboolean atp_tool_list_activate (ATPToolList *this);

#endif
