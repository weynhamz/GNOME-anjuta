/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2009 <jrliggett@cox.net>
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

#include "git-stash-widget.h"

void
git_stash_widget_create (Git *plugin, GtkWidget **stash_widget, 
						 GtkWidget **stash_widget_grip)
{
	gchar *objects[] = {"stash_widget_scrolled_window", 
						"stash_widget_grip_hbox",
						"stash_list_model"};
	GtkBuilder *bxml;
	GError *error;
	GitUIData *data;
	GtkWidget *stash_widget_scrolled_window;
	GtkWidget *stash_widget_grip_hbox;

	bxml = gtk_builder_new ();
	error = NULL;
	data = git_ui_data_new (plugin, bxml);

	if (!gtk_builder_add_objects_from_file (data->bxml, BUILDER_FILE, objects, 
	                                        &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}

	stash_widget_scrolled_window = GTK_WIDGET (gtk_builder_get_object (bxml, 
																	   "stash_widget_scrolled_window"));
	stash_widget_grip_hbox = GTK_WIDGET (gtk_builder_get_object (bxml,
																 "stash_widget_grip_hbox"));

	g_object_set_data_full (G_OBJECT (stash_widget_scrolled_window), "ui-data",
							data, (GDestroyNotify) git_ui_data_free);

	*stash_widget = stash_widget_scrolled_window;
	*stash_widget_grip = stash_widget_grip_hbox;
}
