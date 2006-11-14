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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/interfaces/ianjuta-editor.h>

extern GType cpp_java_plugin_type;
#define CPP_JAVA_PLUGIN_TYPE (cpp_java_plugin_type)
#define CPP_JAVA_PLUGIN(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), CPP_JAVA_PLUGIN_TYPE, CppJavaPlugin))

typedef struct _CppJavaPlugin CppJavaPlugin;
typedef struct _CppJavaPluginClass CppJavaPluginClass;

struct _CppJavaPlugin {
	AnjutaPlugin parent;
	
	GtkActionGroup *action_group;
	gint uiid;
	
	AnjutaPreferences *prefs;
	gint editor_watch_id;
	GObject *current_editor;
	gboolean support_installed;
	const gchar *current_language;
	
	/* Adaptive indentation parameters */
	gint param_tab_size;
	gint param_use_spaces;
	gint param_statement_indentation;
	gint param_brace_indentation;
	gint param_case_indentation;
	gint param_label_indentation;
};

struct _CppJavaPluginClass {
	AnjutaPluginClass parent_class;
};
