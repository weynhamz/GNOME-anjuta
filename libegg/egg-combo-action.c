/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* egg-combo-action widget
 *
 * Copyright (C) 2001 Naba Kumar <kh_naba@yahoo.com>
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
 */
#include <gtk/gtkwidget.h>
#include "egg-combo-action.h"

#ifndef _
# define _(s) (s)
#endif

struct _EggComboActionPriv
{
	GtkTreeModel *model;
	GtkTreeIter *active_iter;
	gint width;
};

enum {
  PROP_0,
  PROP_MODEL,
  PROP_WIDTH
};

static void egg_combo_action_init       (EggComboAction *action);
static void egg_combo_action_class_init (EggComboActionClass *class);

static GtkWidget * create_tool_item        (GtkAction *action);
static void connect_proxy                  (GtkAction *action,
										    GtkWidget *proxy);
static void disconnect_proxy               (GtkAction *action,
										    GtkWidget *proxy);
static void egg_combo_action_finalize      (GObject *object);
static void egg_combo_action_despose       (GObject *object);
static void egg_combo_action_update        (EggComboAction *combo);
static void egg_combo_action_set_property  (GObject *object,
											guint prop_id,
											const GValue *value,
											GParamSpec *pspec);
static void egg_combo_action_get_property  (GObject *object,
											guint prop_id,
											GValue *value,
											GParamSpec *pspec);
static void on_change (GtkComboBox *combo, EggComboAction *action);

static GObjectClass *parent_class = NULL;

GType
egg_combo_action_get_type (void)
{
  static GtkType type = 0;

  if (!type)
    {
      static const GTypeInfo type_info =
      {
        sizeof (EggComboActionClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) egg_combo_action_class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        
        sizeof (EggComboAction),
        0, /* n_preallocs */
        (GInstanceInitFunc) egg_combo_action_init,
      };

      type = g_type_register_static (GTK_TYPE_ACTION,
                                     "EggComboAction",
                                     &type_info, 0);
    }
  return type;
}

static void
egg_combo_action_class_init (EggComboActionClass *class)
{
  GtkActionClass *action_class;
  GObjectClass   *object_class;

  parent_class = g_type_class_peek_parent (class);
  action_class = GTK_ACTION_CLASS (class);
  object_class = G_OBJECT_CLASS (class);

  object_class->finalize     = egg_combo_action_finalize;
  object_class->finalize     = egg_combo_action_despose;
  object_class->set_property = egg_combo_action_set_property;
  object_class->get_property = egg_combo_action_get_property;
 
  action_class->connect_proxy = connect_proxy;
  action_class->disconnect_proxy = disconnect_proxy;
  action_class->menu_item_type = GTK_TYPE_CHECK_MENU_ITEM;
  action_class->toolbar_item_type = GTK_TYPE_TOOL_ITEM;
  action_class->create_tool_item = create_tool_item;

  g_object_class_install_property (object_class,
								   PROP_MODEL,
								   g_param_spec_pointer ("model",
											_("Model"),
											_("Model for the combo box"),
											G_PARAM_READWRITE |
											G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
								   PROP_WIDTH,
								   g_param_spec_int ("width",
											 _("Width"),
											 _("Width of the entry."),
											 5, 500, 100,
											 G_PARAM_READWRITE));
}

static void
egg_combo_action_init (EggComboAction *action)
{
	action->priv = g_new0 (EggComboActionPriv, 1);
	action->priv->model = NULL;
	action->priv->active_iter = NULL;
	action->priv->width = 100;
}

static void
egg_combo_action_set_property (GObject         *object,
							   guint            prop_id,
							   const GValue    *value,
							   GParamSpec      *pspec)
{
  EggComboAction *action;

  action = EGG_COMBO_ACTION (object);

  switch (prop_id)
    {
    case PROP_MODEL:
      egg_combo_action_set_model (action,
								  GTK_TREE_MODEL (g_value_get_pointer (value)));
      break;
    case PROP_WIDTH:
      action->priv->width = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
egg_combo_action_get_property (GObject    *object,
							   guint       prop_id,
							   GValue     *value,
							   GParamSpec *pspec)
{
  EggComboAction *action;

  action = EGG_COMBO_ACTION (object);

  switch (prop_id)
    {
    case PROP_MODEL:
      g_value_set_pointer (value, action->priv->model);
      break;
    case PROP_WIDTH:
      g_value_set_int (value, action->priv->width);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

void static
egg_combo_action_update (EggComboAction *action)
{
	GSList *slist;
	
	for (slist = gtk_action_get_proxies (GTK_ACTION (action));
		 slist; slist = slist->next)
	{
		GtkWidget *proxy = slist->data;
		
		gtk_action_block_activate_from (GTK_ACTION (action), proxy);
		if (GTK_IS_TOOL_ITEM (proxy))
		{
			GtkWidget *combo;
			combo = gtk_bin_get_child (GTK_BIN (proxy));
			if (GTK_IS_COMBO_BOX (combo))
			{
				g_signal_handlers_block_by_func (combo,
												 G_CALLBACK (on_change),
												 action);
				gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo),
											   action->priv->active_iter);
				g_signal_handlers_unblock_by_func (combo,
												   G_CALLBACK (on_change),
												   action);
			}
			else
			{
				g_warning ("Don't know how to change `%s' widgets",
				G_OBJECT_TYPE_NAME (proxy));
			}
		}
		else
		{
			g_warning ("Don't know how to change `%s' widgets",
						G_OBJECT_TYPE_NAME (proxy));
		}
		gtk_action_unblock_activate_from (GTK_ACTION (action), proxy);
	}
}

static void
egg_combo_action_finalize (GObject *object)
{
	EggComboActionPriv *priv;
	
	priv = EGG_COMBO_ACTION (object)->priv;
	if (priv->model)
		g_object_unref (priv->model);
	priv->model = NULL;
	if (parent_class->finalize)
		(* parent_class->finalize) (object);
}

static void
egg_combo_action_despose (GObject *object)
{
  if (EGG_COMBO_ACTION (object)->priv->active_iter)
  	gtk_tree_iter_free (EGG_COMBO_ACTION (object)->priv->active_iter);
  g_free (EGG_COMBO_ACTION (object)->priv);
  if (parent_class->dispose)
    (* parent_class->dispose) (object);
}

static void
on_change (GtkComboBox *combo, EggComboAction *action)
{
	GtkTreeIter iter;
	if (action->priv->active_iter)
		gtk_tree_iter_free (action->priv->active_iter);
	gtk_combo_box_get_active_iter (combo, &iter);
	action->priv->active_iter = gtk_tree_iter_copy (&iter);
	egg_combo_action_update (action);
	gtk_action_activate (GTK_ACTION (action));
}

static GtkWidget *
create_tool_item (GtkAction *action)
{
  GtkToolItem *item;
  GtkWidget *combo;
  GtkCellRenderer *renderer;

  g_return_val_if_fail (EGG_IS_COMBO_ACTION (action), NULL);
  g_message ("Creating combo toolitem");
  item = gtk_tool_item_new ();
  combo = gtk_combo_box_new ();
  gtk_widget_show (combo);
  if (EGG_COMBO_ACTION (action)->priv->model)
	  gtk_combo_box_set_model (GTK_COMBO_BOX (combo),
							   EGG_COMBO_ACTION (action)->priv->model);

  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo),
							  renderer, FALSE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo),
								  renderer,
								  "pixbuf", 0,
								  NULL);
  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo),
							  renderer, FALSE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo),
								  renderer,
								  "text", 1,
								  NULL);

  gtk_container_add (GTK_CONTAINER (item), combo);
  gtk_widget_show(GTK_WIDGET (item));
  return GTK_WIDGET (item);
}

