/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    dialog.h
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

#ifndef __DIALOG_H__
#define __DIALOG_H__

#include "plugin.h"
#include "editor.h"
#include "variable.h"

#include <gtk/gtk.h>

typedef struct _ATPToolDialog ATPToolDialog;

struct _ATPToolDialog {
	GtkDialog* dialog;
	GtkTreeView* view;
	GtkWidget *edit_bt;
	GtkWidget *delete_bt;
	GtkWidget *up_bt;
	GtkWidget *down_bt;
	ATPToolEditorList tedl;
	ATPPlugin* plugin;
};

void atp_tool_dialog_construct (ATPToolDialog *this, ATPPlugin *plugin);
void atp_tool_dialog_destroy (ATPToolDialog *this);

GtkWindow* atp_tool_dialog_get_window (const ATPToolDialog *this);
void atp_tool_dialog_refresh (const ATPToolDialog *this, const gchar* select);

ATPVariable* atp_tool_dialog_get_variable (const ATPToolDialog *this);

gboolean atp_tool_dialog_show (ATPToolDialog *this);
void atp_tool_dialog_close (ATPToolDialog *this);

#endif
