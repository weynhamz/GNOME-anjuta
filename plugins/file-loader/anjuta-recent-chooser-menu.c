/* GTK - The GIMP Toolkit
 * gtkrecentchoosermenu.c - Recently used items menu widget
 * Copyright (C) 2005, Emmanuele Bassi
 *		 2008, Ignacio Casal Quinteiro
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <gdk/gdkscreen.h>

#include "anjuta-recent-chooser-menu.h"

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gi18n.h>

struct _AnjutaRecentChooserMenuPrivate
{
  /* the recent manager object */
  GtkRecentManager *manager;
  
  /* size of the icons of the menu items */  
  gint icon_size;

  /* max size of the menu item label */
  gint label_width;

  gint first_recent_item_pos;
  GtkWidget *placeholder;

  /* RecentChooser properties */
  gint limit;  
  guint show_private : 1;
  guint show_not_found : 1;
  guint show_tips : 1;
  guint show_icons : 1;
  guint local_only : 1;
  
  guint show_numbers : 1;
  
  GtkRecentSortType sort_type;
  GtkRecentSortFunc sort_func;
  gpointer sort_data;
  GDestroyNotify sort_data_destroy;
  
  GSList *filters;
  GtkRecentFilter *current_filter;
 
  guint local_manager : 1;
  gulong manager_changed_id;

  gulong populate_id;
  
  gint prj_pos;
  gint max_files;
};

typedef enum {
  GTK_RECENT_CHOOSER_PROP_FIRST           = 0x3000,
  GTK_RECENT_CHOOSER_PROP_RECENT_MANAGER,
  GTK_RECENT_CHOOSER_PROP_SHOW_PRIVATE,
  GTK_RECENT_CHOOSER_PROP_SHOW_NOT_FOUND,
  GTK_RECENT_CHOOSER_PROP_SHOW_TIPS,
  GTK_RECENT_CHOOSER_PROP_SHOW_ICONS,
  GTK_RECENT_CHOOSER_PROP_SELECT_MULTIPLE,
  GTK_RECENT_CHOOSER_PROP_LIMIT,
  GTK_RECENT_CHOOSER_PROP_LOCAL_ONLY,
  GTK_RECENT_CHOOSER_PROP_SORT_TYPE,
  GTK_RECENT_CHOOSER_PROP_FILTER,
  GTK_RECENT_CHOOSER_PROP_LAST
} GtkRecentChooserProp;


#define FALLBACK_ICON_SIZE 	32
#define FALLBACK_ITEM_LIMIT 	10
#define DEFAULT_LABEL_WIDTH     30

#define ANJUTA_RECENT_CHOOSER_MENU_GET_PRIVATE(obj)	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), ANJUTA_TYPE_RECENT_CHOOSER_MENU, AnjutaRecentChooserMenuPrivate))

static void     anjuta_recent_chooser_menu_finalize    (GObject                   *object);
static void     anjuta_recent_chooser_menu_dispose     (GObject                   *object);
static GObject *anjuta_recent_chooser_menu_constructor (GType                      type,
						     guint                      n_construct_properties,
						     GObjectConstructParam     *construct_params);
						     
static void
anjuta_recent_chooser_menu_set_property (GObject      *object,
				      guint         prop_id,
				      const GValue *value,
				      GParamSpec   *pspec);
				      
static void
anjuta_recent_chooser_menu_get_property (GObject    *object,
				      guint       prop_id,
				      GValue     *value,
				      GParamSpec *pspec);

static void gtk_recent_chooser_iface_init      (GtkRecentChooserIface     *iface);

static gboolean          anjuta_recent_chooser_menu_set_current_uri    (GtkRecentChooser  *chooser,
							             const gchar       *uri,
							             GError           **error);
static gchar *           anjuta_recent_chooser_menu_get_current_uri    (GtkRecentChooser  *chooser);
static gboolean          anjuta_recent_chooser_menu_select_uri         (GtkRecentChooser  *chooser,
								     const gchar       *uri,
								     GError           **error);
static void              anjuta_recent_chooser_menu_unselect_uri       (GtkRecentChooser  *chooser,
								     const gchar       *uri);
static void              anjuta_recent_chooser_menu_select_all         (GtkRecentChooser  *chooser);
static void              anjuta_recent_chooser_menu_unselect_all       (GtkRecentChooser  *chooser);
static GList *           anjuta_recent_chooser_menu_get_items          (GtkRecentChooser  *chooser);
static GtkRecentManager *anjuta_recent_chooser_menu_get_recent_manager (GtkRecentChooser  *chooser);
static void              anjuta_recent_chooser_menu_set_sort_func      (GtkRecentChooser  *chooser,
								     GtkRecentSortFunc  sort_func,
								     gpointer           sort_data,
								     GDestroyNotify     data_destroy);
static void              anjuta_recent_chooser_menu_add_filter         (GtkRecentChooser  *chooser,
								     GtkRecentFilter   *filter);
static void              anjuta_recent_chooser_menu_remove_filter      (GtkRecentChooser  *chooser,
								     GtkRecentFilter   *filter);
static GSList *          anjuta_recent_chooser_menu_list_filters       (GtkRecentChooser  *chooser);
static void              anjuta_recent_chooser_menu_set_current_filter (AnjutaRecentChooserMenu *menu,
								     GtkRecentFilter      *filter);

static void              anjuta_recent_chooser_menu_populate           (AnjutaRecentChooserMenu *menu);
static void              anjuta_recent_chooser_menu_set_show_tips      (AnjutaRecentChooserMenu *menu,
								     gboolean              show_tips);

static void     set_recent_manager (AnjutaRecentChooserMenu *menu,
				    GtkRecentManager     *manager);

static void     chooser_set_sort_type (AnjutaRecentChooserMenu *menu,
				       GtkRecentSortType     sort_type);

