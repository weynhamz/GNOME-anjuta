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

#include "git-revert-pane.h"

struct _GitRevertPanePriv
{
	GtkBuilder *builder;
};

G_DEFINE_TYPE (GitRevertPane, git_revert_pane, GIT_TYPE_PANE);

static void
on_ok_action_activated (GtkAction *action, GitRevertPane *self)
{
	Git *plugin;
	AnjutaEntry *commit_entry;
	GtkToggleButton *no_commit_check;
	gchar *commit;
	GitRevertCommand *revert_command;

	plugin = ANJUTA_PLUGIN_GIT (anjuta_dock_pane_get_plugin (ANJUTA_DOCK_PANE (self)));
	commit_entry = ANJUTA_ENTRY (gtk_builder_get_object (self->priv->builder,
	                                                     "commit_entry"));
	no_commit_check = GTK_TOGGLE_BUTTON (gtk_builder_get_object (self->priv->builder,
	                                                             "no_commit_check"));

	commit = anjuta_entry_dup_text (commit_entry);

	if (!git_pane_check_input (GTK_WIDGET (ANJUTA_PLUGIN (plugin)->shell),
	                           GTK_WIDGET (commit_entry), commit,
	                           _("Please enter a commit.")))
	{
		g_free (commit);

		return;
	}

	revert_command = git_revert_command_new (plugin->project_root_directory,
	                                         commit,
	                                         gtk_toggle_button_get_active (no_commit_check));

	g_signal_connect (G_OBJECT (revert_command), "command-finished",
	                  G_CALLBACK (git_pane_report_errors),
	                  plugin);


	g_signal_connect (G_OBJECT (revert_command), "command-finished",
	                  G_CALLBACK (g_object_unref),
	                  NULL);

	anjuta_command_start (ANJUTA_COMMAND (revert_command));


	g_free (commit);

	git_pane_remove_from_dock (GIT_PANE (self));
}


static void
git_revert_pane_init (GitRevertPane *self)
{
	gchar *objects[] = {"revert_pane",
						"ok_action",
						"cancel_action",
						NULL};
	GError *error = NULL;
	GtkAction *ok_action;
	GtkAction *cancel_action;

	self->priv = g_new0 (GitRevertPanePriv, 1);
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
git_revert_pane_finalize (GObject *object)
{
	GitRevertPane *self;

	self = GIT_REVERT_PANE (object);

	g_object_unref (self->priv->builder);
	g_free (self->priv);

	G_OBJECT_CLASS (git_revert_pane_parent_class)->finalize (object);
}

static GtkWidget *
git_revert_pane_get_widget (AnjutaDockPane *pane)
{
	GitRevertPane *self;

	self = GIT_REVERT_PANE (pane);

	return GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                           "revert_pane"));
}

static void
git_revert_pane_class_init (GitRevertPaneClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaDockPaneClass *pane_class = ANJUTA_DOCK_PANE_CLASS (klass);

	object_class->finalize = git_revert_pane_finalize;
	pane_class->get_widget = git_revert_pane_get_widget;
	pane_class->refresh = NULL;
}


AnjutaDockPane *
git_revert_pane_new (Git *plugin)
{
	return g_object_new (GIT_TYPE_REVERT_PANE, "plugin", plugin, NULL);
}

void
on_revert_button_clicked (GtkAction *action, Git *plugin)
{
	AnjutaDockPane *pane;

	pane = git_revert_pane_new (plugin);

	anjuta_dock_replace_command_pane (ANJUTA_DOCK (plugin->dock), "Revert", 
	                                  _("Revert"), NULL, pane, GDL_DOCK_BOTTOM, NULL, 0,
	                                  NULL);
}