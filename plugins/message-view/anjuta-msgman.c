/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*  anjuta-msgman.c (c) 2004 Johannes Schmid
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <gtk/gtknotebook.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/resources.h>
#include "anjuta-msgman.h"
#include "message-view.h"

struct _AnjutaMsgmanPriv
{
	AnjutaPreferences *preferences;
	MessageView *current_view;
	GList *views;
};

struct _AnjutaMsgmanPage
{
	GtkWidget *widget;
	GtkWidget *pixmap;
	GtkWidget *label;
	GtkWidget *box;
	GtkWidget *button;
	GtkWidget *close_icon;
};

typedef struct _AnjutaMsgmanPage AnjutaMsgmanPage;

static void
on_text_msgman_close_page (GtkButton* button, 
									AnjutaMsgman *msgman)
{
	anjuta_msgman_remove_view (msgman, NULL);
}

static AnjutaMsgmanPage *
anjuta_msgman_page_new (GtkWidget * view, const gchar * name,
			const gchar * pixmap, AnjutaMsgman * msgman)
{
	g_return_val_if_fail (view != NULL, NULL);

	AnjutaMsgmanPage *page;
	
	page = g_new0 (AnjutaMsgmanPage, 1);
	page->widget = GTK_WIDGET (view);
	
	g_object_ref (G_OBJECT (page->widget));
	
	page->label = gtk_label_new (name);
	page->box = gtk_hbox_new (FALSE, 0);
	gtk_box_set_spacing (GTK_BOX (page->box), 5);
	if (pixmap)
	{
		page->pixmap = anjuta_res_get_image_sized (pixmap, 16, 16);
		g_object_ref (page->pixmap);
		gtk_box_pack_start_defaults (GTK_BOX (page->box), page->pixmap);
	}
	gtk_box_pack_start_defaults (GTK_BOX (page->box), page->label);
	page->close_icon = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_widget_set_size_request(page->close_icon, 16,16);
	//gtk_widget_show(page->close_icon);
	page->button = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(page->button), page->close_icon);
	gtk_button_set_relief(GTK_BUTTON(page->button), GTK_RELIEF_NONE);
	gtk_widget_set_size_request (page->button, 16, 16);
	gtk_box_pack_start_defaults (GTK_BOX (page->box), page->button);	
		
	gtk_signal_connect (GTK_OBJECT (page->button), "clicked",
						GTK_SIGNAL_FUNC(on_text_msgman_close_page),
						msgman);
	
	g_object_ref (page->label);
	g_object_ref (page->box);
	g_object_ref (page->button);
	g_object_ref (page->close_icon);	
	
	gtk_widget_show_all (page->box);
	return page;
}

static void
anjuta_msgman_page_destroy (AnjutaMsgmanPage * page)
{
	g_object_unref (G_OBJECT (page->label));
	g_object_unref (G_OBJECT (page->close_icon));
	g_object_unref (G_OBJECT (page->button));
	if (page->pixmap)
	{
		// gtk_container_remove (GTK_CONTAINER (page->box), page->pixmap);
		g_object_unref (G_OBJECT (page->pixmap));
	}
	// gtk_container_remove (GTK_CONTAINER (page->box), page->label);
	// gtk_container_remove (GTK_CONTAINER (page->button), page->close_icon);
	// gtk_container_remove (GTK_CONTAINER (page->box), page->button);
	g_object_unref (G_OBJECT (page->box));
	g_object_unref (G_OBJECT (page->widget));
	g_free (page);
}

static void
on_notebook_switch_page (GtkNotebook * notebook,
			 GtkNotebookPage * page,
			 gint page_num, AnjutaMsgman * msgman)
{
	g_return_if_fail (notebook != NULL);
	g_return_if_fail (page != NULL);
	g_return_if_fail (msgman != NULL);

	msgman->priv->current_view =
		MESSAGE_VIEW (gtk_notebook_get_nth_page (notebook, page_num));
}

static gpointer parent_class;

static void
anjuta_msgman_finalize (GObject *obj)
{
	GList *node;
	AnjutaMsgman *msgman = ANJUTA_MSGMAN (obj);
	
	if (msgman->priv->views)
	{
		node = msgman->priv->views;
		while (node)
		{
			AnjutaMsgmanPage *page = (AnjutaMsgmanPage*) node->data;
			anjuta_msgman_page_destroy (page);
			node = g_list_next (node);
		}
		g_list_free (msgman->priv->views);
		msgman->priv->views = NULL;
	}
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (G_OBJECT(obj)));
}

static void
anjuta_msgman_dispose (GObject *obj)
{
	AnjutaMsgman *msgman = ANJUTA_MSGMAN (obj);
	if (msgman->priv)
	{
		g_free (msgman->priv);
		msgman->priv = NULL;
	}
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (G_OBJECT(obj)));
}

static void
anjuta_msgman_instance_init (AnjutaMsgman * msgman)
{
	g_signal_connect (GTK_NOTEBOOK (msgman), "switch-page",
			  G_CALLBACK (on_notebook_switch_page), msgman);
	msgman->priv = g_new0(AnjutaMsgmanPriv, 1);
}

static void
anjuta_msgman_class_init (AnjutaMsgmanClass * klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	
	parent_class = g_type_class_peek_parent (klass);
	gobject_class->finalize = anjuta_msgman_finalize;
	gobject_class->dispose = anjuta_msgman_dispose;
}

