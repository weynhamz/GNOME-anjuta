/*
    cpu_registers.c
    Copyright (C) 2000  Kh. Naba Kumar Singh

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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "registers.h"

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gnome.h>

/*#define DEBUG*/
#include <libanjuta/resources.h>
#include <libanjuta/anjuta-debug.h>

#include "utilities.h"

/* Constants
 *---------------------------------------------------------------------------*/

#define REGISTER_UNMODIFIED_COLOR  "black"
#define REGISTER_MODIFIED_COLOR    "red"
#define REGISTER_UNKNOWN_VALUE     "??"

/* Types
 *---------------------------------------------------------------------------*/

typedef struct _DmaThreadRegisterList
{
	GtkTreeModel *model;
	guint thread;
	guint last_update;
} DmaThreadRegisterList;

struct _CpuRegisters
{
	IAnjutaDebugger *debugger;
	AnjutaPlugin *plugin;
	DmaThreadRegisterList* current;
	GList *list;
	GtkTreeView *treeview;
	GtkWidget *scrolledwindow;
	guint current_update;
};

enum
{
	REGISTER_MODIFIED = 1 << 0,
};

enum
{
	NUMBER_COLUMN,
	NAME_COLUMN,
	VALUE_COLUMN,
	FLAG_COLUMN,
	COLUMNS_NB
};

/* Private functions
 *---------------------------------------------------------------------------*/

/* Handle list of registers for each thread
 *---------------------------------------------------------------------------*/

static gboolean
on_copy_register_name (GtkTreeModel *model,
                       GtkTreePath *path,
					   GtkTreeIter *iter,
                       gpointer user_data)
{
	GtkListStore *list = (GtkListStore *)user_data;
	GtkTreeIter dest;
	guint num;
	gchar *name;
	
	gtk_tree_model_get(model, iter,
						NUMBER_COLUMN, &num,
						NAME_COLUMN, &name,
 					    -1);	
	
	gtk_list_store_append (list, &dest);
	gtk_list_store_set(list, &dest,
						   NUMBER_COLUMN, num,
						   NAME_COLUMN, name,
						   VALUE_COLUMN, REGISTER_UNKNOWN_VALUE,
						   FLAG_COLUMN, 0,
						   -1);
	g_free (name);
	
	return FALSE;
}

static void
on_cpu_registers_updated (const GList *registers, gpointer user_data, GError *error)
{
	CpuRegisters *self = (CpuRegisters *)user_data;
	const GList *node;
	
	GtkListStore *list;
	GtkTreeIter iter;
	gboolean valid;

	/* Get first item in list view */
	valid = gtk_tree_model_get_iter_first (self->current->model, &iter);
	list = GTK_LIST_STORE (self->current->model);

	if (registers == NULL) return;
	
	self->current->last_update = self->current_update;
	
	for(node = registers;node != NULL; node = g_list_next (node))
	{
		IAnjutaDebuggerRegister *reg = (IAnjutaDebuggerRegister *)node->data;
		guint id;
		gchar *value;
		
		/* Look for corresponding item in list view */
		for (;valid; valid = gtk_tree_model_iter_next (self->current->model, &iter))
		{
			gtk_tree_model_get (self->current->model, &iter, NUMBER_COLUMN, &id, -1);
			if (id >= reg->num) break;
		}
		
		if (!valid)
		{
			/* Append new item in list view */
			gtk_list_store_append (list, &iter);
			gtk_list_store_set(list, &iter,
							   NUMBER_COLUMN, reg->num,
							   NAME_COLUMN, reg->name,
							   VALUE_COLUMN, reg->value == NULL ? REGISTER_UNKNOWN_VALUE : reg->value,
							   FLAG_COLUMN, 0,
							   -1);
		}
		else if (id != reg->num)
		{
			/* Insert new item in list view */
			gtk_list_store_insert_before (list, &iter, &iter);
			gtk_list_store_set(list, &iter,
							   NUMBER_COLUMN, reg->num,
							   NAME_COLUMN, reg->name,
							   VALUE_COLUMN, reg->value == NULL ? REGISTER_UNKNOWN_VALUE : reg->value,
							   FLAG_COLUMN, 0,
							   -1);
		}
		else
		{
			/* Update name */
			if (reg->name != NULL)
			{
				gtk_list_store_set (list, &iter, NAME_COLUMN, reg->name, -1);
			}
			/* Update value */
			if (reg->value != NULL)
			{
				gtk_tree_model_get (self->current->model, &iter, VALUE_COLUMN, &value, -1);
				if ((value != NULL) && (strcmp (value, reg->value) == 0))
				{
					/* Same value */
					gtk_list_store_set (list, &iter, FLAG_COLUMN, 0, -1);
				}
				else
				{
					/* New value */
					gtk_list_store_set (list, &iter, VALUE_COLUMN, reg->value,
										FLAG_COLUMN, REGISTER_MODIFIED,
										-1);
				}
				if (value != NULL) g_free (value);
			}
		}
	}
}

