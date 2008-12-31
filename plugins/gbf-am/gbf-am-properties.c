/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8; coding: utf-8 -*- */
/* gbf-am-properties.c
 *
 * Copyright (C) 2005  Naba Kumar
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
 * Author: Naba Kumar
 */

#include <config.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <glade/glade-xml.h>

#include <glib/gi18n.h>

#include "gbf-am-config.h"
#include "gbf-am-properties.h"

#define GLADE_FILE  PACKAGE_DATA_DIR "/glade/gbf-am-dialogs.glade"

typedef enum {
	GBF_AM_CONFIG_LABEL,
	GBF_AM_CONFIG_ENTRY,
	GBF_AM_CONFIG_TEXT,
	GBF_AM_CONFIG_LIST,
} GbfConfigPropertyType;

enum {
	COL_PACKAGE,
	COL_VERSION,
	N_COLUMNS
};

enum {
	COL_PKG_PACKAGE,
	COL_PKG_DESCRIPTION,
	N_PKG_COLUMNS
};

enum {
	COL_VAR_NAME,
	COL_VAR_VALUE,
	N_VAR_COLUMNS
};

static void
on_property_entry_changed (GtkEntry *entry, GbfAmConfigValue *value)
{
	gbf_am_config_value_set_string (value, gtk_entry_get_text (entry));
}

static void
add_configure_property (GbfAmProject *project, GbfAmConfigMapping *config,
			GbfConfigPropertyType prop_type,
			const gchar *display_name, const gchar *direct_value,
			const gchar *config_key, GtkWidget *table,
			gint position)
{
	GtkWidget *label;
	const gchar *value;
	GtkWidget *widget;
	GbfAmConfigValue *config_value = NULL;
	
	value = "";
	if (direct_value)
	{
		value = direct_value;
	} else {
		config_value = gbf_am_config_mapping_lookup (config,
							     config_key);
		if (!config_value) {
			config_value = gbf_am_config_value_new (GBF_AM_TYPE_STRING);
			gbf_am_config_value_set_string (config_value, "");
			gbf_am_config_mapping_insert (config, config_key,
						      config_value);
		}
		if (config_value && config_value->type == GBF_AM_TYPE_STRING) {
			const gchar *val_str;
			val_str = gbf_am_config_value_get_string (config_value);
			if (val_str)
				value = val_str;
		}
	}
	
	label = gtk_label_new (display_name);
	gtk_misc_set_alignment (GTK_MISC (label), 0, -1);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, position, position+1,
			  GTK_FILL, GTK_FILL, 5, 3);
	switch (prop_type) {
		case GBF_AM_CONFIG_LABEL:
			widget = gtk_label_new (value);
			gtk_misc_set_alignment (GTK_MISC (widget), 0, -1);
			break;
		case GBF_AM_CONFIG_ENTRY:
			widget = gtk_entry_new ();
			gtk_entry_set_text (GTK_ENTRY (widget), value);
			if (config_value) {
				g_signal_connect (widget, "changed",
						  G_CALLBACK (on_property_entry_changed),
						  config_value);
			}
			break;
		default:
			g_warning ("Should not reach here");
			widget = gtk_label_new (_("Unknown"));
			gtk_misc_set_alignment (GTK_MISC (widget), 0, -1);
	}
	gtk_widget_show (widget);
	gtk_table_attach (GTK_TABLE (table), widget, 1, 2, position, position+1,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 3);
}

static void
recursive_config_foreach_cb (const gchar *key, GbfAmConfigValue *value,
			     gpointer user_data)
{
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *widget;
	gint position;
	
	table = GTK_WIDGET (user_data);
	position = g_list_length (GTK_TABLE (table)->children);
	
	label = gtk_label_new (key);
	gtk_misc_set_alignment (GTK_MISC (label), 0, -1);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1,
			  position, position+1,
			  GTK_FILL, GTK_FILL, 5, 3);
		
	if (value->type == GBF_AM_TYPE_STRING) {
		widget = gtk_entry_new ();
		gtk_entry_set_text (GTK_ENTRY (widget),
				    gbf_am_config_value_get_string (value));
		g_signal_connect (widget, "changed",
				  G_CALLBACK (on_property_entry_changed),
				  value);
	} else if (value->type == GBF_AM_TYPE_LIST) {
		/* FIXME: */
		widget = gtk_label_new ("FIXME");
		gtk_misc_set_alignment (GTK_MISC (widget), 0, -1);
	} else if (value->type == GBF_AM_TYPE_MAPPING) {
		/* FIXME: */
		widget = gtk_label_new ("FIXME");
		gtk_misc_set_alignment (GTK_MISC (widget), 0, -1);
	} else {
		g_warning ("Should not be here");
		widget = gtk_label_new (_("Unknown"));
		gtk_misc_set_alignment (GTK_MISC (widget), 0, -1);
	}
	gtk_widget_show (widget);
	gtk_table_attach (GTK_TABLE (table), widget, 1, 2,
			  position, position+1,
			  GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 3);
}

static GtkListStore *
packages_get_pkgconfig_list (void)
{
	GtkListStore *store;
	GtkTreeIter iter;
	gchar line[1024];
	gchar *tmpfile, *pkg_cmd;
	FILE *pkg_fd;
	
	store = gtk_list_store_new (N_PKG_COLUMNS, G_TYPE_STRING,
				    G_TYPE_STRING);
	
	/* Now get all the pkg-config info */
	tmpfile = g_strdup_printf ("%s%cpkgmodules--%d", g_get_tmp_dir (),
				   G_DIR_SEPARATOR, getpid());
	pkg_cmd = g_strconcat ("pkg-config --list-all 2>/dev/null | sort > ",
			       tmpfile, NULL);
	if (system (pkg_cmd) == -1)
		return store;
	pkg_fd = fopen (tmpfile, "r");
	if (!pkg_fd) {
		g_warning ("Can not open %s for reading", tmpfile);
		g_free (tmpfile);
		return store;
	}
	while (fgets (line, 1024, pkg_fd)) {
		gchar *name_end;
		gchar *desc_start;
		gchar *description;
		gchar *name;
		
		if (line[0] == '\0')
			continue;
		
		name_end = line;
		while (!isspace(*name_end))
			name_end++;
		desc_start = name_end;
		while (isspace(*desc_start))
			desc_start++;
		
		name = g_strndup (line, name_end-line);
		description = g_strndup (desc_start, strlen (desc_start)-1);
		
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    COL_PKG_PACKAGE, name,
				    COL_PKG_DESCRIPTION, description,
				    -1);
	}
	fclose (pkg_fd);
	unlink (tmpfile);
	g_free (tmpfile);
	return store;
}

