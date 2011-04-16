/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* am-dialogs.c
 *
 * Copyright (C) 2009  Sébastien Granjoux
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>

#include "dialogs.h"

#include "project-model.h"
#include "project-view.h"

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-pkg-config-chooser.h>

#define ICON_SIZE 16

#define GLADE_FILE  PACKAGE_DATA_DIR "/glade/pm_dialogs.ui"

/* Types
  *---------------------------------------------------------------------------*/

typedef struct _PropertiesTable
{
	AnjutaPmProject *project;
	GtkWidget *dialog;
	GtkWidget *table;
	GtkWidget *head;
	GtkWidget *main;
	GtkWidget *expand;
	GtkWidget *extra;
	GbfTreeData *data;
	AnjutaProjectNode *node;
	GList *properties;
} PropertiesTable;

typedef struct _PropertyEntry
{
	GtkWidget *entry;
	AnjutaProjectProperty *property;
} PropertyEntry;

typedef struct _PropertyValue
{
	AnjutaProjectProperty *property;
	const gchar *value;
} PropertyValue;

enum {
	NAME_COLUMN,
	VALUE_COLUMN,
	LIST_COLUMNS_NB
};

/* Sources list columns in new source dialog */
enum {
	COLUMN_FILE,
	COLUMN_URI,
	N_COLUMNS
};

/* Helper functions
 *---------------------------------------------------------------------------*/

static GtkBuilder *
load_interface (const gchar *top_widget)
{
    GtkBuilder *xml = gtk_builder_new ();
    GError* error = NULL;

    if (!gtk_builder_add_from_file (xml, GLADE_FILE, &error))
    {
        g_warning ("Couldn't load builder file: %s", error->message);
        g_error_free (error);
        return NULL;
    }

    return xml;
}

static void
error_dialog (GtkWindow *parent, const gchar *summary, const gchar *msg, ...)
{
    va_list ap;
    gchar *tmp;
    GtkWidget *dialog;
    
    va_start (ap, msg);
    tmp = g_strdup_vprintf (msg, ap);
    va_end (ap);
    
    dialog = gtk_message_dialog_new_with_markup (parent,
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_OK,
						 "<b>%s</b>\n\n%s", summary, tmp);
    g_free (tmp);
    
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
}

 
/* Private nodes functions
 *---------------------------------------------------------------------------*/

static PropertyEntry*
pm_property_entry_new (GtkWidget *entry, AnjutaProjectProperty *property)
{
	PropertyEntry *prop;
	
	prop = g_slice_new0(PropertyEntry);
	prop->entry = entry;
	prop->property = property;
	
	return prop;
}

static void
pm_property_entry_free (PropertyEntry *prop)
{
	g_slice_free (PropertyEntry, prop);
}

static gboolean
parent_filter_func (GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
	GbfTreeData *data = NULL;
	gboolean visible = FALSE;
	AnjutaProjectNodeType type = (AnjutaProjectNodeType)GPOINTER_TO_INT (user_data);
	AnjutaProjectNode *parent;
	AnjutaProjectNode *node;
	gint need;

	switch (type)
	{
	case ANJUTA_PROJECT_GROUP:
		need = ANJUTA_PROJECT_CAN_ADD_GROUP;
		break;
	case ANJUTA_PROJECT_TARGET:
		need = ANJUTA_PROJECT_CAN_ADD_TARGET;
		break;
	case ANJUTA_PROJECT_SOURCE:
		need = ANJUTA_PROJECT_CAN_ADD_SOURCE;
		break;
	case ANJUTA_PROJECT_MODULE:
		/* Add node containing target too because target can contains module 
		 * It would be probably better to check recursively if any children
		 * can accept a module and keep all parents then. */	
		need = ANJUTA_PROJECT_CAN_ADD_MODULE | ANJUTA_PROJECT_CAN_ADD_TARGET;
		break;
	case ANJUTA_PROJECT_PACKAGE:
		need = ANJUTA_PROJECT_CAN_ADD_PACKAGE;
		break;
	default:
		need = 0;
		break;
	}
	
	gtk_tree_model_get (model, iter,
						GBF_PROJECT_MODEL_COLUMN_DATA, &data, -1);
	node = data == NULL ? NULL : gbf_tree_data_get_node (data);
	if (node != NULL)
	{
		if (anjuta_project_node_get_state (node) & need)
		{
			/* Current node can be used as parent */
			visible = TRUE;
		}
		else if (anjuta_project_node_get_node_type (node) == type)
		{
			/* Check if node can be used as sibling */
			parent = anjuta_project_node_parent (node);
			visible = anjuta_project_node_get_state (parent) & need ? TRUE : FALSE;
			
		}
	}

	return visible;
}

static gboolean
module_filter_func (GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
	GbfTreeData *data = NULL;
	gboolean visible = FALSE;
	AnjutaProjectNode *node;
	
	gtk_tree_model_get (model, iter,
						GBF_PROJECT_MODEL_COLUMN_DATA, &data, -1);
	node = data == NULL ? NULL : gbf_tree_data_get_node (data);
	if (node != NULL)
	{
		AnjutaProjectNodeType type = anjuta_project_node_get_node_type (node);

		visible = (type == ANJUTA_PROJECT_MODULE) || (type == ANJUTA_PROJECT_PACKAGE);
	}
	
	return visible;
}

static void 
setup_nodes_treeview (GbfProjectView           *view,
						GbfProjectView		   *parent,
                  	    GtkTreePath            *root,
						GtkTreeModelFilterVisibleFunc func,
						gpointer               data,
						GtkTreeIter            *selected)
{
	g_return_if_fail (view != NULL && GBF_IS_PROJECT_VIEW (view));
	g_return_if_fail (parent != NULL);

	gbf_project_view_set_parent_view (view, parent, root);
	gbf_project_view_set_visible_func (view, func, data, NULL);
	gbf_project_view_set_cursor_to_iter (view, selected);
}

static void
entry_changed_cb (GtkEditable *editable, gpointer user_data)
{
    GtkWidget *button = user_data;
    gchar *text;

    if (!button)
        return;

    text = gtk_editable_get_chars (editable, 0, -1);
    if (strlen (text) > 0) {
        gtk_widget_set_sensitive (button, TRUE);
        gtk_widget_grab_default (button);
    } else {
        gtk_widget_set_sensitive (button, FALSE);
    }
    g_free (text);
}

static void
on_row_changed(GtkTreeModel* model, GtkTreePath* path, GtkTreeIter* iter, gpointer data)
{
	GtkWidget* button = GTK_WIDGET(data);
	if (gtk_list_store_iter_is_valid(GTK_LIST_STORE(model), iter))
		gtk_widget_set_sensitive(button, TRUE);
	else
		gtk_widget_set_sensitive(button, FALSE);
}

