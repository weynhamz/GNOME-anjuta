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

#include <config.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>

#include "plugin.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-sample.ui"

gpointer parent_class;

static void
on_sample_action_activate (GtkAction *action, SamplePlugin *plugin)
{
	GObject *obj;
	IAnjutaEditor *editor;
	IAnjutaDocumentManager *docman;
	
	/* Query for object implementing IAnjutaDocumentManager interface */
	obj = anjuta_shell_get_object (ANJUTA_PLUGIN (plugin)->shell,
									  "IAnjutaDocumentManager", NULL);
	docman = IANJUTA_DOCUMENT_MANAGER (obj);
	editor = ianjuta_document_manager_get_current_editor (docman, NULL);

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

static void
activate_plugin (AnjutaPlugin *plugin)
{
	GtkWidget *wid;
	AnjutaUI *ui;
	SamplePlugin *sample_plugin;
	
	g_message ("SamplePlugin: Activating Sample plugin ...");
	sample_plugin = (SamplePlugin*) plugin;
	ui = plugin->ui;
	wid = gtk_label_new ("This is a sample plugin");
	sample_plugin->widget = wid;
	
	/* Add all our editor actions */
	anjuta_ui_add_action_group_entries (ui, "ActionGroupSampleFile",
										_("Sample file operations"),
										actions_file,
										G_N_ELEMENTS (actions_file),
										plugin);
	sample_plugin->uiid = anjuta_ui_merge (plugin->ui, UI_FILE);
	anjuta_shell_add_widget (plugin->shell, wid,
				  "AnjutaSamplePlugin", _("SamplePlugin"), NULL);
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	g_message ("SamplePlugin: Dectivating Sample plugin ...");
	anjuta_shell_remove_widget (plugin->shell, ((SamplePlugin*)plugin)->widget,
								NULL);
	anjuta_ui_unmerge (plugin->ui, ((SamplePlugin*)plugin)->uiid);
	return TRUE;
}

static void
dispose (GObject *obj)
{
	// SamplePlugin *plugin = (SamplePlugin*)obj;
}

static void
sample_plugin_instance_init (GObject *obj)
{
	SamplePlugin *plugin = (SamplePlugin*)obj;
	plugin->uiid = 0;
}

static void
sample_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
}

ANJUTA_PLUGIN_BOILERPLATE (SamplePlugin, sample_plugin);
ANJUTA_SIMPLE_PLUGIN (SamplePlugin, sample_plugin);