static void
save_packages_list (GbfAmProject *project, GbfAmConfigMapping *config,
		    GtkTreeModel *model, GtkTreeIter *parent)
{
	gchar *key_name;
	gchar *module_name;
	GbfAmConfigValue *value;
	GtkTreeIter child;
	gboolean has_children;
	GString *packages_list = g_string_new (NULL);
	
	gtk_tree_model_get (model, parent, COL_PACKAGE, &module_name, -1);
	has_children = gtk_tree_model_iter_children (model, &child, parent);
	if (has_children)
	{
	    do
	    {
		    gchar *package, *version;
		    gtk_tree_model_get (model, &child, COL_PACKAGE,
					&package, COL_VERSION,
					&version, -1);
		    if (strlen (packages_list->str) > 0)
			g_string_append (packages_list, ", ");
		    g_string_append (packages_list, package);
		    if (version)
		    {
			    g_string_append (packages_list, " ");
			    g_string_append (packages_list, version);
		    }
		    g_free (package);
		    g_free (version);
	    }
	    while (gtk_tree_model_iter_next (model, &child));
	}
	
	if (strlen (packages_list->str) > 0)
	{
		GbfAmConfigMapping *pkgmodule;
		key_name = g_strconcat ("pkg_check_modules_",
					module_name,
					NULL);
	
		value = gbf_am_config_mapping_lookup (config, key_name);
		if (!value)
		{
			pkgmodule = gbf_am_config_mapping_new ();
			value = gbf_am_config_value_new (GBF_AM_TYPE_MAPPING);
			gbf_am_config_value_set_mapping (value, pkgmodule);
			gbf_am_config_mapping_insert (config,
						      key_name,
						      value);
		}
		pkgmodule = gbf_am_config_value_get_mapping (value);
		value = gbf_am_config_mapping_lookup (pkgmodule,
						      "packages");
		if (!value)
		{
			value = gbf_am_config_value_new (GBF_AM_TYPE_STRING);
			gbf_am_config_value_set_string (value, packages_list->str);
			gbf_am_config_mapping_insert (pkgmodule,
						      "packages",
						      value);
		}
		else
		{
			gbf_am_config_value_set_string (value, packages_list->str);
		}
		g_free (key_name);
	}
	g_free (module_name);
}

static void
package_edited_cb (GtkCellRendererText *cellrenderertext, gchar *arg1,
		   gchar *arg2, GtkWidget *top_level)
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkTreeIter iter, parent;
	GtkTreeModel *model;
	GbfAmProject *project;
	GbfAmConfigMapping *config;
	gboolean has_parent;

	if (strcmp (arg1, arg2) == 0)
		return;
	project = g_object_get_data (G_OBJECT (top_level), "__project");
	config = g_object_get_data (G_OBJECT (top_level), "__config");
	treeview = g_object_get_data (G_OBJECT (project),
				      "__packages_treeview");
	selection = gtk_tree_view_get_selection (treeview);
	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return;
	
	/* If the selected node is module name and the module has children,
	 * do not alow editing it
	 */
	has_parent = gtk_tree_model_iter_parent (model, &parent, &iter);
	if (!has_parent &&
	    gtk_tree_model_iter_n_children (model, &iter) > 0)
		return;
	if (strcmp (arg2, _("Enter new module")) == 0 ||
	    strcmp (arg2, "") == 0)
		gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);
	else
		gtk_tree_store_set (GTK_TREE_STORE (model), &iter, COL_PACKAGE,
				    arg2, -1);
	if (has_parent)
	    save_packages_list (project, config, model, &parent);
}

static void
package_version_edited_cb (GtkCellRendererText *cellrenderertext, gchar *arg1,
			   gchar *arg2, GtkWidget *top_level)
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkTreeIter iter, parent;
	GtkTreeModel *model;
	GbfAmProject *project;
	GbfAmConfigMapping *config;
	
	if (strcmp (arg1, arg2) == 0)
		return;
	project = g_object_get_data (G_OBJECT (top_level), "__project");
	config = g_object_get_data (G_OBJECT (top_level), "__config");
	
	treeview = g_object_get_data (G_OBJECT (project),
				      "__packages_treeview");
	selection = gtk_tree_view_get_selection (treeview);
	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return;
	if (!gtk_tree_model_iter_parent (model, &parent, &iter))
		return;
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter, COL_VERSION,
			    arg2, -1);
	save_packages_list (project, config, model, &parent);
}

static void
add_package_module_clicked_cb (GtkWidget *button, GbfAmProject *project)
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreePath *path;
	
	treeview = g_object_get_data (G_OBJECT (project),
				      "__packages_treeview");
	model = gtk_tree_view_get_model (treeview);
	gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter, COL_PACKAGE,
			    _("Enter new module"), -1);
	selection = gtk_tree_view_get_selection (treeview);
	gtk_tree_selection_select_iter (selection, &iter);
	
	path = gtk_tree_model_get_path (model, &iter);
	gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW(treeview), path,
				      NULL, FALSE, 0, 0);
	gtk_tree_view_set_cursor (GTK_TREE_VIEW(treeview), path,
				  gtk_tree_view_get_column(GTK_TREE_VIEW(treeview),
							   COL_PACKAGE), TRUE);
	gtk_tree_path_free (path);
}

static void
add_package_clicked_cb (GtkWidget *button, GbfAmProject *project)
{
	GladeXML *gxml;
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkTreeIter iter, parent;
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkWidget *dlg;
	GtkWidget *pkg_treeview;
	GtkListStore *store;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *col;
	gchar *pkg_to_add = NULL;
	GbfAmConfigMapping *config;
	
	/* Let user select a package */
	gxml = glade_xml_new (GLADE_FILE, "package_selection_dialog",
			      GETTEXT_PACKAGE);
	dlg = glade_xml_get_widget (gxml, "package_selection_dialog");
	pkg_treeview = glade_xml_get_widget (gxml, "pkg_treeview");
	renderer = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes (_("Module/Packages"),
							renderer,
							"text", COL_PKG_PACKAGE,
							NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (pkg_treeview), col);
	renderer = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes (_("Version"),
							renderer,
							"text",
							COL_PKG_DESCRIPTION,
							NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (pkg_treeview), col);
	
	store = packages_get_pkgconfig_list ();
	gtk_tree_view_set_model (GTK_TREE_VIEW (pkg_treeview),
				 GTK_TREE_MODEL (store));
	if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_ACCEPT)
	{
		GtkTreeSelection *sel;
		GtkTreeIter pkg_iter;
		
		sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (pkg_treeview));
		if (gtk_tree_selection_get_selected (sel, NULL, &pkg_iter))
		{
			gtk_tree_model_get (GTK_TREE_MODEL (store),
					    &pkg_iter, COL_PKG_PACKAGE,
					    &pkg_to_add, -1);
		}
	}
	gtk_widget_destroy (dlg);
	
	if (!pkg_to_add)
		return;
	
	treeview = g_object_get_data (G_OBJECT (project),
				      "__packages_treeview");
	config = g_object_get_data (G_OBJECT (project), "__config");
	
	selection = gtk_tree_view_get_selection (treeview);
	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
	{
		g_free (pkg_to_add);
		return;
	}
	if (!gtk_tree_model_iter_parent (model, &parent, &iter))
		gtk_tree_selection_get_selected (selection, &model, &parent);
	gtk_tree_store_append (GTK_TREE_STORE (model), &iter, &parent);
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
			    COL_PACKAGE, pkg_to_add, -1);
	save_packages_list (project, config, model, &parent);
	
	g_free (pkg_to_add);
	
	path = gtk_tree_model_get_path (model, &parent);
	gtk_tree_view_expand_row (GTK_TREE_VIEW (treeview), path, TRUE);
	gtk_tree_path_free (path);
	
	gtk_tree_selection_select_iter (selection, &iter);
	
	path = gtk_tree_model_get_path (model, &iter);
	gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW(treeview), path,
				      NULL, FALSE, 0, 0);
	gtk_tree_view_set_cursor (GTK_TREE_VIEW(treeview), path,
				  gtk_tree_view_get_column(GTK_TREE_VIEW(treeview),
							   COL_PACKAGE),
				  FALSE);
	gtk_tree_path_free (path);
}

