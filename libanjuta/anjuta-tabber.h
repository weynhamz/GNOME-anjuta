/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-notebook-tabber
 * Copyright (C) Johannes Schmid 2010 <jhs@gnome.org>
 * 
 * anjuta-notebook-tabber is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta-notebook-tabber is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _ANJUTA_TABBER_H_
#define _ANJUTA_TABBER_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define ANJUTA_TYPE_TABBER             (anjuta_tabber_get_type ())
#define ANJUTA_TABBER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_TABBER, AnjutaTabber))
#define ANJUTA_TABBER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_TABBER, AnjutaTabberClass))
#define ANJUTA_IS_TABBER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_TABBER))
#define ANJUTA_IS_TABBER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_TABBER))
#define ANJUTA_TABBER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_TABBER, AnjutaTabberClass))

typedef struct _AnjutaTabberClass AnjutaTabberClass;
typedef struct _AnjutaTabber AnjutaTabber;
typedef struct _AnjutaTabberPriv AnjutaTabberPriv;

struct _AnjutaTabberClass
{
	GtkContainerClass parent_class;
};

struct _AnjutaTabber
{
	GtkContainer parent_instance;

	AnjutaTabberPriv* priv;
};

GType anjuta_tabber_get_type (void) G_GNUC_CONST;

GtkWidget* anjuta_tabber_new (GtkNotebook* notebook);
void anjuta_tabber_add_tab (AnjutaTabber* tabber, GtkWidget* tab_label);

G_END_DECLS

#endif /* _ANJUTA_TABBER_H_ */
