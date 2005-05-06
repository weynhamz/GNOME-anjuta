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
#include <graphviz/dotneato.h>	/* libgraph */
#include <graphviz/graph.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-symbol-manager.h>

#include "plugin.h"
#include "class-inherit.h"

#define DEFAULT_GRAPH_NAME		"Anjuta Graph"
#define CANVAS_MIN_SIZE			250

#define INCH_TO_PIXELS_CONVERSION_FACTOR		72
#define INCH_TO_PIXELS(inch_size) \
				INCH_TO_PIXELS_CONVERSION_FACTOR * inch_size

#define NODE_FONT_DEFAULT "-*-clean-medium-r-normal-*-10-*-*-*-*-*-*"

static gint
on_canvas_event (GnomeCanvasItem *item, GdkEvent *event, gpointer data) {
	AnjutaClassInheritance *plugin;
	
	plugin = (AnjutaClassInheritance*)data;
	
	switch (event->type)
	{
	case GDK_BUTTON_PRESS:
		if (event->button.button == 3)
		{
			g_return_val_if_fail (plugin->menu != NULL, FALSE);
			
			gtk_menu_popup (GTK_MENU (plugin->menu), NULL, NULL, NULL, NULL, 
							event->button.button, event->button.time);
		}
		break;

	default:
		break;
	}
	
	return FALSE;
}

static void
on_toggled_menuitem_clicked (GtkCheckMenuItem *checkmenuitem,
							 gpointer data)
{
	NodeData *node;
	node = (NodeData*)data;
	
	if (node->anchored)
		node->anchored = FALSE;
	else
		node->anchored = TRUE;
}

static void
on_member_menuitem_clicked (GtkMenuItem *menuitem, gpointer data)
{
	NodeData *node;
	const gchar *file;
	gint line;
	
	node = (NodeData*)data;
	file = g_object_get_data (G_OBJECT (menuitem), "__file");
	line = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (menuitem), "__line"));
	if (file)
	{
		/* Goto file line */
		IAnjutaDocumentManager *dm;
		dm = anjuta_shell_get_interface (ANJUTA_PLUGIN (node->plugin)->shell,
										 IAnjutaDocumentManager, NULL);
		if (dm)
		{
			ianjuta_document_manager_goto_file_line (dm, file, line, NULL);
		}
	}
}

static void
cls_inherit_show_dynamic_class_popup_menu (GdkEvent *event,
										   NodeData* nodedata)
{
	GtkWidget *item, *image;
	GtkWidget *checkitem;
	
	/* Destroy the old menu before creating a new one */
	if (nodedata->menu)
	{
		gtk_widget_destroy (nodedata->menu);
	}
		
	nodedata->menu = gtk_menu_new();
	if (nodedata->name && strlen (nodedata->name))
	{
		IAnjutaSymbolManager *sm;
		sm = anjuta_shell_get_interface (ANJUTA_PLUGIN (nodedata->plugin)->shell,
										 IAnjutaSymbolManager, NULL);
		if (sm)
		{
			IAnjutaIterable *iter;
			iter = ianjuta_symbol_manager_get_members (sm, nodedata->name,
													   FALSE, NULL);
			if (iter && ianjuta_iterable_get_length (iter, NULL) > 0)
			{
				do
				{
					IAnjutaSymbol *symbol;
					
					symbol = ianjuta_iterable_get (iter,
												   IANJUTA_TYPE_SYMBOL,
												   NULL);
					if (symbol)
					{
						const gchar *name, *file;
						GdkPixbuf *pixbuf;
						gint line;
						
						name = ianjuta_symbol_name (symbol, NULL);
						pixbuf = ianjuta_symbol_icon (symbol, NULL);
						file = ianjuta_symbol_file (symbol, NULL);
						line = ianjuta_symbol_line (symbol, NULL);
						
						item = gtk_image_menu_item_new_with_label (name);
						image = gtk_image_new_from_pixbuf (pixbuf);
						gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM
													   (item), image);
						
						if (file)
						{
							g_object_set_data_full (G_OBJECT (item), "__file",
													g_strdup (file), g_free);
							g_object_set_data (G_OBJECT (item), "__line",
											   GINT_TO_POINTER (line));
						}
						gtk_container_add (GTK_CONTAINER (nodedata->menu),
										   item);
						g_signal_connect (G_OBJECT (item), "activate",
													G_CALLBACK (on_member_menuitem_clicked),
													nodedata);
					}
				} while (ianjuta_iterable_next (iter, NULL));
			}
			if (iter)
				g_object_unref (iter);
		}
	}
	/* create the check menuitem */
	checkitem = gtk_check_menu_item_new_with_label (_("Fixed data-view"));
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (checkitem),
									nodedata->anchored);
	
	g_signal_connect (G_OBJECT (checkitem), "toggled",
								G_CALLBACK (on_toggled_menuitem_clicked),
								nodedata);
	
	gtk_container_add (GTK_CONTAINER (nodedata->menu), checkitem);
	
	gtk_widget_show_all (nodedata->menu);
	gtk_menu_popup (GTK_MENU (nodedata->menu), NULL, NULL,
	                NULL, NULL, event->button.button,
					event->button.time);	
}

