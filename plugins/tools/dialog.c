/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    dialog.c
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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*
 * First tool configuratin dialog, presenting all tools in a list.
 * In this dialog, you can enable, disable, add, remove and order tools.
 * 
 *---------------------------------------------------------------------------*/

#include <config.h>

#include "dialog.h"

#include "editor.h"
#include "fileop.h"

/*---------------------------------------------------------------------------*/

/* Widget and signal name found in glade file
 *---------------------------------------------------------------------------*/

#define TOOL_LIST "list_dialog"
#define TOOL_TREEVIEW "tools_treeview"
#define TOOL_EDIT_BUTTON "edit_bt"
#define TOOL_DELETE_BUTTON "delete_bt"
#define TOOL_UP_BUTTON "up_bt"
#define TOOL_DOWN_BUTTON "down_bt"

#define LIST_RESPONSE_SIGNAL "on_list_dialog_response"
#define TOOL_ADD_SIGNAL "on_tool_add"
#define TOOL_ACTIVATED_SIGNAL "on_tool_activated"
#define TOOL_EDIT_SIGNAL "on_tool_edit"
#define TOOL_DELETE_SIGNAL "on_tool_delete"
#define TOOL_UP_SIGNAL "on_tool_up"
#define TOOL_DOWN_SIGNAL "on_tool_down"

/* column of the list view */
enum {
	ATP_TOOLS_ENABLED_COLUMN,
	ATP_TOOLS_NAME_COLUMN,
	ATP_TOOLS_DATA_COLUMN,
	ATP_N_TOOLS_COLUMNS
};

/* Useful function used by GUI code
 *---------------------------------------------------------------------------*/

static ATPUserTool*
get_current_tool (const ATPToolDialog *this)
{
	GtkTreeModel *model;
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	ATPUserTool *tool;

	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (this->view));
	if (gtk_tree_selection_get_selected (sel, &model, &iter))
	{
		gtk_tree_model_get (model, &iter, ATP_TOOLS_DATA_COLUMN, &tool, -1);

		return tool;
	}

	return NULL;
}

static void
update_sensitivity (const ATPToolDialog *this)
{
	ATPUserTool *tool;
	gboolean tool_selected;
	gboolean tool_up;
	gboolean tool_down;
	gboolean tool_deletable;

	/* Get current selected tool */
	tool = get_current_tool (this);
	tool_selected = tool != NULL;
	tool_up = tool_selected && (atp_user_tool_previous (tool) != NULL);
	tool_down = tool_selected && (atp_user_tool_next (tool) != NULL);
	tool_deletable = tool_selected && (atp_user_tool_get_storage (tool) > ATP_TSTORE_GLOBAL);

	gtk_widget_set_sensitive(this->edit_bt, tool_selected);
	gtk_widget_set_sensitive(this->delete_bt, tool_deletable);
	gtk_widget_set_sensitive(this->up_bt, tool_up);
	gtk_widget_set_sensitive(this->down_bt, tool_down);
}

static ATPUserTool*
get_current_writable_tool (ATPToolDialog *this)
{
	ATPUserTool *tool;

	tool = get_current_tool(this);
	if (tool)
	{
		if (atp_user_tool_get_storage (tool) <= ATP_TSTORE_GLOBAL)
		{
			/* global tool are read only so create a writeable one */
			tool = atp_user_tool_clone_new (tool, ATP_TSTORE_LOCAL);
		}
	}

	return tool;
}

/* Call backs
 *---------------------------------------------------------------------------*/

static void
on_tool_add (GtkButton *button, gpointer user_data)
{
	ATPToolDialog *this = (ATPToolDialog *)user_data;
	ATPUserTool *tool;
	ATPToolEditor *ted;

	tool = get_current_tool (this);

	if (tool == NULL)
	{
		/* Add the new tool at the end of the list */
		tool = atp_tool_list_append_new (atp_plugin_get_tool_list (this->plugin), NULL, ATP_TSTORE_LOCAL);
	}
	else
	{
		/* Add tool after the current one */
		tool = atp_user_tool_append_new (tool, NULL, ATP_TSTORE_LOCAL);
	}

	ted = atp_tool_editor_new (tool, &this->tedl, this);
	atp_tool_editor_show (ted);
}

static void
on_tool_edit (GtkButton *button, gpointer user_data)
{
	ATPToolDialog *this = (ATPToolDialog *)user_data;
	ATPUserTool *tool;
	ATPToolEditor *ted;

	/* Warning button could be NULL (called from on_row_activated) */
	
	tool = get_current_writable_tool (this);
	if (tool != NULL)
	{
		ted = atp_tool_editor_new (tool, &this->tedl, this);
		atp_tool_editor_show (ted);
	}
}

