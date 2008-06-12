/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) 2008 Sébastien Granjoux

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

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-environment.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>

/* Constantes
 *---------------------------------------------------------------------------*/

#define ICON_FILE "anjuta-scratchbox-48.png"
#define GLADE_FILE PACKAGE_DATA_DIR"/glade/anjuta-scratchbox.glade"

#define SB_ENTRY "preferences_folder:text:/scratchbox:0:build.scratchbox.path"

#define PREF_SB_PATH "build.scratchbox.path"

/* Type defintions
 *---------------------------------------------------------------------------*/

struct _ScratchboxPluginClass
{
	AnjutaPluginClass parent_class;
};

struct _ScratchboxPlugin
{
	AnjutaPlugin parent;
	
	gchar *user_dir;
};


/* Callback for saving session
 *---------------------------------------------------------------------------*/

static void
on_session_save (AnjutaShell *shell, AnjutaSessionPhase phase, AnjutaSession *session, ScratchboxPlugin *self)
{
}

static void on_session_load (AnjutaShell *shell, AnjutaSessionPhase phase, AnjutaSession *session, ScratchboxPlugin *self)
{
}

/* Callbacks
 *---------------------------------------------------------------------------*/

/* Actions table
 *---------------------------------------------------------------------------*/

/* AnjutaPlugin functions
 *---------------------------------------------------------------------------*/

static gboolean
scratchbox_plugin_activate (AnjutaPlugin *plugin)
{
	ScratchboxPlugin *self = ANJUTA_PLUGIN_SCRATCHBOX (plugin);
	
	DEBUG_PRINT ("Scratchbox 1 Plugin: Activating plugin...");

	/* Connect to session signal */
	g_signal_connect (plugin->shell, "save-session",
					  G_CALLBACK (on_session_save), self);
    g_signal_connect (plugin->shell, "load-session",
					  G_CALLBACK (on_session_load), self);
	
	return TRUE;
}

static gboolean
scratchbox_plugin_deactivate (AnjutaPlugin *plugin)
{
	ScratchboxPlugin *self = ANJUTA_PLUGIN_SCRATCHBOX (plugin);

	DEBUG_PRINT ("Scratchbox 1 Plugin: Deactivating plugin...");
	
	g_signal_handlers_disconnect_by_func (plugin->shell, G_CALLBACK (on_session_save), self);
    g_signal_handlers_disconnect_by_func (plugin->shell, G_CALLBACK (on_session_load), self);
	
	return TRUE;
}

/* IAnjutaEnvironment implementation
 *---------------------------------------------------------------------------*/

static gboolean
ienvironment_override (IAnjutaEnvironment* environment, gchar **dir, gchar ***argvp, gchar ***envp, GError** err)
{
	ScratchboxPlugin *plugin = ANJUTA_PLUGIN_SCRATCHBOX (environment);
	AnjutaPreferences* prefs;
	gchar* sb_dir;
	gsize len;

	prefs = anjuta_shell_get_preferences (ANJUTA_PLUGIN (plugin)->shell, NULL);
	sb_dir = anjuta_preferences_get(prefs, PREF_SB_PATH);

	if (plugin->user_dir) g_free (plugin->user_dir);
	plugin->user_dir = g_strconcat (sb_dir, G_DIR_SEPARATOR_S,
						   "users", G_DIR_SEPARATOR_S,
						   g_get_user_name(), NULL);
	
	len = strlen (plugin->user_dir);
	if (strncmp (*dir, plugin->user_dir, len) == 0)
	{
		/* Build in scratchbox environment */
		gchar **new_argv;
		gsize len_argv = g_strv_length (*argvp);

		/* Add scratchbox login */
		new_argv = g_new (gchar*, len_argv + 3);
		memcpy (new_argv + 2, *argvp, sizeof(gchar *) * (len_argv + 1));
		new_argv[0] = g_strconcat (sb_dir, G_DIR_SEPARATOR_S, "login", NULL);
		new_argv[1] = g_strconcat ("-d ", (*dir) + len, NULL);
		
		g_free (*argvp);
		*argvp = new_argv;
	}
	g_free (sb_dir);
	
	return TRUE;
}

static gchar*
ienvironment_get_real_directory (IAnjutaEnvironment* environment, gchar *dir, GError** err)
{
	ScratchboxPlugin *plugin = ANJUTA_PLUGIN_SCRATCHBOX (environment);

	if (plugin->user_dir)
	{
		gchar *real_dir;
		
		real_dir = g_strconcat(plugin->user_dir, dir, NULL);
		g_free (dir);
	
		return real_dir;
	}
	else
	{
		return dir;
	}
}

static void
ienvironment_iface_init(IAnjutaEnvironmentIface* iface)
{
	iface->override = ienvironment_override;
	iface->get_real_directory = ienvironment_get_real_directory;
}

/* IAnjutaPreferences implementation
 *---------------------------------------------------------------------------*/

static GladeXML *gxml;

static void
ipreferences_merge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	GtkWidget *sb_entry;
		
	/* Create the preferences page */
	gxml = glade_xml_new (GLADE_FILE, "preferences_dialog_scratchbox", NULL);
	sb_entry = glade_xml_get_widget(gxml, SB_ENTRY);
	
	anjuta_preferences_add_page (prefs, gxml, "Scratchbox", _("Scratchbox"),  ICON_FILE);
}

static void
ipreferences_unmerge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	GtkWidget *sb_entry;

	sb_entry = glade_xml_get_widget(gxml, SB_ENTRY);
		
	anjuta_preferences_remove_page(prefs, _("Scratchbox"));
	
	g_object_unref (gxml);
}

static void
ipreferences_iface_init(IAnjutaPreferencesIface* iface)
{
	iface->merge = ipreferences_merge;
	iface->unmerge = ipreferences_unmerge;	
}

/* GObject functions
 *---------------------------------------------------------------------------*/

/* Used in dispose and finalize */
static gpointer parent_class;

static void
scratchbox_plugin_instance_init (GObject *obj)
{
	ScratchboxPlugin *plugin = ANJUTA_PLUGIN_SCRATCHBOX (obj);
	
	plugin->user_dir = NULL;
}

/* dispose is used to unref object created with instance_init */

static void
scratchbox_plugin_dispose (GObject *obj)
{
	ScratchboxPlugin *plugin = ANJUTA_PLUGIN_SCRATCHBOX (obj);
	
	/* Warning this function could be called several times */

	if (plugin->user_dir)
	{
		g_free (plugin->user_dir);
		plugin->user_dir = NULL;
	}
	
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
scratchbox_plugin_finalize (GObject *obj)
{
	/*ScratchboxPlugin *self = ANJUTA_PLUGIN_SCRATCHBOX (obj);*/
	
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

/* finalize used to free object created with instance init is not used */

static void
scratchbox_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = scratchbox_plugin_activate;
	plugin_class->deactivate = scratchbox_plugin_deactivate;
	klass->dispose = scratchbox_plugin_dispose;
	klass->finalize = scratchbox_plugin_finalize;
}

/* AnjutaPlugin declaration
 *---------------------------------------------------------------------------*/

ANJUTA_PLUGIN_BEGIN (ScratchboxPlugin, scratchbox_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE (ienvironment, IANJUTA_TYPE_ENVIRONMENT);
ANJUTA_PLUGIN_ADD_INTERFACE (ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_END;
			 
ANJUTA_SIMPLE_PLUGIN (ScratchboxPlugin, scratchbox_plugin);

/* Public functions
*---------------------------------------------------------------------------*/

