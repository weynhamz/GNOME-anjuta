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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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

#include "anjuta-profile.h"
#include "anjuta-marshal.h"
#include "anjuta-debug.h"

enum
{
	PROP_0,
	PROP_PLUGIN_MANAGER,
	PROP_PROFILE_NAME,
	PROP_PROFILE_PLUGINS,
	PROP_SYNC_FILE,
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
	GFile *sync_file;
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
	case PROP_SYNC_FILE:
		if (priv->sync_file)
				g_object_unref (priv->sync_file);
		priv->sync_file = g_value_dup_object (value);
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
	case PROP_SYNC_FILE:
		g_value_set_object (value, priv->sync_file);
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
	                                 PROP_SYNC_FILE,
	                                 g_param_spec_object ("sync-file",
											  _("Synchronization file"),
											  _("File to sync the profile xml"),
											  G_TYPE_FILE,
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
											  _("Select a plugin"),
											  _("Please select a plugin from the list"),
											  descs);
			if (d)
				selected_plugins = g_list_prepend (selected_plugins, d);
		}
		node = g_list_next (node);
	}
	return g_list_reverse (selected_plugins);
}

static GList *
anjuta_profile_read_plugins_from_xml (AnjutaProfile *profile,
									  GFile *file,
									  GError **error)
{
	gchar *read_buf;
	gsize size;
	xmlDocPtr xml_doc;
	GList *descs_list = NULL;
	GList *not_found_names = NULL;
	GList *not_found_urls = NULL;
	gboolean parse_error;

	/* Read xml file */
	if (!g_file_load_contents (file, NULL, &read_buf, &size, NULL, error))
	{
		return NULL;
	}
	
	/* Parse xml file */
	parse_error = TRUE;
	xml_doc = xmlParseMemory (read_buf, size);
	g_free (read_buf);
	if (xml_doc != NULL)
	{
		xmlNodePtr xml_root;
		
		xml_root = xmlDocGetRootElement(xml_doc);
		if (xml_root &&
			(xml_root->name) &&
			xmlStrEqual(xml_root->name, (const xmlChar *)"anjuta"))
		{
			xmlNodePtr xml_node;

			parse_error = FALSE;
			for (xml_node = xml_root->xmlChildrenNode; xml_node; xml_node = xml_node->next)
			{
				GList *groups = NULL;
				GList *attribs = NULL;
				GList *values = NULL;
				xmlChar *name, *url, *mandatory_text;
				xmlNodePtr xml_require_node;
				gboolean mandatory;

				if (!xml_node->name ||
					!xmlStrEqual (xml_node->name, (const xmlChar*)"plugin"))
				{
					continue;
				}
	
				name = xmlGetProp (xml_node, (const xmlChar*)"name");
				url = xmlGetProp (xml_node, (const xmlChar*)"url");
		
				/* Ensure that both name is given */
				if (!name)
				{
					g_warning ("XML error: Plugin name should be present in plugin tag");
					parse_error = TRUE;
					break;
				}
				if (!url)
					url = xmlCharStrdup ("http://anjuta.org/plugins/");
									 
				/* Check if the plugin is mandatory */
				mandatory_text = xmlGetProp (xml_node, (const xmlChar*)"mandatory");
				mandatory = mandatory_text && (xmlStrcasecmp (mandatory_text, (const xmlChar *)"yes") == 0);
				xmlFree(mandatory_text);
										 
				/* For all plugin attribute conditions */
				for (xml_require_node = xml_node->xmlChildrenNode;
			 		xml_require_node;
			 		xml_require_node = xml_require_node->next)
				{
					xmlChar *group;
					xmlChar *attrib;
					xmlChar *value;
		
					if (!xml_require_node->name ||
						!xmlStrEqual (xml_require_node->name,
									  (const xmlChar*)"require"))
					{
						continue;
					}
					group = xmlGetProp (xml_require_node,
										(const xmlChar *)"group");
					attrib = xmlGetProp(xml_require_node,
										(const xmlChar *)"attribute");
					value = xmlGetProp(xml_require_node,
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
						parse_error = TRUE;
						g_warning ("XML parse error: group, attribute and value should be defined in require");
						break;
					}
				}

				if (!parse_error)
				{
					if (g_list_length (groups) == 0)
					{
						parse_error = TRUE;
						g_warning ("XML Error: No attributes to match given");
					}
					else
					{
						GList *plugin_descs;
						
						plugin_descs =
							anjuta_plugin_manager_list_query (profile->priv->plugin_manager,
															  groups,
															  attribs,
															  values);
						if (plugin_descs)
						{
							descs_list = g_list_prepend (descs_list, plugin_descs);
						}
						else if (mandatory)
						{
							not_found_names = g_list_prepend (not_found_names, g_strdup ((const gchar *)name));
							not_found_urls = g_list_prepend (not_found_urls, g_strdup ((const gchar *)url));
						}
					}
				}
				g_list_foreach (groups, (GFunc)xmlFree, NULL);
				g_list_foreach (attribs, (GFunc)xmlFree, NULL);
				g_list_foreach (values, (GFunc)xmlFree, NULL);
				g_list_free (groups);
				g_list_free (attribs);
				g_list_free (values);
				xmlFree (name);
				xmlFree (url);
			}
		}
		xmlFreeDoc(xml_doc);
	}
	
	if (parse_error)
	{
		/* Error during parsing */
		gchar *uri = g_file_get_uri (file);

		g_set_error (error, ANJUTA_PROFILE_ERROR,
					 ANJUTA_PROFILE_ERROR_URI_READ_FAILED,
					 _("Failed to read '%s': XML parse error. "
					   "Invalid or corrupted anjuta plugins profile."),
			 		uri);
		g_free (uri);
		
		g_list_foreach (descs_list, (GFunc)g_list_free, NULL);
		g_list_free (descs_list);
		descs_list = NULL;
	}
	else if (not_found_names)
	{
		/*
	 	* FIXME: Present a nice dialog box to promt the user to download
		* the plugin from corresponding URLs, install them and proceed.
 		*/	
		GList *node_name, *node_url;
		GString *mesg = g_string_new ("");
		gchar *uri = g_file_get_uri (file);
			
		not_found_names = g_list_reverse (not_found_names);
		not_found_urls = g_list_reverse (not_found_urls);
		
		node_name = not_found_names;
		node_url = not_found_urls;
		while (node_name)
		{
			/* <Pluginname>: Install it from <some location on the web> */
			g_string_append_printf (mesg, _("%s: Install it from '%s'\n"),
											(char *)node_name->data,
											(char*)node_url->data);
			node_name = g_list_next (node_name);
			node_url = g_list_next (node_url);
		}
		g_set_error (error, ANJUTA_PROFILE_ERROR,
					 ANJUTA_PROFILE_ERROR_URI_READ_FAILED,
					 _("Failed to read '%s': Following mandatory plugins are missing:\n%s"),
					 uri, mesg->str);
		g_free (uri);
		g_string_free (mesg, TRUE);
		
		g_list_foreach (descs_list, (GFunc)g_list_free, NULL);
		g_list_free (descs_list);
		descs_list = NULL;
	}
	g_list_foreach (not_found_names, (GFunc)g_free, NULL);
	g_list_free (not_found_names);
	g_list_foreach (not_found_urls, (GFunc)g_free, NULL);
	g_list_free (not_found_urls);

	return descs_list;
}

