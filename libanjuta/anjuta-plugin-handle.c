/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-plugin-handle.c
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

#include <string.h>
#include "anjuta-plugin-handle.h"
#include "resources.h"
#include "anjuta-debug.h"

enum
{
	PROP_0,

	PROP_ID,
	PROP_NAME,
	PROP_ABOUT,
	PROP_ICON_PATH,
	PROP_USER_ACTIVATABLE,
	PROP_DESCRIPTION,
	PROP_DEPENDENCY_NAMES,
	PROP_DEPENDENCIES,
	PROP_DEPENDENTS,
	PROP_INTERFACES,
	PROP_CAN_LOAD,
	PROP_CHECKED,
	PROP_RESOLVE_PASS
};

struct _AnjutaPluginHandlePriv
{
	char *id;
	char *name;
	char *about;
	char *icon_path;
	gboolean user_activatable;
	
	AnjutaPluginDescription *description;

	/* The dependencies listed in the oaf file */
	GList *dependency_names;

	/* Interfaces exported by this plugin */
	GList *interfaces;
	
	/* Attributes defined for this plugin */
	/* GHashTable *attributes; */
	
	/* The keys of these tables represent the dependencies and dependents
	 * of the module.  The values point back at the plugins */
	GHashTable *dependencies;
	GHashTable *dependents;
	
	gboolean can_load;
	gboolean checked;

	/* The pass on which the module was resolved, or -1 if
	 * unresolved */
	int resolve_pass;
};

static GObjectClass* parent_class = NULL;

static void
anjuta_plugin_handle_init (AnjutaPluginHandle *object)
{
	object->priv = g_new0 (AnjutaPluginHandlePriv, 1);

	object->priv->resolve_pass = -1;

	object->priv->dependencies = g_hash_table_new (g_direct_hash, 
												   g_direct_equal);
	object->priv->dependents = g_hash_table_new (g_direct_hash, 
												 g_direct_equal);
}

