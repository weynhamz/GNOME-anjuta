/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    tar.h
    Copyright (C) 2010 Sebastien Granjoux

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef __TAR_H__
#define __TAR_H__

#include <glib.h>
#include <gio/gio.h>

typedef void (*NPWTarListFunc) (GFile *tarfile, GList *list, gpointer data, GError *error);
typedef void (*NPWTarCompleteFunc) (GFile *destination, GFile *tarfile, gpointer data, GError *error);

gboolean npw_tar_list (GFile *tarfile, NPWTarListFunc list, gpointer data, GError **error);

gboolean npw_tar_extract (GFile *destination, GFile *tarfile, NPWTarCompleteFunc complete, gpointer data, GError **error);

#endif
