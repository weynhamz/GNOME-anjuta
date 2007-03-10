/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-profile.c
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
 * SECTION:anjuta-profile
 * @short_description: Profile is a collection of plugins
 * @see_also: #AnjutaProfileManager, #AnjutaPlugin
 * @stability: Unstable
 * @include: libanjuta/anjuta-profile.h
 * 
 */

#include <glib/gi18n.h>
#include <glib/gerror.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libgnomevfs/gnome-vfs.h>

#include "anjuta-profile.h"
#include "anjuta-marshal.h"
#include "anjuta-debug.h"

enum
{
	PROP_0,
	PROP_PLUGIN_MANAGER,
	PROP_PROFILE_NAME,
	PROP_PROFILE_PLUGINS,
	PROP_SYNC_URI,
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
	AnjutaPluginManager *plugin_manager;
	GList *plugins;
	GHashTable *plugins_hash;
	GHashTable *plugins_to_exclude_from_sync;
	gchar *sync_uri;
};

static GObjectClass* parent_class = NULL;
static guint profile_signals[LAST_SIGNAL] = { 0 };

GQuark 
anjuta_profile_error_quark (void)
{
	static GQuark quark = 0;
	
	if (quark == 0) {
		quark = g_quark_from_static_string ("anjuta-profile-quark");
	}
	
	return quark;
}

static void
anjuta_profile_init (AnjutaProfile *object)
{
	object->priv = g_new0 (AnjutaProfilePriv, 1);
	object->priv->plugins_hash = g_hash_table_new (g_direct_hash,
												   g_direct_equal);
	object->priv->plugins_to_exclude_from_sync =
		g_hash_table_new (g_direct_hash, g_direct_equal);
}

static void
anjuta_profile_finalize (GObject *object)
{
	AnjutaProfilePriv *priv = ANJUTA_PROFILE (object)->priv;
	g_free (priv->name);
	if (priv->plugins)
		g_list_free (priv->plugins);
	g_hash_table_destroy (priv->plugins_hash);
	g_hash_table_destroy (priv->plugins_to_exclude_from_sync);
	
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
	case PROP_PLUGIN_MANAGER:
		priv->plugin_manager = g_value_get_object (value);
		break;
	case PROP_PROFILE_NAME:
		g_return_if_fail (g_value_get_string (value) != NULL);
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
	case PROP_SYNC_URI:
		g_free (priv->sync_uri);
		priv->sync_uri = NULL;
		if (g_value_get_string (value) != NULL)
			priv->sync_uri = g_value_dup_string (value);
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
	case PROP_PLUGIN_MANAGER:
		g_value_set_object (value, priv->plugin_manager);
		break;
	case PROP_PROFILE_NAME:
		g_value_set_string (value, priv->name);
		break;
	case PROP_PROFILE_PLUGINS:
		g_value_set_pointer (value, priv->plugins);
		break;
	case PROP_SYNC_URI:
		g_value_set_string (value, priv->sync_uri);
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
	GError *error = NULL;
	anjuta_profile_sync (self, &error);
	if (error)
	{
		g_warning ("Failed to synchronize plugins profile '%s': %s",
				   self->priv->name, error->message);
		g_error_free (error);
	}
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
	                                 PROP_PLUGIN_MANAGER,
	                                 g_param_spec_object ("plugin-manager",
											  _("Plugin Manager"),
											  _("The plugin manager to use for resolving plugins"),
											  ANJUTA_TYPE_PLUGIN_MANAGER,
											  G_PARAM_READABLE |
											  G_PARAM_WRITABLE |
											  G_PARAM_CONSTRUCT));
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
	g_object_class_install_property (object_class,
	                                 PROP_SYNC_URI,
	                                 g_param_spec_string ("sync-uri",
											  _("Synchronization URI"),
											  _("URI to sync the proflie xml"),
											  NULL,
											  G_PARAM_READABLE |
											  G_PARAM_WRITABLE));

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
anjuta_profile_new (const gchar* name, AnjutaPluginManager *plugin_manager)
{
	GObject *profile;
	profile = g_object_new (ANJUTA_TYPE_PROFILE, "profile-name", name,
							"plugin-manager", plugin_manager, NULL);
	return ANJUTA_PROFILE (profile);
}