static gint
on_nodedata_event (GnomeCanvasItem *item, GdkEvent *event, gpointer data)
{
	AnjutaClassInheritance *plugin;
	NodeData *nodedata;	
	
	nodedata = (NodeData*)data;
	plugin = nodedata->plugin;

	switch (event->type)
	{
	case GDK_2BUTTON_PRESS:		/* double click */
		break;

	case GDK_BUTTON_PRESS:		/* single click */
		if (event->button.button == 1)
		{
			cls_inherit_show_dynamic_class_popup_menu (event, data);
		}
		break;
		
	case GDK_ENTER_NOTIFY:		/* mouse entered in item's area */
		/* Make the outline wide */
		gnome_canvas_item_set (nodedata->canvas_item,
							   "width_units", 2.5,
							   "fill_color_gdk",
							   &plugin->canvas->style->light[GTK_STATE_SELECTED],
							   "outline_color_gdk",
							   &plugin->canvas->style->fg[GTK_STATE_ACTIVE],
							   NULL);
		return TRUE;

	case GDK_LEAVE_NOTIFY:		/* mouse exited item's area */
		/* Make the outline thin */
		gnome_canvas_item_set (nodedata->canvas_item,
							   "width_units", 1.0,
							   "fill_color_gdk",
							   &plugin->canvas->style->light[GTK_STATE_NORMAL],
							   "outline_color_gdk",
							   &plugin->canvas->style->fg[GTK_STATE_NORMAL],	
							   NULL);
		return TRUE;
	default:
		break;
	}
	
	return FALSE;
}
	
/*----------------------------------------------------------------------------
 * initialize the internal graphviz structure.
 */
static void
cls_inherit_graph_init (AnjutaClassInheritance *plugin, gchar* graph_label)
{
	DEBUG_PRINT ("initializing graph");

	aginit ();
	plugin->graph = agopen (graph_label, AGDIGRAPH);
}

/*----------------------------------------------------------------------------
 * Perform a dot_cleanup and a graph closing. Call this function at the end of
 * call to draw_graph.
 */
static void
cls_inherit_graph_cleanup (AnjutaClassInheritance *plugin)
{
	DEBUG_PRINT ("cleanupping graph");
	if (plugin->graph != NULL)
	{
		dot_cleanup (plugin->graph);
		agclose (plugin->graph);
	}
	
	plugin->graph = NULL;
}

/*----------------------------------------------------------------------------
 * destroys a NodeData element. All it's resources will be deallocated
 * and setted to null.
 */
static void
cls_inherit_nodedata_destroy (NodeData *node_data)
{
	if (node_data->name)
	{
		g_free (node_data->name);
		node_data->name = NULL;
	}
	
	if (node_data->canvas_item)
	{
		gtk_object_destroy (GTK_OBJECT (node_data->canvas_item));
		node_data->canvas_item = NULL;
	}
	
	if (node_data->menu)
	{
		gtk_widget_destroy (node_data->menu);
		node_data->menu = NULL;
	}
	node_data->anchored = FALSE;	
}

/*----------------------------------------------------------------------------
 * clean the canvas and all its painted objects.
 */
