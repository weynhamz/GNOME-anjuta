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
#include "plugin.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-sample.ui"

gpointer parent_class;

static void
on_sample_action_activate (EggAction *action, gpointer data)
{
	SamplePlugin *plugin = (SamplePlugin *)data;
	/* Do whatever with plugin */
	gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (plugin->widget)),
							GTK_DIALOG_DESTROY_WITH_PARENT,
							GTK_MESSAGE_INFO,
							GTK_BUTTONS_NONE,
							"This is a sample action for sample plugin");
}

static EggActionGroupEntry actions_file[] = {
  { "ActionFileSample", N_("_Sample action"), GTK_STOCK_NEW, NULL,
	N_("Sample action"),
    G_CALLBACK (on_sample_action_activate), NULL },
};

static void
init_user_data (EggActionGroupEntry* actions, gint size, gpointer data)
{
	int i;
	for (i = 0; i < size; i++)
		actions[i].user_data = data;
}

static void
ui_set (AnjutaPlugin *plugin)
{
	g_message ("SamplePlugin: UI set");
}

static void
prefs_set (AnjutaPlugin *plugin)
{
	g_message ("SamplePlugin: Prefs set");
}

static void
shell_set (AnjutaPlugin *plugin)
{
	GtkWidget *wid;
	AnjutaUI *ui;
	SamplePlugin *sample_plugin;
	
	g_message ("SamplePlugin: Shell set. Activating Sample plugin ...");
	sample_plugin = (SamplePlugin*) plugin;
	ui = plugin->ui;
	wid = gtk_label_new ("This is a sample plugin");
	sample_plugin->widget = wid;
	
	/* Add all our editor actions */
	init_user_data (actions_file, G_N_ELEMENTS (actions_file), sample_plugin);
	anjuta_ui_add_action_group_entries (ui, "ActionGroupSampleFile",
										_("Sample file operations"),
										actions_file,
										G_N_ELEMENTS (actions_file));
	sample_plugin->uiid = anjuta_ui_merge (plugin->ui, UI_FILE);
	anjuta_shell_add_widget (plugin->shell, wid,
				  "AnjutaSamplePlugin", _("SamplePlugin"), NULL);
}

static void
dispose (GObject *obj)
{
	SamplePlugin *plugin = (SamplePlugin*)obj;
	anjuta_ui_unmerge (ANJUTA_PLUGIN (obj)->ui, plugin->uiid);
}

static void
sample_plugin_instance_init (GObject *obj)
{
	SamplePlugin *plugin = (SamplePlugin*)obj;
	plugin->uiid = 0;
}

static void
sample_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->shell_set = shell_set;
	plugin_class->ui_set = ui_set;
	plugin_class->prefs_set = prefs_set;
	klass->dispose = dispose;
}

ANJUTA_PLUGIN_BOILERPLATE (SamplePlugin, sample_plugin);
ANJUTA_SIMPLE_PLUGIN (SamplePlugin, sample_plugin);