static void
browse_button_clicked_cb (GtkWidget *widget, gpointer user_data)
{
	GtkTreeView *tree = user_data;
	GtkTreeView *treeview;
	GtkFileChooserDialog* dialog;
	GtkTreeModel* model;
	AnjutaProjectNode *parent;
	gint result;

	g_return_if_fail (user_data != NULL && GTK_IS_TREE_VIEW (user_data));
	
	model = gtk_tree_view_get_model(tree);
	/*if (gtk_tree_model_get_iter_first(model, &iter))
	{
		gtk_tree_model_get(model, &iter, COLUMN_URI, &file, -1);
		uri = g_strdup(file);
	}
	else
		uri = g_strdup("");*/
	
	dialog = GTK_FILE_CHOOSER_DIALOG(gtk_file_chooser_dialog_new (_("Select sources…"),
						GTK_WINDOW (gtk_widget_get_toplevel (widget)),
						GTK_FILE_CHOOSER_ACTION_OPEN,
						GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
						NULL));

	/* Get default directory */
	treeview = g_object_get_data (G_OBJECT (widget), "treeview");
	parent = gbf_project_view_find_selected (GBF_PROJECT_VIEW (treeview), ANJUTA_PROJECT_UNKNOWN);
	if (!(anjuta_project_node_get_state (parent) & ANJUTA_PROJECT_CAN_ADD_SOURCE))
	{
		parent = anjuta_project_node_parent (parent);
	}
	gtk_file_chooser_set_current_folder_file(GTK_FILE_CHOOSER(dialog),
			anjuta_project_node_get_file (parent), NULL);

	gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER(dialog), TRUE);

	result = gtk_dialog_run (GTK_DIALOG (dialog));
	switch (result)
	{
	case GTK_RESPONSE_ACCEPT:
	{
		GSList* uris = gtk_file_chooser_get_uris (GTK_FILE_CHOOSER(dialog));
		GSList* node_uri = uris;

		gtk_list_store_clear (GTK_LIST_STORE (model));

		while (node_uri != NULL)
		{
			GtkTreeIter iter;
			gchar* uri = node_uri->data;
			gchar* file = g_path_get_basename (uri);
			gtk_list_store_append (GTK_LIST_STORE (model), &iter);
			gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_FILE,
						file, COLUMN_URI, uri, -1);
			node_uri = g_slist_next (node_uri);
		}
		g_slist_free (uris);
		break;
	}
	default:
		break;
	} 
	gtk_widget_destroy (GTK_WIDGET(dialog));
}
 
/* Private properties functions
 *---------------------------------------------------------------------------*/

static GtkWidget *
add_entry (IAnjutaProject *project, AnjutaProjectNode *node, AnjutaProjectProperty *prop, GtkWidget *table, gint *position)
{
	GtkWidget *label;
	GtkWidget *entry = NULL;
	GtkWidget *view;
	static GType column_type[LIST_COLUMNS_NB] = {
		G_TYPE_STRING, G_TYPE_STRING};
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GList *item;
	gchar *tooltip = NULL;
	gboolean editable = TRUE;

	if (prop->native != NULL)
	{
		label = gtk_label_new (_(prop->native->name));
	}
	else
	{
		label = gtk_label_new (_(prop->name));
	}

	editable = prop->flags & ANJUTA_PROJECT_PROPERTY_READ_ONLY ? FALSE : TRUE;
	
	if (prop->detail != NULL)
	{
		if (!editable)
		{
			tooltip = g_strconcat (_(prop->detail), _(" This property is not modifiable."), NULL);
		}
		else
		{
			tooltip = g_strdup (_(prop->detail));
		}
	}
	
	if (tooltip != NULL)
	{
		gtk_widget_set_tooltip_markup (label, tooltip);
	}
	gtk_misc_set_alignment (GTK_MISC (label), 0, -1);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, *position, *position+1,
			  GTK_FILL, GTK_FILL, 5, 3);

	switch (prop->type)
	{
	case ANJUTA_PROJECT_PROPERTY_STRING:
	case ANJUTA_PROJECT_PROPERTY_LIST:
		if (editable)
		{
			entry = gtk_entry_new ();
			gtk_entry_set_text (GTK_ENTRY (entry), prop->value != NULL ? prop->value : "");
		}
		else
		{
			entry = gtk_label_new (prop->value != NULL ? prop->value : "");
			gtk_misc_set_alignment (GTK_MISC (entry), 0, 0.5);
		}
		break;
	case ANJUTA_PROJECT_PROPERTY_BOOLEAN:
		entry = gtk_check_button_new ();
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (entry), (prop->value != NULL) && (*prop->value == '1'));
		gtk_widget_set_sensitive (entry, editable);
		break;
	case ANJUTA_PROJECT_PROPERTY_MAP:
			model = GTK_TREE_MODEL (gtk_list_store_newv (LIST_COLUMNS_NB, column_type));

			if (prop->native != NULL) prop = prop->native;
			
			for (item = anjuta_project_node_get_custom_properties (node); item != NULL; item = g_list_next (item))
			{
				AnjutaProjectProperty *cust_prop = (AnjutaProjectProperty *)item->data;
				
				if (cust_prop->native == prop)
				{
					gtk_list_store_append (GTK_LIST_STORE (model), &iter);
					gtk_list_store_set (GTK_LIST_STORE (model), &iter, NAME_COLUMN, cust_prop->name, VALUE_COLUMN, cust_prop->value, -1);
				}
			}
			
			gtk_list_store_append (GTK_LIST_STORE (model), &iter);
			gtk_list_store_set (GTK_LIST_STORE (model), &iter, NAME_COLUMN, "", VALUE_COLUMN, "", -1);
			
			entry = gtk_frame_new (NULL);
			gtk_frame_set_shadow_type (GTK_FRAME (entry), GTK_SHADOW_IN);
			
			view = gtk_tree_view_new_with_model (model);
			gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (view)),
											GTK_SELECTION_SINGLE);
			gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW (view), TRUE);
			g_object_unref (G_OBJECT (model));

			renderer = gtk_cell_renderer_text_new ();
			column = gtk_tree_view_column_new_with_attributes (_("Name"),
			    renderer,
			    "text",
			    NAME_COLUMN,
			    NULL);
			gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
			gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
			column = gtk_tree_view_column_new_with_attributes (_("Value"),
			    renderer,
			    "text",
			    VALUE_COLUMN,
			    NULL);
			gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
			gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);

			gtk_container_add (GTK_CONTAINER (entry), view);
			
			break;
	default:
		return NULL;
	}		
	if (tooltip != NULL)
	{
		gtk_widget_set_tooltip_markup (entry, tooltip);
	}
	g_free (tooltip);
	
	gtk_widget_show (entry);
	gtk_table_attach (GTK_TABLE (table), entry, 1, 2, *position, *position+1,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 3);
	
	*position = *position + 1;
	
	return entry;
}

static void
add_label (const gchar *display_name, const gchar *value, GtkWidget *table, gint *position)
{
	GtkWidget *label;
	
	label = gtk_label_new (display_name);
	gtk_misc_set_alignment (GTK_MISC (label), 0, -1);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, *position, *position+1,
			  GTK_FILL, GTK_FILL, 5, 3);

	label = gtk_label_new (value);
	gtk_misc_set_alignment (GTK_MISC (label), 0, -1);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 1, 2, *position, *position+1,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 3);

	*position = *position + 1;
}