const gchar*
anjuta_profile_get_name (AnjutaProfile *profile)
{
	AnjutaProfilePriv *priv;
	g_return_val_if_fail (ANJUTA_IS_PROFILE (profile), NULL);
	priv = ANJUTA_PROFILE (profile)->priv;
	return priv->name;
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

GList*
anjuta_profile_get_plugins (AnjutaProfile *profile)
{
	AnjutaProfilePriv *priv;
	g_return_val_if_fail (ANJUTA_IS_PROFILE (profile), FALSE);
	priv = ANJUTA_PROFILE (profile)->priv;
	return priv->plugins;
}

/* Profile processing */
/* Returns a list of matching plugins */
static GList*
anjuta_profile_query_plugins (AnjutaProfile *profile,
							  GList *groups, GList *attribs,
							  GList *values)
{
	gchar *sec[5];
	gchar *att[5];
	gchar *val[5];
	GList *sec_node, *att_node, *val_node;
	gint length, i;
	AnjutaProfilePriv *priv;

	priv = profile->priv;
	
	/* FIXME: How to call a variable arguments function dynamically !! */
	length = g_list_length (groups);
	
	g_return_val_if_fail ((length > 0 && length <= 5), NULL);
	
	i = 0;
	sec_node = groups;
	att_node = attribs;
	val_node = values;
	while (sec_node)
	{
		sec[i] = sec_node->data;
		att[i] = att_node->data;
		val[i] = val_node->data;
		
		sec_node = g_list_next (sec_node);
		att_node = g_list_next (att_node);
		val_node = g_list_next (val_node);
		i++;
	}
	
	switch (length)
	{
	case 1:
		return anjuta_plugin_manager_query (priv->plugin_manager,
											sec[0], att[0], val[0], NULL);
	case 2:
		return anjuta_plugin_manager_query (priv->plugin_manager,
											sec[0], att[0], val[0],
											sec[1], att[1], val[1],
											NULL);
	case 3:
		return anjuta_plugin_manager_query (priv->plugin_manager,
											sec[0], att[0], val[0],
											sec[1], att[1], val[1],
											sec[2], att[2], val[2],
											NULL);
	case 4:
		return anjuta_plugin_manager_query (priv->plugin_manager,
											sec[0], att[0], val[0],
											sec[1], att[1], val[1],
											sec[2], att[2], val[2],
											sec[3], att[3], val[3],
											NULL);
	case 5:
		return anjuta_plugin_manager_query (priv->plugin_manager,
											sec[0], att[0], val[0],
											sec[1], att[1], val[1],
											sec[2], att[2], val[2],
											sec[3], att[3], val[3],
											sec[4], att[4], val[4],
											NULL);
	default:
		g_warning ("FIXME: How to call a variable args function dynamically !!");
	}
	return NULL;
}

static GList*
anjuta_profile_select_plugins (AnjutaProfile *profile,
							   GList *descs_list)
{
	GList *selected_plugins = NULL;
	GList *node = descs_list;
	AnjutaProfilePriv *priv;
	
	priv = profile->priv;
	
	while (node)
	{
		GList *descs = node->data;
		if (g_list_length (descs) == 1)
		{
			selected_plugins = g_list_prepend (selected_plugins, descs->data);
		}
		else
		{
			AnjutaPluginDescription* d;
			d = anjuta_plugin_manager_select (priv->plugin_manager,
											  "Select a plugin",
											  "Please select a plugin from the list",
											  descs);
			if (d)
				selected_plugins = g_list_prepend (selected_plugins, d);
		}
		node = g_list_next (node);
	}
	return g_list_reverse (selected_plugins);
}

gboolean
anjuta_profile_add_plugins_from_xml (AnjutaProfile *profile,
									 const gchar* profile_xml_uri,
									 gboolean exclude_from_sync,
									 GError **error)
{
	AnjutaProfilePriv *priv;
	xmlDocPtr xml_doc;
	xmlNodePtr xml_root, xml_node;
	GnomeVFSHandle *handle;
	GnomeVFSFileInfo info;
	GnomeVFSResult result;
	int perm, read;
	GList *descs_list = NULL;
	GList *selected_plugins = NULL;
	GList *not_found_names = NULL;
	GList *not_found_urls = NULL;
	gboolean err = FALSE;
	gchar *read_buf = NULL;
	g_return_val_if_fail (ANJUTA_IS_PROFILE (profile), FALSE);
	priv = profile->priv;
	
	result = gnome_vfs_get_file_info (profile_xml_uri, &info,
									  GNOME_VFS_FILE_INFO_DEFAULT |
									  GNOME_VFS_FILE_INFO_GET_ACCESS_RIGHTS);

	/* If I got the info to check it out */
	if(result != GNOME_VFS_OK )
	{
		g_set_error (error, ANJUTA_PROFILE_ERROR,
					 ANJUTA_PROFILE_ERROR_URI_READ_FAILED,
					 _("Failed to read '%s': %s"),
					 profile_xml_uri,
					 gnome_vfs_result_to_string (result));
		return FALSE;
	}
	
	/* FIXME: Fix this bit masking */
	perm = (info.permissions-(int)(info.permissions/65536)*65536);
	read = (int)(perm/256);

	if(read == 0)
	{
		g_set_error (error, ANJUTA_PROFILE_ERROR,
					 ANJUTA_PROFILE_ERROR_URI_READ_FAILED,
					 _("No read permission for: %s"),
					 profile_xml_uri);
		return FALSE;
	}
	if((result = gnome_vfs_open (&handle, profile_xml_uri,
								 GNOME_VFS_OPEN_READ)) != GNOME_VFS_OK)
	{
		g_set_error (error, ANJUTA_PROFILE_ERROR,
					 ANJUTA_PROFILE_ERROR_URI_READ_FAILED,
					 _("Failed to read '%s': %s"),
					 profile_xml_uri,
					 gnome_vfs_result_to_string (result));
		return FALSE;
	}
	read_buf = g_new0(char, info.size + 1);
	
	result = gnome_vfs_read (handle, read_buf, info.size, NULL);
	if(!(result == GNOME_VFS_OK || result == GNOME_VFS_ERROR_EOF))
	{
		g_free (read_buf);
		g_set_error (error, ANJUTA_PROFILE_ERROR,
					 ANJUTA_PROFILE_ERROR_URI_READ_FAILED,
					 _("Failed to read '%s': %s"),
					 profile_xml_uri,
					 gnome_vfs_result_to_string (result));
		return FALSE;
	}
	gnome_vfs_close (handle);
	xml_doc = xmlParseMemory (read_buf, info.size);
	g_free(read_buf);
	if(xml_doc == NULL)
	{
		g_set_error (error, ANJUTA_PROFILE_ERROR,
					 ANJUTA_PROFILE_ERROR_URI_READ_FAILED,
					 _("Failed to read '%s': XML parse error"),
					 profile_xml_uri);
		return FALSE;
	}
	
	xml_root = xmlDocGetRootElement(xml_doc);
	if(xml_root == NULL)
	{
		g_set_error (error, ANJUTA_PROFILE_ERROR,
					 ANJUTA_PROFILE_ERROR_URI_READ_FAILED,
					 _("Failed to read '%s': XML parse error"),
					 profile_xml_uri);
		/* FIXME: Free xml doc */
		return FALSE;
	}
	
	if(!xml_root->name ||
	   !xmlStrEqual(xml_root->name, (const xmlChar *)"anjuta"))
	{
		g_set_error (error, ANJUTA_PROFILE_ERROR,
					 ANJUTA_PROFILE_ERROR_URI_READ_FAILED,
					 _("Failed to read '%s': XML parse error."
					   "Invalid or corrupted anjuta plugins profile."),
					 profile_xml_uri);
		return FALSE;
	}
	
	error = FALSE;
	descs_list = NULL;
	not_found_names = NULL;
	not_found_urls = NULL;
	
	xml_node = xml_root->xmlChildrenNode;
	while (xml_node && !error)
	{
		GList *groups = NULL;
		GList *attribs = NULL;
		GList *values = NULL;
		GList *plugin_descs;
		gchar *name, *url, *mandatory_text;
		xmlNodePtr xml_require_node;
		gboolean mandatory;

		if (!xml_node->name ||
			!xmlStrEqual (xml_node->name, (const xmlChar*)"plugin"))
		{
			xml_node = xml_node->next;
			continue;
		}
		
		name = (gchar*) xmlGetProp (xml_node, (const xmlChar*)"name");
		url = (gchar*) xmlGetProp (xml_node, (const xmlChar*)"url");
		
		/* Ensure that both name is given */
		if (!name)
		{
			g_warning ("XML error: Plugin name should be present in plugin tag");
			err = TRUE;
			break;
		}
		if (!url)
			url = g_strdup ("http://anjuta.org/plugins/");
		
		/* Check if the plugin is mandatory */
		mandatory_text = (gchar*) xmlGetProp (xml_node,
											  (const xmlChar*)"mandatory");
		if (mandatory_text && strcasecmp (mandatory_text, "yes") == 0)
			mandatory = TRUE;
		else
			mandatory = FALSE;
		
		/* For all plugin attribute conditions */
		xml_require_node = xml_node->xmlChildrenNode;
		while (xml_require_node && !error)
		{
			gchar *group;
			gchar *attrib;
			gchar *value;
			
			if (!xml_require_node->name ||
				!xmlStrEqual (xml_require_node->name,
							  (const xmlChar*)"require"))
			{
				xml_require_node = xml_require_node->next;
				continue;
			}
			group = (gchar*) xmlGetProp (xml_require_node,
										 (const xmlChar *)"group");
			attrib = (gchar*) xmlGetProp(xml_require_node,
										 (const xmlChar *)"attribute");
			value = (gchar*) xmlGetProp(xml_require_node,
										(const xmlChar *)"value");
			
			if (group && attrib && value)
			{
				groups = g_list_prepend (groups, group);
				attribs = g_list_prepend (attribs, attrib);
				values = g_list_prepend (values, value);
			}
			else
			{
				if (group) xmlFree (group);
				if (attrib) xmlFree (attrib);
				if (value) xmlFree (value);
				err = TRUE;
				g_warning ("XML parse error: group, attribute and value should be defined in require");
				break;
			}
			xml_require_node = xml_require_node->next;
		}
		if (error)
		{
			g_list_foreach (groups, (GFunc)xmlFree, NULL);
			g_list_foreach (attribs, (GFunc)xmlFree, NULL);
			g_list_foreach (values, (GFunc)xmlFree, NULL);
			g_list_free (groups);
			g_list_free (attribs);
			g_list_free (values);
			xmlFree (name);
			xmlFree (url);
			break;
		}
		if (g_list_length (groups) == 0)
		{
			err = TRUE;
			g_warning ("XML Error: No attributes to match given");
			xmlFree (name);
			xmlFree (url);
			break;
		}
		if (g_list_length (groups) > 5)
		{
			err = TRUE;
			g_warning ("XML Error: Maximum 5 attributes can be given (FIXME)");
			xmlFree (name);
			xmlFree (url);
			break;
		}
		plugin_descs = anjuta_profile_query_plugins (profile,
													 groups, attribs,
													 values);
		if (plugin_descs)
		{
			descs_list = g_list_prepend (descs_list, plugin_descs);
			xmlFree (name);
			xmlFree (url);
		}
		else if (mandatory)
		{
			not_found_names = g_list_prepend (not_found_names, name);
			not_found_urls = g_list_prepend (not_found_urls, url);
		}
		else
		{
			xmlFree (name);
			xmlFree (url);
		}
		xml_node = xml_node->next;
	}
	if (error)
	{
		g_list_foreach (not_found_names, (GFunc)xmlFree, NULL);
		g_list_foreach (not_found_urls, (GFunc)xmlFree, NULL);
		g_list_foreach (descs_list, (GFunc)g_list_free, NULL);
		g_list_free (not_found_names);
		g_list_free (not_found_urls);
		g_list_free (descs_list);
		
		g_set_error (error, ANJUTA_PROFILE_ERROR,
					 ANJUTA_PROFILE_ERROR_URI_READ_FAILED,
					 _("Failed to read '%s': XML parse error."
					   "Invalid or corrupted anjuta plugins profile."),
					 profile_xml_uri);
		return FALSE;
	}
	if (not_found_names)
	{
		/*
		 * FIXME: Present a nice dialog box to promt the user to download
		 * the plugin from corresponding URLs, install them and proceed.
		 */
		GList *node_name, *node_url;
		GString *mesg = g_string_new ("");
		
		not_found_names = g_list_reverse (not_found_names);
		not_found_urls = g_list_reverse (not_found_urls);
		
		node_name = not_found_names;
		node_url = not_found_urls;
		while (node_name)
		{
			g_string_append_printf (mesg, _("%s: Install it from '%s'\n"),
											(char *)node_name->data,
											(char*)node_url->data);
			node_name = g_list_next (node_name);
			node_url = g_list_next (node_url);
		}
		g_set_error (error, ANJUTA_PROFILE_ERROR,
					 ANJUTA_PROFILE_ERROR_URI_READ_FAILED,
					 _("Failed to read '%s': Following mandatory plugins are missing:\n%s"),
					 profile_xml_uri, mesg->str);
		g_string_free (mesg, TRUE);
		
		/* FIXME: It should not return like this ... */
		g_list_foreach (not_found_names, (GFunc)xmlFree, NULL);
		g_list_foreach (not_found_urls, (GFunc)xmlFree, NULL);
		g_list_foreach (descs_list, (GFunc)g_list_free, NULL);
		g_list_free (not_found_names);
		g_list_free (not_found_urls);
		g_list_free (descs_list);
		
		return FALSE;
	}
	if (descs_list)
	{
		GList *node;
		
		/* Now everything okay. Select the plugins */
		descs_list = g_list_reverse (descs_list);
		selected_plugins =
			anjuta_profile_select_plugins (profile, descs_list);
		g_list_foreach (descs_list, (GFunc)g_list_free, NULL);
		g_list_free (descs_list);
		
		node = selected_plugins;
		while (node)
		{
			if (exclude_from_sync)
			{
				g_hash_table_insert (priv->plugins_to_exclude_from_sync,
									 node->data, node->data);
			}
			
			/* Insure no duplicate plugins are added */
			if (g_hash_table_lookup (priv->plugins_hash, node->data) == NULL)
			{
				priv->plugins = g_list_append (priv->plugins, node->data);
				g_hash_table_insert (priv->plugins_hash,
									 node->data, node->data);
			}
			node = g_list_next (node);
		}
		g_list_free (selected_plugins);
	}
	return TRUE;
}

gchar*
anjuta_profile_to_xml (AnjutaProfile *profile)
{
	GList *node;
	GString *str;
	AnjutaProfilePriv *priv;
	
	g_return_val_if_fail (ANJUTA_IS_PROFILE (profile), FALSE);
	priv = profile->priv;
	
	str = g_string_new ("<?xml version=\"1.0\"?>\n<anjuta>\n");
	node = priv->plugins;
	while (node)
	{
		AnjutaPluginDescription *desc;
		desc = (AnjutaPluginDescription *)node->data;
		if (!g_hash_table_lookup (priv->plugins_to_exclude_from_sync,
								  node->data))
		{
			
			gchar *user_activatable = NULL;
			gchar *name = NULL, *plugin_id = NULL;
			
			
			anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
												  "UserActivatable",
												  &user_activatable);
			/* Do not save plugins that are auto activated */
			if (user_activatable && strcmp (user_activatable, "no") == 0)
			{
				g_free (user_activatable);
				break;
			}
			g_free (user_activatable);
			
			anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
												  "Name", &name);
			DEBUG_PRINT("Saving plugin: %s", name);
			if (!name)
				name = g_strdup ("Unknown");
			
			if (anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
													  "Location", &plugin_id))
			{
				g_string_append (str, "    <plugin name=\"");
				g_string_append (str, name);
				g_string_append (str, "\" mandatory=\"no\">\n");
				g_string_append (str, "        <require group=\"Anjuta Plugin\"\n");
				g_string_append (str, "                 attribute=\"Location\"\n");
				g_string_append (str, "                 value=\"");
				g_string_append (str, plugin_id);
				g_string_append (str, "\"/>\n");
				g_string_append (str, "    </plugin>\n");
				
				g_free (plugin_id);
			}
			g_free (name);
		}
		else
		{
			gchar* name;
			anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
												  "Name", &name);
			DEBUG_PRINT("excluding plugin: %s", name);
			g_free(name);
		}
		node = g_list_next (node);
	}
	g_string_append (str, "</anjuta>\n");
	return g_string_free (str, FALSE);
}

