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

#include <bonobo/bonobo-i18n.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkcellrendererpixbuf.h>
#include <gtk/gtkcellrenderertoggle.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libgnome/gnome-macros.h>
#include <libgnomeui/gnome-uidefs.h>
#include <glade/glade.h>

#include "windows-dialog.h"

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
	COLUMN_NAME,
	COLUMN_FLOAT_TOGGLE,
	COLUMN_REMEMBER_TOGGLE,
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
	g_object_ref (G_OBJECT (parent));

	GET_PROPERTY (floating);
	GET_PROPERTY (remember);
	GET_PROPERTY (posx);
	GET_PROPERTY (posy);
	GET_PROPERTY (width);
	GET_PROPERTY (height);
}

static void
anjuta_window_property_destroy (AnjutaWindowProperty *p)
{
	g_free (p->name);
	g_free (p->cannonical_name);
	g_free (p->icon);
	g_object_unref (p->window);
	g_object_unref (p->parent);
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

GNOME_CLASS_BOILERPLATE (AnjutaWindowsDialog, 
						 anjuta_windows_dialog,
						 GtkDialog, GTK_TYPE_DIALOG);

static void
anjuta_windows_dialog_dispose (GObject *obj)
{
	AnjutaWindowsDialog *dlg = ANJUTA_WINDOWS_DIALOG (obj);

	if (dlg->priv->store) {
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

	// dialog_class->response = anjuta_windows_dialog_response;
	dialog_class->close = anjuta_windows_dialog_close;
}

static void
anjuta_windows_dialog_instance_init (AnjutaWindowsDialog *dlg)
{
	GtkTreeSelection *selection;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GladeXML *gxml;
	
	dlg->priv = g_new0 (AnjutaWindowsDialogPrivate, 1);

	gxml = glade_xml_new (PACKAGE_DATA_DIR"/glade/anjuta.glade",
						  "winodws_dialogs_dialog", NULL);
	
	dlg->priv->treeview = glade_xml_get_widget (gxml, "treeview");

	dlg->priv->store = gtk_list_store_new (N_COLUMNS, 
										   GDK_TYPE_PIXBUF,
										   G_TYPE_STRING,
										   G_TYPE_BOOLEAN,
										   G_TYPE_BOOLEAN,
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
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Name"),
													   renderer,
													   "text",
													   COLUMN_NAME,
													   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (dlg->priv->treeview),
								 column);
	renderer = gtk_cell_renderer_toggle_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Floating"),
													   renderer,
													   "active",
													   COLUMN_FLOAT_TOGGLE,
													   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (dlg->priv->treeview),
								 column);
	renderer = gtk_cell_renderer_toggle_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Remember"),
													   renderer,
													   "active",
													   COLUMN_REMEMBER_TOGGLE,
													   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (dlg->priv->treeview),
								 column);
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
}

void
anjuta_windows_register_window (AnjutaWindowsDialog *dlg, GtkWindow *win,
								const gchar *name, const gchar *icon,
								GtkWindow *parent)
{
	AnjutaWindowProperty *p;
	const gchar *icon_name;
	
	g_return_if_fail (ANJUTA_IS_WINDOWS_DIALOG (dlg));
	g_return_if_fail (GTK_IS_WINDOW (win));
	g_return_if_fail (GTK_IS_WINDOW (parent));
	g_return_if_fail (name != NULL);
	
	if (icon)
		icon_name = icon;
	else
		icon_name = "anjuta_icon.png";
	
	p = anjuta_window_property_new (dlg->priv->props, name, icon_name,
									GTK_WINDOW (win), GTK_WINDOW (parent));
}

void
anjuta_windows_unregister_window (AnjutaWindowsDialog* dlg,
								  GtkWidget *win)
{
}
