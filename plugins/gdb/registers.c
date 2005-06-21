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

enum
{
	REG_NUM_COL,
	REG_REG_COL,
	REG_VAL_COL,
	N_COLS
};

static void
add_register (const GDBMIValue *reg_literal, CpuRegisters *cr)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	const gchar *reg;
	gint count;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (cr->widgets.clist));
	count = gtk_tree_model_iter_n_children (model, NULL);
	reg = gdbmi_value_literal_get (reg_literal);
	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	if (reg && strlen (reg) > 0)
	{
		gchar num[32];
		snprintf (num, 32, "%d", count);
		gtk_list_store_set(GTK_LIST_STORE(model), &iter,
						   REG_NUM_COL, num,
						   REG_REG_COL, reg,
						   -1);
	}
}

static void
cpu_registers_update_names (Debugger *debugger, const GDBMIValue *mi_results,
							const GList *lines, gpointer data)
{
	CpuRegisters *cr;
	const GDBMIValue *reg_list;
	
	cr = (CpuRegisters *) data;
	cpu_registers_clear (cr);

	if (!mi_results)
		return;
	
	reg_list = gdbmi_value_hash_lookup (mi_results, "register-names");
	if (reg_list)
		gdbmi_value_foreach (reg_list, (GFunc)add_register, cr);
}

static void
update_register (const GDBMIValue *reg_hash, CpuRegisters *cr)
{
	const GDBMIValue *literal;
	GtkTreeModel *model;
	GtkTreeIter iter;
	const gchar *num, *val;
	gint idx;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (cr->widgets.clist));
	
	literal = gdbmi_value_hash_lookup (reg_hash, "number");
	if (!literal)
		return;
	num = gdbmi_value_literal_get (literal);
	if (!num)
		return;
	idx = atoi (num);
	
	literal = gdbmi_value_hash_lookup (reg_hash, "value");
	if (!literal)
		return;
	val = gdbmi_value_literal_get (literal);
	if (!val)
		return;
	
	if (gtk_tree_model_iter_nth_child (model, &iter, NULL, idx))
	{
		gtk_list_store_set(GTK_LIST_STORE(model), &iter,
						   REG_VAL_COL, val, -1);
	}
}

static void
cpu_registers_update_values (Debugger *debugger, const GDBMIValue *mi_results,
							 const GList *lines, gpointer data)
{
	CpuRegisters *cr;
	const GDBMIValue *reg_list;
	
	cr = (CpuRegisters *) data;

	if (!mi_results)
		return;
	
	reg_list = gdbmi_value_hash_lookup (mi_results, "register-values");
	if (reg_list)
		gdbmi_value_foreach (reg_list, (GFunc)update_register, cr);
}

static gint
on_registers_delete_event(GtkWidget* w, GdkEvent *event, gpointer data)
{
	CpuRegisters* cr = data;
	cpu_registers_hide(cr);
	return TRUE;
}

static gboolean
on_registers_key_press_event(GtkWidget *widget, GdkEventKey *event,
							 gpointer data)
{
	if (event->keyval == GDK_Escape)
	{
		CpuRegisters* cr = data;
  		cpu_registers_hide(cr);
		return TRUE;
	}
	return FALSE;
}

static void
on_registers_selecton_changed (GtkTreeSelection *selection, gpointer user_data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
    CpuRegisters *cr;
    cr = (CpuRegisters *)user_data;
	
	if (gtk_tree_selection_get_selected (selection, &model, &iter))
	{
		gchar *num;
		
		gtk_tree_model_get (model, &iter, REG_NUM_COL, &num, -1);
		if (num)
			cr->current_index = atoi (num);
		else
			cr->current_index = -1;
		g_free (num);
	}
	else
	{
		cr->current_index = -1;
	}
}

static void
on_register_modify_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}

static void
on_register_update_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	CpuRegisters *cr = (CpuRegisters*)user_data;
	
	debugger_command (cr->debugger, "-data-list-register-names", FALSE,
					  cpu_registers_update_names, cr);
	debugger_command (cr->debugger, "-data-list-register-values N", FALSE,
					  cpu_registers_update_values, cr);
}

static gboolean
on_register_event                (GtkWidget       *widget,
								  GdkEvent        *event,
								  gpointer         user_data)
{
	GdkEventButton *bevent;
	CpuRegisters* ew = (CpuRegisters*)user_data;
	
	if (event->type != GDK_BUTTON_PRESS)
		return FALSE;
	if (((GdkEventButton *)event)->button != 3) return FALSE;
		bevent =  (GdkEventButton *)event;
	bevent->button = 1;
	registers_update_controls(ew);
	gtk_menu_popup (GTK_MENU(ew->widgets.menu), NULL,
					NULL, NULL, NULL,
					bevent->button, bevent->time);
	return TRUE;
}

