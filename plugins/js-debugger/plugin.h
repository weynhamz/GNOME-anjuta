/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    Copyright (C) 2009 Maxim Ermilov   <zaspire@rambler.ru>

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

#ifndef _JS_DEBUGGER_H_
#define _JS_DEBUGGER_H_

#include <glib.h>
#include <libanjuta/anjuta-plugin.h>
G_BEGIN_DECLS

extern GType js_debugger_get_type (GTypeModule *module);
#define ANJUTA_TYPE_PLUGIN_JSDBG         (js_debugger_get_type (NULL))
#define ANJUTA_PLUGIN_JSDBG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_JSDBG, JSDbg))
#define ANJUTA_PLUGIN_JSDBG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_JSDBG, JSDbgClass))
#define ANJUTA_IS_PLUGIN_JSDBG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_JSDbg))
#define ANJUTA_IS_PLUGIN_JSDBG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_JSDBG))
#define ANJUTA_PLUGIN_JSDBG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_JSDBG, JSDbgClass))

typedef struct _JSDbg JSDbg;
typedef struct _JSDbgClass JSDbgClass;

G_END_DECLS

#endif
