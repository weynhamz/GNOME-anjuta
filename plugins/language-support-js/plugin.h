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

#ifndef _JS_SUPPORT_PLUGIN_H_
#define _JS_SUPPORT_PLUGIN_H_

#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/anjuta-language-provider.h>

#include "database-symbol.h"

typedef struct _JSLang JSLang;
typedef struct _JSLangClass JSLangClass;

struct _JSLang{
	AnjutaPlugin parent;
	gint editor_watch_id;
	GObject *current_editor;
//	gchar *current;
//	GList *complition_cache;
//	gint uiid;
	DatabaseSymbol* symbol;
	AnjutaLanguageProvider *lang_prov;
//	GtkActionGroup *action_group;

	/* Preferences */
	GtkBuilder* bxml;
	GSettings* prefs;
};

struct _JSLangClass{
	AnjutaPluginClass parent_class;
};

GType js_support_plugin_get_type (GTypeModule *module);

#endif
