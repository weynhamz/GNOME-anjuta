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
#include <libanjuta/anjuta-debug.h>
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
#define ANJUTA_MINI_SPLASH PACKAGE_PIXMAPS_DIR"/anjuta_splash_mini.png"

static gpointer parent_class;

static void default_profile_plugin_close (DefaultProfilePlugin *plugin);
static void default_profile_plugin_load_default (DefaultProfilePlugin *plugin,
												 GError **err);

static void
default_profile_plugin_write_to_file (DefaultProfilePlugin *plugin,
									  const gchar *dir,
									  GSList *plugins_to_exclude)
{
	gchar *profile_name, *profile_file;
	GnomeVFSHandle* vfs_write;
	GnomeVFSResult result;
	GnomeVFSFileSize nchars;
	GSList *active_plugins, *node;
	GHashTable *base_plugins_hash;
	GString *str;

	profile_name = g_path_get_basename (plugin->default_profile);
	profile_file = g_build_filename (dir, profile_name, NULL);
	g_free (profile_name);
	
	/* Prepare the write data */
	base_plugins_hash = g_hash_table_new (g_direct_hash, g_direct_equal);
	node = plugins_to_exclude;
	while (node)
	{
		g_hash_table_insert (base_plugins_hash, node->data, node->data);
		node = g_slist_next (node);
	}
	active_plugins = anjuta_plugins_get_active_plugins (ANJUTA_PLUGIN (plugin)->shell);
	str = g_string_new ("<?xml version=\"1.0\"?>\n<anjuta>\n");
	node = active_plugins;
	while (node)
	{
		if (!g_hash_table_lookup (base_plugins_hash, node->data))
		{
			GObject *plugin_object;
			AnjutaPluginDescription *desc;
			gchar *name = NULL, *plugin_id = NULL;
			
			desc = (AnjutaPluginDescription *)node->data;
			anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
													  "Name", &name);
			if (!name)
				name = g_strdup ("Unknown");
			
			if (anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
													  "Location", &plugin_id))
			{
				plugin_object = anjuta_plugins_get_plugin_by_id (ANJUTA_PLUGIN (plugin)->shell,
																 plugin_id);
				if (plugin_object != G_OBJECT (plugin))
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
			}
			g_free (name);
		}
		node = g_slist_next (node);
	}
	g_string_append (str, "</anjuta>\n");
	
	result = gnome_vfs_create (&vfs_write, profile_file, GNOME_VFS_OPEN_WRITE,
							   FALSE, 0664);
	
 	if (result == GNOME_VFS_OK)
	{
		gnome_vfs_write(vfs_write, str->str, str->len, &nchars);
		gnome_vfs_close(vfs_write);
	}
	g_hash_table_destroy (base_plugins_hash);
	g_slist_free (active_plugins);
	g_string_free (str, TRUE);
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
on_session_save (AnjutaShell *shell, AnjutaSessionPhase phase,
				 AnjutaSession *session, DefaultProfilePlugin *plugin)
{
	GList *files;
	const gchar *dir;
	
	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;
	
	/* Save currently active plugins */
	dir = anjuta_session_get_session_directory (session);
	if (plugin->session_by_me == TRUE)
		/* Project session */
		default_profile_plugin_write_to_file (plugin, dir,
											  plugin->project_plugins);
	else
		/* External session */
		default_profile_plugin_write_to_file (plugin, dir,
											  plugin->system_plugins);
	
	/*
	 * When a project session is being saved (session_by_me == TRUE),
	 * we should not save the current project uri, because project 
	 * sessions are loaded when the project has already been loaded.
	 */
	if (plugin->project_uri && !plugin->session_by_me)
	{
		gchar *session_dir;
		
		files = anjuta_session_get_string_list (session,
												"File Loader",
												"Files");
		files = g_list_append (files, g_strdup (plugin->project_uri));
		anjuta_session_set_string_list (session, "File Loader",
										"Files", files);
		g_list_foreach (files, (GFunc)g_free, NULL);
		g_list_free (files);
		
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

static gboolean
on_close_project_idle (gpointer plugin)
{
	default_profile_plugin_close ((DefaultProfilePlugin *)plugin);
	default_profile_plugin_load_default ((DefaultProfilePlugin *)plugin,
										 NULL);
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
											GETTEXT_PACKAGE, plugin);
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
	if (plugin->default_profile)
	{
		g_free (plugin->default_profile);
		plugin->default_profile = NULL;
	}
	if (plugin->project_uri)
	{
		g_free (plugin->project_uri);
		plugin->project_uri = NULL;
	}
	if (plugin->system_plugins)
	{
		g_slist_free (plugin->system_plugins);
		plugin->system_plugins = NULL;
	}
	if (plugin->project_plugins)
	{
		g_slist_free (plugin->project_plugins);
		plugin->project_plugins = NULL;
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
	plugin->default_profile = NULL;
	plugin->system_plugins = NULL;
	plugin->project_plugins = NULL;
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
										 GSList *selected_plugins)
{
	AnjutaStatus *status;
	GdkPixbuf *icon_pixbuf;
	GSList *node;
	
	/* Freeze shell operations */
	anjuta_shell_freeze (ANJUTA_PLUGIN (plugin)->shell, NULL);
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell, NULL);
	
	/* First of all close existing project profile */
	if (plugin->project_uri)
	{
		default_profile_plugin_close (plugin);
	}
	if (selected_plugins)
		anjuta_status_progress_add_ticks (status,
										  g_slist_length (selected_plugins));
	
	node = selected_plugins;
	while (node)
	{
		AnjutaPluginDescription *d;
		gchar *plugin_id;
		gchar *icon_filename, *label;
		gchar *icon_path = NULL;
		
		d = node->data;
		
		icon_pixbuf = NULL;
		label = NULL;
		if (anjuta_plugin_description_get_string (d, "Anjuta Plugin",
												  "Icon",
											  &icon_filename))
		{
			gchar *title, *description;
			anjuta_plugin_description_get_string (d, "Anjuta Plugin",
												  "Name",
												  &title);
			anjuta_plugin_description_get_string (d, "Anjuta Plugin",
												  "Description",
												  &description);
			icon_path = g_strconcat (PACKAGE_PIXMAPS_DIR"/",
									 icon_filename, NULL);
			// DEBUG_PRINT ("Icon: %s", icon_path);
			label = g_strconcat (_("Loaded: "), title, _("..."), NULL);
			icon_pixbuf = gdk_pixbuf_new_from_file (icon_path, NULL);
			if (!icon_pixbuf)
				g_warning ("Plugin does not define Icon: No such file %s",
						   icon_path);
			g_free (icon_path);
			g_free (icon_filename);
		}
		
		if (anjuta_plugin_description_get_string (d, "Anjuta Plugin",
												  "Location", &plugin_id))
		{
			GObject *plugin_obj;
			
			plugin_obj =
				anjuta_plugins_get_plugin_by_id (ANJUTA_PLUGIN (plugin)->shell,
												 plugin_id);
			g_free (plugin_id);
		}
		anjuta_status_progress_tick (status, icon_pixbuf, label);
		g_free (label);
		if (icon_pixbuf)
			g_object_unref (icon_pixbuf);
		
		node = g_slist_next (node);
	}
	
	/* Thaw shell operations */
	anjuta_shell_thaw (ANJUTA_PLUGIN (plugin)->shell, NULL);
}

static GSList*
default_profile_plugin_read (DefaultProfilePlugin *plugin,
							 const gchar *xml_uri)
{
	xmlDocPtr xml_doc;
	xmlNodePtr xml_root, xml_node;
	GnomeVFSHandle *handle;
	GnomeVFSFileInfo info;
	GnomeVFSResult result;
	GSList *descs_list, *selected_plugins, *not_found_names, *not_found_urls;
	int perm, read;
	gboolean error = FALSE;
	gchar *read_buf = NULL;
	
	result = gnome_vfs_get_file_info(xml_uri, &info,
									 GNOME_VFS_FILE_INFO_DEFAULT |
									 GNOME_VFS_FILE_INFO_GET_ACCESS_RIGHTS);

	/* If I got the info to check it out */
	if(result != GNOME_VFS_OK )
	{
		anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
								  _("Cannot open: %s\n&s"),
								  xml_uri, gnome_vfs_result_to_string(result));
		return NULL;
	}
	
	/* FIXME: Fix this bit masking */
	perm = (info.permissions-(int)(info.permissions/65536)*65536);
	read = (int)(perm/256);

	if(read == 0)
	{
		anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
								  _("No read permission for: %s"),
								  xml_uri);
		return NULL;
	}
	if((result = gnome_vfs_open (&handle, xml_uri,
								 GNOME_VFS_OPEN_READ)) != GNOME_VFS_OK)
	{
		anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
								  _("Cannot open: %s\n&s"),
								  xml_uri, gnome_vfs_result_to_string(result));
		return NULL;
	}
	read_buf = g_new0(char, info.size + 1);
	
	result = gnome_vfs_read (handle, read_buf, info.size, NULL);
	if(!(result == GNOME_VFS_OK || result == GNOME_VFS_ERROR_EOF))
	{
		g_free (read_buf);
		anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
								  _("Cannot open: %s\n&s"),
								  xml_uri, gnome_vfs_result_to_string(result));
		return NULL;
	}
	gnome_vfs_close (handle);
	xml_doc = xmlParseMemory (read_buf, info.size);
	g_free(read_buf);
	if(xml_doc == NULL)
	{
		anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
								  _("Cannot open: %s\nXML parse error."),
								  xml_uri);
		return NULL;
	}
	
	xml_root = xmlDocGetRootElement(xml_doc);
	if(xml_root == NULL)
	{
		anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
								  _("Cannot open: %s\nXML parse error."),
								  xml_uri);
		/* Free xml doc */
		return NULL;
	}
	
	if(!xml_root->name ||
	   !xmlStrEqual(xml_root->name, (const xmlChar *)"anjuta"))
	{
		anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
								  _("Cannot open: %s\nDoes not look like a valid Anjuta project."),
								  xml_uri);
		return NULL;
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
		return NULL;
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
		return NULL;
	}
	if (descs_list)
	{
		/* Now everything okay. Select the plugins */
		descs_list = g_slist_reverse (descs_list);
		selected_plugins = default_profile_plugin_select_plugins (plugin,
																  descs_list);
		g_slist_foreach (descs_list, (GFunc)g_slist_free, NULL);
		g_slist_free (descs_list);
		return selected_plugins;
	}
	return NULL;
}

