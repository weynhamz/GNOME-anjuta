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
#include <libanjuta/interfaces/ianjuta-help.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-message-manager.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include <libanjuta/interfaces/ianjuta-profile.h>
#include <libanjuta/plugins.h>

#include "plugin.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-default-profile.ui"

gpointer parent_class;

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	return TRUE;
}

static void
dispose (GObject *obj)
{
}

static void
default_profile_plugin_instance_init (GObject *obj)
{
}

static void
default_profile_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
}

static void
iprofile_load (IAnjutaProfile *profile, ESplash *splash, GError **err)
{
	gint i, max_icons;
	static const gchar * interfaces[] = 
	{
		"IAnjutaFileLoader",
		"IAnjutaDocumentManager",
		"IAnjutaHelp",
		"IAnjutaMessageManager",
		"IAnjutaFileManager",
		"IAnjutaTerminal",
		"IAnjutaBuildable",
		NULL
	};
	max_icons = 0;
	
	if (splash) {
		i = 0;
		while(interfaces[i]) {
			GSList *plugin_descs = NULL;
			gchar *icon_filename;
			gchar *icon_path = NULL;
			
			plugin_descs = anjuta_plugins_query (ANJUTA_PLUGIN(profile)->shell,
												 "Anjuta Plugin",
												 "Interfaces", interfaces[i],
												 NULL);
			if (plugin_descs) {
				GdkPixbuf *icon_pixbuf;
				AnjutaPluginDescription *desc =
					(AnjutaPluginDescription*)plugin_descs->data;
				if (anjuta_plugin_description_get_string (desc,
														  "Anjuta Plugin",
														  "Icon",
														  &icon_filename)) {
					icon_path = g_strconcat (PACKAGE_PIXMAPS_DIR"/",
											 icon_filename, NULL);
				    // g_message ("Icon: %s", icon_path);
					icon_pixbuf = 
						gdk_pixbuf_new_from_file (icon_path, NULL);
					g_free (icon_path);
					if (icon_pixbuf) {
						e_splash_add_icon (splash, icon_pixbuf);
						max_icons++;
						g_object_unref (icon_pixbuf);
					}
					// while (gtk_events_pending ())
					//	gtk_main_iteration ();
				} else {
					g_warning ("Plugin does not define Icon: No such file %s",
								icon_path);
				}
				g_slist_free (plugin_descs);
			}
			i++;
		}
	}
	
	while (gtk_events_pending ())
		gtk_main_iteration ();

	i = 0;
	while(interfaces[i]) {
		anjuta_shell_get_object (ANJUTA_PLUGIN (profile)->shell,
								 interfaces[i], NULL);
		if (splash && max_icons > 0)
			e_splash_set_icon_highlight (splash, i, TRUE);
		while (gtk_events_pending ())
			gtk_main_iteration ();
		i++;
		max_icons--;
	}
}

static void
iprofile_iface_init(IAnjutaProfileIface *iface)
{
	iface->load = iprofile_load;
}

ANJUTA_PLUGIN_BEGIN (DefaultProfilePlugin, default_profile_plugin);
ANJUTA_INTERFACE (iprofile, IANJUTA_TYPE_PROFILE);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (DefaultProfilePlugin, default_profile_plugin);
