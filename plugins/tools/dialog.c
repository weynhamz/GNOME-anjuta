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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
 * Tools list dialog
 * 
 *---------------------------------------------------------------------------*/

#include <config.h>

#include "dialog.h"

#include "editor.h"
#include "fileop.h"

/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/

/* Widget and signal name found in glade file
 *---------------------------------------------------------------------------*/

#define TOOL_LIST "list_dialog"
#define TOOL_TREEVIEW "tools_treeview"

#define LIST_RESPONSE_SIGNAL "on_list_dialog_response"
#define TOOL_ADD_SIGNAL "on_tool_add"
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

/*---------------------------------------------------------------------------*/

#if 0
/* Activates a tool */
static void an_user_tool_activate(AnUserTool *tool)
{
	GtkWidget *submenu;

	if (FALSE == tool->enabled)
		return;
	else if (NULL == (submenu = an_get_submenu(tool->location)))
	{
		tool->enabled = FALSE;
		tool->menu_item = NULL;
		return;
	}
	else
	{
		tool->menu_item = gtk_menu_item_new_with_label(tool->name);
		gtk_widget_ref(tool->menu_item);
		g_signal_connect (G_OBJECT (tool->menu_item), "activate"
		  , G_CALLBACK (execute_tool), tool);
		gtk_menu_append(GTK_MENU(submenu), tool->menu_item);
		if (tool->shortcut && tool->shortcut[0] != '\0')
		{
			guint mask = 0;
			guint accel_key = '\0';
			char *c = tool->shortcut;
			while (*c != '\0')
			{
				switch(*c)
				{
					case '^':
						mask |= GDK_CONTROL_MASK;
						break;
					case '+':
						mask |= GDK_SHIFT_MASK;
						break;
					case '#':
						mask |= GDK_MOD1_MASK;
						break;
					default:
						accel_key = *c;
						break;
				}
				++ c;
			}
			gtk_widget_add_accelerator(tool->menu_item, "activate", app->accel_group
			  , accel_key, mask ? mask : GDK_CONTROL_MASK | GDK_SHIFT_MASK
			  , GTK_ACCEL_VISIBLE);
		}
		gtk_widget_show(tool->menu_item);
	}
}

void anjuta_tools_sensitize(void)
{
	GSList *tmp;
	AnUserTool *tool;

	for (tmp = tool_list; tmp; tmp = g_slist_next(tmp))
	{
		tool = (AnUserTool *) tmp->data;
		if (tool->menu_item)
		{
			if ((!app->project_dbase->project_is_open && tool->project_level)
			|| (!app->current_text_editor && tool->file_level))
			{
				gtk_widget_set_sensitive(tool->menu_item, FALSE);
			}
			else
				gtk_widget_set_sensitive(tool->menu_item, TRUE);
		}
	}
}

/* Structure containing the required properties of the tool list GUI */
typedef struct _AnToolList
{
	GtkWidget *dialog;
	GtkWidget *clist;
} AnToolList;

/* Forward declarations */
static void
on_user_tool_row_activated (GtkTreePath     *arg1,
                            GtkTreeViewColumn *arg2,
                            gpointer         user_data);

static void
on_user_tool_selection_changed (GtkTreeSelection *sel, AnToolList *tl);

static gint
on_user_tool_delete_event (GtkWindow *window, GdkEvent* event,
						   gpointer user_data);

static void
on_user_tool_response (GtkDialog *dialog, gint res, gpointer user_data);

/* Callbacks for tool list GUI */
void
on_user_tool_row_activated (GtkTreePath     *arg1,
                            GtkTreeViewColumn *arg2,
                            gpointer         user_data)
{
	GtkTreeSelection *sel;
	GtkTreeModel *model;
	GtkTreeIter iter;
	
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tl->clist));
	if (gtk_tree_selection_get_selected (sel, &model, &iter))
		gtk_dialog_response (GTK_DIALOG (tl->dialog), GTK_RESPONSE_APPLY);
}

void
on_user_tool_selection_changed (GtkTreeSelection *sel, AnToolList *tl)
{
	GtkTreeModel *model;
	GtkTreeIter iter;

	if (gtk_tree_selection_get_selected (sel, &model, &iter))
	{
		gtk_dialog_set_response_sensitive (GTK_DIALOG (tl->dialog),
										   GTK_RESPONSE_APPLY, TRUE);
		gtk_dialog_set_response_sensitive (GTK_DIALOG (tl->dialog),
										   GTK_RESPONSE_NO, TRUE);
	}
	else 
	{
		gtk_dialog_set_response_sensitive (GTK_DIALOG (tl->dialog),
										   GTK_RESPONSE_APPLY, FALSE);
		gtk_dialog_set_response_sensitive (GTK_DIALOG (tl->dialog),
										   GTK_RESPONSE_NO, FALSE);
	}
}