static void
anjuta_plugin_handle_finalize (GObject *object)
{
	AnjutaPluginHandlePriv *priv;
	
	priv = ANJUTA_PLUGIN_HANDLE (object)->priv;
	
	g_free (priv->id);
	priv->id = NULL;
	g_free (priv->name);
	priv->name = NULL;
	g_free (priv->icon_path);
	priv->icon_path = NULL;
	
	g_list_foreach (priv->dependency_names, (GFunc)g_free, NULL);
	g_list_free (priv->dependency_names);
	priv->dependency_names = NULL;
	
	g_list_foreach (priv->interfaces, (GFunc)g_free, NULL);
	g_list_free (priv->dependency_names);
	priv->dependency_names = NULL;
	
	g_hash_table_destroy (priv->dependencies);
	priv->dependencies = NULL;
	g_hash_table_destroy (priv->dependents);
	priv->dependents = NULL;
	
	g_free (priv);
	
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
anjuta_plugin_handle_set_property (GObject *object, guint prop_id,
								   const GValue *value, GParamSpec *pspec)
{
	g_return_if_fail (ANJUTA_IS_PLUGIN_HANDLE (object));

	switch (prop_id)
	{
#if 0
	case PROP_ID:
		/* TODO: Add setter for "id" property here */
		break;
	case PROP_NAME:
		/* TODO: Add setter for "name" property here */
		break;
	case PROP_ABOUT:
		/* TODO: Add setter for "about" property here */
		break;
	case PROP_ICON_PATH:
		/* TODO: Add setter for "icon-path" property here */
		break;
	case PROP_USER_ACTIVATABLE:
		/* TODO: Add setter for "user-activatable" property here */
		break;
	case PROP_DESCRIPTION:
		/* TODO: Add setter for "description" property here */
		break;
	case PROP_DEPENDENCY_NAMES:
		/* TODO: Add setter for "dependency-names" property here */
		break;
	case PROP_DEPENDENCIES:
		/* TODO: Add setter for "dependencies" property here */
		break;
	case PROP_DEPENDENTS:
		/* TODO: Add setter for "dependents" property here */
		break;
	case PROP_INTERFACES:
		/* TODO: Add setter for "interfaces" property here */
		break;
	case PROP_CAN_LOAD:
		/* TODO: Add setter for "can-load" property here */
		break;
	case PROP_CHECKED:
		/* TODO: Add setter for "checked" property here */
		break;
	case PROP_RESOLVE_PASS:
		/* TODO: Add setter for "resolve-pass" property here */
		break;
#endif
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
anjuta_plugin_handle_get_property (GObject *object, guint prop_id,
								   GValue *value, GParamSpec *pspec)
{
	AnjutaPluginHandlePriv *priv;
	
	g_return_if_fail (ANJUTA_IS_PLUGIN_HANDLE (object));
	priv = ANJUTA_PLUGIN_HANDLE (object)->priv;
	switch (prop_id)
	{
	case PROP_ID:
		g_value_set_string (value, priv->id);
		break;
	case PROP_NAME:
		g_value_set_string (value, priv->name);
		break;
	case PROP_ABOUT:
		g_value_set_string (value, priv->about);
		break;
	case PROP_ICON_PATH:
		g_value_set_string (value, priv->icon_path);
		break;
	case PROP_USER_ACTIVATABLE:
		g_value_set_boolean (value, priv->user_activatable);
		break;
	case PROP_DESCRIPTION:
		g_value_set_pointer (value, priv->description);
		break;
	case PROP_DEPENDENCY_NAMES:
		g_value_set_pointer (value, priv->dependency_names);
		break;
	case PROP_DEPENDENCIES:
		g_value_set_pointer (value, priv->dependencies);
		break;
	case PROP_DEPENDENTS:
		g_value_set_pointer (value, priv->dependents);
		break;
	case PROP_INTERFACES:
		g_value_set_pointer (value, priv->interfaces);
		break;
	case PROP_CAN_LOAD:
		g_value_set_boolean (value, priv->can_load);
		break;
	case PROP_CHECKED:
		g_value_set_boolean (value, priv->checked);
		break;
	case PROP_RESOLVE_PASS:
		g_value_set_int (value, priv->resolve_pass);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
anjuta_plugin_handle_class_init (AnjutaPluginHandleClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));

	object_class->finalize = anjuta_plugin_handle_finalize;
	object_class->set_property = anjuta_plugin_handle_set_property;
	object_class->get_property = anjuta_plugin_handle_get_property;

	g_object_class_install_property (object_class,
	                                 PROP_ID,
	                                 g_param_spec_string ("id",
	                                                      "ID",
	                                                      "Unique plugin ID",
	                                                      NULL,
	                                                      G_PARAM_READABLE));

	g_object_class_install_property (object_class,
	                                 PROP_NAME,
	                                 g_param_spec_string ("name",
	                                                      "Name",
	                                                      "Plugin name",
	                                                      NULL,
	                                                      G_PARAM_READABLE));

	g_object_class_install_property (object_class,
	                                 PROP_ABOUT,
	                                 g_param_spec_string ("about",
	                                                      "About Plugin",
	                                                      "About description of the plugin",
	                                                      NULL,
	                                                      G_PARAM_READABLE));

	g_object_class_install_property (object_class,
	                                 PROP_ICON_PATH,
	                                 g_param_spec_string ("icon-path",
	                                                      "Icon Path",
	                                                      "Icon path of the plugin",
	                                                      NULL,
	                                                      G_PARAM_READABLE));

	g_object_class_install_property (object_class,
	                                 PROP_USER_ACTIVATABLE,
	                                 g_param_spec_boolean ("user-activatable",
	                                                       "User Activatable",
	                                                       "If the plugin is user activatable",
	                                                       FALSE,
	                                                       G_PARAM_READABLE));

	g_object_class_install_property (object_class,
	                                 PROP_DESCRIPTION,
	                                 g_param_spec_pointer ("description",
	                                                       "Description",
	                                                       "Plugin description",
	                                                       G_PARAM_READABLE));

	g_object_class_install_property (object_class,
	                                 PROP_DEPENDENCY_NAMES,
	                                 g_param_spec_pointer ("dependency-names",
	                                                       "Dependency names",
	                                                       "Plugin dependency names listed in oaf file",
	                                                       G_PARAM_READABLE));

	g_object_class_install_property (object_class,
	                                 PROP_DEPENDENCIES,
	                                 g_param_spec_pointer ("dependencies",
	                                                       "Dependencies",
	                                                       "Plugin dependencies",
	                                                       G_PARAM_READABLE));

	g_object_class_install_property (object_class,
	                                 PROP_DEPENDENTS,
	                                 g_param_spec_pointer ("dependents",
	                                                       "Dependents",
	                                                       "Plugin dependents",
	                                                       G_PARAM_READABLE));

	g_object_class_install_property (object_class,
	                                 PROP_INTERFACES,
	                                 g_param_spec_pointer ("interfaces",
	                                                       "Interfaces",
	                                                       "Interfaces exported by the plugin",
	                                                       G_PARAM_READABLE));

	g_object_class_install_property (object_class,
	                                 PROP_CAN_LOAD,
	                                 g_param_spec_boolean ("can-load",
	                                                       "Can Load",
	                                                       "If the plugin can be loaded",
	                                                       FALSE,
	                                                       G_PARAM_READABLE));

	g_object_class_install_property (object_class,
	                                 PROP_CHECKED,
	                                 g_param_spec_boolean ("checked",
	                                                       "Checked",
	                                                       "If the plugin is checked in UI",
	                                                       FALSE,
	                                                       G_PARAM_READABLE));

	g_object_class_install_property (object_class,
	                                 PROP_RESOLVE_PASS,
	                                 g_param_spec_int ("resolve-pass",
	                                                   "Resolve Pass",
	                                                   "Dependency resolution pass",
	                                                   G_MININT, /* TODO: Adjust minimum property value */
	                                                   G_MAXINT, /* TODO: Adjust maximum property value */
	                                                   0,
	                                                   G_PARAM_READABLE));
}

GType
anjuta_plugin_handle_get_type (void)
{
	static GType our_type = 0;

	if(our_type == 0)
	{
		static const GTypeInfo our_info =
		{
			sizeof (AnjutaPluginHandleClass), /* class_size */
			(GBaseInitFunc) NULL, /* base_init */
			(GBaseFinalizeFunc) NULL, /* base_finalize */
			(GClassInitFunc) anjuta_plugin_handle_class_init, /* class_init */
			(GClassFinalizeFunc) NULL, /* class_finalize */
			NULL /* class_data */,
			sizeof (AnjutaPluginHandle), /* instance_size */
			0, /* n_preallocs */
			(GInstanceInitFunc) anjuta_plugin_handle_init, /* instance_init */
			NULL /* value_table */
		};

		our_type = g_type_register_static (G_TYPE_OBJECT, "AnjutaPluginHandle",
		                                   &our_info, 0);
	}

	return our_type;
}


static char *
get_icon_path (char *icon_name)
{
	char *ret;
	
	if (g_path_is_absolute (icon_name)) {
		ret = g_strdup (icon_name);
	} else {
		ret = anjuta_res_get_pixmap_file (icon_name);
	}
	
	return ret;
}

static GList *
property_to_list (const char *value)
{
	GList *l = NULL;
	char **split_str;
	char **p;
	
	split_str = g_strsplit (value, ",", -1);
	for (p = split_str; *p != NULL; p++) {
		l = g_list_prepend (l, g_strdup (g_strstrip (*p)));
	}
	g_strfreev (split_str);
	return l;
}

AnjutaPluginHandle*
anjuta_plugin_handle_new (const gchar *plugin_desc_path)
{
	AnjutaPluginHandle *plugin_handle;
	AnjutaPluginDescription *desc;
	char *str;
	gchar *contents;
	gboolean success = TRUE;
	
	/* Load file content */
	if (g_file_get_contents (plugin_desc_path, &contents, NULL, NULL)) {
		
		desc = anjuta_plugin_description_new_from_string (contents, NULL);
		g_free (contents);
		if (!desc) {
			g_warning ("Bad plugin file: %s\n", plugin_desc_path);
			return NULL;
		}
	}
	else
	{
		return NULL;
	}
	
	plugin_handle = g_object_new (ANJUTA_TYPE_PLUGIN_HANDLE, NULL);
	
	/* Initialize plugin handle */
	plugin_handle->priv->description = desc;
	plugin_handle->priv->user_activatable = TRUE;
	
	if (anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
											  "Location", &str)) {
		plugin_handle->priv->id = str;
	} else {
		g_warning ("Couldn't find 'Location'");
		success = FALSE;
	}
	
	if (anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
											  "Name", &str)) {
		plugin_handle->priv->name = str;
	} else {
		g_warning ("couldn't find 'Name' attribute.");
		success = FALSE;
	}

	if (anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
											  "Description", &str)) {
		plugin_handle->priv->about = str;
	} else {
		g_warning ("Couldn't find 'Description' attribute.");
		success = FALSE;
	}

	if (anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
									   "Icon", &str)) {
		plugin_handle->priv->icon_path = get_icon_path (str);
		g_free (str);
	}

	if (anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
											  "Dependencies",
											  &str)) {
		plugin_handle->priv->dependency_names = property_to_list (str);
		g_free (str);
	}

	if (anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
											  "Interfaces",
											  &str)) {
		plugin_handle->priv->interfaces = property_to_list (str);
		g_free (str);
	}
	
	if (anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
											  "UserActivatable", &str)) {
		if (str && strcasecmp (str, "no") == 0)
		{
			plugin_handle->priv->user_activatable = FALSE;
			DEBUG_PRINT ("Plugin '%s' is not user activatable",
						 plugin_handle->priv->name? plugin_handle->priv->name : "Unknown");
		}
		g_free (str);
	}
	if (!success) {
		g_object_unref (plugin_handle);
		plugin_handle = NULL;
	}

	return plugin_handle;
}

