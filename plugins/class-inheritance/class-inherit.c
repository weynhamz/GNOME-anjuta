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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
 
#include <glib.h>
#include <graphviz/gvc.h>		/* graphviz */
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-symbol-manager.h>

#include "plugin.h"
#include "class-inherit.h"
#include "class-callbacks.h"

#define DEFAULT_GRAPH_NAME		"Anjuta Graph"
#define CANVAS_MIN_SIZE			250

/* some macros to access deep graphviz's node structures */
#define NODE_LOWER_LEFT(node,main_index,rec_index) \
	(((field_t*)ND_shape_info (node))->fld[main_index]->fld[rec_index]->b.LL)

#define NODE_UPPER_RIGHT(node,main_index,rec_index) \
	(((field_t*)ND_shape_info (node))->fld[main_index]->fld[rec_index]->b.UR)

#define NODE_NUM_FIELDS(node) \
	((field_t*)ND_shape_info (node))->n_flds

#define NODE_NTH_FIELD(node,nth) \
	((field_t*)ND_shape_info (node))->fld[nth]

#define NODE_NTH_TEXT(node,main_index,rec_index) \
	((field_t*)ND_shape_info (node))->fld[main_index]->fld[rec_index]->lp->text


#define INCH_TO_PIXELS_CONVERSION_FACTOR		72
#define INCH_TO_PIXELS(inch_size) \
				INCH_TO_PIXELS_CONVERSION_FACTOR * inch_size


/* TODO: check for symbol_updated event, and check in the nodestatus's hashtable
for the nodes that are gone. In case remove them.
*/


static void
cls_inherit_nodestatus_destroy (NodeExpansionStatus *node) {
	if (node->name)
		g_free (node->name);
	
	g_free (node);
}

void
class_inheritance_show_dynamic_class_popup_menu (GdkEvent *event,
										   NodeData* nodedata)
{
	GtkWidget *item, *image;
	GtkWidget *checkitem, *separator;
	
	/* Destroy the old menu before creating a new one */
	if (nodedata->menu)
	{
		gtk_widget_destroy (nodedata->menu);
	}
		
	nodedata->menu = gtk_menu_new();
	if (nodedata->name && strlen (nodedata->name))
	{
		IAnjutaSymbolManager *sm;
		IAnjutaIterable *iter_searched;
		IAnjutaIterable *iter;
		IAnjutaSymbol *symbol_searched;
		sm = anjuta_shell_get_interface (ANJUTA_PLUGIN (nodedata->plugin)->shell,
										 IAnjutaSymbolManager, NULL);
		if (sm == NULL)
			return;
		
		/* we cannot pass a simple 'name' to get_members () interface. That
		 * wouldn't be enought to identify uniquely the symbol itself.
		 * Think for example at two namespaces with two classes with the same
		 * name... Anyway the situation here wasn't better even before. The problem
		 * persists and to be solved it's needed a re-engineering of class-inherit.
		 */
		iter_searched = ianjuta_symbol_manager_search (sm, IANJUTA_SYMBOL_TYPE_CLASS,
													   TRUE,
													   IANJUTA_SYMBOL_FIELD_SIMPLE,
													   nodedata->name,
													   FALSE,
													   TRUE,
													   NULL);
		
		if (iter_searched == NULL)
			return;
		
		symbol_searched = IANJUTA_SYMBOL (iter_searched);
		iter = ianjuta_symbol_manager_get_members (sm, symbol_searched,
												   IANJUTA_SYMBOL_FIELD_SIMPLE,
												   FALSE, NULL);
		if (iter && ianjuta_iterable_get_length (iter, NULL) > 0)
		{
			IAnjutaSymbol *symbol = IANJUTA_SYMBOL (iter);
			do
			{
				const gchar *name, *file;
				const GdkPixbuf *pixbuf;
				gint line;
				
				name = ianjuta_symbol_get_name (symbol, NULL);
				pixbuf = ianjuta_symbol_get_icon (symbol, NULL);
				file = ianjuta_symbol_get_extra_info_string (symbol, 
								IANJUTA_SYMBOL_FIELD_FILE_PATH, NULL);
				line = ianjuta_symbol_get_line (symbol, NULL);
				
				item = gtk_image_menu_item_new_with_label (name);
				image = gtk_image_new_from_pixbuf ((GdkPixbuf*)pixbuf);
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
			} while (ianjuta_iterable_next (iter, NULL));
		}
		if (iter)  
		{
			g_object_unref (iter);
		}
		if (iter_searched)
		{
			g_object_unref (iter_searched);
		}
			
	}
	
	
	separator = gtk_separator_menu_item_new ();
	/* create the check menuitem */
	checkitem = gtk_check_menu_item_new_with_label (_("Fixed data-view"));
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (checkitem),
									nodedata->anchored);
	
	g_signal_connect (G_OBJECT (checkitem), "toggled",
								G_CALLBACK (on_toggled_menuitem_clicked),
								nodedata);
	
	gtk_container_add (GTK_CONTAINER (nodedata->menu), separator);
	gtk_container_add (GTK_CONTAINER (nodedata->menu), checkitem);
		
	gtk_widget_show_all (nodedata->menu);
	gtk_menu_popup (GTK_MENU (nodedata->menu), NULL, NULL,
	                NULL, NULL, event->button.button,
					event->button.time);	
}
	
