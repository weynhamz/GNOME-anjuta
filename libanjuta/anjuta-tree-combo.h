/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */

/* anjuta-tree-combo.h
 *
 * Copyright (C) 2011  Sébastien Granjoux
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
 *
 */

#ifndef _ANJUTA_TREE_COMBO_H_
#define _ANJUTA_TREE_COMBO_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define ANJUTA_TYPE_TREE_COMBO_BOX            (anjuta_tree_combo_box_get_type ())
#define ANJUTA_TREE_COMBO_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_TREE_COMBO_BOX, AnjutaTreeComboBox))
#define ANJUTA_TREE_COMBO_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_TREE_COMBO_BOX, AnjutaTreeComboBoxClass))
#define ANJUTA_IS_TREE_COMBO_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_TREE_COMBO_BOX))
#define ANJUTA_IS_TREE_COMBO_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_TREE_COMBO_BOX))
#define ANJUTA_TREE_COMBO_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_TREE_COMBO_BOX, AnjutaTreeComboBoxClass))

typedef struct _AnjutaTreeComboBox        AnjutaTreeComboBox;
typedef struct _AnjutaTreeComboBoxPrivate AnjutaTreeComboBoxPrivate;
typedef struct _AnjutaTreeComboBoxClass   AnjutaTreeComboBoxClass;

struct _AnjutaTreeComboBox
{
  GtkToggleButton parent;

  /*< private >*/
  AnjutaTreeComboBoxPrivate *priv;
};

struct _AnjutaTreeComboBoxClass
{
  /*< private >*/
  GtkToggleButtonClass parent_class;

  /* signals */
  void     (* changed)           (AnjutaTreeComboBox *combo);
};


GType			anjuta_tree_combo_box_get_type         (void) G_GNUC_CONST;
GtkWidget *		anjuta_tree_combo_box_new              (void);

void			anjuta_tree_combo_box_set_model			(AnjutaTreeComboBox *combo,
						                                    GtkTreeModel *model);
GtkTreeModel*	anjuta_tree_combo_box_get_model			(AnjutaTreeComboBox *combo);

void			anjuta_tree_combo_box_set_active 		(AnjutaTreeComboBox *combo,
					                                      gint index);
void			anjuta_tree_combo_box_set_active_iter	(AnjutaTreeComboBox *combo,
                                                         GtkTreeIter *iter);
gboolean		anjuta_tree_combo_box_get_active_iter	(AnjutaTreeComboBox *combo,
					                                     GtkTreeIter *iter);

void			anjuta_tree_combo_box_set_valid_function(AnjutaTreeComboBox *combo,
                                                         GtkTreeModelFilterVisibleFunc func,
                                                         gpointer data,
                                                         GDestroyNotify destroy);
void			anjuta_tree_combo_box_set_invalid_text	(AnjutaTreeComboBox *combo,
                                                         const gchar *str);


G_END_DECLS

#endif /* _ANJUTA_TREE_COMBO_H_ */
