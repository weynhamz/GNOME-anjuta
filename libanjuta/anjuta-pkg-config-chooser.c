/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * pkg-config-chooser
 * Copyright (C) Johannes Schmid 2010 <jhs@gnome.org>
 * 
 * pkg-config-chooser is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * pkg-config-chooser is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "anjuta-pkg-config-chooser.h"
#include <glib/gi18n.h>
#include <libanjuta/anjuta-launcher.h>

#define PKG_CONFIG_LIST_ALL "pkg-config --list-all"

enum
{
	PACKAGE_ACTIVATED,
	PACKAGE_DEACTIVATED,
	LAST_SIGNAL
};

enum
{
	COLUMN_SELECTED,
	COLUMN_NAME,
	COLUMN_DESCRIPTION,
	N_COLUMNS
};

struct _AnjutaPkgConfigChooserPrivate
{
	AnjutaLauncher* launcher;
	GtkTreeModel* model;
	GtkTreeModelFilter* filter_model;
	GtkTreeModelSort* sort_model;

	gboolean selected_only;
	gboolean scanning;

	GList* selected_cache;
};

static guint pkg_config_chooser_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (AnjutaPkgConfigChooser, anjuta_pkg_config_chooser, GTK_TYPE_TREE_VIEW);

static gboolean
filter_visible_func (GtkTreeModel* model,
                     GtkTreeIter* iter,
                     gpointer data)
{
	AnjutaPkgConfigChooser* chooser = data;
	
	if (!chooser->priv->selected_only)
		return TRUE;
	else
	{
		gboolean show;
		gtk_tree_model_get (model, iter,
		                    COLUMN_SELECTED, &show, -1);
		return show;
	}
}

static void
on_listall_output (AnjutaLauncher * launcher,
                   AnjutaLauncherOutputType output_type,
                   const gchar * chars, gpointer user_data)
{
	gchar **lines;
	const gchar *curr_line;
	gint i = 0;
	AnjutaPkgConfigChooser* chooser;
	GtkListStore* store;

	if (output_type == ANJUTA_LAUNCHER_OUTPUT_STDERR)
	{
		/* no way. We don't like errors on stderr... */
		return;
	}
	
	chooser = ANJUTA_PKG_CONFIG_CHOOSER (user_data);
	
	store = GTK_LIST_STORE (chooser->priv->model);
	lines = g_strsplit (chars, "\n", -1);
	
	while ((curr_line = lines[i++]) != NULL)
	{
		gchar **pkgs;
		GtkTreeIter iter;
		
		pkgs = g_strsplit (curr_line, " ", 2);
		
		/* just take the first token as it's the package-name */
		if (pkgs == NULL)
			return;		
		
		if (pkgs[0] == NULL || pkgs[1] == NULL) {
			g_strfreev (pkgs);
			continue;
		}	
		
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
		                    COLUMN_SELECTED, FALSE,
		                    COLUMN_NAME, pkgs[0],
		                    COLUMN_DESCRIPTION, g_strstrip(pkgs[1]), -1);
		g_strfreev (pkgs);
	}
	g_strfreev (lines);	
}

static void
on_listall_exit (AnjutaLauncher * launcher, int child_pid,
				   int exit_status, gulong time_taken_in_seconds,
				   gpointer user_data)
{
	AnjutaPkgConfigChooser* chooser = ANJUTA_PKG_CONFIG_CHOOSER (user_data);

	g_signal_handlers_disconnect_by_func (launcher, on_listall_exit,
										  user_data);
	chooser->priv->scanning = FALSE;

	anjuta_pkg_config_chooser_set_active_packages (chooser, chooser->priv->selected_cache);
	g_list_foreach (chooser->priv->selected_cache, (GFunc) g_free, NULL);
	
	g_object_unref (launcher);
}

static void
on_package_toggled (GtkCellRenderer* renderer,
                    const gchar* path,
                    AnjutaPkgConfigChooser* chooser)
{
	GtkTreeIter sort_iter;
	GtkTreeIter filter_iter;
	GtkTreeIter iter;
	gboolean active;
	gchar* package;

	GtkTreePath* tree_path = gtk_tree_path_new_from_string (path);

	gtk_tree_model_get_iter (GTK_TREE_MODEL (chooser->priv->sort_model),
	                         &sort_iter, tree_path);
	gtk_tree_model_sort_convert_iter_to_child_iter (chooser->priv->sort_model,
	                                                &filter_iter, &sort_iter);
	gtk_tree_model_filter_convert_iter_to_child_iter (chooser->priv->filter_model,
	                                                  &iter, &filter_iter);
	g_object_get (renderer, "active", &active, NULL);

	active = !active;
	
	gtk_list_store_set (GTK_LIST_STORE (chooser->priv->model),
	                    &iter, COLUMN_SELECTED, active, -1);
	gtk_tree_model_get (chooser->priv->model, &iter,
	                    COLUMN_NAME, &package, -1);
	
	if (active)
		g_signal_emit_by_name (chooser, "package-activated", package, NULL);
	else
		g_signal_emit_by_name (chooser, "package-deactivated", package, NULL);
}

