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
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/interfaces/ianjuta-help.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-message-manager.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include <libanjuta/interfaces/ianjuta-profile.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/plugins.h>
#include <libgnomevfs/gnome-vfs.h>

#include "plugin.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-default-profile.ui"
#define DEFAULT_PROFILE "file://"PACKAGE_DATA_DIR"/profiles/default.anjuta"

static gpointer parent_class;

static void default_profile_plugin_close (DefaultProfilePlugin *plugin);

static void
on_session_save (AnjutaShell *shell, AnjutaSessionPhase phase,
				 AnjutaSession *session, DefaultProfilePlugin *plugin)
{
	GList *files;
	
	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;
	/*
	 * When a project session is being saved (session_by_me == TRUE),
	 * we should not save the current project uri, because project 
	 * sessions are loaded when the project has already been loaded.
	 */
	if (plugin->project_uri && !plugin->session_by_me)
	{
		files = anjuta_session_get_string_list (session, "File Loader",
												"Files");
		files = g_list_append (files, g_strdup (plugin->project_uri));
		anjuta_session_set_string_list (session, "File Loader", "Files",
										files);
		g_list_foreach (files, (GFunc)g_free, NULL);
		g_list_free (files);
	}
}

static void
update_ui (DefaultProfilePlugin *plugin)
{
	AnjutaUI *ui;
	GtkAction *action;
			
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (plugin)->shell, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupProfile",
								   "ActionProfileCloseProject");
	g_object_set (G_OBJECT (action), "sensitive",
				  (plugin->project_uri != NULL), NULL);
}

static void
on_queue_foreach_disconnect (GObject *obj, DefaultProfilePlugin* plugin)
{
	g_signal_handlers_disconnect_by_func (obj, G_CALLBACK (g_list_remove),
										  plugin->loaded_plugins);
}

static void
on_queue_foreach_unload (GObject *obj, DefaultProfilePlugin* plugin)
{
		anjuta_plugins_unload_plugin (ANJUTA_PLUGIN (plugin)->shell, obj);
}

static gboolean
on_close_project_idle (gpointer plugin)
{
	default_profile_plugin_close ((DefaultProfilePlugin *)plugin);
	return FALSE;
}

static void
on_close_project (GtkAction *action, DefaultProfilePlugin *plugin)
{
	if (plugin->project_uri)
		g_idle_add (on_close_project_idle, plugin);
}

static GtkActionEntry pf_actions[] = 
{
	{
		"ActionProfileCloseProject", NULL,
		N_("Close Pro_ject"), NULL, N_("Close project"),
		G_CALLBACK (on_close_project)
	},
};

static gboolean
default_profile_plugin_activate_plugin (AnjutaPlugin *plugin)
{
	DefaultProfilePlugin *pf_plugin;
	AnjutaUI *ui;
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	pf_plugin = (DefaultProfilePlugin *)plugin;
	
	/* Create actions */
	pf_plugin->action_group = 
		anjuta_ui_add_action_group_entries (ui,
											"ActionGroupProfile",
											_("Profile actions"),
											pf_actions,
											G_N_ELEMENTS (pf_actions),
											plugin);
	/* Merge UI */
	pf_plugin->merge_id = anjuta_ui_merge (ui, UI_FILE);
	update_ui (pf_plugin);
	
	/* Connect to save session */
	g_signal_connect (G_OBJECT (plugin->shell), "save_session",
					  G_CALLBACK (on_session_save), plugin);
	return TRUE;
}

static gboolean
default_profile_plugin_deactivate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	DefaultProfilePlugin *pf_plugin;
	
	pf_plugin = (DefaultProfilePlugin *)plugin;
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	g_signal_handlers_disconnect_by_func (G_OBJECT (plugin->shell),
										  G_CALLBACK (on_session_save), plugin);
	/* Close project if it's open */
	if (pf_plugin->project_uri)
		default_profile_plugin_close (pf_plugin);
	
	anjuta_ui_unmerge (ui, pf_plugin->merge_id);
	anjuta_ui_remove_action_group (ui, pf_plugin->action_group);
	
	return TRUE;
}

