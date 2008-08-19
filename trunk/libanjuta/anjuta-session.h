/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-session.h
 * Copyright (c) 2005 Naba Kumar  <naba@gnome.org>
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _ANJUTA_SESSION_H_
#define _ANJUTA_SESSION_H_

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define ANJUTA_TYPE_SESSION         (anjuta_session_get_type ())
#define ANJUTA_SESSION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_SESSION, AnjutaSession))
#define ANJUTA_SESSION_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), ANJUTA_TYPE_SESSION, AnjutaSessionClass))
#define ANJUTA_IS_SESSION(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_SESSION))
#define ANJUTA_IS_SESSION_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_SESSION))
#define ANJUTA_SESSION_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_SESSION, AnjutaSessionClass))

typedef struct _AnjutaSessionPriv AnjutaSessionPriv;

typedef enum
{
	ANJUTA_SESSION_PHASE_START,
	ANJUTA_SESSION_PHASE_FIRST,
	ANJUTA_SESSION_PHASE_NORMAL,
	ANJUTA_SESSION_PHASE_LAST,
	ANJUTA_SESSION_PHASE_END,
} AnjutaSessionPhase;

typedef struct {
	GObject parent;
	AnjutaSessionPriv *priv;
} AnjutaSession;

typedef struct {
	GObjectClass parent_class;
	/* Add Signal Functions Here */
} AnjutaSessionClass;

GType anjuta_session_get_type (void);
AnjutaSession *anjuta_session_new (const gchar *session_directory);

gchar* anjuta_session_get_session_filename (AnjutaSession *session);
const gchar* anjuta_session_get_session_directory (AnjutaSession *session);

void anjuta_session_sync (AnjutaSession *session);
void anjuta_session_clear (AnjutaSession *session);
void anjuta_session_clear_section (AnjutaSession *session,
								   const gchar *section);

void anjuta_session_set_int (AnjutaSession *session, const gchar *section,
							 const gchar *key, gint value);
void anjuta_session_set_float (AnjutaSession *session, const gchar *section,
							   const gchar *key, gfloat value);
void anjuta_session_set_string (AnjutaSession *session, const gchar *section,
								const gchar *key, const gchar *value);
void anjuta_session_set_string_list (AnjutaSession *session,
									 const gchar *section,
									 const gchar *key, GList *value);

gint anjuta_session_get_int (AnjutaSession *session, const gchar *section,
							 const gchar *key);
gfloat anjuta_session_get_float (AnjutaSession *session, const gchar *section,
								 const gchar *key);
gchar* anjuta_session_get_string (AnjutaSession *session, const gchar *section,
								  const gchar *key);
GList* anjuta_session_get_string_list (AnjutaSession *session,
									   const gchar *section,
									   const gchar *key);
									   
G_END_DECLS

#endif /* _ANJUTA_SESSION_H_ */