/*----------------------------------------------------------------------------
 * initialize the internal graphviz structure.
 */
static void
cls_inherit_graph_init (AnjutaClassInheritance *plugin, gchar* graph_label)
{
	aginit ();
	plugin->graph = agopen (graph_label, AGDIGRAPH);
	plugin->gvc = gvContext();
}

/*----------------------------------------------------------------------------
 * Perform a dot_cleanup and a graph closing. Call this function at the end of
 * call to draw_graph.
 */
static void
cls_inherit_graph_cleanup (AnjutaClassInheritance *plugin)
{
	if (plugin->graph != NULL)
	{
		gvFreeLayout (plugin->gvc, plugin->graph);
		agclose (plugin->graph);
	}

	if (plugin->gvc != NULL )
	{
		gvFreeContext (plugin->gvc);
	}
	
	plugin->graph = NULL;
	plugin->gvc = NULL;

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
	if (node_data->sub_item)
	{
		g_free (node_data->sub_item);
		node_data->sub_item = NULL;
	}
	node_data->anchored = FALSE;	
}

/*----------------------------------------------------------------------------
 * clean the canvas and all its painted objects.
 */
void
class_inheritance_clean_canvas (AnjutaClassInheritance *plugin)
{
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
 * add a node to an Agraph. Check also if the node is yet in the hash_table so 
 * that we can build the label of the node with the class-data.
 */
static gboolean
cls_inherit_add_node (AnjutaClassInheritance *plugin, const gchar* node_name)
{
	Agnode_t *node;
	Agsym_t *sym;
	NodeExpansionStatus *node_status;
	
	/* if graph isn't initialized, init it */
	if (!plugin->graph)
		cls_inherit_graph_init (plugin, _(DEFAULT_GRAPH_NAME));
	
	/* let's add the node to the graph */
	if ((node = agnode (plugin->graph, (gchar*)node_name)) == NULL)
		return FALSE;
	
	/* check for the node in the hash_table */
	if ( (node_status = 
				(NodeExpansionStatus*)g_hash_table_lookup 
				(plugin->expansion_node_list, node_name)) != NULL && 
				node_status->expansion_status != NODE_NOT_EXPANDED) {
		GString *label;
		gint max_label_items = 0;
		gint real_items_length = 0;
		IAnjutaSymbolManager *sm;

		if (!(sym = agfindattr(plugin->graph->proto->n, "shape")))
			sym = agnodeattr(plugin->graph, "shape", "");
		agxset(node, sym->index, "record");
		
		if (!(sym = agfindattr(plugin->graph->proto->n, "label")))
			sym = agnodeattr(plugin->graph, "label", "");
		
		label = g_string_new ("");
		g_string_printf (label, "{%s", node_name);
		
		sm = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
										 IAnjutaSymbolManager, NULL);
		if (sm)
		{
			IAnjutaIterable *iter;
			IAnjutaIterable *iter_searched;
			IAnjutaSymbol *symbol_searched;
			iter_searched = ianjuta_symbol_manager_search (sm, IANJUTA_SYMBOL_TYPE_CLASS,
													   TRUE,
													   IANJUTA_SYMBOL_FIELD_SIMPLE,
													   node_name,
													   FALSE,
													   TRUE,
													   NULL);
		
			if (iter_searched == NULL) {
				g_string_free (label, TRUE);
				return FALSE;
			}
		
			symbol_searched = IANJUTA_SYMBOL (iter_searched);
			
			iter = ianjuta_symbol_manager_get_members (sm, symbol_searched,
													   IANJUTA_SYMBOL_FIELD_SIMPLE,
													   FALSE, NULL);
			real_items_length = ianjuta_iterable_get_length (iter, NULL);

			/* set the max number of items to draw */
			if (real_items_length <= NODE_HALF_DISPLAY_ELEM_NUM || 
						node_status->expansion_status == NODE_FULL_EXPANDED) {
				max_label_items = real_items_length;
				node_status->expansion_status = NODE_FULL_EXPANDED;
			}
			else {
				max_label_items = NODE_HALF_DISPLAY_ELEM_NUM;
			}

			if (iter && real_items_length > 0)
			{
				gint i;
				i = 0;
				do
				{
					const gchar *name;
					IAnjutaSymbol *symbol = IANJUTA_SYMBOL (iter);
						
					name = ianjuta_symbol_get_name (symbol, NULL);
					g_string_append_printf (label, "|%s", name);
					i++;
				} while (ianjuta_iterable_next (iter, NULL) && i < max_label_items);
			}
			if (iter)
				g_object_unref (iter);
			
			if (iter_searched)
				g_object_unref (iter_searched);
		}
		
		if (node_status->expansion_status == NODE_HALF_EXPANDED &&
				real_items_length > NODE_HALF_DISPLAY_ELEM_NUM) {
			g_string_append_printf (label, "|%s", NODE_SHOW_ALL_MEMBERS_STR);
		}
		
		g_string_append_printf (label, "|%s }", NODE_SHOW_NORMAL_VIEW_STR);
		agxset(node, sym->index, label->str);
		
		g_string_free (label, TRUE);
	}
	else {	/* the node isn't in an expansion status. 
				 * Go on setting a regular one */

		/* Set an attribute - in this case one that affects the visible rendering */
		if (!(sym = agfindattr(plugin->graph->proto->n, "shape")))
			sym = agnodeattr(plugin->graph, "shape", "");
		agxset(node, sym->index, "box");
		
		if (!(sym = agfindattr(plugin->graph->proto->n, "label")))
			sym = agnodeattr(plugin->graph, "label", "");
		agxset(node, sym->index, node->name);		
	}

	/* set the font */
	if (!(sym = agfindattr(plugin->graph->proto->n, "fontname")))
		sym = agnodeattr(plugin->graph, "fontname", "");
	agxset(node, sym->index, "Courier new");

	/* set the font-size */	
	if (!(sym = agfindattr(plugin->graph->proto->n, "fontsize")))
		sym = agnodeattr(plugin->graph, "fontsize", "");
	/* hack: set canvas_text_fontsize + 4 points or text would oversize the block */
	/* add some more points for icons 16x16 space */
	agxset(node, sym->index, "17");

	if (!(sym = agfindattr(plugin->graph->proto->n, "ratio")))
		sym = agnodeattr(plugin->graph, "ratio", "");
	agxset(node, sym->index, "expand");	

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
	
	return TRUE;
}