static void
anjuta_pkg_config_chooser_init (AnjutaPkgConfigChooser *chooser)
{
	GtkTreeViewColumn* column;
	GtkCellRenderer* renderer;
	
	chooser->priv = G_TYPE_INSTANCE_GET_PRIVATE (chooser, ANJUTA_TYPE_PKG_CONFIG_CHOOSER,
	                                             AnjutaPkgConfigChooserPrivate);

	/* Create model */
	chooser->priv->model = GTK_TREE_MODEL (gtk_list_store_new (N_COLUMNS,
	                                                           G_TYPE_BOOLEAN,
	                                                           G_TYPE_STRING,
	                                                           G_TYPE_STRING));
	chooser->priv->filter_model = GTK_TREE_MODEL_FILTER (gtk_tree_model_filter_new (chooser->priv->model,
	                                                                                NULL));
	gtk_tree_model_filter_set_visible_func (chooser->priv->filter_model,
	                                        filter_visible_func,
	                                        chooser, NULL);

	chooser->priv->sort_model = 
		GTK_TREE_MODEL_SORT (gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL(chooser->priv->filter_model)));
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (chooser->priv->sort_model),
	                                      COLUMN_NAME, GTK_SORT_ASCENDING);
	
	gtk_tree_view_set_model (GTK_TREE_VIEW (chooser), 
	                         GTK_TREE_MODEL (chooser->priv->sort_model));

	/* Create columns */
	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (renderer, "toggled", G_CALLBACK(on_package_toggled), chooser);
	column = gtk_tree_view_column_new_with_attributes ("",
	                                                   renderer,
	                                                   "active", COLUMN_SELECTED,
	                                                   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (chooser), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("",
	                                                   renderer,
	                                                   "text", COLUMN_NAME,
	                                                   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (chooser), column);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("",
	                                                   renderer,
	                                                   "text", COLUMN_DESCRIPTION,
	                                                   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (chooser), column);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (chooser), FALSE);
	
	/* Create launcher */
	chooser->priv->scanning = TRUE;
	chooser->priv->launcher = anjuta_launcher_new ();
	anjuta_launcher_set_check_passwd_prompt (chooser->priv->launcher,
	                                         FALSE);

	g_signal_connect (G_OBJECT (chooser->priv->launcher), "child-exited",
	                  G_CALLBACK (on_listall_exit), chooser);	
	
	anjuta_launcher_execute (chooser->priv->launcher,
	                         PKG_CONFIG_LIST_ALL, on_listall_output,
	                         chooser);
}

static void
anjuta_pkg_config_chooser_finalize (GObject *object)
{

	G_OBJECT_CLASS (anjuta_pkg_config_chooser_parent_class)->finalize (object);
}

static void
anjuta_pkg_config_chooser_class_init (AnjutaPkgConfigChooserClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = anjuta_pkg_config_chooser_finalize;

	/**
	 * AnjutaPkgConfigChooser::package-activated:
	 * @widget: the AnjutaPkgConfigChooser that received the signal
	 * @package: Name of the package that was activated
	 *
	 * The ::package-activated signal is emitted when a package is activated in the list
	 */
	pkg_config_chooser_signals[PACKAGE_ACTIVATED] =
		g_signal_new ("package-activated",
		              G_OBJECT_CLASS_TYPE (klass),
		              0,
		              G_STRUCT_OFFSET (AnjutaPkgConfigChooserClass, package_activated),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__STRING,
		              G_TYPE_NONE, 1,
		              G_TYPE_STRING);
	/**
	 * AnjutaPkgConfigChooser::package-deactivated:
	 * @widget: the AnjutaPkgConfigChooser that received the signal
	 * @package: Name of the package that was deactivated
	 *
	 * The ::package-activated signal is emitted when a package is deactivated in the list
	 */
	pkg_config_chooser_signals[PACKAGE_DEACTIVATED] =
		g_signal_new ("package-deactivated",
		              G_OBJECT_CLASS_TYPE (klass),
		              0,
		              G_STRUCT_OFFSET (AnjutaPkgConfigChooserClass, package_deactivated),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__STRING,
		              G_TYPE_NONE, 1,
		              G_TYPE_STRING);
	
	g_type_class_add_private (klass, sizeof(AnjutaPkgConfigChooserPrivate));
}

/*
 * anjuta_pkg_config_chooser_new:
 * 
 * Returns: A new AnjutaPkgConfigChooser widget
 */
