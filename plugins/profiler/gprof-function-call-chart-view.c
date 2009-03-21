/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gprof-function-call-chart-view.c
 * Copyright (C) James Liggett 2006 <jrliggett@cox.net>
 * 
 * Portions borrowed from the Class Inheritance Graph
 * Copyright (C) Massimo Cora' 2005 
 * 
 * gprof-function-call-chart-view.c is free software.
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

#include "gprof-function-call-chart-view.h"

/* Drawing helper macros */
#define NODE_LOWER_LEFT(node,main_index,rec_index) \
	((point)((field_t*)ND_shape_info (node))->fld[main_index]->fld[rec_index]->b.LL)

#define NODE_UPPER_RIGHT(node,main_index,rec_index) \
	((point)((field_t*)ND_shape_info (node))->fld[main_index]->fld[rec_index]->b.UR)

#define NODE_NUM_FIELDS(node) \
	((field_t*)ND_shape_info (node))->n_flds

#define NODE_NTH_FIELD(node,nth) \
	((field_t*)ND_shape_info (node))->fld[nth]

#define NODE_NTH_TEXT(node,main_index,rec_index) \
	((field_t*)ND_shape_info (node))->fld[main_index]->fld[rec_index]->lp->text

#define INCH_TO_PIXELS(inch_size) \
				inch_size * 72
				
struct _GProfFunctionCallChartViewPriv
{
	GtkWidget *canvas;
	GtkWidget *scrolled_window;
	gdouble canvas_size_x;
	gdouble canvas_size_y;
	GList *graphs; 
	GVC_t *gvc;
	GList *canvas_items;
	gint y_offset;
	
	/* Used for context drawing */
	Agnode_t *current_node;
	GList *current_graph;
}; 

static void
gprof_function_call_chart_view_destroy_graph (GProfFunctionCallChartView *self)
{
	GList *current_graph;
	
	current_graph = self->priv->graphs;
	
	while (current_graph)
	{
		gvFreeLayout (self->priv->gvc, current_graph->data);
		agclose (current_graph->data);
		
		current_graph = g_list_next (current_graph);
	}
	
	if (self->priv->gvc)
		gvFreeContext (self->priv->gvc);
	
	self->priv->graphs = NULL;
	self->priv->gvc = NULL;
}

static void
gprof_function_call_chart_view_add_function (GProfFunctionCallChartView *self,
											 Agraph_t *subgraph,
											 GProfCallGraph *call_graph,
											 GProfCallGraphBlock *block,
											 Agnode_t *parent_node)
{
	Agnode_t *new_node;
	GList *entry_iter;
	GProfCallGraphBlockEntry *current_entry;
	GProfCallGraphBlockEntry *current_child;
	GProfCallGraphBlock *next_block;
	GProfCallGraphBlockEntry *next_block_primary_entry;
	gchar *current_entry_name;
	gchar *next_block_name;
	
	current_entry = gprof_call_graph_block_get_primary_entry (block);
	current_entry_name = gprof_call_graph_block_entry_get_name (current_entry);
	
	new_node = agnode (subgraph, current_entry_name);
	
	/* Make the node box shaped */
	agsafeset (new_node, "shape", "box", "");
	
	/* Add the edge to this node's parent, if it has one */
	if (parent_node)
		agedge (subgraph, parent_node, new_node);
	
	/* For recursive functions, add an edge that loops back to the node */
	if (gprof_call_graph_block_is_recursive (block))
		agedge (subgraph, new_node, new_node);
	
	current_child = gprof_call_graph_block_get_first_child (block, &entry_iter);
	
	while (current_child)
	{
		next_block = gprof_call_graph_find_block (call_graph,
												  gprof_call_graph_block_entry_get_name (current_child));
		current_child = gprof_call_graph_block_entry_get_next (entry_iter, 
															   &entry_iter);
		
		if (next_block)
		{
			next_block_primary_entry = gprof_call_graph_block_get_primary_entry (next_block);
			next_block_name = gprof_call_graph_block_entry_get_name (next_block_primary_entry);
			
			/* Make sure we don't go into an infinite loop on recursive functions */
			if (strcmp(next_block_name, current_entry_name) == 0)
				continue;
			
			gprof_function_call_chart_view_add_function (self, subgraph, 
														 call_graph, 
														 next_block, new_node);	
		}
		
	}
}

