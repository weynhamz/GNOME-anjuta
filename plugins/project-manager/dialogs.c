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
#include "pkg-config.h"

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-utils.h>

#define ICON_SIZE 16

#define GLADE_FILE  PACKAGE_DATA_DIR "/glade/pm_dialogs.ui"

/* Types
  *---------------------------------------------------------------------------*/

typedef struct _PropertiesTable
{
	AnjutaPmProject *project;
	GtkWidget *table;
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

static PropertyValue*
pm_property_value_new (AnjutaProjectProperty *property, const gchar *value)
{
	PropertyValue *prop;
	
	prop = g_slice_new0(PropertyValue);
	prop->property = property;
	prop->value = value;
	
	return prop;
}

static void
pm_property_value_free (PropertyValue *prop)
{
	g_slice_free (PropertyValue, prop);
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
		need = ANJUTA_PROJECT_CAN_ADD_MODULE;
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
	node = gbf_tree_data_get_node (data);
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
	node = gbf_tree_data_get_node (data);
	if (node != NULL)
	{
		AnjutaProjectNodeType type = anjuta_project_node_get_node_type (node);
		
		visible = (type == ANJUTA_PROJECT_MODULE) || (type == ANJUTA_PROJECT_PACKAGE);
	}
	
	return visible;
}

static void 
setup_nodes_treeview (GbfProjectModel         *model,
						GtkWidget              *view,
						GtkTreeModelFilterVisibleFunc func,
						gpointer               data,
						GtkTreeIter            *selected)
{
	GtkTreeModel *filter;
	GtkTreeIter iter_filter;
	GtkTreePath *path;

	g_return_if_fail (model != NULL);
	g_return_if_fail (view != NULL && GBF_IS_PROJECT_VIEW (view));

	filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (model), NULL);
	gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (filter),
											func, data, NULL);
	gtk_tree_view_set_model (GTK_TREE_VIEW (view), GTK_TREE_MODEL (filter));
	g_object_unref (filter);

	/* select default group */
	if (selected && gtk_tree_model_filter_convert_child_iter_to_iter (
			GTK_TREE_MODEL_FILTER (filter), &iter_filter, selected))
	{
		path = gtk_tree_model_get_path (filter, &iter_filter);
		gtk_tree_view_expand_to_path (GTK_TREE_VIEW (view), path);
	}
	else
	{
		GtkTreePath *root_path;

		gtk_tree_view_expand_all (GTK_TREE_VIEW (view));
		root_path = gbf_project_model_get_project_root (model);
		if (root_path)
		{
			path = gtk_tree_model_filter_convert_child_path_to_path (
						GTK_TREE_MODEL_FILTER (filter), root_path);
			gtk_tree_path_free (root_path);
		}
		else
		{
			path = gtk_tree_path_new_first ();
		}
	}

	gtk_tree_view_set_cursor (GTK_TREE_VIEW (view), path, NULL, FALSE);
	gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (view), path, NULL,
									TRUE, 0.5, 0.0);
	gtk_tree_path_free (path);
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
	AnjutaProjectPropertyInfo *info = anjuta_project_property_get_info (prop);

	if (info->override)
	{
		label = gtk_label_new (_(((AnjutaProjectPropertyInfo *)info->override->data)->name));
	}
	else
	{
		label = gtk_label_new (_(info->name));
	}
	gtk_misc_set_alignment (GTK_MISC (label), 0, -1);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, *position, *position+1,
			  GTK_FILL, GTK_FILL, 5, 3);

	switch (info->type)
	{
	case ANJUTA_PROJECT_PROPERTY_STRING:
		entry = gtk_entry_new ();
		gtk_entry_set_text (GTK_ENTRY (entry), info->value != NULL ? info->value : "");
		break;
	case ANJUTA_PROJECT_PROPERTY_BOOLEAN:
		entry = gtk_check_button_new ();
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (entry), (info->value != NULL) && (*info->value == '1'));
		break;
	case ANJUTA_PROJECT_PROPERTY_LIST:
			model = GTK_TREE_MODEL (gtk_list_store_newv (LIST_COLUMNS_NB, column_type));

			while ((info->override != NULL) && (prop != NULL))
			{
				info = anjuta_project_property_get_info (prop);

				gtk_list_store_append (GTK_LIST_STORE (model), &iter);
				gtk_list_store_set (GTK_LIST_STORE (model), &iter, NAME_COLUMN, info->name, VALUE_COLUMN, info->value, -1);

				prop = anjuta_project_property_next_item (prop);
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

static PropertiesTable*
create_properties_table (IAnjutaProject *project, AnjutaProjectNode *node)
{
	GtkBuilder *bxml;
	PropertiesTable *table;
	GtkWidget *properties;
	GtkWidget *main_table;
	GtkWidget *extra_table;
	GtkWidget *extra_expand;

	gint main_pos;
	gint extra_pos;
	gchar *path;
	GList *item;
	AnjutaProjectNodeType type;
	AnjutaProjectNodeInfo* node_info;
	gboolean single;
	AnjutaProjectProperty *valid_prop;


	bxml = anjuta_util_builder_new (GLADE_FILE, NULL);
	if (!bxml) return NULL;

	table = g_new0 (PropertiesTable, 1);
	table->node = node;
	anjuta_util_builder_get_objects (bxml,
									"properties", &properties,
									"main_table", &main_table,
									"extra_table", &extra_table,
									"extra_expand", &extra_expand,
									NULL);
	table->table = properties;
	g_object_ref (properties);

	main_pos = 0;
	extra_pos = 0;

	/* Always display node name */
	path = g_file_get_path (anjuta_project_node_get_file (node));
	add_label (_("Full Name:"), path, main_table, &main_pos);
	g_free (path);

	/* Display node type only if several types are possible */
	node_info = NULL;
	single = TRUE;
	type = anjuta_project_node_get_node_type (node);
	for (item = ianjuta_project_get_node_info (project, NULL); item != NULL; item = g_list_next (item))
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
		if (!single && (node_info != NULL))
		{
			add_label (_("Type:"), anjuta_project_node_info_name (node_info), main_table, &main_pos);
			break;
		}
	}

	/* Display other node properties */
	single = FALSE;
	for (valid_prop = anjuta_project_node_first_valid_property (node); valid_prop != NULL; valid_prop = anjuta_project_property_next (valid_prop))
	{
		AnjutaProjectProperty *prop;
		GtkWidget *entry;

		prop = anjuta_project_node_get_property (node, valid_prop);
		if (prop != NULL)
		{
			/* This property has been set, display it in the main part */
			entry = add_entry (project, node, prop, main_table, &main_pos);
		}
		else
		{
			/* This property has not been set, hide it by default */
			entry = add_entry (project, node, valid_prop, extra_table, &extra_pos);
			single = TRUE;
		}

		if (entry != NULL)
		{
			table->properties = g_list_prepend (table->properties,
					pm_property_entry_new (entry, valid_prop));
		}
	}
	table->properties = g_list_reverse (table->properties);

	gtk_widget_show_all (properties);
	
	/* Hide expander if it is empty */
	if (!single) gtk_widget_hide (extra_expand);
	
	g_object_unref (bxml);
	
	return table;
}