static gboolean
is_project_node_but_shortcut (GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
	GbfTreeData *data;

	gtk_tree_model_get (GTK_TREE_MODEL (model), iter,
		    GBF_PROJECT_MODEL_COLUMN_DATA, &data,
		    -1);

	return (data != NULL) && (data->shortcut == NULL) && (gbf_tree_data_get_node (data) != NULL);
}

static void
update_properties (PropertiesTable *table)
{
	GFile *file;
	const gchar *title;
	gint head_pos;
	gint main_pos;
	gint extra_pos;
	const GList *item;
	AnjutaProjectNodeType type;
	AnjutaProjectNodeInfo* node_info;
	gboolean single;
	GList *children;
	GList *last;

	head_pos = 0;
	main_pos = 0;
	extra_pos = 0;

	/* Update dialog type */
	switch (anjuta_project_node_get_node_type (table->node))
	{
	case ANJUTA_PROJECT_ROOT:
		title = _("Project properties");
		break;
	case ANJUTA_PROJECT_GROUP:
		title = _("Directory properties");
		break;
	case ANJUTA_PROJECT_TARGET:
		title = _("Target properties");
		break;
	case ANJUTA_PROJECT_SOURCE:
		title = _("Source properties");
		break;
	case ANJUTA_PROJECT_MODULE:
		title = _("Module properties");
		break;
	case ANJUTA_PROJECT_PACKAGE:
		title = _("Package properties");
		break;
	default:
		title = _("Unknown properties");
		break;
	}
	gtk_window_set_title (GTK_WINDOW (table->dialog), title);

	/* Clear table */
	children = gtk_container_get_children (GTK_CONTAINER (table->head));
	/* Clear only the first 4 widgets */
	while ((last = g_list_nth (children, 4)) != NULL) children = g_list_delete_link (children, last);
	g_list_foreach (children, (GFunc)gtk_widget_destroy, NULL);
	g_list_free (children);
	children = gtk_container_get_children (GTK_CONTAINER (table->main));
	g_list_foreach (children, (GFunc)gtk_widget_destroy, NULL);
	g_list_free (children);
	children = gtk_container_get_children (GTK_CONTAINER (table->extra));
	g_list_foreach (children, (GFunc)gtk_widget_destroy, NULL);
	g_list_free (children);
	g_list_foreach (table->properties, (GFunc)pm_property_entry_free, NULL);
	g_list_free (table->properties);
	table->properties = NULL;
	
	/* Update node name */
	file = anjuta_project_node_get_file (table->node);
	if (file != NULL)
	{
		gchar *path;
		
		path = g_file_get_path (file);
		add_label (_("Path:"), path, table->head, &head_pos);
		g_free (path);
	}
	else
	{
		add_label (_("Name:"), anjuta_project_node_get_name (table->node), table->head, &head_pos);
	}
	
	/* Display node type only if several types are possible */
	node_info = NULL;
	single = TRUE;
	type = anjuta_project_node_get_full_type (table->node);
	for (item = ianjuta_project_get_node_info (table->project->project, NULL); item != NULL; item = g_list_next (item))
	{
		AnjutaProjectNodeInfo* info = (AnjutaProjectNodeInfo *)item->data;

		if (info->type == type)
		{
			node_info = info;
		}
		else if ((info->type & ANJUTA_PROJECT_TYPE_MASK) == (type & ANJUTA_PROJECT_TYPE_MASK))
		{
			single = FALSE;
		}
	}
	if (!single && (node_info != NULL))
	{
		add_label (_("Type:"), anjuta_project_node_info_name (node_info), table->main, &main_pos);
	}

	/* Display other node properties */
	single = FALSE;

	for (item = anjuta_project_node_get_native_properties (table->node); item != NULL; item = g_list_next (item))
	{
		AnjutaProjectProperty *valid_prop = (AnjutaProjectProperty *)item->data; 
		AnjutaProjectProperty *prop; 
		GtkWidget *entry;

		prop = anjuta_project_node_get_property (table->node, valid_prop);
		if (prop->native != NULL)
		{
			/* This property has been set, display it in the main part */
			entry = add_entry (table->project->project, table->node, prop, table->main, &main_pos);
		}
		else
		{
			/* This property has not been set, hide it by default */
			entry = add_entry (table->project->project, table->node, valid_prop, table->extra, &extra_pos);
			single = TRUE;
		}

		if (entry != NULL)
		{
			table->properties = g_list_prepend (table->properties,
					pm_property_entry_new (entry, valid_prop));
		}
	}
	table->properties = g_list_reverse (table->properties);
	gtk_widget_show_all (table->table);
	
	/* Hide expander if it is empty */
	if (single)
		gtk_widget_show (table->expand);
	else
		gtk_widget_hide (table->expand);
}

static void
on_properties_dialog_response (GtkWidget *dialog,
							   gint id,
							   PropertiesTable *table)
{
	if (id == GTK_RESPONSE_APPLY)
	{
		GList *item;
		
		/* Get all modified properties */
		for (item = g_list_first (table->properties); item != NULL; item = g_list_next (item))
		{
			PropertyEntry *entry = (PropertyEntry *)item->data;
			AnjutaProjectProperty *prop;
			const gchar *text;
			gboolean active;
			
			/* Get property value in node */
			prop = anjuta_project_node_get_property (table->node, entry->property);
			if (prop == NULL) prop = entry->property;
			
			switch (prop->type)
			{
			case ANJUTA_PROJECT_PROPERTY_STRING:
			case ANJUTA_PROJECT_PROPERTY_LIST:
				if (GTK_IS_ENTRY (entry->entry))
				{
					text = gtk_entry_get_text (GTK_ENTRY (entry->entry));
					if (*text == '\0')
					{
						if ((prop->value != NULL) && (*prop->value != '\0'))
						{
							/* Remove */
							ianjuta_project_set_property (table->project->project, table->node, prop, NULL, NULL);
						}
					}
					else
					{
						if (g_strcmp0 (prop->value, text) != 0)
						{
							/* Modified */
							ianjuta_project_set_property (table->project->project, table->node, prop, text, NULL);
						}
					}
				}
				break;
			case ANJUTA_PROJECT_PROPERTY_BOOLEAN:
				active = prop->value == NULL ? FALSE : (*prop->value == '1' ? TRUE : FALSE);
				text = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (entry->entry)) ? "1" : "0";
					
				if (active != (*text == '1'))
				{
					/* Modified */
					ianjuta_project_set_property (table->project->project, table->node, prop, text, NULL);
				}
				break;
			case ANJUTA_PROJECT_PROPERTY_MAP:
				break;
			default:
				break;
			}
		}
	}
	g_list_foreach (table->properties, (GFunc)pm_property_entry_free, NULL);
	g_free (table);
	gtk_widget_destroy (dialog);
}