/*----------------------------------------------------------------------------
 * Draw an expanded node. Function simplify cls_inherit_draw_graph().
 */
static void
cls_inherit_draw_expanded_node (AnjutaClassInheritance *plugin, Agnode_t *node, 
					point* node_pos, gdouble node_width, gdouble node_height) {
	GnomeCanvasItem *item;
	NodeData *node_data;
	NodeExpansionStatus *node_status;
	gint expansion_status;
	gint i, j;
	IAnjutaSymbolManager *sm;
	IAnjutaIterable *symbol_iter = NULL;
	
	sm = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
										 IAnjutaSymbolManager, NULL);
	if (!sm) {
		return;
	}
						
	IAnjutaIterable *iter_searched;
	IAnjutaSymbol *symbol_searched;
	iter_searched = ianjuta_symbol_manager_search (sm, IANJUTA_SYMBOL_TYPE_CLASS,
												   TRUE,
												   IANJUTA_SYMBOL_FIELD_SIMPLE,
												   node->name,
												   FALSE,
												   TRUE,
												   NULL);
		
	if (iter_searched == NULL) {
		return;
	}
		
	symbol_searched = IANJUTA_SYMBOL (iter_searched);
						
	symbol_iter = ianjuta_symbol_manager_get_members (sm, symbol_searched,
													  IANJUTA_SYMBOL_FIELD_SIMPLE,
													  FALSE, NULL);
						
	/* we need to know which label to draw, wether only the "show all" or just 
	 * the "normal view" */
	if ( (node_status = 
				(NodeExpansionStatus*)g_hash_table_lookup 
				(plugin->expansion_node_list, node->name)) == NULL) {
		expansion_status = NODE_NOT_EXPANDED;
	}
	else
		expansion_status = node_status->expansion_status;

	for (i=0; i < NODE_NUM_FIELDS (node); i++ ) {
		for (j=0; j < NODE_NTH_FIELD (node,i)->n_flds; j++ ) {
			gint y1, y2;			
			y1 = NODE_LOWER_LEFT(node,i,j).y;
			y2 = NODE_UPPER_RIGHT(node,i,j).y;

			node_data = g_new0 (NodeData, 1);

			/* set the plugin reference */
			node_data->plugin = plugin;
			node_data->anchored = TRUE;
			node_data->name = g_strdup (node->name);

			node_data->sub_item = g_strdup (NODE_NTH_TEXT (node,i,j));
			
			node_data->canvas_item =
			gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (plugin->canvas)),
								   gnome_canvas_rect_get_type (),
								   "x1",
								   (gdouble) (node_pos->x -INCH_TO_PIXELS (node_width)/2),
								   "y1",
								   (gdouble) -node_pos->y -INCH_TO_PIXELS (node_height)/2 +
									j*abs(y1-y2),
								   "x2",
								   (gdouble) (node_pos->x +INCH_TO_PIXELS (node_width)/2),
								   "y2",
								   (gdouble) -node_pos->y -INCH_TO_PIXELS (node_height)/2 +
									(j+1)*abs(y1-y2),
								   "fill_color_gdk",
								   &plugin->canvas->style->base[GTK_STATE_ACTIVE],
								   NULL);
									
			/* add to the nodelist: we'll set the __uri and __line properites later
			 * on this loop, when we'll parse symbols. */			
			plugin->node_list = g_list_prepend (plugin->node_list, node_data);

			g_signal_connect (GTK_OBJECT (node_data->canvas_item), "event",
							  G_CALLBACK (on_nodedata_expanded_event),
							  node_data);

			/* --- texts --- */		
			item = gnome_canvas_item_new (gnome_canvas_root
										  (GNOME_CANVAS (plugin->canvas)),
										  gnome_canvas_text_get_type (),
										  "text", NODE_NTH_TEXT (node,i,j),
										  "font", NODE_FONT_DEFAULT,
										  "justification", GTK_JUSTIFY_CENTER,
										  "style", PANGO_STYLE_ITALIC,
										  "anchor", GTK_ANCHOR_CENTER,
									  	  "x",
									  	  (gdouble) (node_pos->x - 
										  INCH_TO_PIXELS (node_width)/2 + 20),
									  	  "y", (gdouble) -node_pos->y -
										  INCH_TO_PIXELS (node_height)/2+(j+0.5)*abs(y1-y2),
									  	  "fill_color_gdk",
									  	  &plugin->canvas->style->text[GTK_STATE_ACTIVE],
									  	  "anchor", GTK_ANCHOR_W,        
									  	  NULL);
			plugin->drawable_list = g_list_prepend (plugin->drawable_list, item);
			
			if (j == 0) {		/* the Class' name case: make it bold */
				gnome_canvas_item_set (item, 
							"weight", PANGO_WEIGHT_BOLD, 
							"style", PANGO_STYLE_NORMAL,
							NULL);
				continue;
			}
			else					/* we need to draw the last 2 elements differently */
				if (expansion_status == NODE_HALF_EXPANDED && 
					j > (NODE_NTH_FIELD (node,i)->n_flds -3)) {
					gnome_canvas_item_set (item, 
								"weight", PANGO_WEIGHT_HEAVY, 
								"style", PANGO_STYLE_NORMAL,
								NULL);						
					continue;
				}
				else				/* only the last one. Usually "Normal view" */
					if (expansion_status == NODE_FULL_EXPANDED && 
						j > (NODE_NTH_FIELD (node,i)->n_flds -2)) {
						gnome_canvas_item_set (item, 
									"weight", PANGO_WEIGHT_HEAVY,
									"style", PANGO_STYLE_NORMAL,
									NULL);
						continue;
					}
					
			/* go on with the icons */
			if (symbol_iter && ianjuta_iterable_get_length (symbol_iter, NULL) > 0) {
				const GdkPixbuf *pixbuf;
				const gchar* uri;
				gint line;
				IAnjutaSymbol *symbol = IANJUTA_SYMBOL (symbol_iter);

				uri = ianjuta_symbol_get_uri (symbol, NULL);
				line = ianjuta_symbol_get_line (symbol, NULL);
				pixbuf = ianjuta_symbol_get_icon (symbol, NULL);
				
				item = gnome_canvas_item_new (	gnome_canvas_root
									(GNOME_CANVAS (plugin->canvas)),
									gnome_canvas_pixbuf_get_type(),
									"x",
									(gdouble) (node_pos->x -
									INCH_TO_PIXELS (node_width)/2 +2),
									"y", 
									(gdouble) -node_pos->y -
									INCH_TO_PIXELS (node_height)/2+(j+0.5)*abs(y1-y2)-5,
									"pixbuf", pixbuf,
									NULL);

				/* set now the object properties on node_data. We still have a 
				 * reference to it so we can access its canvas_item */
				if (uri) {
					g_object_set_data_full (G_OBJECT (node_data->canvas_item), "__uri",
											g_strdup (uri), g_free);
					g_object_set_data (G_OBJECT (node_data->canvas_item), "__line",
									   GINT_TO_POINTER (line));
				}
			}
			plugin->drawable_list = g_list_prepend (plugin->drawable_list, item);
			ianjuta_iterable_next (symbol_iter, NULL);
		}
	}
	
	if (symbol_iter)
		g_object_unref (symbol_iter);
	
	if (iter_searched)
		g_object_unref (iter_searched);
						
	/* make the outline bounds */
	item =
	gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (plugin->canvas)),
							gnome_canvas_rect_get_type (),
							"x1",
							(gdouble) (node_pos->x -INCH_TO_PIXELS (node_width)/2-1 ),
							"y1",
							(gdouble) -node_pos->y -INCH_TO_PIXELS (node_height)/2 -1,
							"x2",
							(gdouble) node_pos->x +INCH_TO_PIXELS (node_width)/2+1,
							"y2",
							(gdouble) -node_pos->y +INCH_TO_PIXELS (node_height)/2 -1,
  						   "outline_color_gdk",
							&plugin->canvas->style->text[GTK_STATE_ACTIVE],
							"width_units", 1.0,
							NULL);

	plugin->drawable_list = g_list_prepend (plugin->drawable_list, item);
}

