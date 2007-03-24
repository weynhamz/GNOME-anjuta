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
#include <libanjuta/interfaces/ianjuta-variable-debugger.h>
#include <libanjuta/anjuta-plugin.h>

G_BEGIN_DECLS

typedef struct _DebugTree DebugTree;

DebugTree* debug_tree_new (AnjutaPlugin* plugin);
DebugTree* debug_tree_new_with_view (AnjutaPlugin *plugin, GtkTreeView *view);
void debug_tree_free (DebugTree *tree);

void debug_tree_connect (DebugTree *tree, IAnjutaDebugger *debugger);
void debug_tree_disconnect (DebugTree *tree);

void debug_tree_remove_all (DebugTree *tree);
void debug_tree_replace_list (DebugTree *tree, const GList *expressions);
void debug_tree_add_watch (DebugTree *tree, const IAnjutaDebuggerVariable* var, gboolean auto_update);
void debug_tree_add_dummy (DebugTree *tree, GtkTreeIter *parent);
void debug_tree_add_full_watch_list (DebugTree *tree, GList *expressions);
void debug_tree_add_watch_list (DebugTree *tree, GList *expressions, gboolean auto_update);
void debug_tree_update_all (DebugTree *tree);

GList* debug_tree_get_full_watch_list (DebugTree *tree);


GtkWidget *debug_tree_get_tree_widget (DebugTree *tree);
gboolean debug_tree_get_current (DebugTree *tree, GtkTreeIter* iter);
gboolean debug_tree_remove (DebugTree *tree, GtkTreeIter* iter);
gboolean debug_tree_update (DebugTree *tree, GtkTreeIter* iter, gboolean force);
void debug_tree_set_auto_update (DebugTree* tree, GtkTreeIter* iter, gboolean state);
gboolean debug_tree_get_auto_update (DebugTree* tree, GtkTreeIter* iter);

GtkTreeModel *debug_tree_get_model (DebugTree *tree);
void debug_tree_set_model (DebugTree *tree, GtkTreeModel *model);
void debug_tree_new_model (DebugTree *tree);
void debug_tree_remove_model (DebugTree *tree, GtkTreeModel *model);

G_END_DECLS

#endif
