[+ autogen5 template +]
/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
[+IF (=(get "IncludeGNUHeader") "1") +]/*
    plugin.c
    Copyright (C) [+Author+]

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
*/[+ENDIF+]

#include <config.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>

#include "plugin.h"

[+IF (=(get "HasUI") "1") +]
#define UI_FILE ANJUTA_DATA_DIR"/ui/[+NameLower+].ui"
[+ENDIF+]
[+IF (=(get "HasGladeFile") "1") +]
#define GLADE_FILE ANJUTA_DATA_DIR"/ui/[+NameLower+].glade"
[+ENDIF+]

static gpointer parent_class;

[+IF (=(get "HasUI") "1") +]
static void
on_sample_action_activate (GtkAction *action, [+PluginClass+] *plugin)
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
[+ENDIF+]

static gboolean
[+NameCLower+]_activate (AnjutaPlugin *plugin)
{
[+IF (=(get "HasUI") "1") +]
	AnjutaUI *ui;
[+ENDIF+]
[+IF (=(get "HasGladeFile") "1") +]
	GtkWidget *wid;
	GladeXML *gxml;
[+ENDIF+]
	[+PluginClass+] *[+NameCLower+];
	
	g_message ("[+PluginClass+]: Activating [+PluginClass+] plugin ...");
	[+NameCLower+] = ([+PluginClass+]*) plugin;
[+IF (=(get "HasUI") "1") +]
	/* Add all UI actions and merge UI */
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	anjuta_ui_add_action_group_entries (ui, "ActionGroupFile[+NameLower+]",
										_("Sample file operations"),
										actions_file,
										G_N_ELEMENTS (actions_file),
										plugin);
	[+NameCLower+]->uiid = anjuta_ui_merge (ui, UI_FILE);
[+ENDIF+]
[+IF (=(get "HasGladeFile") "1") +]
	/* Add plugin widgets to Shell */
	gxml = glade_xml_new (GLADE_FILE, NULL, NULL);
	wid = glade_xml_get_widget (gxml, "top_container");
	[+NameCLower+]->widget = wid;
	anjuta_shell_add_widget (plugin->shell, wid,
							 "[+PluginClass+]Widget", _("[+PluginClass+] widget"), NULL,
							 ANJUTA_SHELL_PLACEMENT_BOTTOM, NULL);
	g_object_unref (gxml);
[+ENDIF+]
	return TRUE;
}

static gboolean
[+NameCLower+]_deactivate (AnjutaPlugin *plugin)
{
[+IF (=(get "HasUI") "1") +]
	AnjutaUI *ui;
[+ENDIF+]
	g_message ("[+PluginClass+]: Dectivating [+PluginClass+] plugin ...");
[+IF (=(get "HasGladeFile") "1") +]
	anjuta_shell_remove_widget (plugin->shell, (([+PluginClass+]*)plugin)->widget,
								NULL);
[+ENDIF+]
[+IF (=(get "HasUI") "1") +]
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	anjuta_ui_unmerge (ui, (([+PluginClass+]*)plugin)->uiid);
[+ENDIF+]	
	return TRUE;
}

static void
[+NameCLower+]_finalize (GObject *obj)
{
	/* Finalization codes here */
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

static void
[+NameCLower+]_dispose (GObject *obj)
{
	/* Disposition codes */
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
[+NameCLower+]_instance_init (GObject *obj)
{
	[+PluginClass+] *plugin = ([+PluginClass+]*)obj;
[+IF (=(get "HasUI") "1") +]
	plugin->uiid = 0;
[+ENDIF+]
[+IF (=(get "HasGladeFile") "1") +]
	plugin->widget = NULL;
[+ENDIF+]
}

static void
[+NameCLower+]_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = [+NameCLower+]_activate;
	plugin_class->deactivate = [+NameCLower+]_deactivate;
	klass->finalize = [+NameCLower+]_finalize;
	klass->dispose = [+NameCLower+]_dispose;
}

ANJUTA_PLUGIN_BOILERPLATE ([+PluginClass+], [+NameCLower+]);
ANJUTA_SIMPLE_PLUGIN ([+PluginClass+], [+NameCLower+]);