static void
cls_inherit_draw_single_node (AnjutaClassInheritance *plugin, Agnode_t *node, 
					point *node_pos, gdouble node_width, gdouble node_height) {
	NodeData *node_data;
	GnomeCanvasItem *item;
	gdouble text_width_value;

	node_data = g_new0 (NodeData, 1);
		
	/* set the plugin reference */
	node_data->plugin = plugin;
	node_data->anchored = FALSE;
	node_data->name = g_strdup (node->name);
	node_data->sub_item = NULL;
		
	node_data->canvas_item =
	gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (plugin->canvas)),
						   gnome_canvas_rect_get_type (),
						   "x1",
						   (gdouble) (node_pos->x - INCH_TO_PIXELS (node_width)/2),
						   "y1",
						   (gdouble) -(node_pos->y - INCH_TO_PIXELS (node_height)/2),
						   "x2",
						   (gdouble) (node_pos->x + INCH_TO_PIXELS (node_width)/2),
						   "y2",
						   (gdouble) -(node_pos->y + INCH_TO_PIXELS (node_height)/2),
						   "fill_color_gdk",
						   &plugin->canvas->style->base[GTK_STATE_NORMAL],
						   "outline_color_gdk",
						   &plugin->canvas->style->text[GTK_STATE_NORMAL],
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
								  "text", node->name,
								  "font", NODE_FONT_DEFAULT,
								  "justification", GTK_JUSTIFY_CENTER,
								  "anchor", GTK_ANCHOR_CENTER,
								  "x",
								  (gdouble) (node_pos->x - INCH_TO_PIXELS (node_width)/2),
								  "y", (gdouble) -node_pos->y,
								  "fill_color_gdk",
								  &plugin->canvas->style->text[GTK_STATE_NORMAL],
								  "anchor", GTK_ANCHOR_W,        
								  NULL );
		
	/* center the text in the node... */
	g_object_get (item, "text_width", &text_width_value, NULL);
						
	gnome_canvas_item_set (item, "x",
						   (gdouble)((node_pos->x - text_width_value/2)), NULL);

	plugin->drawable_list = g_list_prepend (plugin->drawable_list, item);
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
	
	if (plugin->graph == NULL)
		return;
	
	DEBUG_PRINT ("======== going to draw graph ========");
	num_nodes = agnnodes (plugin->graph);	
	g_return_if_fail (num_nodes > 0);
	
	/* compiles nodes/edges informations, such as positions, coordinates etc */
	gvLayout (plugin->gvc, plugin->graph, "dot");
	//dot_layout (plugin->graph);
	
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
		point node_pos;
		
		/* get some infos from the node */
		node_pos = ND_coord_i(node);
		node_width = ND_width (node);
		node_height = ND_height (node);

		if (strcmp ("record", ND_shape (node)->name) == 0 ) {
			cls_inherit_draw_expanded_node (plugin, node, &node_pos, node_width, node_height);
		}
		else			/* it's a normal single node */
			cls_inherit_draw_single_node (plugin, node, &node_pos, node_width, node_height);

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
					gdouble upper_bound = (gdouble)(node_pos.y +
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
							  &plugin->canvas->style->text[GTK_STATE_NORMAL],
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
									   &plugin->canvas->style->text[GTK_STATE_NORMAL],
									   "width_pixels", 1,
									   NULL);
			plugin->drawable_list =
				g_list_prepend (plugin->drawable_list, item);
		}

		if (abs(node_pos.x) > max_canvas_size_x ) 
			max_canvas_size_x = abs(node_pos.x) + INCH_TO_PIXELS (node_width)/2;
		
		if (abs(node_pos.y + node_height) > max_canvas_size_y) 
			max_canvas_size_y = abs(node_pos.y) + INCH_TO_PIXELS (node_height)/2;
	}	
	
	gtk_widget_set_size_request (plugin->canvas, max_canvas_size_x +100,
								 max_canvas_size_y +100);
	gnome_canvas_set_scroll_region ( GNOME_CANVAS (plugin->canvas), -50,
									50, max_canvas_size_x + 50,
									-max_canvas_size_y -100);
	
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

	g_return_if_fail (plugin != NULL);

	if (plugin->top_dir == NULL)
		return;
	
	sm = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
									 IAnjutaSymbolManager, NULL);
	if (!sm)
		return;
	
	iter = ianjuta_symbol_manager_search (sm, IANJUTA_SYMBOL_TYPE_CLASS, 
										  TRUE,
										  IANJUTA_SYMBOL_FIELD_SIMPLE,
										  NULL, FALSE, TRUE, NULL);
	if (!iter)
	{
		DEBUG_PRINT ("class_inheritance_update_graph (): search returned no items.");
		return;
	}
	
	DEBUG_PRINT ("Number of classes found = %d",
				 ianjuta_iterable_get_length (iter, NULL));	