gboolean
on_user_tool_delete_event (GtkWindow *window, GdkEvent* event,
						   gpointer user_data)
{
	/* Don't hide the tool list if a tool is being edited */
	if (!ted)
	{
		GSList *tmp;
		AnUserTool *tool;
		anjuta_tools_save();
		for (tmp = tool_list; tmp; tmp = g_slist_next(tmp))
		{
			tool = (AnUserTool *) tmp->data;
			if (NULL == tool->menu_item)
				an_user_tool_activate(tool);
			gtk_widget_destroy (tl->dialog);
		}
		anjuta_tools_sensitize();
		g_free (tl);
		tl = NULL;
		return FALSE;
	}
	return TRUE;
}

static void really_delete_tool ()
{
	AnUserTool *tool;
	GtkTreeModel *model;
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tl->clist));
	if (gtk_tree_selection_get_selected (sel, &model, &iter))
	{
		gtk_tree_model_get (model, &iter,
		                    AN_TOOLS_DATA_COLUMN, &tool, -1);
		gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
		if (tool)
			an_user_tool_free(tool, TRUE);
	}
}
#endif

static ATPUserTool*
get_current_tool (ATPToolDialog *this)
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
on_tool_add (GtkButton *button, gpointer user_data)
{
	ATPToolDialog *this = (ATPToolDialog *)user_data;
	ATPUserTool *tool;
	ATPToolEditor *ted;
	guint position;

	tool = get_current_tool (this);

	if (tool == NULL)
	{
		tool = atp_tool_list_append_new (atp_plugin_get_tool_list (this->plugin), NULL, ATP_TSTORE_LOCAL);
	}
	else
	{
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

	tool = get_current_tool (this);
	if (tool != NULL);
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
	if (tool != NULL)
	{
		if (anjuta_util_dialog_boolean_question (NULL, _("Are you sure you want to delete the '%s' tool?"), atp_user_tool_get_name(tool)))
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

	tool = get_current_tool (this);
	if (tool != NULL)
	{
		prev = atp_user_tool_previous (tool);
		if (prev != NULL)
		{
			atp_tool_list_move (atp_plugin_get_tool_list (this->plugin), prev, tool);
		}
	}
	atp_tool_dialog_refresh (this, atp_user_tool_get_name (tool));
}

static void
on_tool_down (GtkButton *button, gpointer user_data)
{
	ATPToolDialog *this = (ATPToolDialog *)user_data;
	ATPUserTool *tool;
	ATPUserTool *next;

	tool = get_current_tool (this);
	if (tool != NULL)
	{
		next = atp_user_tool_next (tool);
		if (next != NULL)
		{
			atp_tool_list_move (atp_plugin_get_tool_list (this->plugin), tool, next);
		}
	}
	atp_tool_dialog_refresh (this, atp_user_tool_get_name (tool));
}

static void
on_list_response (GtkDialog *dialog, gint res, gpointer user_data)
{
	ATPToolDialog *this = (ATPToolDialog *)user_data;

	/* Just save tool list */
	atp_anjuta_tools_save(this->plugin);

	/* Close widget */
	atp_tool_dialog_close (this);

	#if 0
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	AnUserTool *tool;
	gboolean has_selection = FALSE;

	if (tl)
	{
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(tl->clist));
		has_selection = gtk_tree_selection_get_selected (selection, &model,
														 &iter);
		if (has_selection) 
		{
			gtk_tree_model_get (model, &iter,
								AN_TOOLS_DATA_COLUMN, &tool, -1);
			g_assert (tool);
		}
	}
	
	switch (res)
	{
	case GTK_RESPONSE_APPLY:
		/* Make sure no tool is selected, so that the edit form is cleared
		when a new tool is created */
		if (has_selection)
			show_tool_editor(tool, TRUE);
		return;
		
	case GTK_RESPONSE_YES:
		if (ted && ted->dialog)
			return;
		if (has_selection)
		{
			/* Make sure no tool is selected, so that the edit form is cleared
			when a new tool is created */
			if (tl)
				gtk_tree_selection_unselect_all (selection);
		}
		show_tool_editor(NULL, FALSE);
		return;

	case GTK_RESPONSE_NO:
		/* Don't allow deletes if a tool is being edited */
		if (ted && ted->dialog)
			return;
		if (has_selection)
		{
			char question[1000];
			GtkWidget *dlg;
			
			snprintf(question, 10000,
					 _("Are you sure you want to delete the '%s' tool?"),
			  		 tool->name);
			dlg = gtk_message_dialog_new (GTK_WINDOW (tl->dialog),
										  GTK_DIALOG_DESTROY_WITH_PARENT,
										  GTK_MESSAGE_QUESTION,
										  GTK_BUTTONS_NONE,
										  question);
			gtk_dialog_add_buttons (GTK_DIALOG (dlg),
									GTK_STOCK_CANCEL, GTK_RESPONSE_NO,
									GTK_STOCK_DELETE, GTK_RESPONSE_YES,
									NULL);
			if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_YES)
				really_delete_tool ();
			gtk_widget_destroy (dlg);
		}
		return;
	}
	#endif
}

