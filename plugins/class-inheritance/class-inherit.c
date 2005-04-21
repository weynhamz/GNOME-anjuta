/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 *  Copyright (C) Massimo Cora' 2005 <maxcvs@email.it>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#include <glib.h>
#include <libanjuta/anjuta-debug.h>

#include <graphviz/dotneato.h>	/* libgraph */
#include <graphviz/graph.h>



// for debug?!
#include <tm_tagmanager.h>

#include "plugin.h"
#include "class-inherit.h"

#define DEFAULT_GRAPH_NAME		"Anjuta Graph"
#define CANVAS_MIN_SIZE			250

#define INCH_TO_PIXELS_CONVERSION_FACTOR		72
#define INCH_TO_PIXELS(inch_size) \
				INCH_TO_PIXELS_CONVERSION_FACTOR * inch_size

#define NODE_COLOR_DEFAULT			"#f6f0c5"
#define NODE_COLOR_BORDER			"#e7cb10"
#define NODE_COLOR_MOUSE_OVER		"#f8191f"
#define NODE_COLOR_TEXT				"#4310e7"

#if 0
static gint
canvas_event (GnomeCanvasItem *canvas, GdkEvent *event, gpointer data)
{
	static double x, y; /* used to keep track of motion coordinates */
	double new_x, new_y;
	
//	DEBUG_PRINT ("canvas event!!");
	switch (event->type) {
	case GDK_MOTION_NOTIFY:
		if (event->button.state & GDK_BUTTON1_MASK) {
			/* Get the new position and move by the difference */

			new_x = event->motion.x;
			new_y = event->motion.y;

			DEBUG_PRINT ("mouse on %f, %f", 403-new_x , 406-new_y );

			x = new_x;
			y = new_y;
			return TRUE;
		}
		break;

	default:
		break;
	}

	return FALSE;
}
	
	
static void
canvas_size_allocate (GnomeCanvas *canvas, GtkAllocation *alloc, gpointer data) {
	AnjutaClassInheritance *plugin;
	
//	DEBUG_PRINT ("canvas size allocate alloc width: %f, plugin->canvas_size %f", alloc->width, plugin->canvas_size);

	plugin = (AnjutaClassInheritance*)data;
/*/	
	gnome_canvas_set_scroll_region (GNOME_CANVAS (canvas),
				-alloc->width, 100, alloc->width, -plugin->canvas_size);
//*/	
//	DEBUG_PRINT ("canvas max size from allocate is %d", plugin->canvas_size); 
}
#endif

static void
cls_inherit_graph_init (AnjutaClassInheritance *plugin, gchar* graph_label) {
	
	DEBUG_PRINT ("initializing graph");

	aginit ();
	plugin->graph = agopen (graph_label, AGDIGRAPH);
}

/*----------------------------------------------------------------------------
 * Perform a dot_cleanup and a graph closing. Call this function after a
 * call to draw_graph.
 */
static void
cls_inherit_graph_cleanup (AnjutaClassInheritance *plugin) {
	
	DEBUG_PRINT ("cleanupping graph");
	if (plugin->graph != NULL) {
		dot_cleanup (plugin->graph);
		agclose (plugin->graph);
	}
	
	plugin->graph = NULL;	
}


/*----------------------------------------------------------------------------
 * add a node to an Agraph.
 */
static gboolean
cls_inherit_add_node (AnjutaClassInheritance *plugin, gchar* node_name) {
	
	Agnode_t *node;
	Agsym_t *sym;
	
	DEBUG_PRINT ("class_inheritance_add_node ");
	/* if graph isn't initialized, init it */
	if (!plugin->graph)
		cls_inherit_graph_init (plugin, _(DEFAULT_GRAPH_NAME));
	
	/* let's add the node to the graph */
	if ((node = agnode (plugin->graph, node_name)) == NULL)
		return FALSE;
	
   /* Set an attribute - in this case one that affects the visible rendering */
   if (!(sym = agfindattr(plugin->graph->proto->n, "shape")))
		sym = agnodeattr(plugin->graph, "shape", "");
	agxset(node, sym->index, "box");

	/* set the font */
	/* FIXME: add the possibility to select the fonts from preferences?! */
   if (!(sym = agfindattr(plugin->graph->proto->n, "fontname")))
		sym = agnodeattr(plugin->graph, "fontname", "");
	agxset(node, sym->index, "Courier new");

	/* set the font-size */	
   if (!(sym = agfindattr(plugin->graph->proto->n, "fontsize")))
		sym = agnodeattr(plugin->graph, "fontsize", "");
	/* hack: set canvas_text_fontsize + 4 points or text would oversize the block */
	agxset(node, sym->index, "14");

   if (!(sym = agfindattr(plugin->graph->proto->n, "ratio")))
		sym = agnodeattr(plugin->graph, "ratio", "");
	agxset(node, sym->index, "expand");
	
	
	DEBUG_PRINT ("added node %s....", node_name);
	
	return TRUE;
}

