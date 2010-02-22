/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) 2008 Ignacio Casal Quinteiro

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
#include <libanjuta/anjuta-encodings.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>

#include "plugin.h"
#include "starter.h"


static gpointer parent_class;

/* Remove the starter plugin once a document was opened */
static void
on_value_added_current_editor (AnjutaPlugin *plugin, const gchar *name,
							   const GValue *value, gpointer data)
{
	GObject* doc = g_value_get_object (value);
	AnjutaShell* shell = ANJUTA_PLUGIN(plugin)->shell;
	StarterPlugin* splugin = ANJUTA_PLUGIN_STARTER (plugin);
	
	if (doc)
	{
		if (splugin->starter)
		{
			DEBUG_PRINT ("Hiding starter");
			anjuta_shell_remove_widget (shell, splugin->starter, NULL);
		}
		splugin->starter = NULL;
	}
}

static void
on_value_removed_current_editor (AnjutaPlugin *plugin, const gchar *name,
								 gpointer data)
{
	AnjutaShell* shell = ANJUTA_PLUGIN(plugin)->shell;
	StarterPlugin* splugin = ANJUTA_PLUGIN_STARTER (plugin);
	IAnjutaDocumentManager* docman = anjuta_shell_get_interface (shell,
	                                                             IAnjutaDocumentManager,
	                                                             NULL);
	
	if (!ianjuta_document_manager_get_doc_widgets (docman,
	                                               NULL))
	{
		DEBUG_PRINT ("Showing starter");
		splugin->starter = GTK_WIDGET (starter_new (shell));
		anjuta_shell_add_widget (shell, splugin->starter,
		                         "AnjutaStarter",
		                         _("Starter"),
		                         GTK_STOCK_ABOUT,
		                         ANJUTA_SHELL_PLACEMENT_CENTER,
		                         NULL);
		anjuta_shell_present_widget (shell, splugin->starter, NULL);
	}
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	StarterPlugin* splugin = ANJUTA_PLUGIN_STARTER (plugin);

	DEBUG_PRINT ("StarterPlugin: Activating document manager plugin...");
	
	splugin->editor_watch_id = 
		anjuta_plugin_add_watch (plugin,
								  IANJUTA_DOCUMENT_MANAGER_CURRENT_DOCUMENT,
								 on_value_added_current_editor,
								 on_value_removed_current_editor,
								 NULL);
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	
	DEBUG_PRINT ("StarterPlugin: Deactivating starter plugin...");
	if (ANJUTA_PLUGIN_STARTER (plugin)->starter)
		anjuta_shell_remove_widget (plugin->shell, ANJUTA_PLUGIN_STARTER (plugin)->starter, NULL);
	return TRUE;
}

static void
dispose (GObject *obj)
{
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
finalize (GObject *obj)
{
	/* Finalization codes here */
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
starter_plugin_instance_init (GObject *obj)
{
}

static void
starter_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
	klass->finalize = finalize;
}

ANJUTA_PLUGIN_BOILERPLATE (StarterPlugin, starter_plugin);
ANJUTA_SIMPLE_PLUGIN (StarterPlugin, starter_plugin);
