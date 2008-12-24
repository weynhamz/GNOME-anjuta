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

/*
 * Plugins functions
 *
 *---------------------------------------------------------------------------*/

#include <config.h>

#include "plugin.h"

#include "druid.h"

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-wizard.h>

/*---------------------------------------------------------------------------*/

/* Used in dispose */
static gpointer parent_class;

static void
npw_plugin_instance_init (GObject *obj)
{
	NPWPlugin *plugin = ANJUTA_PLUGIN_NPW (obj);

	plugin->druid = NULL;
	plugin->install = NULL;
	plugin->view = NULL;
}

/* dispose is used to unref object created with instance_init */

static void
npw_plugin_dispose (GObject *obj)
{
	NPWPlugin *plugin = ANJUTA_PLUGIN_NPW (obj);

	/* Warning this function could be called several times */
	if (plugin->view != NULL)
	{
		g_object_remove_weak_pointer (G_OBJECT (plugin->view),
									  (gpointer*)(gpointer)&plugin->view);
		plugin->view = NULL;
	}

	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
npw_plugin_finalize (GObject *obj)
{
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

/* finalize used to free object created with instance init is not used */

static gboolean
npw_plugin_activate (AnjutaPlugin *plugin)
{
	DEBUG_PRINT ("%s", "Project Wizard Plugin: Activating project wizard plugin...");
	return TRUE;
}

static gboolean
npw_plugin_deactivate (AnjutaPlugin *plugin)
{
	DEBUG_PRINT ("%s", "Project Wizard Plugin: Deactivating project wizard plugin...");
	return TRUE;
}

static void
npw_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = npw_plugin_activate;
	plugin_class->deactivate = npw_plugin_deactivate;
	klass->dispose = npw_plugin_dispose;
	klass->finalize = npw_plugin_finalize;
}

static void
iwizard_activate (IAnjutaWizard *wiz, GError **err)
{
	NPWPlugin *plugin = ANJUTA_PLUGIN_NPW (wiz);
	
	if (plugin->install != NULL)
	{
		/* New project wizard is busy copying project file */
	}
	else if (plugin->druid == NULL)
	{
		/* Create a new project wizard druid */
		npw_druid_new (plugin);
	}

	if (plugin->druid != NULL)
	{
		/* New project wizard druid is waiting for user inputs */
		npw_druid_show (plugin->druid);
	}
}

static void
iwizard_iface_init (IAnjutaWizardIface *iface)
{
	iface->activate = iwizard_activate;
}

ANJUTA_PLUGIN_BEGIN (NPWPlugin, npw_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE (iwizard, IANJUTA_TYPE_WIZARD);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (NPWPlugin, npw_plugin);

/* Control access to anjuta message view to avoid a closed view
 *---------------------------------------------------------------------------*/

static void
on_message_buffer_flush (IAnjutaMessageView *view, const gchar *line,
						 NPWPlugin *plugin)
{
	npw_plugin_print_view (plugin, IANJUTA_MESSAGE_VIEW_TYPE_NORMAL, line, "");
}

IAnjutaMessageView* 
npw_plugin_create_view (NPWPlugin* plugin)
{
	if (plugin->view == NULL)
	{
		IAnjutaMessageManager* man;

		man = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
										  IAnjutaMessageManager, NULL);
		plugin->view =
			ianjuta_message_manager_add_view (man, _("New Project Assistant"),
											  ICON_FILE, NULL);
		if (plugin->view != NULL)
		{
			g_signal_connect (G_OBJECT (plugin->view), "buffer_flushed",
							  G_CALLBACK (on_message_buffer_flush), plugin);
			g_object_add_weak_pointer (G_OBJECT (plugin->view),
									   (gpointer *)(gpointer)&plugin->view);
		}
	}
	else
	{
		ianjuta_message_view_clear (plugin->view, NULL);
	}

	return plugin->view;
}

void
npw_plugin_append_view (NPWPlugin* plugin, const gchar* text)
{
	if (plugin->view)
	{
		ianjuta_message_view_buffer_append (plugin->view, text, NULL);
	}
}

void
npw_plugin_print_view (NPWPlugin* plugin, IAnjutaMessageViewType type,
					   const gchar* summary, const gchar* details)
{
	if (plugin->view)
	{
		ianjuta_message_view_append (plugin->view, type, summary, details, NULL);
	}
}
