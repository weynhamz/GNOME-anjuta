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

#define GLADE_FILE  PACKAGE_DATA_DIR "/glade/am-project.ui"

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

static AnjutaProjectNodeInfo *
project_get_type_info (IAnjutaProject *project, AnjutaProjectNodeType type)
{
	GList *item;
	AnjutaProjectNodeInfo *info = NULL;

	g_message ("get node info list %p", ianjuta_project_get_node_info (project, NULL));
	for (item = ianjuta_project_get_node_info (project, NULL); item != NULL; item = g_list_next (item))
	{
		info = (AnjutaProjectNodeInfo *)item->data;

		g_message ("check node name %s type %x look for %x", info->name, info->type, type);
		if (info->type == type) break;
	}

	return info;
}

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

/* Public functions
 *---------------------------------------------------------------------------*/

GtkWidget *
pm_configure_project_dialog (IAnjutaProject *project, AnjutaProjectNode *node, GError **error)
{
	GtkBuilder *bxml = gtk_builder_new ();
	ConfigureProjectDialog *dlg;
	GtkWidget *table;
	gint pos;
	gchar *name;
	AnjutaProjectProperty *prop;

	bxml = anjuta_util_builder_new (GLADE_FILE, NULL);
	if (!bxml) return NULL;

	dlg = g_new0 (ConfigureProjectDialog, 1);
	anjuta_util_builder_get_objects (bxml,
	    							"general_properties_table", &table,
	                                NULL);
	dlg->top_level = table;
	g_object_ref (table);
	g_signal_connect (table, "destroy", G_CALLBACK (on_project_widget_destroy), dlg);

	pos = 0;
	name = g_file_get_parse_name (anjuta_project_node_get_file (node));
	add_label (_("Path:"), name, table, &pos);
	g_free (name);

	for (prop = anjuta_project_node_first_valid_property (node); prop != NULL; prop = anjuta_project_property_next (prop))
	{
		add_entry (project, NULL, prop, table, &pos);
	}
	
	/*add_entry (project, NULL, _("Name:"), AMP_PROPERTY_NAME, table, 1);
	add_entry (project, NULL, _("Version:"), AMP_PROPERTY_VERSION, table, 2);
	add_entry (project, NULL, _("Bug report URL:"), AMP_PROPERTY_BUG_REPORT, table, 3);
	add_entry (project, NULL, _("Package name:"), AMP_PROPERTY_TARNAME, table, 4);
	add_entry (project, NULL, _("URL:"), AMP_PROPERTY_URL, table, 5);*/
	
	gtk_widget_show_all (table);
	g_object_unref (bxml);

	return table;
}

GtkWidget *
pm_configure_group_dialog (IAnjutaProject *project, AnjutaProjectNode *group, GError **error)
{
	GtkBuilder *bxml = gtk_builder_new ();
	GtkWidget *properties;
	GtkWidget *main_table;
	gint main_pos;
	GtkWidget *extra_table;
	gint extra_pos;
	ConfigureProjectDialog *dlg;
	gchar *name;
	AnjutaProjectProperty *prop;

	bxml = anjuta_util_builder_new (GLADE_FILE, NULL);
	if (!bxml) return NULL;

	dlg = g_new0 (ConfigureProjectDialog, 1);
	anjuta_util_builder_get_objects (bxml,
	                                "properties", &properties,
	    							"main_table", &main_table,
	    							"extra_table", &extra_table,
	                                NULL);
	dlg->top_level = properties;
	g_object_ref (properties);
	g_signal_connect (properties, "destroy", G_CALLBACK (on_project_widget_destroy), dlg);

	main_pos = 0;
	extra_pos = 0;
	name = g_file_get_parse_name (anjuta_project_node_get_file (group));
	add_label (_("Name:"), name, main_table, &main_pos);
	g_free (name);

	for (prop = anjuta_project_node_first_valid_property (group); prop != NULL; prop = anjuta_project_property_next (prop))
	{
		AnjutaProjectProperty *item;

		item = anjuta_project_node_get_property (group, prop);
		if (item != NULL)
		{
			add_entry (project, group, item, main_table, &main_pos);
		}
		else
		{
			add_entry (project, group, prop, extra_table, &extra_pos);
		}
	}
	
	gtk_widget_show_all (properties);
	g_object_unref (bxml);
	
	return properties;
}

GtkWidget *
pm_configure_target_dialog (IAnjutaProject *project, AnjutaProjectNode *target, GError **error)
{
	GtkBuilder *bxml = gtk_builder_new ();
	GtkWidget *properties;
	GtkWidget *main_table;
	gint main_pos;
	GtkWidget *extra_table;
	gint extra_pos;
	ConfigureProjectDialog *dlg;
	AnjutaProjectNodeInfo* info;
	const gchar *name;
	AnjutaProjectProperty *prop;

	bxml = anjuta_util_builder_new (GLADE_FILE, NULL);
	if (!bxml) return NULL;

	dlg = g_new0 (ConfigureProjectDialog, 1);
	anjuta_util_builder_get_objects (bxml,
	                                "properties", &properties,
	    							"main_table", &main_table,
	    							"extra_table", &extra_table,
	                                NULL);
	dlg->top_level = properties;
	g_object_ref (properties);
	g_signal_connect (properties, "destroy", G_CALLBACK (on_project_widget_destroy), dlg);

	main_pos = 0;
	extra_pos = 0;
	name = anjuta_project_node_get_name (target);
	add_label (_("Name:"), name, main_table, &main_pos);
	info = project_get_type_info (project, anjuta_project_node_get_type (target));;
	add_label (_("Type:"), anjuta_project_node_info_name (info), main_table, &main_pos);

	for (prop = anjuta_project_node_first_valid_property (target); prop != NULL; prop = anjuta_project_property_next (prop))
	{
		AnjutaProjectProperty *item;

		item = anjuta_project_node_get_property (target, prop);
		if (item != NULL)
		{
			add_entry (project, target, item, main_table, &main_pos);
		}
		else
		{
			add_entry (project, target, prop, extra_table, &extra_pos);
		}
	}
	
	gtk_widget_show_all (properties);
	g_object_unref (bxml);
	
	return properties;
}

GtkWidget *
pm_configure_source_dialog (IAnjutaProject *project, AnjutaProjectNode *source, GError **error)
{
	return NULL;
}
