/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) 2004 Naba Kumar

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

#include <config.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/interfaces/ianjuta-file.h>
//#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-debugger-be.h>

#include "plugin.h"

#include "attach_process.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-debugger.ui"

gpointer parent_class;


static void on_load_file_response_ok (GtkDialog* dialog, gint id, DebuggerPlugin* plugin);

/*
static void
on_sample_action_activate (GtkAction* action, DebuggerPlugin* plugin)
{
	GObject *obj;
	IAnjutaEditor *editor;
	IAnjutaDocumentManager *docman;
*/
	/* Query for object implementing IAnjutaDocumentManager interface */
/*
	obj = anjuta_shell_get_object (ANJUTA_PLUGIN (plugin)->shell,
									  "IAnjutaDocumentManager", NULL);
	docman = IANJUTA_DOCUMENT_MANAGER (obj);
	editor = ianjuta_document_manager_get_current_editor (docman, NULL);
*/
	/* Do whatever with plugin */
/*
	anjuta_util_dialog_info (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
							 "Document manager pointer is: '0x%X'\n"
							 "Current Editor pointer is: 0x%X", docman,
							 editor);
}
*/



static void
load_file (DebuggerPlugin* plugin, gboolean executable)
{
	const gchar *title_executable = _("Load Executable File");
	const gchar *title_core = _("Load Core File");

	GtkWindow *parent = GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell);
	GtkWidget *dialog = gtk_file_chooser_dialog_new (
			executable ? title_executable : title_core,
			parent,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			NULL);
	gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER(dialog), FALSE);
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (dialog), TRUE);

	GtkFileFilter *filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("All files"));
	gtk_file_filter_add_pattern (filter, "*");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

	g_signal_connect (G_OBJECT (dialog), "response",
			G_CALLBACK (on_load_file_response_ok), plugin);
	g_signal_connect_swapped (GTK_OBJECT (dialog), "response",
			G_CALLBACK (gtk_widget_destroy), GTK_OBJECT (dialog));

	gtk_widget_show (dialog);
}

static void
on_load_file_response_ok (GtkDialog* dialog, gint id, DebuggerPlugin* plugin)
{
	if (plugin->uri != NULL)
	{
		/* TODO - somehow handle situation when an executable has already been loaded */
		return;
	}

	plugin->uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));

	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
on_start_debug_activate (GtkAction* action, DebuggerPlugin* plugin)
{
	GObject *obj;
	IAnjutaDebuggerBE *debugger_be;

	/* Query for object implementing IAnjutaDebuggerBE interface */
	obj = anjuta_shell_get_object (ANJUTA_PLUGIN (plugin)->shell,
			"IAnjutaDebuggerBE", NULL);
	debugger_be = IANJUTA_DEBUGGER_BE (obj);

	ianjuta_debugger_be_start (debugger_be, "anjuta" /* TODO */, NULL);
}

static void
on_load_exec_action_activate (GtkAction* action, DebuggerPlugin* plugin)
{
	load_file (plugin, TRUE);
}

static void
on_load_core_action_activate (GtkAction* action, DebuggerPlugin* plugin)
{
	load_file (plugin, FALSE);
}

static void
on_attach_to_project_action_activate (GtkAction* action, DebuggerPlugin* plugin)
{
	/* TODO: fix the below */
	attach_process_show (attach_process_new());
}

static GtkActionEntry actions_debug[] =
{
	{
		"ActionMenuDebug",                        /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("_Debug"),                             /* Display label */
		NULL,                                     /* short-cut */
		NULL,                                     /* Tooltip */
		NULL                                      /* action callback */
	},
	{
		"ActionStartDebugger",                    /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("_Start Debugger"),                    /* Display label */
		"<shift>F12",                             /* short-cut */
		N_("Start the debugging session"),        /* Tooltip */
		G_CALLBACK (on_start_debug_activate)      /* action callback */
	},
	{
		"ActionLoadExecutable",                   /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("Load E_xecutable ..."),               /* Display label */
		NULL,                                     /* short-cut */
		N_("Open the executable for debugging"),  /* Tooltip */
		G_CALLBACK (on_load_exec_action_activate) /* action callback */
	},
	{
		"ActionLoadCore",                         /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("Load _Core file ..."),                /* Display label */
		NULL,                                     /* short-cut */
		N_("Load a core file to dissect"),        /* Tooltip */
		G_CALLBACK (on_load_core_action_activate) /* action callback */
	},
	{
		"ActionAttachToProcess",                  /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("_Attach to Process ..."),             /* Display label */
		NULL,                                     /* short-cut */
		N_("Attach to a running program"),        /* Tooltip */
		G_CALLBACK (on_attach_to_project_action_activate) /* action callback */
	}
};

static gboolean
activate_plugin (AnjutaPlugin* plugin)
{
	AnjutaUI *ui;
	DebuggerPlugin *debugger_plugin;

	g_message ("DebuggerPlugin: Activating Debugger plugin ...");
	debugger_plugin = (DebuggerPlugin*) plugin;

	ui = anjuta_shell_get_ui (plugin->shell, NULL);

	/* Add all our debugger actions */
	anjuta_ui_add_action_group_entries (ui, "ActionGroupDebug",
										_("Debugger operations"),
										actions_debug,
										G_N_ELEMENTS (actions_debug),
										plugin);
	debugger_plugin->uiid = anjuta_ui_merge (ui, UI_FILE);
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin* plugin)
{
	AnjutaUI *ui = anjuta_shell_get_ui (plugin->shell, NULL);
	g_message ("DebuggerPlugin: Deactivating Debugger plugin ...");
	anjuta_ui_unmerge (ui, ((DebuggerPlugin *) plugin)->uiid);
	return TRUE;
}

static void
dispose (GObject* obj)
{
	DebuggerPlugin *plugin = (DebuggerPlugin *) obj;
	if (plugin->uri != NULL)
	{
		g_free (plugin->uri);
	}
}

static void
debugger_plugin_instance_init (GObject* obj)
{
	DebuggerPlugin *plugin = (DebuggerPlugin *) obj;
	plugin->uiid = 0;
	plugin->uri = NULL;
}

static void
debugger_plugin_class_init (GObjectClass* klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
}


/* Implementation of IAnjutaFile interface */
static void
ifile_open (IAnjutaFile* plugin, const gchar* uri, GError** e)
{
	DebuggerPlugin *debugger = (DebuggerPlugin *) plugin;
	if (debugger->uri == NULL)
	{
		debugger->uri = g_strdup (uri);
		anjuta_util_dialog_info (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
				"Opening executable %s\n", uri);
	}
	else
	{
		/* TODO: error message */
	}
}

static gchar*
ifile_get_uri (IAnjutaFile* plugin, GError** e)
{
	return ((DebuggerPlugin *) plugin)->uri;
}

static void
ifile_iface_init (IAnjutaFileIface* iface)
{
	iface->open = ifile_open;
	iface->get_uri = ifile_get_uri;
}


ANJUTA_PLUGIN_BEGIN (DebuggerPlugin, debugger_plugin);
ANJUTA_INTERFACE(ifile, IANJUTA_TYPE_FILE);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (DebuggerPlugin, debugger_plugin);
