/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* egg-combo-action widget
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
#ifndef EGG_COMBO_ACTION_H
#define EGG_COMBO_ACTION_H

#include <gtk/gtk.h>
#include <libegg/menu/egg-entry-action.h>

#define EGG_TYPE_COMBO_ACTION            (egg_combo_action_get_type ())
#define EGG_COMBO_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EGG_TYPE_COMBO_ACTION, EggComboAction))
#define EGG_COMBO_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), EGG_TYPE_COMBO_ACTION, EggComboActionClass))
#define EGG_IS_COMBO_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EGG_TYPE_COMBO_ACTION))
#define EGG_IS_COMBO_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), EGG_TYPE_COMBO_ACTION))
#define EGG_COMBO_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), EGG_TYPE_COMBO_ACTION, EggComboActionClass))

typedef struct _EggComboAction      EggComboAction;
typedef struct _EggComboActionClass EggComboActionClass;
typedef struct _EggComboActionPriv  EggComboActionPriv;

struct _EggComboAction
{
  GtkAction parent;
  EggComboActionPriv *priv;
};

struct _EggComboActionClass
{
  GtkActionClass parent_class;
  void (*changed) (EggComboAction *combo_action, GtkTreeIter *active_iter); 
};

GType    egg_combo_action_get_type (void);
void     egg_combo_action_set_model (EggComboAction *action, GtkTreeModel *model);
GtkTreeModel* egg_combo_action_get_model (EggComboAction *action);
gboolean egg_combo_action_get_active_iter (EggComboAction *action,
										   GtkTreeIter *iter);
void egg_combo_action_set_active_iter (EggComboAction *action,
				       GtkTreeIter* iter);

#endif