gboolean
anjuta_profile_add_plugins_from_xml (AnjutaProfile *profile,
									 GFile* profile_xml_file,
									 gboolean exclude_from_sync,
									 GError **error)
{
	AnjutaProfilePriv *priv;
	GList *descs_list = NULL;
	
	g_return_val_if_fail (ANJUTA_IS_PROFILE (profile), FALSE);
	
	priv = profile->priv;
	descs_list = anjuta_profile_read_plugins_from_xml (profile, profile_xml_file, error);
	
	if (descs_list)
	{
		GList *selected_plugins = NULL;
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
	
	return descs_list != NULL;
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
				node = g_list_next (node);
				continue;
			}
			g_free (user_activatable);
			/* Do not use the _locale_ version because it's not in UI */
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
			/* Do not use the _locale_ version because it's debugging */
			anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
												  "Name", &name);
			DEBUG_PRINT ("excluding plugin: %s", name);
			g_free (name);
		}
		node = g_list_next (node);
	}
	g_string_append (str, "</anjuta>\n");
	return g_string_free (str, FALSE);
}

void
anjuta_profile_set_sync_file (AnjutaProfile *profile, GFile *sync_file)
{
	AnjutaProfilePriv *priv;
	
	g_return_if_fail (ANJUTA_IS_PROFILE (profile));
	
	priv = profile->priv;
	
	if (priv->sync_file)
		g_object_unref (priv->sync_file);
	priv->sync_file = sync_file;
	if (priv->sync_file);
		g_object_ref (priv->sync_file);
}

gboolean
anjuta_profile_sync (AnjutaProfile *profile, GError **error)
{
	gboolean ok;
	gchar *xml_buffer;
	AnjutaProfilePriv *priv;
	GError* file_error = NULL;
	
	g_return_val_if_fail (ANJUTA_IS_PROFILE (profile), FALSE);
	priv = profile->priv;
	
	if (!priv->sync_file)
		return FALSE;
	
	xml_buffer = anjuta_profile_to_xml (profile);
	ok = g_file_replace_contents (priv->sync_file, xml_buffer, strlen(xml_buffer),
								  NULL, FALSE, G_FILE_CREATE_NONE,
								  NULL, NULL, &file_error);
	if (!ok && g_error_matches (file_error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
	{
		/* Try to create parent directory */
		GFile* parent = g_file_get_parent (priv->sync_file);
		if (g_file_make_directory (parent, NULL, NULL))
		{
			g_clear_error (&file_error);
			ok = g_file_replace_contents (priv->sync_file, xml_buffer, strlen(xml_buffer),
										  NULL, FALSE, G_FILE_CREATE_NONE,
										  NULL, NULL, &file_error);
		}
		g_object_unref (parent);
	}
	g_free (xml_buffer);
	if (file_error != NULL) g_propagate_error (error, file_error);
	
	return ok;
}
