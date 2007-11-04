/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gprof-flat-profile-view.c
 * Copyright (C) James Liggett 2006 <jrliggett@cox.net>
 * 
 * gprof-flat-profile-view.c is free software.
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

#include "gprof-flat-profile-view.h"
#include <glib/gi18n-lib.h>

struct _GProfFlatProfileViewPriv
{
	GladeXML *gxml;
	GtkListStore *list_store;
};

enum
{
	COL_NAME = 0,
	COL_TIME_PERC,
	COL_CUM_SEC,
	COL_SELF_SEC,
	COL_CALLS,
	COL_AVG_MS,
	COL_TOTAL_MS,
	NUM_COLS
};

static void
gprof_flat_profile_view_create_columns (GProfFlatProfileView *self)
{
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
	GtkWidget *list_view;
	
	list_view = glade_xml_get_widget (self->priv->gxml, "flat_profile_view");
	
	/* Function name */
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, _("Function Name"));
	gtk_tree_view_append_column (GTK_TREE_VIEW (list_view), col);
	
	
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", COL_NAME);
	
	/* Function time percentage */
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, _("% Time"));
	gtk_tree_view_append_column (GTK_TREE_VIEW (list_view), col);
	
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", COL_TIME_PERC);	
	
	/* Cumulative seconds */
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, _("Cumulative Seconds"));
	gtk_tree_view_append_column (GTK_TREE_VIEW (list_view), col);
	
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", COL_CUM_SEC);	
	
	/* Self seconds */
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, _("Self Seconds"));
	gtk_tree_view_append_column (GTK_TREE_VIEW (list_view), col);
	
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", COL_SELF_SEC);
	
	/* Calls */
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, _("Calls"));
	gtk_tree_view_append_column (GTK_TREE_VIEW (list_view), col);
	
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", COL_CALLS);
	
	/* Self ms/call */
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, _("Self ms/call"));
	gtk_tree_view_append_column (GTK_TREE_VIEW (list_view), col);
	
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", COL_AVG_MS);
	
	/* Total ms/call */
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, _("Total ms/call"));
	gtk_tree_view_append_column (GTK_TREE_VIEW (list_view), col);
	
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", COL_TOTAL_MS);
	
	/* Model setup */
	gtk_tree_view_set_model (GTK_TREE_VIEW (list_view), 
							 GTK_TREE_MODEL (self->priv->list_store));
	g_object_unref (self->priv->list_store);
	
}

static void 
on_list_view_row_activated (GtkTreeView *list_view,
							GtkTreePath *path,
							GtkTreeViewColumn *col,												 
							gpointer user_data)
{
	GProfView *self;
	GtkTreeIter list_iter;
	GtkTreeModel *model;
	gchar *selected_function_name;
	
	self = GPROF_VIEW (user_data);
	model = gtk_tree_view_get_model (list_view);
	
	if (gtk_tree_model_get_iter (model, &list_iter, path))
	{
		gtk_tree_model_get (model,
							&list_iter, COL_NAME, 
							&selected_function_name, -1);
		
		gprof_view_show_symbol_in_editor (self, selected_function_name);
		
		g_free (selected_function_name);
	}	
}

static void
gprof_flat_profile_view_init (GProfFlatProfileView *self)
{
	GtkWidget *list_view;
	
	self->priv = g_new0 (GProfFlatProfileViewPriv, 1);
	
	self->priv->gxml = glade_xml_new (PACKAGE_DATA_DIR
									  "/glade/profiler-flat-profile.glade",  
									  NULL, NULL);
	self->priv->list_store = gtk_list_store_new (NUM_COLS, G_TYPE_STRING, 
												 G_TYPE_FLOAT, G_TYPE_FLOAT,
												 G_TYPE_FLOAT, G_TYPE_UINT,
												 G_TYPE_FLOAT, G_TYPE_FLOAT);
	
	gprof_flat_profile_view_create_columns (self);
	
	list_view = glade_xml_get_widget (self->priv->gxml, "flat_profile_view");
	
	g_signal_connect (list_view, "row-activated", 
					  G_CALLBACK (on_list_view_row_activated), 
					  (gpointer) self);
	
}

static void
gprof_flat_profile_view_finalize (GObject *obj)
{
	GProfFlatProfileView *self;
	
	self = (GProfFlatProfileView *) obj;
	
	g_object_unref (self->priv->gxml);
	g_free (self->priv);
}

