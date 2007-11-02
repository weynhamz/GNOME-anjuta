/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-error.c
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

#include "anjuta-error.h"

#ifdef NEED_G_ERROR_GET_TYPE

GType
g_error_get_type (void)
{
	static GType type = G_TYPE_INVALID;

	if (type == G_TYPE_INVALID)
	{
		type = g_boxed_type_register_static ("GError",
			       	(GBoxedCopyFunc) g_error_copy,
				(GBoxedFreeFunc) g_error_free);
	}

	return type;
}

#endif
