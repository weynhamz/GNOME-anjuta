/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*  (c) 2003 Johannes Schmid
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#ifndef MESSAGE_VIEW_H
#define MESSAGE_VIEW_H

#include <gnome.h>

/* Message View Properties:
Property |				Description
--------------------------------------------------
"label"			(gchararray) The label of the view, can be translated

"truncate"		(gboolean) Truncate messages
"mesg_first"	(guint) Trucate after n chars, ignored if 
				trucate == FALSE
"mesg_last"		(guint) Show the last n chars, ignroed if0
				truncate == FALSE

"highlight"		(gboolean) Highlite error messages
"color_warning"	(GdkColor) Color to highlite warnings
"color_error"	(GdkColor) Color to highlite errors
"color_message" (GdkColor) Color to highlite program output
*/

G_BEGIN_DECLS

#define MESSAGE_VIEW_TYPE (message_view_get_type ())
#define MESSAGE_VIEW(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), MESSAGE_VIEW_TYPE, MessageView))
#define MESSAGE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), MESSAGE_VIEW_TYPE, MessageViewClass))
#define MESSAGE_IS_VIEW(obj) (G_TYPE_CHECK_TYPE ((obj), MESSAGE_VIEW_TYPE))
#define MESSAGE_IS_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MESSAGE_VIEW_TYPE))
#define MESSAGE_VIEW_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), MESSAGE_VIEW_TYPE, MessageViewClass))

typedef struct _MessageView MessageView;
typedef struct _MessageViewClass MessageViewClass;
typedef struct _MessageViewPrivate MessageViewPrivate;

struct _MessageView
{
	GtkHBox parent;
		
	/* private */
	MessageViewPrivate* privat;
};

struct _MessageViewClass
{
	GtkHBoxClass parent;
	
	void (*message_clicked) (GObject* view, const gchar* message);
};	

GType message_view_get_type (void);
GtkWidget* message_view_new (void);

void message_view_append (MessageView* view, const gchar* message); 

gboolean message_view_select_next (MessageView* view);
gboolean message_view_select_previous (MessageView* view);

guint message_view_get_line (MessageView* view);
gchar* message_view_get_message (MessageView* view);
GList* message_view_get_messages (MessageView* view);

void message_view_clear (MessageView* view);

G_END_DECLS

#endif
