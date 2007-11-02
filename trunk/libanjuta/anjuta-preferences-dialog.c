/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Anjuta
 * Copyright (C) 2002 Dave Camp
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/**
 * SECTION:anjuta-preferences-dialog
 * @short_description: Preferences dialog
 * @see_also: #AnjutaPreferences
 * @stability: Unstable
 * @include: libanjuta/anjuta-preferences-dialog.h
 * 
 * Plugins can added preferences page with anjuta_preferences_dialog_add_page().
 * However, read #AnjutaPreferences for adding proper preferences pages.
 */

#include <config.h>

#include <string.h>
#include <glib/gi18n.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkstock.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtktreeselection.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <libgnome/gnome-macros.h>

#include <cell-renderer-captioned-image.h>
#include <libanjuta/anjuta-preferences-dialog.h>
struct _AnjutaPreferencesDialogPrivate {
	GtkWidget *treeview;
	GtkListStore *store;

	GtkWidget *notebook;
};

enum {
	COL_NAME,
	COL_TITLE,
	COL_PIXBUF,
	COL_WIDGET,
	LAST_COL
};

static void anjuta_preferences_dialog_class_init    (AnjutaPreferencesDialogClass *class);
static void anjuta_preferences_dialog_init (AnjutaPreferencesDialog      *dlg);

G_DEFINE_TYPE (AnjutaPreferencesDialog, anjuta_preferences_dialog,
			   GTK_TYPE_DIALOG)

static void
anjuta_preferences_dialog_finalize (GObject *obj)
{
	AnjutaPreferencesDialog *dlg = ANJUTA_PREFERENCES_DIALOG (obj);	

	if (dlg->priv->store) {
		g_object_unref (dlg->priv->store);
		dlg->priv->store = NULL;
	}
	
	g_free (dlg->priv);
	
	((GObjectClass *) anjuta_preferences_dialog_parent_class)->finalize (obj);
}

static void
anjuta_preferences_dialog_class_init (AnjutaPreferencesDialogClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = anjuta_preferences_dialog_finalize;
}

static void
add_category_columns (AnjutaPreferencesDialog *dlg)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	
	renderer = anjuta_cell_renderer_captioned_image_new ();
	g_object_ref_sink (renderer);
	column = gtk_tree_view_column_new_with_attributes (_("Category"),
							   renderer,
							   "text",
							   COL_TITLE,
							   "pixbuf",
							   COL_PIXBUF,
							   NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (dlg->priv->treeview),
				     column);
}

static void
selection_changed_cb (GtkTreeSelection *selection,
		      AnjutaPreferencesDialog *dlg)
{
	GtkTreeIter iter;

	if (gtk_tree_selection_get_selected (selection, NULL, &iter)) {
		GtkWidget *widget;

		gtk_tree_model_get (GTK_TREE_MODEL (dlg->priv->store), &iter,
				    COL_WIDGET, &widget, -1);
		
		gtk_notebook_set_current_page 
			(GTK_NOTEBOOK (dlg->priv->notebook),
			 gtk_notebook_page_num (GTK_NOTEBOOK (dlg->priv->notebook),
						widget));
		
	}
}

static gint
compare_pref_page_func (GtkTreeModel *model,
						GtkTreeIter *a, GtkTreeIter *b, gpointer user_data)
{
	gint val;
	gchar *name1, *name2;
	
	gtk_tree_model_get (model, a, COL_TITLE, &name1, -1);
	gtk_tree_model_get (model, b, COL_TITLE, &name2, -1);
	
	/* FIXME: Make the general page first */
	if (strcmp (name1, _("General")) == 0)
		return -1;
		
	if (strcmp (name2, _("General")) == 0)
		return 1;
		
	val = strcmp (name1, name2);
	g_free (name1);
	g_free (name2);
	
	return val;
}