const char*
anjuta_plugin_handle_get_id (AnjutaPluginHandle *plugin_handle)
{
	g_return_val_if_fail (ANJUTA_IS_PLUGIN_HANDLE (plugin_handle), NULL);
	return plugin_handle->priv->id;
}

const char*
anjuta_plugin_handle_get_name (AnjutaPluginHandle *plugin_handle)
{
	g_return_val_if_fail (ANJUTA_IS_PLUGIN_HANDLE (plugin_handle), NULL);
	return plugin_handle->priv->name;
}

const char*
anjuta_plugin_handle_get_about (AnjutaPluginHandle *plugin_handle)
{
	g_return_val_if_fail (ANJUTA_IS_PLUGIN_HANDLE (plugin_handle), NULL);
	return plugin_handle->priv->about;
}

const char*
anjuta_plugin_handle_get_icon_path (AnjutaPluginHandle *plugin_handle)
{
	g_return_val_if_fail (ANJUTA_IS_PLUGIN_HANDLE (plugin_handle), NULL);
	return plugin_handle->priv->icon_path;
}

gboolean
anjuta_plugin_handle_get_user_activatable (AnjutaPluginHandle *plugin_handle)
{
	g_return_val_if_fail (ANJUTA_IS_PLUGIN_HANDLE (plugin_handle), FALSE);
	return plugin_handle->priv->user_activatable;
}

