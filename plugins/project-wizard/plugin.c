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
#include <libanjuta/interfaces/ianjuta-wizard.h>

#include "plugin.h"

#include "druid.h"

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
//	NPWPlugin *w_plugin = (NPWPlugin*)plugin;
	
	g_message ("Project Wizard Plugin: Activating project wizard plugin...");

	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
//	NPWPlugin *w_plugin = (NPWPlugin*)plugin;

	g_message ("Project Wizard Plugin: Deactivating project wizard plugin...");

	return TRUE;
}

static void
npw_plugin_instance_init (GObject *obj)
{
	NPWPlugin *plugin = (NPWPlugin*)obj;

	plugin->druid = NULL;
	plugin->install = NULL;
	plugin->view = NULL;
}

static void
npw_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

//	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
}

static void
iwizard_activate (IAnjutaWizard *wiz, GError **err)
{
	NPWPlugin *plugin = (NPWPlugin *)wiz;
	
	if (plugin->install != NULL)
	{
		// New project wizard is busy copying project file
	}
	else if (plugin->druid == NULL)
	{
		// Create a new project wizard druid
		npw_druid_new(plugin);
	}

	if (plugin->druid != NULL)
	{
		// New project wizard druid is waiting for user inputs
		npw_druid_show(plugin->druid);
	}
}

static void
iwizard_iface_init (IAnjutaWizardIface *iface)
{
	iface->activate = iwizard_activate;
}

ANJUTA_PLUGIN_BEGIN (NPWPlugin, npw_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE(iwizard, IANJUTA_TYPE_WIZARD);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (NPWPlugin, npw_plugin);