void
atp_tool_dialog_refresh (const ATPToolDialog *this, const gchar* select)
{
	ATPUserTool *tool;
	GtkTreeModel *model;


	model = gtk_tree_view_get_model (this->view);
	gtk_list_store_clear (GTK_LIST_STORE(model));
	tool = atp_tool_list_first (atp_plugin_get_tool_list (this->plugin));
	while (tool)
	{
		GtkTreeIter iter;

		gtk_list_store_append (GTK_LIST_STORE(model), &iter);
		gtk_list_store_set (GTK_LIST_STORE(model), &iter,
				ATP_TOOLS_ENABLED_COLUMN,
				atp_user_tool_get_enable (tool),
				ATP_TOOLS_NAME_COLUMN,
				atp_user_tool_get_name (tool),
				ATP_TOOLS_DATA_COLUMN, tool,
				-1);
		if ((select != NULL) && (strcmp(select, atp_user_tool_get_name (tool)) == 0))
		{
			GtkTreeSelection *sel;

			sel = gtk_tree_view_get_selection (this->view);
			gtk_tree_selection_select_iter (sel, &iter);
		}
		tool = atp_user_tool_next (tool);
				
	}
	atp_tool_list_activate (atp_plugin_get_tool_list (this->plugin));
}

/* Start the tool lister and editor */

gboolean
atp_tool_dialog_show (ATPToolDialog* this)
{
	GladeXML *xml;
	GList *list;
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

	if (NULL == (xml = glade_xml_new(GLADE_FILE, TOOL_LIST, NULL)))
	{
		anjuta_util_dialog_error(NULL, _("Unable to build user interface for tool list"));
		return FALSE;
	}
	this->dialog = GTK_DIALOG(glade_xml_get_widget(xml, TOOL_LIST));
	gtk_widget_show (GTK_WIDGET(this->dialog));
	gtk_window_set_transient_for (GTK_WINDOW (this->dialog), atp_plugin_get_app_window (this->plugin));
	
	this->view = (GtkTreeView *) glade_xml_get_widget(xml, TOOL_TREEVIEW);
	model = (GtkTreeModel*)gtk_list_store_new (ATP_N_TOOLS_COLUMNS,
		G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_POINTER);
	gtk_tree_view_set_model (this->view, model);
	
	renderer = gtk_cell_renderer_toggle_new ();
	column = gtk_tree_view_column_new_with_attributes ("", renderer,
	   "active", ATP_TOOLS_ENABLED_COLUMN, NULL);
	gtk_tree_view_append_column (this->view, column);
	
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Tool"), renderer,
	   "text", ATP_TOOLS_NAME_COLUMN, NULL);
	gtk_tree_view_append_column (this->view, column);
	g_object_unref (model);

	/* Connect all signals */	
	glade_xml_signal_connect_data (xml, LIST_RESPONSE_SIGNAL, GTK_SIGNAL_FUNC (on_list_response), this);
	glade_xml_signal_connect_data (xml, TOOL_ADD_SIGNAL, GTK_SIGNAL_FUNC (on_tool_add), this);
	glade_xml_signal_connect_data (xml, TOOL_EDIT_SIGNAL, GTK_SIGNAL_FUNC (on_tool_edit), this);
	glade_xml_signal_connect_data (xml, TOOL_DELETE_SIGNAL, GTK_SIGNAL_FUNC (on_tool_delete), this);
	glade_xml_signal_connect_data (xml, TOOL_UP_SIGNAL, GTK_SIGNAL_FUNC (on_tool_up), this);
	glade_xml_signal_connect_data (xml, TOOL_DOWN_SIGNAL, GTK_SIGNAL_FUNC (on_tool_down), this);

	#if 0	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(tl->clist));
	
	gtk_dialog_set_response_sensitive (GTK_DIALOG (tl->dialog),
									   GTK_RESPONSE_APPLY, FALSE);
	gtk_dialog_set_response_sensitive (GTK_DIALOG (tl->dialog),
									   GTK_RESPONSE_NO, FALSE);
	
	g_signal_connect (G_OBJECT (selection), "changed",
					  G_CALLBACK (on_user_tool_selection_changed), tl);
	g_signal_connect (G_OBJECT (tl->clist), "row_activated",
					  G_CALLBACK (on_user_tool_row_activated), NULL);
	g_signal_connect (G_OBJECT (tl->dialog), "delete_event",
					  G_CALLBACK (on_user_tool_delete_event), NULL);
	g_signal_connect (G_OBJECT (tl->dialog), "response",
					  G_CALLBACK (on_user_tool_response), NULL);
	#endif

	g_object_unref (xml);

	atp_tool_dialog_refresh (this, NULL);

	return TRUE;
}

void
atp_tool_dialog_close (ATPToolDialog *this)
{
	if (this->dialog)
	{
		gtk_widget_destroy (GTK_WIDGET(this->dialog));
		this->dialog = NULL;
	}
	atp_tool_editor_list_free_at (&this->tedl);
}

GtkWindow*
atp_tool_dialog_get_window (const ATPToolDialog *this)
{
	return GTK_WINDOW (this->dialog);
}

void
atp_tool_dialog_new_at (ATPToolDialog *this, ATPPlugin *plugin)
{
	this->plugin = plugin;
	this->dialog = NULL;
	atp_tool_editor_list_new_at (&this->tedl);
}

void
atp_tool_dialog_free_at (ATPToolDialog *this)
{
	atp_tool_dialog_close (this);
}
