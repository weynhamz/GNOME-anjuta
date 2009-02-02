/*  -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4; coding: utf-8 -*-
 * 
 * Copyright (C) 2003 Gustavo Giráldez
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA. 
 * 
 * Author: Gustavo Giráldez <gustavo.giraldez@gmx.net>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdarg.h>
#include <glade/glade-xml.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#include "gbf-tree-data.h"
#include "gbf-project-view.h"
#include "gbf-project-model.h"
#include "gbf-project-util.h"

#define ICON_SIZE 16

#define GLADE_FILE PACKAGE_DATA_DIR  "/glade/create_dialogs.glade"

enum {
    COLUMN_FILE,
    COLUMN_URI,
    N_COLUMNS
};

static GtkWidget *
custom_widget_handler (GladeXML *xml,
                       gchar    *func_name,
                       gchar    *name,
                       gchar    *string1,
                       gchar    *string2,
                       gint      int1,
                       gint      int2,
                       gpointer  user_data)
{
    GtkWidget *wid = NULL;

    if (!strcmp (string1, "GtkComboBox")) {
        wid = gtk_combo_box_new ();
    } else if (!strcmp (string1, "GbfProjectView")) {
        wid = gbf_project_view_new ();
    } else {
        g_warning ("Unknown custom widget type '%s'", string1);
    }

    return wid;
}

static GladeXML *
load_interface (const gchar *top_widget)
{
    GladeXML *xml;

    glade_set_custom_handler (custom_widget_handler, NULL);
    xml = glade_xml_new (GLADE_FILE, top_widget, GETTEXT_PACKAGE);

    if (!xml) {
        g_warning ("%s", _("Couldn't load glade file"));
        return NULL;
    }
    
    return xml;
}

static gboolean
groups_filter_fn (GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
    GbfTreeData *data = NULL;
    gboolean retval = FALSE;

    gtk_tree_model_get (model, iter,
                        GBF_PROJECT_MODEL_COLUMN_DATA, &data, -1);
    retval = (data && data->type == GBF_TREE_NODE_GROUP);
    gbf_tree_data_free (data);
    
    return retval;
}

static void 
setup_groups_treeview (GbfProjectModel *model,
                       GtkWidget       *view,
                       const gchar     *select_group)
{
    GtkTreeModel *filter;
    GtkTreePath *path;
    GtkTreeIter iter;
    
    g_return_if_fail (model != NULL);
    g_return_if_fail (view != NULL && GBF_IS_PROJECT_VIEW (view));

    filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (model), NULL);
    gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (filter),
                                            groups_filter_fn, NULL, NULL);
    gtk_tree_view_set_model (GTK_TREE_VIEW (view), GTK_TREE_MODEL (filter));
    g_object_unref (filter);
    
    /* select default group */
    if (select_group && gbf_project_model_find_id (model, &iter,
                                                   GBF_TREE_NODE_GROUP, select_group)) {
        GtkTreeIter iter_filter;
        
        gtk_tree_model_filter_convert_child_iter_to_iter (
            GTK_TREE_MODEL_FILTER (filter), &iter_filter, &iter);
        path = gtk_tree_model_get_path (filter, &iter_filter);
        gtk_tree_view_expand_to_path (GTK_TREE_VIEW (view), path);
    } else {
        GtkTreePath *root_path;

        gtk_tree_view_expand_all (GTK_TREE_VIEW (view));
        root_path = gbf_project_model_get_project_root (model);
        if (root_path) {
            path = gtk_tree_model_filter_convert_child_path_to_path (
                GTK_TREE_MODEL_FILTER (filter), root_path);
            gtk_tree_path_free (root_path);
        } else {
            path = gtk_tree_path_new_first ();
        }
    }
    
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (view), path, NULL, FALSE);
    gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (view), path, NULL,
                                  TRUE, 0.5, 0.0);
    gtk_tree_path_free (path);
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

