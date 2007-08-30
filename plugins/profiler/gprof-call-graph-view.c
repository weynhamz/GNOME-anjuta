/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gprof-call-graph-view.c
 * Copyright (C) James Liggett 2006 <jrliggett@cox.net>
 * 
 * gprof-call-graph-view.c is free software.
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
 *            51 Franklin Street, Fifth Floor,
 *            Boston,  MA  02110-1301, USA.
 */

#include "gprof-call-graph-view.h"

struct _GProfCallGraphViewPriv
{
	GladeXML *gxml;
	GtkListStore *functions_list_store;
	GtkListStore *called_list_store;
	GtkListStore *called_by_list_store;
	GHashTable *functions_iter_table; /* Map functions names to list iters */
};

/* Function list columsns */
enum
{
	FUNCTIONS_COL_RECURSIVE = 0,
	FUNCTIONS_COL_NAME,
	FUNCTIONS_COL_TIME,
	FUNCTIONS_COL_SELF,
	FUNCTIONS_COL_CHILDREN,
	FUNCTIONS_COL_CALLS,
	FUNCTIONS_NUM_COLS
};

/* Called list columsns  (also used by the Called By list) */
enum
{
	CALLED_COL_RECURSIVE = 0,
	CALLED_COL_NAME,
	CALLED_COL_SELF,
	CALLED_COL_CHILDREN,
	CALLED_COL_CALLS,
	CALLED_NUM_COLS
};

/* Add an item to Called/Called by lists */
static void gprof_call_graph_view_add_list_item (GProfCallGraphView *self,
												 GtkListStore *store, 
												 GProfCallGraphBlockEntry *entry,
						   						 GtkTreeIter *iter)
{
	GProfProfileData *data;
	GProfCallGraph *call_graph;
	GProfCallGraphBlock *block;
	
	data = gprof_view_get_data (GPROF_VIEW (self));
	call_graph = gprof_profile_data_get_call_graph (data);
	
	gtk_list_store_append (store, iter);
	gtk_list_store_set (store, iter, CALLED_COL_NAME,
						gprof_call_graph_block_entry_get_name (entry),
						CALLED_COL_SELF,
						gprof_call_graph_block_entry_get_self_sec (entry),
						CALLED_COL_CHILDREN,
						gprof_call_graph_block_entry_get_child_sec (entry),
						CALLED_COL_CALLS,
						gprof_call_graph_block_entry_get_calls (entry),	
						-1);
	
	block = gprof_call_graph_find_block (call_graph,
										 gprof_call_graph_block_entry_get_name (entry));
	
	
	if (block)
	{
		if (gprof_call_graph_block_is_recursive (block))
			gtk_list_store_set (store, iter, CALLED_COL_RECURSIVE, 
								GTK_STOCK_REFRESH);
	}
	
}