static void
default_profile_plugin_dispose (GObject *obj)
{
	DefaultProfilePlugin* plugin = (DefaultProfilePlugin *)obj;
	
	if (plugin->project_uri)
	{
		g_free (plugin->project_uri);
		plugin->project_uri = NULL;
	}
	if (plugin->default_plugins)
	{
		g_hash_table_destroy (plugin->default_plugins);
		plugin->default_plugins = NULL;
	}
	if (plugin->loaded_plugins)
	{
		g_queue_foreach (plugin->loaded_plugins,
						 (GFunc) on_queue_foreach_disconnect,
						 plugin);
		plugin->loaded_plugins = NULL;
	}	
	
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
default_profile_plugin_instance_init (GObject *obj)
{
	DefaultProfilePlugin* plugin = (DefaultProfilePlugin*)obj;
	plugin->project_uri = NULL;
	plugin->merge_id = 0;
	plugin->action_group = NULL;
	plugin->loaded_plugins = NULL;
	plugin->default_plugins = NULL;
	plugin->session_by_me = FALSE;
}

static void
default_profile_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = default_profile_plugin_activate_plugin;
	plugin_class->deactivate = default_profile_plugin_deactivate_plugin;
	klass->dispose = default_profile_plugin_dispose;
}

/* Returns a list of matching plugins */
static GSList*
default_profile_plugin_query_plugins (DefaultProfilePlugin *plugin,
									  GSList *groups, GSList *attribs,
									  GSList *values)
{
	gchar *sec[5];
	gchar *att[5];
	gchar *val[5];
	GSList *sec_node, *att_node, *val_node;
	gint length, i;
	
	/* FIXME: How to call a variable arguments function dynamically !! */
	length = g_slist_length (groups);
	
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
		
		sec_node = g_slist_next (sec_node);
		att_node = g_slist_next (att_node);
		val_node = g_slist_next (val_node);
		i++;
	}
	
	switch (length)
	{
	case 1:
		return anjuta_plugins_query (ANJUTA_PLUGIN (plugin)->shell,
									 sec[0], att[0], val[0], NULL);
	case 2:
		return anjuta_plugins_query (ANJUTA_PLUGIN (plugin)->shell,
									 sec[0], att[0], val[0],
									 sec[1], att[1], val[1],
									 NULL);
	case 3:
		return anjuta_plugins_query (ANJUTA_PLUGIN (plugin)->shell,
									 sec[0], att[0], val[0],
									 sec[1], att[1], val[1],
									 sec[2], att[2], val[2],
									 NULL);
	case 4:
		return anjuta_plugins_query (ANJUTA_PLUGIN (plugin)->shell,
									 sec[0], att[0], val[0],
									 sec[1], att[1], val[1],
									 sec[2], att[2], val[2],
									 sec[3], att[3], val[3],
									 NULL);
	case 5:
		return anjuta_plugins_query (ANJUTA_PLUGIN (plugin)->shell,
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


static void
disconnect_plugin_deactivate_signal (GObject *obj)
{
	DefaultProfilePlugin *plugin = 
		g_object_get_data (G_OBJECT (obj), "__default_profile_plugin");
	g_signal_handlers_disconnect_by_func (obj,
										  G_CALLBACK (g_hash_table_remove),
										  plugin->default_plugins);
}

static GSList*
default_profile_plugin_select_plugins (DefaultProfilePlugin *plugin,
									   GSList *descs_list)
{
	GSList *selected_plugins = NULL;
	GSList *node = descs_list;
	while (node)
	{
		GSList *descs = node->data;
		if (g_slist_length (descs) == 1)
		{
			selected_plugins = g_slist_prepend (selected_plugins, descs->data);
		}
		else
		{
			AnjutaPluginDescription* d;
			d = anjuta_plugins_select (ANJUTA_PLUGIN (plugin)->shell,
									   "Select a plugin",
									   "Please select a plugin from the list",
									   descs);
			if (d)
				selected_plugins = g_slist_prepend (selected_plugins, d);
		}
		node = g_slist_next (node);
	}
	return g_slist_reverse (selected_plugins);
}

static void
default_profile_plugin_activate_plugins (DefaultProfilePlugin *plugin,
										 GSList *selected_plugins,
										 ESplash *splash,
										 gboolean default_profile)
{
	GSList *node;
	gint i, max_icons;
	max_icons = 0;
	
	/* First of all close existing project profile */
	if (plugin->project_uri)
		default_profile_plugin_close (plugin);
	
	if (splash)
	{
		GdkPixbuf *icon_pixbuf;
		i = 0;
		
		gtk_widget_show (GTK_WIDGET (splash));
		node = selected_plugins;
		while (node)
		{
			AnjutaPluginDescription *desc = node->data;
			gchar *icon_filename;
			gchar *icon_path = NULL;
			
			if (anjuta_plugin_description_get_string (desc,
													  "Anjuta Plugin",
													  "Icon",
												  &icon_filename))
			{
				gchar *title, *description;
				anjuta_plugin_description_get_string (desc,
													  "Anjuta Plugin",
													  "Name",
													  &title);
				anjuta_plugin_description_get_string (desc,
													  "Anjuta Plugin",
													  "Description",
													  &description);
				icon_path = g_strconcat (PACKAGE_PIXMAPS_DIR"/",
										 icon_filename, NULL);
				// g_message ("Icon: %s", icon_path);
				icon_pixbuf = 
				gdk_pixbuf_new_from_file (icon_path, NULL);
				g_free (icon_path);
				if (icon_pixbuf) {
					e_splash_add_icon (E_SPLASH (splash),
									   icon_pixbuf, title,
									   description);
					max_icons++;
					g_object_unref (icon_pixbuf);
				}
			} else {
				g_warning ("Plugin does not define Icon: No such file %s",
						   icon_path);
			}
			i++;
			node = g_slist_next (node);
		}
		
		/* Session loading icon */
		icon_pixbuf = 
			gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/anjuta_icon.png",
									  NULL);
		if (icon_pixbuf)
		{
			e_splash_add_icon (E_SPLASH (splash),
							   icon_pixbuf, _("Last Session ..."),
							   _("Restoring last session ..."));
			max_icons++;
			g_object_unref (icon_pixbuf);
		}
		while (gtk_events_pending ())
			gtk_main_iteration ();
	}
	
	i = 0;
	node = selected_plugins;
	while (node)
	{
		AnjutaPluginDescription *d;
		gchar *plugin_id;
		
		d = node->data;
		if (splash && max_icons > 0)
		{
			e_splash_set_icon_highlight (E_SPLASH (splash), i, TRUE);
			while (gtk_events_pending ())
				gtk_main_iteration ();
		}
		if (anjuta_plugin_description_get_string (d,"Anjuta Plugin",
												  "Location", &plugin_id))
		{
			GObject *plugin_obj;
			
			plugin_obj =
				anjuta_plugins_get_plugin_by_id (ANJUTA_PLUGIN (plugin)->shell,
												 plugin_id);
			if (!default_profile && plugin_obj)
			{
				if (!plugin->default_plugins ||
					!g_hash_table_lookup (plugin->default_plugins, plugin_obj))
				{
					/* The non-default-profile plugin is successfully loaded
					 * and is already not loaded
					 */
					if (!plugin->loaded_plugins)
						plugin->loaded_plugins = g_queue_new ();
					
					/* Store the object for later unloading */
					g_queue_push_tail (plugin->loaded_plugins, plugin_obj);
					
					/* Remove the plugin on deactivation */
					g_signal_connect_swapped (G_OBJECT (plugin_obj),
											  "deactivated",
											  G_CALLBACK (g_queue_remove),
											  plugin->loaded_plugins);
				}
			}
			if (default_profile && plugin_obj)
			{
				/* The default-profile plugin is successfully loade.
				 * Record it.
				 */
				if (!plugin->default_plugins)
					plugin->default_plugins = 
						g_hash_table_new_full (g_direct_hash, g_direct_equal,
						  (GDestroyNotify) disconnect_plugin_deactivate_signal,
											   (GDestroyNotify) g_free);
				g_object_set_data (G_OBJECT (plugin_obj),
								   "__default_profile_plugin", plugin);
				g_hash_table_insert (plugin->default_plugins,
									 plugin_obj, g_strdup (plugin_id));
				/* Remove the plugin on deactivation */
				g_signal_connect_swapped (G_OBJECT (plugin_obj), "deactivated",
										  G_CALLBACK (g_hash_table_remove),
										  plugin->default_plugins);
			}
		}
		i++;
		max_icons--;
		node = g_slist_next (node);
	}
	if (splash && max_icons > 0)
	{
		e_splash_set_icon_highlight (E_SPLASH (splash), i, TRUE);
		while (gtk_events_pending ())
			gtk_main_iteration ();
	}
}

static gboolean
default_profile_plugin_load (DefaultProfilePlugin *plugin,
							 const gchar *uri,
							 ESplash *splash,
							 gboolean default_profile, GError **e)
{
	xmlDocPtr xml_doc;
	xmlNodePtr xml_root, xml_node;
	GnomeVFSHandle *handle;
	GnomeVFSFileInfo info;
	GnomeVFSResult result;
	int perm, read;
	GSList *descs_list, *selected_plugins, *not_found_names, *not_found_urls;
	gboolean error = FALSE;
	gchar *read_buf = NULL;
	
	g_return_val_if_fail (uri != NULL, FALSE);
	
	/* FIXME: Propagate the following errors */
	
	/* Can not load more than one default profile */
	g_return_val_if_fail (!(default_profile && plugin->default_plugins != NULL),
						  FALSE);

	/* Can not load default profile if a project profile is loaded */
	g_return_val_if_fail (!(default_profile && plugin->project_uri), FALSE);
	
	result = gnome_vfs_get_file_info(uri, &info,
									 GNOME_VFS_FILE_INFO_DEFAULT |
									 GNOME_VFS_FILE_INFO_GET_ACCESS_RIGHTS);

	/* If I got the info to check it out */
	if(result != GNOME_VFS_OK )
	{
		anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
								  _("Cannot open: %s\n&s"),
								  uri, gnome_vfs_result_to_string(result));
		return FALSE;
	}
	
	/* FIXME: Fix this bit masking */
	perm = (info.permissions-(int)(info.permissions/65536)*65536);
	read = (int)(perm/256);

	if(read == 0)
	{
		anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
								  _("No read permission for: %s"),
								  uri);
		return FALSE;
	}
	if((result = gnome_vfs_open (&handle, uri,
								 GNOME_VFS_OPEN_READ)) != GNOME_VFS_OK)
	{
		anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
								  _("Cannot open: %s\n&s"),
								  uri, gnome_vfs_result_to_string(result));
		return FALSE;
	}
	read_buf = g_new0(char, info.size + 1);
	
	result = gnome_vfs_read (handle, read_buf, info.size, NULL);
	if(!(result == GNOME_VFS_OK || result == GNOME_VFS_ERROR_EOF))
	{
		g_free (read_buf);
		anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
								  _("Cannot open: %s\n&s"),
								  uri, gnome_vfs_result_to_string(result));
		return FALSE;
	}
	gnome_vfs_close (handle);
	xml_doc = xmlParseMemory (read_buf, info.size);
	g_free(read_buf);
	if(xml_doc == NULL)
	{
		anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
								  _("Cannot open: %s\nXML parse error."),
								  uri);
		return FALSE;
	}
	
	xml_root = xmlDocGetRootElement(xml_doc);
	if(xml_root == NULL)
	{
		anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
								  _("Cannot open: %s\nXML parse error."),
								  uri);
		/* Free xml doc */
		return FALSE;
	}
	
	if(!xml_root->name ||
	   !xmlStrEqual(xml_root->name, (const xmlChar *)"anjuta"))
	{
		anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
								  _("Cannot open: %s\nDoes not look like a valid Anjuta project."),
								  uri);
		return FALSE;
	}
	
	error = FALSE;
	descs_list = NULL;
	not_found_names = NULL;
	not_found_urls = NULL;
	
	xml_node = xml_root->xmlChildrenNode;
	while (xml_node && !error)
	{
		GSList *groups = NULL;
		GSList *attribs = NULL;
		GSList *values = NULL;
		GSList *plugin_descs;
		gchar *name, *url, *mandatory_text;
		xmlNodePtr xml_require_node;
		gboolean mandatory;

		if (!xml_node->name ||
			!xmlStrEqual (xml_node->name, (const xmlChar*)"plugin"))
		{
			xml_node = xml_node->next;
			continue;
		}
		
		name = xmlGetProp (xml_node, (const xmlChar*)"name");
		url = xmlGetProp (xml_node, (const xmlChar*)"url");
		
		/* Ensure that both name is given */
		if (!name)
		{
			g_warning ("XML error: Plugin name should be present in plugin tag");
			error = TRUE;
			break;
		}
		if (!url)
			url = g_strdup ("http://anjuta.org/plugins/");
		
		/* Check if the plugin is mandatory */
		mandatory_text = xmlGetProp (xml_node, (const xmlChar*)"mandatory");
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
				!xmlStrEqual (xml_require_node->name, (const xmlChar*)"require"))
			{
				xml_require_node = xml_require_node->next;
				continue;
			}
			group = xmlGetProp(xml_require_node, (const xmlChar *)"group");
			attrib = xmlGetProp(xml_require_node, (const xmlChar *)"attribute");
			value = xmlGetProp(xml_require_node, (const xmlChar *)"value");
			
			if (group && attrib && value)
			{
				groups = g_slist_prepend (groups, group);
				attribs = g_slist_prepend (attribs, attrib);
				values = g_slist_prepend (values, value);
			}
			else
			{
				if (group) xmlFree (group);
				if (attrib) xmlFree (attrib);
				if (value) xmlFree (value);
				error = TRUE;
				g_warning ("XML parse error: group, attribute and value should be defined in require");
				break;
			}
			xml_require_node = xml_require_node->next;
		}
		if (error)
		{
			g_slist_foreach (groups, (GFunc)xmlFree, NULL);
			g_slist_foreach (attribs, (GFunc)xmlFree, NULL);
			g_slist_foreach (values, (GFunc)xmlFree, NULL);
			g_slist_free (groups);
			g_slist_free (attribs);
			g_slist_free (values);
			xmlFree (name);
			xmlFree (url);
			break;
		}
		if (g_slist_length (groups) == 0)
		{
			error = TRUE;
			g_warning ("XML Error: No attributes to match given");
			xmlFree (name);
			xmlFree (url);
			break;
		}
		if (g_slist_length (groups) > 5)
		{
			error = TRUE;
			g_warning ("XML Error: Maximum 5 attributes can be given (FIXME)");
			xmlFree (name);
			xmlFree (url);
			break;
		}
		plugin_descs = default_profile_plugin_query_plugins (plugin, groups,
															 attribs, values);
		if (plugin_descs)
		{
			descs_list = g_slist_prepend (descs_list, plugin_descs);
			xmlFree (name);
			xmlFree (url);
		}
		else if (mandatory)
		{
			not_found_names = g_slist_prepend (not_found_names, name);
			not_found_urls = g_slist_prepend (not_found_urls, url);
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
		g_slist_foreach (not_found_names, (GFunc)xmlFree, NULL);
		g_slist_foreach (not_found_urls, (GFunc)xmlFree, NULL);
		g_slist_foreach (descs_list, (GFunc)g_slist_free, NULL);
		g_slist_free (not_found_names);
		g_slist_free (not_found_urls);
		g_slist_free (descs_list);
		return FALSE;
	}
	if (not_found_names)
	{
		/*
		 * FIXME: Present a nice dialog box to promt the user to download
		 * the plugin from corresponding URLs, install them and proceed.
		 */
		GSList *node_name, *node_url;
		
		not_found_names = g_slist_reverse (not_found_names);
		not_found_urls = g_slist_reverse (not_found_urls);
		
		node_name = not_found_names;
		node_url = not_found_urls;
		while (node_name)
		{
			g_warning ("FIXME: Download plugin '%s' from '%s'",
					   (char *)node_name->data, (char*)node_url->data);
			node_name = g_slist_next (node_name);
			node_url = g_slist_next (node_url);
		}
		
		/* FIXME: It should not return like this ... */
		g_slist_foreach (not_found_names, (GFunc)xmlFree, NULL);
		g_slist_foreach (not_found_urls, (GFunc)xmlFree, NULL);
		g_slist_foreach (descs_list, (GFunc)g_slist_free, NULL);
		g_slist_free (not_found_names);
		g_slist_free (not_found_urls);
		g_slist_free (descs_list);
		return FALSE;
	}
	
	if (descs_list)
	{
		/* Now everything okay. Select the plugins and activate them */
		descs_list = g_slist_reverse (descs_list);
		selected_plugins = default_profile_plugin_select_plugins (plugin,
																  descs_list);
		
		/* Activate them */
		default_profile_plugin_activate_plugins (plugin, selected_plugins,
												 splash,
												 default_profile);
		g_slist_free (selected_plugins);
		g_slist_foreach (descs_list, (GFunc)g_slist_free, NULL);
		g_slist_free (descs_list);
	}
	else
	{
		/* The project file is empty */
		anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
								  _("Cannot open: %s\nDoes not look like a valid Anjuta project."),
								  uri);
		return FALSE;
	}
	return TRUE;
}