GtkWidget*
anjuta_msgman_new (AnjutaPreferences *pref)
{
	GtkWidget *msgman = NULL;
	msgman = gtk_widget_new (ANJUTA_TYPE_MSGMAN, NULL);
	if (msgman)
	{
	    ANJUTA_MSGMAN (msgman)->priv->preferences = pref;
#warning "TODO: Set tab position"
	}
	return msgman;
}

ANJUTA_TYPE_BEGIN (AnjutaMsgman, anjuta_msgman, GTK_TYPE_NOTEBOOK);
ANJUTA_TYPE_END;

static AnjutaMsgmanPage *
anjuta_msgman_page_from_widget (AnjutaMsgman * msgman, MessageView * mv)
{
	GList *node;
	node = msgman->priv->views;
	while (node)
	{
		AnjutaMsgmanPage *page;
		page = node->data;
		g_assert (page);
		if (page->widget == GTK_WIDGET (mv))
			return page;
		node = g_list_next (node);
	}
	return NULL;
}

MessageView *
anjuta_msgman_add_view (AnjutaMsgman * msgman,
			const gchar * name, const gchar * pixmap)
{
	g_return_val_if_fail (msgman != NULL, NULL);
	g_return_val_if_fail (name != NULL, NULL);

	GtkWidget *mv;
	AnjutaMsgmanPage *page;

	mv = message_view_new (msgman->priv->preferences);
	g_return_val_if_fail (mv != NULL, NULL);
	g_object_set (G_OBJECT (mv), "highlite", TRUE, NULL);	
	gtk_widget_show (mv);
	page = anjuta_msgman_page_new (mv, name, pixmap, msgman);

	g_signal_handlers_block_by_func (GTK_OBJECT (msgman),
									 GTK_SIGNAL_FUNC
									 (on_notebook_switch_page), NULL);
	msgman->priv->current_view = MESSAGE_VIEW (mv);
	msgman->priv->views =
		g_list_append (msgman->priv->views, (gpointer) page);

	gtk_notebook_prepend_page (GTK_NOTEBOOK (msgman), mv, page->box);
	gtk_notebook_set_current_page (GTK_NOTEBOOK (msgman), 0);
	gtk_signal_handler_unblock_by_func (GTK_OBJECT (msgman),
										GTK_SIGNAL_FUNC
										(on_notebook_switch_page), NULL);
	return MESSAGE_VIEW (mv);

}

void
anjuta_msgman_remove_view (AnjutaMsgman * msgman, MessageView * view)
{
	AnjutaMsgmanPage *page;
	gint page_num;

	if (!view)
		view = anjuta_msgman_get_current_view (msgman);

	g_return_if_fail (view != NULL);

	page = anjuta_msgman_page_from_widget (msgman, view);

	gtk_signal_disconnect_by_func (GTK_OBJECT (msgman),
								   GTK_SIGNAL_FUNC
								   (on_notebook_switch_page), NULL);

	page_num =
		gtk_notebook_page_num (GTK_NOTEBOOK (msgman),
						       GTK_WIDGET (view));
	gtk_notebook_remove_page (GTK_NOTEBOOK (msgman), page_num);
	msgman->priv->views = g_list_remove (msgman->priv->views, page);
	anjuta_msgman_page_destroy (page);

	/* This is called to set the next active document */
	if (GTK_NOTEBOOK (msgman)->children == NULL)
		anjuta_msgman_set_current_view (msgman, NULL);
	else
	{
//FIXME
		//gtk_widget_grab_focus (GTK_WIDGET (view)); 
	}

	gtk_signal_connect (GTK_OBJECT (msgman), "switch-page",
					    GTK_SIGNAL_FUNC (on_notebook_switch_page), NULL);
}

MessageView *
anjuta_msgman_get_current_view (AnjutaMsgman * msgman)
{
	return msgman->priv->current_view;
}

MessageView *
anjuta_msgman_get_view_by_name (AnjutaMsgman * msgman, const gchar * name)
{
	GList *node;

	g_return_val_if_fail (msgman != NULL, NULL);
	g_return_val_if_fail (name != NULL, NULL);

	node = msgman->priv->views;
	while (node)
	{
		AnjutaMsgmanPage *page;
		page = node->data;
		g_assert (page);
		if (strcmp (gtk_label_get_text (GTK_LABEL (page->label)),
			    name) == 0)
		{
			return MESSAGE_VIEW (page->widget);
		}
		node = g_list_next (node);
	}
	return NULL;
}

void
anjuta_msgman_set_current_view (AnjutaMsgman * msgman, MessageView * mv)
{
	g_return_if_fail (msgman != NULL);
//	g_return_if_fail (mv != NULL);
	if (mv == NULL) return;

	AnjutaMsgmanPage *page;
	gint page_num;

	page = anjuta_msgman_page_from_widget (msgman, mv);
	page_num =
		gtk_notebook_page_num (GTK_NOTEBOOK (msgman),
				       GTK_WIDGET (mv));
	gtk_notebook_set_page (GTK_NOTEBOOK (msgman), page_num);
}

GList *
anjuta_msgman_get_all_views (AnjutaMsgman * msgman)
{
	return msgman->priv->views;
}