static void
on_properties_dialog_response (GtkWidget *dialog,
							   gint id,
							   PropertiesTable *table)
{
	if (id == GTK_RESPONSE_APPLY)
	{
		GList *item;
		GList *modified = NULL;
		
		/* Get all modified properties */
		for (item = g_list_first (table->properties); item != NULL; item = g_list_next (item))
		{
			PropertyEntry *entry = (PropertyEntry *)item->data;
			AnjutaProjectProperty *prop;
			AnjutaProjectPropertyInfo *info;
			const gchar *text;
			
			/* Get property value in node */
			prop = anjuta_project_node_get_property (table->node, entry->property);
			if (prop == NULL) prop = entry->property;
			
			info = anjuta_project_property_get_info (prop);
			switch (info->type)
			{
			case ANJUTA_PROJECT_PROPERTY_STRING:
				text = gtk_entry_get_text (GTK_ENTRY (entry->entry));
				if (*text == '\0')
				{
					if ((info->value != NULL) && (*info->value != '\0'))
					{
						/* Remove */
						PropertyValue *value;
						
						value = g_slice_new0 (PropertyValue);
						value->property = prop;
						modified = g_list_prepend (modified, value);
					}
				}
				else
				{
					if (g_strcmp0 (info->value, text) != 0)
					{
						/* Modified */
						PropertyValue *value;
						
						value = g_slice_new0 (PropertyValue);
						value->property = prop;
						value->value = text;
						modified = g_list_prepend (modified, value);
					}
				}
				break;
			case ANJUTA_PROJECT_PROPERTY_BOOLEAN:
				text = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (entry->entry)) ? "1" : "0";
				if (g_strcmp0 (info->value, text) != 0)
				{
					/* Modified */
					PropertyValue *value;
						
					value = g_slice_new0 (PropertyValue);
					value->property = prop;
					value->value = text;
					modified = g_list_prepend (modified, value);
				}
				break;
			case ANJUTA_PROJECT_PROPERTY_LIST:
				break;
			default:
				break;
			}
		}

		/* Update all modified properties */
		anjuta_pm_project_set_properties (table->project, table->node, modified, NULL);

		/* Display modified properties */
		/*for (item = g_list_first (modified); item != NULL; item = g_list_next (item))
		{
			PropertyValue *value = (PropertyValue *)item->data;
			AnjutaProjectPropertyInfo *info;

			info = anjuta_project_property_get_info (value->property);
			
		}*/
		
		g_list_foreach (modified, (GFunc)pm_property_value_free, NULL);
	}
	g_list_foreach (table->properties, (GFunc)pm_property_entry_free, NULL);
	g_free (table);
	gtk_widget_destroy (dialog);
}

