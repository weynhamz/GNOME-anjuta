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

#include "git-checkout-pane.h"

struct _GitCheckoutPanePriv
{
	GtkBuilder *builder;
};

G_DEFINE_TYPE (GitCheckoutPane, git_checkout_pane, GIT_TYPE_PANE);

static void
on_ok_button_clicked (GtkButton *button, GitCheckoutPane *self)
{
	Git *plugin;
	GtkToggleButton *force_check;
	GList *paths;
	GitCheckoutFilesCommand *checkout_command;

	plugin = ANJUTA_PLUGIN_GIT (anjuta_dock_pane_get_plugin (ANJUTA_DOCK_PANE (self)));
	force_check = GTK_TOGGLE_BUTTON (gtk_builder_get_object (self->priv->builder,
	                                                         "force_check"));

	paths = git_status_pane_get_selected_not_updated_items (GIT_STATUS_PANE (plugin->status_pane),
	                                                        ANJUTA_VCS_STATUS_ALL);
	checkout_command = git_checkout_files_command_new (plugin->project_root_directory,
	                                                   paths,
	                                                   gtk_toggle_button_get_active (force_check));

	anjuta_util_glist_strings_free (paths);

	g_signal_connect (G_OBJECT (checkout_command), "command-finished",
	                  G_CALLBACK (git_pane_report_errors),
	                  plugin);

	g_signal_connect (G_OBJECT (checkout_command), "command-finished",
	                  G_CALLBACK (g_object_unref),
	                  NULL);

	anjuta_command_start (ANJUTA_COMMAND (checkout_command));

	git_pane_remove_from_dock (GIT_PANE (self));
	                            
}

static void
git_checkout_pane_init (GitCheckoutPane *self)
{
	gchar *objects[] = {"checkout_pane",
						NULL};
	GError *error = NULL;
	GtkButton *ok_button;
	GtkButton *cancel_button;

	self->priv = g_new0 (GitCheckoutPanePriv, 1);
	self->priv->builder = gtk_builder_new ();

	if (!gtk_builder_add_objects_from_file (self->priv->builder, BUILDER_FILE, 
	                                        objects, 
	                                        &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}

	ok_button = GTK_BUTTON (gtk_builder_get_object (self->priv->builder, 
	                                                "ok_button"));
	cancel_button = GTK_BUTTON (gtk_builder_get_object (self->priv->builder,
	                                                    "cancel_button"));

	g_signal_connect (G_OBJECT (ok_button), "clicked",
	                  G_CALLBACK (on_ok_button_clicked),
	                  self);

	g_signal_connect_swapped (G_OBJECT (cancel_button), "clicked",
	                          G_CALLBACK (git_pane_remove_from_dock),
	                          self);
}

static void
git_checkout_pane_finalize (GObject *object)
{
	GitCheckoutPane *self;

	self = GIT_CHECKOUT_PANE (object);

	g_object_unref (self->priv->builder);
	g_free (self->priv);

	G_OBJECT_CLASS (git_checkout_pane_parent_class)->finalize (object);
}

static GtkWidget *
get_widget (AnjutaDockPane *pane)
{
	GitCheckoutPane *self;

	self = GIT_CHECKOUT_PANE (pane);

	return GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                           "checkout_pane"));
}

static void
git_checkout_pane_class_init (GitCheckoutPaneClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaDockPaneClass* pane_class = ANJUTA_DOCK_PANE_CLASS (klass);

	object_class->finalize = git_checkout_pane_finalize;
	pane_class->get_widget = get_widget;
	pane_class->refresh = NULL;
}


AnjutaDockPane *
git_checkout_pane_new (Git *plugin)
{
	return g_object_new (GIT_TYPE_CHECKOUT_PANE, "plugin", plugin, NULL);
}

void
on_checkout_button_clicked (GtkAction *action, Git *plugin)
{
	AnjutaDockPane *checkout_pane;

	checkout_pane = git_checkout_pane_new (plugin);

	anjuta_dock_add_pane (ANJUTA_DOCK (plugin->dock), "Checkout",
	                      _("Check Out Files"), NULL, checkout_pane,
	                      GDL_DOCK_BOTTOM, NULL, 0, NULL);
}
