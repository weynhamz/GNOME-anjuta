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

/*
 * Plugins functions
 *
 *---------------------------------------------------------------------------*/

#include <config.h>

#include "plugin.h"

#include "druid.h"

#include <libanjuta/interfaces/ianjuta-wizard.h>

/*---------------------------------------------------------------------------*/

/* Used in dispose */
static gpointer parent_class;

static void
npw_plugin_instance_init (GObject *obj)
{
	NPWPlugin *this = (NPWPlugin*)obj;

	this->druid = NULL;
	this->install = NULL;
	this->view = NULL;
}

/* dispose is used to unref object created with instance_init */

static void
npw_plugin_dispose (GObject *obj)
{
	NPWPlugin *this = (NPWPlugin*)obj;

	/* Warning this function could be called several times */
	if (this->view != NULL)
	{
		g_object_remove_weak_pointer (G_OBJECT (this->view), (gpointer*)&this->view);
		this->view = NULL;
	}

	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (G_OBJECT (obj)));
}

/* finalize used to free object created with instance init is not used */

static gboolean
npw_plugin_activate (AnjutaPlugin *plugin)
{
	GladeXML* gxml;
	NPWPlugin *this = (NPWPlugin*)plugin;
	
	g_message ("Project Wizard Plugin: Activating project wizard plugin...");

	/* Create the messages preferences page */
	this->pref = anjuta_shell_get_preferences (plugin->shell, NULL);
	gxml = glade_xml_new (GLADE_FILE, "Project Wizard", NULL);
	anjuta_preferences_add_page (this->pref, gxml, "Project Wizard", ICON_FILE);
	g_object_unref (gxml);
	
	return TRUE;
}

static gboolean
npw_plugin_deactivate (AnjutaPlugin *plugin)
{
	/*NPWPlugin *this = (NPWPlugin*)plugin; */

	g_message ("Project Wizard Plugin: Deactivating project wizard plugin...");

	/*anjuta_preferences_remove_page (this->pref, "New Project Wizard");*/

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
}

static void
iwizard_activate (IAnjutaWizard *wiz, GError **err)
{
	NPWPlugin *this = (NPWPlugin *)wiz;
	
	if (this->install != NULL)
	{
		/* New project wizard is busy copying project file */
	}
	else if (this->druid == NULL)
	{
		/* Create a new project wizard druid */
		npw_druid_new (this);
	}

	if (this->druid != NULL)
	{
		/* New project wizard druid is waiting for user inputs */
		npw_druid_show (this->druid);
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
						 NPWPlugin *this)
{
	npw_plugin_print_view (this, IANJUTA_MESSAGE_VIEW_TYPE_NORMAL, line, "");
}

IAnjutaMessageView* 
npw_plugin_create_view (NPWPlugin* this)
{
	if (this->view == NULL)
	{
		IAnjutaMessageManager* man;

		man = anjuta_shell_get_interface (ANJUTA_PLUGIN (this)->shell, IAnjutaMessageManager, NULL);
		this->view = ianjuta_message_manager_add_view (man, _("New Project Wizard"), ICON_FILE, NULL);
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
npw_plugin_append_view (NPWPlugin* this, const gchar* text)
{
	if (this->view)
	{
		ianjuta_message_view_buffer_append (this->view, text, NULL);
	}
}

void
npw_plugin_print_view (NPWPlugin* this, IAnjutaMessageViewType type, const gchar* summary, const gchar* details)
{
	if (this->view)
	{
		ianjuta_message_view_append (this->view, type, summary, details, NULL);
	}
}
