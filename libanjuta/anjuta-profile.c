/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-profile.c
 * Copyright (C) Naba Kumar <naba@gnome.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <glib/gi18n.h>
#include "anjuta-profile.h"
#include "anjuta-marshal.h"

enum
{
	PROP_0,
	PROP_PROFILE_NAME,
	PROP_PROFILE_PLUGINS
};

enum
{
	PLUGIN_ADDED,
	PLUGIN_REMOVED,
	CHANGED,
	LAST_SIGNAL
};

struct _AnjutaProfilePriv
{
	gchar *name;
	GList *plugins;
};

static GObjectClass* parent_class = NULL;
static guint profile_signals[LAST_SIGNAL] = { 0 };

static void
anjuta_profile_init (AnjutaProfile *object)
{
	object->priv = g_new0 (AnjutaProfilePriv, 1);
	object->priv->name = NULL;
	object->priv->plugins = NULL;
}

static void
anjuta_profile_finalize (GObject *object)
{
	AnjutaProfilePriv *priv = ANJUTA_PROFILE (object)->priv;
	g_free (priv->name);
	if (priv->plugins)
		g_list_free (priv->plugins);
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
anjuta_profile_set_property (GObject *object, guint prop_id,
							 const GValue *value, GParamSpec *pspec)
{
	AnjutaProfilePriv *priv = ANJUTA_PROFILE (object)->priv;
	
	g_return_if_fail (ANJUTA_IS_PROFILE (object));

	switch (prop_id)
	{
	case PROP_PROFILE_NAME:
		g_return_if_fail (g_value_get_string (value) == NULL);
		g_free (priv->name);
		priv->name = g_strdup (g_value_get_string (value));
		break;
	case PROP_PROFILE_PLUGINS:
		if (priv->plugins)
			g_list_free (priv->plugins);
		if (g_value_get_pointer (value))
			priv->plugins = g_list_copy (g_value_get_pointer (value));
		else
			priv->plugins = NULL;
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
anjuta_profile_get_property (GObject *object, guint prop_id,
							 GValue *value, GParamSpec *pspec)
{
	AnjutaProfilePriv *priv = ANJUTA_PROFILE (object)->priv;
	
	g_return_if_fail (ANJUTA_IS_PROFILE (object));

	switch (prop_id)
	{
	case PROP_PROFILE_NAME:
		g_value_set_string (value, priv->name);
		break;
	case PROP_PROFILE_PLUGINS:
		g_value_set_pointer (value, priv->plugins);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
anjuta_profile_plugin_added (AnjutaProfile *self,
							 AnjutaPluginDescription *plugin)
{
}

static void
anjuta_profile_plugin_removed (AnjutaProfile *self,
							   AnjutaPluginDescription *plugin)
{
}

static void
anjuta_profile_changed (AnjutaProfile *self, GList *plugins)
{
}

static void
anjuta_profile_class_init (AnjutaProfileClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));

	object_class->finalize = anjuta_profile_finalize;
	object_class->set_property = anjuta_profile_set_property;
	object_class->get_property = anjuta_profile_get_property;

	klass->plugin_added = anjuta_profile_plugin_added;
	klass->plugin_removed = anjuta_profile_plugin_removed;
	klass->changed = anjuta_profile_changed;

	g_object_class_install_property (object_class,
	                                 PROP_PROFILE_NAME,
	                                 g_param_spec_string ("profile-name",
											  _("Profile Name"),
											  _("Name of the plugin profile"),
											  NULL,
											  G_PARAM_READABLE |
											  G_PARAM_WRITABLE |
											  G_PARAM_CONSTRUCT));

	g_object_class_install_property (object_class,
	                                 PROP_PROFILE_PLUGINS,
	                                 g_param_spec_pointer ("plugins",
											  _("Profile Plugins"),
											  _("List of plugins for this profile"),
											  G_PARAM_READABLE |
											  G_PARAM_WRITABLE |
											  G_PARAM_CONSTRUCT));

	profile_signals[PLUGIN_ADDED] =
		g_signal_new ("plugin-added",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (AnjutaProfileClass, plugin_added),
		              NULL, NULL,
		              anjuta_cclosure_marshal_VOID__POINTER,
		              G_TYPE_NONE, 1,
		              G_TYPE_POINTER);

	profile_signals[PLUGIN_REMOVED] =
		g_signal_new ("plugin-removed",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (AnjutaProfileClass, plugin_removed),
		              NULL, NULL,
		              anjuta_cclosure_marshal_VOID__POINTER,
		              G_TYPE_NONE, 1,
		              G_TYPE_POINTER);
	profile_signals[CHANGED] =
		g_signal_new ("changed",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (AnjutaProfileClass, changed),
		              NULL, NULL,
		              anjuta_cclosure_marshal_VOID__POINTER,
		              G_TYPE_NONE, 1,
		              G_TYPE_POINTER);
}

GType
anjuta_profile_get_type (void)
{
	static GType our_type = 0;

	if(our_type == 0)
	{
		static const GTypeInfo our_info =
		{
			sizeof (AnjutaProfileClass), /* class_size */
			(GBaseInitFunc) NULL, /* base_init */
			(GBaseFinalizeFunc) NULL, /* base_finalize */
			(GClassInitFunc) anjuta_profile_class_init, /* class_init */
			(GClassFinalizeFunc) NULL, /* class_finalize */
			NULL /* class_data */,
			sizeof (AnjutaProfile), /* instance_size */
			0, /* n_preallocs */
			(GInstanceInitFunc) anjuta_profile_init, /* instance_init */
			NULL /* value_table */
		};

		our_type = g_type_register_static (G_TYPE_OBJECT, "AnjutaProfile",
		                                   &our_info, 0);
	}

	return our_type;
}

AnjutaProfile*
anjuta_profile_new (const gchar* name, GList* plugins)
{
	GObject *profile;
	profile = g_object_new (ANJUTA_TYPE_PROFILE, "name", name,
							"plugins", plugins, NULL);
	return ANJUTA_PROFILE (profile);
}

AnjutaProfile*
anjuta_profile_new_from_xml (const gchar* name, const gchar* xml_text,
							 GError **err)
{
	/* TODO: Add public function implementation here */
	return NULL;
}

void
anjuta_profile_add_plugin (AnjutaProfile *profile,
						   AnjutaPluginDescription *plugin)
{
	AnjutaProfilePriv *priv;
	g_return_if_fail (ANJUTA_IS_PROFILE (profile));
	g_return_if_fail (plugin != NULL);
	priv = ANJUTA_PROFILE (profile)->priv;
	if (priv->plugins == NULL || g_list_find (priv->plugins, plugin) == NULL)
	{
		priv->plugins = g_list_prepend (priv->plugins, plugin);
		g_signal_emit_by_name (profile, "plugin-added", plugin);
		g_signal_emit_by_name (profile, "changed", priv->plugins);
	}
}

void
anjuta_profile_remove_plugin (AnjutaProfile *profile, 
							  AnjutaPluginDescription *plugin)
{
	AnjutaProfilePriv *priv;
	g_return_if_fail (ANJUTA_IS_PROFILE (profile));
	g_return_if_fail (plugin != NULL);
	priv = ANJUTA_PROFILE (profile)->priv;
	if (priv->plugins && g_list_find (priv->plugins, plugin) != NULL)
	{
		priv->plugins = g_list_remove (priv->plugins, plugin);
		g_signal_emit_by_name (profile, "plugin-removed", plugin);
		g_signal_emit_by_name (profile, "changed", priv->plugins);
	}
}

gboolean
anjuta_profile_has_plugin (AnjutaProfile *profile,
						   AnjutaPluginDescription *plugin)
{
	AnjutaProfilePriv *priv;
	g_return_val_if_fail (ANJUTA_IS_PROFILE (profile), FALSE);
	g_return_val_if_fail (plugin != NULL, FALSE);
	priv = ANJUTA_PROFILE (profile)->priv;
	return (priv->plugins != NULL &&
			g_list_find (priv->plugins, plugin) != NULL);
}

gchar*
anjuta_profile_to_xml (AnjutaProfile *profile)
{
	/* TODO: Add public function implementation here */
	return NULL;
}

GList*
anjuta_profile_difference_positive (AnjutaProfile *profile,
									AnjutaProfile *profile_to_diff)
{
	/* TODO: Add public function implementation here */
	return NULL;
}

GList*
anjuta_profile_difference_negative (AnjutaProfile *profile,
									AnjutaProfile *profile_to_diff)
{
	/* TODO: Add public function implementation here */
	return NULL;
}
