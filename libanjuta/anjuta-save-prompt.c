/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-save-prompt.c
 * Copyright (C) 2000 Naba Kumar
 *
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

#include <glib/gi18n.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkcellrenderertoggle.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkstock.h>

#include "anjuta-save-prompt.h"

static void anjuta_save_prompt_class_init(AnjutaSavePromptClass *klass);
static void anjuta_save_prompt_init(AnjutaSavePrompt *sp);
static void anjuta_save_prompt_finalize(GObject *object);

struct _AnjutaSavePromptPrivate {
	/* Place Private Members Here */
	GtkWidget *treeview;
};

enum {
	COL_SAVE_ENABLE,
	COL_LABEL,
	COL_ITEM,
	COL_ITEM_SAVE_FUNC,
	COL_ITEM_SAVE_FUNC_DATA,
	N_COLS
};

static GtkDialogClass *parent_class = NULL;

GType
anjuta_save_prompt_get_type()
{
	static GType type = 0;

	if(type == 0) {
		static const GTypeInfo our_info = {
			sizeof (AnjutaSavePromptClass),
			NULL,
			NULL,
			(GClassInitFunc)anjuta_save_prompt_class_init,
			NULL,
			NULL,
			sizeof (AnjutaSavePrompt),
			0,
			(GInstanceInitFunc)anjuta_save_prompt_init,
		};

		type = g_type_register_static(GTK_TYPE_MESSAGE_DIALOG, 
			"AnjutaSavePrompt", &our_info, 0);
	}

	return type;
}

static void
anjuta_save_prompt_class_init(AnjutaSavePromptClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	parent_class = g_type_class_peek_parent(klass);
	object_class->finalize = anjuta_save_prompt_finalize;
}

static void
on_dialog_response (GtkWidget *dlg, gint response, gpointer user_data)
{
	if (response == ANJUTA_SAVE_PROMPT_RESPONSE_SAVE_CLOSE)
	{
		GtkWidget *treeview;
		GtkTreeModel *model;
		GtkTreeIter iter;
		
		AnjutaSavePrompt *save_prompt = ANJUTA_SAVE_PROMPT (dlg);
		treeview = save_prompt->priv->treeview;
		model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
		if(gtk_tree_model_get_iter_first (model, &iter))
		{
			do {
				gboolean save_enable;
				gpointer item, user_data;
				AnjutaSavePromptSaveFunc item_save_func;
				
				gtk_tree_model_get (model, &iter,
									COL_SAVE_ENABLE, &save_enable,
									COL_ITEM, &item,
									COL_ITEM_SAVE_FUNC, &item_save_func,
									COL_ITEM_SAVE_FUNC_DATA, &user_data,
									-1);
				if (save_enable)
				{
					if (item_save_func(save_prompt, item, user_data))
					{
						gtk_list_store_set (GTK_LIST_STORE (model), &iter,
											COL_SAVE_ENABLE, FALSE,
											-1);
					}
					else
					{
						g_signal_stop_emission_by_name (save_prompt,
														"response");
						break;
					}
				}
			}
			while (gtk_tree_model_iter_next (model, &iter));
		}
	}
}