/*	
	do {
		const gchar *class_name;
		symbol = IANJUTA_SYMBOL (iter);
		class_name = ianjuta_symbol_get_name (symbol, NULL);
		DEBUG_PRINT ("=======> %s", class_name);
	}
	while (ianjuta_iterable_next (iter, NULL) == TRUE);
*/	
	ianjuta_iterable_first (iter, NULL);
	if (ianjuta_iterable_get_length (iter, NULL) <= 0)
	{
		g_object_unref (iter);
		return;
	}
	
	/* Get all classes */
	classes = NULL;
	class_parents = g_hash_table_new_full (g_str_hash, g_str_equal,
										   g_free, g_object_unref);
	do {
		const gchar *class_name, *old_parents;
		IAnjutaIterable *parents;

		symbol = IANJUTA_SYMBOL (iter);
		
		/* get parents of the current class */
		parents = ianjuta_symbol_manager_get_class_parents (sm, symbol, 
															IANJUTA_SYMBOL_FIELD_SIMPLE, 
															NULL);

		if (parents == NULL || ianjuta_iterable_get_length (parents, NULL) <= 0)
		{
			DEBUG_PRINT ("continuing 1...");
			continue;
		}
		
		class_name = ianjuta_symbol_get_name (symbol, NULL);
		
		if ((old_parents = g_hash_table_lookup (class_parents, class_name)))
		{
			/* we already have a class inserted with that name */
			DEBUG_PRINT ("continuing 2...");
			continue;
		}
		
		DEBUG_PRINT ("parsed %s", class_name);
		/* insert into the hash table a class name together with the associated parents */
		g_hash_table_insert (class_parents, g_strdup (class_name), parents);
		classes = g_list_prepend (classes, g_strdup (class_name));
	} while (ianjuta_iterable_next (iter, NULL) == TRUE);
	
	classes = g_list_reverse (classes);
	
	/* we don't need the iter anymore */
	g_object_unref (iter);
	
	/* For all classes get their parents */
	node = classes;
	while (node)
	{
		const gchar *class_name;
		IAnjutaIterable *parents;
				
		class_name = node->data;
		parents = g_hash_table_lookup (class_parents, class_name);
		
		do {
			IAnjutaSymbol *symbol;
			symbol = IANJUTA_SYMBOL (parents);
			const gchar *parent_name;
			
			parent_name = ianjuta_symbol_get_name (symbol, NULL);
			
			cls_inherit_add_node (plugin, class_name);
			cls_inherit_add_node (plugin, parent_name);
			cls_inherit_add_edge (plugin, parent_name, class_name);
		} while (ianjuta_iterable_next (parents, NULL) == TRUE);

		/* parse next deriver class in the glist */
		node = g_list_next (node);
	}
	g_list_foreach (classes, (GFunc)g_free, NULL);
	g_list_free (classes);
	g_hash_table_destroy (class_parents);
	cls_inherit_draw_graph (plugin);
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
class_inheritance_hash_table_clear (AnjutaClassInheritance *plugin) {
	
	if (plugin->expansion_node_list == NULL)
		return;
	
	/* destroy the nodestatus hash table */
	g_hash_table_destroy (plugin->expansion_node_list);
	
	/* reinitialize the table */
	plugin->expansion_node_list = g_hash_table_new_full (g_str_hash, 
												g_str_equal, g_free, 
												(GDestroyNotify)cls_inherit_nodestatus_destroy);	
}

void
class_inheritance_base_gui_init (AnjutaClassInheritance *plugin)
{
	GtkWidget *s_window;

	s_window = gtk_scrolled_window_new (NULL, NULL);
	plugin->canvas = gnome_canvas_new_aa ();
	//gtk_widget_modify_bg (plugin->canvas, GTK_STATE_NORMAL,
	//					  &plugin->canvas->style->base[GTK_STATE_NORMAL]);
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
	
	plugin->expansion_node_list = g_hash_table_new_full (g_str_hash, 
												g_str_equal, g_free, 
												(GDestroyNotify)cls_inherit_nodestatus_destroy);

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
