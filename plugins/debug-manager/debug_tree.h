/*
    debug_tree.h
    Copyright (C) 2006  Sébastien Granjoux

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

#ifndef _DEBUG_TREE_H_
#define _DEBUG_TREE_H_

#include <gtk/gtk.h>
#include <libanjuta/interfaces/ianjuta-debugger.h>
#include <libanjuta/anjuta-plugin.h>

G_BEGIN_DECLS

typedef struct _DebugTree DebugTree;

DebugTree* debug_tree_new (AnjutaPlugin* plugin, gboolean user);
void debug_tree_free (DebugTree *this);

void debug_tree_connect (DebugTree *this, IAnjutaDebugger *debugger);
void debug_tree_disconnect (DebugTree *this);

void debug_tree_remove_all (DebugTree *this);
void debug_tree_add_watch (DebugTree *this, const gchar* expression, gboolean auto_update);
void debug_tree_add_full_watch_list (DebugTree *this, GList *expressions);
void debug_tree_add_watch_list (DebugTree *this, GList *expressions, gboolean auto_update);
void debug_tree_update_all (DebugTree *this, gboolean force);

GList* debug_tree_get_full_watch_list (DebugTree *this);


GtkWidget *debug_tree_get_tree_widget (DebugTree *this);

G_END_DECLS

#endif