/* Properties dialog
 *---------------------------------------------------------------------------*/

GtkWidget *
pm_project_create_properties_dialog (AnjutaPmProject *project, GtkWindow *parent, AnjutaProjectNode *node)
{
	const char *title;
	PropertiesTable *table;
	GtkWidget *dialog;

	g_return_val_if_fail (node != NULL, FALSE);
	
	switch (anjuta_project_node_get_node_type (node))
	{
	case ANJUTA_PROJECT_ROOT:
		title = _("Project properties");
		break;
	case ANJUTA_PROJECT_GROUP:
		title = _("Group properties");
		break;
	case ANJUTA_PROJECT_TARGET:
		title = _("Target properties");
		break;
	case ANJUTA_PROJECT_SOURCE:
		title = _("Source properties");
		break;
	default:
		return FALSE;
	}

	table = create_properties_table (project->project, node);

	if (table != NULL)
	{
		table->project = project;
		dialog = gtk_dialog_new_with_buttons (title,
							   parent,
							   GTK_DIALOG_DESTROY_WITH_PARENT,
							   GTK_STOCK_CANCEL,
							   GTK_RESPONSE_CANCEL,
							   GTK_STOCK_APPLY,
							   GTK_RESPONSE_APPLY, NULL);

		g_signal_connect (dialog, "response",
						G_CALLBACK (on_properties_dialog_response),
						table);

		gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG(dialog))),
				table->table);
		gtk_window_set_default_size (GTK_WINDOW (dialog), 450, -1);
		gtk_widget_show (dialog);
	}

	return dialog;
}

/* Group dialog
 *---------------------------------------------------------------------------*/

