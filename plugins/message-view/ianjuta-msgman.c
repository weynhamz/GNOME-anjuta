/*
 * ianjuta-msgman.c (c) 2004 Johannes Schmid This program is free
 * software; you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version.  This program is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.  You should have
 * received a copy of the GNU General Public License along with this
 * program; if not, write to the Free Software Foundation, Inc., 59 Temple 
 * Place - Suite 330, Boston, MA 02111-1307, USA. 
 */

#include <libanjuta/interfaces/ianjuta-message-manager.h>

#include "anjuta-msgman.h"

/*
 * IAnjutaMessagerManager interface implementation 
 */

void
ianjuta_message_manager_add_view (IAnjutaMessageManager * msgman,
				  const gchar * file, const gchar * icon,
				  GError ** e)
{
	anjuta_msgman_add_view (ANJUTA_MSGMAN (msgman), file, icon);
}

void
ianjuta_message_manager_remove_view (IAnjutaMessageManager * msgman,
				     IAnjutaMessageView * view, GError ** e)
{
	anjuta_msgman_remove_view (ANJUTA_MSGMAN (msgman),
				   MESSAGE_VIEW (view));
}

IAnjutaMessageView
	* ianjuta_message_manager_get_current_view (IAnjutaMessageManager *
						    msgman, GError ** e)
{
	return IANJUTA_MESSAGE_VIEW (anjuta_msgman_get_current_view
				     (ANJUTA_MSGMAN (msgman)));
}

IAnjutaMessageView
	* ianjuta_message_manager_get_view_by_name (IAnjutaMessageManager *
						    msgman,
						    const gchar * name,
						    GError ** e)
{
	return IANJUTA_MESSAGE_VIEW (anjuta_msgman_get_view_by_name
				     (ANJUTA_MSGMAN (msgman), name));
}

GList *
ianjuta_message_manager_get_all_views (IAnjutaMessageManager * msgman,
				       GError ** e)
{
	return anjuta_msgman_get_all_views (ANJUTA_MSGMAN (msgman));
}

void
ianjuta_message_manager_set_current_view (IAnjutaMessageManager * msgman,
					  IAnjutaMessageView *
					  message_view, GError ** e)
{
	return anjuta_msgman_set_current_view (ANJUTA_MSGMAN (msgman),
					       MESSAGE_VIEW (message_view));
}
