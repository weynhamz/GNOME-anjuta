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

#include "git-reset-pane.h"

struct _GitResetPanePriv
{
	GtkBuilder *builder;
};

G_DEFINE_TYPE (GitResetPane, git_reset_pane, GIT_TYPE_PANE);

static void
on_ok_button_clicked (GtkButton *button, GitResetPane *self)
{
	Git *plugin;
	AnjutaEntry *revision_entry;
	GtkToggleButton *mixed_radio;
	GtkToggleButton *soft_radio;
	const gchar *revision;
	GitResetTreeMode mode;
	GitResetTreeCommand *reset_command;

	plugin = ANJUTA_PLUGIN_GIT (anjuta_dock_pane_get_plugin (ANJUTA_DOCK_PANE (self)));
	revision_entry = ANJUTA_ENTRY (gtk_builder_get_object (self->priv->builder,
	                                                       "revision_entry"));
	mixed_radio = GTK_TOGGLE_BUTTON (gtk_builder_get_object (self->priv->builder,
	                                                         "mixed_radio"));
	soft_radio = GTK_TOGGLE_BUTTON (gtk_builder_get_object (self->priv->builder,
	                                                        "soft_radio"));

	revision = anjuta_entry_get_text (revision_entry);

	if (g_utf8_strlen (revision, -1) == 0)
		revision = GIT_RESET_TREE_PREVIOUS;

	if (gtk_toggle_button_get_active (mixed_radio))
		mode = GIT_RESET_TREE_MODE_MIXED;
	else if (gtk_toggle_button_get_active (soft_radio))
		mode = GIT_RESET_TREE_MODE_SOFT;
	else
		mode = GIT_RESET_TREE_MODE_HARD;

	reset_command = git_reset_tree_command_new (plugin->project_root_directory,
	                                            revision, mode);

	g_signal_connect (G_OBJECT (reset_command), "command-finished",
	                  G_CALLBACK (git_pane_report_errors),
	                  plugin);


	g_signal_connect (G_OBJECT (reset_command), "command-finished",
	                  G_CALLBACK (g_object_unref),
	                  NULL);

	anjuta_command_start (ANJUTA_COMMAND (reset_command));


	git_pane_remove_from_dock (GIT_PANE (self));
}


static void
git_reset_pane_init (GitResetPane *self)
{
	gchar *objects[] = {"reset_pane",
						NULL};
	GError *error = NULL;
	GtkWidget *ok_button;
	GtkWidget *cancel_button;
	

	self->priv = g_new0 (GitResetPanePriv, 1);
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
git_reset_pane_finalize (GObject *object)
{
	GitResetPane *self;

	self = GIT_RESET_PANE (object);

	g_object_unref (self->priv->builder);
	g_free (self->priv);

	G_OBJECT_CLASS (git_reset_pane_parent_class)->finalize (object);
}

static GtkWidget *
git_reset_pane_get_widget (AnjutaDockPane *pane)
{
	GitResetPane *self;

	self = GIT_RESET_PANE (pane);

	return GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                           "reset_pane"));
}

static void
git_reset_pane_class_init (GitResetPaneClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaDockPaneClass *pane_class = ANJUTA_DOCK_PANE_CLASS (klass);

	object_class->finalize = git_reset_pane_finalize;
	pane_class->get_widget = git_reset_pane_get_widget;
	pane_class->refresh = NULL;
}


AnjutaDockPane *
git_reset_pane_new (Git *plugin)
{
	return g_object_new (GIT_TYPE_RESET_PANE, "plugin", plugin, NULL);
}

void
on_reset_button_clicked (GtkAction *action, Git *plugin)
{
	AnjutaDockPane *pane;

	pane = git_reset_pane_new (plugin);

	anjuta_dock_replace_command_pane (ANJUTA_DOCK (plugin->dock), "Reset", 
	                                  _("Reset"), NULL, pane, GDL_DOCK_BOTTOM, NULL, 0,
	                                  NULL);
}