static void
anjuta_save_prompt_init(AnjutaSavePrompt *obj)
{
	GtkWidget *vbox;
	GtkWidget *treeview;
	GtkListStore *store;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
	GtkWidget *label;
	GtkWidget *scrolledwindow;
	
	obj->priv = g_new0(AnjutaSavePromptPrivate, 1);

	gtk_label_set_text_with_mnemonic (GTK_LABEL (GTK_MESSAGE_DIALOG (obj)->label),
									  "Uninitialized");
	gtk_window_set_resizable (GTK_WINDOW (obj), TRUE);
	gtk_window_set_default_size (GTK_WINDOW (obj), 400, 300);
	
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (obj)->vbox),
						vbox, TRUE, TRUE, 0);
	gtk_widget_show (vbox);
	
	label = gtk_label_new (_("Select the items to save:"));
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
	gtk_misc_set_padding  (GTK_MISC (label), 10, -1);
	gtk_misc_set_alignment (GTK_MISC (label), 0, -1);
	gtk_widget_show (label);
	
	store = gtk_list_store_new (N_COLS, G_TYPE_BOOLEAN,
								G_TYPE_STRING, G_TYPE_POINTER,
								G_TYPE_POINTER, G_TYPE_POINTER);
	
	scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX (vbox), scrolledwindow, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (scrolledwindow), 10);
	gtk_widget_show (scrolledwindow);
	
	treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
	g_object_unref (store);
	gtk_container_add (GTK_CONTAINER (scrolledwindow), treeview);
	gtk_widget_show (treeview);
	
	label = gtk_label_new (_("If you do not save, all your changes will be lost."));
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
	gtk_misc_set_padding (GTK_MISC (label), 10, -1);
	gtk_misc_set_alignment (GTK_MISC (label), 0, -1);
	gtk_widget_show (label);
	
	renderer = gtk_cell_renderer_toggle_new ();
	col = gtk_tree_view_column_new_with_attributes (_("Save"),
													renderer,
													"active", COL_SAVE_ENABLE,
													NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), col);
	
	renderer = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes (_("Item"),
													renderer,
													"markup", COL_LABEL,
													NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), col);
	
	gtk_dialog_add_buttons (GTK_DIALOG (obj), _("_Discard changes"),
							ANJUTA_SAVE_PROMPT_RESPONSE_DISCARD,
							GTK_STOCK_CANCEL,
							ANJUTA_SAVE_PROMPT_RESPONSE_CANCEL,
							GTK_STOCK_SAVE,
							ANJUTA_SAVE_PROMPT_RESPONSE_SAVE_CLOSE,
							NULL);
	gtk_dialog_set_has_separator (GTK_DIALOG (obj), TRUE);
	gtk_dialog_set_default_response (GTK_DIALOG (obj),
									 ANJUTA_SAVE_PROMPT_RESPONSE_SAVE_CLOSE);
	g_signal_connect (obj, "response",
					  G_CALLBACK (on_dialog_response),
					  obj);
	obj->priv->treeview = treeview;
}

static void
anjuta_save_prompt_finalize(GObject *object)
{
	AnjutaSavePrompt *cobj;
	cobj = ANJUTA_SAVE_PROMPT(object);
	
	/* Free private members, etc. */
		
	g_free(cobj->priv);
	G_OBJECT_CLASS(parent_class)->finalize(object);
}

AnjutaSavePrompt *
anjuta_save_prompt_new (GtkWindow *parent)
{
	AnjutaSavePrompt *obj;
	
	g_return_val_if_fail (GTK_IS_WINDOW (parent), NULL);
	
	obj = ANJUTA_SAVE_PROMPT(g_object_new(ANJUTA_TYPE_SAVE_PROMPT,
										  "message-type", GTK_MESSAGE_QUESTION,
										  NULL));
	gtk_dialog_set_has_separator (GTK_DIALOG (obj), TRUE);
	gtk_window_set_transient_for (GTK_WINDOW (obj), parent);
	return obj;
}

gint
anjuta_save_prompt_get_items_count (AnjutaSavePrompt *save_prompt)
{
	GtkTreeModel *model;
	g_return_val_if_fail (ANJUTA_IS_SAVE_PROMPT (save_prompt), -1);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (save_prompt->priv->treeview));
	return gtk_tree_model_iter_n_children (model, NULL);
}


void
anjuta_save_prompt_add_item (AnjutaSavePrompt *save_prompt,
							 const gchar *item_name,
							 const gchar *item_detail,
							 gpointer item,
							 AnjutaSavePromptSaveFunc item_save_func,
							 gpointer user_data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *label;
	gint items_count;
	
	g_return_if_fail (ANJUTA_IS_SAVE_PROMPT (save_prompt));
	g_return_if_fail (item_name != NULL);
	g_return_if_fail (item_save_func != NULL);
	
	if (item_detail)
		label = g_strdup_printf ("<b>%s</b>\n%s", item_name, item_detail);
	else
		label = g_strdup (item_name);
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (save_prompt->priv->treeview));
	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (model),
						&iter,
						COL_SAVE_ENABLE, TRUE,
						COL_LABEL, label,
						COL_ITEM, item,
						COL_ITEM_SAVE_FUNC, item_save_func,
						COL_ITEM_SAVE_FUNC_DATA, user_data,
						-1);
	g_free (label);
	
	items_count = anjuta_save_prompt_get_items_count (save_prompt);
	
	if (items_count > 1)
	{
		label = g_strdup_printf (_("<b>There are %d items with unsaved changes."
								   " Save changes before closing?</b>"),
								 items_count);
	}
	else
	{
		label = g_strdup (_("<b>There is an item with unsaved changes."
							" Save changes before closing?</b>"));
	}
	
	gtk_label_set_markup (GTK_LABEL (GTK_MESSAGE_DIALOG (save_prompt)->label),
						  label);
	
}
