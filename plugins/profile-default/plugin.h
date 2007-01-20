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

#include <libanjuta/anjuta-plugin.h>

extern GType default_profile_plugin_get_type (GluePlugin *plugin);
#define ANJUTA_TYPE_PLUGIN_DEFAULT_PROFILE         (default_profile_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_DEFAULT_PROFILE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_DEFAULT_PROFILE, DefaultProfilePlugin))
#define ANJUTA_PLUGIN_DEFAULT_PROFILE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_DEFAULT_PROFILE, DefaultProfilePluginClass))
#define ANJUTA_IS_PLUGIN_DEFAULT_PROFILE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_DEFAULT_PROFILE))
#define ANJUTA_IS_PLUGIN_DEFAULT_PROFILE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_DEFAULT_PROFILE))
#define ANJUTA_PLUGIN_DEFAULT_PROFILE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_DEFAULT_PROFILE, DefaultProfilePluginClass))

typedef struct _DefaultProfilePlugin DefaultProfilePlugin;
typedef struct _DefaultProfilePluginClass DefaultProfilePluginClass;

struct _DefaultProfilePlugin{
	AnjutaPlugin parent;
	gchar *project_uri;
	gint merge_id;
	GtkActionGroup *action_group;
	
	gchar *default_profile;
	
	/* This hash table stores the list of plugins specified in system default
	 * profile. When default profile is loaded (the profile that is loaded
	 * when Anjuta is started and that corresponds to profile before a project
	 * is loaded), this list is merged with user's saved default profile.
	 * 
	 * User's saved default profile is the list of plugins that were active
	 * in the last session or before a project profile is loaded, minus the
	 * plugins that are mentioned in system default profile. The reason these
	 * plugins are excluded from the users profile is because they are anyway
	 * merged with system default profile before loading.
	 *
	 * This hash table is created basically to make a 'diff' of plugins
	 * between currently loaded plugins and system default plugin. The diff is
	 * then stored as either user's default profile or project profile.
	 *
	 * The keys are plugin IDs. The values are irrelevent.
	 */
	GList *system_plugins;
	GList *project_plugins;
	
	/* A flag to indicate that session ops is by this plugin */
	gboolean session_by_me;
};

struct _DefaultProfilePluginClass{
	AnjutaPluginClass parent_class;
};
