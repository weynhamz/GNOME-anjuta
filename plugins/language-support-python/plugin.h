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
#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include <libanjuta/anjuta-shell.h>

#include "python-assist.h"

extern GType python_plugin_get_type (GTypeModule *module);
#define ANJUTA_TYPE_PLUGIN_PYTHON         (python_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_PYTHON(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_PYTHON, PythonPlugin))
#define ANJUTA_PLUGIN_PYTHON_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_PYTHON, PythonPluginClass))
#define ANJUTA_IS_PLUGIN_PYTHON(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_PYTHON))
#define ANJUTA_IS_PLUGIN_PYTHON_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_PYTHON))
#define ANJUTA_PLUGIN_PYTHON_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_PYTHON, PythonPluginClass))


typedef struct _PythonPlugin PythonPlugin;
typedef struct _PythonPluginClass PythonPluginClass;

struct _PythonPlugin{
	AnjutaPlugin parent;
	gint uiid;
	GtkActionGroup *action_group;

	AnjutaPreferences *prefs;
	GObject *current_editor;
	gboolean support_installed;
	const gchar *current_language;

	gchar *project_root_directory;

	/* Watches */
	gint project_root_watch_id;
	gint editor_watch_id;
	
	/* Assist */
	PythonAssist *assist;

	/* Preferences */
	GtkBuilder* bxml;
	GSettings* settings;
	GSettings* editor_settings;
};

struct _PythonPluginClass{
	AnjutaPluginClass parent_class;
};

#endif
