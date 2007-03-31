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
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-shell.h>

#include <libanjuta/resources.h>
#include "anjuta-msgman.h"
#include "message-view.h"

struct _AnjutaMsgmanPriv
{
	AnjutaPreferences *preferences;
	GtkWidget* popup_menu;
	GtkWidget* tab_popup;
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
on_msgman_close_page (GtkButton* button, 
									AnjutaMsgman *msgman)
{
	MessageView *view = MESSAGE_VIEW (g_object_get_data (G_OBJECT (button),
														 "message_view"));
	anjuta_msgman_remove_view (msgman, view);
}

static void 
on_msgman_close_all(GtkMenuItem* item, AnjutaMsgman* msgman)
{
	
	anjuta_msgman_remove_all_views(msgman);
}

static GtkWidget*
create_tab_popup_menu(AnjutaMsgman* msgman)
{
	GtkWidget* menu;
	GtkWidget* item_close_all;
	
	menu = gtk_menu_new();
	
	item_close_all = gtk_menu_item_new_with_label(_("Close all message tabs"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_close_all);
	g_signal_connect(G_OBJECT(item_close_all), "activate", G_CALLBACK(on_msgman_close_all),
					 msgman);
	gtk_widget_show_all(menu);
	
	gtk_menu_attach_to_widget(GTK_MENU(menu), GTK_WIDGET(msgman), NULL);
	return menu;
}

static void
on_msgman_popup_menu(GtkWidget* widget, AnjutaMsgman* msgman)
{
	gtk_menu_popup(GTK_MENU(msgman->priv->tab_popup),
				   NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
}

static gboolean
on_tab_button_press_event(GtkWidget* widget, GdkEventButton* event, AnjutaMsgman* msgman)
{
	if (event->type == GDK_BUTTON_PRESS)
	{
		if (event->button == 3)
		{
				gtk_menu_popup(GTK_MENU(msgman->priv->tab_popup), NULL, NULL, NULL, NULL, 
							   event->button, event->time);
			return TRUE;
		}
	}
	return FALSE;
}

static AnjutaMsgmanPage *
anjuta_msgman_page_new (GtkWidget * view, const gchar * name,
			const gchar * pixmap, AnjutaMsgman * msgman)
{
	AnjutaMsgmanPage *page;
	int h, w;
	GtkRcStyle *rcstyle;
	
	g_return_val_if_fail (view != NULL, NULL);

	page = g_new0 (AnjutaMsgmanPage, 1);
	page->widget = GTK_WIDGET (view);
	
	page->label = gtk_label_new (name);
	gtk_misc_set_alignment (GTK_MISC(page->label), 0.0, 0.5);
	page->box = gtk_hbox_new (FALSE, 0);
	gtk_box_set_spacing (GTK_BOX (page->box), 5);
	if (pixmap  && strlen(pixmap))
	{
		GtkStockItem unused;
		if (gtk_stock_lookup(pixmap, &unused))
		{
			page->pixmap = gtk_image_new_from_stock(pixmap, GTK_ICON_SIZE_MENU);
		}
		else
		{
			page->pixmap = anjuta_res_get_image_sized (pixmap, 16, 16);
		}
		gtk_box_pack_start (GTK_BOX (page->box), page->pixmap, FALSE, FALSE, 0);
	}
	gtk_box_pack_start (GTK_BOX (page->box), page->label, TRUE, TRUE, 0);
	gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &w, &h);
	page->close_icon = gtk_image_new_from_stock(GTK_STOCK_CLOSE,
												GTK_ICON_SIZE_MENU);
	gtk_widget_set_size_request(page->close_icon, w, h);
	
	page->button = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(page->button), page->close_icon);
	gtk_widget_set_size_request (page->button, w, h);	
	gtk_button_set_focus_on_click (GTK_BUTTON (page->button), FALSE);
	gtk_button_set_relief(GTK_BUTTON(page->button), GTK_RELIEF_NONE);
	rcstyle = gtk_rc_style_new ();
	rcstyle->xthickness = rcstyle->ythickness = 0;
	gtk_widget_modify_style (page->button, rcstyle);
	gtk_rc_style_unref (rcstyle);
	
	gtk_box_pack_start (GTK_BOX (page->box), page->button, FALSE, FALSE, 0);
	
	g_object_set_data (G_OBJECT (page->button), "message_view", page->widget);
	g_signal_connect (GTK_OBJECT (page->button), "clicked",
						G_CALLBACK(on_msgman_close_page),
						msgman);
	
	gtk_widget_show_all (page->box);
	return page;
}

static void
anjuta_msgman_page_destroy (AnjutaMsgmanPage * page)
{
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
	
	anjuta_msgman_set_current_view(msgman, NULL);
}

static gpointer parent_class;

static void
anjuta_msgman_dispose (GObject *obj)
{
	AnjutaMsgman *msgman = ANJUTA_MSGMAN (obj);
	if (msgman->priv->views)
	{
		anjuta_msgman_remove_all_views (msgman);
	}
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (G_OBJECT(obj)));
}

