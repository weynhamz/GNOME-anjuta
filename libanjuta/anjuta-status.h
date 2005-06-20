/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-status.h Copyright (C) 2004 Naba Kumar  <naba@gnome.org>
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free 
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59 
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#ifndef _ANJUTA_STATUS_H_
#define _ANJUTA_STATUS_H_

#include <libgnomeui/gnome-appbar.h>
#include <libgnome/gnome-macros.h>

#define ANJUTA_TYPE_STATUS        (anjuta_status_get_type ())
#define ANJUTA_STATUS(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_STATUS, AnjutaStatus))
#define ANJUTA_STATUS_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), ANJUTA_TYPE_STATUS, AnjutaStatusClass))
#define ANJUTA_IS_STATUS(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_STATUS))
#define ANJUTA_IS_STATUS_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_STATUS))

typedef struct _AnjutaStatus AnjutaStatus;
typedef struct _AnjutaStatusPriv AnjutaStatusPriv;
typedef struct _AnjutaStatusClass AnjutaStatusClass;

struct _AnjutaStatus
{
	GnomeAppBar parent;
	AnjutaStatusPriv *priv;
};

struct _AnjutaStatusClass
{
	GnomeAppBarClass parent_class;
	
	/* signals */
	void (*busy) (AnjutaStatus *status, gboolean state);
};

GType anjuta_status_get_type (void);
GtkWidget* anjuta_status_new (void);

/* Status bar text manipulation */
void anjuta_status_set (AnjutaStatus *status, gchar * mesg, ...);
void anjuta_status_push (AnjutaStatus *status, gchar * mesg, ...);
#define anjuta_status_pop(obj) gnome_appbar_pop(GNOME_APPBAR((obj)));
#define anjuta_status_clear_stack(obj) gnome_appbar_clear_stack(GNOME_APPBAR((obj)));
void anjuta_status_busy_push (AnjutaStatus *status);
void anjuta_status_busy_pop (AnjutaStatus *status);
void anjuta_status_set_default (AnjutaStatus *status, const gchar *label,
								const gchar *value_format, ...);
void anjuta_status_add_widget (AnjutaStatus *status, GtkWidget *widget);

/* Status bar progress manipulation */
void anjuta_status_progress_set_splash (AnjutaStatus *status, const gchar *splash_file, gint splash_progress_position);
void anjuta_status_progress_disable_splash (AnjutaStatus *status, gboolean disable_splash);
void anjuta_status_progress_add_ticks (AnjutaStatus *status, gint ticks);
void anjuta_status_progress_tick (AnjutaStatus *status,
								  GdkPixbuf *icon, const gchar *text);
void anjuta_status_progress_reset (AnjutaStatus *status);
#endif
