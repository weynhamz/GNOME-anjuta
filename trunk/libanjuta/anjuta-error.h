/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-error.h
 * Copyright (C) 2007 Sebastien Granjoux  <seb.sfo@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __ANJUTA_ERROR_H__
#define __ANJUTA_ERROR_H__

#include <glib-object.h>

G_BEGIN_DECLS

/* Boxed type for GError will not be done in GLib see bug #300610 */

#ifndef G_TYPE_ERROR
#define G_TYPE_ERROR		(g_error_get_type())
#define NEED_G_ERROR_GET_TYPE
GType g_error_get_type (void) G_GNUC_CONST;
#endif

G_END_DECLS

#endif				/* __ANJUTA_ERROR_H__ */
