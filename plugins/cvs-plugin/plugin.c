/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) 2000 Naba Kumar, Johannes Schmid

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

#include <libanjuta/anjuta-shell.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/anjuta-preferences.h>

#include "plugin.h"
#include "cvs-actions.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-cvs.ui"
#define ICON_FILE "anjuta-cvs-plugin.png"

static gpointer parent_class;

static GtkActionEntry actions_cvs[] = {
	{
		"ActionMenuCVS",                       /* Action name */
		NULL,                            /* Stock icon, if any */
		N_("_CVS"),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		NULL
	},
	{
		"ActionCVSAdd",                       /* Action name */
		GTK_STOCK_ADD,                            /* Stock icon, if any */
		N_("_Add"),                     /* Display label */
		NULL,                                     /* short-cut */
		N_("Add a new file/directory to the CVS tree"),                      /* Tooltip */
		G_CALLBACK (on_cvs_add_activate)    /* action callback */
	},
	{
		"ActionCVSRemove",                       /* Action name */
		GTK_STOCK_REMOVE,                            /* Stock icon, if any */
		N_("_Remove"),                     /* Display label */
		NULL,                                     /* short-cut */
		N_("Remove a file/directory from CVS tree"),                      /* Tooltip */
		G_CALLBACK (on_cvs_remove_activate)    /* action callback */
	},
	{
		"ActionCVSCommit",                       /* Action name */
		GTK_STOCK_YES,                            /* Stock icon, if any */
		N_("_Commit"),                     /* Display label */
		NULL,                                     /* short-cut */
		N_("Commit your changes to the CVS tree"),                      /* Tooltip */
		G_CALLBACK (on_cvs_commit_activate)    /* action callback */
	},
	{
		"ActionCVSUpdate",                       /* Action name */
		GTK_STOCK_REFRESH,                            /* Stock icon, if any */
		N_("_Update"),                     /* Display label */
		NULL,                                     /* short-cut */
		N_("Sync your local copy with the CVS tree"),                      /* Tooltip */
		G_CALLBACK (on_cvs_update_activate)    /* action callback */
	},
	{
		"ActionCVSDiff",                       /* Action name */
		GTK_STOCK_ZOOM_100,                            /* Stock icon, if any */
		N_("_Diff"),                     /* Display label */
		NULL,                                     /* short-cut */
		N_("Show differences between your local copy and the tree"),                      /* Tooltip */
		G_CALLBACK (on_cvs_diff_activate)    /* action callback */
	},
	{
		"ActionCVSStatus",                       /* Action name */
		GTK_STOCK_ZOOM_100,                            /* Stock icon, if any */
		N_("_Show Status"),                     /* Display label */
		NULL,                                     /* short-cut */
		N_("Show the status of a file/directory"),                      /* Tooltip */
		G_CALLBACK (on_cvs_status_activate)    /* action callback */
	},
	{
		"ActionCVSImport",                       /* Action name */
		GTK_STOCK_ADD,                            /* Stock icon, if any */
		N_("_Import Tree"),                     /* Display label */
		NULL,                                     /* short-cut */
		N_("Import a new source tree to CVS"),                      /* Tooltip */
		G_CALLBACK (on_cvs_import_activate)    /* action callback */
	}
};

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	GladeXML* gxml;
	AnjutaPreferences *prefs;
	AnjutaUI *ui;
	CVSPlugin *cvs_plugin;
	
	g_message ("CVSPlugin: Activating CVS plugin ...");
	cvs_plugin = (CVSPlugin*) plugin;
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	/* Create the messages preferences page */
	prefs = anjuta_shell_get_preferences (plugin->shell, NULL);
	gxml = glade_xml_new (GLADE_FILE, "cvs", NULL);
	anjuta_preferences_add_page (prefs, gxml, "cvs", ICON_FILE);
	g_object_unref (gxml);
	
	/* Add all our actions */
	anjuta_ui_add_action_group_entries (ui, "ActionGroupCVS",
					_("CVS operations"),
					actions_cvs,
					G_N_ELEMENTS (actions_cvs),
					plugin);
	cvs_plugin->uiid = anjuta_ui_merge (ui, UI_FILE);
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui = anjuta_shell_get_ui (plugin->shell, NULL);
	g_message ("CVSPlugin: Dectivating CVS plugin ...");
	anjuta_ui_unmerge (ui, ((CVSPlugin*)plugin)->uiid);
	return TRUE;
}

static void
finalize (GObject *obj)
{
	// CVSPlugin *plugin = (CVSPlugin*)obj;
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (G_OBJECT(obj)));
}

static void
dispose (GObject *obj)
{
	// CVSPlugin *plugin = (CVSPlugin*)obj;
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (G_OBJECT(obj)));
}

static void
cvs_plugin_instance_init (GObject *obj)
{
	CVSPlugin *plugin = (CVSPlugin*)obj;
	plugin->uiid = 0;
	plugin->executing_command = FALSE;
	plugin->mesg_view = NULL;
	plugin->launcher = NULL;
	plugin->diff_editor = NULL;
}

static void
cvs_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
	klass->finalize = finalize;
}

ANJUTA_PLUGIN_BOILERPLATE (CVSPlugin, cvs_plugin);
ANJUTA_SIMPLE_PLUGIN (CVSPlugin, cvs_plugin);
