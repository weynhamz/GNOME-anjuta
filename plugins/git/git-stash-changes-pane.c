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

#include "git-stash-changes-pane.h"

struct _GitStashChangesPanePriv
{
	GtkBuilder *builder;
};

G_DEFINE_TYPE (GitStashChangesPane, git_stash_changes_pane, GIT_TYPE_PANE);

static void
on_ok_action_activated (GtkAction *action, GitStashChangesPane *self)
{
	Git *plugin;
	AnjutaColumnTextView *message_view;
	GtkToggleButton *stash_index_check;
	gchar *message;
	GitStashSaveCommand *save_command;

	plugin = ANJUTA_PLUGIN_GIT (anjuta_dock_pane_get_plugin (ANJUTA_DOCK_PANE (self)));
	message_view = ANJUTA_COLUMN_TEXT_VIEW (gtk_builder_get_object (self->priv->builder,
	                                                                "message_view"));
	stash_index_check = GTK_TOGGLE_BUTTON (gtk_builder_get_object (self->priv->builder,
	                                                               "stash_index_check"));
	message = anjuta_column_text_view_get_text (message_view);

	/* Don't pass an empty message */
	if (!g_utf8_strlen (message, -1))
	{
		g_free (message);
		message = NULL;
	}

	save_command = git_stash_save_command_new (plugin->project_root_directory,
	                                           gtk_toggle_button_get_active (stash_index_check),
	                                           message);

	g_free (message);

	g_signal_connect (G_OBJECT (save_command), "command-finished",
	                  G_CALLBACK (git_pane_report_errors),
	                  plugin);


	g_signal_connect (G_OBJECT (save_command), "command-finished",
	                  G_CALLBACK (g_object_unref),
	                  NULL);

	anjuta_command_start (ANJUTA_COMMAND (save_command));
	

	git_pane_remove_from_dock (GIT_PANE (self));
	                            
}

static void
git_stash_changes_pane_init (GitStashChangesPane *self)
{
	gchar *objects[] = {"stash_changes_pane",
						"ok_action",
						"cancel_action",
						NULL};
	GError *error = NULL;
	GtkAction *ok_action;
	GtkAction *cancel_action;

	self->priv = g_new0 (GitStashChangesPanePriv, 1);
	self->priv->builder = gtk_builder_new ();

	if (!gtk_builder_add_objects_from_file (self->priv->builder, BUILDER_FILE, 
	                                        objects, 
	                                        &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}

	ok_action = GTK_ACTION (gtk_builder_get_object (self->priv->builder,
	                                                "ok_action"));
	cancel_action = GTK_ACTION (gtk_builder_get_object (self->priv->builder,
	                                                "cancel_action"));

	g_signal_connect (G_OBJECT (ok_action), "activate",
	                  G_CALLBACK (on_ok_action_activated),
	                  self);

	g_signal_connect_swapped (G_OBJECT (cancel_action), "activate",
	                          G_CALLBACK (git_pane_remove_from_dock),
	                          self);
}

static void
git_stash_changes_pane_finalize (GObject *object)
{
	GitStashChangesPane *self;

	self = GIT_STASH_CHANGES_PANE (object);

	g_object_unref (self->priv->builder);
	g_free (self->priv);

	G_OBJECT_CLASS (git_stash_changes_pane_parent_class)->finalize (object);
}

static GtkWidget *
get_widget (AnjutaDockPane *pane)
{
	GitStashChangesPane *self;

	self = GIT_STASH_CHANGES_PANE (pane);

	return GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                           "stash_changes_pane"));
}

static void
git_stash_changes_pane_class_init (GitStashChangesPaneClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaDockPaneClass* pane_class = ANJUTA_DOCK_PANE_CLASS (klass);

	object_class->finalize = git_stash_changes_pane_finalize;
	pane_class->get_widget = get_widget;
	pane_class->refresh = NULL;
}


AnjutaDockPane *
git_stash_changes_pane_new (Git *plugin)
{
	return g_object_new (GIT_TYPE_STASH_CHANGES_PANE, "plugin", plugin, NULL);
}

void
on_stash_changes_button_clicked (GtkAction *action, Git *plugin)
{
	AnjutaDockPane *stash_changes_pane;

	stash_changes_pane = git_stash_changes_pane_new (plugin);

	anjuta_dock_replace_command_pane (ANJUTA_DOCK (plugin->dock), "StashChanges",
	                                  _("Stash Uncommitted Changes"), NULL, 
	                                  stash_changes_pane, GDL_DOCK_BOTTOM, NULL, 0, NULL);
}