static void
default_profile_plugin_load (DefaultProfilePlugin *plugin,
							 GSList *selected_plugins, GError **e)
{
	GSList *active_plugins, *node, *plugins_to_activate;
	GHashTable *active_plugins_hash, *plugins_to_activate_hash;
	
	/* Prepare plugins to activate */
	plugins_to_activate_hash = g_hash_table_new (g_direct_hash, g_direct_equal);
	node = selected_plugins;
	while (node)
	{
		g_hash_table_insert (plugins_to_activate_hash, node->data, node->data);
		node = g_slist_next (node);
	}
	
	/* Deactivate plugins that are already active, but are not requested to
	 * to be active */
	active_plugins = anjuta_plugins_get_active_plugins (ANJUTA_PLUGIN (plugin)->shell);
	active_plugins_hash = g_hash_table_new (g_direct_hash, g_direct_equal);
	node = active_plugins;
	while (node)
	{
		if (!g_hash_table_lookup (plugins_to_activate_hash, node->data))
		{
			AnjutaPluginDescription *desc;
			GObject *plugin_object;
			gchar *plugin_id = NULL;
			
			desc = (AnjutaPluginDescription *)node->data;
			anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
												  "Location", &plugin_id);
			g_assert (plugin_id != NULL);
			
			plugin_object = anjuta_plugins_get_plugin_by_id (ANJUTA_PLUGIN (plugin)->shell,
															 plugin_id);
			g_assert (plugin_object != NULL);
			
			/* Refrain from unloading self. bah */
			if (G_OBJECT (plugin) != plugin_object)
			{
				anjuta_plugins_unload_plugin (ANJUTA_PLUGIN (plugin)->shell,
											  plugin_object);
			}
			g_free (plugin_id);
		}
		g_hash_table_insert (active_plugins_hash, node->data, node->data);
		node = g_slist_next (node);
	}
	
	/* Prepare the plugins to activate */
	plugins_to_activate = NULL;
	node = selected_plugins;
	while (node)
	{
		if (!g_hash_table_lookup (active_plugins_hash, node->data))
			plugins_to_activate = g_slist_prepend (plugins_to_activate,
												   node->data);
		node = g_slist_next (node);
	}
	
	/* Now activate the plugins */
	if (plugins_to_activate)
	{
		/* Activate them */
		plugins_to_activate = g_slist_reverse (plugins_to_activate);
		default_profile_plugin_activate_plugins (plugin, plugins_to_activate);
	}
	g_slist_free (plugins_to_activate);
	g_slist_free (active_plugins);
	g_hash_table_destroy (plugins_to_activate_hash);
	g_hash_table_destroy (active_plugins_hash);
}

