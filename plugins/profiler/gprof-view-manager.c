/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gprof-view-manager.c
 * Copyright (C) James Liggett 2006 <jrliggett@cox.net>
 * 
 * gprof-view-manager.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2, or (at your option) any later version.
 * 
 * plugin.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with plugin.c.  See the file "COPYING".  If not,
 * write to:  The Free Software Foundation, Inc.,
 *            59 Temple Place - Suite 330,
 *            Boston,  MA  02111-1307, USA.
 */

#include "gprof-view-manager.h"

struct _GProfViewManagerPriv
{
	GtkWidget *notebook;
	GList *views;
};

static void 
gprof_view_manager_init (GProfViewManager *self)
{
	self->priv = g_new0 (GProfViewManagerPriv, 1);
	
	self->priv->notebook = gtk_notebook_new ();
	self->priv->views = NULL;
}

static void 
gprof_view_manager_finalize (GObject *obj)
{
	GProfViewManager *self;
	GList *current;
	
	self = (GProfViewManager *) obj;
	current = self->priv->views;
	
	while (current)
	{
		g_object_unref (current->data);
		current = g_list_next (current);
	}
	
	g_list_free (self->priv->views);
	
	/* Don't destroy notebook widget--will be destroyed with container */
	
	g_free (self->priv);
}

static void
gprof_view_manager_class_init (GProfViewManagerClass *klass)
{
	GObjectClass *object_class;
	
	object_class = (GObjectClass *) klass;
	object_class->finalize = gprof_view_manager_finalize;
}

GType 
gprof_view_manager_get_type ()
{
	static GType obj_type = 0;
	
	if (!obj_type)
	{
		static const GTypeInfo obj_info = 
		{
			sizeof (GProfViewManagerClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gprof_view_manager_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,           /* class_data */
			sizeof (GProfViewManager),
			0,              /* n_preallocs */
			(GInstanceInitFunc) gprof_view_manager_init,
			NULL            /* value_table */
		};
		obj_type = g_type_register_static (G_TYPE_OBJECT,
		                                   "GProfViewManager", &obj_info, 0);
	}
	return obj_type;
}

GProfViewManager *
gprof_view_manager_new ()
{
	return g_object_new (GPROF_VIEW_MANAGER_TYPE, NULL);	
}

void
gprof_view_manager_free (GProfViewManager *self)
{
	g_object_unref (self);
}

void
gprof_view_manager_add_view (GProfViewManager *self, GProfView *view,
							 const gchar *label)
{
	GtkWidget *new_tab_label;
	GtkWidget *new_view_widget;
	GtkWidget *new_view_parent;
	
	self->priv->views = g_list_append (self->priv->views, view);
	
	new_tab_label = gtk_label_new (label);
	new_view_widget = gprof_view_get_widget (view);
	new_view_parent = gtk_widget_get_parent (new_view_widget);
	g_object_ref (new_view_widget);
	
	if (new_view_parent)
		gtk_container_remove (GTK_CONTAINER (new_view_parent), new_view_widget);
	
	gtk_notebook_append_page (GTK_NOTEBOOK (self->priv->notebook), new_view_widget, 
							  new_tab_label);
	
	g_object_unref (new_view_widget); 
	
}

void
gprof_view_manager_refresh_views (GProfViewManager *self)
{
	GList *current;
	GProfView *view;
	
	current = self->priv->views;
	
	while (current)
	{
		view = GPROF_VIEW (current->data);
		gprof_view_refresh (view);
	
		current = g_list_next (current);
	}
}

GtkWidget *
gprof_view_manager_get_notebook (GProfViewManager *self)
{
	return self->priv->notebook;
}
