/*	 plugin.c	(C) 2005 Massimo Cora'
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <config.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-plugin.h>

#include "plugin.h"
#include "patch-plugin.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-patch.ui"

static gpointer parent_class;


static gboolean patch_plugin_activate (AnjutaPlugin *plugin);
static gboolean patch_plugin_deactivate (AnjutaPlugin *plugin);
/*
static void dispose (GObject *obj);
static void finalize (GObject *obj);
*/
static void patch_plugin_instance_init (GObject *obj);
static void patch_plugin_class_init (GObjectClass *klass);
static void on_patch_action_activate (GtkAction *action, PatchPlugin *plugin);

static void
on_patch_action_activate (GtkAction *action, PatchPlugin *plugin) {
	
	patch_show_gui( plugin );
	
}


static GtkActionEntry actions_tools[] = {	
	{
		"ActionMenuTools",	/* Action name */
		NULL,			/* Stock icon, if any */
		N_("_Tools"),		/* Display label */
		NULL,			/* Short-cut */
		NULL,			/* Tooltip */
		NULL			/* Callback */
	},
	{
		"ActionToolsPatch",		/* Action name */
		"patch-plugin-icon",							/* Stock icon, if any */
		N_("_Patch..."), 				/* Display label */
		NULL, 						/* short-cut */
		NULL, 						/* Tooltip */
		G_CALLBACK (on_patch_action_activate) /* action callback */
	}	
};



static gboolean
patch_plugin_activate (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	PatchPlugin *p_plugin;
	
	DEBUG_PRINT ("%s", "PatchPlugin: Activating Patch plugin...");
	
	p_plugin = ANJUTA_PLUGIN_PATCH (plugin);

	p_plugin->launcher = anjuta_launcher_new ();
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	/* Register icon */
	BEGIN_REGISTER_ICON (plugin);
	REGISTER_ICON (ICON_FILE, "patch-plugin-icon");
	END_REGISTER_ICON;


	/* Add all our actions */
	anjuta_ui_add_action_group_entries (ui, "ActionMenuTools",
										_("Patch files/directories"),
										actions_tools,
										G_N_ELEMENTS (actions_tools),
										GETTEXT_PACKAGE, TRUE, p_plugin);

	p_plugin->uiid = anjuta_ui_merge (ui, UI_FILE);
	
	return TRUE;
}


static gboolean
patch_plugin_deactivate (AnjutaPlugin *plugin)
{
	AnjutaUI *ui = anjuta_shell_get_ui (plugin->shell, NULL);
	DEBUG_PRINT ("%s", "AnjutaPatchPlugin: Dectivating Patch plugin ...");

	anjuta_ui_unmerge (ui, ANJUTA_PLUGIN_PATCH (plugin)->uiid);

	/* FIXME: should launcher be unreferenced? */
	
	return TRUE;
}

static void
patch_plugin_finalize (GObject *obj)
{
	/*/
	PatchPlugin *p_plugin;
	p_plugin = ANJUTA_PLUGIN_PATCH (obj);
	/*/
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}


static void
patch_plugin_dispose (GObject *obj)
{
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}


static void
patch_plugin_instance_init (GObject *obj)
{
	PatchPlugin *plugin = ANJUTA_PLUGIN_PATCH (obj);	
	plugin->uiid = 0;
	plugin->launcher = NULL;
}


static void
patch_plugin_class_init (GObjectClass *klass)
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = patch_plugin_activate;
	plugin_class->deactivate = patch_plugin_deactivate;
	klass->dispose = patch_plugin_dispose;
	klass->finalize = patch_plugin_finalize;
}

ANJUTA_PLUGIN_BOILERPLATE (PatchPlugin, patch_plugin);
ANJUTA_SIMPLE_PLUGIN (PatchPlugin, patch_plugin);