static void
default_profile_plugin_load_default (DefaultProfilePlugin *plugin,
									 GError **err)
{
	AnjutaStatus *status;
	gchar *session_plugins, *profile_name;
	GSList *selected_plugins, *temp_plugins;
	
	g_return_if_fail (plugin->default_profile != NULL);
	
	/* Load system default plugins */
	selected_plugins = default_profile_plugin_read (plugin,
													plugin->default_profile);
	status = anjuta_shell_get_status (ANJUTA_PLUGIN(plugin)->shell, NULL);
	anjuta_status_progress_add_ticks (status, 1);
	
	/* Save the list for later comparision */
	if (plugin->system_plugins) {
		g_slist_free (plugin->system_plugins);
		plugin->system_plugins = NULL;
	}
	plugin->system_plugins = g_slist_copy (selected_plugins);
	
	/* Load user session plugins */
	profile_name = g_path_get_basename (plugin->default_profile);
	session_plugins = g_build_filename (g_get_home_dir(), ".anjuta",
										profile_name, NULL);
	g_free (profile_name);
	if (g_file_test (session_plugins, G_FILE_TEST_EXISTS))
	{
		temp_plugins = default_profile_plugin_read (plugin, session_plugins);
		selected_plugins = g_slist_concat (selected_plugins, temp_plugins);
	}
	default_profile_plugin_load (plugin, selected_plugins, err);
	anjuta_status_progress_tick (status, NULL, _("Loaded default profile..."));
	
	g_slist_free (selected_plugins);
	g_free (session_plugins);
}