static gint     get_icon_size_for_widget (GtkWidget *widget);

static void     item_activate_cb   (GtkWidget        *widget,
			            gpointer          user_data);
static void     manager_changed_cb (GtkRecentManager *manager,
				    gpointer          user_data);

G_DEFINE_TYPE_WITH_CODE (AnjutaRecentChooserMenu,
			 anjuta_recent_chooser_menu,
			 GTK_TYPE_MENU,
			 G_IMPLEMENT_INTERFACE (GTK_TYPE_RECENT_CHOOSER,
				 		gtk_recent_chooser_iface_init))


static void
gtk_recent_chooser_iface_init (GtkRecentChooserIface *iface)
{
  iface->set_current_uri = anjuta_recent_chooser_menu_set_current_uri;
  iface->get_current_uri = anjuta_recent_chooser_menu_get_current_uri;
  iface->select_uri = anjuta_recent_chooser_menu_select_uri;
  iface->unselect_uri = anjuta_recent_chooser_menu_unselect_uri;
  iface->select_all = anjuta_recent_chooser_menu_select_all;
  iface->unselect_all = anjuta_recent_chooser_menu_unselect_all;
  iface->get_items = anjuta_recent_chooser_menu_get_items;
  iface->get_recent_manager = anjuta_recent_chooser_menu_get_recent_manager;
  iface->set_sort_func = anjuta_recent_chooser_menu_set_sort_func;
  iface->add_filter = anjuta_recent_chooser_menu_add_filter;
  iface->remove_filter = anjuta_recent_chooser_menu_remove_filter;
  iface->list_filters = anjuta_recent_chooser_menu_list_filters;
}

static void
_anjuta_recent_chooser_install_properties (GObjectClass *klass)
{
  g_object_class_override_property (klass,
  				    GTK_RECENT_CHOOSER_PROP_RECENT_MANAGER,
  				    "recent-manager");  				    
  g_object_class_override_property (klass,
  				    GTK_RECENT_CHOOSER_PROP_SHOW_PRIVATE,
  				    "show-private");
  g_object_class_override_property (klass,
  				    GTK_RECENT_CHOOSER_PROP_SHOW_TIPS,
  				    "show-tips");
  g_object_class_override_property (klass,
  				    GTK_RECENT_CHOOSER_PROP_SHOW_ICONS,
  				    "show-icons");
  g_object_class_override_property (klass,
  				    GTK_RECENT_CHOOSER_PROP_SHOW_NOT_FOUND,
  				    "show-not-found");
  g_object_class_override_property (klass,
  				    GTK_RECENT_CHOOSER_PROP_SELECT_MULTIPLE,
  				    "select-multiple");
  g_object_class_override_property (klass,
  				    GTK_RECENT_CHOOSER_PROP_LIMIT,
  				    "limit");
  g_object_class_override_property (klass,
		  		    GTK_RECENT_CHOOSER_PROP_LOCAL_ONLY,
				    "local-only");
  g_object_class_override_property (klass,
		  		    GTK_RECENT_CHOOSER_PROP_SORT_TYPE,
				    "sort-type");
  g_object_class_override_property (klass,
  				    GTK_RECENT_CHOOSER_PROP_FILTER,
  				    "filter");
}

static gint
sort_recent_items_mru (GtkRecentInfo *a,
		       GtkRecentInfo *b,
		       gpointer       unused)
{
  g_assert (a != NULL && b != NULL);
  
  return gtk_recent_info_get_modified (b) - gtk_recent_info_get_modified (a);
}

static gboolean
get_is_recent_filtered (GtkRecentFilter *filter,
			GtkRecentInfo   *info)
{
  GtkRecentFilterInfo filter_info;
  GtkRecentFilterFlags needed;
  gboolean retval;

  g_assert (info != NULL);
  
  needed = gtk_recent_filter_get_needed (filter);
  
  filter_info.contains = GTK_RECENT_FILTER_URI | GTK_RECENT_FILTER_MIME_TYPE;
  
  filter_info.uri = gtk_recent_info_get_uri (info);
  filter_info.mime_type = gtk_recent_info_get_mime_type (info);
  
  if (needed & GTK_RECENT_FILTER_DISPLAY_NAME)
    {
      filter_info.display_name = gtk_recent_info_get_display_name (info);
      filter_info.contains |= GTK_RECENT_FILTER_DISPLAY_NAME;
    }
  else
    filter_info.uri = NULL;
  
  if (needed & GTK_RECENT_FILTER_APPLICATION)
    {
      filter_info.applications = (const gchar **) gtk_recent_info_get_applications (info, NULL);
      filter_info.contains |= GTK_RECENT_FILTER_APPLICATION;
    }
  else
    filter_info.applications = NULL;

  if (needed & GTK_RECENT_FILTER_GROUP)
    {
      filter_info.groups = (const gchar **) gtk_recent_info_get_groups (info, NULL);
      filter_info.contains |= GTK_RECENT_FILTER_GROUP;
    }
  else
    filter_info.groups = NULL;

  if (needed & GTK_RECENT_FILTER_AGE)
    {
      filter_info.age = gtk_recent_info_get_age (info);
      filter_info.contains |= GTK_RECENT_FILTER_AGE;
    }
  else
    filter_info.age = -1;
  
  retval = gtk_recent_filter_filter (filter, &filter_info);
  
  /* these we own */
  if (filter_info.applications)
    g_strfreev ((gchar **) filter_info.applications);
  if (filter_info.groups)
    g_strfreev ((gchar **) filter_info.groups);
  
  return !retval;
}

