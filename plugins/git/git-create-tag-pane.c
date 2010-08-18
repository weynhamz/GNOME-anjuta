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

#include "git-create-tag-pane.h"

struct _GitCreateTagPanePriv
{
	GtkBuilder *builder;
};

G_DEFINE_TYPE (GitCreateTagPane, git_create_tag_pane, GIT_TYPE_PANE);

static void
on_ok_button_clicked (GtkButton *button, GitCreateTagPane *self)
{
	Git *plugin;
	GtkEntry *name_entry;
	GtkToggleButton *revision_radio;
	GtkEntry *revision_entry;
	GtkToggleButton *force_check;
	GtkToggleButton *sign_check;
	GtkToggleButton *annotate_check;
	AnjutaColumnTextView *log_view;
	gchar *name;
	gchar *revision;
	gchar *log;
	GitTagCreateCommand *create_command;

	plugin = ANJUTA_PLUGIN_GIT (anjuta_dock_pane_get_plugin (ANJUTA_DOCK_PANE (self)));
	name_entry = GTK_ENTRY (gtk_builder_get_object (self->priv->builder,
	                                                "name_entry"));
	revision_radio = GTK_TOGGLE_BUTTON (gtk_builder_get_object (self->priv->builder,
	                                                            "revision_radio"));
	revision_entry = GTK_ENTRY (gtk_builder_get_object (self->priv->builder,
	                                                    "revision_entry"));
	force_check = GTK_TOGGLE_BUTTON (gtk_builder_get_object (self->priv->builder,
	                                                         "force_check"));
	sign_check = GTK_TOGGLE_BUTTON (gtk_builder_get_object (self->priv->builder,
	                                                        "sign_check"));
	annotate_check = GTK_TOGGLE_BUTTON (gtk_builder_get_object (self->priv->builder,
	                                                            "annotate_check"));
	log_view = ANJUTA_COLUMN_TEXT_VIEW (gtk_builder_get_object (self->priv->builder,
	                                                            "log_view"));
	name = gtk_editable_get_chars (GTK_EDITABLE (name_entry), 0, -1);
	revision = NULL;
	log = NULL;

	if (!git_pane_check_input (GTK_WIDGET (ANJUTA_PLUGIN (plugin)->shell),
	                           GTK_WIDGET (name_entry), name,
	                           _("Please enter a tag name.")))
	{
		g_free (name);

		return;
	}

	if (gtk_toggle_button_get_active (revision_radio))
	{
		revision = gtk_editable_get_chars (GTK_EDITABLE (revision_entry), 0, 
		                                   -1);

		if (!git_pane_check_input (GTK_WIDGET (ANJUTA_PLUGIN (plugin)->shell),
		                           GTK_WIDGET (revision_entry), revision,
		                           _("Please enter a revision.")))
		{
			g_free (name);
			g_free (revision);

			return;
		}
	}

	if (gtk_toggle_button_get_active (annotate_check))
	{
		log = anjuta_column_text_view_get_text (log_view);

		if (!git_pane_check_input (GTK_WIDGET (ANJUTA_PLUGIN (plugin)->shell),
		                           GTK_WIDGET (log_view), log,
		                           _("Please enter a log message.")))
		{
			g_free (name);
			g_free (revision);
			g_free (log);

			return;
		}
	}

	create_command = git_tag_create_command_new (plugin->project_root_directory,
	                                             name,
	                                             revision,
	                                             log,
	                                             gtk_toggle_button_get_active (sign_check),
	                                             gtk_toggle_button_get_active (force_check));

	g_signal_connect (G_OBJECT (create_command), "command-finished",
	                  G_CALLBACK (git_pane_report_errors),
	                  plugin);


	g_signal_connect (G_OBJECT (create_command), "command-finished",
	                  G_CALLBACK (g_object_unref),
	                  NULL);

	anjuta_command_start (ANJUTA_COMMAND (create_command));


	g_free (name);
	g_free (revision);
	g_free (log);

	git_pane_remove_from_dock (GIT_PANE (self));
}

static void
set_widget_sensitive (GtkToggleButton *button, GtkWidget *widget)
{
	gtk_widget_set_sensitive (widget,
	                          gtk_toggle_button_get_active (button));
}

static void
git_create_tag_pane_init (GitCreateTagPane *self)
{
	gchar *objects[] = {"create_tag_pane",
						NULL};
	GError *error = NULL;
	GtkWidget *ok_button;
	GtkWidget *cancel_button;
	GtkWidget *revision_radio;
	GtkWidget *revision_entry;
	GtkWidget *annotate_check;
	GtkWidget *log_view;
	

	self->priv = g_new0 (GitCreateTagPanePriv, 1);
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
	revision_radio = GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                                     "revision_radio"));
	revision_entry = GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                                     "revision_entry"));
	annotate_check = GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                                     "annotate_check"));
	log_view = GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                               "log_view"));

	g_signal_connect (G_OBJECT (ok_button), "clicked",
	                  G_CALLBACK (on_ok_button_clicked),
	                  self);

	g_signal_connect_swapped (G_OBJECT (cancel_button), "clicked",
	                          G_CALLBACK (git_pane_remove_from_dock),
	                          self);

	g_signal_connect (G_OBJECT (revision_radio), "toggled",
	                  G_CALLBACK (set_widget_sensitive),
	                  revision_entry);

	g_signal_connect (G_OBJECT (annotate_check), "toggled",
	                  G_CALLBACK (set_widget_sensitive),
	                  log_view);
}

static void
git_create_tag_pane_finalize (GObject *object)
{
	GitCreateTagPane *self;

	self = GIT_CREATE_TAG_PANE (object);

	g_object_unref (self->priv->builder);
	g_free (self->priv);

	G_OBJECT_CLASS (git_create_tag_pane_parent_class)->finalize (object);
}

static GtkWidget *
git_create_tag_pane_get_widget (AnjutaDockPane *pane)
{
	GitCreateTagPane *self;

	self = GIT_CREATE_TAG_PANE (pane);

	return GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                           "create_tag_pane"));
}

static void
git_create_tag_pane_class_init (GitCreateTagPaneClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaDockPaneClass *pane_class = ANJUTA_DOCK_PANE_CLASS (klass);

	object_class->finalize = git_create_tag_pane_finalize;
	pane_class->get_widget = git_create_tag_pane_get_widget;
	pane_class->refresh = NULL;
}


AnjutaDockPane *
git_create_tag_pane_new (Git *plugin)
{
	return g_object_new (GIT_TYPE_CREATE_TAG_PANE, "plugin", plugin, NULL);
}

void
on_create_tag_button_clicked (GtkAction *action, Git *plugin)
{
	AnjutaDockPane *pane;

	pane = git_create_tag_pane_new (plugin);

	anjuta_dock_add_pane (ANJUTA_DOCK (plugin->dock), "CreateTag", 
	                      _("Create Tag"), NULL, pane, GDL_DOCK_BOTTOM, NULL, 0,
	                      NULL);
}