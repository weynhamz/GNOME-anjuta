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
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-stock.h>
#include <libanjuta/interfaces/ianjuta-buildable.h>

#include "build-basic-autotools.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-build-basic-autotools-plugin.ui"

gpointer parent_class;

static void
build_build_project (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
}

static void
build_install_project (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
}

static void
build_clean_project (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
}

static void
build_configure_project (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
}

static void
build_autogen_project (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
}

static void
build_distribution_project (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
}

static void
build_build_module (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
}

static void
build_install_module (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
}

static void
build_clean_module (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
}

static void
build_compile_file (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
}

static GtkActionEntry build_actions[] = 
{
	{
		"ActionMenuBuild", NULL,
		N_("_Build"), NULL, NULL, NULL
	},
	{
		"ActionBuildBuildProject", ANJUTA_STOCK_BUILD,
		N_("_Build project"), "<shift>F11",
		N_("Build whole project"),
		G_CALLBACK (build_build_project)
	},
	{
		"ActionBuildInstallProject", NULL,
		N_("_Install project"), NULL,
		N_("Install whole project"),
		G_CALLBACK (build_install_project)
	},
	{
		"ActionBuildCleanProject", NULL,
		N_("_Clean project"), NULL,
		N_("Clean whole project"),
		G_CALLBACK (build_clean_project)
	},
	{
		"ActionBuildConfigure", NULL,
		N_("C_onfigure ..."), NULL,
		N_("Configure project"),
		G_CALLBACK (build_configure_project)
	},
	{
		"ActionBuildAutogen", NULL,
		N_("_Auto generate ..."), NULL,
		N_("Autogenrate project files"),
		G_CALLBACK (build_autogen_project)
	},
	{
		"ActionBuildDistribution", NULL,
		N_("Build _distribution"), NULL,
		N_("Build project distribution"),
		G_CALLBACK (build_distribution_project)
	},
	{
		"ActionBuildBuildModule", ANJUTA_STOCK_BUILD,
		N_("_Build module"), "F11",
		N_("Build module"),
		G_CALLBACK (build_build_module)
	},
	{
		"ActionBuildInstallModule", NULL,
		N_("_Install module"), NULL,
		N_("Install module"),
		G_CALLBACK (build_install_module)
	},
	{
		"ActionBuildCleanModule", NULL,
		N_("_Clean module"), NULL,
		N_("Clean module"),
		G_CALLBACK (build_clean_module)
	},
	{
		"ActionBuildCompileFile", ANJUTA_STOCK_COMPILE,
		N_("Co_mpile file"), "F9",
		N_("Compile current editor file"),
		G_CALLBACK (build_compile_file)
	}
};

/* File manager context menu */
static void
fm_compile (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
}

static void
fm_build (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
}

static void
fm_install (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
}
static void
fm_clean (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
}

static GtkActionEntry fm_actions[] = 
{
	{
		"ActionPopupBuild", NULL,
		N_("_Build"), NULL, NULL, NULL
	},
	{
		"ActionPopupBuildCompile", NULL,
		N_("_Compile"), NULL,
		N_("Complie file"),
		G_CALLBACK (fm_compile)
	},
	{
		"ActionPopupBuildBuild", NULL,
		N_("_Build"), NULL,
		N_("Build module"),
		G_CALLBACK (fm_build)
	},
	{
		"ActionPopupBuildInstall", NULL,
		N_("_Install"), NULL,
		N_("Install module"),
		G_CALLBACK (fm_install)
	},
	{
		"ActionPopupBuildClean", NULL,
		N_("_Clean"), NULL,
		N_("Clean module"),
		G_CALLBACK (fm_clean)
	}
};

/* Build popup submenu actions */
static const gchar * fm_build_popup = " \
<ui>\
<popup name=\"PopupFileManager\">\
	<placeholder name=\"PlaceholderPopupFileBuild\">\
		<menu name=\"PopupFileBuild\" action=\"ActionPopupBuild\">\
			<menuitem name=\"PopupBuildCompile\" action=\"ActionPopupBuildCompile\"/>\
			<menuitem name=\"PopupBuildBuild\" action=\"ActionPopupBuildBuild\"/>\
			<menuitem name=\"PopupBuildInstall\" action=\"ActionPopupBuildInstall\"/>\
			<menuitem name=\"PopupBuildClean\" action=\"ActionPopupBuildClean\"/>\
		</menu>\
	</placeholder>\
</popup>\
</ui>\
";