static void
gprof_function_call_chart_view_create_graph (GProfFunctionCallChartView *self)
{
	GProfProfileData *profile_data;
	GProfCallGraph *call_graph;
	GProfCallGraphBlock *current_block;
	GProfCallGraphBlockEntry *current_entry;
	gchar *entry_name;
	GList *root_iter;
	Agraph_t *subgraph;
	
	/* If a previous graph exists, delete it and start over */
	gprof_function_call_chart_view_destroy_graph (self);
	
	self->priv->gvc = gvContext ();
	
	profile_data = gprof_view_get_data (GPROF_VIEW (self));
	call_graph = gprof_profile_data_get_call_graph (profile_data);
	
	current_block = gprof_call_graph_get_root (call_graph, &root_iter);
	
	/* If the call graph doesn't have any roots, that usually means the user
	 * is showing only select functions. Recurse from each block so we still
	 * get a graph */
	 if (!current_block)
	 	current_block = gprof_call_graph_get_first_block (call_graph, 
	 													  &root_iter);
	
	while (current_block)
	{
		current_entry = gprof_call_graph_block_get_primary_entry (current_block);
		entry_name = gprof_call_graph_block_entry_get_name (current_entry);
		subgraph = agopen (entry_name, AGDIGRAPHSTRICT);
		self->priv->graphs = g_list_append (self->priv->graphs, subgraph);
		
		gprof_function_call_chart_view_add_function (self, subgraph, 
													 call_graph, 
													 current_block, NULL);
		
		current_block = gprof_call_graph_block_get_next (root_iter, &root_iter);
	}
	
	
}

static void
gprof_function_call_chart_view_clear_canvas (GProfFunctionCallChartView *self)
{
	if (self->priv->canvas_items)
	{
		g_list_foreach (self->priv->canvas_items, (GFunc) gtk_object_destroy,
						NULL);
		g_list_free (self->priv->canvas_items);
		
		self->priv->canvas_items = NULL;
	}
}

static gint
on_node_event (GnomeCanvasItem *item, GdkEvent *event, gpointer data)
{
	GProfFunctionCallChartView *self;
	gchar *function_name;
	
	self = GPROF_FUNCTION_CALL_CHART_VIEW (data);
	
	
	switch (event->type)
	{
		case GDK_ENTER_NOTIFY:
			/* Make the outline wide */
			gnome_canvas_item_set (item,
							   	   "width_units", 2.5,
							   	   "fill_color_gdk",
							   	   &self->priv->canvas->style->base[GTK_STATE_PRELIGHT],
							   	   "outline_color_gdk",
							   	   &self->priv->canvas->style->text[GTK_STATE_PRELIGHT],
							   	   NULL);
		
			return TRUE;
			break;
		case GDK_LEAVE_NOTIFY:
			/* Make the outline thin */
			gnome_canvas_item_set (item,
							   	   "width_units", 1.0,
							   	   "fill_color_gdk",
							   	   &self->priv->canvas->style->base[GTK_STATE_NORMAL],
							   	   "outline_color_gdk",
							   	   &self->priv->canvas->style->text[GTK_STATE_NORMAL],	
							   	   NULL);
			return TRUE;
			break;
		case GDK_2BUTTON_PRESS:
			function_name = (gchar *)g_object_get_data (G_OBJECT (item), 
													    "function-name");
			gprof_view_show_symbol_in_editor (GPROF_VIEW (self), function_name);
			g_free (function_name);
		
			return TRUE;
			break;
		default:
			break;
	}
	
	return FALSE;
}