static gchar*
default_profile_plugin_get_session_dir (DefaultProfilePlugin *plugin)
{
	GnomeVFSURI *vfs_uri;
	const gchar *project_root_uri;
	gchar *session_dir = NULL;
	
	anjuta_shell_get (ANJUTA_PLUGIN (plugin)->shell, "project_root_uri",
					  G_TYPE_STRING, &project_root_uri, NULL);
	
	g_return_val_if_fail (project_root_uri, NULL);
	
	vfs_uri = gnome_vfs_uri_new (project_root_uri);
	if (vfs_uri && gnome_vfs_uri_is_local (vfs_uri))
	{
		gchar *local_dir;
		
		local_dir = gnome_vfs_get_local_path_from_uri (project_root_uri);
		if (local_dir)
		{
			session_dir = g_build_filename (local_dir, ".anjuta", NULL);
		}
		g_free (local_dir);
	}
	if (vfs_uri)
		gnome_vfs_uri_unref (vfs_uri);
	
	return session_dir;
}

static void
default_profile_plugin_close (DefaultProfilePlugin *plugin)
{
	gchar *session_dir;
	
	g_return_if_fail (plugin->project_uri != NULL);
	
	if (!plugin->loaded_plugins)
		return;
	
	/* Save project session */
	session_dir = default_profile_plugin_get_session_dir (plugin);
	if (session_dir)
	{
		plugin->session_by_me = TRUE;
		anjuta_shell_session_save (ANJUTA_PLUGIN (plugin)->shell,
								   session_dir, NULL);
		plugin->session_by_me = FALSE;
		g_free (session_dir);
	}
	
	anjuta_shell_remove_value (ANJUTA_PLUGIN (plugin)->shell,
							   "project_root_uri", NULL);
	
	/* Unload the plugins */
	g_queue_foreach (plugin->loaded_plugins,
					 (GFunc) on_queue_foreach_disconnect,
					 plugin);
	
	/* Unload the plugin in reverse order */
	g_queue_reverse (plugin->loaded_plugins);
	
	/* Unload them */
	g_queue_foreach (plugin->loaded_plugins,
					 (GFunc) on_queue_foreach_unload,
					 plugin);
	
	g_queue_free (plugin->loaded_plugins);
	plugin->loaded_plugins = NULL;
	g_free (plugin->project_uri);
	plugin->project_uri = NULL;
	update_ui (plugin);
}

