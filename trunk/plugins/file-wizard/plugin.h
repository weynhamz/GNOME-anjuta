/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.h
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

#ifndef __FILE_WIZARD_PLUGIN__
#define __FILE_WIZARD_PLUGIN__

#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-ui.h>

extern GType file_wizard_plugin_get_type (AnjutaGluePlugin *plugin);
#define ANJUTA_TYPE_PLUGIN_FILE_WIZARD         (file_wizard_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_FILE_WIZARD(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_FILE_WIZARD, AnjutaFileWizardPlugin))
#define ANJUTA_PLUGIN_FILE_WIZARD_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_FILE_WIZARD, AnjutaFileWizardPluginClass))
#define ANJUTA_IS_PLUGIN_FILE_WIZARD(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_FILE_WIZARD))
#define ANJUTA_IS_PLUGIN_FILE_WIZARD_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_FILE_WIZARD))
#define ANJUTA_PLUGIN_FILE_WIZARD_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_FILE_WIZARD, AnjutaFileWizardPluginClass))

typedef struct _AnjutaFileWizardPlugin AnjutaFileWizardPlugin;
typedef struct _AnjutaFileWizardPluginClass AnjutaFileWizardPluginClass;

struct _AnjutaFileWizardPlugin {
	AnjutaPlugin parent;
	AnjutaPreferences *prefs;
	GtkActionGroup *action_group;
	gint root_watch_id;
	gchar *top_dir;
};

struct _AnjutaFileWizardPluginClass {
	AnjutaPluginClass parent_class;
};

#endif
