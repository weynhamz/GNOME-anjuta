#include <gtk/gtkwidget.h>
#include <gtk/gtkentry.h>
#include <gtk/gtktoolitem.h>
//#include <libegg/toolbar/eggtoolitem.h>

#include "egg-entry-action.h"

#ifndef _
# define _(s) (s)
#endif

enum {
  CHANGED,
  FOCUS_IN,
  FOCUS_OUT,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_TEXT,
  PROP_WIDTH
};

static void egg_entry_action_init       (EggEntryAction *action);
static void egg_entry_action_class_init (EggEntryActionClass *class);

GType
egg_entry_action_get_type (void)
{
  static GtkType type = 0;

  if (!type)
    {
      static const GTypeInfo type_info =
      {
        sizeof (EggEntryActionClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) egg_entry_action_class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        
        sizeof (EggEntryAction),
        0, /* n_preallocs */
        (GInstanceInitFunc) egg_entry_action_init,
      };

      type = g_type_register_static (GTK_TYPE_ACTION,
                                     "EggEntryAction",
                                     &type_info, 0);
    }
  return type;
}

//static void egg_entry_action_activate      (GtkAction *action);
static void egg_entry_action_real_changed  (EggEntryAction *action);
static GtkWidget * create_tool_item        (GtkAction *action);
static void connect_proxy                  (GtkAction *action,
										    GtkWidget *proxy);
static void disconnect_proxy               (GtkAction *action,
										    GtkWidget *proxy);
static void entry_changed                  (GtkEditable *editable,
										    EggEntryAction *entry_action);
static void entry_focus_in                 (GtkEntry *entry, GdkEvent *event,
										    EggEntryAction *action);
static void entry_focus_out                (GtkEntry *entry, GdkEvent *event,
										    EggEntryAction *action);
static void entry_activate                 (GtkEntry *entry, GtkAction *action);
static void egg_entry_action_finalize      (GObject *object);
static void egg_entry_action_set_property  (GObject *object,
											guint prop_id,
											const GValue *value,
											GParamSpec *pspec);
static void egg_entry_action_get_property  (GObject *object,
											guint prop_id,
											GValue *value,
											GParamSpec *pspec);

static GObjectClass *parent_class = NULL;
static guint         action_signals[LAST_SIGNAL] = { 0 };