/*----------------------------------------------------------------------------
 * add an edge to an Agraph.
 */
static gboolean
cls_inherit_add_edge (AnjutaClassInheritance *plugin, gchar* node_from, gchar* node_to) {
	
	Agedge_t *edge;
	Agnode_t *n_from, *n_to;

	DEBUG_PRINT ("class_inheritance_add_edge ");	
	/* if we hadn't initialized out graph we return FALSE. Edges require two nodes */
	if (!plugin->graph)
		return FALSE;
	
	if ((n_from = agfindnode (plugin->graph, node_from)) == NULL)
		return FALSE;
	
	if ((n_to = agfindnode (plugin->graph, node_to)) == NULL)
		return FALSE;
	
	if ((edge = agedge (plugin->graph, n_from, n_to)) == NULL)
		return FALSE;
	
	DEBUG_PRINT ("added edge: %s ---> %s", node_from, node_to);
}

static void
cls_item_destroy (GList* node) {
	
	gtk_object_destroy(GTK_OBJECT(node->data));
	
}

static void
cls_inherit_draw_graph (AnjutaClassInheritance *plugin) {
	
	gint num_nodes;
	gint i, tmp;
	gdouble max_canvas_size_x, max_canvas_size_y;
	GnomeCanvasItem *item;
	Agnode_t *node;
	Agedge_t *edge;
	ArtBpath *bpath;
	
	g_return_if_fail (plugin->graph != NULL);
	
	num_nodes = agnnodes (plugin->graph);	
	g_return_if_fail (num_nodes > 0);
	
	/* compiles nodes/edges informations, such as positions, coordinates etc */
	dot_layout (plugin->graph);
	
/*/ DEBUG	
	num_nodes = agnnodes (plugin->graph);
	DEBUG_PRINT ("num of nodes: %d", num_nodes); 
	for (node = agfstnode (plugin->graph); node; node = agnxtnode (plugin->graph, node)) {
		point p;
		p = ND_coord_i(node);
		DEBUG_PRINT("%d,%d. rw is %d\n", p.x, p.y, ND_rw_i(node));
	}	
	agwrite (plugin->graph, stdout);	
//*/
	
	/* set the size of the canvas. We need this to set the scrolling.. */
	max_canvas_size_x = max_canvas_size_y = CANVAS_MIN_SIZE;
	
	/* check whether we had yet drawn something on the canvas. In case remove the 
	items so we can clean up the canvas ready for others paints */
	if ( (tmp=g_list_length (plugin->item_list)) > 0) {
		/* destroying a gnome_canvas_item will un-paint automatically from the canvas */
		g_list_foreach (plugin->item_list, (GFunc)gtk_object_destroy, NULL);
		g_list_free(plugin->item_list);
		
		/* re-initializing the g_list */
		plugin->item_list = NULL;
	}
		
	/* first of all draw the nodes */
	for (node = agfstnode (plugin->graph); node; node = agnxtnode (plugin->graph, node)) {
		gdouble width;
		gdouble height;
		point p;

		/* get some infos from the node */
		p = ND_coord_i(node);
		width = ND_width (node);
		height = ND_height (node);
/*/		
		DEBUG_PRINT ( "*******NODE %s: width is %f so %f-->%f; height is %f so %f-->%f; p.x %d; p.y %d", 
						ND_label (node)->text, width, p.x - INCH_TO_PIXELS (width)/2, p.x + INCH_TO_PIXELS (width)/2,
													  height, p.y - INCH_TO_PIXELS (height)/2, p.y + INCH_TO_PIXELS (height)/2,
													  p.x, p.y );
/*/		
		/* create the item and then add it to the list */
		item = gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (plugin->canvas)),
				      gnome_canvas_rect_get_type (),
				      "x1", (gdouble) (p.x - INCH_TO_PIXELS (width)/2),
				      "y1", (gdouble) -(p.y - INCH_TO_PIXELS (height)/2),
				      "x2", (gdouble) (p.x + INCH_TO_PIXELS (width)/2),
				      "y2", (gdouble) -(p.y + INCH_TO_PIXELS (height)/2),
				      "fill_color", NODE_COLOR_DEFAULT,
				      "outline_color", NODE_COLOR_BORDER,
				      "width_units", 1.0,
				      NULL);
		plugin->item_list = g_list_prepend (plugin->item_list, item);
						
		item = gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (plugin->canvas)),
						gnome_canvas_text_get_type (),
              		"text", ND_label (node)->text, 
						"font", "Courier new 10",
                  "x", (gdouble) (p.x - INCH_TO_PIXELS (width)/2 +2),
                  "y", (gdouble) -p.y,
                  "fill_color", NODE_COLOR_TEXT,
                  "anchor", GTK_ANCHOR_W,        
                  NULL );
		plugin->item_list = g_list_prepend (plugin->item_list, item);						

		/* let's draw edges */
		for (edge = agfstedge (plugin->graph, node); edge; edge = agnxtedge (plugin->graph, edge, node)) {
			GnomeCanvasPathDef *path_def;
 			gint i, k;
			
			path_def = gnome_canvas_path_def_new();
			
			for ( i = 0; i < ED_spl(edge)->list->size-1; i+=3) {
		
				DEBUG_PRINT ("curr_index %d", i);
				/* go on with bezier curves. We can retrieve the info such as control points 
				from the struct of the edge */
	         gnome_canvas_path_def_moveto (path_def, ((ED_spl(edge))->list->list[0+i]).x, 
													 -((ED_spl(edge))->list->list[0+i]).y);
				
         	gnome_canvas_path_def_curveto (path_def, 
													((ED_spl(edge))->list->list[1+i]).x, 
													-((ED_spl(edge))->list->list[1+i]).y,
													((ED_spl(edge))->list->list[2+i]).x, 
													-((ED_spl(edge))->list->list[2+i]).y,
													((ED_spl(edge))->list->list[3+i]).x, 
													-((ED_spl(edge))->list->list[3+i]).y );
				
				/* check whether we have to draw an arrow. Is the right point? */				
				if ( i+3 >= (ED_spl(edge)->list->size-1) ) {
					GnomeCanvasPoints * points;
					gdouble upper_bound = (gdouble)(p.y + INCH_TO_PIXELS (height)/2);
					gdouble x_offset;
					gint h;					
					
					/*
					          __|__   _ 
					          \   /    \
					           \ /     | h 
					            °     _/
					      ^^^^^^^^^^^^^
					*/
					
					h = abs (((ED_spl(edge))->list->list[3+i]).y - upper_bound);
					
					if ( (((ED_spl(edge))->list->list[3+i]).x - ((ED_spl(edge))->list->list[2+i]).x) > 0 )
						x_offset = sqrt( abs(10*10 - h*h) );
					else
						x_offset = -sqrt( abs(10*10 - h*h) );
			
/*/					
					DEBUG_PRINT ("===> drawing arrow!\n"
									"x_offset e' %f\n"
									"h %d\n"
									"p.y %d\n"
									"p.x %d\n"
									"((ED_spl(edge))->list->list[3+i]).y %d\n"
									"upper_bound %f\n"
									"heigth %f", 
									x_offset, 
									h, 
									p.y, 
									p.x,
									((ED_spl(edge))->list->list[3+i]).y, 
									upper_bound, 
									height);
/*/					
					/* let's draw a canvas_line with an arrow-end */
					points = gnome_canvas_points_new (2);

					// starting point
					points->coords[0] = ((ED_spl(edge))->list->list[3+i]).x;
					points->coords[1] = -((ED_spl(edge))->list->list[3+i]).y;
					
					// pointer
					points->coords[2] = ((ED_spl(edge))->list->list[3+i]).x + x_offset;
					points->coords[3] = -upper_bound;
					
					/* ok we take an arrow_max_length of 10 pixels for default. */
					if ( abs(x_offset) <= 10 ) {
						DEBUG_PRINT ("tracing arrow to ----> %s", ND_label(node)->text);
						
						item = gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (plugin->canvas)),
														gnome_canvas_line_get_type(),
														"points", points,
														"fill_color", "black",
														"last_arrowhead", TRUE,
														"arrow_shape_a", 10.0,
														"arrow_shape_b", 10.0,
														"arrow_shape_c", 4.0,
											   		"width_units", 1.0,
														NULL);
						plugin->item_list = g_list_prepend (plugin->item_list, item);
					}
				}
			}
			
			/* draw the path_def */
         item = gnome_canvas_item_new(	gnome_canvas_root (GNOME_CANVAS (plugin->canvas)), 
														gnome_canvas_bpath_get_type(),
     	                                    "bpath", path_def,
        	                                 "outline_color", "black",
           	                              "width_pixels", 1,
              	                           NULL);
			plugin->item_list = g_list_prepend (plugin->item_list, item);

		}

		if (abs(p.x) > max_canvas_size_x ) 
			max_canvas_size_x = abs(p.x) + INCH_TO_PIXELS (width)/2;
		
		if (abs(p.y) > max_canvas_size_y) 
			max_canvas_size_y = abs(p.y) + INCH_TO_PIXELS (height)/2;
	}	
	
	gtk_widget_set_size_request (plugin->canvas, max_canvas_size_x +100, max_canvas_size_y +100);
	gnome_canvas_set_scroll_region ( GNOME_CANVAS (plugin->canvas), -50,
					50, max_canvas_size_x + 50, -max_canvas_size_y -50);
		
	cls_inherit_graph_cleanup (plugin);
}

