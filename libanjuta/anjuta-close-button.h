/*
 * anjuta-close-button.h
 *
 * Copyright (C) 2010 - Paolo Borelli
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __ANJUTA_CLOSE_BUTTON_H__
#define __ANJUTA_CLOSE_BUTTON_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define ANJUTA_TYPE_CLOSE_BUTTON		(anjuta_close_button_get_type ())
#define ANJUTA_CLOSE_BUTTON(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_CLOSE_BUTTON, AnjutaCloseButton))
#define ANJUTA_CLOSE_BUTTON_CONST(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_CLOSE_BUTTON, AnjutaCloseButton const))
#define ANJUTA_CLOSE_BUTTON_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_CLOSE_BUTTON, AnjutaCloseButtonClass))
#define ANJUTA_IS_CLOSE_BUTTON(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_CLOSE_BUTTON))
#define ANJUTA_IS_CLOSE_BUTTON_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_CLOSE_BUTTON))
#define ANJUTA_CLOSE_BUTTON_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_CLOSE_BUTTON, AnjutaCloseButtonClass))

typedef struct _AnjutaCloseButton		AnjutaCloseButton;
typedef struct _AnjutaCloseButtonClass		AnjutaCloseButtonClass;
typedef struct _AnjutaCloseButtonClassPrivate	AnjutaCloseButtonClassPrivate;

struct _AnjutaCloseButton
{
	GtkButton parent;
};

struct _AnjutaCloseButtonClass
{
	GtkButtonClass parent_class;

	AnjutaCloseButtonClassPrivate *priv;
};

GType		  anjuta_close_button_get_type (void) G_GNUC_CONST;

GtkWidget	 *anjuta_close_button_new      (void);

G_END_DECLS

#endif /* __ANJUTA_CLOSE_BUTTON_H__ */
/* ex:set ts=8 noet: */
