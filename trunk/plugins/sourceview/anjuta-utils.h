/*
 * anjuta-utils.h
 * This file is part of anjuta
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi
 * Copyright (C) 2002 - 2005 Paolo Maggi
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA 02110-1301, USA.
 */
 
/*
 * Modified by the anjuta Team, 1998-2005. See the AUTHORS file for a 
 * list of people on the anjuta Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id$
 */

#ifndef __GEDIT_UTILS_H__
#define __GEDIT_UTILS_H__

#include <glib.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkaboutdialog.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtktextiter.h>
#include <atk/atk.h>
#include "anjuta-encodings.h"

G_BEGIN_DECLS

/* useful macro */
#define GBOOLEAN_TO_POINTER(i) ((gpointer) ((i) ? 2 : 1))
#define GPOINTER_TO_BOOLEAN(i) ((gboolean) ((((gint)(i)) == 2) ? TRUE : FALSE))

#define IS_VALID_BOOLEAN(v) (((v == TRUE) || (v == FALSE)) ? TRUE : FALSE)

enum { GEDIT_ALL_WORKSPACES = 0xffffffff };

gboolean	 anjuta_utils_uri_has_writable_scheme	(const gchar *uri);
gboolean	 anjuta_utils_uri_has_file_scheme	(const gchar *uri);

gboolean	 g_utf8_caselessnmatch			(const char *s1,
							 const char *s2,
							 gssize n1,
							 gssize n2);

gboolean	 anjuta_utils_uri_exists			(const gchar* text_uri);

gchar		*anjuta_utils_make_valid_utf8		(const char *name);

gboolean	 anjuta_utils_is_valid_uri		(const gchar *uri);

G_END_DECLS

#endif /* __ANJUTA_UTILS_H__ */