static void
remove_package_clicked_cb (GtkWidget *button, GbfAmProject *project)
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkTreeIter iter, parent;
	GtkTreeModel *model;
	const gchar *msg;
	gchar *name;
	GtkWidget *dlg;
	gboolean has_parent;
	GbfAmConfigMapping *config;
	
	treeview = g_object_get_data (G_OBJECT (project),
				      "__packages_treeview");
	config = g_object_get_data (G_OBJECT (project),
				      "__config");
	selection = gtk_tree_view_get_selection (treeview);
	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return;
	gtk_tree_model_get (model, &iter, COL_PACKAGE, &name, -1);
	has_parent = gtk_tree_model_iter_parent (model, &parent, &iter);
	if (!has_parent)
		msg = _("Are you sure you want to remove module \"%s\" and all its associated packages?");
	else
		msg = _("Are you sure you want to remove package \"%s\"?");
	dlg = gtk_message_dialog_new_with_markup (NULL,
						  GTK_DIALOG_DESTROY_WITH_PARENT,
						  GTK_MESSAGE_QUESTION,
						  GTK_BUTTONS_YES_NO,
						  msg, name);
	if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_YES)
		gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);
	gtk_widget_destroy (dlg);
	g_free (name);
	if (has_parent)
		save_packages_list (project, config, model, &parent);
}

static void
packages_treeview_selection_changed_cb (GtkTreeSelection *selection,
					GbfAmProject *project)
{
	GtkTreeIter iter/*, parent*/;
	GtkTreeModel *model;
	GtkWidget *add_module_button, *add_package_button, *remove_button;
	
	add_module_button = g_object_get_data (G_OBJECT (project),
					       "__add_module_button");
	add_package_button = g_object_get_data (G_OBJECT (project),
					       "__add_package_button");
	remove_button = g_object_get_data (G_OBJECT (project),
					       "__remove_button");
	
	gtk_widget_set_sensitive (add_module_button, TRUE);
	gtk_widget_set_sensitive (add_package_button, TRUE);
	gtk_widget_set_sensitive (remove_button, TRUE);
	
	if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_widget_set_sensitive (add_package_button, FALSE);
		gtk_widget_set_sensitive (remove_button, FALSE);
	}		
}

static void
variable_name_edited_cb (GtkCellRendererText *cellrenderertext, gchar *arg1,
			 gchar *arg2, GtkWidget *top_level)
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GbfAmProject *project;
	
	project = g_object_get_data (G_OBJECT (top_level), "__project");
	treeview = g_object_get_data (G_OBJECT (project),
				      "__variables_treeview");
	selection = gtk_tree_view_get_selection (treeview);
	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return;
	g_message ("Var name = %s", arg2);
	if (strcmp (arg2, _("Enter new variable")) == 0 ||
	    strcmp (arg2, "") == 0)
	{
		gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
	}
	else if (strcmp (arg1, arg2) != 0 && strlen (arg2) != 0)
	{
		gtk_list_store_set (GTK_LIST_STORE (model), &iter, COL_VAR_NAME,
				    arg2, -1);
	}
}

static void
variable_value_edited_cb (GtkCellRendererText *cellrenderertext, gchar *arg1,
			 gchar *arg2, GtkWidget *top_level)
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GbfAmConfigMapping *config;
	GbfAmConfigMapping *variables;
	GbfAmConfigValue *value;
	GbfAmProject *project;
	gchar *variable_name;
	
	project = g_object_get_data (G_OBJECT (top_level), "__project");
	config = g_object_get_data (G_OBJECT (top_level), "__config");
	
	if (strcmp (arg1, arg2) == 0)
		return;
	treeview = g_object_get_data (G_OBJECT (project),
				      "__variables_treeview");
	selection = gtk_tree_view_get_selection (treeview);
	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return;

	gtk_list_store_set (GTK_LIST_STORE (model), &iter, COL_VAR_VALUE,
			    arg2, -1);
	gtk_tree_model_get (model, &iter, COL_VAR_NAME, &variable_name, -1);

	value = gbf_am_config_mapping_lookup (config, "variables");
	if (!value)
	{
		variables = gbf_am_config_mapping_new ();
		value = gbf_am_config_value_new (GBF_AM_TYPE_MAPPING);
		gbf_am_config_value_set_mapping (value, variables);
		gbf_am_config_mapping_insert (config, "variables", value);
					      
	}
	else
	{
		variables = gbf_am_config_value_get_mapping (value);
	}
	value = gbf_am_config_mapping_lookup (variables, variable_name);

	if (!value)
	{
		value = gbf_am_config_value_new (GBF_AM_TYPE_STRING);
		gbf_am_config_value_set_string (value, arg2);
		gbf_am_config_mapping_insert (variables, variable_name, value);
	}
	else
	{
		gbf_am_config_value_set_string (value, arg2);
	}
	g_free (variable_name);
}

static void
add_variable_clicked_cb (GtkWidget *button, GbfAmProject *project)
{
	GtkTreeView *treeview;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreePath *path;
	
	treeview = g_object_get_data (G_OBJECT (project),
				      "__variables_treeview");
	model = gtk_tree_view_get_model (treeview);
	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, COL_VAR_NAME,
			    _("Enter new variable"), -1);
	
	path = gtk_tree_model_get_path (model, &iter);
	gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW(treeview), path,
				      NULL, FALSE, 0, 0);
	gtk_tree_view_set_cursor (GTK_TREE_VIEW(treeview), path,
				  gtk_tree_view_get_column(GTK_TREE_VIEW(treeview),
							   COL_VAR_NAME),
				  TRUE);
	gtk_tree_path_free (path);
}

