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

#include "git-apply-mailbox-pane.h"

struct _GitApplyMailboxPanePriv
{
	GtkBuilder *builder;
};

G_DEFINE_TYPE (GitApplyMailboxPane, git_apply_mailbox_pane, GIT_TYPE_PANE);

static void
on_ok_action_activated (GtkAction *action, GitApplyMailboxPane *self)
{
	Git *plugin;
	AnjutaFileList *mailbox_list;
	GtkToggleAction *signoff_action;
	GList *paths;
	GitApplyMailboxCommand *apply_mailbox_command;

	plugin = ANJUTA_PLUGIN_GIT (anjuta_dock_pane_get_plugin (ANJUTA_DOCK_PANE (self)));
	mailbox_list = ANJUTA_FILE_LIST (gtk_builder_get_object (self->priv->builder,
	                                                         "mailbox_list"));
	signoff_action = GTK_TOGGLE_ACTION (gtk_builder_get_object (self->priv->builder,
	                                                           "signoff_action"));
	paths = anjuta_file_list_get_paths (mailbox_list);

	apply_mailbox_command = git_apply_mailbox_command_new (plugin->project_root_directory,
	                                                       paths,
	                                                       gtk_toggle_action_get_active (signoff_action));

	git_pane_create_message_view (plugin);

	g_signal_connect (G_OBJECT (apply_mailbox_command), "data-arrived",
	                  G_CALLBACK (git_pane_on_command_info_arrived),
	                  plugin);

	g_signal_connect (G_OBJECT (apply_mailbox_command), "command-finished",
	                  G_CALLBACK (git_pane_report_errors),
	                  plugin);


	g_signal_connect (G_OBJECT (apply_mailbox_command), "command-finished",
	                  G_CALLBACK (g_object_unref),
	                  NULL);

	anjuta_command_start (ANJUTA_COMMAND (apply_mailbox_command));

	anjuta_util_glist_strings_free (paths);

	git_pane_remove_from_dock (GIT_PANE (self));
}


static void
git_apply_mailbox_pane_init (GitApplyMailboxPane *self)
{
	gchar *objects[] = {"apply_mailbox_pane",
						"ok_action",
						"cancel_action",
						"signoff_action",
						NULL};
	GError *error = NULL;
	GtkAction *ok_action;
	GtkAction *cancel_action;

	self->priv = g_new0 (GitApplyMailboxPanePriv, 1);
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
git_apply_mailbox_pane_finalize (GObject *object)
{
	GitApplyMailboxPane *self;

	self = GIT_APPLY_MAILBOX_PANE (object);

	g_object_unref (self->priv->builder);
	g_free (self->priv);

	G_OBJECT_CLASS (git_apply_mailbox_pane_parent_class)->finalize (object);
}

static GtkWidget *
git_apply_mailbox_pane_get_widget (AnjutaDockPane *pane)
{
	GitApplyMailboxPane *self;

	self = GIT_APPLY_MAILBOX_PANE (pane);

	return GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                           "apply_mailbox_pane"));
}

static void
git_apply_mailbox_pane_class_init (GitApplyMailboxPaneClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaDockPaneClass *pane_class = ANJUTA_DOCK_PANE_CLASS (klass);

	object_class->finalize = git_apply_mailbox_pane_finalize;
	pane_class->get_widget = git_apply_mailbox_pane_get_widget;
	pane_class->refresh = NULL;
}


AnjutaDockPane *
git_apply_mailbox_pane_new (Git *plugin)
{
	return g_object_new (GIT_TYPE_APPLY_MAILBOX_PANE, "plugin", plugin, NULL);
}

void
on_apply_mailbox_button_clicked (GtkAction *action, Git *plugin)
{
	AnjutaDockPane *pane;

	pane = git_apply_mailbox_pane_new (plugin);

	anjuta_dock_replace_command_pane (ANJUTA_DOCK (plugin->dock), "ApplyMailbox", 
	                                  _("Apply Mailbox Files"), NULL, pane,  
	                                  GDL_DOCK_BOTTOM, NULL, 0, NULL);
}

void
on_apply_mailbox_continue_button_clicked (GtkAction *action, Git *plugin)
{
	GitApplyMailboxContinueCommand *continue_command;

	continue_command = git_apply_mailbox_continue_command_new (plugin->project_root_directory,
	                                                           GIT_APPLY_MAILBOX_CONTINUE_ACTION_RESOLVED);

	git_pane_create_message_view (plugin);

	g_signal_connect (G_OBJECT (continue_command), "data-arrived",
	                  G_CALLBACK (git_pane_on_command_info_arrived),
	                  plugin);

	g_signal_connect (G_OBJECT (continue_command), "command-finished",
	                  G_CALLBACK (git_pane_report_errors),
	                  plugin);

	g_signal_connect (G_OBJECT (continue_command), "command-finished",
	                  G_CALLBACK (g_object_unref),
	                  NULL);

	anjuta_command_start (ANJUTA_COMMAND (continue_command));
}

void
on_apply_mailbox_skip_button_clicked (GtkAction *action, Git *plugin)
{
	GitApplyMailboxContinueCommand *continue_command;

	continue_command = git_apply_mailbox_continue_command_new (plugin->project_root_directory,
	                                                           GIT_APPLY_MAILBOX_CONTINUE_ACTION_SKIP);

	git_pane_create_message_view (plugin);

	g_signal_connect (G_OBJECT (continue_command), "data-arrived",
	                  G_CALLBACK (git_pane_on_command_info_arrived),
	                  plugin);

	g_signal_connect (G_OBJECT (continue_command), "command-finished",
	                  G_CALLBACK (git_pane_report_errors),
	                  plugin);

	g_signal_connect (G_OBJECT (continue_command), "command-finished",
	                  G_CALLBACK (g_object_unref),
	                  NULL);

	anjuta_command_start (ANJUTA_COMMAND (continue_command));
}

void
on_apply_mailbox_abort_button_clicked (GtkAction *action, Git *plugin)
{
	GitApplyMailboxContinueCommand *continue_command;

	continue_command = git_apply_mailbox_continue_command_new (plugin->project_root_directory,
	                                                           GIT_APPLY_MAILBOX_CONTINUE_ACTION_ABORT);

	git_pane_create_message_view (plugin);

	g_signal_connect (G_OBJECT (continue_command), "data-arrived",
	                  G_CALLBACK (git_pane_on_command_info_arrived),
	                  plugin);

	g_signal_connect (G_OBJECT (continue_command), "command-finished",
	                  G_CALLBACK (git_pane_report_errors),
	                  plugin);

	g_signal_connect (G_OBJECT (continue_command), "command-finished",
	                  G_CALLBACK (g_object_unref),
	                  NULL);

	anjuta_command_start (ANJUTA_COMMAND (continue_command));
}