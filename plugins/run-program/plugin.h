/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.h
    Copyright (C) 2008 Sébastien Granjoux

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

/* Anjuta Shell value set by this plugin */

#define RUN_PROGRAM_URI	"run_program_uri"
#define RUN_PROGRAM_ARGS "run_program_args"
#define RUN_PROGRAM_DIR	"run_program_directory"
#define RUN_PROGRAM_ENV	"run_program_environment"
#define RUN_PROGRAM_NEED_TERM	"run_program_need_terminal"

extern GType run_plugin_get_type 				(GTypeModule *module);
#define ANJUTA_TYPE_PLUGIN_RUN_PROGRAM			(run_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_RUN_PROGRAM(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_RUN_PROGRAM, RunProgramPlugin))
#define ANJUTA_PLUGIN_RUN_PROGRAM_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_RUN_PROGRAM, RunProgramPluginClass))
#define ANJUTA_IS_PLUGIN_RUN_PROGRAM(o)			(G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_RUN_PROGRAM))
#define ANJUTA_IS_PLUGIN_RUN_PROGRAM_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_RUN_PROGRAM))
#define ANJUTA_PLUGIN_RUN_PROGRAM_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_RUN_PROGRAM, RunProgramPluginClass))

typedef struct _RunProgramPlugin RunProgramPlugin;
typedef struct _RunProgramPluginClass RunProgramPluginClass;
typedef struct _RunProgramChild RunProgramChild;

struct _RunProgramPlugin
{
	AnjutaPlugin parent;
	
	/* Menu item */
	gint uiid;
	GtkActionGroup *action_group;
	
	/* Save data */
	gboolean run_in_terminal;
 	GList *source_dirs;
	gchar **environment_vars;
	GList *recent_target;
	GList *recent_dirs;
	GList *recent_args;	
	
	/* Child watches */
	GList *child;
	guint child_exited_connection;
	
	/* Build data */	
	gchar *build_uri;
	gpointer build_handle;
};

void run_plugin_update_shell_value (RunProgramPlugin *plugin);
void run_plugin_update_menu_sensitivity (RunProgramPlugin *plugin);

#endif