gchar*
gbf_project_util_new_group (GbfProjectModel *model,
                            GtkWindow       *parent,
                            const gchar     *default_group,
                            const gchar     *default_group_name_to_add)
{
    GladeXML *gui;
    GtkWidget *dialog, *group_name_entry, *ok_button;
    GtkWidget *groups_view;
    gint response;
    GbfProject *project;
    gboolean finished = FALSE;
    gchar *new_group = NULL;

    g_return_val_if_fail (model != NULL, NULL);
    
    project = gbf_project_model_get_project (model);
    if (!project)
        return NULL;

    gui = load_interface ("new_group_dialog");
    g_return_val_if_fail (gui != NULL, NULL);
    
    /* get all needed widgets */
    dialog = glade_xml_get_widget (gui, "new_group_dialog");
    groups_view = glade_xml_get_widget (gui, "groups_view");
    group_name_entry = glade_xml_get_widget (gui, "group_name_entry");
    ok_button = glade_xml_get_widget (gui, "ok_button");
    
    /* set up dialog */
    if (default_group_name_to_add)
        gtk_entry_set_text (GTK_ENTRY (group_name_entry),
                            default_group_name_to_add);
    g_signal_connect (group_name_entry, "changed",
                      (GCallback) entry_changed_cb, ok_button);
    if (default_group_name_to_add)
        gtk_widget_set_sensitive (ok_button, TRUE);
    else
        gtk_widget_set_sensitive (ok_button, FALSE);
    
    setup_groups_treeview (model, groups_view, default_group);
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
                GbfTreeData *data;
                gchar *parent_id = NULL, *name;
            
                name = gtk_editable_get_chars (
                    GTK_EDITABLE (group_name_entry), 0, -1);
                data = gbf_project_view_find_selected (GBF_PROJECT_VIEW (groups_view),
                                                       GBF_TREE_NODE_GROUP);
                if (data) {
                    gchar *new_group = NULL;
                    
                    parent_id = g_strdup (data->id);
                    gbf_tree_data_free (data);
		    
                    new_group = gbf_project_add_group (project, parent_id, name, &err);
                    if (err) {
                        error_dialog (parent, _("Can not add group"), "%s",
                                      err->message);
                        g_error_free (err);
                    } else {
			finished = TRUE;
		    }
                    g_free (parent_id);
                } else {
                    error_dialog (parent, _("Can not add group"),
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

enum {
    TARGET_TYPE_TYPE = 0,
    TARGET_TYPE_NAME,
    TARGET_TYPE_PIXBUF,
    TARGET_TYPE_N_COLUMNS
};

/* create a tree model with the target types */
static GtkListStore *
build_types_store (GbfProject *project)
{
    GtkListStore *store;
    GtkTreeIter iter;
    gchar **types;
    gint i;
    
    types = gbf_project_get_types (project);
    store = gtk_list_store_new (TARGET_TYPE_N_COLUMNS,
                                G_TYPE_STRING,
                                G_TYPE_STRING,
                                GDK_TYPE_PIXBUF);
    
    for (i = 0; types [i]; i++) {
        GdkPixbuf *pixbuf;
        const gchar *name;

        name = gbf_project_name_for_type (project, types [i]);
        pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default(),
                                           GTK_STOCK_CONVERT,
                                           ICON_SIZE,
                                           GTK_ICON_LOOKUP_GENERIC_FALLBACK,
                                           NULL);

        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter,
                            TARGET_TYPE_TYPE, types [i],
                            TARGET_TYPE_NAME, name,
                            TARGET_TYPE_PIXBUF, pixbuf,
                            -1);

        if (pixbuf)
            g_object_unref (pixbuf);
    }

    g_strfreev (types);

    return store;
}

gchar* 
gbf_project_util_new_target (GbfProjectModel *model,
                             GtkWindow       *parent,
                             const gchar     *default_group,
                             const gchar     *default_target_name_to_add)
{
    GladeXML *gui;
    GtkWidget *dialog, *target_name_entry, *ok_button;
    GtkWidget *target_type_combo, *groups_view;
    GtkListStore *types_store;
    GtkCellRenderer *renderer;
    gint response;
    GbfProject *project;
    gboolean finished = FALSE;
    gchar *new_target = NULL;
    
    g_return_val_if_fail (model != NULL, NULL);
    
    project = gbf_project_model_get_project (model);
    if (!project)
        return NULL;

    gui = load_interface ("new_target_dialog");
    g_return_val_if_fail (gui != NULL, NULL);
    
    /* get all needed widgets */
    dialog = glade_xml_get_widget (gui, "new_target_dialog");
    groups_view = glade_xml_get_widget (gui, "groups_view");
    target_name_entry = glade_xml_get_widget (gui, "target_name_entry");
    target_type_combo = glade_xml_get_widget (gui, "target_type_combo");
    ok_button = glade_xml_get_widget (gui, "ok_button");
    
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
    
    setup_groups_treeview (model, groups_view, default_group);
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
                GbfTreeData *data;
                GtkTreeIter iter;
                gchar *group_id = NULL, *name, *type = NULL;
                
                name = gtk_editable_get_chars (
                    GTK_EDITABLE (target_name_entry), 0, -1);
                data = gbf_project_view_find_selected (GBF_PROJECT_VIEW (groups_view),
                                                       GBF_TREE_NODE_GROUP);

                /* retrieve target type */
                if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (target_type_combo), &iter)) {
                    gtk_tree_model_get (GTK_TREE_MODEL (types_store), &iter, 
                                        TARGET_TYPE_TYPE, &type,
                                        -1);
                }
                
                if (data && type) {
                    group_id = g_strdup (data->id);
                    gbf_tree_data_free (data);
            
                    new_target = gbf_project_add_target (project, group_id, name, type, &err);
                    if (err) {
                        error_dialog (parent, _("Can not add target"), "%s",
                                      err->message);
                        g_error_free (err);
                    } else {
			finished = TRUE;
		    }
		    g_free (group_id);
                    g_free (type);
                } else {
                    error_dialog (parent, _("Can not add target"), "%s",
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

static gboolean
targets_filter_fn (GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
    GbfTreeData *data = NULL;
    gboolean retval = FALSE;

    gtk_tree_model_get (model, iter,
                        GBF_PROJECT_MODEL_COLUMN_DATA, &data, -1);
    retval = (data && !data->is_shortcut &&
              (data->type == GBF_TREE_NODE_GROUP ||
               data->type == GBF_TREE_NODE_TARGET));
    gbf_tree_data_free (data);
    
    return retval;
}

static void 
setup_targets_treeview (GbfProjectModel *model,
                        GtkWidget       *view,
                        const gchar     *select_target,
                        const gchar     *select_group)
{
    GtkTreeModel *filter;
    GtkTreeIter iter, iter_filter;
    GtkTreePath *path = NULL;
    
    g_return_if_fail (model != NULL);
    g_return_if_fail (view != NULL && GBF_IS_PROJECT_VIEW (view));

    filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (model), NULL);
    gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (filter),
                                            targets_filter_fn, NULL, NULL);
    gtk_tree_view_set_model (GTK_TREE_VIEW (view), GTK_TREE_MODEL (filter));
    g_object_unref (filter);

    /* select default target */
    if (select_target) {
        if (gbf_project_model_find_id (model, &iter,
                                       GBF_TREE_NODE_TARGET, select_target)) {
            gtk_tree_model_filter_convert_child_iter_to_iter (
                GTK_TREE_MODEL_FILTER (filter), &iter_filter, &iter);
            path = gtk_tree_model_get_path (filter, &iter_filter);
        }
    } else if (select_group) {
        if (gbf_project_model_find_id (model, &iter,
                                       GBF_TREE_NODE_GROUP, select_group)) {
            gtk_tree_model_filter_convert_child_iter_to_iter (
                  GTK_TREE_MODEL_FILTER (filter), &iter_filter, &iter);
            path = gtk_tree_model_get_path (filter, &iter_filter);
        }
    }
    if (path)
    {
        gtk_tree_view_expand_to_path (GTK_TREE_VIEW (view), path);
        gtk_tree_view_set_cursor (GTK_TREE_VIEW (view), path, NULL, FALSE);
        gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (view), path, NULL,
                                      TRUE, 0.5, 0.0);
        gtk_tree_path_free (path);
    } else {
        gtk_tree_view_expand_all (GTK_TREE_VIEW (view));
    }
}

