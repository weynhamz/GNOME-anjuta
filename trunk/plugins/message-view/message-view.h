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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
 
#ifndef MESSAGE_VIEW_H
#define MESSAGE_VIEW_H

#include <gnome.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-serializer.h>

/* Message View Properties:
Property |				Description
--------------------------------------------------
"label"			(gchararray) The label of the view, can be translated

"truncate"		(gboolean) Truncate messages
"highlight"		(gboolean) Highlite error messages
*/

G_BEGIN_DECLS

#define MESSAGE_VIEW_TYPE        (message_view_get_type ())
#define MESSAGE_VIEW(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MESSAGE_VIEW_TYPE, MessageView))
#define MESSAGE_VIEW_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), MESSAGE_VIEW_TYPE, MessageViewClass))
#define MESSAGE_IS_VIEW(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MESSAGE_VIEW_TYPE))
#define MESSAGE_IS_VIEW_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MESSAGE_VIEW_TYPE))

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
};	

/* Note: MessageView implements IAnjutaMessageView interface */
GType message_view_get_type (void);
GtkWidget* message_view_new (AnjutaPreferences* prefs, GtkWidget* popup_menu);

void message_view_next(MessageView* view);
void message_view_previous(MessageView* view);
void message_view_save(MessageView* view);
gboolean message_view_serialize (MessageView *view,
								 AnjutaSerializer *serializer);
gboolean message_view_deserialize (MessageView *view,
								   AnjutaSerializer *serializer);

G_END_DECLS

#endif