static void
remove_variable_clicked_cb (GtkWidget *button, GtkWidget *top_level)
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeModel *model;
	const gchar *msg;
	gchar *name;
	GtkWidget *dlg;
	GbfAmProject *project = g_object_get_data (G_OBJECT (top_level), "__project");
	GbfAmConfigMapping *config = g_object_get_data (G_OBJECT (top_level), "__config");
	
	treeview = g_object_get_data (G_OBJECT (project),
				      "__variables_treeview");
	selection = gtk_tree_view_get_selection (treeview);
	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return;
	gtk_tree_model_get (model, &iter, COL_VAR_NAME, &name, -1);
	msg = _("Are you sure you want to remove variable \"%s\"?");
	dlg = gtk_message_dialog_new_with_markup (NULL,
						  GTK_DIALOG_DESTROY_WITH_PARENT,
						  GTK_MESSAGE_QUESTION,
						  GTK_BUTTONS_YES_NO,
						  msg, name);
	if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_YES)
	{
		GbfAmConfigMapping *variables;
		GbfAmConfigValue *value;
		
		value = gbf_am_config_mapping_lookup (config, "variables");
		if (value)
		{
			variables = gbf_am_config_value_get_mapping (value);
			value = gbf_am_config_value_new (GBF_AM_TYPE_STRING);
			gbf_am_config_value_set_string (value, "");
			gbf_am_config_mapping_insert (variables, name, value);
		}
		gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
	}
	gtk_widget_destroy (dlg);
	g_free (name);
}

static void
variables_treeview_selection_changed_cb (GtkTreeSelection *selection,
					 GbfAmProject *project)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkWidget *remove_variable_button;
	
	remove_variable_button = g_object_get_data (G_OBJECT (project),
					       "__remove_variable_button");
	
	
	if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_widget_set_sensitive (remove_variable_button, FALSE);
	}
	else
	{
		gtk_widget_set_sensitive (remove_variable_button, TRUE);
	}
}

static void
on_variables_hash_foreach (const gchar* variable_name, GbfAmConfigValue *value,
                           gpointer user_data)
{
	GtkTreeIter iter;
	GtkListStore *store = (GtkListStore*) user_data;
	const gchar *variable_value = gbf_am_config_value_get_string (value);
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, COL_VAR_NAME, variable_name,
			    COL_VAR_VALUE, variable_value, -1);
}

static void
on_project_widget_destroy (GtkWidget *wid, GtkWidget *top_level)
{
	GError *err = NULL;
	
	GbfAmProject *project = g_object_get_data (G_OBJECT (top_level), "__project");
	GbfAmConfigMapping *new_config = g_object_get_data (G_OBJECT (top_level), "__config");
	gbf_am_project_set_config (project, new_config, &err);
	if (err) {
		g_warning ("%s", err->message);
		g_error_free (err);
	}
	g_object_unref (top_level);
}

GtkWidget*
gbf_am_properties_get_widget (GbfAmProject *project, GError **error)
{
	GladeXML *gxml;
	GbfAmConfigMapping *config;
	GbfAmConfigValue *value;
	GtkWidget *top_level;
	GtkWidget *table;
	GtkWidget *treeview;
	GtkWidget *add_module_button;
	GtkWidget *add_package_button;
	GtkWidget *remove_button;
	GtkWidget *add_variable_button;
	GtkWidget *remove_variable_button;
	GtkTreeStore *store;
	GtkListStore *variables_store;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
	const gchar *pkg_modules;
	GError *err = NULL;
	GbfAmConfigMapping *variables;
	
	g_return_val_if_fail (GBF_IS_AM_PROJECT (project), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
	
	config = gbf_am_project_get_config (project, &err);
	if (err) {
		g_propagate_error (error, err);
		return NULL;
	}
	
	gxml = glade_xml_new (GLADE_FILE, "project_properties_dialog",
			      GETTEXT_PACKAGE);
	top_level = glade_xml_get_widget (gxml, "top_level");
        
	g_object_set_data (G_OBJECT (top_level), "__project", project);
	g_object_set_data_full (G_OBJECT (top_level), "__config", config,
				(GDestroyNotify)gbf_am_config_mapping_destroy);
	g_signal_connect (top_level, "destroy",
			  G_CALLBACK (on_project_widget_destroy), top_level);
	g_object_ref (top_level);
        
	add_module_button = glade_xml_get_widget (gxml, "add_module_button");
	g_object_set_data (G_OBJECT (project), "__add_module_button",
			   add_module_button);
	
	add_package_button = glade_xml_get_widget (gxml, "add_package_button");
	g_object_set_data (G_OBJECT (project), "__add_package_button",
			   add_package_button);
	
	remove_button = glade_xml_get_widget (gxml, "remove_button");
	g_object_set_data (G_OBJECT (project), "__remove_button",
			   remove_button);
	
	gtk_widget_set_sensitive (add_module_button, TRUE);
	gtk_widget_set_sensitive (add_package_button, FALSE);
	gtk_widget_set_sensitive (remove_button, FALSE);
	
	table = glade_xml_get_widget (gxml, "general_properties_table");
	
	g_object_ref (top_level);
	gtk_container_remove (GTK_CONTAINER(top_level->parent), top_level);
	
	g_signal_connect (add_module_button, "clicked",
			  G_CALLBACK (add_package_module_clicked_cb),
			  project);
	g_signal_connect (add_package_button, "clicked",
			  G_CALLBACK (add_package_clicked_cb),
			  project);
	g_signal_connect (remove_button, "clicked",
			  G_CALLBACK (remove_package_clicked_cb),
			  project);
	
	/* Project Info */
	add_configure_property (project, config, GBF_AM_CONFIG_LABEL,
				_("Project:"), project->project_root_uri,
				NULL, table, 0);
	add_configure_property (project, config, GBF_AM_CONFIG_ENTRY,
				_("Package name:"), NULL, "package_name",
				table, 1);
	add_configure_property (project, config, GBF_AM_CONFIG_ENTRY,
				_("Version:"), NULL, "package_version",
				table, 2);
	add_configure_property (project, config, GBF_AM_CONFIG_ENTRY,
				_("Url:"), NULL, "package_url",
				table, 3);
	
	/* pkg config packages */
	store = gtk_tree_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);
	if ((value = gbf_am_config_mapping_lookup (config, "pkg_check_modules")) &&
	    (pkg_modules = gbf_am_config_value_get_string (value))) {
		GtkTreeIter module_iter;
		gchar **module;
		gchar **modules = g_strsplit (pkg_modules, ", ", -1);
		module = modules;
		while (*module) {
			GbfAmConfigValue *module_info;
			GbfAmConfigMapping *module_info_map;
			
			gchar *module_key = g_strconcat ("pkg_check_modules_",
							 *module,
							 NULL);
			
			if ((module_info = gbf_am_config_mapping_lookup (config, module_key)) &&
			    (module_info_map = gbf_am_config_value_get_mapping (module_info))) {
				GbfAmConfigValue *pkgs_val;
				const gchar *packages;
				
				gtk_tree_store_append (store, &module_iter, NULL);
				gtk_tree_store_set (store, &module_iter, COL_PACKAGE, *module, -1);
				
				if ((pkgs_val = gbf_am_config_mapping_lookup (module_info_map, "packages")) &&
				    (packages = gbf_am_config_value_get_string (pkgs_val))) {
					gchar **pkgs, **pkg;
					pkgs = g_strsplit (packages, ", ", -1);
					pkg = pkgs;
					while (*pkg) {
						GtkTreeIter iter;
						gchar *version;
						
						gtk_tree_store_append (store, &iter, &module_iter);
						if ((version = strchr (*pkg, ' '))) {
							*version = '\0';
							version++;
							gtk_tree_store_set (store, &iter, COL_PACKAGE, *pkg,
									    COL_VERSION, version, -1);
						} else {
							gtk_tree_store_set (store, &iter, COL_PACKAGE, *pkg, -1);
						}
						pkg++;
					}
					g_strfreev (pkgs);
				}
			}
			g_free (module_key);
			module++;
		}
		g_strfreev (modules);
	}
	
	treeview = glade_xml_get_widget (gxml, "packages_treeview");
	
	g_object_set_data (G_OBJECT (project), "__packages_treeview", treeview);
	g_object_set_data (G_OBJECT (project), "__config", config);
	
	gtk_tree_view_set_model (GTK_TREE_VIEW(treeview),
				 GTK_TREE_MODEL (store));
	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "editable", TRUE, NULL);
	g_signal_connect (G_OBJECT (renderer), "edited",
			  G_CALLBACK (package_edited_cb), top_level);
	col = gtk_tree_view_column_new_with_attributes (_("Module/Packages"),
							renderer,
							"text", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), col);
	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "editable", TRUE, NULL);
	g_signal_connect (G_OBJECT (renderer), "edited",
			  G_CALLBACK (package_version_edited_cb), top_level);
	col = gtk_tree_view_column_new_with_attributes (_("Version"),
							renderer,
							"text", 1, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), col);
	gtk_tree_view_expand_all (GTK_TREE_VIEW (treeview));
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	g_signal_connect (selection, "changed",
			  G_CALLBACK (packages_treeview_selection_changed_cb),
			  project);
	
	/* Set up variables list */
	variables_store = gtk_list_store_new (N_VAR_COLUMNS,
					      G_TYPE_STRING,
					      G_TYPE_STRING,
					      G_TYPE_BOOLEAN);
	if ((value = gbf_am_config_mapping_lookup (config, "variables")) &&
	    (variables = gbf_am_config_value_get_mapping (value))) {
		gbf_am_config_mapping_foreach (variables,
					       on_variables_hash_foreach,
					       variables_store);
	}
	
	treeview = glade_xml_get_widget (gxml, "variables_treeview");
	g_object_set_data (G_OBJECT (project), "__variables_treeview",
			   treeview);
	gtk_tree_view_set_model (GTK_TREE_VIEW(treeview),
				 GTK_TREE_MODEL (variables_store));
	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "editable", TRUE, NULL);
	g_signal_connect (G_OBJECT (renderer), "edited",
			  G_CALLBACK (variable_name_edited_cb), top_level);
	col = gtk_tree_view_column_new_with_attributes (_("Variable"),
							renderer,
							"text", 
							COL_VAR_NAME, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), col);
	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "editable", TRUE, NULL);
	g_signal_connect (G_OBJECT (renderer), "edited",
			  G_CALLBACK (variable_value_edited_cb), top_level);
	col = gtk_tree_view_column_new_with_attributes (_("Value"),
							renderer,
							"text", COL_VAR_VALUE,
							NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), col);
	gtk_tree_view_expand_all (GTK_TREE_VIEW (treeview));

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	g_signal_connect (selection, "changed",
			  G_CALLBACK (variables_treeview_selection_changed_cb),
			  project);

	add_variable_button = glade_xml_get_widget (gxml, "add_variable_button");
	g_object_set_data (G_OBJECT (project), "__add_variable_button",
			   add_variable_button);
	
	remove_variable_button = glade_xml_get_widget (gxml, "remove_variable_button");
	g_object_set_data (G_OBJECT (project), "__remove_variable_button",
			   remove_variable_button);
	gtk_widget_set_sensitive (add_variable_button, TRUE);
	gtk_widget_set_sensitive (remove_variable_button, FALSE);
	
	g_signal_connect (add_variable_button, "clicked",
			  G_CALLBACK (add_variable_clicked_cb),
			  project);
	g_signal_connect (remove_variable_button, "clicked",
			  G_CALLBACK (remove_variable_clicked_cb),
			  top_level);
	
	gtk_widget_show_all (top_level);
	
	g_object_unref (variables_store);
	g_object_unref (store);
	g_object_unref (gxml);
	return top_level;
}

