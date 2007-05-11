/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* egg-entry-action widget
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
#ifndef EGG_ENTRY_ACTION_H
#define EGG_ENTRY_ACTION_H

#include <gtk/gtk.h>
// #include <libegg/menu/egg-action.h>

#define EGG_TYPE_ENTRY_ACTION            (egg_entry_action_get_type ())
#define EGG_ENTRY_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EGG_TYPE_ENTRY_ACTION, EggEntryAction))
#define EGG_ENTRY_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), EGG_TYPE_ENTRY_ACTION, EggEntryActionClass))
#define EGG_IS_ENTRY_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EGG_TYPE_ENTRY_ACTION))
#define EGG_IS_ENTRY_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), EGG_TYPE_ENTRY_ACTION))
#define EGG_ENTRY_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), EGG_TYPE_ENTRY_ACTION, EggEntryActionClass))

typedef struct _EggEntryAction      EggEntryAction;
typedef struct _EggEntryActionClass EggEntryActionClass;

struct _EggEntryAction
{
  GtkAction parent;

  gchar* text;
  gint   width;
};

struct _EggEntryActionClass
{
  GtkActionClass parent_class;

  /* Signals */
  void (* changed)   (EggEntryAction *action);
  void (* focus_in)  (EggEntryAction *action);
  void (* focus_out) (EggEntryAction *action);
};

GType    egg_entry_action_get_type   (void);

void     egg_entry_action_changed (EggEntryAction *action);
void     egg_entry_action_set_text (EggEntryAction *action,
				    const gchar *text);
const gchar * egg_entry_action_get_text (EggEntryAction *action);

#endif
