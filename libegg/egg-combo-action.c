#include <gtk/gtkwidget.h>
#include <gtk/gtkentry.h>
// #include <libegg/toolbar/eggtoolitem.h>

#include "egg-combo-action.h"

#ifndef _
# define _(s) (s)
#endif

static void egg_combo_action_init       (EggComboAction *action);
static void egg_combo_action_class_init (EggComboActionClass *class);

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

      type = g_type_register_static (EGG_TYPE_ENTRY_ACTION,
                                     "EggComboAction",
                                     &type_info, 0);
    }
  return type;
}

static GtkWidget * create_tool_item        (GtkAction *action);
static void connect_proxy                  (GtkAction *action,
					    GtkWidget *proxy);
static void disconnect_proxy               (GtkAction *action,
					    GtkWidget *proxy);
static void egg_combo_action_finalize      (GObject *object);

static GObjectClass *parent_class = NULL;

static void
egg_combo_action_class_init (EggComboActionClass *class)
{
  GtkActionClass *action_class;
  GObjectClass   *object_class;

  parent_class = g_type_class_peek_parent (class);
  action_class = GTK_ACTION_CLASS (class);
  object_class = G_OBJECT_CLASS (class);

  object_class->finalize     = egg_combo_action_finalize;
  
  //action_class->activate = egg_combo_action_activate;
  action_class->connect_proxy = connect_proxy;
  action_class->disconnect_proxy = disconnect_proxy;

  action_class->menu_item_type = GTK_TYPE_CHECK_MENU_ITEM;
  action_class->toolbar_item_type = GTK_TYPE_TOOL_ITEM;
  action_class->create_tool_item = create_tool_item;
}

static void
egg_combo_action_init (EggComboAction *action)
{
  /* action->list = NULL; */
}

static void
egg_combo_action_finalize (GObject *object)
{
  /*
  GList *node;
  EggComboAction *combo_action;

  g_return_if_fail (EGG_IS_ENTRY_ACTION (object));
  combo_action = EGG_COMBO_ACTION (object);

  node = combo_action->list;
  while (node)
    {
      g_free (node->data);
      node = g_list_next (node);
    }
  g_list_free (combo_action->list);
  combo_action->list = NULL;
  if (parent_class->finalize)
    (* parent_class->finalize) (object);
  */
}

static GtkWidget *
create_tool_item (GtkAction *action)
{
  GtkToolItem *item;
  GtkWidget *combo;

  g_return_val_if_fail (EGG_IS_COMBO_ACTION (action), NULL);

  item = gtk_tool_item_new ();
  combo = gtk_combo_new();
  gtk_widget_set_size_request (combo, EGG_ENTRY_ACTION (action)->width, -1);
  gtk_widget_show(combo);
  gtk_container_add (GTK_CONTAINER (item), combo);
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
      if (GTK_IS_COMBO (combo))
	{
	  (* GTK_ACTION_CLASS (parent_class)->connect_proxy) (action,
			       GTK_COMBO(combo)->entry);
	}
    }
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
	  (* GTK_ACTION_CLASS (parent_class)->disconnect_proxy) (action,
			       GTK_COMBO(combo)->entry);
	}
    }
}

/**
 * egg_combo_action_set_popdown_strings:
 * @action: the action object
 * @text: List of Text to set in the combo
 *
 * Sets a list of strings in the action.
 */
void
egg_combo_action_set_popdown_strings (EggComboAction *action, GList *strs)
{
  GSList *slist;

  g_return_if_fail (EGG_IS_ENTRY_ACTION (action));

  for (slist = gtk_action_get_proxies (GTK_ACTION(action));
	   slist; slist = slist->next)
    {
      GtkWidget *proxy = slist->data;

      gtk_action_block_activate_from (GTK_ACTION (action), proxy);
      if (GTK_IS_CHECK_MENU_ITEM (proxy))
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (proxy),
					TRUE);
      else if (GTK_IS_TOOL_ITEM (proxy))
	{
	  GtkWidget *combo;
	  combo = gtk_bin_get_child (GTK_BIN (proxy));
	  if (GTK_IS_COMBO (combo))
	    {
	      //g_signal_handlers_block_by_func (entry,
		//			       G_CALLBACK (entry_changed),
		//			       action);
	      gtk_combo_set_popdown_strings (GTK_COMBO (combo), strs);
	      //g_signal_handlers_unblock_by_func (entry,
		//				 G_CALLBACK (entry_changed),
		//				 action);
	    }
	  else
	    {
	      g_warning ("Don't know how to set popdown for `%s' widgets",
			 G_OBJECT_TYPE_NAME (proxy));
	    }
	}
      else {
	g_warning ("Don't know how to set popdown for `%s' widgets",
		   G_OBJECT_TYPE_NAME (proxy));
      }
      gtk_action_unblock_activate_from (GTK_ACTION (action), proxy);
    }
}
