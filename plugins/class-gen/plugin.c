/*
 *  Copyright (C) 2005 Massimo Cora'
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <gtk/gtkactiongroup.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnome/gnome-i18n.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-wizard.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>


#include "plugin.h"
#include "class_gen.h"

#define ICON_FILE "class_logo.xpm"

static gpointer parent_class;

static void dispose (GObject *obj);
static void finalize (GObject *obj);
static void class_gen_plugin_instance_init (GObject *obj);
static void class_gen_plugin_class_init (GObjectClass *klass);


static void
project_root_added (AnjutaPlugin *plugin, const gchar *name,
					const GValue *value, gpointer user_data)
{
	AnjutaClassGenPlugin *cg_plugin;
	const gchar *root_uri;

	cg_plugin = (AnjutaClassGenPlugin*) plugin;
	root_uri = g_value_get_string (value);
	
	if (root_uri)
	{
		gchar *root_dir = gnome_vfs_get_local_path_from_uri (root_uri);
		if (root_dir)
		{	
			cg_plugin->top_dir = g_strdup(root_dir);
		}
		else
			cg_plugin->top_dir = NULL;
		g_free (root_dir);
	}
	else
		cg_plugin->top_dir = NULL;
}

static void
project_root_removed (AnjutaPlugin *plugin, const gchar *name,
					  gpointer user_data)
{
	AnjutaClassGenPlugin *cg_plugin;
	cg_plugin = (AnjutaClassGenPlugin*) plugin;
	
	if (cg_plugin->top_dir)
		g_free(cg_plugin->top_dir);
	cg_plugin->top_dir = NULL;
}


static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	AnjutaClassGenPlugin *cg_plugin;
	
	DEBUG_PRINT ("AnjutaClassGenPlugin: Activating ClassGen plugin...");
	return TRUE;
	cg_plugin = (AnjutaClassGenPlugin *)plugin;
	cg_plugin->prefs = anjuta_shell_get_preferences (plugin->shell, NULL);
	
	g_return_val_if_fail (cg_plugin->prefs != NULL, FALSE);
	
	cg_plugin->top_dir = NULL;
	
	/* set up project directory watch */
	cg_plugin->root_watch_id = anjuta_plugin_add_watch (plugin,
									"project_root_uri",
									project_root_added,
									project_root_removed, NULL);
	return TRUE;
}


static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	AnjutaClassGenPlugin *cg_plugin;
	cg_plugin = (AnjutaClassGenPlugin *) plugin;
	DEBUG_PRINT ("AnjutaClassGenPlugin: Dectivating ClassGen plugin ...");
	
	/* Remove watches */
	anjuta_plugin_remove_watch (plugin, cg_plugin->root_watch_id, TRUE);
	
	return TRUE;
}



static void
class_gen_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
	klass->finalize = finalize;
}


static void
class_gen_plugin_instance_init (GObject *obj)
{
}


static void
iwizard_activate (IAnjutaWizard *wiz, GError **err)
{
	/* IAnjutaProjectManager *pm; */
	AnjutaClassGenPlugin *cg_plugin;
	
	cg_plugin = (AnjutaClassGenPlugin*)wiz;
	
	/* check if the top_dir is setted, i.e. a project is loaded */
	if (cg_plugin->top_dir != NULL ) 
		on_classgen_new ((AnjutaClassGenPlugin*)wiz);
	else
		anjuta_util_dialog_error (NULL,
								  _("A project must be open before creating a class"));
}

static void
iwizard_iface_init (IAnjutaWizardIface *iface)
{
	iface->activate = iwizard_activate;
}

static void
dispose (GObject *obj)
{
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}


static void
finalize (GObject *obj)
{
	AnjutaClassGenPlugin *cg_plugin;
	cg_plugin = (AnjutaClassGenPlugin *) obj;

	if (cg_plugin->top_dir)
		g_free (cg_plugin->top_dir);
	
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

ANJUTA_PLUGIN_BEGIN (AnjutaClassGenPlugin, class_gen_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE(iwizard, IANJUTA_TYPE_WIZARD);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (AnjutaClassGenPlugin, class_gen_plugin);