static void
on_node_changed (GtkTreeView       *view,
                 gpointer           user_data)     
{
	PropertiesTable *table = (PropertiesTable *)user_data;
	GtkTreeIter iter;
	GtkTreePath *path;
	GtkTreeModel *model;

	model = gtk_tree_view_get_model (view);
	gtk_tree_view_get_cursor (view, &path, NULL);
	
	if (gtk_tree_model_get_iter (model, &iter, path))
	{
		GbfTreeData *data;
		
		gtk_tree_model_get (model, &iter, GBF_PROJECT_MODEL_COLUMN_DATA, &data, -1);

		if (table->data->properties_dialog != NULL)
		{
			g_object_remove_weak_pointer (G_OBJECT (table->dialog), (gpointer *)&table->data->properties_dialog);
			table->data->properties_dialog = NULL;
		}
		if (data->properties_dialog != NULL)
		{
			g_object_unref (data->properties_dialog);
		}
		table->data = data;
		data->properties_dialog = table->dialog;
		g_object_add_weak_pointer (G_OBJECT (table->dialog), (gpointer *)&table->data->properties_dialog);

		table->node = gbf_tree_data_get_node (data);
		update_properties (table);
	}

	if (path != NULL) gtk_tree_path_free (path);
}

static GtkWidget *
pm_project_create_properties_dialog (AnjutaPmProject *project, GtkWindow *parent, GbfProjectView *view, GbfTreeData *data, GtkTreeIter *selected)
{
	PropertiesTable *table;
	GtkWidget *dialog = NULL;
	GtkBuilder *bxml;
	GtkWidget *node_view;
	
	g_return_val_if_fail (data != NULL, NULL);
	
	bxml = anjuta_util_builder_new (GLADE_FILE, NULL);
	if (!bxml) return NULL;

	table = g_new0 (PropertiesTable, 1);
	table->data = data;
	table->node = gbf_tree_data_get_node (data);
	table->project = project;
	anjuta_util_builder_get_objects (bxml,
									"properties", &table->table,
									"head_table", &table->head,
	                                "nodes_view", &node_view,
									"main_table", &table->main,
									"extra_table", &table->extra,
									"extra_expand", &table->expand,
									NULL);
	g_object_ref (table->table);
	g_object_unref (bxml);

	/* Add tree view */
	setup_nodes_treeview (GBF_PROJECT_VIEW (node_view),
	                        view,
	                        NULL,
							is_project_node_but_shortcut,
							NULL,
							selected);
	gtk_widget_show (node_view);

	dialog = gtk_dialog_new_with_buttons (NULL,
						   parent,
						   GTK_DIALOG_DESTROY_WITH_PARENT,
						   GTK_STOCK_CANCEL,
						   GTK_RESPONSE_CANCEL,
						   GTK_STOCK_APPLY,
						   GTK_RESPONSE_APPLY, NULL);
	table->dialog = dialog;

	update_properties (table);
	
	g_signal_connect (node_view, "cursor-changed",
					G_CALLBACK (on_node_changed),
					table);
	
	g_signal_connect (dialog, "response",
					G_CALLBACK (on_properties_dialog_response),
					table);

	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG(dialog))),
					table->table);
	gtk_window_set_default_size (GTK_WINDOW (dialog), 450, -1);
	gtk_widget_show (dialog);
	
	return dialog;
}


/* Properties dialog
 *---------------------------------------------------------------------------*/

/* Display properties dialog. These dialogs are not modal, so a pointer on each
 * dialog is kept with in node data to be able to destroy them if the node is
 * removed. It is useful to put the dialog at the top if the same target is
 * selected while the corresponding dialog already exist instead of creating
 * two times the same dialog.
 * The project properties dialog is display if the node iterator is NULL. */

gboolean
anjuta_pm_project_show_properties_dialog (ProjectManagerPlugin *plugin, GtkTreeIter *selected)
{
	GtkWidget **dialog_ptr;
	GtkTreeIter iter;
	
	if (selected == NULL)
	{
		/* Display root properties by default */
		if (gbf_project_view_get_project_root (plugin->view, &iter))
		{
			selected = &iter;
		}
	}

	if (selected)
	{
		GbfTreeData *data;
		
		gtk_tree_model_get (GTK_TREE_MODEL (gbf_project_view_get_model (plugin->view)), selected, GBF_PROJECT_MODEL_COLUMN_DATA, &data, -1);

		dialog_ptr = &data->properties_dialog;
	
		if (*dialog_ptr != NULL)
		{
			/* Show already existing dialog */
			gtk_window_present (GTK_WINDOW (*dialog_ptr));
		}
		else
		{
			*dialog_ptr = pm_project_create_properties_dialog (
				plugin->project,
				GTK_WINDOW (plugin->project->plugin->shell),
			    plugin->view,
			    data,                                               
				selected);
			if (*dialog_ptr != NULL)
			{
				g_object_add_weak_pointer (G_OBJECT (*dialog_ptr), (gpointer *)dialog_ptr);
			}
		}
	}

	return selected != NULL;
}



/* Group dialog
 *---------------------------------------------------------------------------*/

AnjutaProjectNode*
anjuta_pm_project_new_group (ProjectManagerPlugin *plugin, GtkWindow *parent, GtkTreeIter *selected, const gchar *default_name)
{
	GtkBuilder *gui;
	GtkWidget *dialog, *group_name_entry, *ok_button;
	GtkWidget *groups_view;
	GtkTreePath *root;
	gint response;
	gboolean finished = FALSE;
	AnjutaProjectNode *new_group = NULL;

	g_return_val_if_fail (plugin->project != NULL, NULL);

	gui = load_interface ("new_group_dialog");
	g_return_val_if_fail (gui != NULL, NULL);

	/* get all needed widgets */
	dialog = GTK_WIDGET (gtk_builder_get_object (gui, "new_group_dialog"));
	groups_view = GTK_WIDGET (gtk_builder_get_object (gui, "groups_view"));
	group_name_entry = GTK_WIDGET (gtk_builder_get_object (gui, "group_name_entry"));
	ok_button = GTK_WIDGET (gtk_builder_get_object (gui, "ok_group_button"));

	/* set up dialog */
	if (default_name)
		gtk_entry_set_text (GTK_ENTRY (group_name_entry),
							default_name);
	g_signal_connect (group_name_entry, "changed",
						(GCallback) entry_changed_cb, ok_button);
	if (default_name)
		gtk_widget_set_sensitive (ok_button, TRUE);
	else
		gtk_widget_set_sensitive (ok_button, FALSE);

	root = gbf_project_model_get_project_root (gbf_project_view_get_model (plugin->view));
	setup_nodes_treeview (GBF_PROJECT_VIEW (groups_view),
	                        plugin->view,
	                      	root,
							parent_filter_func,
							GINT_TO_POINTER (ANJUTA_PROJECT_GROUP),
							selected);
	gtk_tree_path_free (root);
	gtk_widget_show (groups_view);

	if (parent)
	{
		gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);
	}

	/* execute dialog */
	while (!finished)
	{
		response = gtk_dialog_run (GTK_DIALOG (dialog));

		switch (response) {
			case GTK_RESPONSE_OK: 
			{
				GError *err = NULL;
				AnjutaProjectNode *group;
				gchar *name;

				name = gtk_editable_get_chars (
						GTK_EDITABLE (group_name_entry), 0, -1);

				group = gbf_project_view_find_selected (GBF_PROJECT_VIEW (groups_view),
														ANJUTA_PROJECT_GROUP);
				if (group)
				{
					new_group = anjuta_pm_project_add_group (plugin->project, group, NULL, name, &err);
					if (err)
					{
						error_dialog (parent, _("Cannot add group"), "%s",
										err->message);
						g_error_free (err);
					}
					else
					{
						finished = TRUE;
					}
				}
				else
				{
					error_dialog (parent, _("Cannot add group"),
									"%s", _("No parent group selected"));
				}
				g_free (name);
				break;
			}
			default:
				finished = TRUE;
				break;
		}
	}

	/* destroy stuff */
	gtk_widget_destroy (dialog);
	g_object_unref (gui);
	
	return new_group;
}

