/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 *  ianjuta-message-manager.h
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

#ifndef _IANJUTA_MESSAGE_MANAGER_H_
#define _IANJUTA_MESSAGE_MANAGER_H_

#include <glib-object.h>
#include <gtk/gtkwidget.h>

#include <libanjuta/interfaces/ianjuta-message-view.h>

G_BEGIN_DECLS

#define IANJUTA_TYPE_MESSAGE_MANAGER            (ianjuta_message_manager_get_type ())
#define IANJUTA_MESSAGE_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), IANJUTA_TYPE_MESSAGE_MANAGER, IAnjutaMessageManager))
#define IANJUTA_IS_MESSAGE_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IANJUTA_TYPE_MESSAGE_MANAGER))
#define IANJUTA_MESSAGE_MANAGER_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), IANJUTA_TYPE_MESSAGE_MANAGER, IAnjutaMessageManagerIface))

#define IANJUTA_MESSAGE_MANAGER_ERROR ianjuta_message_manager_error_quark()

typedef struct _IAnjutaMessageManager       IAnjutaMessageManager;
typedef struct _IAnjutaMessageManagerIface  IAnjutaMessageManagerIface;

struct _IAnjutaMessageManagerIface {
	GTypeInterface g_iface;
	
	/* Virtual Table */
	void (*add_view) (IAnjutaMessageManager *msgman,
					  const gchar *name, const gchar *icon, GError **e);
	void (*remove_view) (IAnjutaMessageManager *msgman,
						 IAnjutaMessageView *view, GError **e);
	IAnjutaMessageView* (*get_current_view) (IAnjutaMessageManager *msgman,
											 GError **e);
	IAnjutaMessageView* (*get_view_by_name) (IAnjutaMessageManager *msgman,
											 const gchar *name, GError **e);
	GList* (*get_all_views) (IAnjutaMessageManager *msgman, GError **e);
	void (*set_current_view) (IAnjutaMessageManager *msgman,
							  IAnjutaMessageView *view, GError **e);
};

enum IAnjutaMessageManagerError {
	IANJUTA_MESSAGE_MANAGER_ERROR_DOESNT_EXIST,
};

GQuark ianjuta_message_manager_error_quark (void);
GType  ianjuta_message_manager_get_type (void);
void ianjuta_message_manager_add_view (IAnjutaMessageManager *msgman,
									   const gchar *file, const gchar *icon,
									   GError **e);
void ianjuta_message_manager_remove_view (IAnjutaMessageManager *msgman,
										  IAnjutaMessageView *view, GError **e);

IAnjutaMessageView* ianjuta_message_manager_get_current_view (IAnjutaMessageManager *msgman,
															  GError **e);
IAnjutaMessageView* ianjuta_message_manager_get_view_by_name (IAnjutaMessageManager *msgman,
															  const gchar *name,
															  GError **e);
GList* ianjuta_message_manager_get_all_views (IAnjutaMessageManager *msgman,
											  GError **e);
void ianjuta_message_manager_set_current_view (IAnjutaMessageManager *msgman,
											   IAnjutaMessageView *message_view,
											   GError **e);

G_END_DECLS

#endif