static GnomeUIInfo register_menu_uiinfo[] =
{
  {
    GNOME_APP_UI_ITEM, N_("Modify"),
    NULL,
    on_register_modify_activate, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  {
    GNOME_APP_UI_ITEM, N_("Update"),
    NULL,
    on_register_update_activate, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  GNOMEUIINFO_END
};

static GtkWidget*
create_register_menu (CpuRegisters *cr)
{
	GtkWidget *register_menu;

	register_menu = gtk_menu_new ();
	register_menu_uiinfo[0].user_data = cr;
	register_menu_uiinfo[1].user_data = cr;
	
	gnome_app_fill_menu (GTK_MENU_SHELL (register_menu), register_menu_uiinfo,
						 NULL, FALSE, 0);
	return register_menu;
}

static void
create_cpu_registers_gui (CpuRegisters *cr)
{
	GtkWidget *window3, *scrolledwindow;
	GtkTreeModel* model;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	
	window3 = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_usize (window3, 170, -2);
	gtk_window_set_title (GTK_WINDOW (window3), _("CPU Registers"));
	gtk_window_set_wmclass (GTK_WINDOW (window3), "cpu_registers", "Anjuta");
	gtk_window_set_default_size (GTK_WINDOW (window3), 240, 230);
	gnome_window_icon_set_from_default(GTK_WINDOW(window3));
	
	model = GTK_TREE_MODEL(gtk_list_store_new (N_COLS,
											   G_TYPE_STRING,
											   G_TYPE_STRING,
											   G_TYPE_STRING));
	scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow),
										 GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (window3), scrolledwindow);
	
	cr->widgets.clist = gtk_tree_view_new_with_model (model);
	gtk_widget_show (cr->widgets.clist);
	gtk_container_add (GTK_CONTAINER (scrolledwindow),
					   cr->widgets.clist);
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (cr->widgets.clist));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	g_object_unref (G_OBJECT (model));
	
	/* Columns */
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Num"));
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
										REG_NUM_COL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (cr->widgets.clist), column);
	gtk_tree_view_set_expander_column (GTK_TREE_VIEW (cr->widgets.clist),
									   column);
	
	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
										REG_REG_COL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Register"));
	gtk_tree_view_append_column (GTK_TREE_VIEW (cr->widgets.clist), column);

	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
										REG_VAL_COL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Value"));
	gtk_tree_view_append_column (GTK_TREE_VIEW (cr->widgets.clist), column);
	
	g_signal_connect (cr->widgets.clist, "event",
					  G_CALLBACK (on_register_event), cr);  
	g_signal_connect (selection, "changed",
					  G_CALLBACK (on_registers_selecton_changed), cr);  
	gtk_signal_connect (GTK_OBJECT (window3), "delete_event",
						GTK_SIGNAL_FUNC (on_registers_delete_event), cr);
	gtk_signal_connect (GTK_OBJECT (window3), "key-press-event",
						GTK_SIGNAL_FUNC (on_registers_key_press_event), cr);

  cr->widgets.window = window3;
  cr->widgets.menu = create_register_menu (cr);
  cr->widgets.menu_modify = register_menu_uiinfo[0].widget;
  cr->widgets.menu_update = register_menu_uiinfo[1].widget;
}

static void
on_program_stopped (Debugger *debugger, const GDBMIValue *mi_results,
					CpuRegisters *cr)
{
	if (cr->is_showing)
	{
		debugger_command (debugger, "-data-list-register-names", TRUE,
						  cpu_registers_update_names, cr);
		debugger_command (debugger, "-data-list-register-values N", TRUE,
						  cpu_registers_update_values, cr);
	}
}

CpuRegisters*
cpu_registers_new(Debugger *debugger)
{
	CpuRegisters* ew;
	ew = g_malloc(sizeof(CpuRegisters));
	if(ew)
	{
		ew->debugger = debugger;
		create_cpu_registers_gui(ew);
		ew->current_index = 0;
		ew->is_showing = FALSE;
		ew->win_pos_x = 250;
		ew->win_pos_y = 100;
		ew->win_width = 200;
		ew->win_height = 300;
		
		g_object_ref (debugger);
		g_signal_connect (debugger, "program-stopped",
						  G_CALLBACK (on_program_stopped), ew);
		g_signal_connect_swapped (debugger, "program-exited",
								  G_CALLBACK (cpu_registers_clear), ew);
	}
	return ew;
}

void
cpu_registers_clear(CpuRegisters *ew)
{
	GtkTreeModel *model;
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ew->widgets.clist));
	gtk_list_store_clear (GTK_LIST_STORE (model));
}

void
cpu_registers_show(CpuRegisters* ew)
{
	if(ew)
	{
		if(ew->is_showing)
		{
			gdk_window_raise(ew->widgets.window->window);
		}
		else
		{
			gtk_widget_set_uposition(ew->widgets.window, ew->win_pos_x,
									 ew->win_pos_y);
			gtk_window_set_default_size(GTK_WINDOW(ew->widgets.window),
										ew->win_width, ew->win_height);
			gtk_widget_show(ew->widgets.window);
			debugger_command (ew->debugger, "-data-list-register-names", TRUE,
							  cpu_registers_update_names, ew);
			debugger_command (ew->debugger, "-data-list-register-values N", TRUE,
							  cpu_registers_update_values, ew);
			
			ew->is_showing = TRUE;
		}
	}
}

void
cpu_registers_hide(CpuRegisters* ew)
{
	if(ew)
	{
		if(ew->is_showing == FALSE) return;
			gdk_window_get_root_origin(ew ->widgets.window->window,
									   &ew->win_pos_x, &ew->win_pos_y);
		gdk_window_get_size(ew ->widgets.window->window,
							&ew->win_width, &ew->win_height);
		gtk_widget_hide(ew->widgets.window);
		ew->is_showing = FALSE;
	}
}

void
cpu_registers_destroy(CpuRegisters* cr)
{
	if(cr)
	{
		g_signal_handlers_disconnect_by_func (cr->debugger,
											  G_CALLBACK (on_program_stopped),
											  cr);
		g_signal_handlers_disconnect_by_func (cr->debugger,
											  G_CALLBACK (cpu_registers_clear),
											  cr);
		g_object_unref (cr->debugger);
		cpu_registers_clear(cr);
		gtk_widget_destroy(cr->widgets.window);
		gtk_widget_destroy(cr->widgets.menu);
		g_free(cr);
	}
}

void
registers_update_controls(CpuRegisters* ew)
{
	gboolean R;
	
	R = debugger_is_ready(ew->debugger);
	
	gtk_widget_set_sensitive(ew->widgets.menu_modify, R);
	gtk_widget_set_sensitive(ew->widgets.menu_update, R);
}