static GList *
_gtk_recent_chooser_get_items (GtkRecentChooser  *chooser,
                               GtkRecentFilter   *filter,
                               GtkRecentManager  *manager,
                               gpointer           sort_data)
{
  gint limit;
  GtkRecentSortType sort_type;
  GList *items;
  GCompareFunc compare_func;
  gint length;

  g_return_val_if_fail (GTK_IS_RECENT_CHOOSER (chooser), NULL);

  if (!manager)
    return NULL;

  items = gtk_recent_manager_get_items (manager);
  if (!items)
    return NULL;

  limit = 100;
  if (limit == 0)
    return NULL;

  if (filter)
    {
      GList *filter_items, *l;
      gboolean local_only = FALSE;
      gboolean show_private = FALSE;
      gboolean show_not_found = FALSE;

      g_object_get (G_OBJECT (chooser),
                    "local-only", &local_only,
                    "show-private", &show_private,
                    "show-not-found", &show_not_found,
                    NULL);

      filter_items = NULL;
      for (l = items; l != NULL; l = l->next)
        {
          GtkRecentInfo *info = l->data;
          gboolean remove_item = FALSE;

          if (get_is_recent_filtered (filter, info))
            remove_item = TRUE;
          
          if (local_only && !gtk_recent_info_is_local (info))
            remove_item = TRUE;

          if (!show_private && gtk_recent_info_get_private_hint (info))
            remove_item = TRUE;

          if (!show_not_found && !gtk_recent_info_exists (info))
            remove_item = TRUE;
          
          if (!remove_item)
            filter_items = g_list_prepend (filter_items, info);
          else
            gtk_recent_info_unref (info);
        }
      
      g_list_free (items);
      items = filter_items;
    }

  if (!items)
    return NULL;

  sort_type = gtk_recent_chooser_get_sort_type (chooser);
  switch (sort_type)
    {
    case GTK_RECENT_SORT_NONE:
      compare_func = NULL;
      break;
    case GTK_RECENT_SORT_MRU:
      compare_func = (GCompareFunc) sort_recent_items_mru;
      break;
    case GTK_RECENT_SORT_LRU:
      compare_func = NULL;
      break;
    case GTK_RECENT_SORT_CUSTOM:
      compare_func = NULL;
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  if (compare_func)
    {

      items = g_list_sort (items, compare_func);
    }
  
  length = g_list_length (items);
  if ((limit != -1) && (length > limit))
    {
      GList *clamp, *l;
      
      clamp = g_list_nth (items, limit - 1);
      if (!clamp)
        return items;
      
      l = clamp->next;
      clamp->next = NULL;
    
      g_list_foreach (l, (GFunc) gtk_recent_info_unref, NULL);
      g_list_free (l);
    }

  return items;
}


static void
anjuta_recent_chooser_menu_class_init (AnjutaRecentChooserMenuClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructor = anjuta_recent_chooser_menu_constructor;
  gobject_class->dispose = anjuta_recent_chooser_menu_dispose;
  gobject_class->finalize = anjuta_recent_chooser_menu_finalize;
  gobject_class->set_property = anjuta_recent_chooser_menu_set_property;
  gobject_class->get_property = anjuta_recent_chooser_menu_get_property;
  
  _anjuta_recent_chooser_install_properties (gobject_class);
  
  g_type_class_add_private (klass, sizeof (AnjutaRecentChooserMenuPrivate));
}

static void
anjuta_recent_chooser_menu_init (AnjutaRecentChooserMenu *menu)
{
  AnjutaRecentChooserMenuPrivate *priv;
  
  priv = ANJUTA_RECENT_CHOOSER_MENU_GET_PRIVATE (menu);
  
  menu->priv = priv;
  
  priv->show_icons= TRUE;
  priv->show_numbers = FALSE;
  priv->show_tips = FALSE;
  priv->show_not_found = TRUE;
  priv->show_private = FALSE;
  priv->local_only = TRUE;
  
  priv->limit = FALLBACK_ITEM_LIMIT;
  priv->sort_type = GTK_RECENT_SORT_NONE;
  priv->icon_size = FALLBACK_ICON_SIZE;
  priv->label_width = DEFAULT_LABEL_WIDTH;
  
  priv->first_recent_item_pos = -1;
  priv->placeholder = NULL;

  priv->current_filter = NULL;
}

static void
anjuta_recent_chooser_menu_finalize (GObject *object)
{
  AnjutaRecentChooserMenu *menu = ANJUTA_RECENT_CHOOSER_MENU (object);
  AnjutaRecentChooserMenuPrivate *priv = menu->priv;
  
  priv->manager = NULL;
  
  if (priv->sort_data_destroy)
    {
      priv->sort_data_destroy (priv->sort_data);
      priv->sort_data_destroy = NULL;
    }

  priv->sort_data = NULL;
  priv->sort_func = NULL;
  
  G_OBJECT_CLASS (anjuta_recent_chooser_menu_parent_class)->finalize (object);
}

static void
anjuta_recent_chooser_menu_dispose (GObject *object)
{
  AnjutaRecentChooserMenu *menu = ANJUTA_RECENT_CHOOSER_MENU (object);
  AnjutaRecentChooserMenuPrivate *priv = menu->priv;

  if (priv->manager_changed_id)
    {
      if (priv->manager)
        g_signal_handler_disconnect (priv->manager, priv->manager_changed_id);

      priv->manager_changed_id = 0;
    }
  
  if (priv->populate_id)
    {
      g_source_remove (priv->populate_id);
      priv->populate_id = 0;
    }

  if (priv->current_filter)
    {
      g_object_unref (priv->current_filter);
      priv->current_filter = NULL;
    }
  
  G_OBJECT_CLASS (anjuta_recent_chooser_menu_parent_class)->dispose (object);
}

static GObject *
anjuta_recent_chooser_menu_constructor (GType                  type,
				     guint                  n_params,
				     GObjectConstructParam *params)
{
  AnjutaRecentChooserMenu *menu;
  AnjutaRecentChooserMenuPrivate *priv;
  GObjectClass *parent_class;
  GObject *object;
  
  parent_class = G_OBJECT_CLASS (anjuta_recent_chooser_menu_parent_class);
  object = parent_class->constructor (type, n_params, params);
  menu = ANJUTA_RECENT_CHOOSER_MENU (object);
  priv = menu->priv;
  
  g_assert (priv->manager);

  /* we create a placeholder menuitem, to be used in case
   * the menu is empty. this placeholder will stay around
   * for the entire lifetime of the menu, and we just hide it
   * when it's not used. we have to do this, and do it here,
   * because we need a marker for the beginning of the recent
   * items list, so that we can insert the new items at the
   * right place when idly populating the menu in case the
   * user appended or prepended custom menu items to the
   * recent chooser menu widget.
   */
  priv->placeholder = gtk_menu_item_new_with_label (_("No items found"));
  gtk_widget_set_sensitive (priv->placeholder, FALSE);
  g_object_set_data (G_OBJECT (priv->placeholder),
                     "gtk-recent-menu-placeholder",
                     GINT_TO_POINTER (TRUE));

  gtk_menu_shell_insert (GTK_MENU_SHELL (menu), priv->placeholder, 0);
  gtk_widget_set_no_show_all (priv->placeholder, TRUE);
  gtk_widget_show (priv->placeholder);

  /* (re)populate the menu */
  anjuta_recent_chooser_menu_populate (menu);

  return object;
}

static void
anjuta_recent_chooser_menu_set_property (GObject      *object,
				      guint         prop_id,
				      const GValue *value,
				      GParamSpec   *pspec)
{
  AnjutaRecentChooserMenu *menu = ANJUTA_RECENT_CHOOSER_MENU (object);
  AnjutaRecentChooserMenuPrivate *priv = menu->priv;
  
  switch (prop_id)
    {
    case GTK_RECENT_CHOOSER_PROP_RECENT_MANAGER:
      set_recent_manager (menu, g_value_get_object (value));
      break;
    case GTK_RECENT_CHOOSER_PROP_SHOW_PRIVATE:
      priv->show_private = g_value_get_boolean (value);
      break;
    case GTK_RECENT_CHOOSER_PROP_SHOW_NOT_FOUND:
      priv->show_not_found = g_value_get_boolean (value);
      break;
    case GTK_RECENT_CHOOSER_PROP_SHOW_TIPS:
      anjuta_recent_chooser_menu_set_show_tips (menu, g_value_get_boolean (value));
      break;
    case GTK_RECENT_CHOOSER_PROP_SHOW_ICONS:
      priv->show_icons = g_value_get_boolean (value);
      break;
    case GTK_RECENT_CHOOSER_PROP_SELECT_MULTIPLE:
      g_warning ("%s: Choosers of type `%s' do not support selecting multiple items.",
                 G_STRFUNC,
                 G_OBJECT_TYPE_NAME (object));
      break;
    case GTK_RECENT_CHOOSER_PROP_LOCAL_ONLY:
      priv->local_only = g_value_get_boolean (value);
      break;
    case GTK_RECENT_CHOOSER_PROP_LIMIT:
      priv->limit = g_value_get_int (value);
      break;
    case GTK_RECENT_CHOOSER_PROP_SORT_TYPE:
      chooser_set_sort_type (menu, g_value_get_enum (value));
      break;
    case GTK_RECENT_CHOOSER_PROP_FILTER:
      anjuta_recent_chooser_menu_set_current_filter (menu, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
anjuta_recent_chooser_menu_get_property (GObject    *object,
				      guint       prop_id,
				      GValue     *value,
				      GParamSpec *pspec)
{
  AnjutaRecentChooserMenu *menu = ANJUTA_RECENT_CHOOSER_MENU (object);
  AnjutaRecentChooserMenuPrivate *priv = menu->priv;
  
  switch (prop_id)
    {
    case GTK_RECENT_CHOOSER_PROP_SHOW_TIPS:
      g_value_set_boolean (value, priv->show_tips);
      break;
    case GTK_RECENT_CHOOSER_PROP_LIMIT:
      g_value_set_int (value, priv->limit);
      break;
    case GTK_RECENT_CHOOSER_PROP_LOCAL_ONLY:
      g_value_set_boolean (value, priv->local_only);
      break;
    case GTK_RECENT_CHOOSER_PROP_SORT_TYPE:
      g_value_set_enum (value, priv->sort_type);
      break;
    case GTK_RECENT_CHOOSER_PROP_SHOW_PRIVATE:
      g_value_set_boolean (value, priv->show_private);
      break;
    case GTK_RECENT_CHOOSER_PROP_SHOW_NOT_FOUND:
      g_value_set_boolean (value, priv->show_not_found);
      break;
    case GTK_RECENT_CHOOSER_PROP_SHOW_ICONS:
      g_value_set_boolean (value, priv->show_icons);
      break;
    case GTK_RECENT_CHOOSER_PROP_SELECT_MULTIPLE:
      g_value_set_boolean (value, FALSE);
      break;
    case GTK_RECENT_CHOOSER_PROP_FILTER:
      g_value_set_object (value, priv->current_filter);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static gboolean
anjuta_recent_chooser_menu_set_current_uri (GtkRecentChooser  *chooser,
					 const gchar       *uri,
					 GError           **error)
{
  AnjutaRecentChooserMenu *menu = ANJUTA_RECENT_CHOOSER_MENU (chooser);
  GList *children, *l;
  GtkWidget *menu_item = NULL;
  gboolean found = FALSE;
  
  children = gtk_container_get_children (GTK_CONTAINER (menu));
  
  for (l = children; l != NULL; l = l->next)
    {
      GtkRecentInfo *info;
      
      menu_item = GTK_WIDGET (l->data);
      
      info = g_object_get_data (G_OBJECT (menu_item), "gtk-recent-info");
      if (!info)
        continue;
      
      if (strcmp (uri, gtk_recent_info_get_uri (info)) == 0)
        {
          gtk_menu_shell_activate_item (GTK_MENU_SHELL (menu),
	                                menu_item,
					TRUE);
	  found = TRUE;

	  break;
	}
    }

  g_list_free (children);
  
  if (!found)  
    {
      g_set_error (error, GTK_RECENT_CHOOSER_ERROR,
      		   GTK_RECENT_CHOOSER_ERROR_NOT_FOUND,
      		   _("No recently used resource found with URI `%s'"),
      		   uri);
    }
  
  return found;
}

static gchar *
anjuta_recent_chooser_menu_get_current_uri (GtkRecentChooser  *chooser)
{
  AnjutaRecentChooserMenu *menu = ANJUTA_RECENT_CHOOSER_MENU (chooser);
  GtkWidget *menu_item;
  GtkRecentInfo *info;
  
  menu_item = gtk_menu_get_active (GTK_MENU (menu));
  if (!menu_item)
    return NULL;
  
  info = g_object_get_data (G_OBJECT (menu_item), "gtk-recent-info");
  if (!info)
    return NULL;
  
  return g_strdup (gtk_recent_info_get_uri (info));
}

static gboolean
anjuta_recent_chooser_menu_select_uri (GtkRecentChooser  *chooser,
				    const gchar       *uri,
				    GError           **error)
{
  AnjutaRecentChooserMenu *menu = ANJUTA_RECENT_CHOOSER_MENU (chooser);
  GList *children, *l;
  GtkWidget *menu_item = NULL;
  gboolean found = FALSE;
  
  children = gtk_container_get_children (GTK_CONTAINER (menu));
  for (l = children; l != NULL; l = l->next)
    {
      GtkRecentInfo *info;
      
      menu_item = GTK_WIDGET (l->data);
      
      info = g_object_get_data (G_OBJECT (menu_item), "gtk-recent-info");
      if (!info)
        continue;
      
      if (0 == strcmp (uri, gtk_recent_info_get_uri (info)))
        found = TRUE;
    }

  g_list_free (children);
  
  if (!found)  
    {
      g_set_error (error, GTK_RECENT_CHOOSER_ERROR,
      		   GTK_RECENT_CHOOSER_ERROR_NOT_FOUND,
      		   _("No recently used resource found with URI `%s'"),
      		   uri);
      return FALSE;
    }
  else
    {
      gtk_menu_shell_select_item (GTK_MENU_SHELL (menu), menu_item);

      return TRUE;
    }
}

static void
anjuta_recent_chooser_menu_unselect_uri (GtkRecentChooser *chooser,
				       const gchar     *uri)
{
  AnjutaRecentChooserMenu *menu = ANJUTA_RECENT_CHOOSER_MENU (chooser);
  
  gtk_menu_shell_deselect (GTK_MENU_SHELL (menu));
}

static void
anjuta_recent_chooser_menu_select_all (GtkRecentChooser *chooser)
{
  g_warning (_("This function is not implemented for "
               "widgets of class '%s'"),
             g_type_name (G_OBJECT_TYPE (chooser)));
}

static void
anjuta_recent_chooser_menu_unselect_all (GtkRecentChooser *chooser)
{
  g_warning (_("This function is not implemented for "
               "widgets of class '%s'"),
             g_type_name (G_OBJECT_TYPE (chooser)));
}

static void
anjuta_recent_chooser_menu_set_sort_func (GtkRecentChooser  *chooser,
				       GtkRecentSortFunc  sort_func,
				       gpointer           sort_data,
				       GDestroyNotify     data_destroy)
{
  AnjutaRecentChooserMenu *menu = ANJUTA_RECENT_CHOOSER_MENU (chooser);
  AnjutaRecentChooserMenuPrivate *priv = menu->priv;
  
  if (priv->sort_data_destroy)
    {
      priv->sort_data_destroy (priv->sort_data);
      priv->sort_data_destroy = NULL;
    }
      
  priv->sort_func = NULL;
  priv->sort_data = NULL;
  priv->sort_data_destroy = NULL;
  
  if (sort_func)
    {
      priv->sort_func = sort_func;
      priv->sort_data = sort_data;
      priv->sort_data_destroy = data_destroy;
    }
}

static void
chooser_set_sort_type (AnjutaRecentChooserMenu *menu,
		       GtkRecentSortType     sort_type)
{
  if (menu->priv->sort_type == sort_type)
    return;

  menu->priv->sort_type = sort_type;
}


static GList *
anjuta_recent_chooser_menu_get_items (GtkRecentChooser *chooser)
{
  AnjutaRecentChooserMenu *menu = ANJUTA_RECENT_CHOOSER_MENU (chooser);
  AnjutaRecentChooserMenuPrivate *priv = menu->priv;

  return _gtk_recent_chooser_get_items (chooser,
                                        priv->current_filter,
                                        priv->manager,
                                        priv->sort_data);
}

static GtkRecentManager *
anjuta_recent_chooser_menu_get_recent_manager (GtkRecentChooser *chooser)
{
  AnjutaRecentChooserMenuPrivate *priv;
 
  priv = ANJUTA_RECENT_CHOOSER_MENU (chooser)->priv;
  
  return priv->manager;
}

static void
anjuta_recent_chooser_menu_add_filter (GtkRecentChooser *chooser,
				    GtkRecentFilter  *filter)
{
  AnjutaRecentChooserMenu *menu;

  menu = ANJUTA_RECENT_CHOOSER_MENU (chooser);
  
  anjuta_recent_chooser_menu_set_current_filter (menu, filter);
}

static void
anjuta_recent_chooser_menu_remove_filter (GtkRecentChooser *chooser,
				       GtkRecentFilter  *filter)
{
  AnjutaRecentChooserMenu *menu;

  menu = ANJUTA_RECENT_CHOOSER_MENU (chooser);
  
  if (filter == menu->priv->current_filter)
    {
      g_object_unref (menu->priv->current_filter);
      menu->priv->current_filter = NULL;

      g_object_notify (G_OBJECT (menu), "filter");
    }
}

static GSList *
anjuta_recent_chooser_menu_list_filters (GtkRecentChooser *chooser)
{
  AnjutaRecentChooserMenu *menu;
  GSList *retval = NULL;

  menu = ANJUTA_RECENT_CHOOSER_MENU (chooser);
 
  if (menu->priv->current_filter)
    retval = g_slist_prepend (retval, menu->priv->current_filter);

  return retval;
}

static void
anjuta_recent_chooser_menu_set_current_filter (AnjutaRecentChooserMenu *menu,
					    GtkRecentFilter      *filter)
{
  AnjutaRecentChooserMenuPrivate *priv;

  priv = menu->priv;
  
  if (priv->current_filter)
    g_object_unref (G_OBJECT (priv->current_filter));
  
  if (filter)
    {
      priv->current_filter = filter;
      g_object_ref_sink (priv->current_filter);
    }

  anjuta_recent_chooser_menu_populate (menu);
  
  g_object_notify (G_OBJECT (menu), "filter");
}

/* taken from libeel/eel-strings.c */
static gchar *
escape_underscores (const gchar *string)
{
  gint underscores;
  const gchar *p;
  gchar *q;
  gchar *escaped;

  if (!string)
    return NULL;
	
  underscores = 0;
  for (p = string; *p != '\0'; p++)
    underscores += (*p == '_');

  if (underscores == 0)
    return g_strdup (string);

  escaped = g_new (char, strlen (string) + underscores + 1);
  for (p = string, q = escaped; *p != '\0'; p++, q++)
    {
      /* Add an extra underscore. */
      if (*p == '_')
        *q++ = '_';
      
      *q = *p;
    }
  
  *q = '\0';
	
  return escaped;
}

static void
anjuta_recent_chooser_menu_add_tip (AnjutaRecentChooserMenu *menu,
				 GtkRecentInfo        *info,
				 GtkWidget            *item)
{
  AnjutaRecentChooserMenuPrivate *priv;
  gchar *path;

  g_assert (info != NULL);
  g_assert (item != NULL);

  priv = menu->priv;
  
  path = gtk_recent_info_get_uri_display (info);
  if (path)
    {
      gchar *tip_text = g_strdup_printf (_("Open '%s'"), path);

      gtk_widget_set_tooltip_text (item, tip_text);
      gtk_widget_set_has_tooltip (item, priv->show_tips);

      g_free (path);
      g_free (tip_text);
    }
}

static GtkWidget *
anjuta_recent_chooser_menu_create_item (AnjutaRecentChooserMenu *menu,
				     GtkRecentInfo        *info,
				     gint                  count)
{
  AnjutaRecentChooserMenuPrivate *priv;
  gchar *text;
  GtkWidget *item, *image, *label;
  GdkPixbuf *icon;

  g_assert (info != NULL);

  priv = menu->priv;

  if (priv->show_numbers)
    {
      gchar *name, *escaped;
      
      name = g_strdup (gtk_recent_info_get_display_name (info));
      if (!name)
        name = g_strdup (_("Unknown item"));
      
      escaped = escape_underscores (name);
      
      /* avoid clashing mnemonics */
      if (count <= 10)
        /* This is the label format that is used for the first 10 items 
         * in a recent files menu. The %d is the number of the item,
         * the %s is the name of the item. Please keep the _ in front
         * of the number to give these menu items a mnemonic.
         *
         * Don't include the prefix "recent menu label|" in the translation.
         */
        text = g_strdup_printf (Q_("recent menu label|_%d. %s"), count, escaped);
      else
        /* This is the format that is used for items in a recent files menu. 
         * The %d is the number of the item, the %s is the name of the item. 
         *
         * Don't include the prefix "recent menu label|" in the translation.
         */
        text = g_strdup_printf (Q_("recent menu label|%d. %s"), count, escaped);
      
      item = gtk_image_menu_item_new_with_mnemonic (text);
      
      g_free (escaped);
      g_free (name);
    }
  else
    {
      text = g_strdup (gtk_recent_info_get_display_name (info));
      item = gtk_image_menu_item_new_with_label (text);
    }

  g_free (text);

  /* ellipsize the menu item label, in case the recent document
   * display name is huge.
   */
  label = GTK_BIN (item)->child;
  if (GTK_IS_LABEL (label))
    {
      gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
      gtk_label_set_max_width_chars (GTK_LABEL (label), priv->label_width);
    }
  
  if (priv->show_icons)
    {
      icon = gtk_recent_info_get_icon (info, priv->icon_size);
        
      image = gtk_image_new_from_pixbuf (icon);
      gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
      g_object_unref (icon);
    }

  g_signal_connect (item, "activate",
  		    G_CALLBACK (item_activate_cb),
  		    menu);

  return item;
}

static void
anjuta_recent_chooser_menu_insert_item (AnjutaRecentChooserMenu *menu,
                                     GtkWidget            *menuitem,
                                     gint                  position,
                                     gboolean              anjuta_project)
{
  AnjutaRecentChooserMenuPrivate *priv = menu->priv;
  gint real_position;

  if (priv->first_recent_item_pos == -1)
    {
      GList *children, *l;

      children = gtk_container_get_children (GTK_CONTAINER (menu));

      for (real_position = 0, l = children;
           l != NULL;
           real_position += 1, l = l->next)
        {
          GObject *child = l->data;
          gboolean is_placeholder = FALSE;

          is_placeholder =
            GPOINTER_TO_INT (g_object_get_data (child, "gtk-recent-menu-placeholder"));

          if (is_placeholder)
            break;
        }

      g_list_free (children);
      priv->first_recent_item_pos = real_position;
      priv->prj_pos = 0;
    }
  else
    real_position = priv->first_recent_item_pos;

  if (anjuta_project)
  {
    if (priv->prj_pos != 5)
    {
      gtk_menu_shell_insert (GTK_MENU_SHELL (menu), menuitem,
                             priv->prj_pos);
      priv->prj_pos++;
    }
  } 
  else gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  
  gtk_widget_show (menuitem);
}

/* removes the items we own from the menu */
static void
anjuta_recent_chooser_menu_dispose_items (AnjutaRecentChooserMenu *menu)
{
  GList *children, *l;
 
  children = gtk_container_get_children (GTK_CONTAINER (menu));
  for (l = children; l != NULL; l = l->next)
    {
      GtkWidget *menu_item = GTK_WIDGET (l->data);
      gboolean has_mark = FALSE;
      
      /* check for our mark, in order to remove just the items we own */
      has_mark =
        GPOINTER_TO_INT (g_object_get_data (G_OBJECT (menu_item), "gtk-recent-menu-mark"));

      if (has_mark)
        {
          GtkRecentInfo *info;
          
          /* destroy the attached RecentInfo struct, if found */
          info = g_object_get_data (G_OBJECT (menu_item), "gtk-recent-info");
          if (info)
            g_object_set_data_full (G_OBJECT (menu_item), "gtk-recent-info",
            			    NULL, NULL);
          
          /* and finally remove the item from the menu */
          gtk_container_remove (GTK_CONTAINER (menu), menu_item);
        }
    }

  /* recalculate the position of the first recent item */
  menu->priv->first_recent_item_pos = -1;

  g_list_free (children);
}

typedef struct
{
  GList *items;
  gint n_items;
  gint loaded_items;
  gint displayed_items;
  AnjutaRecentChooserMenu *menu;
  GtkWidget *placeholder;
} MenuPopulateData;

static gboolean
idle_populate_func (gpointer data)
{
  MenuPopulateData *pdata;
  AnjutaRecentChooserMenuPrivate *priv;
  GtkRecentInfo *info;
  gboolean retval;
  GtkWidget *item;

  pdata = (MenuPopulateData *) data;
  priv = pdata->menu->priv;

  if (!pdata->items)
    {
      pdata->items = gtk_recent_chooser_get_items (GTK_RECENT_CHOOSER (pdata->menu));
      if (!pdata->items)
        {
          /* show the placeholder here */
          gtk_widget_show (pdata->placeholder);
          pdata->displayed_items = 1;
          priv->populate_id = 0;

	  return FALSE;
	}
      
      /* We add the separator */
      GtkWidget *menuitem;
      menuitem = gtk_separator_menu_item_new ();
      anjuta_recent_chooser_menu_insert_item (pdata->menu, menuitem,
                                       pdata->displayed_items, FALSE);
	  g_object_set_data (G_OBJECT (menuitem),
                     "gtk-recent-menu-mark",
      		     GINT_TO_POINTER (TRUE));
      
      pdata->n_items = g_list_length (pdata->items);
      pdata->loaded_items = 0;
    }

  info = g_list_nth_data (pdata->items, pdata->loaded_items);
  item = anjuta_recent_chooser_menu_create_item (pdata->menu,
                                              info,
					      pdata->displayed_items);
  if (!item)
    goto check_and_return;
      
  anjuta_recent_chooser_menu_add_tip (pdata->menu, info, item);
  if (strcmp (gtk_recent_info_get_mime_type (info), "application/x-anjuta") == 0)
    anjuta_recent_chooser_menu_insert_item (pdata->menu, item,
                                       pdata->displayed_items, TRUE);
  else
  {
    if (priv->max_files != 14)
    { 
      anjuta_recent_chooser_menu_insert_item (pdata->menu, item,
                                              pdata->displayed_items, FALSE);
	  priv->max_files++;  
	}
  }
  
  pdata->displayed_items += 1;
      
  /* mark the menu item as one of our own */
  g_object_set_data (G_OBJECT (item),
                     "gtk-recent-menu-mark",
      		     GINT_TO_POINTER (TRUE));
      
  /* attach the RecentInfo object to the menu item, and own a reference
   * to it, so that it will be destroyed with the menu item when it's
   * not needed anymore.
   */
  g_object_set_data_full (G_OBJECT (item), "gtk-recent-info",
      			  gtk_recent_info_ref (info),
      			  (GDestroyNotify) gtk_recent_info_unref);
  
check_and_return:
  pdata->loaded_items += 1;

  if (pdata->loaded_items == pdata->n_items)
    {
      g_list_foreach (pdata->items, (GFunc) gtk_recent_info_unref, NULL);
      g_list_free (pdata->items);

      priv->populate_id = 0;

      retval = FALSE;
    }
  else
    retval = TRUE;

  return retval;
}

static void
idle_populate_clean_up (gpointer data)
{
  MenuPopulateData *pdata = data;

  if (pdata->menu->priv->populate_id == 0)
    {
      /* show the placeholder in case no item survived
       * the filtering process in the idle loop
       */
      if (!pdata->displayed_items)
        gtk_widget_show (pdata->placeholder);
      g_object_unref (pdata->placeholder);

      g_slice_free (MenuPopulateData, data);
    }
}

static void
anjuta_recent_chooser_menu_populate (AnjutaRecentChooserMenu *menu)
{
  MenuPopulateData *pdata;
  AnjutaRecentChooserMenuPrivate *priv = menu->priv;

  if (menu->priv->populate_id)
    return;

  pdata = g_slice_new (MenuPopulateData);
  pdata->items = NULL;
  pdata->n_items = 0;
  pdata->loaded_items = 0;
  pdata->displayed_items = 0;
  pdata->menu = menu;
  pdata->placeholder = g_object_ref (priv->placeholder);

  priv->icon_size = get_icon_size_for_widget (GTK_WIDGET (menu));
  priv->prj_pos = 0;
  priv->max_files = 0;
  
  /* remove our menu items first and hide the placeholder */
  anjuta_recent_chooser_menu_dispose_items (menu);
  gtk_widget_hide (priv->placeholder);
  
  priv->populate_id = gdk_threads_add_idle_full (G_PRIORITY_HIGH_IDLE + 30,
  					         idle_populate_func,
					         pdata,
                                                 idle_populate_clean_up);
}

/* bounce activate signal from the recent menu item widget 
 * to the recent menu widget
 */
static void
item_activate_cb (GtkWidget *widget,
		  gpointer   user_data)
{
  GtkRecentChooser *chooser = GTK_RECENT_CHOOSER (user_data);
  
  g_signal_emit_by_name (chooser, "item_activated");
}

/* we force a redraw if the manager changes when we are showing */
static void
manager_changed_cb (GtkRecentManager *manager,
		    gpointer          user_data)
{
  AnjutaRecentChooserMenu *menu = ANJUTA_RECENT_CHOOSER_MENU (user_data);

  anjuta_recent_chooser_menu_populate (menu);
}

static void
set_recent_manager (AnjutaRecentChooserMenu *menu,
		    GtkRecentManager     *manager)
{
  AnjutaRecentChooserMenuPrivate *priv = menu->priv;

  if (priv->manager)
    {
      if (priv->manager_changed_id)
        {
          g_signal_handler_disconnect (priv->manager, priv->manager_changed_id);
          priv->manager_changed_id = 0;
        }

      if (priv->populate_id)
        {
          g_source_remove (priv->populate_id);
          priv->populate_id = 0;
        }

      priv->manager = NULL;
    }
  
  if (manager)
    priv->manager = manager;
  else
    priv->manager = gtk_recent_manager_get_default ();
  
  if (priv->manager)
    priv->manager_changed_id = g_signal_connect (priv->manager, "changed",
                                                 G_CALLBACK (manager_changed_cb),
                                                 menu);
}

static gint
get_icon_size_for_widget (GtkWidget *widget)
{
  GtkSettings *settings;
  gint width, height;

  if (gtk_widget_has_screen (widget))
    settings = gtk_settings_get_for_screen (gtk_widget_get_screen (widget));
  else
    settings = gtk_settings_get_default ();

  if (gtk_icon_size_lookup_for_settings (settings, GTK_ICON_SIZE_MENU,
                                         &width, &height))
    return MAX (width, height);

  return FALLBACK_ICON_SIZE;
}

static void
foreach_set_shot_tips (GtkWidget *widget,
                       gpointer   user_data)
{
  AnjutaRecentChooserMenu *menu = user_data;
  AnjutaRecentChooserMenuPrivate *priv = menu->priv;
  gboolean has_mark;

  /* toggle the tooltip only on the items we create */
  has_mark =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "gtk-recent-menu-mark"));

  if (has_mark)
    gtk_widget_set_has_tooltip (widget, priv->show_tips);
}

static void
anjuta_recent_chooser_menu_set_show_tips (AnjutaRecentChooserMenu *menu,
				       gboolean              show_tips)
{
  AnjutaRecentChooserMenuPrivate *priv = menu->priv;

  if (priv->show_tips == show_tips)
    return;
  
  priv->show_tips = show_tips;
  gtk_container_foreach (GTK_CONTAINER (menu), foreach_set_shot_tips, menu);
}

/*
 * Public API
 */

/**
 * anjuta_recent_chooser_menu_new:
 *
 * Creates a new #AnjutaRecentChooserMenu widget.
 *
 * This kind of widget shows the list of recently used resources as
 * a menu, each item as a menu item.  Each item inside the menu might
 * have an icon, representing its MIME type, and a number, for mnemonic
 * access.
 *
 * This widget implements the #GtkRecentChooser interface.
 *
 * This widget creates its own #GtkRecentManager object.  See the
 * anjuta_recent_chooser_menu_new_for_manager() function to know how to create
 * a #AnjutaRecentChooserMenu widget bound to another #GtkRecentManager object.
 *
 * Return value: a new #AnjutaRecentChooserMenu
 *
 * Since: 2.10
 */
GtkWidget *
anjuta_recent_chooser_menu_new (void)
{
  return g_object_new (ANJUTA_TYPE_RECENT_CHOOSER_MENU,
  		       "recent-manager", NULL,
  		       NULL);
}

/**
 * anjuta_recent_chooser_menu_new_for_manager:
 * @manager: a #GtkRecentManager
 *
 * Creates a new #AnjutaRecentChooserMenu widget using @manager as
 * the underlying recently used resources manager.
 *
 * This is useful if you have implemented your own recent manager,
 * or if you have a customized instance of a #GtkRecentManager
 * object or if you wish to share a common #GtkRecentManager object
 * among multiple #GtkRecentChooser widgets.
 *
 * Return value: a new #AnjutaRecentChooserMenu, bound to @manager.
 *
 * Since: 2.10
 */
GtkWidget *
anjuta_recent_chooser_menu_new_for_manager (GtkRecentManager *manager)
{
  g_return_val_if_fail (manager == NULL || GTK_IS_RECENT_MANAGER (manager), NULL);
  
  return g_object_new (ANJUTA_TYPE_RECENT_CHOOSER_MENU,
  		       "recent-manager", manager,
  		       NULL);
}
