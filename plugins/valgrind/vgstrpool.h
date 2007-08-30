/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Jeffrey Stedfast <fejj@ximian.com>
 *
 *  Copyright 2003 Ximian, Inc. (www.ximian.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */


#ifndef __VG_STRPOOL_H__
#define __VG_STRPOOL_H__

#include <glib.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

#ifdef ENABLE_STRPOOL
void vg_strpool_init (void);
void vg_strpool_shutdown (void);

char *vg_strpool_add (char *string, int own);

char *vg_strdup (const char *string);
char *vg_strndup (const char *string, size_t n);
void vg_strfree (char *string);
#else
#define vg_strpool_init()
#define vg_strpool_shutdown()
#define vg_strpool_add(str,own) (own ? str : g_strdup (str))
#define vg_strdup(str) g_strdup (str)
#define vg_strndup(str, n) g_strndup (str, n)
#define vg_strfree(str) g_free (str)
#endif /* ENABLE_STRPOOL */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VG_STRPOOL_H__ */