AnjutaPluginDescription*
anjuta_plugin_handle_get_description (AnjutaPluginHandle *plugin_handle)
{
	g_return_val_if_fail (ANJUTA_IS_PLUGIN_HANDLE (plugin_handle), NULL);
	return plugin_handle->priv->description;
}

GList*
anjuta_plugin_handle_get_dependency_names (AnjutaPluginHandle *plugin_handle)
{
	g_return_val_if_fail (ANJUTA_IS_PLUGIN_HANDLE (plugin_handle), NULL);
	return plugin_handle->priv->dependency_names;
}

GHashTable*
anjuta_plugin_handle_get_dependencies (AnjutaPluginHandle *plugin_handle)
{
	g_return_val_if_fail (ANJUTA_IS_PLUGIN_HANDLE (plugin_handle), NULL);
	return plugin_handle->priv->dependencies;
}

GHashTable*
anjuta_plugin_handle_get_dependents (AnjutaPluginHandle *plugin_handle)
{
	g_return_val_if_fail (ANJUTA_IS_PLUGIN_HANDLE (plugin_handle), NULL);
	return plugin_handle->priv->dependents;
}

GList*
anjuta_plugin_handle_get_interfaces (AnjutaPluginHandle *plugin_handle)
{
	g_return_val_if_fail (ANJUTA_IS_PLUGIN_HANDLE (plugin_handle), NULL);
	return plugin_handle->priv->interfaces;
}

gboolean
anjuta_plugin_handle_get_can_load (AnjutaPluginHandle *plugin_handle)
{
	g_return_val_if_fail (ANJUTA_IS_PLUGIN_HANDLE (plugin_handle), FALSE);
	return plugin_handle->priv->can_load;
}

gboolean
anjuta_plugin_handle_get_checked (AnjutaPluginHandle *plugin_handle)
{
	g_return_val_if_fail (ANJUTA_IS_PLUGIN_HANDLE (plugin_handle), FALSE);
	return plugin_handle->priv->checked;
}

gint
anjuta_plugin_handle_get_resolve_pass (AnjutaPluginHandle *plugin_handle)
{
	g_return_val_if_fail (ANJUTA_IS_PLUGIN_HANDLE (plugin_handle), 0);
	return plugin_handle->priv->resolve_pass;
}

void
anjuta_plugin_handle_set_can_load (AnjutaPluginHandle *plugin_handle,
								   gboolean can_load)
{
	g_return_if_fail (ANJUTA_IS_PLUGIN_HANDLE (plugin_handle));
	plugin_handle->priv->can_load = can_load;
}

void
anjuta_plugin_handle_set_checked (AnjutaPluginHandle *plugin_handle,
								  gboolean checked)
{
	g_return_if_fail (ANJUTA_IS_PLUGIN_HANDLE (plugin_handle));
	plugin_handle->priv->checked = checked;
}

void
anjuta_plugin_handle_set_resolve_pass (AnjutaPluginHandle *plugin_handle,
									   gboolean resolve_pass)
{
	g_return_if_fail (ANJUTA_IS_PLUGIN_HANDLE (plugin_handle));
	plugin_handle->priv->resolve_pass = resolve_pass;
}