/* Function selection callback */
static gboolean
on_function_selected (GtkTreeSelection *selection, GtkTreeModel *model, 
					  GtkTreePath *path, gboolean path_currently_selected,
					  gpointer user_data)
{
	GProfCallGraphView *self;
	GProfProfileData *data;
	GProfCallGraph *call_graph;
	GProfCallGraphBlock *block;
	GProfCallGraphBlockEntry *current_entry;
	GtkWidget *called_list_view;
	GtkWidget *called_by_list_view;
	GList *entry_iter;
	GtkTreeIter list_iter;
	gchar *selected_function_name;
	
	self = GPROF_CALL_GRAPH_VIEW (user_data);
	data = gprof_view_get_data (GPROF_VIEW (self));
	call_graph = gprof_profile_data_get_call_graph (data);
	called_list_view = glade_xml_get_widget (self->priv->gxml, 
											 "called_list_view");
	called_by_list_view = glade_xml_get_widget (self->priv->gxml,
												"called_by_list_view");
	
	gtk_tree_model_get_iter (model, &list_iter, path);
	gtk_tree_model_get (model, &list_iter, FUNCTIONS_COL_NAME, 
						&selected_function_name, -1);
	
	if (selected_function_name)
	{
		g_object_ref (self->priv->called_list_store);
		g_object_ref (self->priv->called_by_list_store);
		
		gtk_tree_view_set_model (GTK_TREE_VIEW (called_list_view), NULL);
		gtk_tree_view_set_model (GTK_TREE_VIEW (called_by_list_view), NULL);
		gtk_list_store_clear (self->priv->called_list_store);
		gtk_list_store_clear (self->priv->called_by_list_store);
		
		block = gprof_call_graph_find_block (call_graph, 
											 selected_function_name);
		current_entry = gprof_call_graph_block_get_first_child (block, 
																&entry_iter);
		
		g_free (selected_function_name);
		
		while (current_entry)
		{
			gprof_call_graph_view_add_list_item (self, 
												 self->priv->called_list_store, 
												 current_entry, &list_iter);
			
			current_entry = gprof_call_graph_block_entry_get_next (entry_iter, 
																   &entry_iter);
		}
		
		current_entry = gprof_call_graph_block_get_first_parent (block, 
																 &entry_iter);
		
		while (current_entry)
		{
			gprof_call_graph_view_add_list_item (self, 
												 self->priv->called_by_list_store, 
												 current_entry, &list_iter);
			
			current_entry = gprof_call_graph_block_entry_get_next (entry_iter, 
																   &entry_iter);
		}
		
		gtk_tree_view_set_model (GTK_TREE_VIEW (called_list_view), 
								 GTK_TREE_MODEL (self->priv->called_list_store));
		gtk_tree_view_set_model (GTK_TREE_VIEW (called_by_list_view), 
								 GTK_TREE_MODEL (self->priv->called_by_list_store));
		g_object_unref (self->priv->called_list_store);
		g_object_unref (self->priv->called_by_list_store);
		
		/* Set up search colums again */
		gtk_tree_view_set_search_column (GTK_TREE_VIEW (called_list_view),
										 CALLED_COL_NAME);
		gtk_tree_view_set_search_column (GTK_TREE_VIEW (called_by_list_view),
										 CALLED_COL_NAME);
	}
	
	return TRUE;
}

/* Select a function in the Functions list by name */
static void
gprof_call_graph_view_select_function (GProfCallGraphView *self, gchar *name)
{
	GtkWidget *functions_list_view;
	GtkTreeSelection *functions_list_selection;
	GtkTreeIter *functions_list_iter;
	GtkTreePath *functions_list_path;
	
	functions_list_view = glade_xml_get_widget (self->priv->gxml, 
												"functions_list_view");
	functions_list_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (functions_list_view));
	
	functions_list_iter = g_hash_table_lookup (self->priv->functions_iter_table,
											   name);
	
	if (functions_list_iter)
	{
		gtk_tree_selection_select_iter (functions_list_selection, 
										functions_list_iter);
		functions_list_path = gtk_tree_model_get_path (GTK_TREE_MODEL (self->priv->functions_list_store),
								 					   functions_list_iter);
		gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (functions_list_view),
									  functions_list_path, NULL, TRUE, 0.5, 
									  0.0);

		gtk_tree_path_free (functions_list_path);		
		
	}
}

static void 
on_functions_list_view_row_activated (GtkTreeView *list_view,
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
							&list_iter, CALLED_COL_NAME, 
							&selected_function_name, -1);
		
		gprof_view_show_symbol_in_editor (self, selected_function_name);
		
		g_free (selected_function_name);
	}	
}

/* Called/Called By list row activation callbacks */
static void on_called_list_view_row_activated (GtkTreeView *list_view,
											   GtkTreePath *path,
											   GtkTreeViewColumn *col,												 
											   gpointer user_data)
{
	GProfCallGraphView *self;
	GtkTreeIter list_iter;
	GtkTreeModel *model;
	gchar *selected_function_name;
	
	self = GPROF_CALL_GRAPH_VIEW (user_data);
	model = gtk_tree_view_get_model (list_view);
	
	if (gtk_tree_model_get_iter (model, &list_iter, path))
	{
		gtk_tree_model_get (model,
							&list_iter, CALLED_COL_NAME, 
							&selected_function_name, -1);
		
		gprof_call_graph_view_select_function (self, selected_function_name);
		
		g_free (selected_function_name);
	}	
}