static void
value_added_fm_current_filename (AnjutaPlugin *plugin, const char *name,
								 const GValue *value, gpointer data)
{
	AnjutaUI *ui;
	GtkAction *compile_action;
	const gchar *filename;
	gchar *dirname;
	gchar *makefile;
	gboolean is_dir, makefile_exists;
	
	filename = g_value_get_string (value);
	g_return_if_fail (name != NULL);
	
	g_message ("Current file: %s", filename);
	BasicAutotoolsPlugin *ba_plugin = (BasicAutotoolsPlugin*)plugin;
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	is_dir = g_file_test (filename, G_FILE_TEST_IS_DIR);
	if (is_dir)
		dirname = g_strdup (filename);
	else
		dirname = g_path_get_dirname (filename);

	makefile_exists = TRUE;	
	makefile = g_strconcat (dirname, "/Makefile", NULL);
	if (!g_file_test (makefile, G_FILE_TEST_EXISTS))
	{
		g_free (makefile);
		makefile = g_strconcat (dirname, "/makefile", NULL);
		if (!g_file_test (makefile, G_FILE_TEST_EXISTS))
		{
			g_free (makefile);
			makefile = g_strconcat (dirname, "/MAKEFILE", NULL);
			if (!g_file_test (makefile, G_FILE_TEST_EXISTS))
			{
				makefile_exists = FALSE;
			}
		}
	}
	g_free (makefile);
	g_free (dirname);
	
	if (!makefile_exists)
		return;
	g_message ("Makefile found");
	ba_plugin->fm_merge_id =
		gtk_ui_manager_add_ui_from_string (GTK_UI_MANAGER (ui), fm_build_popup,
										   -1, NULL);
	compile_action =
		gtk_action_group_get_action (ba_plugin->fm_popup_action_group,
									 "ActionPopupBuildCompile");
	if (is_dir)
	{
		g_object_set (G_OBJECT (compile_action), "sensitive", FALSE);
	}
	else
		g_object_set (G_OBJECT (compile_action), "sensitive", TRUE);
}

static void
value_removed_fm_current_filename (AnjutaPlugin *plugin,
								   const char *name, gpointer data)
{
	AnjutaUI *ui;
	BasicAutotoolsPlugin *ba_plugin = (BasicAutotoolsPlugin*)plugin;
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	if (ba_plugin->fm_merge_id)
		gtk_ui_manager_remove_ui (GTK_UI_MANAGER (ui),
								  ba_plugin->fm_merge_id);
	ba_plugin->fm_merge_id = 0;
}

static void
value_added_project_root_directory (AnjutaPlugin *plugin, const char *name,
								    const GValue *value, gpointer data)
{
#if 0
	AnjutaUI *ui;
	GtkAction *compile_action;
	const gchar *filename;
	gchar *dirname;
	gchar *makefile;
	gboolean is_dir, makefile_exists;
	
	filename = g_value_get_string (value);
	g_return_if_fail (name != NULL);
	
	g_message ("Current file: %s", filename);
	BasicAutotoolsPlugin *ba_plugin = (BasicAutotoolsPlugin*)plugin;
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	is_dir = g_file_test (filename, G_FILE_TEST_IS_DIR);
	if (is_dir)
		dirname = g_strdup (filename);
	else
		dirname = g_path_get_dirname (filename);

	makefile_exists = TRUE;	
	makefile = g_strconcat (dirname, "/Makefile", NULL);
	if (!g_file_test (makefile, G_FILE_TEST_EXISTS))
	{
		g_free (makefile);
		makefile = g_strconcat (dirname, "/makefile", NULL);
		if (!g_file_test (makefile, G_FILE_TEST_EXISTS))
		{
			g_free (makefile);
			makefile = g_strconcat (dirname, "/MAKEFILE", NULL);
			if (!g_file_test (makefile, G_FILE_TEST_EXISTS))
			{
				makefile_exists = FALSE;
			}
		}
	}
	g_free (makefile);
	g_free (dirname);
	
	if (!makefile_exists)
		return;
	g_message ("Makefile found");
	ba_plugin->fm_merge_id =
		gtk_ui_manager_add_ui_from_string (GTK_UI_MANAGER (ui), fm_build_popup,
										   -1, NULL);
	compile_action =
		gtk_action_group_get_action (ba_plugin->fm_popup_action_group,
									 "ActionPopupBuildCompile");
	if (is_dir)
	{
		g_object_set (G_OBJECT (compile_action), "sensitive", FALSE);
	}
	else
		g_object_set (G_OBJECT (compile_action), "sensitive", TRUE);
#endif
}

