/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* egg-recent-action widget
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef EGG_RECENT_ACTION_H
#define EGG_RECENT_ACTION_H

#include <gtk/gtk.h>
#include <gtk/gtkaction.h>
#include <libegg/recent-files/egg-recent-model.h>

#define EGG_TYPE_RECENT_ACTION            (egg_recent_action_get_type ())
#define EGG_RECENT_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EGG_TYPE_RECENT_ACTION, EggRecentAction))
#define EGG_RECENT_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), EGG_TYPE_RECENT_ACTION, EggRecentActionClass))
#define EGG_IS_RECENT_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EGG_TYPE_RECENT_ACTION))
#define EGG_IS_RECENT_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), EGG_TYPE_RECENT_ACTION))
#define EGG_RECENT_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), EGG_TYPE_RECENT_ACTION, EggRecentActionClass))

typedef struct _EggRecentAction      EggRecentAction;
typedef struct _EggRecentActionClass EggRecentActionClass;
typedef struct _EggRecentActionPriv  EggRecentActionPriv;

struct _EggRecentAction
{
  GtkAction parent;
  EggRecentActionPriv *priv;
};

struct _EggRecentActionClass
{
  GtkActionClass parent_class;
  void (*selected) (EggRecentAction *recent_action, const gchar *uri);
};

GType    egg_recent_action_get_type (void);
void     egg_recent_action_set_model (EggRecentAction *action, EggRecentModel *model);
EggRecentModel* egg_recent_action_get_model (EggRecentAction *action);
const gchar* egg_recent_action_get_selected_uri (EggRecentAction *action);

#endif