static void
gprof_flat_profile_view_class_init (GProfFlatProfileViewClass *klass)
{
	GObjectClass *object_class;
	GProfViewClass *view_class;
	
	object_class = (GObjectClass *) klass;
	view_class = GPROF_VIEW_CLASS (klass);
	
	object_class->finalize = gprof_flat_profile_view_finalize;
	view_class->refresh = gprof_flat_profile_view_refresh;
	view_class->get_widget = gprof_flat_profile_view_get_widget;
}

GType
gprof_flat_profile_view_get_type (void)
{
	static GType obj_type = 0;
	
	if (!obj_type)
	{
		static const GTypeInfo obj_info = 
		{
			sizeof (GProfFlatProfileViewClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gprof_flat_profile_view_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,           /* class_data */
			sizeof (GProfFlatProfileView),
			0,              /* n_preallocs */
			(GInstanceInitFunc) gprof_flat_profile_view_init,
			NULL            /* value_table */
		};
		obj_type = g_type_register_static (GPROF_VIEW_TYPE,
		                                   "GProfFlatProfileView", &obj_info, 0);
	}
	return obj_type;
}

GProfFlatProfileView *
gprof_flat_profile_view_new (GProfProfileData *profile_data,
							 IAnjutaSymbolManager *symbol_manager,
							 IAnjutaDocumentManager *document_manager)
{
	GProfFlatProfileView *view;
	
	view = g_object_new (GPROF_FLAT_PROFILE_VIEW_TYPE, NULL);
	gprof_view_set_data (GPROF_VIEW (view), profile_data);
	gprof_view_set_symbol_manager (GPROF_VIEW (view), symbol_manager);
	gprof_view_set_document_manager (GPROF_VIEW (view), document_manager);
	
	return view;
}

void 
gprof_flat_profile_view_refresh (GProfView *view)
{
	GProfFlatProfileView *self;
	GProfProfileData *data;
	GProfFlatProfile *flat_profile;
	GProfFlatProfileEntry *current_entry;
	GList *entry_iter;
	GtkWidget *list_view;
	GtkTreeIter view_iter;
	
	self = GPROF_FLAT_PROFILE_VIEW (view);
	list_view = glade_xml_get_widget (self->priv->gxml, "flat_profile_view");
	
	g_object_ref (self->priv->list_store);
	gtk_tree_view_set_model (GTK_TREE_VIEW (list_view), NULL);
	gtk_list_store_clear (self->priv->list_store);
	
	data = gprof_view_get_data (view);
	flat_profile = gprof_profile_data_get_flat_profile (data);
	current_entry = gprof_flat_profile_get_first_entry (flat_profile, 
														&entry_iter);
	
	while (current_entry)
	{	
		gtk_list_store_append (self->priv->list_store, &view_iter);
		gtk_list_store_set (self->priv->list_store, &view_iter,
							COL_NAME, 
							gprof_flat_profile_entry_get_name (current_entry),
							COL_TIME_PERC,
							gprof_flat_profile_entry_get_time_perc (current_entry),
							COL_CUM_SEC,
							gprof_flat_profile_entry_get_cum_sec (current_entry),
							COL_SELF_SEC,
							gprof_flat_profile_entry_get_self_sec (current_entry),
							COL_CALLS,
							gprof_flat_profile_entry_get_calls (current_entry),
							COL_AVG_MS,
							gprof_flat_profile_entry_get_avg_ms (current_entry),
							COL_TOTAL_MS,
							gprof_flat_profile_entry_get_total_ms (current_entry),
							-1);
		
		current_entry = gprof_flat_profile_entry_get_next (entry_iter, 
														   &entry_iter);
	}
	
	gtk_tree_view_set_model (GTK_TREE_VIEW (list_view), 
							 GTK_TREE_MODEL (self->priv->list_store));
	g_object_unref (self->priv->list_store);
	
}

GtkWidget *
gprof_flat_profile_view_get_widget (GProfView *view)
{
	GProfFlatProfileView *self;
	
	self = GPROF_FLAT_PROFILE_VIEW (view);
	
	return glade_xml_get_widget (self->priv->gxml, "flat_profile_scrolled");
}