static void
value_removed_project_root_directory (AnjutaPlugin *plugin,
									  const char *name, gpointer data)
{
#if 0
	AnjutaUI *ui;
	BasicAutotoolsPlugin *ba_plugin = (BasicAutotoolsPlugin*)plugin;
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	if (ba_plugin->fm_merge_id)
		gtk_ui_manager_remove_ui (GTK_UI_MANAGER (ui),
								  ba_plugin->fm_merge_id);
	ba_plugin->fm_merge_id = 0;
#endif
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	BasicAutotoolsPlugin *ba_plugin = (BasicAutotoolsPlugin*)plugin;
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	/* register actions */
	ba_plugin->fm_popup_action_group = 
		anjuta_ui_add_action_group_entries (ui,
											"ActionGroupPopupBuild",
											_("Popup build commands"),
											fm_actions,
											sizeof(fm_actions)/sizeof(GtkActionEntry),
											plugin);
	ba_plugin->build_action_group = 
		anjuta_ui_add_action_group_entries (ui,
											"ActionGroupBuild",
											_("Build commands"),
											build_actions,
											sizeof(build_actions)/sizeof(GtkActionEntry),
											plugin);
	ba_plugin->build_merge_id = anjuta_ui_merge (ui, UI_FILE);

	/* Register value watches */
	ba_plugin->fm_watch_id = 
		anjuta_plugin_add_watch (plugin, "file_manager_current_filename",
								 value_added_fm_current_filename,
								 value_removed_fm_current_filename, NULL);
	ba_plugin->project_watch_id = 
		anjuta_plugin_add_watch (plugin, "project_root_directory",
								 value_added_project_root_directory,
								 value_removed_project_root_directory, NULL);
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	BasicAutotoolsPlugin *ba_plugin = (BasicAutotoolsPlugin*)plugin;
	anjuta_plugin_remove_watch (plugin, ba_plugin->fm_watch_id, TRUE);
	anjuta_plugin_remove_watch (plugin, ba_plugin->project_watch_id, TRUE);
	anjuta_ui_unmerge (ui, ba_plugin->build_merge_id);
	anjuta_ui_remove_action_group (ui, ba_plugin->build_action_group);
	anjuta_ui_remove_action_group (ui, ba_plugin->fm_popup_action_group);
	return TRUE;
}

static void
dispose (GObject *obj)
{
}

static void
basic_autotools_plugin_instance_init (GObject *obj)
{
	BasicAutotoolsPlugin *ba_plugin = (BasicAutotoolsPlugin*) obj;
	ba_plugin->fm_merge_id = 0;
}

static void
basic_autotools_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
}

static void
ibuildable_compile (IAnjutaBuildable *manager, const gchar * filename,
					GError **err)
{
}

static void
ibuildable_build (IAnjutaBuildable *manager, GError **err)
{
}

static void
ibuildable_clean (IAnjutaBuildable *manager, GError **err)
{
}

static void
ibuildable_install (IAnjutaBuildable *manager, GError **err)
{
}

static void
ibuildable_iface_init (IAnjutaBuildableIface *iface)
{
	iface->compile = ibuildable_compile;
	iface->build = ibuildable_build;
	iface->clean = ibuildable_clean;
	iface->install = ibuildable_install;
}

ANJUTA_PLUGIN_BEGIN (BasicAutotoolsPlugin, basic_autotools_plugin);
ANJUTA_INTERFACE (ibuildable, IANJUTA_TYPE_BUILDABLE);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (BasicAutotoolsPlugin, basic_autotools_plugin);
