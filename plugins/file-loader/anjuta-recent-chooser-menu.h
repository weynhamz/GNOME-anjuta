/* GTK - The GIMP Toolkit
 * gtkrecentchoosermenu.h - Recently used items menu widget
 * Copyright (C) 2006, Emmanuele Bassi
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __ANJUTA_RECENT_CHOOSER_MENU_H__
#define __ANJUTA_RECENT_CHOOSER_MENU_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define ANJUTA_TYPE_RECENT_CHOOSER_MENU		(anjuta_recent_chooser_menu_get_type ())
#define ANJUTA_RECENT_CHOOSER_MENU(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_RECENT_CHOOSER_MENU, AnjutaRecentChooserMenu))
#define ANJUTA_IS_RECENT_CHOOSER_MENU(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_RECENT_CHOOSER_MENU))
#define ANJUTA_RECENT_CHOOSER_MENU_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_RECENT_CHOOSER_MENU, AnjutaRecentChooserMenuClass))
#define ANJUTA_IS_RECENT_CHOOSER_MENU_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_RECENT_CHOOSER_MENU))
#define ANJUTA_RECENT_CHOOSER_MENU_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_RECENT_CHOOSER_MENU, AnjutaRecentChooserMenuClass))

typedef struct _AnjutaRecentChooserMenu		AnjutaRecentChooserMenu;
typedef struct _AnjutaRecentChooserMenuClass	AnjutaRecentChooserMenuClass;
typedef struct _AnjutaRecentChooserMenuPrivate	AnjutaRecentChooserMenuPrivate;

struct _AnjutaRecentChooserMenu
{
  /*< private >*/
  GtkMenu parent_instance;

  AnjutaRecentChooserMenuPrivate *priv;
};

struct _AnjutaRecentChooserMenuClass
{
  GtkMenuClass parent_class;
  
  /* padding for future expansion */
  void (* gtk_recent1) (void);
  void (* gtk_recent2) (void);
  void (* gtk_recent3) (void);
  void (* gtk_recent4) (void);
};

GType      anjuta_recent_chooser_menu_get_type         (void) G_GNUC_CONST;

GtkWidget *anjuta_recent_chooser_menu_new              (void);
GtkWidget *anjuta_recent_chooser_menu_new_for_manager  (GtkRecentManager     *manager);

gboolean   anjuta_recent_chooser_menu_get_show_numbers (AnjutaRecentChooserMenu *menu);
void       anjuta_recent_chooser_menu_set_show_numbers (AnjutaRecentChooserMenu *menu,
						     gboolean              show_numbers);

G_END_DECLS

#endif /* ! __ANJUTA_RECENT_CHOOSER_MENU_H__ */
