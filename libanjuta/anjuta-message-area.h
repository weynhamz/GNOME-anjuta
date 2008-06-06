/*
 * anjuta-message-area.h
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Maggi 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA.
 */
 
/*
 * Modified by the gedit Team, 2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id$
 */

#ifndef __ANJUTA_MESSAGE_AREA_H__
#define __ANJUTA_MESSAGE_AREA_H__

#include <gtk/gtkhbox.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define ANJUTA_TYPE_MESSAGE_AREA              (anjuta_message_area_get_type())
#define ANJUTA_MESSAGE_AREA(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), ANJUTA_TYPE_MESSAGE_AREA, AnjutaMessageArea))
#define ANJUTA_MESSAGE_AREA_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), ANJUTA_TYPE_MESSAGE_AREA, AnjutaMessageAreaClass))
#define ANJUTA_IS_MESSAGE_AREA(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), ANJUTA_TYPE_MESSAGE_AREA))
#define ANJUTA_IS_MESSAGE_AREA_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_MESSAGE_AREA))
#define ANJUTA_MESSAGE_AREA_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), ANJUTA_TYPE_MESSAGE_AREA, AnjutaMessageAreaClass))

/* Private structure type */
typedef struct _AnjutaMessageAreaPrivate AnjutaMessageAreaPrivate;

/*
 * Main object structure
 */
typedef struct _AnjutaMessageArea AnjutaMessageArea;

struct _AnjutaMessageArea 
{
	GtkHBox parent;

	/*< private > */
	AnjutaMessageAreaPrivate *priv;
};

/*
 * Class definition
 */
typedef struct _AnjutaMessageAreaClass AnjutaMessageAreaClass;

struct _AnjutaMessageAreaClass 
{
	GtkHBoxClass parent_class;

	/* Signals */
	void (* response) (AnjutaMessageArea *message_area, gint response_id);

	/* Keybinding signals */
	void (* close)    (AnjutaMessageArea *message_area);	
};

/*
 * Public methods
 */
GType 		 anjuta_message_area_get_type 		(void) G_GNUC_CONST;

GtkWidget	*anjuta_message_area_new      		(const gchar       *text,
                                                 const gchar       *icon_stock_id);

void         anjuta_message_area_set_text       (AnjutaMessageArea *message_area,
                                                 const gchar *text);

void         anjuta_message_area_set_image      (AnjutaMessageArea *message_area,
                                                 const gchar *icon_stock_id);

GtkWidget	*anjuta_message_area_add_button    	(AnjutaMessageArea *message_area,
                                         		 const gchar       *button_text,
                                         		 gint               response_id);
             		 
GtkWidget	*anjuta_message_area_add_button_with_stock_image 
                                                (AnjutaMessageArea *message_area, 
                                                 const gchar       *text, 
                                                 const gchar       *stock_id, 
                                                 gint               response_id);

/* Emit response signal */
void		 anjuta_message_area_response       (AnjutaMessageArea *message_area,
                                    			 gint               response_id);

G_END_DECLS

#endif  /* __ANJUTA_MESSAGE_AREA_H__  */