void
class_inheritance_clean_canvas (AnjutaClassInheritance *plugin)
{
	DEBUG_PRINT ("cleaning canvas");
	if (plugin->drawable_list == NULL || plugin->node_list == NULL)
		return;
	
	/* destroying a gnome_canvas_item will un-paint automatically from
	 * the canvas
	 */
	g_list_foreach (plugin->drawable_list, (GFunc)gtk_object_destroy, NULL);
	g_list_free(plugin->drawable_list);
	
	/* the same for the nodes' list */
	g_list_foreach (plugin->node_list, (GFunc)cls_inherit_nodedata_destroy,
					NULL);
	g_list_free(plugin->node_list);
	
	/* re-initializing the g_list */
	plugin->drawable_list = NULL;
	plugin->node_list = NULL;
}


/*----------------------------------------------------------------------------
 * add a node to an Agraph.
 */
static gboolean
cls_inherit_add_node (AnjutaClassInheritance *plugin, const gchar* node_name)
{
	Agnode_t *node;
	Agsym_t *sym;
	
	/* if graph isn't initialized, init it */
	if (!plugin->graph)
		cls_inherit_graph_init (plugin, _(DEFAULT_GRAPH_NAME));
	
	/* let's add the node to the graph */
	if ((node = agnode (plugin->graph, (gchar*)node_name)) == NULL)
		return FALSE;
	
	/* Set an attribute - in this case one that affects the visible rendering */
	if (!(sym = agfindattr(plugin->graph->proto->n, "shape")))
		sym = agnodeattr(plugin->graph, "shape", "");
	agxset(node, sym->index, "box");

	/* set the font */
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
	
	/* DEBUG_PRINT ("added node %s....", node_name); */
	return TRUE;
}

/*----------------------------------------------------------------------------
 * add an edge to an Agraph.
 */
static gboolean
cls_inherit_add_edge (AnjutaClassInheritance *plugin,
					  const gchar* node_from,
					  const gchar* node_to)
{
	Agedge_t *edge;
	Agnode_t *n_from, *n_to;

	/* if we hadn't initialized out graph we return FALSE. Edges require
	 * two nodes
	 */
	if (!plugin->graph)
		return FALSE;
	
	if ((n_from = agfindnode (plugin->graph, (gchar*)node_from)) == NULL)
		return FALSE;
	
	if ((n_to = agfindnode (plugin->graph, (gchar*)node_to)) == NULL)
		return FALSE;
	
	if ((edge = agedge (plugin->graph, n_from, n_to)) == NULL)
		return FALSE;
	
	/* DEBUG_PRINT ("added edge: %s ---> %s", node_from, node_to); */
	return TRUE;
}

/*----------------------------------------------------------------------------
 * draw the graph on the canvas. So nodes, edges, arrows, texts..
 * If something is found already drawn on the canvas it is cleaned before
 * draw new things.
 */
