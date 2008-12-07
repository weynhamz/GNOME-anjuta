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

#include "plugin.h"
#include "starter.h"


static gpointer parent_class;

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	GtkWidget *starter;

	DEBUG_PRINT ("StarterPlugin: Activating document manager plugin...");

	AnjutaShell* shell = ANJUTA_PLUGIN(plugin)->shell;

	/*
	 * Adding starter
	 */
	starter = GTK_WIDGET (starter_new (shell));
	gtk_widget_show (starter);
	ANJUTA_PLUGIN_STARTER (plugin)->starter = starter;
	
	anjuta_shell_add_widget (shell, starter,
	                         "AnjutaStarter",
	                         _("Starter"),
	                         GTK_STOCK_ABOUT,
	                         ANJUTA_SHELL_PLACEMENT_CENTER,
	                         NULL);
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	
	DEBUG_PRINT ("StarterPlugin: Deactivating starter plugin...");
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