static void
anjuta_msgman_finalize (GObject *obj)
{
	AnjutaMsgman *msgman = ANJUTA_MSGMAN (obj);
	gtk_widget_destroy(msgman->priv->tab_popup);
	if (msgman->priv)
	{
		g_free (msgman->priv);
		msgman->priv = NULL;
	}
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (G_OBJECT(obj)));
}

static void
anjuta_msgman_instance_init (AnjutaMsgman * msgman)
{
	g_signal_connect (GTK_NOTEBOOK (msgman), "switch-page",
			  G_CALLBACK (on_notebook_switch_page), msgman);
	gtk_notebook_set_scrollable (GTK_NOTEBOOK (msgman), TRUE);
	msgman->priv = g_new0(AnjutaMsgmanPriv, 1);
	msgman->priv->views = NULL;
	msgman->priv->current_view = NULL;
	msgman->priv->tab_popup = create_tab_popup_menu(msgman);
	g_signal_connect(GTK_OBJECT(msgman), "popup-menu", 
                       G_CALLBACK(on_msgman_popup_menu), msgman);
    g_signal_connect(GTK_OBJECT(msgman), "button-press-event", 
                       G_CALLBACK(on_tab_button_press_event), msgman);
}

static void
anjuta_msgman_class_init (AnjutaMsgmanClass * klass)
{
	static gboolean initialized = FALSE;
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	
	if (!initialized) {
		/* Signal */
		g_signal_new ("view-changed",
			ANJUTA_TYPE_MSGMAN,
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (AnjutaMsgman, view_changed),
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0,
			NULL);


		initialized = TRUE;
	}
	
	parent_class = g_type_class_peek_parent (klass);
	gobject_class->finalize = anjuta_msgman_finalize;
	gobject_class->dispose = anjuta_msgman_dispose;
}

static void
set_message_tab(AnjutaPreferences *pref, GtkNotebook *msgman)
{
	gchar *tab_pos; 
	GtkPositionType pos;
	
	tab_pos = anjuta_preferences_get (pref, MESSAGES_TABS_POS);
	pos = GTK_POS_TOP;
	if (tab_pos)
	{
		if (strcasecmp (tab_pos, "left") == 0)
			pos = GTK_POS_LEFT;
		else if (strcasecmp (tab_pos, "right") == 0)
			pos = GTK_POS_RIGHT;
		else if (strcasecmp (tab_pos, "bottom") == 0)
			pos = GTK_POS_BOTTOM;
		g_free (tab_pos);
	}
	gtk_notebook_set_tab_pos (msgman, pos);
}

void
on_gconf_notify_message_pref (GConfClient *gclient, guint cnxn_id,
					   GConfEntry *entry, gpointer user_data)
{
	AnjutaPreferences *pref;
	
	pref = ANJUTA_MSGMAN (user_data)->priv->preferences;
	set_message_tab(pref, GTK_NOTEBOOK (user_data));
}