static void
gprof_call_graph_view_create_columns (GProfCallGraphView *self)
{
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
	GtkWidget *functions_list_view;
	GtkWidget *called_list_view;
	GtkWidget *called_by_list_view;
	
	functions_list_view = glade_xml_get_widget (self->priv->gxml, 
												"functions_list_view");
	called_list_view = glade_xml_get_widget (self->priv->gxml,
											 "called_list_view");
	called_by_list_view = glade_xml_get_widget (self->priv->gxml,
												"called_by_list_view");
	
	/* The Functions list will have all fields; all others have everything 
	 * except a time field. */
	
	/* Recursive icon */
	col = gtk_tree_view_column_new ();
	gtk_tree_view_append_column (GTK_TREE_VIEW (functions_list_view), col);
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "stock-id", 
										FUNCTIONS_COL_RECURSIVE);
	
	col = gtk_tree_view_column_new ();
	gtk_tree_view_append_column (GTK_TREE_VIEW (called_list_view), col);
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "stock-id", 
										CALLED_COL_RECURSIVE);
	
	col = gtk_tree_view_column_new ();
	gtk_tree_view_append_column (GTK_TREE_VIEW (called_by_list_view), col);
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "stock-id", 
										CALLED_COL_RECURSIVE);

	/* Function Name */
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, "Function Name");
	gtk_tree_view_append_column (GTK_TREE_VIEW (functions_list_view), col);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", 
										FUNCTIONS_COL_NAME);
	
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, "Function Name");
	gtk_tree_view_append_column (GTK_TREE_VIEW (called_list_view), col);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", 
										CALLED_COL_NAME);
	
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, "Function Name");
	gtk_tree_view_append_column (GTK_TREE_VIEW (called_by_list_view), col);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", 
										CALLED_COL_NAME);

										
	/* Time */
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, "Time");
	gtk_tree_view_append_column (GTK_TREE_VIEW (functions_list_view), col);
	
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", 
										FUNCTIONS_COL_TIME);
										
	/* Self */
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, "Self");
	gtk_tree_view_append_column (GTK_TREE_VIEW (functions_list_view), col);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", 
										FUNCTIONS_COL_SELF);
	
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, "Self");
	gtk_tree_view_append_column (GTK_TREE_VIEW (called_list_view), col);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", 
										CALLED_COL_SELF);
	
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, "Self");
	gtk_tree_view_append_column (GTK_TREE_VIEW (called_by_list_view), col);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", 
										CALLED_COL_SELF);
										
	/* Children */
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, "Children");
	gtk_tree_view_append_column (GTK_TREE_VIEW (functions_list_view), col);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", 
										FUNCTIONS_COL_CHILDREN);
	
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, "Children");
	gtk_tree_view_append_column (GTK_TREE_VIEW (called_list_view), col);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", 
										CALLED_COL_CHILDREN);
	
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, "Children");
	gtk_tree_view_append_column (GTK_TREE_VIEW (called_by_list_view), col);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", 
										CALLED_COL_CHILDREN);
										
	/* Calls */
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, "Calls");
	gtk_tree_view_append_column (GTK_TREE_VIEW (functions_list_view), col);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", 
										FUNCTIONS_COL_CALLS);
	
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, "Calls");
	gtk_tree_view_append_column (GTK_TREE_VIEW (called_list_view), col);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", 
										CALLED_COL_CALLS);
	
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, "Calls");
	gtk_tree_view_append_column (GTK_TREE_VIEW (called_by_list_view), col);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", 
										CALLED_COL_CALLS);
										
	/* Model setup */
	gtk_tree_view_set_model (GTK_TREE_VIEW (functions_list_view),
							 GTK_TREE_MODEL (self->priv->functions_list_store));
	g_object_unref (self->priv->functions_list_store);
	
	gtk_tree_view_set_model (GTK_TREE_VIEW (called_list_view),
							 GTK_TREE_MODEL (self->priv->called_list_store));
	g_object_unref (self->priv->called_list_store);
	
	gtk_tree_view_set_model (GTK_TREE_VIEW (called_by_list_view),
							 GTK_TREE_MODEL (self->priv->called_by_list_store));
	g_object_unref (self->priv->called_by_list_store);
	
	
}

