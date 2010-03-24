/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/* pkg-config.h
 *
 * Copyright (C) 2005  Naba Kumar
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _PKG_CONFIG_H_
#define _PKG_CONFIG_H_

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

enum {
	COL_PKG_PACKAGE,
	COL_PKG_DESCRIPTION,
	N_PKG_COLUMNS
};

GtkListStore *packages_get_pkgconfig_list (void);

G_END_DECLS

#endif /* _PKG_CONFIG_H_ */