GtkWidget*
anjuta_msgman_new (AnjutaPreferences *pref, GtkWidget *popup_menu)
{
	GtkWidget *msgman = NULL;
	msgman = gtk_widget_new (ANJUTA_TYPE_MSGMAN, NULL);
	if (msgman)
	{
	    ANJUTA_MSGMAN (msgman)->priv->preferences = pref;
	    ANJUTA_MSGMAN (msgman)->priv->popup_menu = popup_menu;
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

static void
on_message_view_destroy (MessageView *view, AnjutaMsgman *msgman)
{
	AnjutaMsgmanPage *page;
	gint page_num;

	page = anjuta_msgman_page_from_widget (msgman, view);

	g_signal_handlers_disconnect_by_func (G_OBJECT (view),
					  G_CALLBACK (on_message_view_destroy), msgman);

	g_signal_handlers_block_by_func (GTK_OBJECT (msgman),
									 GTK_SIGNAL_FUNC
									 (on_notebook_switch_page), msgman);

	page_num =
		gtk_notebook_page_num (GTK_NOTEBOOK (msgman),
						       GTK_WIDGET (view));
	msgman->priv->views = g_list_remove (msgman->priv->views, page);
	anjuta_msgman_page_destroy (page);

	// gtk_notebook_remove_page (GTK_NOTEBOOK (msgman), page_num);
	
	/* This is called to set the next active document */
	if (GTK_NOTEBOOK (msgman)->children == NULL)
		anjuta_msgman_set_current_view (msgman, NULL);
	else
	{
//FIXME
		//gtk_widget_grab_focus (GTK_WIDGET (view)); 
	}

	g_signal_handlers_unblock_by_func (GTK_OBJECT (msgman),
									   GTK_SIGNAL_FUNC
									   (on_notebook_switch_page), msgman);
}

static void
anjuta_msgman_append_view (AnjutaMsgman * msgman, GtkWidget *mv,
						   const gchar * name, const gchar * pixmap)
{
	AnjutaMsgmanPage *page;

	g_return_if_fail (msgman != NULL);
	g_return_if_fail (mv != NULL);
	g_return_if_fail (name != NULL);

	gtk_widget_show (mv);
	page = anjuta_msgman_page_new (mv, name, pixmap, msgman);

	g_signal_handlers_block_by_func (GTK_OBJECT (msgman),
									 GTK_SIGNAL_FUNC
									 (on_notebook_switch_page), msgman);
	msgman->priv->current_view = MESSAGE_VIEW (mv);
	g_signal_emit_by_name(G_OBJECT(msgman), "view_changed");
	msgman->priv->views =
		g_list_prepend (msgman->priv->views, (gpointer) page);

	gtk_notebook_prepend_page (GTK_NOTEBOOK (msgman), mv, page->box);
	gtk_notebook_set_current_page (GTK_NOTEBOOK (msgman), 0);
	
	g_signal_connect (G_OBJECT (mv), "destroy",
					  G_CALLBACK (on_message_view_destroy), msgman);
	g_signal_handlers_unblock_by_func (GTK_OBJECT (msgman),
									   GTK_SIGNAL_FUNC
									   (on_notebook_switch_page), msgman);
}

MessageView *
anjuta_msgman_add_view (AnjutaMsgman * msgman,
						const gchar * name, const gchar * pixmap)
{
	GtkWidget *mv;

	g_return_val_if_fail (msgman != NULL, NULL);
	g_return_val_if_fail (name != NULL, NULL);

	mv = message_view_new (msgman->priv->preferences, msgman->priv->popup_menu);
	g_return_val_if_fail (mv != NULL, NULL);
	g_object_set (G_OBJECT (mv), "highlite", TRUE, "label", name,
				  "pixmap", pixmap, NULL);
	anjuta_msgman_append_view (msgman, mv, name, pixmap);
	return MESSAGE_VIEW (mv);
}

void
anjuta_msgman_remove_view (AnjutaMsgman * msgman, MessageView *passed_view)
{
	MessageView *view;

	view = passed_view;
	if (!view)
		view = anjuta_msgman_get_current_view (msgman);

	g_return_if_fail (view != NULL);
	gtk_widget_destroy (GTK_WIDGET (view));
	anjuta_msgman_set_current_view(msgman, NULL);
}

void
anjuta_msgman_remove_all_views (AnjutaMsgman * msgman)
{
	GList *views, *node;
	AnjutaMsgmanPage *page;
	
	g_signal_handlers_block_by_func (GTK_OBJECT (msgman),
									 GTK_SIGNAL_FUNC
									 (on_notebook_switch_page), msgman);
	views = NULL;
	node = msgman->priv->views;
	while (node)
	{
		page = node->data;
		views = g_list_prepend (views, page->widget);
		node = g_list_next (node);
	}
	node = views;
	while (node)
	{
		gtk_widget_destroy (GTK_WIDGET (node->data));
		node = g_list_next (node);
	}
	
	g_list_free (msgman->priv->views);
	g_list_free (views);
	
	msgman->priv->views = NULL;
	anjuta_msgman_set_current_view (msgman, NULL);
	g_signal_handlers_unblock_by_func (GTK_OBJECT (msgman),
									   GTK_SIGNAL_FUNC
									   (on_notebook_switch_page), msgman);
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
	AnjutaMsgmanPage *page;
	gint page_num;

	if (mv == NULL)
	{
		if (gtk_notebook_get_n_pages == 0)
		{
			msgman->priv->current_view = NULL;
		}
		else
		{
			msgman->priv->current_view = 
				MESSAGE_VIEW(gtk_notebook_get_nth_page(GTK_NOTEBOOK(msgman),
													   gtk_notebook_get_current_page(GTK_NOTEBOOK(msgman))));
		}
	}
	else
	{
		page = anjuta_msgman_page_from_widget (msgman, mv);
		page_num =
			gtk_notebook_page_num (GTK_NOTEBOOK (msgman),
					       GTK_WIDGET (mv));
		gtk_notebook_set_page (GTK_NOTEBOOK (msgman), page_num);
	}
	g_signal_emit_by_name(G_OBJECT(msgman), "view_changed");
}

GList *
anjuta_msgman_get_all_views (AnjutaMsgman * msgman)
{
	return msgman->priv->views;
}

void
anjuta_msgman_set_view_title (AnjutaMsgman *msgman, MessageView *view,
							  const gchar *title)
{
	AnjutaMsgmanPage *page;
	
	g_return_if_fail (title != NULL);
	
	page = anjuta_msgman_page_from_widget (msgman, view);
	g_return_if_fail (page != NULL);
	
	gtk_label_set_text (GTK_LABEL (page->label), title);
}

gboolean
anjuta_msgman_serialize (AnjutaMsgman *msgman, AnjutaSerializer *serializer)
{
	GList *node;
	
	if (!anjuta_serializer_write_int (serializer, "views",
									  g_list_length (msgman->priv->views)))
		return FALSE;
	
	node = msgman->priv->views;
	while (node)
	{
		AnjutaMsgmanPage *page = (AnjutaMsgmanPage*)node->data;
		if (!message_view_serialize (MESSAGE_VIEW (page->widget), serializer))
			return FALSE;
		node = g_list_next (node);
	}
	return TRUE;
}

gboolean
anjuta_msgman_deserialize (AnjutaMsgman *msgman, AnjutaSerializer *serializer)
{
	gint views, i;
	
	if (!anjuta_serializer_read_int (serializer, "views", &views))
		return FALSE;
	
	for (i = 0; i < views; i++)
	{
		gchar *label, *pixmap;
		GtkWidget *view;
		view = message_view_new (msgman->priv->preferences,
								 msgman->priv->popup_menu);
		g_return_val_if_fail (view != NULL, FALSE);
		if (!message_view_deserialize (MESSAGE_VIEW (view), serializer))
		{
			gtk_widget_destroy (view);
			return FALSE;
		}
		g_object_get (view, "label", &label, "pixmap", &pixmap, NULL);
		anjuta_msgman_append_view (msgman, view, label, pixmap);
		g_free (label);
		g_free (pixmap);
	}
	return TRUE;
}
