/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.h
    Copyright (C) 2000 Naba Kumar

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

#ifndef __TOOLS_PLUGIN__
#define __TOOLS_PLUGIN__

#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/interfaces/ianjuta-message-manager.h>
#include <libanjuta/anjuta-launcher.h>

#include <gtk/gtk.h>

#include "variable.h"

typedef struct _ATPPlugin ATPPlugin;
typedef struct _ATPPluginClass ATPPluginClass;

/*GtkDialog* atp_plugin_get_dialog (const ATPPlugin *this);
void atp_plugin_show_dialog (const ATPPlugin *this);
void atp_plugin_close_dialog(ATPPlugin *this);
void atp_plugin_set_dialog (ATPPlugin *this, GtkDialog *dialog);

GtkTreeView* atp_plugin_get_treeview(const ATPPlugin* this);
void atp_plugin_set_treeview(ATPPlugin* this, GtkTreeView* dialog);*/

struct _ATPToolList* atp_plugin_get_tool_list (const ATPPlugin *this);
struct _ATPToolDialog* atp_plugin_get_tool_dialog (const ATPPlugin *this);
ATPVariable* atp_plugin_get_variable (const ATPPlugin *this);
GtkWindow* atp_plugin_get_app_window (const ATPPlugin *this);

IAnjutaMessageView* atp_plugin_create_view (ATPPlugin* this);
void atp_plugin_append_view (ATPPlugin* this, const gchar* text);
void atp_plugin_print_view (ATPPlugin* this, IAnjutaMessageViewType type, const gchar* summary, const gchar* details);

AnjutaLauncher* atp_plugin_get_launcher (ATPPlugin* this);
#endif