static void
on_tool_delete (GtkButton *button, gpointer user_data)
{
	ATPToolDialog *this = (ATPToolDialog *)user_data;
	ATPUserTool *tool;

	tool = get_current_tool (this);
	if ((tool != NULL) && (atp_user_tool_get_storage (tool) > ATP_TSTORE_GLOBAL))
	{
		if (anjuta_util_dialog_boolean_question (GTK_WINDOW (this->dialog), _("Are you sure you want to delete the '%s' tool?"), atp_user_tool_get_name(tool)))
		{
			atp_user_tool_free (tool);
			atp_tool_dialog_refresh (this, NULL);
		}
	}	
}

static void
on_tool_up (GtkButton *button, gpointer user_data)
{
	ATPToolDialog *this = (ATPToolDialog *)user_data;
	ATPUserTool *tool;
	ATPUserTool *prev;

	tool = get_current_writable_tool (this);
	if (tool != NULL)
	{
		prev = atp_user_tool_previous (tool);
		if (prev != NULL)
		{
			if (atp_user_tool_get_storage (prev) <= ATP_TSTORE_GLOBAL)
			{
				/* global tool are read only so create a writeable one */
				prev = atp_user_tool_clone_new (prev, ATP_TSTORE_LOCAL);
			}
			atp_user_tool_move_after (prev, tool);
			atp_tool_dialog_refresh (this, atp_user_tool_get_name (tool));
		}
	}
}

static void
on_tool_down (GtkButton *button, gpointer user_data)
{
	ATPToolDialog *this = (ATPToolDialog *)user_data;
	ATPUserTool *tool;
	ATPUserTool *next;

	tool = get_current_writable_tool (this);
	if (tool != NULL)
	{
		next = atp_user_tool_next (tool);
		if (next != NULL)
		{
			atp_user_tool_move_after (tool, next);
			atp_tool_dialog_refresh (this, atp_user_tool_get_name (tool));
		}
	}
}

static void
on_tool_enable (GtkCellRendererToggle *cell_renderer, const gchar *path, gpointer user_data)
{
	ATPToolDialog *this = (ATPToolDialog *)user_data;
	GtkTreeModel *model;
	GtkTreeIter iter;
	ATPUserTool *tool;

	model = gtk_tree_view_get_model (this->view);
	if (gtk_tree_model_get_iter_from_string (model, &iter, path))
	{
		gtk_tree_model_get (model, &iter, ATP_TOOLS_DATA_COLUMN, &tool, -1);
		atp_user_tool_set_flag (tool, ATP_TOOL_ENABLE | ATP_TOGGLE);

		gtk_list_store_set (GTK_LIST_STORE(model), &iter, ATP_TOOLS_ENABLED_COLUMN,
				atp_user_tool_get_flag (tool, ATP_TOOL_ENABLE), -1);
	}	
}

static void
on_tool_activated (GtkTreeView  *treeview, GtkTreePath *path, GtkTreeViewColumn *col, gpointer user_data)
{
	on_tool_edit (NULL, user_data);
}

static void
on_tool_selection_changed (GtkTreeSelection *selection, gpointer user_data)
{
	ATPToolDialog *this = (ATPToolDialog *)user_data;

	update_sensitivity (this);
}

static void
on_list_response (GtkDialog *dialog, gint res, gpointer user_data)
{
	ATPToolDialog *this = (ATPToolDialog *)user_data;

	/* Just save tool list */
	atp_anjuta_tools_save(this->plugin);

	/* Close widget */
	atp_tool_dialog_close (this);
}

/* Public functions
 *---------------------------------------------------------------------------*/

void
atp_tool_dialog_refresh (const ATPToolDialog *this, const gchar* select)
{
	ATPUserTool *tool;
	GtkTreeModel *model;
	GtkTreeSelection *selection;

	/* Disable changed signal to avoid changing buttons sensitivity */
	selection = gtk_tree_view_get_selection (this->view);
	g_signal_handler_block (selection, this->changed_sig);	

	/* Regenerate the tool list */
	model = gtk_tree_view_get_model (this->view);
	gtk_list_store_clear (GTK_LIST_STORE(model));
	tool = atp_tool_list_first (atp_plugin_get_tool_list (this->plugin));
	while (tool)
	{
		GtkTreeIter iter;

		gtk_list_store_append (GTK_LIST_STORE(model), &iter);
		gtk_list_store_set (GTK_LIST_STORE(model), &iter,
				ATP_TOOLS_ENABLED_COLUMN,
				atp_user_tool_get_flag (tool, ATP_TOOL_ENABLE),
				ATP_TOOLS_NAME_COLUMN,
				atp_user_tool_get_name (tool),
				ATP_TOOLS_DATA_COLUMN, tool,
				-1);
		if ((select != NULL) && (strcmp(select, atp_user_tool_get_name (tool)) == 0))
		{
			gtk_tree_selection_select_iter (selection, &iter);
		}
		tool = atp_user_tool_next (tool);
				
	}

	/* Regenerate the tool menu */
	atp_tool_list_activate (atp_plugin_get_tool_list (this->plugin));

	/* Restore changed signal */
	g_signal_handler_unblock (selection, this->changed_sig);	
	update_sensitivity(this);
}

