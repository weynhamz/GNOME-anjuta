/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 *  ianjuta-message-manager.c
 *  Copyright (C) 2004 Naba Kumar  <naba@gnome.org>
 *
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

#include <config.h>

#include <string.h>
#include <gobject/gvaluecollector.h>
#include <libanjuta/anjuta-marshal.h>
#include "ianjuta-message-manager.h"

GQuark
ianjuta_message_manager_error_quark (void)
{
	static GQuark quark = 0;
	
	if (quark == 0) {
		quark = g_quark_from_static_string ("ianjuta-message-manager-quark");
	}
	
	return quark;
}

void
ianjuta_message_manager_add_view (IAnjutaMessageManager *msgman,
								  const gchar *name, const gchar *icon,
								  GError **e)
{
	g_return_if_fail (msgman != NULL);
	g_return_if_fail (name != NULL);
	g_return_if_fail (icon != NULL);
	
	g_return_if_fail (IANJUTA_IS_MESSAGE_MANAGER (msgman));

	IANJUTA_MESSAGE_MANAGER_GET_IFACE (msgman)->add_view (msgman,
														  name, icon,
														  e);
}

void
ianjuta_message_manager_remove_view (IAnjutaMessageManager *msgman,
									 IAnjutaMessageView *message_view,
									 GError **e)
{
	g_return_if_fail (msgman != NULL);
	g_return_if_fail (message_view != NULL);
	
	g_return_if_fail (IANJUTA_IS_MESSAGE_MANAGER (msgman));
	g_return_if_fail (IANJUTA_IS_MESSAGE_VIEW (message_view));

	IANJUTA_MESSAGE_MANAGER_GET_IFACE (msgman)->remove_view (msgman,
															 message_view,
															 e);
}

IAnjutaMessageView*
ianjuta_message_manager_get_current_view (IAnjutaMessageManager *msgman,
										  GError **e)
{
	g_return_val_if_fail (msgman != NULL, NULL);
	g_return_val_if_fail (IANJUTA_IS_MESSAGE_MANAGER (msgman), NULL);

	return IANJUTA_MESSAGE_MANAGER_GET_IFACE (msgman)->get_current_view (msgman,
																		 e);
}

IAnjutaMessageView*
ianjuta_message_manager_get_view_by_name (IAnjutaMessageManager *msgman,
										  const gchar *name,
										  GError **e)
{
	g_return_val_if_fail (msgman != NULL, NULL);
	g_return_val_if_fail (IANJUTA_IS_MESSAGE_MANAGER (msgman), NULL);
	g_return_val_if_fail (name != NULL, NULL);
	
	return IANJUTA_MESSAGE_MANAGER_GET_IFACE (msgman)->get_view_by_name (msgman,
																		 name,
																		 e);
}

GList*
ianjuta_message_manager_get_all_views (IAnjutaMessageManager *msgman,
									   GError **e)
{
	g_return_val_if_fail (msgman != NULL, NULL);
	g_return_val_if_fail (IANJUTA_IS_MESSAGE_MANAGER (msgman), NULL);

	return IANJUTA_MESSAGE_MANAGER_GET_IFACE (msgman)->get_all_views (msgman, e);
}

void
ianjuta_message_manager_set_current_view (IAnjutaMessageManager *msgman,
										  IAnjutaMessageView *message_view,
										  GError **e)
{
	g_return_if_fail (msgman != NULL);
	g_return_if_fail (message_view != NULL);
	
	g_return_if_fail (IANJUTA_IS_MESSAGE_MANAGER (msgman));
	g_return_if_fail (IANJUTA_IS_MESSAGE_VIEW (message_view));

	IANJUTA_MESSAGE_MANAGER_GET_IFACE (msgman)->set_current_view (msgman,
																  message_view,
																  e);
}

static void
ianjuta_message_manager_base_init (gpointer gclass)
{
/*
	static gboolean initialized = FALSE;
	
	if (!initialized) {
		g_signal_new ("message-clicked",
			      IANJUTA_TYPE_MESSAGE_VIEW,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (IAnjutaMessageManagerIface, message_clicked),
			      NULL, NULL,
			      anjuta_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, G_TYPE_STRING);
	}
*/
}

GType
ianjuta_message_manager_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (IAnjutaMessageManagerIface),
			ianjuta_message_manager_base_init,
			NULL, 
			NULL,
			NULL,
			NULL,
			0,
			0,
			NULL
		};
		
		type = g_type_register_static (G_TYPE_INTERFACE, 
					       "IAnjutaMessageManager", 
					       &info,
					       0);
		
		g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
	}
	return type;			
}
