/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.h
    Copyright (C) 2000 Naba Kumar

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

#ifndef _PLUGIN_H_
#define _PLUGIN_H_

#include <libanjuta/anjuta-plugin.h>
#include "parser-cxx-assist.h"

extern GType parser_cxx_plugin_get_type (GTypeModule *module);
#define ANJUTA_TYPE_PLUGIN_PARSER_CXX         (parser_cxx_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_PARSER_CXX(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_PARSER_CXX, ParserCxxPlugin))
#define ANJUTA_PLUGIN_PARSER_CXX_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_PARSER_CXX, ParserCxxPluginClass))
#define ANJUTA_IS_PLUGIN_PARSER_CXX(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_PARSER_CXX))
#define ANJUTA_IS_PLUGIN_PARSER_CXX_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_PARSER_CXX))
#define ANJUTA_PLUGIN_PARSER_CXX_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_PARSER_CXX, ParserCxxPluginClass))

typedef struct _ParserCxxPlugin ParserCxxPlugin;
typedef struct _ParserCxxPluginClass ParserCxxPluginClass;

struct _ParserCxxPlugin {
	AnjutaPlugin parent;
	
	GSettings* settings;
	GSettings* editor_settings;
	gint editor_watch_id;
	gboolean support_installed;
	GObject *current_editor;
	const gchar *current_language;

	/* Assist */
	ParserCxxAssist *assist;
	
	/* Preferences */
	GtkBuilder* bxml;
};

struct _ParserCxxPluginClass {
	AnjutaPluginClass parent_class;
};

#endif