/* Source dialog
 *---------------------------------------------------------------------------*/

AnjutaProjectNode*
anjuta_pm_project_new_source (ProjectManagerPlugin *plugin,
							GtkWindow           *parent,
							GtkTreeIter         *default_parent,
							const gchar         *default_uri)
{
	GList* new_sources;
	gchar* uri = NULL;
	GList* uris = NULL;
	
	if (default_uri)
	{
		uri = g_strdup (default_uri);
		uris = g_list_append (NULL, uri);
	}
	new_sources = 
		anjuta_pm_project_new_multiple_source (plugin, parent,
										default_parent, uris);
	g_free (uri);
	g_list_free (uris);
	
	if (new_sources && g_list_length (new_sources))
	{
		AnjutaProjectNode *new_source = new_sources->data;
		g_list_free (new_sources);
		return new_source;
	}
	else
		return NULL;
}

GList* 
anjuta_pm_project_new_multiple_source (ProjectManagerPlugin *plugin,
								GtkWindow           *top_window,
								GtkTreeIter         *default_parent,
								GList               *uris_to_add)
{
	GtkBuilder *gui;
	GtkWidget *dialog, *source_file_tree;
	GtkWidget *ok_button, *browse_button;
	GtkWidget *targets_view;
	gint response;
	gboolean finished = FALSE;
	GtkListStore* list;
	GtkCellRenderer* renderer;
	GtkTreeViewColumn* column_filename;
	GList* new_sources = NULL;
	GList* uri_node;
	GtkTreePath *root;

	g_return_val_if_fail (plugin->project != NULL, NULL);

	gui = load_interface ("add_source_dialog");
	g_return_val_if_fail (gui != NULL, NULL);

	/* get all needed widgets */
	dialog = GTK_WIDGET (gtk_builder_get_object (gui, "add_source_dialog"));
	targets_view = GTK_WIDGET (gtk_builder_get_object (gui, "targets_view"));
	source_file_tree = GTK_WIDGET (gtk_builder_get_object (gui, "source_file_tree"));
	browse_button = GTK_WIDGET (gtk_builder_get_object (gui, "browse_button"));
	ok_button = GTK_WIDGET (gtk_builder_get_object (gui, "ok_source_button"));

	/* Prepare file tree */
	list = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model (GTK_TREE_VIEW (source_file_tree),
								GTK_TREE_MODEL (list));
	renderer = gtk_cell_renderer_text_new ();
	column_filename = gtk_tree_view_column_new_with_attributes ("Files",
							        renderer,
							        "text",
							        COLUMN_FILE,
							        NULL);
	gtk_tree_view_column_set_sizing (column_filename,
				     GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column (GTK_TREE_VIEW (source_file_tree),
				 column_filename);

	/* set up dialog */
	uri_node = uris_to_add;
	while (uri_node) 
	{
		GtkTreeIter iter;
		gchar* filename = g_path_get_basename (uri_node->data);
		if (!filename)
			filename = g_strdup (uri_node->data);
		gtk_list_store_append (list, &iter);
        gtk_list_store_set (list, &iter, COLUMN_FILE, filename,
			    COLUMN_URI, g_strdup(uri_node->data), -1);
		g_free (filename);
		uri_node = g_list_next (uri_node);
	}
	if (!g_list_length (uris_to_add))
		gtk_widget_set_sensitive (ok_button, FALSE);
	else
		gtk_widget_set_sensitive (ok_button, TRUE);

	g_signal_connect (G_OBJECT(list), "row_changed",
					G_CALLBACK(on_row_changed), ok_button);
    
	g_signal_connect (browse_button, "clicked",
					G_CALLBACK (browse_button_clicked_cb), source_file_tree);

	g_object_set_data_full (G_OBJECT (browse_button), "treeview", targets_view, NULL);
	/*project_root = g_file_get_uri (anjuta_project_node_get_file (anjuta_pm_project_get_root (project)));
	g_object_set_data_full (G_OBJECT (browse_button), "root", project_root, g_free);*/

	root = gbf_project_model_get_project_root_group (gbf_project_view_get_model (plugin->view));
	setup_nodes_treeview (GBF_PROJECT_VIEW (targets_view),
	                        plugin->view,
	                       	root,
							parent_filter_func,
							GINT_TO_POINTER (ANJUTA_PROJECT_SOURCE),
							default_parent);
	gtk_tree_path_free (root);
	gtk_widget_show (targets_view);

	if (top_window) {
		gtk_window_set_transient_for (GTK_WINDOW (dialog), top_window);
	}

	if (default_parent)
		gtk_widget_grab_focus (source_file_tree);
	else
		gtk_widget_grab_focus (targets_view);

	/* execute dialog */
	while (!finished) {
		response = gtk_dialog_run (GTK_DIALOG (dialog));

		switch (response)
		{
		case GTK_RESPONSE_OK: 
		{
			AnjutaProjectNode *parent = NULL;
			AnjutaProjectNode *sibling = NULL;

			parent = gbf_project_view_find_selected (GBF_PROJECT_VIEW (targets_view),
							ANJUTA_PROJECT_UNKNOWN);

			/* Check that selected node can be used as a parent or a sibling */
			if (parent)
			{
				if (!(anjuta_project_node_get_state (parent) & ANJUTA_PROJECT_CAN_ADD_SOURCE))
				{
					sibling = parent;
					parent = anjuta_project_node_parent (parent);
				}
				if (!(anjuta_project_node_get_state (parent) & ANJUTA_PROJECT_CAN_ADD_SOURCE))
				{
					parent = NULL;
				}
			}
			
			if (parent)
			{
				GtkTreeIter iter;
				GString *err_mesg = g_string_new (NULL);

				if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list),
								&iter))
					break;
				do
				{
					GError *err = NULL;
					AnjutaProjectNode* new_source;
					gchar* uri;

					gtk_tree_model_get (GTK_TREE_MODEL(list), &iter,
						COLUMN_URI, &uri, -1);

					new_source = anjuta_pm_project_add_source (plugin->project,
									parent,
									sibling,
									uri,
									&err);
					if (err) {
						gchar *str = g_strdup_printf ("%s: %s\n",
								uri,
								err->message);
						g_string_append (err_mesg, str);
						g_error_free (err);
						g_free (str);
					}
					else
						new_sources = g_list_append (new_sources,
								new_source);

					g_free (uri);
				} while (gtk_tree_model_iter_next (GTK_TREE_MODEL(list),
								&iter));

				if (err_mesg->str && strlen (err_mesg->str) > 0)
				{
					error_dialog (top_window, _("Cannot add source files"),
							"%s", err_mesg->str);
				}
				else {
					finished = TRUE;
				}
				g_string_free (err_mesg, TRUE);
			}
			else 
			{
				error_dialog (top_window, _("Cannot add source files"),
						"%s", _("The selected node cannot contain source files."));
			}
			break;
		}
		default:
			gtk_list_store_clear (GTK_LIST_STORE (list));
			finished = TRUE;
			break;
		}
	}

	/* destroy stuff */
	gtk_widget_destroy (dialog);
	g_object_unref (gui);
	
	return new_sources;
}