static void
gprof_call_graph_view_init (GProfCallGraphView *self)
{
	GtkWidget *functions_list_view;
	GtkWidget *called_list_view;
	GtkWidget *called_by_list_view;
	GtkWidget *called_jump_to_button;
	GtkWidget *called_by_jump_to_button;	
	GtkTreeSelection *functions_list_selection;
	
	self->priv = g_new0 (GProfCallGraphViewPriv, 1);
	
	self->priv->gxml = glade_xml_new (PACKAGE_DATA_DIR
									  "/glade/profiler-call-graph.glade",  
									  NULL, NULL);
	self->priv->functions_list_store = gtk_list_store_new (FUNCTIONS_NUM_COLS,
														   G_TYPE_STRING,
														   G_TYPE_STRING,
														   G_TYPE_FLOAT,
														   G_TYPE_FLOAT,
														   G_TYPE_FLOAT,
														   G_TYPE_STRING);
	
	self->priv->called_list_store = gtk_list_store_new (CALLED_NUM_COLS,
														G_TYPE_STRING,
														G_TYPE_STRING,
														G_TYPE_FLOAT,
														G_TYPE_FLOAT,
														G_TYPE_STRING);
	
	self->priv->called_by_list_store = gtk_list_store_new (CALLED_NUM_COLS,
														   G_TYPE_STRING,
														   G_TYPE_STRING,
														   G_TYPE_FLOAT,
														   G_TYPE_FLOAT,
														   G_TYPE_STRING);
														   
	gprof_call_graph_view_create_columns (self);
	
	functions_list_view = glade_xml_get_widget (self->priv->gxml,
												"functions_list_view");
	called_list_view = glade_xml_get_widget (self->priv->gxml,
											 "called_list_view");
	called_by_list_view = glade_xml_get_widget (self->priv->gxml,
												"called_by_list_view");
	
	/* Function selection */
	functions_list_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (functions_list_view));
	gtk_tree_selection_set_select_function (functions_list_selection, 
											on_function_selected, 
											(gpointer) self, NULL);	
											
	/* Jump to button callbacks */
	called_jump_to_button = glade_xml_get_widget (self->priv->gxml, 
												  "called_jump_to_button");
	called_by_jump_to_button = glade_xml_get_widget (self->priv->gxml, 
												  	 "called_by_jump_to_button");
													 
	g_signal_connect (functions_list_view, "row-activated", 
					  G_CALLBACK (on_functions_list_view_row_activated), 
					  (gpointer) self);
	g_signal_connect (called_list_view, "row-activated", 
					  G_CALLBACK (on_called_list_view_row_activated), 
					  (gpointer) self);
	g_signal_connect (called_by_list_view, "row-activated", 
					  G_CALLBACK (on_called_list_view_row_activated), 
					  (gpointer) self);
}

static void
gprof_call_graph_view_finalize (GObject *obj)
{
	GProfCallGraphView *self;
	
	self = (GProfCallGraphView *) obj;
	
	g_object_unref (self->priv->gxml);
	
	if (self->priv->functions_iter_table)
		g_hash_table_destroy (self->priv->functions_iter_table);
	
	g_free (self->priv);
}

static void 
gprof_call_graph_view_class_init (GProfCallGraphViewClass *klass)
{
	GObjectClass *object_class;
	GProfViewClass *view_class;
	
	object_class = (GObjectClass *) klass;
	view_class = GPROF_VIEW_CLASS (klass);
	
	object_class->finalize = gprof_call_graph_view_finalize;
	view_class->refresh = gprof_call_graph_view_refresh;
	view_class->get_widget = gprof_call_graph_view_get_widget;
}

