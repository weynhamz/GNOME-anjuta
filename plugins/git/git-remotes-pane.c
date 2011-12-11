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

#include "git-remotes-pane.h"

struct _GitRemotesPanePriv
{
	GtkBuilder *builder;
	gchar *selected_remote;
	GtkWidget* remotes_combo;
};

G_DEFINE_TYPE (GitRemotesPane, git_remotes_pane, GIT_TYPE_PANE);

static void
git_remotes_pane_init (GitRemotesPane *self)
{
	gchar *objects[] = {"remotes_pane",
						"remotes_list_model",
						NULL};
	GError *error = NULL;

	self->priv = g_new0 (GitRemotesPanePriv, 1);
	self->priv->builder = gtk_builder_new ();

	if (!gtk_builder_add_objects_from_file (self->priv->builder, BUILDER_FILE, 
	                                        objects, 
	                                        &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}

	self->priv->remotes_combo = GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                                                "remotes_combo"));
}

static void
git_remotes_pane_finalize (GObject *object)
{
	GitRemotesPane *self;

	self = GIT_REMOTES_PANE (object);

	g_object_unref (self->priv->builder);
	g_free (self->priv->selected_remote);
	g_free (self->priv);

	G_OBJECT_CLASS (git_remotes_pane_parent_class)->finalize (object);
}

static GtkWidget *
git_remotes_pane_get_widget (AnjutaDockPane *pane)
{
	GitRemotesPane *self;

	self = GIT_REMOTES_PANE (pane);

	return GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                           "remotes_pane"));
}

static void
git_remotes_pane_class_init (GitRemotesPaneClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaDockPaneClass* pane_class = ANJUTA_DOCK_PANE_CLASS (klass);

	object_class->finalize = git_remotes_pane_finalize;
	pane_class->get_widget = git_remotes_pane_get_widget;
	pane_class->refresh = NULL;
}

static void
on_remote_list_command_data_arrived (AnjutaCommand *command, 
                                     GitRemotesPane *self)
{
	GQueue *output;
	gchar *remote;

	output = git_raw_output_command_get_output (GIT_RAW_OUTPUT_COMMAND (command));

	while (g_queue_peek_head (output))
	{
		remote = g_queue_pop_head (output);

		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(self->priv->remotes_combo),
		                                remote);
		if (gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->remotes_combo))
		    == -1)
		{
			gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->remotes_combo), 0);
		}

		g_free (remote);
	}
}

AnjutaDockPane *
git_remotes_pane_new (Git *plugin)
{
	GitRemotesPane *self;
	GtkListStore *remotes_list_model;

	self = g_object_new (GIT_TYPE_REMOTES_PANE, "plugin", plugin, NULL);
	remotes_list_model = GTK_LIST_STORE (gtk_builder_get_object (self->priv->builder,
	                                                             "remotes_list_model"));

	g_signal_connect_swapped (G_OBJECT (plugin->remote_list_command), 
	                          "command-started",
	                          G_CALLBACK (gtk_list_store_clear),
	                          remotes_list_model);

	g_signal_connect (G_OBJECT (plugin->remote_list_command), "data-arrived",
	                  G_CALLBACK (on_remote_list_command_data_arrived),
	                  self);

	return ANJUTA_DOCK_PANE (self);
}

gchar *
git_remotes_pane_get_selected_remote (GitRemotesPane *self)
{
	return gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (self->priv->remotes_combo));
}
