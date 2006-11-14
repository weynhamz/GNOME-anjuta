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

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gnome.h>

#include <libanjuta/resources.h>
#include "utilities.h"
#include "registers.h"

/* Constants
 *---------------------------------------------------------------------------*/

#define REGISTER_UNMODIFIED_COLOR  "black"
#define REGISTER_MODIFIED_COLOR    "red"
#define REGISTER_UNKNOWN_VALUE     "??"

/* Types
 *---------------------------------------------------------------------------*/

struct _CpuRegisters
{
	IAnjutaDebugger *debugger;
	AnjutaPlugin *plugin;
	GtkTreeModel *model;
	GtkWidget *scrolledwindow;
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

static void
on_cpu_registers_updated (const GList *registers, gpointer user_data, GError *error)
{
	CpuRegisters *this = (CpuRegisters *)user_data;
	const GList *node;
	
	GtkListStore *list;
	GtkTreeIter iter;
	gboolean valid;

	/* Get first item in list view */
	valid = gtk_tree_model_get_iter_first (this->model, &iter);
	list = GTK_LIST_STORE (this->model);
	
	for(node = registers;node != NULL; node = g_list_next (node))
	{
		IAnjutaDebuggerRegister *reg = (IAnjutaDebuggerRegister *)node->data;
		guint id;
		gchar *value;
		
		/* Look for corresponding item in list view */
		for (;valid; valid = gtk_tree_model_iter_next (this->model, &iter))
		{
			gtk_tree_model_get (this->model, &iter, NUMBER_COLUMN, &id, -1);
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
				gtk_tree_model_get (this->model, &iter, VALUE_COLUMN, &value, -1);
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
on_cpu_registers_changed (GtkCellRendererText *cell,
						  gchar *path_string,
                          gchar *text,
                          gpointer user_data)
{
	CpuRegisters *this = (CpuRegisters *)user_data;
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter_from_string (this->model, &iter, path_string))
	{
		IAnjutaDebuggerRegister reg;
		gchar *name;
		
		gtk_tree_model_get (this->model, &iter, NUMBER_COLUMN, &reg.num, NAME_COLUMN, &name, -1);
		reg.value = text;
		reg.name = name;
		
		ianjuta_cpu_debugger_write_register (IANJUTA_CPU_DEBUGGER (this->debugger), &reg, NULL);
		ianjuta_cpu_debugger_update_register (
				IANJUTA_CPU_DEBUGGER (this->debugger),
				(IAnjutaDebuggerCallback)on_cpu_registers_updated,
				this,
				NULL);
		g_free (name);
	}
}

static void
cpu_registers_update (CpuRegisters *this)
{
	if ((this->debugger != NULL) && GTK_WIDGET_MAPPED (this->scrolledwindow))
	{
		ianjuta_cpu_debugger_update_register (
				IANJUTA_CPU_DEBUGGER (this->debugger),
				(IAnjutaDebuggerCallback)on_cpu_registers_updated,
				this,
				NULL);
	}
}

static void
on_program_stopped (CpuRegisters *this)
{
	cpu_registers_update (this);
}

static void
on_map (GtkWidget* widget, CpuRegisters *this)
{
	if (ianjuta_debugger_get_status (this->debugger, NULL) == IANJUTA_DEBUGGER_PROGRAM_STOPPED)
	{
		cpu_registers_update (this);
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
create_cpu_registers_gui (CpuRegisters *this)
{
	GtkListStore *store;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeView *treeview;

	/* Create list store */
	store = gtk_list_store_new (COLUMNS_NB,
								G_TYPE_UINT,
								G_TYPE_STRING,
								G_TYPE_STRING,
								G_TYPE_UINT);
	this->model = GTK_TREE_MODEL (store);
	
	/* Create list view */
	treeview = GTK_TREE_VIEW (gtk_tree_view_new_with_model (GTK_TREE_MODEL (store)));
	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (treeview),
					     GTK_SELECTION_SINGLE);
	g_object_unref (G_OBJECT (store));
	
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (N_("Register"),
												renderer, "text", NAME_COLUMN, NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (treeview, column);
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, "editable", TRUE, NULL);	
	g_signal_connect(renderer, "edited", (GCallback) on_cpu_registers_changed, this);
	column = gtk_tree_view_column_new_with_attributes (N_("Value"),
												renderer, NULL);
	gtk_tree_view_column_set_cell_data_func (column, renderer,
											 cpu_registers_value_cell_data_func,
											 NULL, NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (treeview, column);
	
	/* Add register window */
	this->scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (this->scrolledwindow),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (this->scrolledwindow),
									 GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (this->scrolledwindow), GTK_WIDGET (treeview));
	g_signal_connect(this->scrolledwindow, "map", (GCallback) on_map, this);

	anjuta_shell_add_widget (this->plugin->shell,
							 this->scrolledwindow,
                             "AnjutaDebuggerRegisters", _("Registers"),
                             NULL, ANJUTA_SHELL_PLACEMENT_LEFT,
							 NULL);

}

static void
on_debugger_started (CpuRegisters *this)
{
	create_cpu_registers_gui (this);
	ianjuta_cpu_debugger_list_register (
			IANJUTA_CPU_DEBUGGER (this->debugger),
			(IAnjutaDebuggerCallback)on_cpu_registers_updated,
			this,
			NULL);
}

static void
destroy_cpu_registers_gui (CpuRegisters *this)
{
	if (this->scrolledwindow != NULL)
	{
		gtk_list_store_clear (GTK_LIST_STORE (this->model));
		gtk_widget_destroy (this->scrolledwindow);
		this->scrolledwindow = NULL;
	}
}

static void
on_debugger_stopped (CpuRegisters *this)
{
	destroy_cpu_registers_gui (this);
}

/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

CpuRegisters*
cpu_registers_new(AnjutaPlugin *plugin, IAnjutaDebugger *debugger)
{
	CpuRegisters* this;
	
	this = g_new0 (CpuRegisters, 1);
	
	this->debugger = debugger;
	if (debugger != NULL) g_object_ref (debugger);
	this->plugin = plugin;
	
	g_signal_connect_swapped (this->debugger, "debugger-started", G_CALLBACK (on_debugger_started), this);
	g_signal_connect_swapped (this->debugger, "debugger-stopped", G_CALLBACK (on_debugger_stopped), this);
	g_signal_connect_swapped (this->debugger, "program-stopped", G_CALLBACK (on_program_stopped), this);
	
	return this;
}

void
cpu_registers_free(CpuRegisters* this)
{
	g_return_if_fail (this != NULL);

	destroy_cpu_registers_gui (this);
	if (this->debugger != NULL)
	{
		g_signal_handlers_disconnect_by_func (this->debugger, G_CALLBACK (on_debugger_started), this);
		g_signal_handlers_disconnect_by_func (this->debugger, G_CALLBACK (on_debugger_stopped), this);
		g_signal_handlers_disconnect_by_func (this->debugger, G_CALLBACK (on_program_stopped), this);
		g_object_unref (this->debugger);
	}
	g_free(this);
}