static void
cls_inherit_draw_graph (AnjutaClassInheritance *plugin)
{
	gint num_nodes;
	gdouble max_canvas_size_x, max_canvas_size_y;
	GnomeCanvasItem *item;
	Agnode_t *node;
	Agedge_t *edge;
	/* ArtBpath *bpath; */
	
	if (plugin->graph == NULL)
		return;
	DEBUG_PRINT ("======== going to draw graph ========");
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
	
	/* check whether we had already drawn something on the canvas.
	 * In case remove the items so we can clean up the canvas ready
	 * for others paints
	 */
	if (g_list_length (plugin->drawable_list) > 0 ||
		g_list_length (plugin->node_list) > 0)
	{
		class_inheritance_clean_canvas (plugin);
	}
		
	/* first of all draw the nodes */
	for (node = agfstnode (plugin->graph); node;
		 node = agnxtnode (plugin->graph, node))
	{
		gdouble node_width;
		gdouble node_height;
		point p;
		gdouble text_width_value;
		NodeData *node_data;
		
		/* get some infos from the node */
		p = ND_coord_i(node);
		node_width = ND_width (node);
		node_height = ND_height (node);
		/*/
		DEBUG_PRINT ( "*******NODE %s: width is %f so %f-->%f; height is %f so %f-->%f; p.x %d; p.y %d", 
						ND_label (node)->text, width, p.x - INCH_TO_PIXELS (width)/2, p.x + INCH_TO_PIXELS (width)/2,
													  height, p.y - INCH_TO_PIXELS (height)/2, p.y + INCH_TO_PIXELS (height)/2,
													  p.x, p.y );
		/*/
		
		/* --- node --- */
		/* create the nodedata item and then add it to the list */
		node_data = g_new0 (NodeData, 1);
		
		/* set the plugin reference */
		node_data->plugin = plugin;
		node_data->anchored = FALSE;
		node_data->name = g_strdup (ND_label (node)->text);
		
		node_data->canvas_item =
		gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (plugin->canvas)),
							   gnome_canvas_rect_get_type (),
							   "x1",
							   (gdouble) (p.x - INCH_TO_PIXELS (node_width)/2),
							   "y1",
							   (gdouble) -(p.y - INCH_TO_PIXELS (node_height)/2),
							   "x2",
							   (gdouble) (p.x + INCH_TO_PIXELS (node_width)/2),
							   "y2",
							   (gdouble) -(p.y + INCH_TO_PIXELS (node_height)/2),
							   "fill_color_gdk",
							   &plugin->canvas->style->light[GTK_STATE_NORMAL],
							   "outline_color_gdk",
							   &plugin->canvas->style->fg[GTK_STATE_NORMAL],
							   "width_units", 1.0,
							   NULL);
		
		plugin->node_list = g_list_prepend (plugin->node_list, node_data);
		
		g_signal_connect (GTK_OBJECT (node_data->canvas_item), "event",
						  G_CALLBACK (on_nodedata_event),
						  node_data);

		/* --- texts --- */		
		item = gnome_canvas_item_new (gnome_canvas_root
									  (GNOME_CANVAS (plugin->canvas)),
									  gnome_canvas_text_get_type (),
									  "text", ND_label (node)->text, 
									  "font", NODE_FONT_DEFAULT,
									  "justification", GTK_JUSTIFY_CENTER,
									  "anchor", GTK_ANCHOR_CENTER,
									  "x",
									  (gdouble) (p.x - INCH_TO_PIXELS (node_width)/2),
									  "y", (gdouble) -p.y,
									  "fill_color_gdk",
									  &plugin->canvas->style->text[GTK_STATE_NORMAL],
									  "anchor", GTK_ANCHOR_W,        
									  NULL );
		
		/* center the text in the node... */
		g_object_get (item, "text_width", &text_width_value, NULL);
						
		gnome_canvas_item_set (item, "x",
							   (gdouble)((p.x - text_width_value/2)), NULL);

		plugin->drawable_list = g_list_prepend (plugin->drawable_list, item);

		/* --- edges --- */
		for (edge = agfstedge (plugin->graph, node); edge;
			 edge = agnxtedge (plugin->graph, edge, node))
		{
			GnomeCanvasPathDef *path_def;
 			gint i;
			
			path_def = gnome_canvas_path_def_new();
			
			for ( i = 0; i < ED_spl(edge)->list->size-1; i+=3)
			{
				
				/* go on with bezier curves. We can retrieve the info such
				 * as control points from the struct of the edge
				 */
				gnome_canvas_path_def_moveto (path_def,
									  ((ED_spl(edge))->list->list[0+i]).x, 
									  -((ED_spl(edge))->list->list[0+i]).y);
				
				gnome_canvas_path_def_curveto (path_def, 
									   ((ED_spl(edge))->list->list[1+i]).x, 
									   -((ED_spl(edge))->list->list[1+i]).y,
									   ((ED_spl(edge))->list->list[2+i]).x, 
									   -((ED_spl(edge))->list->list[2+i]).y,
									   ((ED_spl(edge))->list->list[3+i]).x, 
									   -((ED_spl(edge))->list->list[3+i]).y);
				
				/* check whether we have to draw an arrow. Is the right point? */				
				if ( i+3 >= (ED_spl(edge)->list->size-1) )
				{
					GnomeCanvasPoints * points;
					gdouble upper_bound = (gdouble)(p.y +
											INCH_TO_PIXELS (node_height)/2);
					gdouble x_offset;
					gint h;
					
					/*
					          __|__   _ 
					          \   /    |
					           \ /     | h 
					            °     _|
					      ^^^^^^^^^^^^^
					*/
					
					h = abs (((ED_spl(edge))->list->list[3+i]).y - upper_bound);
					
					if ((((ED_spl(edge))->list->list[3+i]).x -
						((ED_spl(edge))->list->list[2+i]).x) > 0)
						x_offset = sqrt( abs(10*10 - h*h));
					else
						x_offset = -sqrt( abs(10*10 - h*h));
					
					/* let's draw a canvas_line with an arrow-end */
					points = gnome_canvas_points_new (2);

					/* starting point */
					points->coords[0] = ((ED_spl(edge))->list->list[3+i]).x;
					points->coords[1] = -((ED_spl(edge))->list->list[3+i]).y;
					
					/* pointer */
					points->coords[2] =
						((ED_spl(edge))->list->list[3+i]).x + x_offset;
					points->coords[3] = -upper_bound;
					
					/* ok we take an arrow_max_length of 10 pixels for default. */
					if ( abs(x_offset) <= 10 )
					{
						item =
							gnome_canvas_item_new (gnome_canvas_root
										   (GNOME_CANVAS (plugin->canvas)),
										   gnome_canvas_line_get_type(),
										   "points", points,
										   "fill_color_gdk",
							  &plugin->canvas->style->dark[GTK_STATE_SELECTED],
												   "last_arrowhead", TRUE,
												   "arrow_shape_a", 10.0,
												   "arrow_shape_b", 10.0,
												   "arrow_shape_c", 4.0,
												   "width_units", 1.0,
												   NULL);
						plugin->drawable_list =
							g_list_prepend (plugin->drawable_list, item);
					}
				}
			}
			
			/* draw the path_def */
         item = gnome_canvas_item_new (gnome_canvas_root
									   (GNOME_CANVAS (plugin->canvas)), 
									   gnome_canvas_bpath_get_type(),
									   "bpath", path_def,
									   "outline_color_gdk",
									   &plugin->canvas->style->dark[GTK_STATE_SELECTED],
									   "width_pixels", 1,
									   NULL);
			plugin->drawable_list =
				g_list_prepend (plugin->drawable_list, item);
		}

		if (abs(p.x) > max_canvas_size_x ) 
			max_canvas_size_x = abs(p.x) + INCH_TO_PIXELS (node_width)/2;
		
		if (abs(p.y) > max_canvas_size_y) 
			max_canvas_size_y = abs(p.y) + INCH_TO_PIXELS (node_height)/2;
	}	
	
	gtk_widget_set_size_request (plugin->canvas, max_canvas_size_x +100,
								 max_canvas_size_y +100);
	gnome_canvas_set_scroll_region ( GNOME_CANVAS (plugin->canvas), -50,
									50, max_canvas_size_x + 50,
									-max_canvas_size_y -50);
	
	cls_inherit_graph_cleanup (plugin);
}