AnjutaProjectNode*
anjuta_pm_project_new_group (AnjutaPmProject *project, GtkWindow *parent, GtkTreeIter *selected, const gchar *default_name)
{
	GtkBuilder *gui;
	GtkWidget *dialog, *group_name_entry, *ok_button;
	GtkWidget *groups_view;
	gint response;
	gboolean finished = FALSE;
	AnjutaProjectNode *new_group = NULL;

	g_return_val_if_fail (project != NULL, NULL);

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

	setup_nodes_treeview (anjuta_pm_project_get_model (project),
							groups_view,
							parent_filter_func,
							GINT_TO_POINTER (ANJUTA_PROJECT_GROUP),
							selected);
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
					new_group = anjuta_pm_project_add_group (project, group, NULL, name, &err);
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
anjuta_pm_project_new_source (AnjutaPmProject *project,
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
		anjuta_pm_project_new_multiple_source (project, parent,
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
anjuta_pm_project_new_multiple_source (AnjutaPmProject *project,
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

	g_return_val_if_fail (project != NULL, NULL);

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

	setup_nodes_treeview (anjuta_pm_project_get_model (project),
							targets_view,
							parent_filter_func,
							GINT_TO_POINTER (ANJUTA_PROJECT_SOURCE),
							default_parent);
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

					new_source = anjuta_pm_project_add_source (project,
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
						"%s", _("The selected node cannot contains source files."));
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
build_types_store (AnjutaPmProject *project)
{
    GtkListStore *store;
    GtkTreeIter iter;
    GList *types;
    GList *node;

    types = anjuta_pm_project_get_node_info (project);
    store = gtk_list_store_new (TARGET_TYPE_N_COLUMNS,
                                G_TYPE_POINTER,
                                G_TYPE_STRING,
                                GDK_TYPE_PIXBUF);
    
    for (node = g_list_first (types); node != NULL; node = g_list_next (node)) {
        GdkPixbuf *pixbuf;
        const gchar *name;
        AnjutaProjectNodeType type;

        type = anjuta_project_node_info_type ((AnjutaProjectNodeInfo *)node->data);
        if (type & ANJUTA_PROJECT_TARGET)
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

    g_list_free (types);

    return store;
}

AnjutaProjectNode* 
anjuta_pm_project_new_target (AnjutaPmProject *project,
                             GtkWindow       *parent,
                             GtkTreeIter     *default_group,
                             const gchar     *default_target_name_to_add)
{
	GtkBuilder *gui;
	GtkWidget *dialog, *target_name_entry, *ok_button;
	GtkWidget *target_type_combo, *groups_view;
	GtkListStore *types_store;
	GtkCellRenderer *renderer;
	gint response;
	gboolean finished = FALSE;
	AnjutaProjectNode *new_target = NULL;

	g_return_val_if_fail (project != NULL, NULL);

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

	setup_nodes_treeview (anjuta_pm_project_get_model (project),
							groups_view,
							parent_filter_func,
							GINT_TO_POINTER (ANJUTA_PROJECT_TARGET),
							default_group);
	gtk_widget_show (groups_view);

	/* setup target types combo box */
	types_store = build_types_store (project);
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
					new_target = anjuta_pm_project_add_target (project, group, NULL, name, type, &err);
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
anjuta_pm_project_new_module (AnjutaPmProject *project,
                             GtkWindow          *parent,
                             GtkTreeIter        *default_target,
                             const gchar        *default_module)
{
	GtkBuilder *gui;
	GtkWidget *dialog;
	GtkWidget *ok_button, *new_button;
	GtkWidget *targets_view;
	GtkWidget *modules_view;
	gint response;
	gboolean finished = FALSE;
	GList* new_modules = NULL;
	GtkTreeSelection *module_selection;

	g_return_val_if_fail (project != NULL, NULL);

	gui = load_interface ("add_module_dialog");
	g_return_val_if_fail (gui != NULL, NULL);

	/* get all needed widgets */
	dialog = GTK_WIDGET (gtk_builder_get_object (gui, "add_module_dialog"));
	targets_view = GTK_WIDGET (gtk_builder_get_object (gui, "module_targets_view"));
	modules_view = GTK_WIDGET (gtk_builder_get_object (gui, "modules_view"));
	new_button = GTK_WIDGET (gtk_builder_get_object (gui, "new_package_button"));
	ok_button = GTK_WIDGET (gtk_builder_get_object (gui, "ok_module_button"));

	setup_nodes_treeview (anjuta_pm_project_get_model (project),
							targets_view,
							parent_filter_func,
							GINT_TO_POINTER (ANJUTA_PROJECT_MODULE),
							default_target);
	gtk_widget_show (targets_view);
	setup_nodes_treeview (anjuta_pm_project_get_model (project),
							modules_view,
							module_filter_func,
							NULL,
							NULL);
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
				anjuta_pm_project_new_package (project, parent, NULL, NULL);

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
						GError *err = NULL;
						AnjutaProjectNode* selected_module = (AnjutaProjectNode *)node->data;
						AnjutaProjectNode* new_module;
						gchar* uri = NULL;
						GFile* module_file;

						/*module_file = gbf_tree_data_get_file (selected_module);
						new_module = ianjuta_project_add_module (project,
															target,
															module_file,
															&err);
						g_object_unref (module_file);*/
						new_module = NULL;
						if (err) {
							gchar *str = g_strdup_printf ("%s: %s\n",
															uri,
															err->message);
							g_string_append (err_mesg, str);
							g_error_free (err);
							g_free (str);
						}
						else
							new_modules = g_list_append (new_modules, new_module);

						g_free (uri);
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
    GtkTreeSelection *selection = gtk_tree_view_get_selection(view);

    if (gtk_tree_selection_count_selected_rows (selection) == 1)
    {
        GtkTreeModel *model;
        GList *list;
        GtkTreeIter iter;

        list = gtk_tree_selection_get_selected_rows (selection, &model);
        if (gtk_tree_model_get_iter (model, &iter, (GtkTreePath *)(list->data)))
        {
            gchar *name;
            gchar *ptr;
            
            gtk_tree_model_get (model, &iter, COL_PKG_PACKAGE, &name, -1);

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
            g_free (name);
        }
        
        g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
        g_list_free (list);
    }
}

static void
on_changed_disconnect (GtkEditable* entry, gpointer data)
{
    g_signal_handlers_block_by_func (G_OBJECT (data), on_cursor_changed_set_entry, entry);
}

GList* 
anjuta_pm_project_new_package (AnjutaPmProject *project,
                              GtkWindow        *parent,
                              GtkTreeIter      *default_module,
                              GList            *packages_to_add)
{
    GtkBuilder *gui;
    GtkWidget *dialog;
    GtkWidget *ok_button;
    GtkWidget *module_entry;
    GtkWidget *packages_view;
    GtkCellRenderer* renderer;
    GtkListStore *store;
    GList *packages = NULL;
    GtkTreeViewColumn *col;
    gint response;
    gboolean finished = FALSE;
    GtkTreeSelection *package_selection;
    GtkTreeIter root;
    gboolean valid;
    gint default_pos = -1;
    GbfProjectModel *model;
    
    g_return_val_if_fail (project != NULL, NULL);
    
    gui = load_interface ("add_package_dialog");
    g_return_val_if_fail (gui != NULL, NULL);
    
    /* get all needed widgets */
    dialog = GTK_WIDGET (gtk_builder_get_object (gui, "add_package_dialog"));
    module_entry = GTK_WIDGET (gtk_builder_get_object (gui, "module_entry"));
    packages_view = GTK_WIDGET (gtk_builder_get_object (gui, "packages_view"));
    ok_button = GTK_WIDGET (gtk_builder_get_object (gui, "ok_package_button"));

    /* Fill combo box with modules */
    store = gtk_list_store_new(1, G_TYPE_STRING);
    gtk_combo_box_entry_set_text_column (GTK_COMBO_BOX_ENTRY (module_entry), 0);

    model = anjuta_pm_project_get_model(project);
    for (valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model), &root); valid != FALSE; valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (model), &root))
    {
        GbfTreeData *data;
        
        gtk_tree_model_get (GTK_TREE_MODEL (model), &root, GBF_PROJECT_MODEL_COLUMN_DATA, &data, -1);
        if (data && (data->type == GBF_TREE_NODE_GROUP)) break;
    }
    
    if (valid)
    {
        GtkTreeIter iter;
        gint pos = 0;
        GbfTreeData *data;
        GtkTreePath *default_path = default_module != NULL ? gtk_tree_model_get_path(GTK_TREE_MODEL (model), default_module) : NULL;

        gtk_tree_model_get (GTK_TREE_MODEL (model), &root, GBF_PROJECT_MODEL_COLUMN_DATA, &data, -1);
        
        for (valid = gtk_tree_model_iter_children (GTK_TREE_MODEL (model), &iter, &root); valid != FALSE; valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (model), &iter))
        {
            GbfTreeData *data;
            
            gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, GBF_PROJECT_MODEL_COLUMN_DATA, &data, -1);

            if (data && (data->type == GBF_TREE_NODE_MODULE))
            {
                GtkTreeIter list_iter;
                GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL (model), &iter);

                gtk_list_store_append (store, &list_iter);
                gtk_list_store_set (store, &list_iter, 0, data->name, -1);
                if ((default_path != NULL) && (gtk_tree_path_compare (path, default_path) == 0))
                {
                    default_pos = pos;
                }
                gtk_tree_path_free (path);
                pos++;
            }
        }
        if (default_path != NULL) gtk_tree_path_free (default_path);
    }
    gtk_combo_box_set_model (GTK_COMBO_BOX(module_entry), GTK_TREE_MODEL(store));
    gtk_combo_box_entry_set_text_column (GTK_COMBO_BOX_ENTRY (module_entry), 0);
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
    renderer = gtk_cell_renderer_text_new ();
    col = gtk_tree_view_column_new_with_attributes (_("Package"),
                                                    renderer,
                                                    "text", COL_PKG_PACKAGE,
                                                    NULL);
    gtk_tree_view_column_set_sort_column_id (col, COL_PKG_PACKAGE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (packages_view), col);
    renderer = gtk_cell_renderer_text_new ();
    col = gtk_tree_view_column_new_with_attributes (_("Description"),
                                                    renderer,
                                                    "text",
                                                    COL_PKG_DESCRIPTION,
                                                    NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (packages_view), col);
    store = packages_get_pkgconfig_list ();
    gtk_tree_view_set_model (GTK_TREE_VIEW (packages_view),
                            GTK_TREE_MODEL (store));

    on_cursor_changed (GTK_TREE_VIEW (packages_view), ok_button);
    g_signal_connect (G_OBJECT(packages_view), "cursor-changed",
        G_CALLBACK(on_cursor_changed), ok_button);
    package_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (packages_view));
    gtk_tree_selection_set_mode (package_selection, GTK_SELECTION_MULTIPLE);

    if (parent) {
        gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);
    }
    
    /* execute dialog */
    while (!finished) {
        response = gtk_dialog_run (GTK_DIALOG (dialog));

        switch (response) {
            case GTK_RESPONSE_OK: 
            {
                gchar *module;

                module = gtk_combo_box_get_active_text (GTK_COMBO_BOX (module_entry));
                if (module)
                {
                    GString *err_mesg = g_string_new (NULL);
                    GList *list;
                    GList *node;
                    GtkTreeModel *model;
                    GtkTreeIter iter;
                    
                    list = gtk_tree_selection_get_selected_rows (package_selection, &model);
                    for (node = list; node != NULL; node = g_list_next (node))
                    {
                        if (gtk_tree_model_get_iter (model, &iter, (GtkTreePath *)(node->data)))
                        {
                            gchar *name;
                            AnjutaProjectNode* new_package;
                            GError *err = NULL;
                            
                            gtk_tree_model_get (model, &iter, COL_PKG_PACKAGE, &name, -1);

                            /*new_package = ianjuta_project_add_package (project,
                                                            module,
                                                            name,
                                                            &err);*/
                            new_package = NULL;
                            if (err)
                            {
                                gchar *str = g_strdup_printf ("%s: %s\n",
                                                                name,
                                                                err->message);
                                g_string_append (err_mesg, str);
                                g_error_free (err);
                                g_free (str);
                            }
                            else
                            {
                                packages = g_list_append (packages,
                                                                new_package);
                            }
                            g_free (name);
                        }
                    }
                    g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
                    g_list_free (list);
                    
                    if (err_mesg->str && strlen (err_mesg->str) > 0)
                    {
                        error_dialog (parent, _("Cannot add packages"),
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
                    error_dialog (parent, _("Cannot add packages"),
                            "%s", _("No module has been selected"));
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
    
    return packages;
}