enum
{
	COLUMN_CHECK,
	COLUMN_MODULE_NAME
};

static const gchar* get_libs_key(GbfProjectTarget *target)
{
	g_return_val_if_fail (target != NULL, "ldadd");
	/* We need to care here if it is ldadd or libadd #476315 */
	if (g_str_equal (target->type, "shared_lib"))
	{
		return "libadd";
	}
	else
	{
		return "ldadd";
	}
}

static void
on_module_activate (GtkCellRendererToggle* cell_renderer,
		    const gchar* tree_path,
		    GtkTreeView* tree)
{
	GtkTreeIter iter;
	GtkTreeModel* model = gtk_tree_view_get_model (tree);
	gchar* module_name;
	gchar* module_cflags;
	gchar* module_libs;
	gboolean use;
	GtkTreePath* path = gtk_tree_path_new_from_string (tree_path);
	GbfProjectTarget* target = g_object_get_data (G_OBJECT(tree), "target");
	GbfAmConfigMapping* config = g_object_get_data (G_OBJECT(tree), "config");
	GbfAmConfigMapping* group_config = g_object_get_data (G_OBJECT(tree), "group_config");	
	GbfAmConfigValue* am_cpp_value = gbf_am_config_mapping_lookup(group_config, "amcppflags");
	GbfAmConfigValue* cpp_value = gbf_am_config_mapping_lookup(config, "cppflags");
	GbfAmConfigValue* libs_value = gbf_am_config_mapping_lookup(config, 
								    get_libs_key(target));
	
	if (!cpp_value)
	{
		cpp_value = gbf_am_config_value_new (GBF_AM_TYPE_STRING);
	}
	if (!libs_value)
	{
		libs_value = gbf_am_config_value_new (GBF_AM_TYPE_STRING);
	}
	if (!am_cpp_value)
	{
		am_cpp_value = gbf_am_config_value_new (GBF_AM_TYPE_STRING);
	}
	
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter,
			    COLUMN_MODULE_NAME, &module_name,
			    COLUMN_CHECK, &use,
			    -1);
	
	module_cflags = g_strdup_printf ("$(%s_CFLAGS)", (gchar*) module_name);
	module_libs = g_strdup_printf ("$(%s_LIBS)", (gchar*) module_name);
	g_free (module_name);
	gtk_tree_path_free (path);
	
	if (!use)
	{
		GString* cppflags;
		GString* am_cpp_flags;
		GString* libs;
		cppflags = g_string_new (gbf_am_config_value_get_string (cpp_value));
		am_cpp_flags = g_string_new (gbf_am_config_value_get_string (am_cpp_value));
		libs = g_string_new (gbf_am_config_value_get_string (libs_value));

		if (strlen (cppflags->str) && !strstr (cppflags->str, module_cflags))
		{
			g_string_append_printf (cppflags, " %s", module_cflags);
			gbf_am_config_value_set_string (cpp_value, cppflags->str);
			gbf_am_config_mapping_insert (config, "cppflags", cpp_value);
		}
		else if (!strstr (am_cpp_flags->str, module_cflags))
		{
			g_string_append_printf (am_cpp_flags, " %s", module_cflags);
			gbf_am_config_value_set_string (am_cpp_value, am_cpp_flags->str);
			gbf_am_config_mapping_insert (group_config, "amcppflags", am_cpp_value);
		}
		if (!strstr (libs->str, module_libs))
		{
			g_string_append_printf (libs, " %s", module_libs);		
			gbf_am_config_value_set_string (libs_value, libs->str);
			gbf_am_config_mapping_insert (config, get_libs_key (target), libs_value);
		}

		g_string_free (libs, TRUE);
		g_string_free (cppflags, TRUE);
		g_string_free (am_cpp_flags, TRUE);
		
	}
	else
	{
		const gchar* cpp_flags = cpp_value ? gbf_am_config_value_get_string(cpp_value) : NULL;
		const gchar* am_cpp_flags = am_cpp_value ? gbf_am_config_value_get_string(am_cpp_value) : NULL;
		const gchar* libs_flags = libs_value ? gbf_am_config_value_get_string(libs_value) : NULL;

		if (cpp_flags && strlen (cpp_flags))
		{
			const gchar* start = strstr (cpp_flags, module_cflags);
			GString* str = g_string_new (cpp_flags);
			if (start)
				g_string_erase (str, start - cpp_flags, strlen (module_cflags));
			gbf_am_config_value_set_string (cpp_value, str->str);
			g_string_free (str, TRUE);
			gbf_am_config_mapping_insert (config, "cppflags", cpp_value);
		}
		else if (am_cpp_flags)
		{
			const gchar* start = strstr (am_cpp_flags, module_cflags);
			GString* str = g_string_new (am_cpp_flags);
			if (start)
				g_string_erase (str, start - am_cpp_flags, strlen (module_cflags));
			gbf_am_config_value_set_string (am_cpp_value, str->str);
			g_string_free (str, TRUE);
			gbf_am_config_mapping_insert (group_config, "amcppflags", am_cpp_value);
		}
		if (libs_flags)
		{
			const gchar* start = strstr (libs_flags, module_libs);
			GString* str = g_string_new (libs_flags);
			if (start)
				g_string_erase (str, start - libs_flags, strlen (module_libs));
			gbf_am_config_value_set_string (libs_value, str->str);
			g_string_free (str, TRUE);
			gbf_am_config_mapping_insert (config, get_libs_key (target), libs_value);
		}
	}
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_CHECK, !use, -1);
	g_free (module_cflags);
	g_free (module_libs);	
}

