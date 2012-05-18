/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * plugin.h
 * Copyright (C) Ishan Chattopadhyaya 2009 <ichattopadhyaya@gmail.com>
 * 
 * plugin.h is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * plugin.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with plugin.h.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef _PYTHON_PLUGIN_H_
#define _PYTHON_PLUGIN_H_

#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-symbol-manager.h>

extern GType indent_python_plugin_get_type (GTypeModule *module);
#define ANJUTA_TYPE_PLUGIN_INDENT_PYTHON         (indent_python_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_INDENT_PYTHON(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_INDENT_PYTHON, IndentPythonPlugin))
#define ANJUTA_PLUGIN_INDENT_PYTHON_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_INDENT_PYTHON, IndentPythonPluginClass))
#define ANJUTA_IS_PLUGIN_INDENT_PYTHON(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_INDENT_PYTHON))
#define ANJUTA_IS_PLUGIN_INDENT_PYTHON_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_INDENT_PYTHON))
#define ANJUTA_PLUGIN_INDENT_PYTHON_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_INDENT_PYTHON, IndentPythonPluginClass))


typedef struct _IndentPythonPlugin IndentPythonPlugin;
typedef struct _IndentPythonPluginClass IndentPythonPluginClass;

struct _IndentPythonPlugin{
	AnjutaPlugin parent;
	gint uiid;
	GtkActionGroup *action_group;

	AnjutaPreferences *prefs;
	GObject *current_editor;
	gboolean support_installed;
	const gchar *current_language;

	gchar *project_root_directory;
	gchar *current_editor_filename;
	gchar *current_fm_filename;

	/* Watches */
	gint editor_watch_id;
	
	/* Adaptive indentation parameters */
	gint param_tab_size;
	gint param_use_spaces;
	gint param_statement_indentation;
	gint param_brace_indentation;
	gint param_case_indentation;
	gint param_label_indentation;

	/* Preferences */
	GtkBuilder* bxml;
	GSettings* settings;
	GSettings* editor_settings;
};

struct _IndentPythonPluginClass{
	AnjutaPluginClass parent_class;
};

#endif