static void
default_profile_plugin_close (DefaultProfilePlugin *plugin)
{
	gchar *session_dir;
	
	g_return_if_fail (plugin->project_uri != NULL);
	
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
	
	g_free (plugin->project_uri);
	plugin->project_uri = NULL;
	update_ui (plugin);
}

static void
iprofile_load (IAnjutaProfile *profile, GError **err)
{
	AnjutaStatus *status;
	DefaultProfilePlugin *plugin;
	
	plugin = (DefaultProfilePlugin*)ANJUTA_PLUGIN (profile);
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (profile)->shell, NULL);
	
	anjuta_status_progress_add_ticks (status, 1);
	if (plugin->default_profile)
	{
		g_free (plugin->default_profile);
		plugin->default_profile = NULL;
	}
	plugin->default_profile = g_strdup (DEFAULT_PROFILE);
	default_profile_plugin_load_default (plugin, err);
	
	anjuta_status_progress_tick (status, NULL, _("Loaded Profile..."));
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
	AnjutaStatus *status;
	GnomeVFSURI *vfs_uri;
	gchar *dirname, *vfs_dir, *session_dir, *session_plugins, *profile_name;
	DefaultProfilePlugin *plugin;
	GValue *value;
	GSList *selected_plugins, *temp_plugins;
	
	plugin = (DefaultProfilePlugin*)G_OBJECT (ifile);
	
	/* Load system default plugins */
	selected_plugins = default_profile_plugin_read (plugin,
													plugin->default_profile);
	
	/* Load project default plugins */
	temp_plugins = default_profile_plugin_read (plugin, uri);
	if (!temp_plugins)
	{
		/* The project file is empty */
		anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
								  _("Cannot open: %s\n"
									"Does not look like a valid Anjuta project."),
								  uri);
		g_slist_free (selected_plugins);
		return; /* FIXME: Propagate error */
	}
	selected_plugins = g_slist_concat (selected_plugins, temp_plugins);
	
	/* Save the list for later comparision */
	if (plugin->project_plugins) {
		g_slist_free (plugin->project_plugins);
		plugin->project_plugins = NULL;
	}
	plugin->project_plugins = g_slist_copy (selected_plugins);
	
	/* Freeze shell */
	anjuta_shell_freeze (ANJUTA_PLUGIN (ifile)->shell, NULL);
	
	/* Load project session plugins */
	vfs_uri = gnome_vfs_uri_new (uri);
	dirname = gnome_vfs_uri_extract_dirname (vfs_uri);
	profile_name = g_path_get_basename (plugin->default_profile);
	gnome_vfs_uri_unref (vfs_uri);
	session_plugins = g_build_filename (dirname, ".anjuta",
										profile_name, NULL);
	g_free (profile_name);
	
	DEBUG_PRINT ("Loading session profile: %s", session_plugins);

	status = anjuta_shell_get_status (ANJUTA_PLUGIN (ifile)->shell, NULL);
	anjuta_status_progress_add_ticks (status, 2);
	
	if (g_file_test (session_plugins, G_FILE_TEST_EXISTS))
	{
		temp_plugins = default_profile_plugin_read (plugin, session_plugins);
		selected_plugins = g_slist_concat (selected_plugins, temp_plugins);
	}
	g_free (session_plugins);
	
	if (!plugin->project_uri)
	{
		/* If there is no project opened, save the currently active
		 * plugins so that they could be restored later when project
		 * is closed.
		 */
		gchar *dir;
		
		dir = g_build_filename (g_get_home_dir(), ".anjuta", NULL);
		default_profile_plugin_write_to_file (plugin, dir,
											  plugin->system_plugins);
		g_free (dir);
	}
	default_profile_plugin_load (plugin, selected_plugins, e);
	g_slist_free (selected_plugins);
	
	/* Set project uri */
	vfs_dir = gnome_vfs_get_uri_from_local_path (dirname);
	g_free (dirname);

	value = g_new0 (GValue, 1);
	g_value_init (value, G_TYPE_STRING);
	g_value_take_string (value, vfs_dir);
	
	g_free (plugin->project_uri);
	plugin->project_uri = g_strdup (uri);
	anjuta_status_progress_tick (status, NULL, _("Loaded Project... Initializing"));
	
	anjuta_shell_add_value (ANJUTA_PLUGIN(plugin)->shell,
							"project_root_uri",
							value, NULL);
	update_ui (plugin);
	
	/* Thaw shell */
	/* FIXME: The shell can not be thawed after the session is loaded,
	 * because the layout is loaded in the session. If shell is not thawed
	 * before that, the widgets on which layout is applied are not present
	 * side the shell (freezing delays the widgets addition).
	 */
	anjuta_shell_thaw (ANJUTA_PLUGIN (ifile)->shell, NULL);
	
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
	anjuta_status_progress_tick (status, NULL, _("Loaded Profile..."));
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
