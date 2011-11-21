/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2010 <jrliggett@cox.net>
	 * 
 * anjuta is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
	 * 
 * anjuta is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "git-remove-files-pane.h"

struct _GitRemoveFilesPanePriv
{
	GtkBuilder *builder;
};

G_DEFINE_TYPE (GitRemoveFilesPane, git_remove_files_pane, GIT_TYPE_PANE);

static void
on_ok_button_clicked (GtkButton *button, GitRemoveFilesPane *self)
{
	Git *plugin;
	AnjutaFileList *file_list;
	GtkToggleButton *force_check;
	GList *paths;
	GitRemoveCommand *remove_command;

	plugin = ANJUTA_PLUGIN_GIT (anjuta_dock_pane_get_plugin (ANJUTA_DOCK_PANE (self)));
	file_list = ANJUTA_FILE_LIST (gtk_builder_get_object (self->priv->builder,
	                                                      "file_list"));
	force_check = GTK_TOGGLE_BUTTON (gtk_builder_get_object (self->priv->builder,
	                                                         "force_check"));
	paths = anjuta_file_list_get_paths (file_list);
	remove_command = git_remove_command_new_list (plugin->project_root_directory,
	                                              paths,
	                                              gtk_toggle_button_get_active (force_check));

	anjuta_util_glist_strings_free (paths);

	g_signal_connect (G_OBJECT (remove_command), "command-finished",
	                  G_CALLBACK (git_pane_report_errors),
	                  plugin);


	g_signal_connect (G_OBJECT (remove_command), "command-finished",
	                  G_CALLBACK (g_object_unref),
	                  NULL);

	anjuta_command_start (ANJUTA_COMMAND (remove_command));

	git_pane_remove_from_dock (GIT_PANE (self));
}

static void
git_remove_files_pane_init (GitRemoveFilesPane *self)
{
	gchar *objects[] = {"remove_pane",
						NULL};
	GError *error = NULL;
	GtkWidget *ok_button;
	GtkWidget *cancel_button;

	self->priv = g_new0 (GitRemoveFilesPanePriv, 1);
	self->priv->builder = gtk_builder_new ();

	if (!gtk_builder_add_objects_from_file (self->priv->builder, BUILDER_FILE, 
	                                        objects, 
	                                        &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}

	ok_button = GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                                "ok_button"));
	cancel_button = GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                                    "cancel_button"));

	g_signal_connect (G_OBJECT (ok_button), "clicked",
	                  G_CALLBACK (on_ok_button_clicked),
	                  self);

	g_signal_connect_swapped (G_OBJECT (cancel_button), "clicked",
	                          G_CALLBACK (git_pane_remove_from_dock),
	                          self);
}

static void
git_remove_files_pane_finalize (GObject *object)
{
	GitRemoveFilesPane *self;

	self = GIT_REMOVE_FILES_PANE (object);

	g_object_unref (self->priv->builder);
	g_free (self->priv);

	G_OBJECT_CLASS (git_remove_files_pane_parent_class)->finalize (object);
}

static GtkWidget *
git_remove_files_pane_get_widget (AnjutaDockPane *pane)
{
	GitRemoveFilesPane *self;

	self = GIT_REMOVE_FILES_PANE (pane);

	return GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                           "remove_pane"));
}

static void
git_remove_files_pane_class_init (GitRemoveFilesPaneClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaDockPaneClass* pane_class = ANJUTA_DOCK_PANE_CLASS (klass);

	object_class->finalize = git_remove_files_pane_finalize;
	pane_class->get_widget = git_remove_files_pane_get_widget;
	pane_class->refresh = NULL;
}

AnjutaDockPane *
git_remove_files_pane_new (Git *plugin)
{
	GitRemoveFilesPane *self;
	AnjutaFileList *file_list;

	self = g_object_new (GIT_TYPE_REMOVE_FILES_PANE, "plugin", plugin, NULL);
	file_list = ANJUTA_FILE_LIST (gtk_builder_get_object (self->priv->builder,
	                                                      "file_list"));

	anjuta_file_list_set_relative_path (file_list, 
	                                    plugin->project_root_directory);

	return ANJUTA_DOCK_PANE (self);
}

void
on_remove_button_clicked (GtkAction *action, Git* plugin)
{
	AnjutaDockPane *pane;

	pane = git_remove_files_pane_new (plugin);

	anjuta_dock_replace_command_pane (ANJUTA_DOCK (plugin->dock), "RemoveFiles", 
	                                  _("Remove Files"), NULL, pane, GDL_DOCK_BOTTOM,
	                                  NULL, 0, NULL);
}
