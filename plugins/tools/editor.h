/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    editor.h
    Copyright (C) 2003 Biswapesh Chattopadhyay

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

#ifndef __EDITOR_H__
#define __EDITOR_H__

#include "tool.h"

#define GLADE_FILE PACKAGE_DATA_DIR"/glade/anjuta-tools.glade"

typedef struct _ATPToolEditor ATPToolEditor;
typedef struct _ATPToolEditorList ATPToolEditorList;

struct _ATPToolEditorList {
	ATPToolEditor* first;
};

struct _ATPToolDialog;

ATPToolEditor* atp_tool_editor_new (ATPUserTool *tool, ATPToolEditorList* list, struct _ATPToolDialog* dialog);
gboolean atp_tool_editor_free (ATPToolEditor* this);
gboolean atp_tool_editor_show (ATPToolEditor* this);

gboolean atp_tool_editor_show (ATPToolEditor* this);


ATPToolEditorList* atp_tool_editor_list_construct (ATPToolEditorList* this);
void atp_tool_editor_list_destroy (ATPToolEditorList* this);

#endif
