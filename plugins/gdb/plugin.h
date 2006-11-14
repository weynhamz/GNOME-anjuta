/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) 2005 Sebastien Granjoux

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef GDB_PLUGIN_H
#define GDB_PLUGIN_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

extern GType gdb_plugin_type;
#define GDB_PLUGIN_TYPE (gdb_plugin_type)
#define GDB_PLUGIN(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GDB_PLUGIN_TYPE, GdbPlugin))

typedef struct _GdbPlugin GdbPlugin;
typedef struct _GdbPluginClass GdbPluginClass;

G_END_DECLS

#endif