/*----------------------------------------------------------------------------
 * update the internal graphviz graph-structure, then redraw the graph on the 
 * canvas
 */
void
class_inheritance_update_graph (AnjutaClassInheritance *plugin)
{
	IAnjutaSymbolManager *sm;
	IAnjutaIterable *iter;
	IAnjutaSymbol *symbol;
	GList *classes, *node;
	GHashTable *class_parents;
	gboolean first;

	g_return_if_fail (plugin != NULL);

	if (plugin->top_dir == NULL)
		return;
	
	sm = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
									 IAnjutaSymbolManager, NULL);
	if (!sm)
		return;
	
	iter = ianjuta_symbol_manager_search (sm, IANJUTA_SYMBOL_TYPE_CLASS,
										  "", FALSE, FALSE, NULL);
	if (!iter)
		return;
	
	DEBUG_PRINT ("Number of classes found = %d",
				 ianjuta_iterable_get_length (iter, NULL));
	
	if (ianjuta_iterable_get_length (iter, NULL) <= 0)
	{
		g_object_unref (iter);
		return;
	}
	
	/* Get all classes */
	classes = NULL;
	class_parents = g_hash_table_new_full (g_str_hash, g_str_equal,
										   g_free, g_free);
	first = TRUE;
	while (first || ianjuta_iterable_next (iter, NULL))
	{
		const gchar *class_name, *parents, *old_parents;
		
		first = FALSE;
		
		symbol = ianjuta_iterable_get (iter, IANJUTA_TYPE_SYMBOL, NULL);
		if (!symbol)
			continue;
		
		class_name = ianjuta_symbol_name (symbol, NULL);
		parents = ianjuta_symbol_inheritance (symbol, NULL);
		
		if (!class_name || !parents || strlen (class_name) <= 0 ||
			strlen (parents) <= 0)
			continue;
		
		if ((old_parents = g_hash_table_lookup (class_parents, class_name)))
		{
			if (strcmp (parents, old_parents) != 0)
			{
				g_warning ("Class '%s' has different parents '%s' and '%s'",
						   class_name, old_parents, parents);
			}
			continue;
		}
		g_hash_table_insert (class_parents, g_strdup (class_name),
							 g_strdup (parents));
		classes = g_list_prepend (classes, g_strdup (class_name));
	}
	classes = g_list_reverse (classes);
	g_object_unref (iter);
	
	/* For all classes get their parents */
	node = classes;
	while (node)
	{
		const gchar *class_name, *parents;
		gchar **parentsv, **ptr;
		
		class_name = node->data;
		parents = g_hash_table_lookup (class_parents, class_name);
		parentsv = g_strsplit_set (parents, ";:,", -1);
		
		ptr = parentsv;
		while (*ptr)
		{
			if (strcmp (*ptr, "") == 0)
			{
				ptr++;
				continue;
			}
			cls_inherit_add_node (plugin, class_name);
			cls_inherit_add_node (plugin, *ptr);
			cls_inherit_add_edge (plugin, *ptr, class_name);
			
			DEBUG_PRINT ("%s:%s", *ptr, class_name);
			ptr++;
		}
		g_strfreev (parentsv);
		node = g_list_next (node);
	}
	g_list_foreach (classes, (GFunc)g_free, NULL);
	g_list_free (classes);
	cls_inherit_draw_graph (plugin);	
}