/* Target dialog
 *---------------------------------------------------------------------------*/

enum {
    TARGET_TYPE_TYPE = 0,
    TARGET_TYPE_NAME,
    TARGET_TYPE_PIXBUF,
    TARGET_TYPE_N_COLUMNS
};

/* create a tree model with the target types */
static GtkListStore *
build_types_store (AnjutaPmProject *project, AnjutaProjectNodeType store_type)
{
    GtkListStore *store;
    GtkTreeIter iter;
    const GList *types;
    const GList *node;

    types = anjuta_pm_project_get_node_info (project);
    store = gtk_list_store_new (TARGET_TYPE_N_COLUMNS,
                                G_TYPE_POINTER,
                                G_TYPE_STRING,
                                GDK_TYPE_PIXBUF);
    
    for (node = types; node != NULL; node = g_list_next (node)) {
        GdkPixbuf *pixbuf;
        const gchar *name;
        AnjutaProjectNodeType type;

		type = anjuta_project_node_info_type ((AnjutaProjectNodeInfo *)node->data);
        if (((store_type == 0) || ((type & ANJUTA_PROJECT_TYPE_MASK) == store_type)) && !(type & ANJUTA_PROJECT_READ_ONLY))
        {
            name = anjuta_project_node_info_name ((AnjutaProjectNodeInfo *)node->data);
            pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default(),
                                            GTK_STOCK_CONVERT,
                                            ICON_SIZE,
                                            GTK_ICON_LOOKUP_GENERIC_FALLBACK,
                                            NULL);

            gtk_list_store_append (store, &iter);
            gtk_list_store_set (store, &iter,
                                TARGET_TYPE_TYPE, type,
                                TARGET_TYPE_NAME, name,
                                TARGET_TYPE_PIXBUF, pixbuf,
                                -1);

            if (pixbuf)
                g_object_unref (pixbuf);
        }
    }

    return store;
}

AnjutaProjectNode* 
anjuta_pm_project_new_target (ProjectManagerPlugin *plugin,
                             GtkWindow       *parent,
                             GtkTreeIter     *default_group,
                             const gchar     *default_target_name_to_add)
{
	GtkBuilder *gui;
	GtkWidget *dialog, *target_name_entry, *ok_button;
	GtkWidget *target_type_combo, *groups_view;
	GtkTreePath *root;
	GtkListStore *types_store;
	GtkCellRenderer *renderer;
	gint response;
	gboolean finished = FALSE;
	AnjutaProjectNode *new_target = NULL;

	g_return_val_if_fail (plugin->project != NULL, NULL);

	gui = load_interface ("new_target_dialog");
	g_return_val_if_fail (gui != NULL, NULL);

	/* get all needed widgets */
	dialog = GTK_WIDGET (gtk_builder_get_object (gui, "new_target_dialog"));
	groups_view = GTK_WIDGET (gtk_builder_get_object (gui, "target_groups_view"));
	target_name_entry = GTK_WIDGET (gtk_builder_get_object (gui, "target_name_entry"));
	target_type_combo = GTK_WIDGET (gtk_builder_get_object (gui, "target_type_combo"));
	ok_button = GTK_WIDGET (gtk_builder_get_object (gui, "ok_target_button"));

	/* set up dialog */
	if (default_target_name_to_add)
		gtk_entry_set_text (GTK_ENTRY (target_name_entry),
							default_target_name_to_add);
	g_signal_connect (target_name_entry, "changed",
						(GCallback) entry_changed_cb, ok_button);
	if (default_target_name_to_add)
		gtk_widget_set_sensitive (ok_button, TRUE);
	else
		gtk_widget_set_sensitive (ok_button, FALSE);

	root = gbf_project_model_get_project_root_group (gbf_project_view_get_model (plugin->view));
	setup_nodes_treeview (GBF_PROJECT_VIEW (groups_view),
	                        plugin->view,
	                       	root,
							parent_filter_func,
							GINT_TO_POINTER (ANJUTA_PROJECT_TARGET),
							default_group);
	gtk_tree_path_free (root);
	gtk_widget_show (groups_view);

	/* setup target types combo box */
	types_store = build_types_store (plugin->project, ANJUTA_PROJECT_TARGET);
	gtk_combo_box_set_model (GTK_COMBO_BOX (target_type_combo), 
							GTK_TREE_MODEL (types_store));

	/* create cell renderers */
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (target_type_combo),
								renderer, FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (target_type_combo),
									renderer,
									"pixbuf", TARGET_TYPE_PIXBUF,
									NULL);

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (target_type_combo),
								renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (target_type_combo),
									renderer,
									"text", TARGET_TYPE_NAME,
									NULL);
	gtk_widget_show (target_type_combo);

	/* preselect */
	gtk_combo_box_set_active (GTK_COMBO_BOX (target_type_combo), 0);

	if (parent)
	{
		gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);
	}

	/* execute dialog */
	while (!finished)
	{
		response = gtk_dialog_run (GTK_DIALOG (dialog));

		switch (response)
		{
			case GTK_RESPONSE_OK: 
			{
				GError *err = NULL;
				AnjutaProjectNode *group;
				GtkTreeIter iter;
				gchar *name;
				AnjutaProjectNodeType type;

				name = gtk_editable_get_chars (
						GTK_EDITABLE (target_name_entry), 0, -1);
				group = gbf_project_view_find_selected (GBF_PROJECT_VIEW (groups_view),
														ANJUTA_PROJECT_GROUP);

				/* retrieve target type */
				if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (target_type_combo), &iter))
				{
					gtk_tree_model_get (GTK_TREE_MODEL (types_store), &iter, 
										TARGET_TYPE_TYPE, &type,
										-1);
				}

				if (group && type)
				{
					new_target = anjuta_pm_project_add_target (plugin->project, group, NULL, name, type, &err);
					if (err)
					{
						error_dialog (parent, _("Cannot add target"), "%s",
							err->message);
						g_error_free (err);
					}
					else
					{
						finished = TRUE;
					}
				}
				else
				{
						error_dialog (parent, _("Cannot add target"), "%s",
									_("No group selected"));
				}

				g_free (name);

				break;
			}
			default:
				finished = TRUE;
				break;
		}
	}

	/* destroy stuff */
	g_object_unref (types_store);
	gtk_widget_destroy (dialog);
	g_object_unref (gui);

	return new_target;
}

 
/* Module dialog
 *---------------------------------------------------------------------------*/

