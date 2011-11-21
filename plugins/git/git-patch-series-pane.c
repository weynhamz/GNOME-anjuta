/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * git-shell-test
 * Copyright (C) James Liggett 2010 <jrliggett@cox.net>
 * 
 * git-shell-test is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * git-shell-test is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "git-patch-series-pane.h"

struct _GitPatchSeriesPanePriv
{
	GtkBuilder *builder;
};

G_DEFINE_TYPE (GitPatchSeriesPane, git_patch_series_pane, GIT_TYPE_PANE);

static void
on_ok_button_clicked (GtkButton *button, GitPatchSeriesPane *self)
{
	Git *plugin;
	AnjutaEntry *revision_entry;
	GtkFileChooser *folder_chooser_button;
	GtkToggleButton *signoff_check;
	const gchar *revision;
	gchar *output_path;
	GitFormatPatchCommand *format_patch_command;

	plugin = ANJUTA_PLUGIN_GIT (anjuta_dock_pane_get_plugin (ANJUTA_DOCK_PANE (self)));
	revision_entry = ANJUTA_ENTRY (gtk_builder_get_object (self->priv->builder,
	                                                       "revision_entry"));
	folder_chooser_button = GTK_FILE_CHOOSER (gtk_builder_get_object (self->priv->builder,
	                                                                  "folder_chooser_button"));
	signoff_check = GTK_TOGGLE_BUTTON (gtk_builder_get_object (self->priv->builder,
	                                                           "signoff_check"));
	revision = anjuta_entry_get_text (revision_entry);

	if (g_utf8_strlen (revision, -1) == 0)
		revision = "origin";

	output_path = gtk_file_chooser_get_filename (folder_chooser_button);

	format_patch_command = git_format_patch_command_new (plugin->project_root_directory,
	                                                     output_path,
	                                                     revision,
	                                                     gtk_toggle_button_get_active (signoff_check));
	
	git_pane_create_message_view (plugin);

	g_signal_connect (G_OBJECT (format_patch_command), "data-arrived",
	                  G_CALLBACK (git_pane_on_command_info_arrived),
	                  plugin);

	g_signal_connect (G_OBJECT (format_patch_command), "command-finished",
	                  G_CALLBACK (git_pane_report_errors),
	                  plugin);


	g_signal_connect (G_OBJECT (format_patch_command), "command-finished",
	                  G_CALLBACK (g_object_unref),
	                  NULL);

	anjuta_command_start (ANJUTA_COMMAND (format_patch_command));

	g_free (output_path);

	git_pane_remove_from_dock (GIT_PANE (self));
}


static void
git_patch_series_pane_init (GitPatchSeriesPane *self)
{
	gchar *objects[] = {"patch_series_pane",
						NULL};
	GError *error = NULL;
	GtkWidget *ok_button;
	GtkWidget *cancel_button;
	

	self->priv = g_new0 (GitPatchSeriesPanePriv, 1);
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
git_patch_series_pane_finalize (GObject *object)
{
	GitPatchSeriesPane *self;

	self = GIT_PATCH_SERIES_PANE (object);

	g_object_unref (self->priv->builder);
	g_free (self->priv);

	G_OBJECT_CLASS (git_patch_series_pane_parent_class)->finalize (object);
}

static GtkWidget *
git_patch_series_pane_get_widget (AnjutaDockPane *pane)
{
	GitPatchSeriesPane *self;

	self = GIT_PATCH_SERIES_PANE (pane);

	return GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                           "patch_series_pane"));
}

static void
git_patch_series_pane_class_init (GitPatchSeriesPaneClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaDockPaneClass *pane_class = ANJUTA_DOCK_PANE_CLASS (klass);

	object_class->finalize = git_patch_series_pane_finalize;
	pane_class->get_widget = git_patch_series_pane_get_widget;
	pane_class->refresh = NULL;
}


AnjutaDockPane *
git_patch_series_pane_new (Git *plugin)
{
	return g_object_new (GIT_TYPE_PATCH_SERIES_PANE, "plugin", plugin, NULL);
}

void
on_patch_series_button_clicked (GtkAction *action, Git *plugin)
{
	AnjutaDockPane *pane;

	pane = git_patch_series_pane_new (plugin);

	anjuta_dock_replace_command_pane (ANJUTA_DOCK (plugin->dock), "PatchSeries", 
	                                  _("Generate Patch Series"), NULL, pane,  
	                                  GDL_DOCK_BOTTOM, NULL, 0, NULL);
}