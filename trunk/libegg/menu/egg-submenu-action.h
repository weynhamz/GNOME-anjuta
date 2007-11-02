/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* egg-submenu-action widget
 *
 * Copyright (C) Naba Kumar <naba@gnome.org>
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#ifndef EGG_SUBMENU_ACTION_H
#define EGG_SUBMENU_ACTION_H

#include <gtk/gtk.h>
#include <gtk/gtkaction.h>

G_BEGIN_DECLS

#define EGG_TYPE_SUBMENU_ACTION            (egg_submenu_action_get_type ())
#define EGG_SUBMENU_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EGG_TYPE_SUBMENU_ACTION, EggSubmenuAction))
#define EGG_SUBMENU_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), EGG_TYPE_SUBMENU_ACTION, EggSubmenuActionClass))
#define EGG_IS_SUBMENU_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EGG_TYPE_SUBMENU_ACTION))
#define EGG_IS_SUBMENU_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), EGG_TYPE_SUBMENU_ACTION))
#define EGG_SUBMENU_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), EGG_TYPE_SUBMENU_ACTION, EggSubmenuActionClass))

typedef struct _EggSubmenuAction      EggSubmenuAction;
typedef struct _EggSubmenuActionClass EggSubmenuActionClass;
typedef struct _EggSubmenuActionPriv  EggSubmenuActionPriv;

typedef GtkWidget* (*EggSubmenuFactory) (gpointer user_data);

struct _EggSubmenuAction
{
  GtkAction parent;
  EggSubmenuActionPriv *priv;
};

struct _EggSubmenuActionClass
{
  GtkActionClass parent_class;
};

GType    egg_submenu_action_get_type (void);
void     egg_submenu_action_set_menu_factory (EggSubmenuAction *action,
											  EggSubmenuFactory factory,
											  gpointer user_data);
G_END_DECLS

#endif
