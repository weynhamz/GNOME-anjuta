/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
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

/**
Anjuta user-defined tools requirements statement:
Convention:
(M) = Must have
(R) = Recommended
(N) = Nice to have
(U) = Unecessary
(D) = Done (Implemented)
(P) = Partly implemented

User-defined tools should provide the user with a powerful mechanism to
do various activities customized to his needs from with the Anjuta GUI
framework. The tool system should also be flexible enough so that developers
of Anjuta can use it to easily integrate external programs in Anjuta and
enhance functionality of the IDE through the use of tools.

The following is a list of requirements and their relative pririties.
Progress should be tracked by marking each of the items as and when they are
implemented. Feel free to add/remove/reprioritize these items but please
discuss in the devel list before making any major changes.

R1: Modify GUI at program startup
	1) (D) Add new menu items associated with external commands.
	2) (D) Add drop-down toolbar item for easy access to all tools.
	4) (D) Should be able to associate icons.
	5) (D) Should be able to associate shortcuts.
	5) (U) Should be appendable under any of the top/sub level menus.

R2: Command line parameters
	1) (D) Pass variable command-line parameters to the tool.
	2) (D) Use existing properties system to pass parameters.
	3) (D) Ask user at run-time for parameters.
	4) (U) Generate a dialog for asking several parameters at run time

R3: Working directory
	1) (D) Specify working directory for the tool.
	2) (D) Ability to specify property variables for working dir.

R4: Standard input to the tool
	1) (D) Specify current buffer as standard input.
	2) (D) Specify property variables as standard input.
	3) (D) Specify list of open files as standard input.

R5: Output and error redirection
	1) (D) Output to a message pane windows.
	2) (D) Run in terminal (detached mode).
	3) (D) Output to current/new buffer.
	4) (D) Show output in a popup window.
	5) (U) Try to parser error and warning messages

R6: Tool manipulation GUI
	1) (D) Add/remove tools with all options.
	2) (D) Enable/disable tool loading.

R7: Tool Storage
	1) (D) Gloabal and local tool storage.
	2) (D) Override global tools with local settings.
	3) (N) Project specific tools (load/unload with project)
*/

/*
 * Plugins functions
 *
 *---------------------------------------------------------------------------*/

#include <config.h>

#include "plugin.h"

#include "dialog.h"
#include "fileop.h"
#include "tool.h"
#include "editor.h"
#include "variable.h"
#include "execute.h"

#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>

/*---------------------------------------------------------------------------*/

#define ICON_FILE "anjuta-tools-plugin.png"
#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-tools.ui"

/*---------------------------------------------------------------------------*/

struct _ATPPlugin {
	AnjutaPlugin parent;
	gint uiid;
	ATPToolList list;
	ATPToolDialog dialog;
	ATPVariable variable;
	ATPContextList context;
};

struct _ATPPluginClass {
	AnjutaPluginClass parent_class;
};

/* Call backs
 *---------------------------------------------------------------------------*/

static void
atp_on_menu_tools_configure (GtkAction* action, ATPPlugin* plugin)
{
	atp_tool_dialog_show(&plugin->dialog);
};

/*---------------------------------------------------------------------------*/

static GtkActionEntry actions_tools[] = {
	{
		"ActionMenuTools",	/* Action name */
		NULL,			/* Stock icon, if any */
		N_("_Tools"),		/* Display label */
		NULL,			/* Short-cut */
		NULL,			/* Tooltip */
		NULL			/* Callback */
	},
	{
		"ActionConfigureTools",
	 	NULL,
	 	N_("_Configure"),
	 	NULL,
	 	N_("Configure external tools"),
	 	G_CALLBACK (atp_on_menu_tools_configure)
	}
};
		 
/*---------------------------------------------------------------------------*/

/* Used in dispose */
static gpointer parent_class;

static void
atp_plugin_instance_init (GObject *obj)
{
}

/* dispose is used to unref object created with instance_init */

static void
atp_plugin_dispose (GObject *obj)
{
	/* Warning this function could be called several times */

	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (G_OBJECT (obj)));
}

/* finalize used to free object created with instance init is not used */

static gboolean
atp_plugin_activate (AnjutaPlugin *plugin)
{
	ATPPlugin *this = (ATPPlugin*)plugin;
	AnjutaUI *ui;
	GtkMenu* menu;
	GtkWidget* sep;
	
	DEBUG_PRINT ("Tools Plugin: Activating tools plugin...");
	
	/* Add all our actions */
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	anjuta_ui_add_action_group_entries (ui, "ActionGroupTools",
					_("Tool operations"),
					actions_tools,
					G_N_ELEMENTS (actions_tools),
					GETTEXT_PACKAGE, plugin);
	this->uiid = anjuta_ui_merge (ui, UI_FILE);

	/* Load tools */
	menu = GTK_MENU (gtk_menu_item_get_submenu (GTK_MENU_ITEM (gtk_ui_manager_get_widget (GTK_UI_MANAGER(ui), MENU_PLACEHOLDER))));

	/* Add a separator */
	sep = gtk_separator_menu_item_new();
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), sep);
	gtk_widget_show (sep);

	/* Add tool menu item */
	atp_tool_list_construct (&this->list, this);
	atp_anjuta_tools_load (this);
	atp_tool_list_activate (&this->list);

	/* initialization */
	atp_tool_dialog_construct (&this->dialog, this);
	atp_variable_construct (&this->variable, plugin->shell);
	atp_context_list_construct (&this->context);
	
	return TRUE;
}

static gboolean
atp_plugin_deactivate (AnjutaPlugin *plugin)
{
	ATPPlugin *this = (ATPPlugin*)plugin;
	AnjutaUI *ui;

	DEBUG_PRINT ("Tools Plugin: Deactivating tools plugin...");

	atp_context_list_destroy (&this->context);
	atp_variable_destroy (&this->variable);
	atp_tool_dialog_destroy (&this->dialog);
	atp_tool_list_destroy (&this->list);

	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	anjuta_ui_unmerge (ui, this->uiid);

	return TRUE;
}

static void
atp_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = atp_plugin_activate;
	plugin_class->deactivate = atp_plugin_deactivate;
	klass->dispose = atp_plugin_dispose;
}

ANJUTA_PLUGIN_BOILERPLATE (ATPPlugin, atp_plugin);
ANJUTA_SIMPLE_PLUGIN (ATPPlugin, atp_plugin);

/* Access plugin variables
 *---------------------------------------------------------------------------*/

GtkWindow*
atp_plugin_get_app_window (const ATPPlugin *this)
{
	return GTK_WINDOW (ANJUTA_PLUGIN (this)->shell);
}

ATPToolList*
atp_plugin_get_tool_list (const ATPPlugin* this)
{
	return &(((ATPPlugin *)this)->list);
}

ATPToolDialog*
atp_plugin_get_tool_dialog (const ATPPlugin *this)
{
	return &(((ATPPlugin *)this)->dialog);
}

ATPVariable*
atp_plugin_get_variable (const ATPPlugin *this)
{
	return &(((ATPPlugin *)this)->variable);
}

ATPContextList* 
atp_plugin_get_context_list (const ATPPlugin *this)
{
	return &(((ATPPlugin *)this)->context);
}
