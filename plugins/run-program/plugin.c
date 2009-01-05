/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
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

/*
 * Plugins functions
 *
 *---------------------------------------------------------------------------*/

#include <config.h>

#include "plugin.h"

#include "execute.h"
#include "parameters.h"

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>

#include <signal.h>

/*---------------------------------------------------------------------------*/

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-run-program.ui"

#define MAX_RECENT_ITEM	10

/* Type defintions
 *---------------------------------------------------------------------------*/

struct _RunProgramPluginClass
{
	AnjutaPluginClass parent_class;
};

/* Helper functions
 *---------------------------------------------------------------------------*/

static void
anjuta_session_set_limited_string_list (AnjutaSession *session, const gchar *section, const gchar *key, GList **value)
{
	GList *node;
	
	while ((node = g_list_nth (*value, MAX_RECENT_ITEM)) != NULL)
	{
		g_free (node->data);
		*value = g_list_delete_link (*value, node);
	}
	anjuta_session_set_string_list (session, section, key, *value);
}

static void
anjuta_session_set_strv (AnjutaSession *session, const gchar *section, const gchar *key, gchar **value)
{
	GList *list = NULL;
	
	if (value != NULL)
	{
		for (; *value != NULL; value++)
		{
			list = g_list_append (list, *value);
		}
		list = g_list_reverse (list);
	}
	
	anjuta_session_set_string_list (session, section, key, list);
	g_list_free (list);
}

static gchar**
anjuta_session_get_strv (AnjutaSession *session, const gchar *section, const gchar *key)
{
	GList *list;
	gchar **value = NULL;
	
	list = anjuta_session_get_string_list (session, section, key);
	
	if (list != NULL)
	{
		gchar **var;
		GList *node;

		value = g_new (gchar *, g_list_length (list) + 1);
		var = value;
		for (node = g_list_first (list); node != NULL; node = g_list_next (node))
		{
			*var = (gchar *)node->data;
			++var;
		}
		*var = NULL;
	}
			
	return value;
}

/* Callback for saving session
 *---------------------------------------------------------------------------*/

static void
on_session_save (AnjutaShell *shell, AnjutaSessionPhase phase, AnjutaSession *session, RunProgramPlugin *self)
{
	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;

	anjuta_session_set_limited_string_list (session, "Execution", "Program arguments", &self->recent_args);
	anjuta_session_set_limited_string_list (session, "Execution", "Program uri", &self->recent_target);
	anjuta_session_set_int (session, "Execution", "Run in terminal", self->run_in_terminal + 1);
	anjuta_session_set_string_list (session, "Execution", "Working directories", self->recent_dirs);
	anjuta_session_set_strv (session, "Execution", "Environment variables", self->environment_vars);
}

static void on_session_load (AnjutaShell *shell, AnjutaSessionPhase phase, AnjutaSession *session, RunProgramPlugin *self)
{
    gint run_in_terminal;

	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;

 	if (self->recent_args != NULL)
 	{		
 		g_list_foreach (self->recent_args, (GFunc)g_free, NULL);
 		g_list_free (self->recent_args);
 	}
 	self->recent_args = anjuta_session_get_string_list (session, "Execution", "Program arguments");

 	if (self->recent_target != NULL)
 	{		
 		g_list_foreach (self->recent_target, (GFunc)g_free, NULL);
 		g_list_free (self->recent_target);
 	}
 	self->recent_target = anjuta_session_get_string_list (session, "Execution", "Program uri");
	
	/* The flag is store as 1 == FALSE, 2 == TRUE */
	run_in_terminal = anjuta_session_get_int (session, "Execution", "Run in terminal");
	if (run_in_terminal == 0)
		self->run_in_terminal = TRUE;	/* Default value */
	else
		self->run_in_terminal = run_in_terminal - 1;
	
 	if (self->recent_dirs != NULL)
 	{		
 		g_list_foreach (self->recent_dirs, (GFunc)g_free, NULL);
 		g_list_free (self->recent_dirs);
 	}
 	self->recent_dirs = anjuta_session_get_string_list (session, "Execution", "Working directories");

	g_strfreev (self->environment_vars);
 	self->environment_vars = anjuta_session_get_strv (session, "Execution", "Environment variables");
	
	run_plugin_update_shell_value (self);
}

/* Callbacks
 *---------------------------------------------------------------------------*/

static void
on_run_program_activate (GtkAction* action, RunProgramPlugin* plugin)
{
	if (plugin->child != NULL)
	{
       gchar *msg = _("The program is running.\n"
                      	"Do you want to restart it?");
		if (anjuta_util_dialog_boolean_question (GTK_WINDOW ( ANJUTA_PLUGIN (plugin)->shell), msg))
		{
			run_plugin_kill_program (plugin, FALSE);
		}
	}
	if (plugin->recent_target == NULL)
	{
		run_parameters_dialog_run (plugin);
	}
	
	run_plugin_run_program(plugin);
}

static void
on_kill_program_activate (GtkAction* action, RunProgramPlugin* plugin)
{
	run_plugin_kill_program (plugin, TRUE);
}

static void
on_program_parameters_activate (GtkAction* action, RunProgramPlugin* plugin)
{
	/* Run as a modal dialog */
	run_parameters_dialog_run (plugin);
}

/* Actions table
 *---------------------------------------------------------------------------*/