static gboolean
update_tree_foreach (GtkTreeModel* model,
		     GtkTreePath* path,
		     GtkTreeIter* iter,
		     GtkWidget* view)
{
	gchar* module;
	gboolean use = FALSE;
	GbfAmConfigMapping* config = g_object_get_data (G_OBJECT(view), "config");
	GbfAmConfigMapping* group_config = g_object_get_data (G_OBJECT(view), "group_config");
	GbfProjectTarget* target = g_object_get_data (G_OBJECT(view), "target");
	GbfAmConfigValue* am_cpp_value = gbf_am_config_mapping_lookup(group_config, "amcppflags");
	GbfAmConfigValue* cpp_value = gbf_am_config_mapping_lookup(config, "cppflags");
	GbfAmConfigValue* libs_value = gbf_am_config_mapping_lookup(config, get_libs_key(target));
	const gchar* cpp_flags = cpp_value ? gbf_am_config_value_get_string(cpp_value) : NULL;
	const gchar* am_cpp_flags = am_cpp_value ? gbf_am_config_value_get_string(am_cpp_value) : NULL;
	const gchar* libs_flags = libs_value ? gbf_am_config_value_get_string(libs_value) : NULL;

	gchar* config_cflags; 
	gchar* config_libs;
	
	gtk_tree_model_get (model, iter,
			    COLUMN_MODULE_NAME, &module,
			    -1);
	
	config_cflags = g_strdup_printf ("$(%s_CFLAGS)", (gchar*) module);
	config_libs = g_strdup_printf ("$(%s_LIBS)", (gchar*) module);
	g_free (module);
	
	if ((cpp_flags && strstr (cpp_flags, config_cflags)) || 
	    (am_cpp_flags && strstr (am_cpp_flags, config_cflags)))
	{
		if (libs_flags && strstr(libs_flags, config_libs))
			use = TRUE;
	}
	
	gtk_list_store_set (GTK_LIST_STORE(model), iter,
			    COLUMN_CHECK, use,
			    -1);
	g_free (config_cflags);
	g_free (config_libs);
	
	return FALSE; /* continue iteration */
}

static GtkWidget*
create_module_list (GbfAmProject *project,
		    GbfProjectTarget *target,
		    GbfAmConfigMapping *config,
		    GbfAmConfigMapping *group_config)
{
	GtkWidget* view;
	GtkListStore* list;
	GList* modules;
	GList* node;
	GtkCellRenderer* renderer_text;
	GtkCellRenderer* renderer_toggle;
	GtkTreeViewColumn* column_text;
	GtkTreeViewColumn* column_toggle;
	
	g_return_val_if_fail (GBF_IS_AM_PROJECT (project), NULL);
	
	list = gtk_list_store_new (2, G_TYPE_BOOLEAN, G_TYPE_STRING);
	view = gtk_tree_view_new_with_model (GTK_TREE_MODEL(list));
	g_object_set_data (G_OBJECT(view), "config", config);
	g_object_set_data (G_OBJECT(view), "group_config", group_config);
	g_object_set_data (G_OBJECT(view), "target", target);	
	renderer_text = gtk_cell_renderer_text_new();
	renderer_toggle = gtk_cell_renderer_toggle_new();
	g_signal_connect (renderer_toggle, "toggled", G_CALLBACK (on_module_activate),
			  view);
	
	column_toggle = gtk_tree_view_column_new_with_attributes (_("Use"),
								  renderer_toggle,
								  "active", COLUMN_CHECK,
								  NULL);
	
	column_text = gtk_tree_view_column_new_with_attributes (_("Module"),
								renderer_text,
								"text", COLUMN_MODULE_NAME,
								NULL);
	
	gtk_tree_view_append_column (GTK_TREE_VIEW (view), column_toggle);
	gtk_tree_view_append_column (GTK_TREE_VIEW (view), column_text);
	gtk_widget_set_size_request (view, -1, 200);
	
	modules = gbf_project_get_config_modules (GBF_PROJECT (project), NULL);
	
	for (node = modules; node != NULL; node = g_list_next (node))
	{
		GtkTreeIter iter;		
		
		gtk_list_store_append (list, &iter);
		gtk_list_store_set (list, &iter,
				    COLUMN_CHECK, FALSE,
				    COLUMN_MODULE_NAME, node->data,
				    -1);
	}
	gtk_tree_model_foreach (GTK_TREE_MODEL(list),
				(GtkTreeModelForeachFunc) update_tree_foreach,
				view);
	
	return view;
}