GType
gprof_call_graph_view_get_type (void)
{
	static GType obj_type = 0;
	
	if (!obj_type)
	{
		static const GTypeInfo obj_info = 
		{
			sizeof (GProfCallGraphViewClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gprof_call_graph_view_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,           /* class_data */
			sizeof (GProfCallGraphView),
			0,              /* n_preallocs */
			(GInstanceInitFunc) gprof_call_graph_view_init,
			NULL            /* value_table */
		};
		obj_type = g_type_register_static (GPROF_VIEW_TYPE,
		                                   "GProfCallGraphView", &obj_info, 0);
	}
	return obj_type;
}

GProfCallGraphView *
gprof_call_graph_view_new (GProfProfileData *profile_data,
						   IAnjutaSymbolManager *symbol_manager,
						   IAnjutaDocumentManager *document_manager)
{
	GProfCallGraphView *view;
	
	view = g_object_new (GPROF_CALL_GRAPH_VIEW_TYPE, NULL);
	gprof_view_set_data (GPROF_VIEW (view), profile_data);
	gprof_view_set_symbol_manager (GPROF_VIEW (view), symbol_manager);
	gprof_view_set_document_manager (GPROF_VIEW (view), document_manager);
	
	return view;
}

void
gprof_call_graph_view_refresh (GProfView *view)
{
	GProfCallGraphView *self;
	GtkWidget *functions_list_view;
	GtkWidget *called_list_view;
	GtkWidget *called_by_list_view;
	GProfProfileData *data;
	GProfCallGraph *call_graph;
	GProfCallGraphBlock *current_block;
	GProfCallGraphBlockEntry *primary_entry;
	GList *block_iter;
	GtkTreeIter list_iter;
	
	self = GPROF_CALL_GRAPH_VIEW (view);
	data = gprof_view_get_data (view);
	call_graph = gprof_profile_data_get_call_graph (data);
	
	functions_list_view = glade_xml_get_widget (self->priv->gxml, 
												"functions_list_view");
	called_list_view = glade_xml_get_widget (self->priv->gxml,
											 "called_list_view");
	called_by_list_view = glade_xml_get_widget (self->priv->gxml,
												"called_by_list_view");
	
	/* Clear all lists and repopulate the functions list. The two others won't
	 * be repopulated until a user selects a function from the Functions list */
	g_object_ref (self->priv->functions_list_store);
	g_object_ref (self->priv->called_list_store);
	g_object_ref (self->priv->called_by_list_store);
	
	gtk_tree_view_set_model (GTK_TREE_VIEW (functions_list_view), NULL);
	gtk_tree_view_set_model (GTK_TREE_VIEW (called_list_view), NULL);
	gtk_tree_view_set_model (GTK_TREE_VIEW (called_by_list_view), NULL);
	gtk_list_store_clear (self->priv->functions_list_store);
	gtk_list_store_clear (self->priv->called_list_store);
	gtk_list_store_clear (self->priv->called_by_list_store);
	
	gtk_tree_view_set_model (GTK_TREE_VIEW (called_list_view),
							 GTK_TREE_MODEL (self->priv->called_list_store));
	gtk_tree_view_set_model (GTK_TREE_VIEW (called_by_list_view),
							 GTK_TREE_MODEL (self->priv->called_by_list_store));
									   
	g_object_unref (self->priv->called_list_store);
	g_object_unref (self->priv->called_by_list_store);
	
	current_block = gprof_call_graph_get_first_block (call_graph, &block_iter);
	
	/* Rebuild the Functions iter table. This table maps function names to 
	 * GtkTreeIters in the functions list to be used by the Jump to buttons
	 * next to the Called and Called by lists. When a user selects a function
	 * in either of these lists, that function should be selected in the
	 * functions list. */
	if (self->priv->functions_iter_table)
		g_hash_table_destroy (self->priv->functions_iter_table);
	
	self->priv->functions_iter_table = g_hash_table_new_full (g_str_hash,
															  g_str_equal, NULL,
															  g_free);
	
	while (current_block)
	{
		primary_entry = gprof_call_graph_block_get_primary_entry (current_block);
		
		gtk_list_store_append (self->priv->functions_list_store, &list_iter);
		gtk_list_store_set (self->priv->functions_list_store, &list_iter,
							FUNCTIONS_COL_NAME,
							gprof_call_graph_block_entry_get_name (primary_entry),
							FUNCTIONS_COL_TIME,
							gprof_call_graph_block_entry_get_time_perc (primary_entry),
							FUNCTIONS_COL_SELF,
							gprof_call_graph_block_entry_get_self_sec (primary_entry),
							FUNCTIONS_COL_CHILDREN,
							gprof_call_graph_block_entry_get_child_sec (primary_entry),
							FUNCTIONS_COL_CALLS,
							gprof_call_graph_block_entry_get_calls (primary_entry),
							-1);
		
		if (gprof_call_graph_block_is_recursive (current_block))
		{
			gtk_list_store_set (self->priv->functions_list_store, &list_iter,
								FUNCTIONS_COL_RECURSIVE, 
								GTK_STOCK_REFRESH, -1);
		}
		
		/* Add the current iter to the Functions iter lookup table */
		g_hash_table_insert (self->priv->functions_iter_table,
							 gprof_call_graph_block_entry_get_name (primary_entry),
							 g_memdup (&list_iter, sizeof (GtkTreeIter)));
		
		current_block = gprof_call_graph_block_get_next (block_iter, 
														 &block_iter);
	}

	gtk_tree_view_set_model (GTK_TREE_VIEW (functions_list_view), 
							 GTK_TREE_MODEL (self->priv->functions_list_store));
	
	g_object_unref (self->priv->functions_list_store);

	/* Set search column on Functions list */
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (functions_list_view),
									 FUNCTIONS_COL_NAME);	
}

GtkWidget *
gprof_call_graph_view_get_widget (GProfView *view)
{
	GProfCallGraphView *self;
	
	self = GPROF_CALL_GRAPH_VIEW (view);
	
	return glade_xml_get_widget (self->priv->gxml, "call_graph_vbox");
}
