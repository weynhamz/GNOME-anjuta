/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* preferences.h
 *
 * Copyright (C) 2010  Sébastien Granjoux
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef _PREFERENCES_H_
#define _PREFERENCES_H_

#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-session.h>
#include <glib.h>

G_BEGIN_DECLS

typedef struct
{
	gboolean enable;
	gchar *path;
	gchar *function;
} GdbPrettyPrinter;

GList *gdb_load_pretty_printers (AnjutaSession *session);
gboolean gdb_save_pretty_printers (AnjutaSession *session, GList *list);

void gdb_pretty_printer_free (GdbPrettyPrinter *printer);

void gdb_merge_preferences (AnjutaPreferences* prefs, GList **list);
void gdb_unmerge_preferences (AnjutaPreferences* prefs);

G_END_DECLS

#endif /* _PREFERENCES_H_ */