static void
on_cursor_changed(GtkTreeView* view, gpointer data)
{
	GtkWidget* button = GTK_WIDGET(data);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(view);
 
	if (gtk_tree_selection_count_selected_rows (selection) > 0)
		gtk_widget_set_sensitive(button, TRUE);
	else
		gtk_widget_set_sensitive(button, FALSE);
}

GList*
anjuta_pm_project_new_module (ProjectManagerPlugin *plugin,
                             GtkWindow          *parent,
                             GtkTreeIter        *default_target,
                             const gchar        *default_module)
{
	GtkBuilder *gui;
	GtkWidget *dialog;
	GtkWidget *ok_button, *new_button;
	GtkWidget *targets_view;
	GtkWidget *modules_view;
	GtkTreePath *root;
	gint response;
	gboolean finished = FALSE;
	GList* new_modules = NULL;
	GtkTreeSelection *module_selection;

	g_return_val_if_fail (plugin->project != NULL, NULL);

	gui = load_interface ("add_module_dialog");
	g_return_val_if_fail (gui != NULL, NULL);

	/* get all needed widgets */
	dialog = GTK_WIDGET (gtk_builder_get_object (gui, "add_module_dialog"));
	targets_view = GTK_WIDGET (gtk_builder_get_object (gui, "module_targets_view"));
	modules_view = GTK_WIDGET (gtk_builder_get_object (gui, "modules_view"));
	new_button = GTK_WIDGET (gtk_builder_get_object (gui, "new_package_button"));
	ok_button = GTK_WIDGET (gtk_builder_get_object (gui, "ok_module_button"));

	root = gbf_project_model_get_project_root_group (gbf_project_view_get_model (plugin->view));
	setup_nodes_treeview (GBF_PROJECT_VIEW (targets_view),
	                        plugin->view,
	                       	root,
							parent_filter_func,
							GINT_TO_POINTER (ANJUTA_PROJECT_MODULE),
							default_target);
	gtk_tree_path_free (root);
	gtk_widget_show (targets_view);
	root = gbf_project_model_get_project_root (gbf_project_view_get_model (plugin->view));
	setup_nodes_treeview (GBF_PROJECT_VIEW (modules_view),
	                        plugin->view,
	                        root,
							module_filter_func,
							NULL,
							NULL);
	gtk_tree_path_free (root);
	gtk_widget_show (modules_view);
	module_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (modules_view));
	gtk_tree_selection_set_mode (module_selection, GTK_SELECTION_MULTIPLE);

	if (gbf_project_view_find_selected (GBF_PROJECT_VIEW (modules_view), ANJUTA_PROJECT_MODULE))
	{
		gtk_widget_set_sensitive (ok_button, TRUE);
	}
	else
	{
		gtk_widget_set_sensitive (ok_button, FALSE);
	}
	g_signal_connect (G_OBJECT(modules_view), "cursor-changed",
						G_CALLBACK(on_cursor_changed), ok_button);


	if (parent)
	{
		gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);
	}

	if (default_module)
		gtk_widget_grab_focus (modules_view);
	else
		gtk_widget_grab_focus (targets_view);

	/* execute dialog */
	while (!finished)
	{
		response = gtk_dialog_run (GTK_DIALOG (dialog));

		switch (response) {
			case 1:
			{
				anjuta_pm_project_new_package (plugin, parent, NULL, NULL);

				break;
			}
			case GTK_RESPONSE_OK: 
			{
				AnjutaProjectNode *target;

				target = gbf_project_view_find_selected (GBF_PROJECT_VIEW (targets_view),
														ANJUTA_PROJECT_TARGET);
				if (target)
				{
					GString *err_mesg = g_string_new (NULL);
					GList *list;
					GList *node;

					list = gbf_project_view_get_all_selected (GBF_PROJECT_VIEW (modules_view));
					for (node = g_list_first (list); node != NULL; node = g_list_next (node))
					{
						GError *error = NULL;
						AnjutaProjectNode* new_module;
						const gchar *name;

						new_module = gbf_tree_data_get_node (node->data);
						name = anjuta_project_node_get_name (new_module);

						new_module = ianjuta_project_add_node_after (plugin->project->project, target, NULL, ANJUTA_PROJECT_MODULE, NULL, name, &error);
						if (error) {
							gchar *str = g_strdup_printf ("%s: %s\n",
															name,
															error->message);
							g_string_append (err_mesg, str);
							g_error_free (error);
							g_free (str);
						}
						else
							new_modules = g_list_append (new_modules, new_module);
					}
					g_list_free (list);

					if (err_mesg->str && strlen (err_mesg->str) > 0)
					{
						error_dialog (parent, _("Cannot add modules"),
									"%s", err_mesg->str);
					}
					else
					{
						finished = TRUE;
					}
					g_string_free (err_mesg, TRUE);
				}
				else
				{
					error_dialog (parent, _("Cannot add modules"),
									"%s", _("No target has been selected"));
				}

				break;
			}
			default:
				finished = TRUE;
				break;
		}
	}

	/* destroy stuff */
	gtk_widget_destroy (dialog);
	g_object_unref (gui);

	return new_modules;
}

/* Package dialog
 *---------------------------------------------------------------------------*/

static void on_changed_disconnect (GtkEditable* entry, gpointer data);

static void
on_cursor_changed_set_entry(GtkTreeView* view, gpointer data)
{
    GtkWidget* entry = GTK_WIDGET(data);
    AnjutaPkgConfigChooser* chooser = ANJUTA_PKG_CONFIG_CHOOSER (view);
	GList* packages = anjuta_pkg_config_chooser_get_active_packages (chooser);
	
    if (packages)
    {
		gchar* name = packages->data;
		gchar* ptr;
		/* Remove numeric suffix */
		ptr = name + strlen(name) - 1;
		while (g_ascii_isdigit (*ptr))
		{
			while (g_ascii_isdigit (*ptr)) ptr--;
			if ((*ptr != '_') && (*ptr != '-') && (*ptr != '.')) break;
			*ptr = '\0';
			ptr--;
		}

		/* Convert to upper case and remove invalid characters */
		for (ptr = name; *ptr != '\0'; ptr++)
		{
			if (g_ascii_isalnum (*ptr))
			{
				*ptr = g_ascii_toupper (*ptr);
			}
			else
			{
				*ptr = '_';
			}
		}

		g_signal_handlers_block_by_func (G_OBJECT (entry), on_changed_disconnect, view);
		gtk_entry_set_text (GTK_ENTRY (entry), name);
		g_signal_handlers_unblock_by_func (G_OBJECT (entry), on_changed_disconnect, view);
        anjuta_util_glist_strings_free (packages);
    }
}

static void
on_changed_disconnect (GtkEditable* entry, gpointer data)
{
    g_signal_handlers_block_by_func (G_OBJECT (data), on_cursor_changed_set_entry, entry);
}