static void
connect_proxy (GtkAction *action, GtkWidget *proxy)
{
  EggComboAction *combo_action;

  combo_action = EGG_COMBO_ACTION (action);

  /* do this before hand, so that we don't call the "activate" handler */
  if (GTK_IS_MENU_ITEM (proxy))
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (proxy),
				    TRUE);
  else if (GTK_IS_TOOL_ITEM (proxy))
    {
      GtkWidget *combo;
      combo = gtk_bin_get_child (GTK_BIN (proxy));
      if (GTK_IS_COMBO_BOX (combo))
		{
		  if (EGG_COMBO_ACTION (action)->priv->model)
			  gtk_combo_box_set_model (GTK_COMBO_BOX (combo),
									   EGG_COMBO_ACTION (action)->priv->model);
			  g_signal_connect (G_OBJECT (combo), "changed",
								G_CALLBACK (on_change), action);
		}
    }
  (* GTK_ACTION_CLASS (parent_class)->connect_proxy) (action, proxy);
}

static void
disconnect_proxy (GtkAction *action, GtkWidget *proxy)
{
  EggComboAction *combo_action;

  combo_action = EGG_COMBO_ACTION (action);

  if (GTK_IS_TOOL_ITEM (proxy))
    {
      GtkWidget *combo;
      combo = gtk_bin_get_child (GTK_BIN (proxy));
      if (GTK_IS_COMBO (combo))
		{
		  g_signal_connect (combo, "changed",
							G_CALLBACK (on_change), action);
		}
    }
  (* GTK_ACTION_CLASS (parent_class)->connect_proxy) (action, proxy);
}

/**
 * egg_combo_action_set_model:
 * @action: the action object
 * @model: a GtkTreeModel
 *
 * Sets a model in the action.
 */
void
egg_combo_action_set_model (EggComboAction *action, GtkTreeModel *model)
{
  GSList *slist;

  g_return_if_fail (EGG_IS_COMBO_ACTION (action));

  if (action->priv->model) {
	  g_object_unref (action->priv->model);
  }
  g_object_ref (model);
  action->priv->model = model;

  for (slist = gtk_action_get_proxies (GTK_ACTION(action));
	   slist; slist = slist->next)
    {
      GtkWidget *proxy = slist->data;

      gtk_action_block_activate_from (GTK_ACTION (action), proxy);
      if (GTK_IS_CHECK_MENU_ITEM (proxy))
	  {
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (proxy),
						TRUE);
	  }
	  else if (GTK_IS_TOOL_ITEM (proxy))
	  {
		  GtkWidget *combo;
		  combo = gtk_bin_get_child (GTK_BIN (proxy));
		  if (GTK_IS_COMBO (combo))
			{
			  gtk_combo_box_set_model (GTK_COMBO_BOX (combo), model);
			}
		  else
			{
			  g_warning ("Don't know how to set popdown for `%s' widgets",
				 G_OBJECT_TYPE_NAME (proxy));
			}
	  }
      else
	  {
			g_warning ("Don't know how to set popdown for `%s' widgets",
				   G_OBJECT_TYPE_NAME (proxy));
      }
      gtk_action_unblock_activate_from (GTK_ACTION (action), proxy);
    }
}

GtkTreeModel*
egg_combo_action_get_model (EggComboAction *action)
{
	return action->priv->model;
}
