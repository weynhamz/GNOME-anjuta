/*  imessage-view.c (c) 2004 Johannes Schmid
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

#include <libanjuta/interfaces/ianjuta-message-view.h>

#include "message-view.h"

/* IAnjutaMessageView interface implementation */
void
ianjuta_message_view_append (IAnjutaMessageView * message_view,
			     const gchar * message, GError ** e)
{
	message_view_append (MESSAGE_VIEW (message_view), message);
}

void
ianjuta_message_view_clear (IAnjutaMessageView * message_view, GError ** e)
{
	message_view_clear (MESSAGE_VIEW (message_view));
}

void
ianjuta_message_view_select_next (IAnjutaMessageView * message_view,
				  GError ** e)
{
	message_view_select_next (MESSAGE_VIEW (message_view));
}

void
ianjuta_message_view_select_previous (IAnjutaMessageView * message_view,
				      GError ** e)
{
	message_view_select_previous (MESSAGE_VIEW (message_view));
}

gint
ianjuta_message_view_get_line (IAnjutaMessageView * message_view, GError ** e)
{
	return message_view_get_line (MESSAGE_VIEW (message_view));
}

gchar *
ianjuta_message_view_get_message (IAnjutaMessageView * message_view,
				  GError ** e)
{
	return message_view_get_message (MESSAGE_VIEW (message_view));
}

GList *
ianjuta_message_view_get_all_messages (IAnjutaMessageView * message_view,
				       GError ** e)
{
	return message_view_get_messages (MESSAGE_VIEW (message_view));
}
