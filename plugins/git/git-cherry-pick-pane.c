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

#include "git-cherry-pick-pane.h"

struct _GitCherryPickPanePriv
{
	GtkBuilder *builder;
};

G_DEFINE_TYPE (GitCherryPickPane, git_cherry_pick_pane, GIT_TYPE_PANE);

static void
on_ok_action_activated (GtkAction *action, GitCherryPickPane *self)
{
	Git *plugin;
	AnjutaEntry *cherry_pick_revision_entry;
	GtkToggleAction *no_commit_action;
	GtkToggleButton *show_source_check;
	GtkToggleAction *signoff_action;
	gchar *revision;
	GitCherryPickCommand *cherry_pick_command;

	plugin = ANJUTA_PLUGIN_GIT (anjuta_dock_pane_get_plugin (ANJUTA_DOCK_PANE (self)));
	cherry_pick_revision_entry = ANJUTA_ENTRY (gtk_builder_get_object (self->priv->builder,
	                                                   				   "cherry_pick_revision_entry"));
	no_commit_action = GTK_TOGGLE_ACTION (gtk_builder_get_object (self->priv->builder,
	                                                              "no_commit_action"));
	show_source_check = GTK_TOGGLE_BUTTON (gtk_builder_get_object (self->priv->builder,
	                                                               "show_source_check"));
	signoff_action = GTK_TOGGLE_ACTION (gtk_builder_get_object (self->priv->builder,
	                                                           "signoff_action"));
	revision = anjuta_entry_dup_text (cherry_pick_revision_entry);

	if (!git_pane_check_input (GTK_WIDGET (ANJUTA_PLUGIN (plugin)->shell),
	                           GTK_WIDGET (cherry_pick_revision_entry), revision,
	                           _("Please enter a revision.")))
	{
		g_free (revision);
		return;
	}
	
	cherry_pick_command = git_cherry_pick_command_new (plugin->project_root_directory,
	                                        		   revision,
	                                                   gtk_toggle_action_get_active (no_commit_action),
	                                                   gtk_toggle_button_get_active (show_source_check),
	                                                   gtk_toggle_action_get_active (signoff_action));

	git_pane_create_message_view (plugin);

	g_signal_connect (G_OBJECT (cherry_pick_command), "data-arrived",
	                  G_CALLBACK (git_pane_on_command_info_arrived),
	                  plugin);

	g_signal_connect (G_OBJECT (cherry_pick_command), "command-finished",
	                  G_CALLBACK (git_pane_report_errors),
	                  plugin);


	g_signal_connect (G_OBJECT (cherry_pick_command), "command-finished",
	                  G_CALLBACK (g_object_unref),
	                  NULL);

	anjuta_command_start (ANJUTA_COMMAND (cherry_pick_command));

	g_free (revision);

	git_pane_remove_from_dock (GIT_PANE (self));
}


static void
git_cherry_pick_pane_init (GitCherryPickPane *self)
{
	gchar *objects[] = {"cherry_pick_pane",
						"ok_action",
						"cancel_action",
						"signoff_action",
						"no_commit_action",
						NULL};
	GError *error = NULL;
	GtkAction *ok_action;
	GtkAction *cancel_action;

	self->priv = g_new0 (GitCherryPickPanePriv, 1);
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
git_cherry_pick_pane_finalize (GObject *object)
{
	GitCherryPickPane *self;

	self = GIT_CHERRY_PICK_PANE (object);

	g_object_unref (self->priv->builder);
	g_free (self->priv);

	G_OBJECT_CLASS (git_cherry_pick_pane_parent_class)->finalize (object);
}

static GtkWidget *
git_cherry_pick_pane_get_widget (AnjutaDockPane *pane)
{
	GitCherryPickPane *self;

	self = GIT_CHERRY_PICK_PANE (pane);

	return GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                           "cherry_pick_pane"));
}

static void
git_cherry_pick_pane_class_init (GitCherryPickPaneClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaDockPaneClass *pane_class = ANJUTA_DOCK_PANE_CLASS (klass);

	object_class->finalize = git_cherry_pick_pane_finalize;
	pane_class->get_widget = git_cherry_pick_pane_get_widget;
	pane_class->refresh = NULL;
}


AnjutaDockPane *
git_cherry_pick_pane_new (Git *plugin)
{
	return g_object_new (GIT_TYPE_CHERRY_PICK_PANE, "plugin", plugin, NULL);
}

void
on_cherry_pick_button_clicked (GtkAction *action, Git *plugin)
{
	AnjutaDockPane *pane;

	pane = git_cherry_pick_pane_new (plugin);

	anjuta_dock_replace_command_pane (ANJUTA_DOCK (plugin->dock), "CherryPick", 
	                                  _("Cherry Pick"), NULL, pane, GDL_DOCK_BOTTOM, NULL, 
	                                  0, NULL);
}