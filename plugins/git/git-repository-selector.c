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

#include "git-repository-selector.h"

struct _GitRepositorySelectorPriv
{
	GtkWidget *remote_toggle;
	GtkWidget *url_toggle;
	GtkWidget *notebook;
	GtkWidget *selected_remote_label;
	GtkWidget *url_entry;
	GitRepositorySelectorMode mode;
	gchar *remote;
};

G_DEFINE_TYPE (GitRepositorySelector, git_repository_selector, GTK_TYPE_VBOX);

static void
on_mode_button_toggled (GtkToggleButton *button, GitRepositorySelector *self)
{
	GitRepositorySelectorMode mode;

	if (gtk_toggle_button_get_active (button))
	{
		/* Each mode corresponds to a page in the notebook */
		mode = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "mode"));
		self->priv->mode = mode;

		gtk_notebook_set_current_page (GTK_NOTEBOOK (self->priv->notebook), 
		                               mode);
	}
}

static void
git_repository_selector_init (GitRepositorySelector *self)
{
	GtkWidget *button_hbox;
	GtkWidget *remote_hbox;
	GtkWidget *label;

	self->priv = g_new0 (GitRepositorySelectorPriv, 1);

	/* Mode selector buttons. Allows the user to use a selected remote or 
	 * enter a URL. */
	button_hbox = gtk_hbox_new (TRUE, 0);

	/* Remote toggle button */
	self->priv->remote_toggle = gtk_radio_button_new_with_label (NULL, 
	                                                             _("Remote"));
	g_object_set (G_OBJECT (self->priv->remote_toggle), "draw-indicator", FALSE,
	              NULL);
	gtk_box_pack_start (GTK_BOX (button_hbox), self->priv->remote_toggle, TRUE,
	                    TRUE, 0);

	g_object_set_data (G_OBJECT (self->priv->remote_toggle), "mode", 
	                   GINT_TO_POINTER (GIT_REPOSITORY_SELECTOR_REMOTE));

	g_signal_connect (G_OBJECT (self->priv->remote_toggle), "toggled",
	                  G_CALLBACK (on_mode_button_toggled),
	                  self);

	/* URL toggle button */
	self->priv->url_toggle = gtk_radio_button_new_with_label (gtk_radio_button_get_group (GTK_RADIO_BUTTON (self->priv->remote_toggle)),
	                                                          _("URL"));
	g_object_set (G_OBJECT (self->priv->url_toggle), "draw-indicator", FALSE,
	              NULL);
	gtk_box_pack_start (GTK_BOX (button_hbox), self->priv->url_toggle, TRUE, 
	                    TRUE, 0);

	g_object_set_data (G_OBJECT (self->priv->url_toggle), "mode", 
	                   GINT_TO_POINTER (GIT_REPOSITORY_SELECTOR_URL));

	g_signal_connect (G_OBJECT (self->priv->url_toggle), "toggled",
	                  G_CALLBACK (on_mode_button_toggled),
	                  self);

	gtk_box_pack_start (GTK_BOX (self), button_hbox, FALSE, FALSE, 0);

	/* Selected remote label */
	remote_hbox = gtk_hbox_new (FALSE, 2);

	label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label), _("<b>Selected Remote:</b>"));
	gtk_box_pack_start (GTK_BOX (remote_hbox), label, FALSE, FALSE, 0);

	self->priv->selected_remote_label = gtk_label_new (NULL);
	g_object_set (G_OBJECT (self->priv->selected_remote_label), "xalign", 
	              0.0f, NULL);
	gtk_box_pack_start (GTK_BOX (remote_hbox), 
	                    self->priv->selected_remote_label, TRUE, TRUE, 0);

	/* URL entry */
	self->priv->url_entry = gtk_entry_new ();

	/* Notebook */
	self->priv->notebook = gtk_notebook_new ();
	gtk_notebook_set_show_border (GTK_NOTEBOOK (self->priv->notebook), FALSE);
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (self->priv->notebook), FALSE);
	gtk_notebook_append_page (GTK_NOTEBOOK (self->priv->notebook), remote_hbox,
	                          NULL);
	gtk_notebook_append_page (GTK_NOTEBOOK (self->priv->notebook), 
	                          self->priv->url_entry, NULL);

	gtk_box_pack_start (GTK_BOX (self), self->priv->notebook, TRUE, TRUE, 0);

	/* Set the selected repository label to a resonable default. */
	git_repository_selector_set_remote (self, NULL);

	gtk_widget_show_all (GTK_WIDGET (self));
}

static void
git_repository_selector_finalize (GObject *object)
{
	GitRepositorySelector *self;

	self = GIT_REPOSITORY_SELECTOR (object);

	g_free (self->priv->remote);
	g_free (self->priv);

	G_OBJECT_CLASS (git_repository_selector_parent_class)->finalize (object);
}

static void
git_repository_selector_class_init (GitRepositorySelectorClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);;

	object_class->finalize = git_repository_selector_finalize;
}


GtkWidget *
git_repository_selector_new (void)
{
	return g_object_new (GIT_TYPE_REPOSITORY_SELECTOR, NULL);
}

GitRepositorySelectorMode
git_repository_selector_get_mode (GitRepositorySelector *self)
{
	return self->priv->mode;
}

void
git_repository_selector_set_remote (GitRepositorySelector *self,
                                    const gchar *remote)
{
	g_free (self->priv->remote);
	self->priv->remote = NULL;

	if (remote)
	{
		self->priv->remote = g_strdup (remote);
		gtk_label_set_text (GTK_LABEL (self->priv->selected_remote_label),
		                    remote);
	}
	else
	{
		gtk_label_set_text (GTK_LABEL (self->priv->selected_remote_label),
		                    _("No remote selected; using origin by default.\n"
		                      "To push to a different remote, select one from "
		                      "the Remotes list above."));
	}
}

gchar *
git_repository_selector_get_repository (GitRepositorySelector *self)
{
	if (self->priv->mode == GIT_REPOSITORY_SELECTOR_REMOTE)
	{
		if (self->priv->remote)
			return g_strdup (self->priv->remote);
		else
			return g_strdup ("origin");
	}
	else
	{
		return gtk_editable_get_chars (GTK_EDITABLE (self->priv->url_entry), 0, 
		                               -1);
	}
		
}