GtkWidget*
anjuta_pkg_config_chooser_new (void)
{
	return GTK_WIDGET (g_object_new (ANJUTA_TYPE_PKG_CONFIG_CHOOSER, NULL));
}

/*
 * anjuta_pkg_config_chooser_get_active_packages:
 * @chooser: A AnjutaPkgConfigChooser
 *
 * Return value: (element-type utf8) (transfer full): 
 * List of packages that are activated
 */
GList*
anjuta_pkg_config_chooser_get_active_packages (AnjutaPkgConfigChooser* chooser)
{
	GList* packages = NULL;
	GtkTreeIter iter;

	g_return_val_if_fail (ANJUTA_IS_PKG_CONFIG_CHOOSER (chooser), NULL);
	
	if (gtk_tree_model_get_iter_first (chooser->priv->model, &iter))
	{
		do
		{
			gchar* model_pkg;
			gboolean selected;
			gtk_tree_model_get (chooser->priv->model, &iter,
			                    COLUMN_NAME, &model_pkg,
			                    COLUMN_SELECTED, &selected, -1);
			if (selected)
			{
				packages = g_list_append (packages, model_pkg);
			}
		}
		while (gtk_tree_model_iter_next (chooser->priv->model,
		                                 &iter));
	}
	return packages;
}

/*
 * anjuta_pkg_config_chooser_get_active_packages:
 * @chooser: A AnjutaPkgConfigChooser
 * @packages: (element-type utf8) (transfer full): List of packages to be activated in the list
 *
 */
void
anjuta_pkg_config_chooser_set_active_packages (AnjutaPkgConfigChooser* chooser, GList* packages)
{
	GList* pkg;

	g_return_if_fail (ANJUTA_IS_PKG_CONFIG_CHOOSER (chooser));
	
	for (pkg = packages; pkg != NULL; pkg = g_list_next (pkg))
	{
		GtkTreeIter iter;
		if (chooser->priv->scanning)
		{
			chooser->priv->selected_cache = g_list_append (chooser->priv->selected_cache,
			                                               g_strdup(pkg->data));
		}
		else if (gtk_tree_model_get_iter_first (chooser->priv->model, &iter))
		{
			do
			{
				gchar* model_pkg;
				gtk_tree_model_get (chooser->priv->model, &iter,
				                    COLUMN_NAME, &model_pkg, -1);
				if (g_str_equal (model_pkg, pkg->data))
				{
					gtk_list_store_set (GTK_LIST_STORE (chooser->priv->model), &iter,
					                    COLUMN_SELECTED, TRUE, -1);
				}
				g_free (model_pkg);
			}
			while (gtk_tree_model_iter_next (chooser->priv->model,
			                                 &iter));
		}
	}
}

/*
 * anjuta_pkg_config_chooser_show_active_only:
 * @chooser: A AnjutaPkgConfigChooser
 * @show_selected: whether to show only activated packages
 *
 * Show activated packages only, this is mainly useful when the tree is set
 * insensitive but the user should be able to see which packages have been activated
 */
void
anjuta_pkg_config_chooser_show_active_only (AnjutaPkgConfigChooser* chooser, gboolean show_selected)
{
	g_return_if_fail (ANJUTA_IS_PKG_CONFIG_CHOOSER (chooser));
	
	chooser->priv->selected_only = show_selected;
	gtk_tree_model_filter_refilter (chooser->priv->filter_model);
}

/*
 * anjuta_pkg_config_chooser_show_active_column:
 * @chooser: A AnjutaPkgConfigChooser
 * @show_column: whether the active column should be shown
 *
 * Can be used to hide the active column in situation where you are more interested
 * in the selection then in the activated packages.
 */
void 
anjuta_pkg_config_chooser_show_active_column (AnjutaPkgConfigChooser* chooser, gboolean show_column)
{
	GtkTreeViewColumn* column;

	g_return_if_fail (ANJUTA_IS_PKG_CONFIG_CHOOSER (chooser));
	
	column = gtk_tree_view_get_column (GTK_TREE_VIEW (chooser), COLUMN_NAME);
	gtk_tree_view_column_set_visible (column, show_column);
}

/*
 * anjuta_pkg_config_chooser_get_selected_package:
 * @chooser: A AnjutaPkgConfigChooser
 *
 * Return value: the currently selected packages in the list
 *
 */
gchar* 
anjuta_pkg_config_chooser_get_selected_package (AnjutaPkgConfigChooser* chooser)
{
	GtkTreeIter iter;
	GtkTreeModel* model;
	GtkTreeSelection* selection;

	g_return_val_if_fail (ANJUTA_IS_PKG_CONFIG_CHOOSER (chooser), NULL);
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (chooser));

	if (gtk_tree_selection_get_selected (selection, &model, &iter))
	{
		gchar* package;
		gtk_tree_model_get (model, &iter, COLUMN_NAME, &package, NULL);

		return package;
	}
	return NULL;
}