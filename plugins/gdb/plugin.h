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
#include "libanjuta/glue-plugin.h"

G_BEGIN_DECLS

extern GType gdb_plugin_get_type (GluePlugin *plugin);
#define ANJUTA_TYPE_PLUGIN_GDB         (gdb_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_GDB(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_GDB, GdbPlugin))
#define ANJUTA_PLUGIN_GDB_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_GDB, GdbPluginClass))
#define ANJUTA_IS_PLUGIN_GDB(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_GDB))
#define ANJUTA_IS_PLUGIN_GDB_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_GDB))
#define ANJUTA_PLUGIN_GDB_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_GDB, GdbPluginClass))

typedef struct _GdbPlugin GdbPlugin;
typedef struct _GdbPluginClass GdbPluginClass;

G_END_DECLS

#endif
