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

#define GLADE_FILE  PACKAGE_DATA_DIR "/glade/create_dialogs.ui"

/* Types
  *---------------------------------------------------------------------------*/

typedef struct _ConfigureProjectDialog
{
	IAnjutaProject *project;

	GtkWidget *top_level;
} ConfigureProjectDialog;

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

static gboolean
nodes_filter_fn (GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
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
		else if (anjuta_project_node_get_type (node) == type)
		{
			/* Check if node can be used as sibling */
			parent = anjuta_project_node_parent (node);
			visible = anjuta_project_node_get_state (parent) & need ? TRUE : FALSE;
			
		}
	}

	return visible;
}

static void 
setup_nodes_treeview (GbfProjectModel         *model,
                       GtkWidget              *view,
                       AnjutaProjectNodeType  type,
                       GtkTreeIter            *selected)
{
	GtkTreeModel *filter;
	GtkTreeIter iter_filter;
	GtkTreePath *path;

	g_return_if_fail (model != NULL);
	g_return_if_fail (view != NULL && GBF_IS_PROJECT_VIEW (view));

	filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (model), NULL);
	gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (filter),
											nodes_filter_fn,  GINT_TO_POINTER (type), NULL);
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

static void
on_project_widget_destroy (GtkWidget *wid, ConfigureProjectDialog *dlg)
{
	g_object_unref (dlg->top_level);
	g_free (dlg);
}

static void
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
		return;
	}		
	gtk_widget_show (entry);
	gtk_table_attach (GTK_TABLE (table), entry, 1, 2, *position, *position+1,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 3);
	
	*position = *position + 1;
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

static GtkWidget *
create_properties_table (IAnjutaProject *project, AnjutaProjectNode *node)
{
	GtkBuilder *bxml;
	ConfigureProjectDialog *dlg;
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

	dlg = g_new0 (ConfigureProjectDialog, 1);
	anjuta_util_builder_get_objects (bxml,
									"properties", &properties,
									"main_table", &main_table,
									"extra_table", &extra_table,
									"extra_expand", &extra_expand,
									NULL);
	dlg->top_level = properties;
	g_object_ref (properties);
	g_signal_connect (properties, "destroy", G_CALLBACK (on_project_widget_destroy), dlg);

	main_pos = 0;
	extra_pos = 0;

	/* Always display node name */
	path = g_file_get_path (anjuta_project_node_get_file (node));
	add_label (_("Full Name:"), path, main_table, &main_pos);
	g_free (path);

	/* Display node type only if several types are possible */
	node_info = NULL;
	single = TRUE;
	type = anjuta_project_node_get_type (node);
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

		prop = anjuta_project_node_get_property (node, valid_prop);
		if (prop != NULL)
		{
			/* This property has been set, display it in the main part */
			add_entry (project, node, prop, main_table, &main_pos);
		}
		else
		{
			/* This property has not been set, hide it by default */
			add_entry (project, node, valid_prop, extra_table, &extra_pos);
			single = TRUE;
		}
	}

	gtk_widget_show_all (properties);
	
	/* Hide expander if it is empty */
	if (!single) gtk_widget_hide (extra_expand);
	
	g_object_unref (bxml);
	
	return properties;
}

static void
on_properties_dialog_response (GtkDialog *win,
							   gint id,
							   GtkWidget **dialog)
{
	gtk_widget_destroy (*dialog);
	*dialog = NULL;
}

/* Public functions
 *---------------------------------------------------------------------------*/

gboolean
pm_project_create_properties_dialog (IAnjutaProject *project, GtkWidget **dialog, GtkWindow *parent, AnjutaProjectNode *node)
{
	const char *title;
	GtkWidget *properties;

	g_return_val_if_fail (node != NULL, FALSE);
	g_return_val_if_fail (*dialog == NULL, FALSE);
	
	switch (anjuta_project_node_get_type (node))
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

	properties = create_properties_table (project, node);

	if (properties)
	{
		*dialog = gtk_dialog_new_with_buttons (title,
							   parent,
							   GTK_DIALOG_DESTROY_WITH_PARENT,
							   GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL, NULL);

		g_signal_connect (*dialog, "response",
						G_CALLBACK (on_properties_dialog_response),
						dialog);

		gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG(*dialog))),
				properties);
		gtk_window_set_default_size (GTK_WINDOW (*dialog), 450, -1);
		gtk_widget_show (*dialog);
	}

	return *dialog != NULL;
}

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
    
    setup_nodes_treeview (anjuta_pm_project_get_model (project), groups_view, ANJUTA_PROJECT_GROUP, selected);
    gtk_widget_show (groups_view);
    
    if (parent) {
        gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);
    }
    
    /* execute dialog */
    while (!finished) {
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
                if (group) {
                    new_group = anjuta_pm_project_add_group (project, group, NULL, name, &err);
                    if (err) {
                        error_dialog (parent, _("Cannot add group"), "%s",
                                      err->message);
                        g_error_free (err);
                    } else {
			finished = TRUE;
		    }
                } else {
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

	setup_nodes_treeview (anjuta_pm_project_get_model (project), targets_view, ANJUTA_PROJECT_SOURCE, default_parent);
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