static void
cpu_registers_update (CpuRegisters *self)
{
	if ((self->debugger != NULL) && GTK_WIDGET_MAPPED (self->scrolledwindow))
	{
		ianjuta_cpu_debugger_update_register (
				IANJUTA_CPU_DEBUGGER (self->debugger),
				(IAnjutaDebuggerCallback)on_cpu_registers_updated,
				self,
				NULL);
	}
}

static DmaThreadRegisterList*
dma_thread_create_new_register_list (CpuRegisters *self, guint thread)
{
	DmaThreadRegisterList *regs;
	GtkListStore *store;
	
	if ((self->list != NULL)
		&& (((DmaThreadRegisterList *)g_list_first (self->list)->data)->thread == 0))
	{
		/* List as been created but not associated to a thread */
		
		regs = ((DmaThreadRegisterList *)g_list_first (self->list)->data);
		
		regs->thread = thread;
		
		return regs;
	}
	
	/* Create new list store */
	store = gtk_list_store_new (COLUMNS_NB,
								G_TYPE_UINT,
								G_TYPE_STRING,
								G_TYPE_STRING,
								G_TYPE_UINT);
		
	regs = g_new (DmaThreadRegisterList, 1);
	regs->thread = thread;
	regs->model = GTK_TREE_MODEL (store);
	regs->last_update = 0;
		
	if (self->list == NULL)
	{
		self->current = regs;
		
		/* List is empty, ask debugger to get all register name */
		ianjuta_cpu_debugger_list_register (
				IANJUTA_CPU_DEBUGGER (self->debugger),
				(IAnjutaDebuggerCallback)on_cpu_registers_updated,
				self,
				NULL);
	}
	else
	{
		/* One register list already exist, copy register name and ID */
		
		gtk_tree_model_foreach (((DmaThreadRegisterList *)g_list_first (self->list)->data)->model,
							on_copy_register_name, store);
	}
	self->list = g_list_append (self->list, regs);
	
	return regs;
}

static void
on_clear_register_list (gpointer data, gpointer user_data)
{
	DmaThreadRegisterList *regs = (DmaThreadRegisterList *) data;
	
	g_object_unref (G_OBJECT (regs->model));
	g_free (regs);
}

static void
dma_thread_clear_all_register_list (CpuRegisters *self)
{
	/* Clear all GtkListStore */
	g_list_foreach (self->list, (GFunc)on_clear_register_list, NULL);
	g_list_free (self->list);
	
	self->current = NULL;
	self->list = NULL;
}

static gboolean
on_find_register_list (gconstpointer a, gconstpointer b)
{
	const DmaThreadRegisterList *regs = (const DmaThreadRegisterList *)a;
	guint thread = (guint)b;
	
	return regs->thread != thread;
}

static void
dma_thread_set_register_list (CpuRegisters *self, guint thread)
{
	GList *list;
	DmaThreadRegisterList *regs;

	if (self->current->thread != thread)
	{
		list = g_list_find_custom (self->list, (gconstpointer) thread, on_find_register_list);
	
		if (list == NULL)
		{
			/* Create new find */
			regs = dma_thread_create_new_register_list(self, thread);
		}
		else
		{
			regs = (DmaThreadRegisterList *)list->data;
		}
		self->current = regs;
		gtk_tree_view_set_model (self->treeview, regs->model);
	}
	
	if (self->current_update != self->current->last_update)
	{
			cpu_registers_update (self);
	}
}

static void
on_cpu_registers_changed (GtkCellRendererText *cell,
						  gchar *path_string,
                          gchar *text,
                          gpointer user_data)
{
	CpuRegisters *self = (CpuRegisters *)user_data;
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter_from_string (self->current->model, &iter, path_string))
	{
		IAnjutaDebuggerRegister reg;
		gchar *name;
		
		gtk_tree_model_get (self->current->model, &iter, NUMBER_COLUMN, &reg.num, NAME_COLUMN, &name, -1);
		reg.value = text;
		reg.name = name;
		
		ianjuta_cpu_debugger_write_register (IANJUTA_CPU_DEBUGGER (self->debugger), &reg, NULL);
		ianjuta_cpu_debugger_update_register (
				IANJUTA_CPU_DEBUGGER (self->debugger),
				(IAnjutaDebuggerCallback)on_cpu_registers_updated,
				self,
				NULL);
		g_free (name);
	}
}

static void
on_program_stopped (CpuRegisters *self, guint thread)
{
	self->current_update++;
	dma_thread_set_register_list (self, thread);
}

