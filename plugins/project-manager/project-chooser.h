/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */

/* project-chooser.h
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

#ifndef _PM_CHOOSER_H_
#define _PM_CHOOSER_H_

#include <gtk/gtk.h>

#include "libanjuta/anjuta-tree-combo.h"

G_BEGIN_DECLS

#define ANJUTA_TYPE_PM_CHOOSER_BUTTON            (anjuta_pm_chooser_button_get_type ())
#define ANJUTA_PM_CHOOSER_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_PM_CHOOSER_BUTTON, AnjutaPmChooserButton))
#define ANJUTA_PM_CHOOSER_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_PM_CHOOSER_BUTTON, AnjutaPmChooserButtonClass))
#define ANJUTA_IS_PM_CHOOSER_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_PM_CHOOSER_BUTTON))
#define ANJUTA_IS_PM_CHOOSER_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_PM_CHOOSER_BUTTON))
#define ANJUTA_PM_CHOOSER_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_PM_CHOOSER_BUTTON, AnjutaPmChooserButtonClass))

typedef struct _AnjutaPmChooserButton        AnjutaPmChooserButton;
typedef struct _AnjutaPmChooserButtonPrivate AnjutaPmChooserButtonPrivate;
typedef struct _AnjutaPmChooserButtonClass   AnjutaPmChooserButtonClass;

struct _AnjutaPmChooserButton
{
  AnjutaTreeComboBox parent;

  /*< private >*/
  AnjutaPmChooserButtonPrivate *priv;
};

struct _AnjutaPmChooserButtonClass
{
  /*< private >*/
  AnjutaTreeComboBoxClass parent_class;
};


GType			anjuta_pm_chooser_button_get_type (void) G_GNUC_CONST;

GtkWidget *		anjuta_pm_chooser_button_new (void);

void			anjuta_pm_chooser_button_register (GTypeModule *module);


G_END_DECLS

#endif /* _ANJUTA_PM_CHOOSER_BUTTON_H_ */
