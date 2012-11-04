/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-application.h
 * Copyright (C) 2000 Naba Kumar  <naba@gnome.org>
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA 
 */

#ifndef _ANJUTA_APPLICATION_H_
#define _ANJUTA_APPLICATION_H_

#include <libanjuta/e-splash.h>
#include "anjuta-window.h"

#define ANJUTA_TYPE_APPLICATION        (anjuta_application_get_type ())
#define ANJUTA_APPLICATION(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_APPLICATION, AnjutaApplication))
#define ANJUTA_APPLICATION_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), ANJUTA_TYPE_APPLICATION, AnjutaApplicationClass))
#define ANJUTA_IS_APPLICATION(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_APPLICATION))
#define ANJUTA_IS_APPLICATION_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_APPLICATION))

typedef struct _AnjutaApplication AnjutaApplication;
typedef struct _AnjutaApplicationClass AnjutaApplicationClass;
typedef struct _AnjutaApplicationPrivate AnjutaApplicationPrivate;

struct _AnjutaApplicationClass
{
	GtkApplicationClass parent;
};

struct _AnjutaApplication
{
	GtkApplication parent;

	AnjutaApplicationPrivate *priv;	
};

GType anjuta_application_get_type (void);

AnjutaApplication *anjuta_application_new (void);

gboolean anjuta_application_get_proper_shutdown (AnjutaApplication *app);

AnjutaWindow* anjuta_application_create_window (AnjutaApplication *app);

#endif
