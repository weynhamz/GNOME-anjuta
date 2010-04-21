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


/* Helper functions
 *---------------------------------------------------------------------------*/

/* Private functions
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