static void
browse_button_clicked_cb (GtkWidget *widget, gpointer user_data)
{
    GtkTreeView *tree = user_data;
    gchar *file, *uri;
	GFile *gio_file, *tmp;
    GtkFileChooserDialog* dialog;
	GtkTreeModel* model;
    GtkTreeIter iter;
    gint result;
    
    g_return_if_fail (user_data != NULL && GTK_IS_TREE_VIEW (user_data));
	
	model = gtk_tree_view_get_model(tree);
	if (gtk_tree_model_get_iter_first(model, &iter))
	{
		gtk_tree_model_get(model, &iter, COLUMN_URI, &file, -1);
		uri = g_strdup(file);
	}
	else
		uri = g_strdup("");
	
    dialog = GTK_FILE_CHOOSER_DIALOG(gtk_file_chooser_dialog_new (_("Select sources..."),
				      GTK_WINDOW (gtk_widget_get_toplevel (widget)),
				      GTK_FILE_CHOOSER_ACTION_OPEN,
				      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				      NULL));
	/* FIXME: this is somewhat UGLY */
	gio_file = g_file_new_for_uri (uri);
	tmp = g_file_get_parent (gio_file);
    g_free (uri);
	uri = NULL;
	if (g_file_query_exists (tmp, NULL))
	{
	    uri = g_file_get_uri (tmp);
	}

    
    gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(dialog),
    	uri ? uri : g_object_get_data (G_OBJECT (widget), "root"));
    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER(dialog), TRUE);
    g_free (uri);

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

