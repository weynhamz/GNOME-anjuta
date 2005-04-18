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
	1) (P) Add new menu items associated with external commands.
	2) (N) Add drop-down toolbar item for easy access to all tools.
	3) (P) Should be appendable under any of the top/sub level menus.
	4) (N) Should be able to associate icons.
	5) (R) Should be able to associate shortcuts.

R2: Command line parameters
	1) (D) Pass variable command-line parameters to the tool.
	2) (D) Use existing properties system to pass parameters.
	3) (N) Ask user at run-time for parameters.

R3: Working directory
	1) (D) Specify working directory for the tool.
	2) (D) Ability to specify property variables for working dir.

R4: Standard input to the tool
	1) (D) Specify current buffer as standard input.
	2) (D) Specify property variables as standard input.
	3) (N) Specify list of open files as standard input.

R5: Output and error redirection
	1) (D) Output to any of the message pane windows.
	2) (D) Run in terminal (detached mode).
	3) (D) Output to current/new buffer.
	4) (D) Show output in a popup window.

R6: Tool manipulation GUI
	1) (D) Add/remove tools with all options.
	2) (D) Enable/disable tool loading.

R7: Tool Storage
	1) (D) Gloabal and local tool storage.
	2) (R) Override global tools with local settings.
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

/*---------------------------------------------------------------------------*/

#define ICON_FILE "anjuta-tools-plugin.png"

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

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-tools.ui"

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
		 
/* Access plugin variable
 *---------------------------------------------------------------------------*/

/*GtkDialog*
atp_plugin_get_dialog (const ATPPlugin* this)
{
	return this->dialog;
}

void
atp_plugin_set_dialog (ATPPlugin* this, GtkDialog* dialog)
{
	if (this->dialog != NULL) gtk_widget_destroy (GTK_WIDGET(this->dialog));

	this->dialog = dialog;
}

void
atp_plugin_show_dialog (const ATPPlugin* this)
{
	gtk_widget_show (GTK_WIDGET(this->dialog));
	gtk_window_set_transient_for (GTK_WINDOW (this->dialog), GTK_WINDOW (ANJUTA_PLUGIN (this)->shell));
}

void
atp_plugin_close_dialog (ATPPlugin *this)
{
	gtk_widget_destroy (GTK_WIDGET(this->dialog));
	this->dialog = NULL;
	this->view = NULL;
}

GtkTreeView*
atp_plugin_get_treeview (const ATPPlugin* this)
{
	return this->view;
}

void
atp_plugin_set_treeview (ATPPlugin* this, GtkTreeView* view)
{
	if (this->view != NULL) gtk_widget_destroy (GTK_WIDGET(view));

	this->view = view;
}*/

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
	
	g_message ("Tools Plugin: Activating tools plugin...");
	
	/* Add all our actions */
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	anjuta_ui_add_action_group_entries (ui, "ActionGroupTools",
					_("Tool operations"),
					actions_tools,
					G_N_ELEMENTS (actions_tools),
					plugin);
	this->uiid = anjuta_ui_merge (ui, UI_FILE);

	/* Load tools */
	menu = GTK_MENU (gtk_menu_item_get_submenu (GTK_MENU_ITEM (gtk_ui_manager_get_widget (GTK_UI_MANAGER(ui), "/MenuMain/PlaceHolderBuildMenus/Tools"))));
	atp_tool_list_construct (&this->list, this, menu);
	atp_anjuta_tools_load (this);
	sep = gtk_separator_menu_item_new();
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), sep);
	gtk_widget_show (sep);
	
	atp_tool_list_activate (&this->list);

	/* initialize dialog box */
	atp_tool_dialog_new_at (&this->dialog, this);

	atp_variable_construct (&this->variable, plugin->shell);
	atp_context_list_construct (&this->context);
	
	return TRUE;
}

static gboolean
atp_plugin_deactivate (AnjutaPlugin *plugin)
{
	ATPPlugin *this = (ATPPlugin*)plugin;
	AnjutaUI *ui;

	g_message ("Tools Plugin: Deactivating tools plugin...");

	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	anjuta_ui_unmerge (ui, this->uiid);

	atp_tool_list_destroy (&this->list);
	atp_tool_dialog_free_at (&this->dialog);
	atp_context_list_destroy (&this->context);
	atp_variable_destroy (&this->variable);

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

/* Control access to anjuta message view to avoid a closed view
 *---------------------------------------------------------------------------*/

/*static void
on_message_buffer_flush (IAnjutaMessageView *view, const gchar *line,
						 ATPPlugin *this)
{
	atp_plugin_print_view (this, IANJUTA_MESSAGE_VIEW_TYPE_NORMAL, line, "");
}

IAnjutaMessageView* 
atp_plugin_create_view (ATPPlugin* this)
{
	if (this->view == NULL)
	{
		IAnjutaMessageManager* man;

		man = anjuta_shell_get_interface (ANJUTA_PLUGIN (this)->shell, IAnjutaMessageManager, NULL);
		this->view = ianjuta_message_manager_add_view (man, _("Anjuta Tools"), ICON_FILE, NULL);
		if (this->view != NULL)
		{
			g_signal_connect (G_OBJECT (this->view), "buffer_flushed",
							  G_CALLBACK (on_message_buffer_flush), this);
			g_object_add_weak_pointer (G_OBJECT (this->view), (gpointer *)&this->view);
		}
	}
	else
	{
		ianjuta_message_view_clear (this->view, NULL);
	}

	return this->view;
}

void
atp_plugin_append_view (ATPPlugin* this, const gchar* text)
{
	if (this->view)
	{
		ianjuta_message_view_buffer_append (this->view, text, NULL);
	}
}

void
atp_plugin_print_view (ATPPlugin* this, IAnjutaMessageViewType type, const gchar* summary, const gchar* details)
{
	if (this->view)
	{
		ianjuta_message_view_append (this->view, type, summary, details, NULL);
	}
}

AnjutaLauncher*
atp_plugin_get_launcher (ATPPlugin* this)
{
	if (this->launcher == NULL)
	{
		this->launcher = anjuta_launcher_new ();
	}

	return this->launcher;
}*/
