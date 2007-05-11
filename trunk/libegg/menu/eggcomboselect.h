/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* eggcomboselect widget
 *
 * Copyright (C) Naba Kumar <naba@gnome.org>
 *
 * Codes taken from:
 * gtkcombo_select - combo_select widget for gtk+
 * Copyright 1999-2001 Adrian E. Feiguin <feiguin@ifir.edu.ar>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __EGG_COMBO_SELECT_H__
#define __EGG_COMBO_SELECT_H__

#include <gtk/gtkhbox.h>
#include <gtk/gtktreemodel.h>

#define EGG_TYPE_COMBO_SELECT            (egg_combo_select_get_type ())
#define EGG_COMBO_SELECT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EGG_TYPE_COMBO_SELECT, EggComboSelect))
#define EGG_COMBO_SELECT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), EGG_TYPE_COMBO_SELECT, EggComboSelectClass))
#define EGG_IS_COMBO_SELECT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EGG_TYPE_COMBO_SELECT))
#define EGG_IS_COMBO_SELECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), EGG_TYPE_COMBO_SELECT))
#define EGG_COMBO_SELECT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), EGG_TYPE_COMBO_SELECT, EggComboSelectClass))

typedef struct _EggComboSelect		EggComboSelect;
typedef struct _EggComboSelectClass	EggComboSelectClass;
typedef struct _EggComboSelectPriv	EggComboSelectPriv;

struct _EggComboSelect {
	GtkHBox hbox;
	EggComboSelectPriv *priv;
};

struct _EggComboSelectClass {
	GtkHBoxClass parent_class;
	/* signals */
	void (* changed)       (EggComboSelect *combo_box);
	
	/* Padding for future expansion */
	void (*_gtk_reserved0) (void);
	void (*_gtk_reserved1) (void);
	void (*_gtk_reserved2) (void);
	void (*_gtk_reserved3) (void);
};

GType      egg_combo_select_get_type              (void);

GtkWidget *egg_combo_select_new                   (void);

void	   egg_combo_select_popup     (EggComboSelect *combo_select);
void	   egg_combo_select_popdown   (EggComboSelect *combo_select);
void       egg_combo_select_set_model (EggComboSelect *combo_select, GtkTreeModel *model);
void       egg_combo_select_set_active (EggComboSelect *combo_select, gint iter_index);
gint       egg_combo_select_get_active (EggComboSelect *combo_select);
void       egg_combo_select_set_active_iter (EggComboSelect *combo_select,
											 GtkTreeIter *iter);
gboolean   egg_combo_select_get_active_iter (EggComboSelect *combo_select,
											 GtkTreeIter *iter);
void egg_combo_select_set_title (EggComboSelect *combo_select, const gchar *title);

#endif /* __EGG_COMBO_SELECT_H__ */