static void
gprof_function_call_chart_view_draw_node (GProfFunctionCallChartView *self,
										  Agnode_t *node, point *node_pos,
										  gdouble node_width, 
										  gdouble node_height)
{
	GnomeCanvasItem *item;
	gdouble text_width;
	
	/* Node box */
	item = gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (self->priv->canvas)),
						   		  gnome_canvas_rect_get_type (),
						   		  "x1",
						   		  (gdouble) (node_pos->x - INCH_TO_PIXELS (node_width) / 2),
						   		  "y1",
						   	  	  (gdouble) -(node_pos->y - INCH_TO_PIXELS (node_height) / 2),
						   		  "x2",
						   		  (gdouble) (node_pos->x + INCH_TO_PIXELS (node_width) / 2),
						   		  "y2",
						   		  (gdouble) -(node_pos->y + INCH_TO_PIXELS (node_height) / 2),
						   		  "fill_color_gdk",
						   		  &self->priv->canvas->style->base[GTK_STATE_NORMAL],
						   		  "outline_color_gdk",
						   		  &self->priv->canvas->style->text[GTK_STATE_NORMAL],
						   		  "width_units", 
						   		  1.0,
						   		  NULL);
	
	g_object_set_data_full (G_OBJECT (item), "function-name", g_strdup (node->name),
							g_free);
	g_signal_connect (GTK_OBJECT (item), "event", G_CALLBACK (on_node_event),
					  (gpointer) self);
					  
	self->priv->canvas_items = g_list_append (self->priv->canvas_items, item);
						   
	/* Label */
	item = gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (self->priv->canvas)),
								  gnome_canvas_text_get_type (),
								  "text", 
								  node->name,
								  "justification", 
								  GTK_JUSTIFY_CENTER,
								  "anchor", 
								  GTK_ANCHOR_CENTER,
								  "x",
								  (gdouble) (node_pos->x - INCH_TO_PIXELS (node_width) / 2),
								  "y", 
								  (gdouble) -node_pos->y,
								  "fill_color_gdk",
								  &self->priv->canvas->style->text[GTK_STATE_NORMAL],
								  "anchor", 
								  GTK_ANCHOR_W,        
								  NULL);
								  
	/* Center the label in the box */
	g_object_get (item, "text_width", &text_width, NULL);
	gnome_canvas_item_set (item, "x",
						   (gdouble) (node_pos->x - text_width / 2), NULL);
	self->priv->canvas_items = g_list_append (self->priv->canvas_items, item);
	
	/* Check if we've drawn outside the scroll region. If so, expand the 
	 * canvas */
	if (abs(node_pos->x + node_width) > self->priv->canvas_size_x )
	{ 
		self->priv->canvas_size_x = abs(node_pos->x) + 
									INCH_TO_PIXELS (node_width) / 2;
	}
	
	if (abs(node_pos->y + node_height) > self->priv->canvas_size_y) 
	{
		self->priv->canvas_size_y = abs(node_pos->y) + 
									INCH_TO_PIXELS (node_height) / 2;
	}
									
	/* Set up the scrolling region */
	gtk_widget_set_size_request (self->priv->canvas, 
								 self->priv->canvas_size_x + 100,
								 self->priv->canvas_size_y + 100);
	gnome_canvas_set_scroll_region (GNOME_CANVAS (self->priv->canvas), -50, 50, 
									(gdouble) self->priv->canvas_size_x + 50,
									(gdouble) -self->priv->canvas_size_y - 100);
}