static void
egg_entry_action_class_init (EggEntryActionClass *class)
{
  GtkActionClass *action_class;
  GObjectClass   *object_class;

  parent_class = g_type_class_peek_parent (class);
  action_class = GTK_ACTION_CLASS (class);
  object_class = G_OBJECT_CLASS (class);

  object_class->finalize     = egg_entry_action_finalize;
  object_class->set_property = egg_entry_action_set_property;
  object_class->get_property = egg_entry_action_get_property;
  
  //action_class->activate = egg_entry_action_activate;
  action_class->connect_proxy = connect_proxy;
  action_class->disconnect_proxy = disconnect_proxy;

  action_class->menu_item_type = GTK_TYPE_CHECK_MENU_ITEM;
  action_class->toolbar_item_type = GTK_TYPE_TOOL_ITEM;
  action_class->create_tool_item = create_tool_item;

  class->changed = egg_entry_action_real_changed;

  g_object_class_install_property (object_class,
				   PROP_TEXT,
				   g_param_spec_string ("text",
							_("Text"),
							_("Text in the entry"),
							"",
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
				   PROP_WIDTH,
				   g_param_spec_int ("width",
						     _("Width"),
						     _("Width of the entry."),
						     5, 500, 100,
						     G_PARAM_READWRITE));
  action_signals[CHANGED] =
    g_signal_new ("changed",
                  G_OBJECT_CLASS_TYPE (class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (EggEntryActionClass, changed),
		  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
  action_signals[FOCUS_IN] =
    g_signal_new ("focus-in",
                  G_OBJECT_CLASS_TYPE (class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (EggEntryActionClass, focus_in),
		  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
  action_signals[FOCUS_OUT] =
    g_signal_new ("focus-out",
                  G_OBJECT_CLASS_TYPE (class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (EggEntryActionClass, focus_out),
		  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}

static void
egg_entry_action_init (EggEntryAction *action)
{
  action->text = g_strdup("");
  action->width = 100;
}

static void
egg_entry_action_finalize (GObject *object)
{
  g_return_if_fail (EGG_IS_ENTRY_ACTION (object));
  g_free (EGG_ENTRY_ACTION (object)->text);
  if (parent_class->finalize)
    (* parent_class->finalize) (object);
}

static void
egg_entry_action_set_property (GObject         *object,
			       guint            prop_id,
			       const GValue    *value,
			       GParamSpec      *pspec)
{
  EggEntryAction *action;

  action = EGG_ENTRY_ACTION (object);

  switch (prop_id)
    {
    case PROP_TEXT:
      // g_free (action->text);
      // action->text = g_value_dup_string (value);
      egg_entry_action_set_text (action, g_value_get_string (value));
      break;
    case PROP_WIDTH:
      action->width = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
egg_entry_action_get_property (GObject    *object,
			       guint       prop_id,
			       GValue     *value,
			       GParamSpec *pspec)
{
  EggEntryAction *action;

  action = EGG_ENTRY_ACTION (object);

  switch (prop_id)
    {
    case PROP_TEXT:
      g_value_set_string (value, action->text);
      break;
    case PROP_WIDTH:
      g_value_set_int (value, action->width);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
egg_entry_action_real_changed (EggEntryAction *action)
{
	GSList *slist;
	
	g_return_if_fail (EGG_IS_ENTRY_ACTION (action));
	
	for (slist = gtk_action_get_proxies (GTK_ACTION (action));
		 slist; slist = slist->next)
	{
		GtkWidget *proxy = slist->data;
		
		gtk_action_block_activate_from (GTK_ACTION (action), proxy);
		if (GTK_IS_CHECK_MENU_ITEM (proxy))
			gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (proxy),
											TRUE);
		else if (GTK_IS_TOOL_ITEM (proxy))
		{
			GtkWidget *entry;
			entry = gtk_bin_get_child (GTK_BIN (proxy));
			if (GTK_IS_ENTRY (entry))
			{
				g_signal_handlers_block_by_func (entry,
												 G_CALLBACK (entry_changed),
												 action);
				gtk_entry_set_text (GTK_ENTRY (entry), action->text);
				g_signal_handlers_unblock_by_func (entry,
												   G_CALLBACK (entry_changed),
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

static GtkWidget *
create_tool_item (GtkAction *action)
{
  GtkToolItem *item;
  GtkWidget *entry;

  g_return_val_if_fail (EGG_IS_ENTRY_ACTION (action), NULL);

  item = gtk_tool_item_new ();
  entry = gtk_entry_new();
  gtk_widget_set_size_request (entry, EGG_ENTRY_ACTION (action)->width, -1);
  gtk_widget_show(entry);
  gtk_container_add (GTK_CONTAINER (item), entry);
  return GTK_WIDGET (item);
}

static void
connect_proxy (GtkAction *action, GtkWidget *proxy)
{
  EggEntryAction *entry_action;

  entry_action = EGG_ENTRY_ACTION (action);

  /* do this before hand, so that we don't call the "activate" handler */
  if (GTK_IS_MENU_ITEM (proxy))
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (proxy),
				    TRUE);
  else if (GTK_IS_TOOL_ITEM (proxy))
    {
      GtkWidget *entry;
      entry = gtk_bin_get_child (GTK_BIN (proxy));
      if (GTK_IS_ENTRY (entry))
	{
	  gtk_entry_set_text (GTK_ENTRY (entry), entry_action->text);
	  g_signal_connect (entry, "activate",
			    G_CALLBACK (entry_activate), action);
	  g_signal_connect (entry, "changed",
			    G_CALLBACK (entry_changed), action);
	  g_signal_connect (entry, "focus-in-event",
			    G_CALLBACK (entry_focus_in), action);
	  g_signal_connect (entry, "focus-out-event",
			    G_CALLBACK (entry_focus_out), action);
	}
    }
  (* GTK_ACTION_CLASS (parent_class)->connect_proxy) (action, proxy);
}

static void
disconnect_proxy (GtkAction *action, GtkWidget *proxy)
{
  EggEntryAction *entry_action;

  entry_action = EGG_ENTRY_ACTION (action);
  if (GTK_IS_TOOL_ITEM (proxy))
    {
      GtkWidget *entry;
      entry = gtk_bin_get_child (GTK_BIN (proxy));
      if (GTK_IS_ENTRY (entry))
	{
	  g_signal_handlers_disconnect_by_func (entry,
						G_CALLBACK (entry_changed),
						action);
	  g_signal_handlers_disconnect_by_func (entry,
						G_CALLBACK (entry_activate),
						action);
	  g_signal_handlers_disconnect_by_func (entry,
						G_CALLBACK (entry_focus_in),
						action);
	  g_signal_handlers_disconnect_by_func (entry,
						G_CALLBACK (entry_focus_out),
						action);
	}
    }
  (* GTK_ACTION_CLASS (parent_class)->disconnect_proxy) (action, proxy);
}

static void
entry_changed (GtkEditable *editable, EggEntryAction *entry_action)
{
  if (entry_action->text)
    g_free (entry_action->text);
  entry_action->text = g_strdup (gtk_entry_get_text (GTK_ENTRY (editable)));
  egg_entry_action_changed (entry_action);
}

static void
entry_activate (GtkEntry *entry, GtkAction *action)
{
  gtk_action_activate (action);
}

static void
entry_focus_in (GtkEntry *entry, GdkEvent *event, EggEntryAction *action)
{
  g_signal_emit (action, action_signals[FOCUS_IN], 0);
}

static void
entry_focus_out (GtkEntry *entry, GdkEvent *event, EggEntryAction *action)
{
  g_signal_emit (action, action_signals[FOCUS_OUT], 0);
}

/**
 * egg_entry_action_changed:
 * @action: the action object
 *
 * Emits the "changed" signal on the toggle action.
 */
void
egg_entry_action_changed (EggEntryAction *action)
{
  g_return_if_fail (EGG_IS_ENTRY_ACTION (action));

  g_signal_emit (action, action_signals[CHANGED], 0);
}

/**
 * egg_entry_action_set_text:
 * @action: the action object
 * @text: Text to set in the entry
 *
 * Sets the checked state on the toggle action.
 */
void
egg_entry_action_set_text (EggEntryAction *action, const gchar *text)
{
  g_return_if_fail (EGG_IS_ENTRY_ACTION (action));
  g_return_if_fail (text != NULL);
  if (action->text)
    g_free (action->text);
  action->text = g_strdup (text);
  egg_entry_action_changed (action);
}

/**
 * egg_entry_action_get_active:
 * @action: the action object
 *
 * Returns: The text in the entry
 */
const gchar *
egg_entry_action_get_text (EggEntryAction *action)
{
  g_return_val_if_fail (EGG_IS_ENTRY_ACTION (action), FALSE);

  return action->text;
}
