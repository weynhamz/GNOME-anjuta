/* Anjuta
 * Copyright (C) 2003 Naba Kumar
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
#include <ctype.h>

#include <gtk/gtkscrolledwindow.h>
#include <bonobo/bonobo-i18n.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkcellrendererpixbuf.h>
#include <gtk/gtkcellrenderertoggle.h>
#include <gtk/gtkstock.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libgnome/gnome-macros.h>
#include <libgnomeui/gnome-uidefs.h>
#include <glade/glade.h>

#include "windows-dialog.h"
#include "resources.h"

typedef struct
{
	gchar *name;
	gchar *cannonical_name;
	gchar *icon;
	GtkWindow *window;
	GtkWindow *parent;
	gboolean floating;
	gboolean remember;
	gint posx, posy, width, height;
	
} AnjutaWindowProperty;

struct _AnjutaWindowsDialogPrivate
{
	PropsID        props;
	GtkWidget     *treeview;
	GtkListStore  *store;
};

enum
{
	COLUMN_PIXBUF,
	COLUMN_FLOAT_TOGGLE,
	COLUMN_NAME,
	COLUMN_DATA,
	N_COLUMNS
};

static void anjuta_windows_dialog_class_init    (AnjutaWindowsDialogClass *class);
static void anjuta_windows_dialog_instance_init (AnjutaWindowsDialog      *dlg);

static gchar *
get_cannonical_name (const gchar *name)
{
	gchar *ret;
	gchar *idx;
	
	ret = g_strdup (name);
	idx = ret;
	while (*idx)
	{
		if (!isalnum(*idx))
			*idx = '.';
		*idx = tolower (*idx);
		idx++;
	}
	return ret;
}

#define GET_PROPERTY(var) \
	key = g_strconcat ("window.", p->cannonical_name, ".property.", #var, NULL); \
	p->var = prop_get_int (props, key, 0); \
	g_free (key);

static AnjutaWindowProperty *
anjuta_window_property_new (PropsID props, const gchar *name, const gchar *icon,
							GtkWindow *win, GtkWindow *parent)
{
	AnjutaWindowProperty *p;
	gchar *key;
	
	p = g_new0 (AnjutaWindowProperty, 1);
	p->name = g_strdup (name);
	p->cannonical_name = get_cannonical_name (name);
	p->icon = g_strdup (icon);
	p->window = win;
	p->parent = parent;
	g_object_ref (G_OBJECT (win));
	if (parent) g_object_ref (G_OBJECT (parent));

	GET_PROPERTY (floating);
	GET_PROPERTY (remember);
	GET_PROPERTY (posx);
	GET_PROPERTY (posy);
	GET_PROPERTY (width);
	GET_PROPERTY (height);
	return p;
}

static void
anjuta_window_property_destroy (AnjutaWindowProperty *p)
{
	g_free (p->name);
	g_free (p->cannonical_name);
	g_free (p->icon);
	g_object_unref (p->window);
	if (p->parent) g_object_unref (p->parent);
	g_free (p);
}

#define PUT_PROPERTY_INT(var) \
	key = g_strconcat ("window.", p->cannonical_name, ".property.", #var, NULL); \
	prop_set_int_with_key (props, key, p->var); \
	g_free (key);

#define PUT_PROPERTY_STR(var) \
	key = g_strconcat ("window.", p->cannonical_name, ".property.", #var, NULL); \
	prop_set_with_key (props, key, p->var); \
	g_free (key);

static void
anjuta_window_property_save (PropsID props, AnjutaWindowProperty *p)
{
	gchar *key;
	
	PUT_PROPERTY_INT (floating);
	PUT_PROPERTY_INT (remember);
	PUT_PROPERTY_INT (posx);
	PUT_PROPERTY_INT (posy);
	PUT_PROPERTY_INT (width);
	PUT_PROPERTY_INT (height);
}

static void
anjuta_window_property_apply (AnjutaWindowProperty *p)
{
	if (p->window && p->parent)
	{
		if (p->floating)
		{
			gtk_window_set_transient_for (GTK_WINDOW (p->window),
										  GTK_WINDOW (p->parent));
		}
		else
		{
			gtk_window_set_transient_for (GTK_WINDOW (p->window), NULL);
		}
	}
}

GNOME_CLASS_BOILERPLATE (AnjutaWindowsDialog, 
						 anjuta_windows_dialog,
						 GtkDialog, GTK_TYPE_DIALOG);

static void
anjuta_windows_dialog_dispose (GObject *obj)
{
	AnjutaWindowsDialog *dlg = ANJUTA_WINDOWS_DIALOG (obj);

	if (dlg->priv->store) {
		g_object_unref (dlg->priv->treeview);
		g_object_unref (dlg->priv->store);
		dlg->priv->store = NULL;
	}
	
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
anjuta_windows_dialog_finalize (GObject *obj)
{
	AnjutaWindowsDialog *dlg = ANJUTA_WINDOWS_DIALOG (obj);

	g_free (dlg->priv);

	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

#if 0
static void
anjuta_windows_dialog_response (GtkDialog *dialog, int response_id)
{
	g_return_if_fail (response_id == 0);
	
	gtk_widget_hide (GTK_WIDGET (dialog));
}
#endif

static void
anjuta_windows_dialog_close (GtkDialog *dialog)
{
	gtk_widget_hide (GTK_WIDGET (dialog));
}

static void
anjuta_windows_dialog_class_init (AnjutaWindowsDialogClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (class);

	parent_class = g_type_class_peek_parent (class);
	
	object_class->dispose = anjuta_windows_dialog_dispose;
	object_class->finalize = anjuta_windows_dialog_finalize;

	dialog_class->response = anjuta_windows_dialog_close;
	dialog_class->close = anjuta_windows_dialog_close;
}

static gboolean
on_delete_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gtk_widget_hide (widget);
	return TRUE;
}

static void
anjuta_windows_dialog_instance_init (AnjutaWindowsDialog *dlg)
{
	GtkTreeSelection *selection;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkWidget *scrollwindow;
	
	dlg->priv = g_new0 (AnjutaWindowsDialogPrivate, 1);
	
	gtk_dialog_add_button (GTK_DIALOG (dlg), GTK_STOCK_CLOSE,
						   GTK_RESPONSE_CLOSE);
	gtk_window_set_default_size (GTK_WINDOW (dlg), 400, 300);
	g_signal_connect (G_OBJECT (dlg), "delete_event",
					  G_CALLBACK (on_delete_event), NULL);
	
	scrollwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrollwindow);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrollwindow),
										 GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwindow),
									GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dlg)->vbox), scrollwindow);
	
	dlg->priv->treeview = gtk_tree_view_new ();
	gtk_widget_show (dlg->priv->treeview);
	gtk_tree_view_set_reorderable (GTK_TREE_VIEW (dlg->priv->treeview), FALSE);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (dlg->priv->treeview), TRUE);
	g_object_ref (dlg->priv->treeview);
	gtk_container_add (GTK_CONTAINER (scrollwindow), dlg->priv->treeview);
	

	dlg->priv->store = gtk_list_store_new (N_COLUMNS, 
										   GDK_TYPE_PIXBUF,
										   G_TYPE_BOOLEAN,
										   G_TYPE_STRING,
										   G_TYPE_POINTER);

	gtk_tree_view_set_model (GTK_TREE_VIEW (dlg->priv->treeview),
 							 GTK_TREE_MODEL (dlg->priv->store));

	renderer = gtk_cell_renderer_pixbuf_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Icon"),
													   renderer,
													   "pixbuf",
													   COLUMN_PIXBUF,
													   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (dlg->priv->treeview),
								 column);
	renderer = gtk_cell_renderer_toggle_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Float"),
													   renderer,
													   "active",
													   COLUMN_FLOAT_TOGGLE,
													   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (dlg->priv->treeview),
								 column);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Name"),
													   renderer,
													   "text",
													   COLUMN_NAME,
													   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (dlg->priv->treeview),
								 column);
	// g_object_unref (gxml);
}

static gboolean
delete_event_cb (AnjutaWindowsDialog *dlg, gpointer data)
{
	gtk_widget_hide (GTK_WIDGET (dlg));
	return FALSE;
}

GtkWidget *
anjuta_windows_dialog_new (PropsID props)
{
	GtkWidget *w;
	w = gtk_widget_new (ANJUTA_TYPE_WINDOWS_DIALOG, 
						"title", _("Anjuta Windows Dialog"),
						NULL);
	ANJUTA_WINDOWS_DIALOG (w)->priv->props = props;
	return w;
}

void
anjuta_windows_register_window (AnjutaWindowsDialog *dlg, GtkWindow *win,
								const gchar *name, const gchar *icon,
								GtkWindow *parent)
{
	GtkTreeIter iter;
	AnjutaWindowProperty *p;
	const gchar *icon_name;
	GdkPixbuf *pixbuf;
	
	g_return_if_fail (ANJUTA_IS_WINDOWS_DIALOG (dlg));
	g_return_if_fail (GTK_IS_WINDOW (win));
	//g_return_if_fail (GTK_IS_WINDOW (parent));
	g_return_if_fail (name != NULL);
	
	if (icon)
		icon_name = icon;
	else
		icon_name = "anjuta_icon.png";
	
	pixbuf = gdk_pixbuf_scale_simple (anjuta_res_get_pixbuf (icon), 16, 16,
									  GDK_INTERP_BILINEAR);
	p = anjuta_window_property_new (dlg->priv->props, name, icon_name,
									GTK_WINDOW (win), parent);
	anjuta_window_property_apply (p);
	gtk_list_store_append (GTK_LIST_STORE (dlg->priv->store), &iter);
	gtk_list_store_set (GTK_LIST_STORE (dlg->priv->store), &iter,
						COLUMN_PIXBUF, pixbuf,
						COLUMN_FLOAT_TOGGLE, p->floating,
						COLUMN_NAME, name,
						COLUMN_DATA, p, -1);
	g_object_unref (pixbuf);
}

void
anjuta_windows_unregister_window (AnjutaWindowsDialog* dlg,
								  GtkWidget *win)
{
}