static void
gprof_function_call_chart_view_draw_edge (GProfFunctionCallChartView *self,
										  Agedge_t *edge, point *node_pos,
										  gdouble node_height)
{
	GnomeCanvasItem *item;
	GnomeCanvasPathDef *edge_path_def;
	gdouble arrow_upper_bound;
	gdouble arrow_x_offset;
	gdouble arrow_height;
	GnomeCanvasPoints *arrow_points;
	gint i;
	
	edge_path_def = gnome_canvas_path_def_new ();
	
	for (i = 0; i < (ED_spl(edge)->list->size - 1); i += 3)
	{
		gnome_canvas_path_def_moveto (edge_path_def,
									  ((ED_spl(edge))->list->list[i]).x, 
									  -((ED_spl(edge))->list->list[i]).y - 
									  	 self->priv->y_offset);
		gnome_canvas_path_def_curveto (edge_path_def, 
									   ((ED_spl(edge))->list->list[i + 1]).x, 
									   -((ED_spl(edge))->list->list[i + 1]).y -
									     self->priv->y_offset,
									   ((ED_spl(edge))->list->list[i + 2]).x, 
									   -((ED_spl(edge))->list->list[i + 2]).y -
									     self->priv->y_offset,
									   ((ED_spl(edge))->list->list[i + 3]).x, 
									   -((ED_spl(edge))->list->list[i + 3]).y -
									     self->priv->y_offset);
		
		/* Draw the arrow at the end of the edge */
		if (i + 3 >= (ED_spl(edge)->list->size - 1))
		{
			arrow_upper_bound = (gdouble) (node_pos->y  +
										   INCH_TO_PIXELS (node_height) / 2) -
										   self->priv->y_offset;
			arrow_height = abs (((ED_spl(edge))->list->list[i + 3]).y - 
							    arrow_upper_bound);
			
			/* Determine the length (x offset) of the arrowhead.
			 * Arrow x offsets shouldn't be more than 20 units. */
			if ((((ED_spl(edge))->list->list[i + 3]).x -
				((ED_spl(edge))->list->list[i + 2]).x) > 0)
			{
				arrow_x_offset = sqrt( abs(100 - arrow_height * arrow_height));
			}
			else
			{
				arrow_x_offset = -sqrt( abs(100 - arrow_height * arrow_height));
			}
			
			/* Now actually draw the arrow */
			arrow_points = gnome_canvas_points_new (2);
			
			/* Starting point */
			arrow_points->coords[0] = ((ED_spl(edge))->list->list[i + 3]).x;
			arrow_points->coords[1] = -((ED_spl(edge))->list->list[i + 3].y +
										 self->priv->y_offset);
			
			/* Arrow head */
			arrow_points->coords[2] = ((ED_spl(edge))->list->list[i + 3]).x + 
									   arrow_x_offset;
			arrow_points->coords[3] = -(arrow_upper_bound + self->priv->y_offset);
			
			if (abs (arrow_x_offset) <= 20)
			{
				item = gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (self->priv->canvas)),
										   	  gnome_canvas_line_get_type(),
										   	  "points", 
											  arrow_points,
										   	  "fill_color_gdk",
							  				  &self->priv->canvas->style->text[GTK_STATE_NORMAL],
											  "last_arrowhead", 
											  TRUE,
											  "arrow_shape_a", 
											  10.0,
											  "arrow_shape_b", 
											  10.0,
											  "arrow_shape_c", 
											  4.0,
											  "width_units", 
											  1.0,
											  NULL);
				self->priv->canvas_items = g_list_append (self->priv->canvas_items,
														  item);
			}
			
			gnome_canvas_points_free (arrow_points);
			
		}
	}
	
	/* Draw the edge path */
	item = gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (self->priv->canvas)), 
								  gnome_canvas_bpath_get_type(),
								  "bpath", 
								  edge_path_def,
								  "outline_color_gdk",
								  &self->priv->canvas->style->text[GTK_STATE_NORMAL],
								  "width_pixels", 
								  1,
								  NULL);
	self->priv->canvas_items = g_list_append (self->priv->canvas_items, item);
}

static void 
gprof_function_call_chart_view_init (GProfFunctionCallChartView *self)
{
	self->priv = g_new0 (GProfFunctionCallChartViewPriv, 1);
	
	self->priv->canvas = gnome_canvas_new_aa ();
	self->priv->scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (self->priv->
														 scrolled_window),
									GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (self->priv->
																scrolled_window),
					   					   self->priv->canvas);
	gtk_widget_show_all (self->priv->scrolled_window);
	
	/* Set up graph stuff */
	aginit ();
}

static void
gprof_function_call_chart_view_finalize (GObject *obj)
{
	GProfFunctionCallChartView *self;
	
	self = (GProfFunctionCallChartView *) obj;
	
	g_free (self->priv);
}

static void
gprof_function_call_chart_view_class_init (GProfFunctionCallChartViewClass *klass)
{
	GObjectClass *object_class;
	GProfViewClass *view_class;
	
	object_class = (GObjectClass *) klass;
	view_class = GPROF_VIEW_CLASS (klass);
	
	object_class->finalize = gprof_function_call_chart_view_finalize;
	view_class->refresh = gprof_function_call_chart_view_refresh;
	view_class->get_widget = gprof_function_call_chart_view_get_widget;
}