void
anjuta_profile_set_sync_uri (AnjutaProfile *profile, const gchar *sync_uri)
{
	AnjutaProfilePriv *priv;
	
	g_return_if_fail (ANJUTA_IS_PROFILE (profile));
	priv = profile->priv;
	g_free (priv->sync_uri);
	priv->sync_uri = NULL;
	if (sync_uri)
		priv->sync_uri = g_strdup (sync_uri);
}

gboolean
anjuta_profile_sync (AnjutaProfile *profile, GError **error)
{
	GnomeVFSHandle* vfs_write;
	GnomeVFSResult result;
	GnomeVFSFileSize nchars;
	gchar *xml_buffer;
	AnjutaProfilePriv *priv;
	
	g_return_val_if_fail (ANJUTA_IS_PROFILE (profile), FALSE);
	priv = profile->priv;
	
	if (!priv->sync_uri)
		return FALSE;
	
	xml_buffer = anjuta_profile_to_xml (profile);
	result = gnome_vfs_create (&vfs_write, priv->sync_uri,
							   GNOME_VFS_OPEN_WRITE,
							   FALSE, 0664);
 	if (result == GNOME_VFS_OK)
	{
		result = gnome_vfs_write (vfs_write, xml_buffer, strlen (xml_buffer),
								  &nchars);
		gnome_vfs_close (vfs_write);
	}

	if (result != GNOME_VFS_OK)
	{
		g_set_error (error, ANJUTA_PROFILE_ERROR,
					 ANJUTA_PROFILE_ERROR_URI_WRITE_FAILED,
					 "%s", gnome_vfs_result_to_string (result));
	}
	g_free (xml_buffer);
	return (result == GNOME_VFS_OK);
}