static void
anjuta_preferences_dialog_init (AnjutaPreferencesDialog *dlg)
{
	GtkWidget *hbox;
	GtkWidget *scrolled_window;
	GtkTreeSelection *selection;
	GtkTreeSortable *sortable;
	
	dlg->priv = g_new0 (AnjutaPreferencesDialogPrivate, 1);

	hbox = gtk_hbox_new (FALSE, 0);	
	
	dlg->priv->treeview = gtk_tree_view_new ();
	gtk_widget_show (dlg->priv->treeview);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (dlg->priv->treeview),
					   FALSE);
	dlg->priv->store = gtk_list_store_new (LAST_COL, 
					       G_TYPE_STRING,
					       G_TYPE_STRING,
					       GDK_TYPE_PIXBUF,
					       GTK_TYPE_WIDGET,
					       G_TYPE_INT);
	sortable = GTK_TREE_SORTABLE (dlg->priv->store);
	gtk_tree_sortable_set_sort_column_id (sortable, COL_TITLE,
										  GTK_SORT_ASCENDING);
	gtk_tree_sortable_set_sort_func (sortable, COL_TITLE,
									 compare_pref_page_func,
									 NULL, NULL);

	gtk_tree_view_set_model (GTK_TREE_VIEW (dlg->priv->treeview),
				 GTK_TREE_MODEL (dlg->priv->store));

	add_category_columns (dlg);

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	gtk_scrolled_window_set_shadow_type 
		(GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);
	
	gtk_widget_show (scrolled_window);
	gtk_container_add (GTK_CONTAINER (scrolled_window), 
			   dlg->priv->treeview);
	gtk_box_pack_start (GTK_BOX (hbox), scrolled_window,
			    FALSE, FALSE, 0);

	dlg->priv->notebook = gtk_notebook_new ();
	gtk_widget_show (dlg->priv->notebook);
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (dlg->priv->notebook), FALSE);
	gtk_notebook_set_show_border (GTK_NOTEBOOK (dlg->priv->notebook), 
				      FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (dlg->priv->notebook),
					8);
	
	gtk_box_pack_start (GTK_BOX (hbox), dlg->priv->notebook,
			    TRUE, TRUE, 0);


	selection = gtk_tree_view_get_selection 
		(GTK_TREE_VIEW (dlg->priv->treeview));

	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

	g_signal_connect (selection, "changed", 
			  G_CALLBACK (selection_changed_cb), dlg);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), hbox,
			    TRUE, TRUE, 0);
	
	gtk_dialog_add_button (GTK_DIALOG (dlg), GTK_STOCK_CLOSE, -7);
	
	gtk_widget_show (hbox);
}

/**
 * anjuta_preferences_dialog_new:
 * 
 * Creates a new #AnjutaPreferencesDialog object.
 *
 * Return value: a new #AnjutaPreferencesDialog object.
 */
GtkWidget *
anjuta_preferences_dialog_new (void)
{
	return g_object_new (ANJUTA_TYPE_PREFERENCES_DIALOG, 
				 "title", _("Anjuta Preferences"),
				 NULL);
}

/**
 * anjuta_preferences_dialog_add_page:
 * @dlg: A #AnjutaPreferencesDialog object.
 * @name: Name of the preferences page.
 * @icon: Icon file name.
 * @page: page widget.
 *
 * Adds a widget page in preferences dialog. Name and icon appears
 * on the left icon list where differnt pages are selected.
 */
void
anjuta_preferences_dialog_add_page (AnjutaPreferencesDialog *dlg,
									const gchar *name,
									const gchar *title,
									GdkPixbuf *icon,
									GtkWidget *page)
{
	GtkTreeIter iter;

	gtk_widget_show (page);
	
	gtk_notebook_append_page (GTK_NOTEBOOK (dlg->priv->notebook), page, NULL);

	gtk_list_store_append (dlg->priv->store, &iter);
	
	gtk_list_store_set (dlg->priv->store, &iter,
			    COL_NAME, name,
			    COL_TITLE, title,
			    COL_PIXBUF, icon,
			    COL_WIDGET, page,
			    -1);
}

/**
 * anjuta_preferences_dialog_remove_page:
 * @dlg: A #AnjutaPreferencesDialog object.g_signal_handler
 * @name: Name of the preferences page.
 *
 * Removes a preferences page.
 */
void
anjuta_preferences_dialog_remove_page (AnjutaPreferencesDialog *dlg,
									   const char *title)
{
	GtkTreeModel* model = GTK_TREE_MODEL(dlg->priv->store);
	GtkTreeIter iter;
	GtkWidget *page;
	
	if (gtk_tree_model_get_iter_first(model, &iter))
	{
		do
		{
			gchar* page_title;
			GObject* page_widget;

			gtk_tree_model_get(model, &iter,
					COL_TITLE, &page_title,
					COL_WIDGET, &page_widget,
					-1);

			if (g_str_equal(page_title, title))
			{
				int page_num;

				page_num = gtk_notebook_page_num (
						GTK_NOTEBOOK(dlg->priv->notebook),
						GTK_WIDGET (page_widget));
				
				page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (dlg->priv->notebook),
												  page_num);

				gtk_notebook_remove_page(
						GTK_NOTEBOOK(dlg->priv->notebook), page_num);
				
				gtk_widget_destroy (page);

				gtk_list_store_remove(dlg->priv->store, &iter);
				return;
			}
		}
		while (gtk_tree_model_iter_next(model, &iter));
	}
	g_warning("Could not find page to remove");
}
