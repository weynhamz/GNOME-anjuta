/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-profile-manager.c
 * Copyright (C) Naba Kumar  <naba@gnome.org>
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

/**
 * SECTION:anjuta-profile-manager
 * @short_description: Managers a stack of plugins profiles
 * @see_also: #AnjutaPluginManager, #AnjutaProfile
 * @stability: Unstable
 * @include: libanjuta/anjuta-profile-manager.h
 * 
 */

#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libgnomevfs/gnome-vfs.h>

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-marshal.h>
#include "anjuta-profile-manager.h"

enum
{
	PROP_0,
	PROP_PLUGIN_MANAGER,
	PROP_PROFILES
};

enum
{
	PROFILE_PUSHED,
	PROFILE_POPPED,
	PROFILE_DESCOPED,
	PROFILE_SCOPED,
	LAST_SIGNAL
};

struct _AnjutaProfileManagerPriv
{
	AnjutaPluginManager *plugin_manager;
	GList *profiles;
	
	/* Pending queue. Profiles are queued until freeze count becomes 0 */
	GList *profiles_queue;
	
	/* Freeze count. Pending profiles are loaded when it reaches 0 */
	gint freeze_count;
};

static GObjectClass* parent_class = NULL;
static guint profile_manager_signals[LAST_SIGNAL] = { 0 };

static void
on_plugin_activated (AnjutaPluginManager *plugin_manager,
					 AnjutaPluginDescription *plugin_desc,
					 GObject *plugin_object,
					 AnjutaProfileManager *profile_manager)
{
	AnjutaProfileManagerPriv *priv;
	priv = profile_manager->priv;
	
	if (priv->profiles)
	{
		/* Add it current profile */
		anjuta_profile_add_plugin (ANJUTA_PROFILE (priv->profiles->data),
								   plugin_desc);
	}
}

static void
on_plugin_deactivated (AnjutaPluginManager *plugin_manager,
					   AnjutaPluginDescription *plugin_desc,
					   GObject *plugin_object,
					   AnjutaProfileManager *profile_manager)
{
	AnjutaProfileManagerPriv *priv;
	priv = profile_manager->priv;
	
	if (priv->profiles)
	{
		/* Remove from current profile */
		anjuta_profile_remove_plugin (ANJUTA_PROFILE (priv->profiles->data),
									  plugin_desc);
	}
}

static void
anjuta_profile_manager_init (AnjutaProfileManager *object)
{
	object->priv = g_new0 (AnjutaProfileManagerPriv, 1);
}