static void
on_row_changed(GtkTreeModel* model, GtkTreePath* path, GtkTreeIter* iter, gpointer data)
{
	GtkWidget* button = GTK_WIDGET(data);
	if (gtk_list_store_iter_is_valid(GTK_LIST_STORE(model), iter))
		gtk_widget_set_sensitive(button, TRUE);
	else
		gtk_widget_set_sensitive(button, FALSE);
}

gchar*
gbf_project_util_add_source (GbfProjectModel *model,
                             GtkWindow       *parent,
                             const gchar     *default_target,
                             const gchar     *default_group,
                             const gchar     *default_uri)
{
        GList* new_sources;
	gchar* uri = NULL;
	GList* uris = NULL;
        
        if (default_uri) {
            uri = g_strdup (default_uri);
            uris = g_list_append (NULL, uri);
        }
	new_sources = 
		gbf_project_util_add_source_multi (model, parent,
                                                   default_target,
                                                   default_group, uris);
	g_free (uri);
	
	if (new_sources && g_list_length (new_sources))
	{
		gchar* new_source = g_strdup (new_sources->data);
                g_list_foreach (new_sources, (GFunc) g_free, NULL);
		g_list_free (new_sources);
		return new_source;
	}
	else
		return NULL;
}

GList* 
gbf_project_util_add_source_multi (GbfProjectModel *model,
				   GtkWindow       *parent,
				   const gchar     *default_target,
				   const gchar     *default_group,
				   GList     *uris_to_add)
{
    GladeXML *gui;
    GtkWidget *dialog, *source_file_tree;
    GtkWidget *ok_button, *browse_button;
    GtkWidget *targets_view;
    gint response;
    GbfProject *project;
    gboolean finished = FALSE;
    gchar *project_root;
    GtkListStore* list;
    GtkCellRenderer* renderer;
    GtkTreeViewColumn* column_filename;
    GList* new_sources = NULL;
    GList* uri_node;
    
    g_return_val_if_fail (model != NULL, NULL);
    
    project = gbf_project_model_get_project (model);
    if (!project)
        return NULL;

    gui = load_interface ("add_source_dialog");
    g_return_val_if_fail (gui != NULL, NULL);
    
    /* get all needed widgets */
    dialog = glade_xml_get_widget (gui, "add_source_dialog");
    targets_view = glade_xml_get_widget (gui, "targets_view");
    source_file_tree = glade_xml_get_widget (gui, "source_file_tree");
    browse_button = glade_xml_get_widget (gui, "browse_button");
    ok_button = glade_xml_get_widget (gui, "ok_button");
    
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
    
    g_object_get (project, "project-dir", &project_root, NULL);
    g_object_set_data_full (G_OBJECT (browse_button), "root",
                            project_root, g_free);
    
    setup_targets_treeview (model, targets_view, default_target,
			    default_group);
    gtk_widget_show (targets_view);
    
    if (parent) {
        gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);
    }

    if (default_target)
        gtk_widget_grab_focus (source_file_tree);
    else
        gtk_widget_grab_focus (targets_view);
    
    /* execute dialog */
    while (!finished) {
        response = gtk_dialog_run (GTK_DIALOG (dialog));

        switch (response) {
            case GTK_RESPONSE_OK: 
            {
                GbfTreeData *data;
                gchar *target_id = NULL;
            
                data = gbf_project_view_find_selected (GBF_PROJECT_VIEW (targets_view),
                                                       GBF_TREE_NODE_TARGET);
		if (data) {
		    GtkTreeIter iter;
		    GString *err_mesg = g_string_new (NULL);
		    
		    target_id = g_strdup (data->id);
		    gbf_tree_data_free (data);
		    
		    if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list),
							&iter))
			break;
		    do
		    {
			GError *err = NULL;
			gchar* new_source;
			gchar* source_file;
			gtk_tree_model_get (GTK_TREE_MODEL(list), &iter,
					    COLUMN_URI, &source_file, -1);
			
			new_source = gbf_project_add_source (project,
							     target_id,
							     source_file,
							     &err);
			if (err) {
			    gchar *str = g_strdup_printf ("%s: %s\n",
							  source_file,
							  err->message);
			    g_string_append (err_mesg, str);
			    g_error_free (err);
			    g_free (str);
			}
			else
			    new_sources = g_list_append (new_sources,
							 new_source);
			
			g_free (source_file);
		    } while (gtk_tree_model_iter_next (GTK_TREE_MODEL(list),
						       &iter));
		    
                    g_free (target_id);
		    
		    if (err_mesg->str && strlen (err_mesg->str) > 0) {
			error_dialog (parent, _("Can not add source files"),
				      "%s", err_mesg->str);
		    } else {
			finished = TRUE;
		    }
		    g_string_free (err_mesg, TRUE);
                } else {
                    error_dialog (parent, _("Can not add source files"),
				  "%s", _("No target has been selected"));
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
