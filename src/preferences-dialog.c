/* Anjuta
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

#include <config.h>
#include "preferences-dialog.h"

#include <bonobo/bonobo-i18n.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkstock.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtktreeselection.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libgnome/gnome-macros.h>
#include <libgnomeui/gnome-uidefs.h>

#include <cell-renderer-captioned-image.h>

struct _AnjutaPreferencesDialogPrivate {
	GtkWidget *treeview;
	GtkListStore *store;

	GtkWidget *notebook;
};

enum {
	COL_NAME,
	COL_PIXBUF,
	COL_WIDGET,
	LAST_COL
};

static void anjuta_preferences_dialog_class_init    (AnjutaPreferencesDialogClass *class);
static void anjuta_preferences_dialog_instance_init (AnjutaPreferencesDialog      *dlg);

GNOME_CLASS_BOILERPLATE (AnjutaPreferencesDialog, 
			 anjuta_preferences_dialog,
			 GtkDialog, GTK_TYPE_DIALOG);

static void
anjuta_preferences_dialog_dispose (GObject *obj)
{
	AnjutaPreferencesDialog *dlg = ANJUTA_PREFERENCES_DIALOG (obj);

	if (dlg->priv->store) {
		g_object_unref (dlg->priv->store);
		dlg->priv->store = NULL;
	}
	
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
anjuta_preferences_dialog_finalize (GObject *obj)
{
	AnjutaPreferencesDialog *dlg = ANJUTA_PREFERENCES_DIALOG (obj);	

	g_free (dlg->priv);

	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

#if 0
static void
anjuta_preferences_dialog_response (GtkDialog *dialog, int response_id)
{
	g_return_if_fail (response_id == 0);
	
	gtk_widget_hide (GTK_WIDGET (dialog));
}
#endif

static void
anjuta_preferences_dialog_close (GtkDialog *dialog)
{
	gtk_widget_hide (GTK_WIDGET (dialog));
}

static void
anjuta_preferences_dialog_class_init (AnjutaPreferencesDialogClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (class);

	parent_class = g_type_class_peek_parent (class);
	
	object_class->dispose = anjuta_preferences_dialog_dispose;
	object_class->finalize = anjuta_preferences_dialog_finalize;

	// dialog_class->response = anjuta_preferences_dialog_response;
	dialog_class->close = anjuta_preferences_dialog_close;
}

static gboolean
delete_event_cb (AnjutaPreferencesDialog *dlg, gpointer data)
{
	gtk_widget_hide (GTK_WIDGET (dlg));
	return FALSE;
}

static void
add_category_columns (AnjutaPreferencesDialog *dlg)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	
	renderer = anjuta_cell_renderer_captioned_image_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Category"),
							   renderer,
							   "text",
							   COL_NAME,
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

static void
anjuta_preferences_dialog_instance_init (AnjutaPreferencesDialog *dlg)
{
	GtkWidget *hbox;
	GtkWidget *scrolled_window;
	GtkTreeSelection *selection;
	
	dlg->priv = g_new0 (AnjutaPreferencesDialogPrivate, 1);

	hbox = gtk_hbox_new (FALSE, 0);	
	
	dlg->priv->treeview = gtk_tree_view_new ();
	gtk_widget_show (dlg->priv->treeview);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (dlg->priv->treeview),
					   FALSE);

	dlg->priv->store = gtk_list_store_new (LAST_COL, 
					       G_TYPE_STRING,
					       GDK_TYPE_PIXBUF,
					       GTK_TYPE_WIDGET);

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
					GNOME_PAD_SMALL);
	
	gtk_box_pack_start (GTK_BOX (hbox), dlg->priv->notebook,
			    TRUE, TRUE, 0);


	selection = gtk_tree_view_get_selection 
		(GTK_TREE_VIEW (dlg->priv->treeview));

	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

	g_signal_connect (selection, "changed", 
			  G_CALLBACK (selection_changed_cb), dlg);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), hbox,
			    TRUE, TRUE, 0);
	
	gtk_widget_show (hbox);
}

GtkWidget *
anjuta_preferences_dialog_new (void)
{
	return gtk_widget_new (ANJUTA_TYPE_PREFERENCES_DIALOG, 
				 "title", _("Anjuta Preferences Dialog"),
				 NULL);
}

void
anjuta_preferences_dialog_add_page (AnjutaPreferencesDialog *dlg,
									const char *name,
									GdkPixbuf *icon,
									GtkWidget *page)
{
	GtkTreeIter iter;
	
	gtk_widget_show (page);
	
	gtk_notebook_append_page (GTK_NOTEBOOK (dlg->priv->notebook), 
				  page, NULL);

	gtk_list_store_append (dlg->priv->store, &iter);
	
	gtk_list_store_set (dlg->priv->store, &iter,
			    COL_NAME, name,
			    COL_PIXBUF, icon,
			    COL_WIDGET, page, 
			    -1);
}

void
anjuta_preferences_dialog_remove_page (AnjutaPreferencesDialog *dlg,
									   const char *name)
{
	/* FIXME */
}
