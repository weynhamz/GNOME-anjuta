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

#include "git-add-remote-pane.h"

struct _GitAddRemotePanePriv
{
	GtkBuilder *builder;
};

G_DEFINE_TYPE (GitAddRemotePane, git_add_remote_pane, GIT_TYPE_PANE);

static void
on_ok_button_clicked (GtkButton *button, GitAddRemotePane *self)
{
	Git *plugin;
	GtkEntry *name_entry;
	GtkEntry *url_entry;
	GtkToggleButton *fetch_check;
	gchar *name;
	gchar *url;
	GitRemoteAddCommand *add_remote_command;

	plugin = ANJUTA_PLUGIN_GIT (anjuta_dock_pane_get_plugin (ANJUTA_DOCK_PANE (self)));
	name_entry = GTK_ENTRY (gtk_builder_get_object (self->priv->builder,
	                                                "name_entry"));
	url_entry = GTK_ENTRY (gtk_builder_get_object (self->priv->builder,
	                                               "url_entry"));
	fetch_check = GTK_TOGGLE_BUTTON (gtk_builder_get_object (self->priv->builder,
	                                                         "fetch_check"));
	name = gtk_editable_get_chars (GTK_EDITABLE (name_entry), 0, -1);
	url = gtk_editable_get_chars (GTK_EDITABLE (url_entry), 0, -1);

	if (!git_pane_check_input (GTK_WIDGET (ANJUTA_PLUGIN (plugin)->shell),
	                           GTK_WIDGET (name_entry), name,
	                           _("Please enter a remote name.")) ||
	    !git_pane_check_input (GTK_WIDGET (ANJUTA_PLUGIN (plugin)->shell),
	                           GTK_WIDGET (url_entry), url,
	                           _("Please enter a URL")))
	{
		g_free (name);
		g_free (url);

		return;
	}

	add_remote_command = git_remote_add_command_new (plugin->project_root_directory,
	                                                 name, url,
	                                                 gtk_toggle_button_get_active (fetch_check));

	/* Info outupt is only relevant if we're also fetching */
	if (gtk_toggle_button_get_active (fetch_check))
	{
		git_pane_create_message_view (plugin);

		g_signal_connect (G_OBJECT (add_remote_command), "data-arrived",
		                  G_CALLBACK (git_pane_on_command_info_arrived),
		                  plugin);
	}

	g_signal_connect (G_OBJECT (add_remote_command), "command-finished",
	                  G_CALLBACK (git_pane_report_errors),
	                  plugin);


	g_signal_connect (G_OBJECT (add_remote_command), "command-finished",
	                  G_CALLBACK (g_object_unref),
	                  NULL);

	anjuta_command_start (ANJUTA_COMMAND (add_remote_command));


	g_free (name);
	g_free (url);

	git_pane_remove_from_dock (GIT_PANE (self));
}

static void
git_add_remote_pane_init (GitAddRemotePane *self)
{
	gchar *objects[] = {"add_remote_pane",
						NULL};
	GError *error = NULL;
	GtkWidget *ok_button;
	GtkWidget *cancel_button;

	self->priv = g_new0 (GitAddRemotePanePriv, 1);
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
git_add_remote_pane_finalize (GObject *object)
{
	GitAddRemotePane *self;

	self = GIT_ADD_REMOTE_PANE (object);

	g_object_unref (self->priv->builder);
	g_free (self->priv);

	G_OBJECT_CLASS (git_add_remote_pane_parent_class)->finalize (object);
}

static GtkWidget *
git_add_remote_pane_get_widget (AnjutaDockPane *pane)
{
	GitAddRemotePane *self;

	self = GIT_ADD_REMOTE_PANE (pane);

	return GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                           "add_remote_pane"));
}

static void
git_add_remote_pane_class_init (GitAddRemotePaneClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaDockPaneClass *pane_class = ANJUTA_DOCK_PANE_CLASS (klass);

	object_class->finalize = git_add_remote_pane_finalize;
	pane_class->get_widget = git_add_remote_pane_get_widget;
	pane_class->refresh = NULL;
}


AnjutaDockPane *
git_add_remote_pane_new (Git *plugin)
{
	return g_object_new (GIT_TYPE_ADD_REMOTE_PANE, "plugin", plugin, NULL);
}

void
on_add_remote_button_clicked (GtkAction *action, Git *plugin)
{
	AnjutaDockPane *pane;

	pane = git_add_remote_pane_new (plugin);

	anjuta_dock_replace_command_pane (ANJUTA_DOCK (plugin->dock), "AddRemote", 
	                                  _("Add Remote"), NULL, pane, GDL_DOCK_BOTTOM, NULL, 0,
	                                  NULL);
}