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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <config.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>

#include "plugin.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-sample.ui"

static gpointer parent_class;

static void
on_sample_action_activate (GtkAction *action, SamplePlugin *plugin)
{
	GObject *obj;
	IAnjutaDocument *editor;
	IAnjutaDocumentManager *docman;
	
	/* Query for object implementing IAnjutaDocumentManager interface */
	obj = anjuta_shell_get_object (ANJUTA_PLUGIN (plugin)->shell,
									  "IAnjutaDocumentManager", NULL);
	docman = IANJUTA_DOCUMENT_MANAGER (obj);
	editor = ianjuta_document_manager_get_current_document (docman, NULL);

	/* Do whatever with plugin */
	anjuta_util_dialog_info (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
							 "Document manager pointer is: '0x%X'\n"
							 "Current Editor pointer is: 0x%X", docman,
							 editor);
}

static GtkActionEntry actions_file[] = {
	{
		"ActionFileSample",                       /* Action name */
		GTK_STOCK_NEW,                            /* Stock icon, if any */
		N_("_Sample action"),                     /* Display label */
		NULL,                                     /* short-cut */
		N_("Sample action"),                      /* Tooltip */
		G_CALLBACK (on_sample_action_activate)    /* action callback */
	}
};

static gboolean
sample_plugin_activate_plugin (AnjutaPlugin *plugin)
{
	GtkWidget *wid;
	AnjutaUI *ui;
	SamplePlugin *sample_plugin;
	
	DEBUG_PRINT ("SamplePlugin: Activating Sample plugin ...");
	sample_plugin = ANJUTA_PLUGIN_SAMPLE (plugin);
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	wid = gtk_label_new ("This is a sample plugin");
	sample_plugin->widget = wid;
	
	/* Add all our editor actions */
	anjuta_ui_add_action_group_entries (ui, "ActionGroupSampleFile",
										_("Sample file operations"),
										actions_file,
										G_N_ELEMENTS (actions_file),
										GETTEXT_PACKAGE, TRUE, plugin);
	sample_plugin->uiid = anjuta_ui_merge (ui, UI_FILE);
	anjuta_shell_add_widget (plugin->shell, wid,
							 "AnjutaSamplePlugin", _("SamplePlugin"), NULL,
							 ANJUTA_SHELL_PLACEMENT_BOTTOM, NULL);
	return TRUE;
}

static gboolean
sample_plugin_deactivate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui = anjuta_shell_get_ui (plugin->shell, NULL);
	DEBUG_PRINT ("SamplePlugin: Dectivating Sample plugin ...");
	anjuta_shell_remove_widget (plugin->shell, ANJUTA_PLUGIN_SAMPLE (plugin)->widget,
								NULL);
	anjuta_ui_unmerge (ui, ANJUTA_PLUGIN_SAMPLE (plugin)->uiid);
	return TRUE;
}

static void
sample_plugin_finalize (GObject *obj)
{
	/* Finalization codes here */
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

static void
sample_plugin_dispose (GObject *obj)
{
	/* Disposition codes */
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
anjuta_sample_plugin_instance_init (GObject *obj)
{
	SamplePlugin *plugin = ANJUTA_PLUGIN_SAMPLE (obj);
	plugin->uiid = 0;
}

static void
anjuta_sample_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = sample_plugin_activate_plugin;
	plugin_class->deactivate = sample_plugin_deactivate_plugin;
	klass->finalize = sample_plugin_finalize;
	klass->dispose = sample_plugin_dispose;
}

ANJUTA_PLUGIN_BOILERPLATE (SamplePlugin, anjuta_sample_plugin);
ANJUTA_SIMPLE_PLUGIN (SamplePlugin, anjuta_sample_plugin);