static void
anjuta_profile_manager_finalize (GObject *object)
{
	AnjutaProfileManagerPriv *priv;
	priv = ANJUTA_PROFILE_MANAGER (object)->priv;
	if (priv->profiles)
	{
		g_list_foreach (priv->profiles, (GFunc)g_object_unref, NULL);
		g_list_free (priv->profiles);
		priv->profiles = NULL;
	}
	if (priv->profiles_queue)
	{
		g_list_foreach (priv->profiles_queue, (GFunc)g_object_unref, NULL);
		g_list_free (priv->profiles_queue);
		priv->profiles_queue = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
anjuta_profile_manager_set_property (GObject *object, guint prop_id,
									 const GValue *value, GParamSpec *pspec)
{
	AnjutaProfileManagerPriv *priv;
	g_return_if_fail (ANJUTA_IS_PROFILE_MANAGER (object));
	priv = ANJUTA_PROFILE_MANAGER (object)->priv;
	
	switch (prop_id)
	{
	case PROP_PLUGIN_MANAGER:
		g_return_if_fail (ANJUTA_IS_PLUGIN_MANAGER (g_value_get_object (value)));
		priv->plugin_manager = g_value_get_object (value);
		g_signal_connect (priv->plugin_manager, "plugin-activated",
						  G_CALLBACK (on_plugin_activated), object);
		g_signal_connect (priv->plugin_manager, "plugin-deactivated",
						  G_CALLBACK (on_plugin_deactivated), object);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
anjuta_profile_manager_get_property (GObject *object, guint prop_id,
									 GValue *value, GParamSpec *pspec)
{
	AnjutaProfileManagerPriv *priv;
	g_return_if_fail (ANJUTA_IS_PROFILE_MANAGER (object));
	priv = ANJUTA_PROFILE_MANAGER (object)->priv;
	
	switch (prop_id)
	{
	case PROP_PLUGIN_MANAGER:
		g_value_set_object (value, priv->plugin_manager);
		break;
	case PROP_PROFILES:
		g_value_set_pointer (value, priv->profiles);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
anjuta_profile_manager_class_init (AnjutaProfileManagerClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));

	object_class->finalize = anjuta_profile_manager_finalize;
	object_class->set_property = anjuta_profile_manager_set_property;
	object_class->get_property = anjuta_profile_manager_get_property;
	
	g_object_class_install_property (object_class,
	                                 PROP_PLUGIN_MANAGER,
	                                 g_param_spec_object ("plugin-manager",
	                                                      "Plugin Manager",
	                                                      "The plugin manager to use for profile plugins",
	                                                      ANJUTA_TYPE_PLUGIN_MANAGER,
	                                                      G_PARAM_READABLE |
														  G_PARAM_WRITABLE |
														  G_PARAM_CONSTRUCT));
	profile_manager_signals[PROFILE_PUSHED] =
		g_signal_new ("profile-pushed",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (AnjutaProfileManagerClass,
									   profile_pushed),
		              NULL, NULL,
		              anjuta_cclosure_marshal_VOID__OBJECT,
		              G_TYPE_NONE, 1,
		              ANJUTA_TYPE_PROFILE);
	profile_manager_signals[PROFILE_POPPED] =
		g_signal_new ("profile-popped",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (AnjutaProfileManagerClass,
									   profile_popped),
		              NULL, NULL,
					  anjuta_cclosure_marshal_VOID__OBJECT,
		              G_TYPE_NONE, 1,
		              ANJUTA_TYPE_PROFILE);
	profile_manager_signals[PROFILE_DESCOPED] =
		g_signal_new ("profile-descoped",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (AnjutaProfileManagerClass,
									   profile_descoped),
		              NULL, NULL,
					  anjuta_cclosure_marshal_VOID__OBJECT,
		              G_TYPE_NONE, 1,
		              ANJUTA_TYPE_PROFILE);
	profile_manager_signals[PROFILE_SCOPED] =
		g_signal_new ("profile-scoped",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (AnjutaProfileManagerClass,
									   profile_scoped),
		              NULL, NULL,
					  anjuta_cclosure_marshal_VOID__OBJECT,
		              G_TYPE_NONE, 1,
		              ANJUTA_TYPE_PROFILE);
}

GType
anjuta_profile_manager_get_type (void)
{
	static GType our_type = 0;

	if(our_type == 0)
	{
		static const GTypeInfo our_info =
		{
			sizeof (AnjutaProfileManagerClass), /* class_size */
			(GBaseInitFunc) NULL, /* base_init */
			(GBaseFinalizeFunc) NULL, /* base_finalize */
			(GClassInitFunc) anjuta_profile_manager_class_init, /* class_init */
			(GClassFinalizeFunc) NULL, /* class_finalize */
			NULL /* class_data */,
			sizeof (AnjutaProfileManager), /* instance_size */
			0, /* n_preallocs */
			(GInstanceInitFunc) anjuta_profile_manager_init, /* instance_init */
			NULL /* value_table */
		};

		our_type = g_type_register_static (G_TYPE_OBJECT, "AnjutaProfileManager",
		                                   &our_info, 0);
	}

	return our_type;
}

AnjutaProfileManager*
anjuta_profile_manager_new (AnjutaPluginManager *plugin_manager)
{
	GObject *obj;
	
	obj = g_object_new (ANJUTA_TYPE_PROFILE_MANAGER, "plugin-manager",
						plugin_manager, NULL);
	return ANJUTA_PROFILE_MANAGER (obj);
}

static gboolean
anjuta_profile_manager_load_profile (AnjutaProfileManager *profile_manager,
									 AnjutaProfile *profile,
									 AnjutaProfile *previous_profile,
									 GError **error)
{
	AnjutaProfileManagerPriv *priv;
	GList *active_plugins, *node;
	GList *plugins_to_activate, *plugins_to_deactivate;
	GList *selected_plugins;
		
	GHashTable *active_plugins_hash, *plugins_to_activate_hash;
	
	priv = profile_manager->priv;
	
	/* Disable profile synchronization while the profile is being activated */
	g_signal_handlers_block_by_func (priv->plugin_manager,
									 G_CALLBACK (on_plugin_activated),
									 profile_manager);
	g_signal_handlers_block_by_func (priv->plugin_manager,
									 G_CALLBACK (on_plugin_deactivated),
									 profile_manager);
	
	/* Emit pre-change for the last profile */
	if (previous_profile)
	{
		g_signal_emit_by_name (profile_manager, "profile-descoped",
							   previous_profile);
	}
	
	/* Prepare plugins to activate */
	plugins_to_activate_hash = g_hash_table_new (g_direct_hash, g_direct_equal);
	
	/* Select plugins in the profile */
	if (profile)
		selected_plugins = anjuta_profile_get_plugins (profile);
	else
		selected_plugins = NULL;
	
	node = selected_plugins;
	while (node)
	{
		g_hash_table_insert (plugins_to_activate_hash, node->data, node->data);
		node = g_list_next (node);
	}
	
	/* Prepare active plugins hash */
	active_plugins =
		anjuta_plugin_manager_get_active_plugins (priv->plugin_manager);
	active_plugins_hash = g_hash_table_new (g_direct_hash, g_direct_equal);
	node = active_plugins;
	while (node)
	{
		g_hash_table_insert (active_plugins_hash, node->data, node->data);
		node = g_list_next (node);
	}
	
	plugins_to_deactivate = NULL;
	
	/* Prepare plugins to deactiveate that are already active, but are
	 * not requested to be active
	 */
	node = active_plugins;
	while (node)
	{
		if (!g_hash_table_lookup (plugins_to_activate_hash, node->data))
		{
			plugins_to_deactivate = g_list_prepend (plugins_to_deactivate,
													node->data);
		}
		node = g_list_next (node);
	}
	plugins_to_deactivate = g_list_reverse (plugins_to_deactivate);

	/* Deactivate plugins */
	node = plugins_to_deactivate;
	while (node)
	{
		AnjutaPluginDescription *desc;
		GObject *plugin_object;
		gchar *plugin_id = NULL;
		
		desc = (AnjutaPluginDescription *)node->data;
		anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
											  "Location", &plugin_id);
		g_assert (plugin_id != NULL);
		
		/* DEBUG_PRINT ("Profile: deactivating %s", plugin_id); */
		
		plugin_object =
			anjuta_plugin_manager_get_plugin_by_id (priv->plugin_manager,
													plugin_id);
		g_assert (plugin_object != NULL);
		
		anjuta_plugin_manager_unload_plugin (priv->plugin_manager,
											 plugin_object);
		g_free (plugin_id);
		node = g_list_next (node);
	}
	
	/* Prepare the plugins to activate */
	plugins_to_activate = NULL;
	node = selected_plugins;
	while (node)
	{
		if (!g_hash_table_lookup (active_plugins_hash, node->data))
			plugins_to_activate = g_list_prepend (plugins_to_activate,
												   node->data);
		node = g_list_next (node);
	}
	
	/* Now activate the plugins */
	if (plugins_to_activate)
	{
		/* Activate them */
		plugins_to_activate = g_list_reverse (plugins_to_activate);
		anjuta_plugin_manager_activate_plugins (priv->plugin_manager,
												plugins_to_activate);
	}
	
	g_list_free (plugins_to_activate);
	g_list_free (active_plugins);
	
	g_hash_table_destroy (plugins_to_activate_hash);
	g_hash_table_destroy (active_plugins_hash);

	/* Enable profile synchronization */
	g_signal_handlers_unblock_by_func (priv->plugin_manager,
									   G_CALLBACK (on_plugin_activated),
									   profile_manager);
	g_signal_handlers_unblock_by_func (priv->plugin_manager,
									   G_CALLBACK (on_plugin_deactivated),
									   profile_manager);
	g_signal_emit_by_name (profile_manager, "profile-scoped", profile);
	return TRUE;
}

static gboolean
anjuta_profile_manager_queue_profile (AnjutaProfileManager *profile_manager,
									  AnjutaProfile *profile,
									  GError **error)
{
	AnjutaProfileManagerPriv *priv;
	
	priv = profile_manager->priv;
	priv->profiles_queue = g_list_prepend (priv->profiles_queue,
											profile);
	/* If there is no freeze load profile now */
	if (priv->freeze_count <= 0)
	{
		AnjutaProfile *previous_profile = NULL;
		
		if (priv->profiles)
			previous_profile = priv->profiles->data;
		
		/* Push queued profiles in stack */
		priv->profiles = g_list_concat (priv->profiles_queue, priv->profiles);
		priv->profiles_queue = NULL;
		
		return anjuta_profile_manager_load_profile (profile_manager,
										ANJUTA_PROFILE (priv->profiles->data),
													previous_profile,
													error);
	}
	else
	{
		return FALSE;
	}
}

gboolean
anjuta_profile_manager_push (AnjutaProfileManager *profile_manager,
							 AnjutaProfile *profile, GError **error)
{
	AnjutaProfileManagerPriv *priv;
	
	g_return_val_if_fail (ANJUTA_IS_PROFILE_MANAGER (profile_manager), FALSE);
	priv = profile_manager->priv;
	
	/* Emit profile push signal */
	g_signal_emit_by_name (profile_manager, "profile-pushed",
						   profile);
	
	return anjuta_profile_manager_queue_profile (profile_manager, profile,
												 error);
}

/* Only the last profile can be popped */
gboolean
anjuta_profile_manager_pop (AnjutaProfileManager *profile_manager,
							const gchar *profile_name, GError **error)
{
	AnjutaProfileManagerPriv *priv;
	AnjutaProfile *profile;
	g_return_val_if_fail (ANJUTA_IS_PROFILE_MANAGER (profile_manager), FALSE);
	
	priv = profile_manager->priv;
	
	/* First check in the queue */
	if (priv->profiles_queue)
	{
		profile = priv->profiles_queue->data;
		g_return_val_if_fail (strcmp (anjuta_profile_get_name (profile),
									  profile_name) == 0, FALSE);
		priv->profiles_queue = g_list_remove (priv->profiles_queue, profile);
		
		g_signal_emit_by_name (profile_manager, "profile-popped",
							   profile);
		
		g_object_unref (profile);
		return TRUE;
	}
	
	/* Then check in the current stack */
	if (priv->profiles)
	{
		profile = priv->profiles->data;
		g_return_val_if_fail (strcmp (anjuta_profile_get_name (profile),
									  profile_name) == 0, FALSE);
		priv->profiles = g_list_remove (priv->profiles, profile);
		
		g_signal_emit_by_name (profile_manager, "profile-popped",
							   profile);
		
		/* Restore the next profile in the stack */
		if (priv->profiles)
		{
			return anjuta_profile_manager_load_profile (profile_manager,
									ANJUTA_PROFILE (priv->profiles->data),
														profile,
														error);
		}
		else
		{
			return anjuta_profile_manager_load_profile (profile_manager,
														NULL, profile,
														error);
		}
		g_object_unref (profile);
	}
	
	g_warning ("No profiles in the stack. Can not pop out any profile: %s",
			   profile_name);
	return FALSE;
}

void
anjuta_profile_manager_freeze (AnjutaProfileManager *profile_manager)
{
	AnjutaProfileManagerPriv *priv;
	g_return_if_fail (ANJUTA_IS_PROFILE_MANAGER (profile_manager));
	priv = profile_manager->priv;
	priv->freeze_count++;
}

gboolean
anjuta_profile_manager_thaw (AnjutaProfileManager *profile_manager,
							GError **error)
{
	AnjutaProfileManagerPriv *priv;
	g_return_val_if_fail (ANJUTA_IS_PROFILE_MANAGER (profile_manager), FALSE);
	priv = profile_manager->priv;
	
	if (priv->freeze_count > 0)
		priv->freeze_count--;
	
	if (priv->freeze_count <= 0 && priv->profiles_queue)
	{
		AnjutaProfile *previous_profile = NULL;
		
		if (priv->profiles)
			previous_profile = priv->profiles->data;
		
		/* Push queued profiles in stack */
		priv->profiles = g_list_concat (priv->profiles_queue, priv->profiles);
		priv->profiles_queue = NULL;
		
		/* Load the profile */
		return anjuta_profile_manager_load_profile (profile_manager,
										ANJUTA_PROFILE (priv->profiles->data),
													previous_profile,
													error);
	}
	else
	{
		return FALSE;
	}
}
