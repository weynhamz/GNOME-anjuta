/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*  anjuta-msgman.h (c) 2004 Johannes Schmid
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

#ifndef _ANJUTA_MSGMAN_H
#define _ANJUTA_MSGMAN_H

#include <gtk/gtknotebook.h>
#include <libanjuta/anjuta-preferences.h>
#include "message-view.h"

#define MESSAGES_TABS_POS  "messages.tab.position"

#define ANJUTA_TYPE_MSGMAN        (anjuta_msgman_get_type ())
#define ANJUTA_MSGMAN(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_MSGMAN, AnjutaMsgman))
#define ANJUTA_MSGMAN_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), ANJUTA_TYPE_MSGMAN, AnjutaMsgmanClass))
#define ANJUTA_IS_MSGMAN(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_MSGMAN))
#define ANJUTA_IS_MSGMAN_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_MSGMAN))

typedef struct _AnjutaMsgman AnjutaMsgman;
typedef struct _AnjutaMsgmanPriv AnjutaMsgmanPriv;
typedef struct _AnjutaMsgmanClass AnjutaMsgmanClass;

struct _AnjutaMsgman
{
	GtkNotebook parent;
	AnjutaMsgmanPriv *priv;
	
	/* Signal */
	void (*view_changed) (AnjutaMsgman *obj);
};

struct _AnjutaMsgmanClass
{
	GtkNotebookClass parent_class;
};

GType anjuta_msgman_get_type (void);
GtkWidget *anjuta_msgman_new (AnjutaPreferences * pref, GtkWidget* popup_menu);

MessageView *anjuta_msgman_add_view (AnjutaMsgman * msgman,
									 const gchar * name,
									 const gchar * pixmap);
void anjuta_msgman_remove_view (AnjutaMsgman * msgman, MessageView * view);
void anjuta_msgman_remove_all_views (AnjutaMsgman * msgman);

MessageView *anjuta_msgman_get_current_view (AnjutaMsgman * msgman);
MessageView *anjuta_msgman_get_view_by_name (AnjutaMsgman * msgman,
										     const gchar * name);

void anjuta_msgman_set_current_view (AnjutaMsgman * msgman, MessageView * mv);

GList *anjuta_msgman_get_all_views (AnjutaMsgman * msgman);

void anjuta_msgman_set_view_title (AnjutaMsgman *msgman, MessageView *view,
								   const gchar *title);

gboolean anjuta_msgman_serialize (AnjutaMsgman *msgman,
								  AnjutaSerializer *serializer);
gboolean anjuta_msgman_deserialize (AnjutaMsgman *msgman,
									AnjutaSerializer *serializer);

void
on_gconf_notify_message_pref (GConfClient *gclient, guint cnxn_id,
					   GConfEntry *entry, gpointer user_data);

#endif /* _ANJUTA_MSGMAN_H */