static void
iprofile_load (IAnjutaProfile *profile, ESplash *splash, GError **err)
{
	DefaultProfilePlugin *plugin;
	
	plugin = (DefaultProfilePlugin*)ANJUTA_PLUGIN (profile);
	default_profile_plugin_load (plugin, DEFAULT_PROFILE,
								 splash, TRUE, err);
}

static void
iprofile_iface_init(IAnjutaProfileIface *iface)
{
	iface->load = iprofile_load;
}

static void
ifile_open (IAnjutaFile *ifile, const gchar* uri,
			GError **e)
{
	GnomeVFSURI *vfs_uri;
	gchar *dirname, *vfs_dir, *session_dir;
	DefaultProfilePlugin *plugin;
	GValue *value;
	
	plugin = (DefaultProfilePlugin*)G_OBJECT (ifile);
	
	if (!default_profile_plugin_load (plugin, uri, NULL, FALSE, e))
		return; /* FIXME: Propagate error */
	
	/* Set project uri */
	vfs_uri = gnome_vfs_uri_new (uri);
	dirname = gnome_vfs_uri_extract_dirname (vfs_uri);
	gnome_vfs_uri_unref (vfs_uri);
	vfs_dir = gnome_vfs_get_uri_from_local_path (dirname);
	g_free (dirname);

	value = g_new0 (GValue, 1);
	g_value_init (value, G_TYPE_STRING);
	g_value_take_string (value, vfs_dir);
	
	g_free (plugin->project_uri);
	plugin->project_uri = g_strdup (uri);
	
	anjuta_shell_add_value (ANJUTA_PLUGIN(plugin)->shell,
							"project_root_uri",
							value, NULL);
	update_ui (plugin);
	
	/* Load Project session */
	session_dir = default_profile_plugin_get_session_dir (plugin);
	if (session_dir)
	{
		/*
		 * If there is a session load already in progress (that is this
		 * project is being opened in session restoration), our session
		 * load would be ignored. Good thing.
		 */
		plugin->session_by_me = TRUE;
		anjuta_shell_session_load (ANJUTA_PLUGIN (plugin)->shell,
								   session_dir, NULL);
		plugin->session_by_me = FALSE;
		g_free (session_dir);
	}
}

static gchar*
ifile_get_uri (IAnjutaFile *ifile, GError **e)
{
	DefaultProfilePlugin *plugin;
	
	plugin = (DefaultProfilePlugin*)G_OBJECT (ifile);
	if (plugin->project_uri)
		return g_strdup (plugin->project_uri);
	else
		return NULL;
}

static void
ifile_iface_init(IAnjutaFileIface *iface)
{
	iface->open = ifile_open;
	iface->get_uri = ifile_get_uri;
}

ANJUTA_PLUGIN_BEGIN (DefaultProfilePlugin, default_profile_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE (iprofile, IANJUTA_TYPE_PROFILE);
ANJUTA_PLUGIN_ADD_INTERFACE (ifile, IANJUTA_TYPE_FILE);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (DefaultProfilePlugin, default_profile_plugin);