static void
on_pkg_chooser_selection_changed (AnjutaPkgConfigChooser* chooser,
                                  gchar* package,
                                  gpointer data)
{
	GtkWidget* button = GTK_WIDGET(data);
	GList* packages = anjuta_pkg_config_chooser_get_active_packages (chooser);

	if (packages != NULL)
		gtk_widget_set_sensitive(button, TRUE);
	else
		gtk_widget_set_sensitive(button, FALSE);

	anjuta_util_glist_strings_free (packages);
}

GList* 
anjuta_pm_project_new_package (ProjectManagerPlugin *plugin,
                              GtkWindow        *parent,
                              GtkTreeIter      *default_module,
                              GList            *packages_to_add)
{
    GtkBuilder *gui;
    GtkWidget *dialog;
    GtkWidget *ok_button;
    GtkWidget *module_entry;
    GtkWidget *packages_view;
    GList *packages = NULL;
    gint response;
    gboolean finished = FALSE;
	GtkListStore *store;
    AnjutaProjectNode *root;
	AnjutaProjectNode *node;
	AnjutaProjectNode *module = NULL;
    gint default_pos = -1;
	gint pos;
    
    g_return_val_if_fail (plugin->project != NULL, NULL);
    
    gui = load_interface ("add_package_dialog");
    g_return_val_if_fail (gui != NULL, NULL);
    
    /* get all needed widgets */
    dialog = GTK_WIDGET (gtk_builder_get_object (gui, "add_package_dialog"));
    module_entry = GTK_WIDGET (gtk_builder_get_object (gui, "module_entry"));
    packages_view = GTK_WIDGET (gtk_builder_get_object (gui, "packages_view"));
    ok_button = GTK_WIDGET (gtk_builder_get_object (gui, "ok_package_button"));

	/* Get default parent */
	if (default_module != NULL)
	{
        GbfTreeData *data;
	    GbfProjectModel *model;

		model = gbf_project_view_get_model(plugin->view);
		gtk_tree_model_get (GTK_TREE_MODEL (model), default_module, GBF_PROJECT_MODEL_COLUMN_DATA, &data, -1);
		if (data != NULL)
		{
			module = gbf_tree_data_get_node (data);
		}
	}
	
    /* Fill combo box with modules */
    store = gtk_list_store_new(1, G_TYPE_STRING);	
    gtk_combo_box_set_entry_text_column (GTK_COMBO_BOX (module_entry), 0);

	root = ianjuta_project_get_root (plugin->project->project, NULL);
	pos = 0;
	for (node = anjuta_project_node_first_child (root); node != NULL; node = anjuta_project_node_next_sibling (node))
	{
		if (anjuta_project_node_get_node_type (node) == ANJUTA_PROJECT_MODULE)
		{
			GtkTreeIter list_iter;
			const gchar *name;

			name = anjuta_project_node_get_name (node);
			gtk_list_store_append (store, &list_iter);
			gtk_list_store_set (store, &list_iter, 0, name, -1);

			if (node == module)
			{
				default_pos = pos;
			}
			pos ++;
		}
    }
    gtk_combo_box_set_model (GTK_COMBO_BOX(module_entry), GTK_TREE_MODEL(store));
    gtk_combo_box_set_entry_text_column (GTK_COMBO_BOX (module_entry), 0);
    g_object_unref (store);
    if (default_pos >= 0)
    {
        gtk_combo_box_set_active (GTK_COMBO_BOX (module_entry), default_pos);
    }
    else
    {
        /* Create automatically a module name from the package name when missing */
        GtkWidget *entry = gtk_bin_get_child (GTK_BIN (module_entry));
        
        g_signal_connect (G_OBJECT(packages_view), "cursor-changed",
            G_CALLBACK(on_cursor_changed_set_entry), entry);
        g_signal_connect (G_OBJECT(entry), "changed",
            G_CALLBACK(on_changed_disconnect), packages_view);
    }
    
    /* Fill package list */
	anjuta_pkg_config_chooser_show_active_column (ANJUTA_PKG_CONFIG_CHOOSER (packages_view),
	                                              TRUE);
    g_signal_connect (G_OBJECT(packages_view), "package-activated",
        G_CALLBACK(on_pkg_chooser_selection_changed), ok_button);
    g_signal_connect (G_OBJECT(packages_view), "package-deactivated",
        G_CALLBACK(on_pkg_chooser_selection_changed), ok_button);
	
    if (parent) {
        gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);
    }
    
    /* execute dialog */
    while (!finished) {
        response = gtk_dialog_run (GTK_DIALOG (dialog));

        switch (response) {
            case GTK_RESPONSE_OK: 
            {
                gchar *name;
				AnjutaProjectNode *module = NULL;
				GString *error_message = g_string_new (NULL);

				name = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (module_entry)))));
				if (name != NULL) name = g_strstrip (name);

				if ((name == NULL) || (*name == '\0'))
				{
					/* Missing module name */
					g_string_append (error_message, _("Missing module name"));
				}
				else
				{
					/* Look for already existing module */
					module = anjuta_pm_project_get_module (plugin->project, name);
					if (module == NULL)
					{
						/* Create new module */
						AnjutaProjectNode *root;
						GError *error = NULL;

						root = ianjuta_project_get_root (plugin->project->project, NULL);

						module = ianjuta_project_add_node_after (plugin->project->project, root, NULL, ANJUTA_PROJECT_MODULE, NULL, name, &error);
						if (error != NULL)
						{
                    		gchar *str = g_strdup_printf ("%s: %s\n", name, error->message);
						
							g_string_append (error_message, str);
                        	g_error_free (error);
                        	g_free (str);
						}
					}
				}
				g_free (name);
				
                if (module != NULL)
                {
                    GList *list;
                    GList *node;
                    
                    list = anjuta_pkg_config_chooser_get_active_packages (ANJUTA_PKG_CONFIG_CHOOSER (packages_view));
                    for (node = list; node != NULL; node = g_list_next (node))
                    {
						gchar *name;
						AnjutaProjectNode* new_package;
						GError *error = NULL;

						name = node->data;

						new_package = ianjuta_project_add_node_after (plugin->project->project, module, NULL, ANJUTA_PROJECT_PACKAGE, NULL, name, &error);
						if (error)
						{
							gchar *str = g_strdup_printf ("%s: %s\n",
							                              name,
							                              error->message);
							g_string_append (error_message, str);
							g_error_free (error);
							g_free (str);
						}
						else
						{
							packages = g_list_append (packages, new_package);
							finished = TRUE;
						}
					}
					anjuta_util_glist_strings_free (list);
				}
					
                if (error_message->len != 0)
                {
                    error_dialog (parent, _("Cannot add packages"),
                            "%s",error_message->str);
                }
				else
				{
					finished = TRUE;
				}
				g_string_free (error_message, TRUE);
                break;
            }
            default:
                finished = TRUE;
                break;
        }
    }
    
    /* destroy stuff */
    gtk_widget_destroy (dialog);
    g_object_unref (gui);
    
    return packages;
}