static void
on_group_widget_destroy (GtkWidget *wid, GtkWidget *table)
{
	GError *err = NULL;
	
	GbfAmProject *project = g_object_get_data (G_OBJECT (table), "__project");
	GbfAmConfigMapping *new_config = g_object_get_data (G_OBJECT (table), "__config");
	const gchar *group_id = g_object_get_data (G_OBJECT (table), "__group_id");
	gbf_am_project_set_group_config (project, group_id, new_config, &err);
	if (err) {
		g_warning ("%s", err->message);
		g_error_free (err);
	}
	g_object_unref (table);
}

GtkWidget*
gbf_am_properties_get_group_widget (GbfAmProject *project,
				    const gchar *group_id,
				    GError **error)
{
	GbfProjectGroup *group;
	GbfAmConfigMapping *config;
	GbfAmConfigValue *value;
	GtkWidget *table;
	GtkWidget *table_cflags;
	GtkWidget *expander_cflags;
	GError *err = NULL;
	
	g_return_val_if_fail (GBF_IS_AM_PROJECT (project), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	group = gbf_project_get_group (GBF_PROJECT (project), group_id, &err);
	if (err) {
		g_propagate_error (error, err);
		return NULL;
	}
	config = gbf_am_project_get_group_config (project, group_id, &err);
	if (err) {
		g_propagate_error (error, err);
		return NULL;
	}
	
	g_return_val_if_fail (group != NULL, NULL);
	g_return_val_if_fail (config != NULL, NULL);
	
	table = gtk_table_new (7, 2, FALSE);
	g_object_ref (table);
	g_object_set_data (G_OBJECT (table), "__project", project);
	g_object_set_data_full (G_OBJECT (table), "__config", config,
				(GDestroyNotify)gbf_am_config_mapping_destroy);
	g_object_set_data_full (G_OBJECT (table), "__group_id",
				g_strdup (group_id),
				(GDestroyNotify)g_free);
	g_signal_connect (table, "destroy",
			  G_CALLBACK (on_group_widget_destroy), table);
	
	/* Group name */
	add_configure_property (project, config, GBF_AM_CONFIG_LABEL,
				_("Group name:"), group->name, NULL, table, 0);
	/* CFlags */
	table_cflags = gtk_table_new(7,2, FALSE);
	expander_cflags = gtk_expander_new(_("Advanced"));
	gtk_table_attach (GTK_TABLE (table), expander_cflags, 0, 2, 2, 3,
				  GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 3);
	gtk_container_add(GTK_CONTAINER(expander_cflags), table_cflags);
	add_configure_property (project, config, GBF_AM_CONFIG_ENTRY,
				_("C compiler flags:"), NULL, "amcflags", table_cflags, 0);
	add_configure_property (project, config, GBF_AM_CONFIG_ENTRY,
				_("C preprocessor flags:"), NULL, "amcppflags", table_cflags, 1);
	add_configure_property (project, config, GBF_AM_CONFIG_ENTRY,
				_("C++ compiler flags:"), NULL, "amcxxflags", table_cflags, 2);
	add_configure_property (project, config, GBF_AM_CONFIG_ENTRY,
				_("gcj compiler flags (ahead-of-time):"), NULL, "amgcjflags", table_cflags, 3);
	add_configure_property (project, config, GBF_AM_CONFIG_ENTRY,
				_("Java compiler flags (just-in-time):"), NULL, "amjavaflags", table_cflags, 4);
	add_configure_property (project, config, GBF_AM_CONFIG_ENTRY,
				_("Fortran compiler flags:"), NULL, "amfflags", table_cflags, 5);
	/* Includes */
	add_configure_property (project, config, GBF_AM_CONFIG_ENTRY,
				_("Includes (deprecated):"), NULL, "includes",
				table_cflags, 6);
				
	/* Install directories */
	value = gbf_am_config_mapping_lookup (config, "installdirs");
	if (value) {
		GtkWidget *table2, *frame, *lab;
		char *text;
		frame = gtk_frame_new ("");
		
		lab = gtk_frame_get_label_widget (GTK_FRAME(frame));
		text = g_strdup_printf ("<b>%s</b>", _("Install directories:"));
		gtk_label_set_markup (GTK_LABEL(lab), text);
		g_free (text);
				      
		gtk_frame_set_shadow_type (GTK_FRAME (frame),
					   GTK_SHADOW_NONE);
		gtk_widget_show (frame);
		gtk_table_attach (GTK_TABLE (table), frame, 0, 2, 3, 4,
				  GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 3);
		table2 = gtk_table_new (0, 0, FALSE);
		gtk_widget_show (table2);
		gtk_container_set_border_width (GTK_CONTAINER (table2), 5);
		gtk_container_add (GTK_CONTAINER(frame), table2);
		gbf_am_config_mapping_foreach (value->mapping,
					       recursive_config_foreach_cb,
					       table2);
	}
	gtk_widget_show_all (table);
	gbf_project_group_free (group);
	return table;
}

static void
on_target_widget_destroy (GtkWidget *wid, GtkWidget *table)
{
	GError *err = NULL;
	
	GbfAmProject *project = g_object_get_data (G_OBJECT (table), "__project");
	GbfAmConfigMapping *new_config = g_object_get_data (G_OBJECT (table), "__config");
	GbfAmConfigMapping *new_group_config = g_object_get_data (G_OBJECT (table), "__group_config");
	const gchar *target_id = g_object_get_data (G_OBJECT (table), "__target_id");
	const gchar *group_id = g_object_get_data (G_OBJECT (table), "__group_id");
	gbf_am_project_set_target_config (project, target_id, new_config, &err);
	if (err) {
		g_warning ("%s", err->message);
		g_error_free (err);
	}
	err = NULL;
	gbf_am_project_set_group_config (project, group_id, new_group_config, &err);
	if (err) {
		g_warning ("%s", err->message);
		g_error_free (err);
	}
	g_object_unref (table);
}

static void
on_advanced_clicked (GtkButton* button,
		     GtkWidget* target_widget)
{
	GtkWidget* dialog;
	GtkWidget* table_cflags;
	const gchar* lib_type;
	GbfAmProject* project;
	GbfAmConfigMapping* config;
	GbfProjectTarget* target;
	GtkWidget* view;
	GtkTreeModel* model;
	
	project = g_object_get_data (G_OBJECT(target_widget), "__project");
	config = g_object_get_data (G_OBJECT(target_widget), "__config");
	target = g_object_get_data (G_OBJECT(target_widget), "__target");
	view = g_object_get_data (G_OBJECT(target_widget), "__view");
		
	table_cflags = gtk_table_new(9,2, FALSE);
	add_configure_property (project, config, GBF_AM_CONFIG_ENTRY,
				_("C compiler flags:"), NULL, "cflags", table_cflags, 0);
	add_configure_property (project, config, GBF_AM_CONFIG_ENTRY,
				_("C preprocessor flags"), NULL, "cppflags", table_cflags, 1);
	add_configure_property (project, config, GBF_AM_CONFIG_ENTRY,
				_("C++ compiler flags"), NULL, "cxxflags", table_cflags, 2);
	add_configure_property (project, config, GBF_AM_CONFIG_ENTRY,
				_("gcj compiler flags (ahead-of-time)"), NULL, "gcjflags", table_cflags, 3);
	add_configure_property (project, config, GBF_AM_CONFIG_ENTRY,
				_("Fortran compiler flags:"), NULL, "amfflags", table_cflags, 4);
	/* LDFLAGS */
	add_configure_property (project, config,
				GBF_AM_CONFIG_ENTRY,
				_("Linker flags:"), NULL,
				"ldflags", table_cflags, 6);
	
	lib_type = get_libs_key (target);			     
	add_configure_property (project, config,
				GBF_AM_CONFIG_ENTRY,
				_("Libraries:"), NULL,
				lib_type, table_cflags, 7);
	
	/* DEPENDENCIES */
	add_configure_property (project, config,
				GBF_AM_CONFIG_ENTRY,
				_("Dependencies:"), NULL,
				"explicit_deps", table_cflags, 8);
	
	dialog = gtk_dialog_new_with_buttons (_("Advanced options"),
					      NULL,
					      GTK_DIALOG_MODAL,
					      GTK_STOCK_CLOSE,
					      GTK_RESPONSE_CLOSE,
					      NULL);
	gtk_container_add (GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), table_cflags);
	gtk_widget_show_all (dialog);
	
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	
	/* Little hack to reuse func... */
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));
	gtk_tree_model_foreach (model,
				(GtkTreeModelForeachFunc) update_tree_foreach,
				view);
}