/*----------------------------------------------------------------------------
 * callback for the canvas' right-click menu - update button.
 */
static void
on_update_menu_item_selected (GtkMenuItem *item,
							  AnjutaClassInheritance *plugin)
{
	class_inheritance_update_graph (plugin);
}

/*----------------------------------------------------------------------------
 * callback for theme/colors changes
 */
static void
on_style_set (GtkWidget *widget, GtkStyle  *previous_style,
			  AnjutaClassInheritance *plugin)
{
	class_inheritance_update_graph (plugin);
}

static GnomeUIInfo canvas_menu_uiinfo[] = {
	{ /*0*/
	 GNOME_APP_UI_ITEM, 
	 N_("Update"),
	 N_("Update the graph"),
	 on_update_menu_item_selected,
	 NULL, 
	 NULL,
	 GNOME_APP_PIXMAP_NONE,
	 NULL,
	 0, 
	 0, 
	 NULL},
	GNOMEUIINFO_END
};

void
class_inheritance_base_gui_init (AnjutaClassInheritance *plugin)
{
	GtkWidget *s_window;

	s_window = gtk_scrolled_window_new (NULL, NULL);
	plugin->canvas = gnome_canvas_new_aa ();

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (s_window),
									GTK_POLICY_AUTOMATIC, 
									GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (s_window),
										   plugin->canvas);

	gtk_widget_set_size_request (plugin->canvas, CANVAS_MIN_SIZE,
								 CANVAS_MIN_SIZE);
	gnome_canvas_set_scroll_region (GNOME_CANVAS (plugin->canvas),
									-CANVAS_MIN_SIZE/2, 
									CANVAS_MIN_SIZE/2,
									CANVAS_MIN_SIZE/2,
									-CANVAS_MIN_SIZE/2);

	g_signal_connect (G_OBJECT (plugin->canvas), "event",
					  G_CALLBACK (on_canvas_event),
					  plugin);
					  
	g_signal_connect (G_OBJECT (plugin->canvas),
					  "style_set",
					  G_CALLBACK (on_style_set),
					  plugin);
					  
	plugin->widget = gtk_vbox_new (FALSE, 2);
	/* --packing-- */
	/* vbox */
	gtk_box_pack_start (GTK_BOX (plugin->widget), s_window, TRUE, TRUE, TRUE);
	
	gtk_widget_show_all (plugin->widget);
	
	/* create new GList */
	plugin->drawable_list = NULL;
	plugin->node_list = NULL;

	/* menu create */
	plugin->menu = gtk_menu_new ();
	
	/* set the user data on update selection */
	canvas_menu_uiinfo[0].user_data = plugin;
	
	gnome_app_fill_menu (GTK_MENU_SHELL (plugin->menu), canvas_menu_uiinfo,
						NULL, FALSE, 0);
	
	plugin->update = canvas_menu_uiinfo[0].widget;
	
	gtk_widget_ref (plugin->menu);
	gtk_widget_ref (plugin->update);
}