static GtkActionEntry actions_run[] = {
	{
		"ActionMenuRun",	/* Action name */
		NULL,				/* Stock icon, if any */
		N_("_Run"),		    /* Display label */
		NULL,				/* short-cut */
		NULL,				/* Tooltip */
		NULL				/* action callback */
	},
	{
		"ActionRunProgram",
		GTK_STOCK_EXECUTE,
		N_("Execute"),
		"F3",
		N_("Run program without debugger"),
		G_CALLBACK (on_run_program_activate)
	},
	{
		"ActionStopProgram",
		GTK_STOCK_STOP,
		N_("Stop Program"),
		NULL,
		N_("Kill program"),
		G_CALLBACK (on_kill_program_activate)
	},
	{
		"ActionProgramParameters",
		NULL,
		N_("Program Parameters..."),
		NULL,
		N_("Set current program, arguments and so on"),
		G_CALLBACK (on_program_parameters_activate)
	},
};

/* AnjutaPlugin functions
 *---------------------------------------------------------------------------*/

static gboolean
run_plugin_activate (AnjutaPlugin *plugin)
{
	RunProgramPlugin *self = ANJUTA_PLUGIN_RUN_PROGRAM (plugin);
	AnjutaUI *ui;
	
	DEBUG_PRINT ("%s", "Run Program Plugin: Activating plugin...");

	/* Connect to session signal */
	g_signal_connect (plugin->shell, "save-session",
					  G_CALLBACK (on_session_save), self);
    g_signal_connect (plugin->shell, "load-session",
					  G_CALLBACK (on_session_load), self);
	
	/* Add actions */
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	self->action_group = anjuta_ui_add_action_group_entries (ui,
									"ActionGroupRun", _("Run operations"),
									actions_run, G_N_ELEMENTS (actions_run),
									GETTEXT_PACKAGE, TRUE, self);
	
	self->uiid = anjuta_ui_merge (ui, UI_FILE);

	run_plugin_update_menu_sensitivity (self);
	
	return TRUE;
}

static gboolean
run_plugin_deactivate (AnjutaPlugin *plugin)
{
	RunProgramPlugin *self = ANJUTA_PLUGIN_RUN_PROGRAM (plugin);
	AnjutaUI *ui;

	DEBUG_PRINT ("%s", "Run Program Plugin: Deactivating plugin...");
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	anjuta_ui_remove_action_group (ui, self->action_group);
	
	anjuta_ui_unmerge (ui, self->uiid);
	
	g_signal_handlers_disconnect_by_func (plugin->shell, G_CALLBACK (on_session_save), self);
    g_signal_handlers_disconnect_by_func (plugin->shell, G_CALLBACK (on_session_load), self);
	
	
	return TRUE;
}

/* GObject functions
 *---------------------------------------------------------------------------*/

/* Used in dispose and finalize */
static gpointer parent_class;

static void
run_plugin_instance_init (GObject *obj)
{
	RunProgramPlugin *self = ANJUTA_PLUGIN_RUN_PROGRAM (obj);
	
	self->recent_target = NULL;
	self->recent_args = NULL;
	self->recent_dirs = NULL;
	self->environment_vars = NULL;
	
	self->child = NULL;
	
	self->build_uri = NULL;
}

/* dispose is used to unref object created with instance_init */

static void
run_plugin_dispose (GObject *obj)
{
	RunProgramPlugin *plugin = ANJUTA_PLUGIN_RUN_PROGRAM (obj);
	
	/* Warning this function could be called several times */

	run_free_all_children (plugin);
	
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
run_plugin_finalize (GObject *obj)
{
	RunProgramPlugin *self = ANJUTA_PLUGIN_RUN_PROGRAM (obj);
	
	g_list_foreach (self->recent_target, (GFunc)g_free, NULL);
	g_list_free (self->recent_target);
	g_list_foreach (self->recent_args, (GFunc)g_free, NULL);
	g_list_free (self->recent_args);
	g_list_foreach (self->recent_args, (GFunc)g_free, NULL);
	g_list_free (self->recent_args);
	g_list_foreach (self->recent_dirs, (GFunc)g_free, NULL);
	g_list_free (self->recent_dirs);
	g_strfreev (self->environment_vars);
	
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

/* finalize used to free object created with instance init is not used */

static void
run_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = run_plugin_activate;
	plugin_class->deactivate = run_plugin_deactivate;
	klass->dispose = run_plugin_dispose;
	klass->finalize = run_plugin_finalize;
}

/* AnjutaPlugin declaration
 *---------------------------------------------------------------------------*/

ANJUTA_PLUGIN_BEGIN (RunProgramPlugin, run_plugin);
ANJUTA_PLUGIN_END;
					 
ANJUTA_SIMPLE_PLUGIN (RunProgramPlugin, run_plugin);

/* Public functions
 *---------------------------------------------------------------------------*/

void
run_plugin_update_shell_value (RunProgramPlugin *plugin)
{
	/* Update Anjuta shell value */
	anjuta_shell_add (ANJUTA_PLUGIN (plugin)->shell,
					 	RUN_PROGRAM_URI, G_TYPE_STRING, plugin->recent_target == NULL ? NULL : plugin->recent_target->data,
						RUN_PROGRAM_ARGS, G_TYPE_STRING, plugin->recent_args == NULL ? NULL : plugin->recent_args->data,
					    RUN_PROGRAM_DIR, G_TYPE_STRING, plugin->recent_dirs == NULL ? NULL : plugin->recent_dirs->data,
					  	RUN_PROGRAM_ENV, G_TYPE_STRV, plugin->environment_vars == NULL ? NULL : plugin->environment_vars,
						RUN_PROGRAM_NEED_TERM, G_TYPE_BOOLEAN, plugin->run_in_terminal,
					  	NULL);
}

void
run_plugin_update_menu_sensitivity (RunProgramPlugin *plugin)
{
	GtkAction *action;
	action = gtk_action_group_get_action (plugin->action_group, "ActionStopProgram");
	
	gtk_action_set_sensitive (action, plugin->child != NULL);
}