/* Start the tool lister and editor */

gboolean
atp_tool_dialog_show (ATPToolDialog* this)
{
	GladeXML *xml;
	GtkTreeModel *model;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;

	if (this->dialog != NULL)
	{
		/* dialog is already displayed */
		gtk_window_present (GTK_WINDOW (this->dialog));
		return FALSE;
	}

	/* Load glade file */
	if (NULL == (xml = glade_xml_new(GLADE_FILE, TOOL_LIST, NULL)))
	{
		anjuta_util_dialog_error(atp_plugin_get_app_window (this->plugin), _("Unable to build user interface for tool list"));
		return FALSE;
	}
	this->dialog = GTK_DIALOG(glade_xml_get_widget(xml, TOOL_LIST));
	gtk_widget_show (GTK_WIDGET(this->dialog));
	gtk_window_set_transient_for (GTK_WINDOW (this->dialog), atp_plugin_get_app_window (this->plugin));

	/* Create tree view */	
	this->view = GTK_TREE_VIEW (glade_xml_get_widget(xml, TOOL_TREEVIEW));
	model = GTK_TREE_MODEL (gtk_list_store_new (ATP_N_TOOLS_COLUMNS,
		G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_POINTER));
	gtk_tree_view_set_model (this->view, model);
	
	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (G_OBJECT (renderer), "toggled", G_CALLBACK (on_tool_enable), this);
	column = gtk_tree_view_column_new_with_attributes ("", renderer,
	   "active", ATP_TOOLS_ENABLED_COLUMN, NULL);
	gtk_tree_view_append_column (this->view, column);
	
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Tool"), renderer,
	   "text", ATP_TOOLS_NAME_COLUMN, NULL);
	gtk_tree_view_append_column (this->view, column);
	g_object_unref (model);

	/* Get all buttons */
	this->edit_bt = glade_xml_get_widget(xml, TOOL_EDIT_BUTTON);
	this->delete_bt = glade_xml_get_widget(xml, TOOL_DELETE_BUTTON);
	this->up_bt = glade_xml_get_widget(xml, TOOL_UP_BUTTON);
	this->down_bt = glade_xml_get_widget(xml, TOOL_DOWN_BUTTON);

	/* Connect all signals */	
	glade_xml_signal_connect_data (xml, LIST_RESPONSE_SIGNAL, GTK_SIGNAL_FUNC (on_list_response), this);
	glade_xml_signal_connect_data (xml, TOOL_ADD_SIGNAL, GTK_SIGNAL_FUNC (on_tool_add), this);
	glade_xml_signal_connect_data (xml, TOOL_ACTIVATED_SIGNAL, GTK_SIGNAL_FUNC (on_tool_activated), this);
	glade_xml_signal_connect_data (xml, TOOL_EDIT_SIGNAL, GTK_SIGNAL_FUNC (on_tool_edit), this);
	glade_xml_signal_connect_data (xml, TOOL_DELETE_SIGNAL, GTK_SIGNAL_FUNC (on_tool_delete), this);
	glade_xml_signal_connect_data (xml, TOOL_UP_SIGNAL, GTK_SIGNAL_FUNC (on_tool_up), this);
	glade_xml_signal_connect_data (xml, TOOL_DOWN_SIGNAL, GTK_SIGNAL_FUNC (on_tool_down), this);
	selection = gtk_tree_view_get_selection (this->view);
	this->changed_sig = g_signal_connect (G_OBJECT (selection), "changed", G_CALLBACK (on_tool_selection_changed), this);

	g_object_unref (xml);

	atp_tool_dialog_refresh (this, NULL);

	return TRUE;
}

void
atp_tool_dialog_close (ATPToolDialog *this)
{
	atp_tool_editor_list_destroy (&this->tedl);

	if (this->dialog)
	{
		gtk_widget_destroy (GTK_WIDGET(this->dialog));
		this->dialog = NULL;
	}
}

/* Access functions
 *---------------------------------------------------------------------------*/

GtkWindow*
atp_tool_dialog_get_window (const ATPToolDialog *this)
{
	return GTK_WINDOW (this->dialog);
}

ATPVariable*
atp_tool_dialog_get_variable (const ATPToolDialog *this)
{
	return atp_plugin_get_variable (this->plugin);
}

/* Creation and Destruction
 *---------------------------------------------------------------------------*/

void
atp_tool_dialog_construct (ATPToolDialog *this, ATPPlugin *plugin)
{
	this->plugin = plugin;
	this->dialog = NULL;
	atp_tool_editor_list_construct (&this->tedl);
}

void
atp_tool_dialog_destroy (ATPToolDialog *this)
{
	atp_tool_dialog_close (this);
}