static void
on_map (GtkWidget* widget, CpuRegisters *self)
{
	if (ianjuta_debugger_get_status (self->debugger, NULL) == IANJUTA_DEBUGGER_PROGRAM_STOPPED)
	{
		cpu_registers_update (self);
	}
}

static void
cpu_registers_value_cell_data_func (GtkTreeViewColumn *tree_column,
					GtkCellRenderer *cell, GtkTreeModel *tree_model,
					GtkTreeIter *iter, gpointer data)
{
	gchar *value;
	guint flag;
	GValue gvalue = {0, };

	gtk_tree_model_get (tree_model, iter, VALUE_COLUMN, &value, FLAG_COLUMN, &flag, -1);
	
	g_value_init (&gvalue, G_TYPE_STRING);
	g_value_set_static_string (&gvalue, value);
	g_object_set_property (G_OBJECT (cell), "text", &gvalue);
	g_free (value);
	
	g_value_reset (&gvalue);
	g_value_set_static_string (&gvalue, flag & REGISTER_MODIFIED ? REGISTER_MODIFIED_COLOR : REGISTER_UNMODIFIED_COLOR);
	g_object_set_property (G_OBJECT (cell), "foreground", &gvalue);
}


static void
create_cpu_registers_gui (CpuRegisters *self)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	/* Create list store */
	dma_thread_create_new_register_list (self, 0);
	
	/* Create list view */
	self->treeview = GTK_TREE_VIEW (gtk_tree_view_new_with_model (self->current->model));
	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (self->treeview),
					     GTK_SELECTION_SINGLE);
	
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (N_("Register"),
												renderer, "text", NAME_COLUMN, NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (self->treeview, column);
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, "editable", TRUE, NULL);	
	g_signal_connect(renderer, "edited", (GCallback) on_cpu_registers_changed, self);
	column = gtk_tree_view_column_new_with_attributes (N_("Value"),
												renderer, NULL);
	gtk_tree_view_column_set_cell_data_func (column, renderer,
											 cpu_registers_value_cell_data_func,
											 NULL, NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (self->treeview, column);
	
	/* Add register window */
	self->scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (self->scrolledwindow),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (self->scrolledwindow),
									 GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (self->scrolledwindow), GTK_WIDGET (self->treeview));
	g_signal_connect(self->scrolledwindow, "map", (GCallback) on_map, self);

	anjuta_shell_add_widget (self->plugin->shell,
							 self->scrolledwindow,
                             "AnjutaDebuggerRegisters", _("Registers"),
                             NULL, ANJUTA_SHELL_PLACEMENT_LEFT,
							 NULL);

}

static void
on_debugger_started (CpuRegisters *self)
{
	create_cpu_registers_gui (self);
	self->current_update = 0;
}

static void
destroy_cpu_registers_gui (CpuRegisters *self)
{
	if (self->scrolledwindow != NULL)
	{
		dma_thread_clear_all_register_list (self);
		gtk_widget_destroy (self->scrolledwindow);
		self->scrolledwindow = NULL;
	}
}

static void
on_debugger_stopped (CpuRegisters *self)
{
	destroy_cpu_registers_gui (self);
}

static void
on_frame_changed (CpuRegisters *self, guint frame, guint thread)
{
	dma_thread_set_register_list (self, thread);
}

/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

CpuRegisters*
cpu_registers_new(AnjutaPlugin *plugin, IAnjutaDebugger *debugger)
{
	CpuRegisters* self;
	
	self = g_new0 (CpuRegisters, 1);
	
	self->debugger = debugger;
	if (debugger != NULL) g_object_ref (debugger);
	self->plugin = plugin;
	self->list = NULL;
	
	g_signal_connect_swapped (self->debugger, "debugger-started", G_CALLBACK (on_debugger_started), self);
	g_signal_connect_swapped (self->debugger, "debugger-stopped", G_CALLBACK (on_debugger_stopped), self);
	g_signal_connect_swapped (self->debugger, "program-stopped", G_CALLBACK (on_program_stopped), self);
	g_signal_connect_swapped (self->debugger, "frame_changed", G_CALLBACK (on_frame_changed), self);
	
	return self;
}

void
cpu_registers_free(CpuRegisters* self)
{
	g_return_if_fail (self != NULL);

	destroy_cpu_registers_gui (self);
	if (self->debugger != NULL)
	{
		g_signal_handlers_disconnect_by_func (self->debugger, G_CALLBACK (on_debugger_started), self);
		g_signal_handlers_disconnect_by_func (self->debugger, G_CALLBACK (on_debugger_stopped), self);
		g_signal_handlers_disconnect_by_func (self->debugger, G_CALLBACK (on_program_stopped), self);
		g_signal_handlers_disconnect_by_func (self->debugger, G_CALLBACK (on_frame_changed), self);
		g_object_unref (self->debugger);
	}
	g_free(self);
}