void
class_inheritance_update_graph (AnjutaClassInheritance *plugin) {
	guint type;	
	GnomeCanvasItem *item;
	GnomeCanvasPoints *points;

	// FIXME: use tm_manager from the interface. Change here!!
	/* trying to retrieve tags... */
	TMWorkObject *tm_project;
	TMSymbol *sym;
	int j;
	
	if (plugin->top_dir == NULL)
		return;
	
	tm_project = tm_project_new (plugin->top_dir, NULL, NULL, FALSE);

	DEBUG_PRINT ("Total nodes: %d", tm_project->tags_array->len );
	
	/* search throught the tree list of symbols */
	for (j = 0; j < tm_project->tags_array->len; j++) {
		TMTag *tag = TM_TAG (tm_project->tags_array->pdata[j]);

		if (tag == NULL)
			continue;
		
		if (tag->type == tm_tag_class_t ) {
			if ( tag->atts.entry.inheritance != NULL ) {
				DEBUG_PRINT	("class %s with inheritance %s", tag->name, tag->atts.entry.inheritance );				
				cls_inherit_add_node (plugin, tag->name);
				cls_inherit_add_node (plugin, tag->atts.entry.inheritance);
				cls_inherit_add_edge (plugin, tag->atts.entry.inheritance, tag->name );
			}
		}
	}
	
	
/*/ DEBUG GRAPH. Decomment here to have one more graph to display.
	
	cls_inherit_add_node (plugin, "first");
	cls_inherit_add_node (plugin, "second");
	cls_inherit_add_node (plugin, "third");
	cls_inherit_add_node (plugin, "fourth");
	cls_inherit_add_node (plugin, "fifth");
	cls_inherit_add_node (plugin, "sixth");

	cls_inherit_add_edge (plugin, "first", "second");
	cls_inherit_add_edge (plugin, "second", "third");
	cls_inherit_add_edge (plugin, "first", "third");
	cls_inherit_add_edge (plugin, "third", "fourth");
	cls_inherit_add_edge (plugin, "fifth", "sixth");
	cls_inherit_add_edge (plugin, "first", "fourth");
	cls_inherit_add_edge (plugin, "first", "fifth");
	cls_inherit_add_edge (plugin, "third", "fifth");
//*/	
	cls_inherit_draw_graph (plugin);
	
}