GType
gprof_function_call_chart_view_get_type (void)
{
	static GType obj_type = 0;
	
	if (!obj_type)
	{
		static const GTypeInfo obj_info = 
		{
			sizeof (GProfFunctionCallChartViewClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gprof_function_call_chart_view_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,           /* class_data */
			sizeof (GProfFunctionCallChartView),
			0,              /* n_preallocs */
			(GInstanceInitFunc) gprof_function_call_chart_view_init,
			NULL            /* value_table */
		};
		obj_type = g_type_register_static (GPROF_VIEW_TYPE,
		                                   "GProfFunctionCallChartView", &obj_info, 0);
	}
	return obj_type;
}

GProfFunctionCallChartView *
gprof_function_call_chart_view_new (GProfProfileData *profile_data,
									IAnjutaSymbolManager *symbol_manager,
									IAnjutaDocumentManager *document_manager)
{
	GProfFunctionCallChartView *view;
	
	view = g_object_new (GPROF_FUNCTION_CALL_CHART_VIEW_TYPE, NULL);
	gprof_view_set_data (GPROF_VIEW (view), profile_data);
	gprof_view_set_symbol_manager (GPROF_VIEW (view), symbol_manager);
	gprof_view_set_document_manager (GPROF_VIEW (view), document_manager);
	
	return view;
}

static gboolean
async_draw_graph (gpointer user_data)
{
	GProfFunctionCallChartView *self;
	Agedge_t *current_edge;
	Agraph_t *current_graph;
	point node_pos;
#ifndef ND_coord_i	
	pointf node_posf;
#endif	
	gdouble node_width;
	gdouble node_height;
	
	self = GPROF_FUNCTION_CALL_CHART_VIEW (user_data);
	
	if (self->priv->current_graph)
	{
		current_graph = self->priv->current_graph->data;
		
		if (self->priv->current_node)
		{
#ifdef ND_coord_i			
			node_pos = ND_coord_i (self->priv->current_node);
#else
			node_posf = ND_coord (self->priv->current_node);
			PF2P(node_posf,node_pos);
#endif			
			
			node_pos.y += self->priv->y_offset;
			node_width = ND_width (self->priv->current_node);
			node_height = ND_height (self->priv->current_node);
			
			gprof_function_call_chart_view_draw_node (self, self->priv->current_node, 
													  &node_pos, node_width, node_height);
													  
			
			/* Handle the edges */
			current_edge = agfstedge (current_graph, 
									  self->priv->current_node);
			
			while (current_edge)
			{
				gprof_function_call_chart_view_draw_edge (self, current_edge, 
														  &node_pos, node_height);
				
				current_edge = agnxtedge (current_graph, current_edge, 
										  self->priv->current_node);
			}
			
			self->priv->current_node = agnxtnode (current_graph, 
												  self->priv->current_node);
			return TRUE;
		}
		
		self->priv->y_offset = self->priv->canvas_size_y + 25;
		self->priv->current_graph = g_list_next (self->priv->current_graph);
		
		if (self->priv->current_graph)
		{
			self->priv->current_node = agfstnode (self->priv->current_graph->data);
			return TRUE;
		}
	}
	
	gprof_function_call_chart_view_destroy_graph (self);
	
	return FALSE;
}

void
gprof_function_call_chart_view_refresh (GProfView *view)
{
	GProfFunctionCallChartView *self;
	GList *current_graph;
	
	self = GPROF_FUNCTION_CALL_CHART_VIEW (view);
	
	gprof_function_call_chart_view_create_graph (self);
	gprof_function_call_chart_view_clear_canvas (self);
	
	/* Set up layout of graph */
	current_graph = self->priv->graphs;
	
	/* Set canvas size for scrolling */
	self->priv->canvas_size_x = 250;
	self->priv->canvas_size_y = 250;
	self->priv->y_offset = 0;
	
	while (current_graph)
	{
		gvLayout (self->priv->gvc, current_graph->data, "dot");
		current_graph = g_list_next (current_graph);
	}
	
	/* Handle the graph asyncronously */
	self->priv->current_graph = self->priv->graphs;
	
	if (self->priv->current_graph)
		self->priv->current_node = agfstnode (self->priv->current_graph->data);
	
	g_idle_add_full (G_PRIORITY_DEFAULT_IDLE + 200, async_draw_graph, self, 
					 NULL);
}

GtkWidget *
gprof_function_call_chart_view_get_widget (GProfView *view)
{
	GProfFunctionCallChartView *self;
	
	self = GPROF_FUNCTION_CALL_CHART_VIEW (view);
	
	return self->priv->scrolled_window;
}