GtkWidget*
gbf_am_properties_get_target_widget (GbfAmProject *project,
				     const gchar *target_id, GError **error)
{
	GbfProjectGroup *group;
	GbfAmConfigMapping *group_config;
	GbfProjectTarget *target;
	GbfAmConfigMapping *config;
	GbfAmConfigValue *value;
	GbfAmConfigMapping *installdirs;
	GbfAmConfigValue *installdirs_val, *dir_val;
	GtkWidget *table;
	GError *err = NULL;
	
	g_return_val_if_fail (GBF_IS_AM_PROJECT (project), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
	
	target = gbf_project_get_target (GBF_PROJECT (project), target_id, &err);
	if (err) {
		g_propagate_error (error, err);
		return NULL;
	}
	config = gbf_am_project_get_target_config (project, target_id, &err);
	if (err) {
		g_propagate_error (error, err);
		return NULL;
	}
	g_return_val_if_fail (target != NULL, NULL);
	g_return_val_if_fail (config != NULL, NULL);
	
	group = gbf_project_get_group (GBF_PROJECT (project),
				       target->group_id, NULL);
	group_config = gbf_am_project_get_group_config (project,
							target->group_id,
							NULL);
	table = gtk_table_new (9, 2, FALSE);
	g_object_ref (table);
	g_object_set_data (G_OBJECT (table), "__project", project);
	g_object_set_data_full (G_OBJECT (table), "__config", config,
				(GDestroyNotify)gbf_am_config_mapping_destroy);
	g_object_set_data_full (G_OBJECT (table), "__group_config", group_config,
				(GDestroyNotify)gbf_am_config_mapping_destroy);
	g_object_set_data_full (G_OBJECT (table), "__target_id",
				g_strdup (target_id),
				(GDestroyNotify)g_free);
	g_object_set_data_full (G_OBJECT (table), "__group_id",
				g_strdup(group->id),
				(GDestroyNotify)g_free);
	g_object_set_data_full (G_OBJECT (table), "__target", 
				target,
				(GDestroyNotify)gbf_project_target_free);
	g_signal_connect (table, "destroy",
			  G_CALLBACK (on_target_widget_destroy), table);
	
	/* Target name */
	add_configure_property (project, config, GBF_AM_CONFIG_LABEL,
				_("Target name:"), target->name, NULL, table, 0);
	/* Target type */
	add_configure_property (project, config, GBF_AM_CONFIG_LABEL,
				_("Type:"),
				gbf_project_name_for_type (GBF_PROJECT (project),
							   target->type),
				NULL, table, 1);
	/* Target group */
	add_configure_property (project, config, GBF_AM_CONFIG_LABEL,
				_("Group:"), group->name,
				NULL, table, 2);
	
	/* Target primary */
	/* FIXME: Target config 'installdir' actually stores target primary,
	 * and not what it really seems to mean, i.e the primary prefix it
	 * belongs to. The actual install directory of a target is the
	 * install directory of primary prefix it belongs to and is
	 * configured in group properties.
	 */
	value = gbf_am_config_mapping_lookup (config, "installdir");
	installdirs_val = gbf_am_config_mapping_lookup (group_config,
							"installdirs");
	if (installdirs_val)
		installdirs = gbf_am_config_value_get_mapping (installdirs_val);
	
	if (!value || !installdirs_val) {
		add_configure_property (project, config, GBF_AM_CONFIG_LABEL,
					_("Install directory:"), NULL, "installdir",
					table, 3);
	} else {
		const gchar *primary_prefix;
		gchar *installdir;
		
		primary_prefix = gbf_am_config_value_get_string (value);
		installdirs = gbf_am_config_value_get_mapping (installdirs_val);
		dir_val = gbf_am_config_mapping_lookup (installdirs,
							primary_prefix);
		if (dir_val) {
			installdir = g_strconcat (primary_prefix, " = ",
						  gbf_am_config_value_get_string (dir_val),
						  NULL);
			add_configure_property (project, config,
						GBF_AM_CONFIG_LABEL,
						_("Install directory:"),
						installdir, NULL,
						table, 3);
			g_free (installdir);
		} else {
			add_configure_property (project, config,
						GBF_AM_CONFIG_LABEL,
						_("Install directory:"),
						NULL, "installdir",
						table, 3);
		}
	}

	if (target->type && (strcmp (target->type, "program") == 0 ||
			     strcmp (target->type, "shared_lib") == 0 ||
			     strcmp (target->type, "static_lib") == 0)) {
		GtkWidget* scrolled_window;
		GtkWidget* view =  create_module_list(project, target, 
						      config, group_config);
		GtkWidget* button = gtk_button_new_with_label (_("Advanced..."));
		scrolled_window = gtk_scrolled_window_new (NULL, NULL);
		gtk_container_add (GTK_CONTAINER(scrolled_window), view);
		gtk_table_attach (GTK_TABLE (table), scrolled_window, 0, 2, 4, 5,
				  GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 3);
		gtk_table_attach (GTK_TABLE (table), button, 0, 2, 5, 6,
				  GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 3);	
		g_object_set_data (G_OBJECT (table), "__view", view);
		g_signal_connect (button, "clicked", G_CALLBACK(on_advanced_clicked), table);
	}
	gtk_widget_show_all (table);
	return table;
}