static void
on_update_clicked (GtkWidget *button, AnjutaClassInheritance *plugin) {
	
	class_inheritance_update_graph (plugin);	
}	



void
class_inheritance_base_gui_init (AnjutaClassInheritance *plugin) {
	GtkWidget *hbox;
	GtkWidget *s_window;
	
	s_window = gtk_scrolled_window_new (NULL, NULL);
	plugin->canvas = gnome_canvas_new_aa ();

	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (s_window), plugin->canvas);

	gtk_widget_set_size_request (plugin->canvas, CANVAS_MIN_SIZE, CANVAS_MIN_SIZE);
	gnome_canvas_set_scroll_region (GNOME_CANVAS (plugin->canvas), -CANVAS_MIN_SIZE/2, 
					CANVAS_MIN_SIZE/2, CANVAS_MIN_SIZE/2, -CANVAS_MIN_SIZE/2);
/*/	
	gtk_signal_connect (GTK_OBJECT (plugin->canvas), "event",
			    (GtkSignalFunc) canvas_event,
			    NULL);
	
	gtk_signal_connect (GTK_OBJECT (plugin->canvas), "size_allocate",
			    (GtkSignalFunc) canvas_size_allocate,
			    plugin);

	
//*/	
	plugin->widget = gtk_vbox_new (FALSE, 2);
	hbox = gtk_hbox_new (FALSE, 2);
	
	plugin->update_button = gtk_button_new_with_label (_("Update"));
	gtk_signal_connect (GTK_OBJECT (plugin->update_button), "clicked",
			    (GtkSignalFunc) on_update_clicked ,
			    plugin);

	/* --packing-- */
	/* hbox */
	gtk_box_pack_start (GTK_BOX (hbox), plugin->update_button, TRUE, FALSE, FALSE);
	
	/* vbox */
	gtk_box_pack_start (GTK_BOX (plugin->widget), s_window, TRUE, TRUE, TRUE);
	gtk_box_pack_end (GTK_BOX (plugin->widget), hbox, FALSE, TRUE, TRUE);
	
	gtk_widget_show_all (plugin->widget);
	
	/* create new GList */
	plugin->item_list = NULL;
}
