/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 *  Copyright (C) Massimo Cora' 2005 <maxcvs@email.it>
 *                2010 Naba Kumar <naba@gnome.org>
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

#define GRAPH_TO_CANVAS_Y(y) (-y)

#define INCH_TO_PIXELS_CONVERSION_FACTOR		72
#define INCH_TO_PIXELS(inch_size) \
				INCH_TO_PIXELS_CONVERSION_FACTOR * inch_size

#define NODE_EDGE_ARROW_LENGTH 10

/* TODO: check for symbol_updated event, and check in the nodestatus's hashtable
for the nodes that are gone. In case remove them.
*/

typedef enum {
	CLS_ARROW_DIR_UP,
	CLS_ARROW_DIR_DOWN,
	CLS_ARROW_DIR_LEFT,
	CLS_ARROW_DIR_RIGHT
} CanvasArrowDir;

typedef struct {
	gdouble x1, y1, x2, y2;
} ClsBox;

static GnomeCanvasItem*
create_canvas_arrow_item (GnomeCanvasGroup *canvas_group,
                          CanvasArrowDir direction,
                          const GdkColor *fill_color,
                          /* Bounding box */
                          gint x1, gint y1, gint x2, gint y2)
{
	GnomeCanvasItem *item;
	const gint offset = 4;

	/* FIXME: Use direction to draw different arrows, now it only is down */
	/* Arrows */
	GnomeCanvasPoints *triangle = gnome_canvas_points_new (4);
	triangle->coords[0] = x1 + offset;
	triangle->coords[1] = y1 + offset;
	triangle->coords[2] = x2 - offset;
	triangle->coords[3] = y1 + offset;
	triangle->coords[4] = x1 + (x2 - x1)/2;
	triangle->coords[5] = y2 - offset;
	triangle->coords[6] = x1 + offset;
	triangle->coords[7] = y1 + offset;

	item = gnome_canvas_item_new (canvas_group,
	                              gnome_canvas_polygon_get_type (),
	                              "points", triangle,
	                              "fill_color_gdk",
	                              fill_color,
	                              NULL);
	gnome_canvas_points_unref (triangle);
	return item;
}

static GnomeCanvasItem*
create_canvas_line_item (GnomeCanvasGroup *canvas_group, GdkColor *fill_color,
                          gint x1, gint y1, gint x2, gint y2)
{
	GnomeCanvasItem *item;
	GnomeCanvasPoints *points;

	points = gnome_canvas_points_new (2);
	points->coords[0] = x1;
	points->coords[1] = y1;
	points->coords[2] = x2;
	points->coords[3] = y2;

	item = 
		gnome_canvas_item_new (canvas_group,
				               gnome_canvas_line_get_type(),
				               "points", points,
				               "fill_color_gdk",
				               fill_color,
				               "width_units", 1.0,
				               NULL);
	gnome_canvas_points_unref (points);
	return item;
}

static void
cls_node_item_free (ClsNodeItem *cls_item)
{
	g_free (cls_item->label);
	g_free (cls_item->args);
	g_free (cls_item->type_name);

	if (cls_item->file)
		g_object_unref (cls_item->file);
	if (cls_item->icon)
		gdk_pixbuf_unref (cls_item->icon);
	
	if (cls_item->tooltip_timeout)
		g_source_remove (cls_item->tooltip_timeout);

	if (cls_item->tooltip)
		gtk_object_destroy (GTK_OBJECT (cls_item->tooltip));

	g_free (cls_item);
}

static void
cls_node_edge_free (ClsNodeEdge *cls_edge)
{
	/* Delete agedeges here? */
	gtk_object_destroy (GTK_OBJECT (cls_edge->canvas_line));
	gtk_object_destroy (GTK_OBJECT (cls_edge->canvas_arrow));
	agdelete (cls_edge->cls_node_from->graph, cls_edge->agedge);
	g_free (cls_edge);
}

static void
cls_node_free (ClsNode *cls_node)
{
	g_free (cls_node->sym_name);
	g_hash_table_destroy (cls_node->edges_to);
	g_hash_table_destroy (cls_node->edges_from);
	g_hash_table_destroy (cls_node->members);
	if (cls_node->canvas_group)
		gtk_object_destroy (GTK_OBJECT (cls_node->canvas_group));
	agdelete (cls_node->graph, cls_node->agnode);
	g_free (cls_node);
}

static gboolean
cls_node_add_edge (ClsNode *cls_node_from, ClsNode *cls_node_to)
{
	ClsNodeEdge *cls_edge;

	g_return_val_if_fail (cls_node_from->graph != NULL, FALSE);

	/* Check if the edge already exists */
	cls_edge = g_hash_table_lookup (cls_node_from->edges_to, cls_node_to);
	if (cls_edge)
		return TRUE;
	
	cls_edge = g_new0 (ClsNodeEdge, 1);
	cls_edge->cls_node_from = cls_node_from;
	cls_edge->cls_node_to = cls_node_to;
	
	if ((cls_edge->agedge = agedge (cls_node_from->graph,
	                                cls_node_from->agnode,
	                                cls_node_to->agnode)) == NULL)
	{
		g_free (cls_edge);
		return FALSE;
	}

	g_hash_table_insert (cls_node_from->edges_to, cls_node_to, cls_edge);
	g_hash_table_insert (cls_node_to->edges_from, cls_node_from, cls_edge);
	return TRUE;
}

static void
on_cls_node_unlink_from_foreach (ClsNode *cls_node_from, ClsNodeEdge *cls_edge,
                            ClsNode *cls_node_to)
{
	g_hash_table_remove (cls_node_from->edges_to, cls_node_to);
}

static void
on_cls_node_unlink_to_foreach (ClsNode *cls_node_to, ClsNodeEdge *cls_edge,
                            ClsNode *cls_node_from)
{
	/* This is will also free cls_edge */
	g_hash_table_remove (cls_node_to->edges_from, cls_node_from);
}

static void
cls_node_unlink (ClsNode *cls_node)
{
	g_hash_table_foreach (cls_node->edges_from,
	                      (GHFunc) on_cls_node_unlink_from_foreach,
	                      cls_node);
	g_hash_table_foreach (cls_node->edges_to,
	                      (GHFunc) on_cls_node_unlink_to_foreach,
	                      cls_node);
}

/*----------------------------------------------------------------------------
 * add a node to an Agraph. Check also if the node is yet in the hash_table so 
 * that we can build the label of the node with the class-data.
 */
static ClsNode*
cls_inherit_create_node (AnjutaClassInheritance *plugin,
                         const IAnjutaSymbol *node_sym)
{
	ClsNode *cls_node;
	Agsym_t *sym;
	gint font_size;
	const gchar *font_name;
	
#define FONT_SIZE_STR_LEN 16
	gchar font_size_str[FONT_SIZE_STR_LEN];

	cls_node = g_new0 (ClsNode, 1);
	cls_node->graph = plugin->graph;
	cls_node->canvas = plugin->canvas;
	cls_node->plugin = plugin;
	cls_node->sym_manager =
			anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
			                            IAnjutaSymbolManager, NULL);

	cls_node->sym_name =
		g_strdup (ianjuta_symbol_get_name (IANJUTA_SYMBOL (node_sym), NULL));
	cls_node->klass_id =
		ianjuta_symbol_get_id (IANJUTA_SYMBOL (node_sym), NULL);
	cls_node->members =
		g_hash_table_new_full (g_str_hash, g_str_equal,
		                       (GDestroyNotify) g_free,
		                       (GDestroyNotify) cls_node_item_free);
	cls_node->expansion_status = CLS_NODE_COLLAPSED;
	cls_node->drawn_expansion_status = CLS_NODE_COLLAPSED;
	cls_node->edges_to =
		g_hash_table_new_full (g_direct_hash, g_direct_equal,
		                       NULL, (GDestroyNotify)cls_node_edge_free);
	cls_node->edges_from = g_hash_table_new (g_direct_hash, g_direct_equal);
	
	/* let's add the node to the graph */
	if ((cls_node->agnode = agnode (cls_node->graph,
	                                cls_node->sym_name)) == NULL)
	{
		cls_node_free (cls_node);
		return NULL;
	}
	
	/* set the font */
	if (!(sym = agfindattr(plugin->graph->proto->n, "fontname")))
		sym = agnodeattr(plugin->graph, "fontname", "");
	font_name =
		pango_font_description_get_family (plugin->canvas->style->font_desc);
	agxset(cls_node->agnode, sym->index, (char*)font_name);

	/* set the font-size */	
	if (!(sym = agfindattr(plugin->graph->proto->n, "fontsize")))
		sym = agnodeattr(plugin->graph, "fontsize", "");

	font_size =
		pango_font_description_get_size (plugin->canvas->style->font_desc)/
			PANGO_SCALE;

	/* The above font size in points is with real screen DPI, but graphviz
	 * rendering is done at fixed INCH_TO_PIXELS_CONVERSION_FACTOR dpi. So
	 * convert to the right font size points for graphviz.
	 */
	font_size =
		font_size * gdk_screen_get_resolution (gdk_screen_get_default ())
					/ INCH_TO_PIXELS_CONVERSION_FACTOR;
	
	snprintf (font_size_str, FONT_SIZE_STR_LEN, "%d", font_size);
	agxset(cls_node->agnode, sym->index, font_size_str);

	if (!(sym = agfindattr(plugin->graph->proto->n, "ratio")))
		sym = agnodeattr(plugin->graph, "ratio", "");
	agxset(cls_node->agnode, sym->index, "expand");	

	/* Set an attribute - in this case one that affects the visible rendering */
	if (!(sym = agfindattr(plugin->graph->proto->n, "shape")))
		sym = agnodeattr(plugin->graph, "shape", "");
	agxset(cls_node->agnode, sym->index, "box");
	
	if (!(sym = agfindattr(plugin->graph->proto->n, "label")))
		sym = agnodeattr(plugin->graph, "label", "");
	agxset(cls_node->agnode, sym->index, cls_node->sym_name);

	return cls_node;
}

gboolean
cls_node_collapse (ClsNode *cls_node)
{
	Agsym_t *sym;
	
	if (cls_node->expansion_status == CLS_NODE_COLLAPSED)
		return FALSE;

	if (!(sym = agfindattr(cls_node->graph->proto->n, "label")))
		sym = agnodeattr(cls_node->graph, "label", "");
	agxset(cls_node->agnode, sym->index, cls_node->sym_name);
	cls_node->expansion_status = CLS_NODE_COLLAPSED;

	return TRUE;
}

gboolean
cls_node_expand (ClsNode *cls_node, ClsNodeExpansionType expansion_type)
{
	Agsym_t *sym;
	GString *label;
	gint max_label_items = 0;
	gint real_items_length = 0;
	gint var_order = -1000;
	gint method_order = 0;
	IAnjutaSymbol *node_sym;
	IAnjutaIterable *iter;
	
	if (cls_node->expansion_status == expansion_type ||
	    expansion_type == CLS_NODE_COLLAPSED)
		return FALSE;

	node_sym =
		ianjuta_symbol_manager_get_symbol_by_id (cls_node->sym_manager,
		                                         cls_node->klass_id,
		                                         IANJUTA_SYMBOL_FIELD_SIMPLE,
		                                         NULL);
	if (!node_sym)
		return FALSE;
	
	if (!(sym = agfindattr(cls_node->graph->proto->n, "shape")))
		sym = agnodeattr(cls_node->graph, "shape", "");
	agxset (cls_node->agnode, sym->index, "record");
	
	if (!(sym = agfindattr(cls_node->graph->proto->n, "label")))
		sym = agnodeattr(cls_node->graph, "label", "");
	
	label = g_string_new ("");
	g_string_printf (label, "{%s", cls_node->sym_name);
	
	/* get members from the passed symbol node */
	iter = ianjuta_symbol_manager_get_members (cls_node->sym_manager, node_sym,
	                                           IANJUTA_SYMBOL_FIELD_SIMPLE|
	                                           IANJUTA_SYMBOL_FIELD_KIND |
	                                           IANJUTA_SYMBOL_FIELD_TYPE |
	                                           IANJUTA_SYMBOL_FIELD_TYPE_NAME |
	                                           IANJUTA_SYMBOL_FIELD_ACCESS |
	                                           IANJUTA_SYMBOL_FIELD_FILE_PATH,
	                                           NULL);
	real_items_length = ianjuta_iterable_get_length (iter, NULL);
	
	/* set the max number of items to draw */
	if (real_items_length <= NODE_HALF_DISPLAY_ELEM_NUM || 
	    expansion_type == CLS_NODE_FULL_EXPANDED) 
	{
		max_label_items = real_items_length;
		cls_node->expansion_status = CLS_NODE_FULL_EXPANDED;
	}
	else
	{
		max_label_items = NODE_HALF_DISPLAY_ELEM_NUM;
		cls_node->expansion_status = CLS_NODE_SEMI_EXPANDED;
	}

	g_hash_table_remove_all (cls_node->members);
	if (iter && real_items_length > 0)
	{
		gint i = 0;

		/* First member variables */
		do
		{
			const gchar *name, *args, *type_name;
			IAnjutaSymbol *symbol;
			GdkPixbuf *icon;
			
			symbol = IANJUTA_SYMBOL (iter);
			name = g_strdup (ianjuta_symbol_get_name (symbol, NULL));
			args = ianjuta_symbol_get_args (symbol, NULL);
			icon = (GdkPixbuf*) ianjuta_symbol_get_icon (symbol, NULL);
			
			if (!args) /* Member variables */
			{
				ClsNodeItem *cls_item = g_new0 (ClsNodeItem, 1);

				type_name = ianjuta_symbol_get_extra_info_string (symbol, IANJUTA_SYMBOL_FIELD_TYPE_NAME, NULL);
				cls_item->cls_node = cls_node;
				cls_item->label = g_strconcat (name, " : ", type_name, NULL);
				cls_item->order = var_order++;
				if (icon)
					gdk_pixbuf_ref (icon);
				cls_item->icon = icon;
				
				g_hash_table_insert (cls_node->members,
				                     g_strdup (cls_item->label),
				                     cls_item);
				g_string_append_printf (label, "|%s", cls_item->label);

				/* Setup file and line */
				cls_item->type_name = g_strdup (type_name);
				cls_item->line = ianjuta_symbol_get_line (symbol, NULL);
				cls_item->file = ianjuta_symbol_get_file (symbol, NULL);
				g_object_ref (cls_item->file);
			}
			else /* Member methods */
			{
				ClsNodeItem *cls_item;
				gchar *method_key = g_strconcat (name, args, NULL);
				cls_item = g_hash_table_lookup (cls_node->members, method_key);

				if (cls_item) /* We found an entry for this method */
				{
					IAnjutaSymbolType sym_type
						= ianjuta_symbol_get_sym_type (symbol, NULL);
					if (!(sym_type & IANJUTA_SYMBOL_TYPE_PROTOTYPE))
					{
						/* This one is method, so take this one instead */
						g_free (cls_item->args);
						cls_item->args = g_strdup (args);

						if (cls_item->file) g_object_unref (cls_item->file);
						cls_item->file = NULL;

						/* Setup file and line */
						cls_item->line = ianjuta_symbol_get_line (symbol, NULL);
						cls_item->file = ianjuta_symbol_get_file (symbol, NULL);
						g_object_ref (cls_item->file);
					}
				}
				else /* We did not find a member entry, create a new one */
				{
					ClsNodeItem *cls_item = g_new0 (ClsNodeItem, 1);
					type_name = ianjuta_symbol_get_returntype (symbol, NULL);

					cls_item->cls_node = cls_node;
					if (type_name)
					{
						if (strlen (args) > 2)
							cls_item->label = g_strconcat (name, "(...)",
							                               " : ", type_name,
							                               NULL);
						else
							cls_item->label = g_strconcat (name, "()", " : ",
							                               type_name, NULL);
					}
					else
					{
						if (strlen (args) > 2)
							cls_item->label = g_strconcat (name, "(...)", NULL);
						else
							cls_item->label = g_strconcat (name, "()", NULL);
					}
					cls_item->args = g_strdup (args);
					cls_item->type_name = g_strdup (type_name);
					cls_item->order = method_order++;
					if (icon)
						gdk_pixbuf_ref (icon);
					cls_item->icon = icon;
					
					g_string_append_printf (label, "|%s", cls_item->label);
					g_hash_table_insert (cls_node->members, method_key,
					                     cls_item);
					
					/* Setup file and line */
					cls_item->line = ianjuta_symbol_get_line (symbol, NULL);
					cls_item->file = ianjuta_symbol_get_file (symbol, NULL);
					g_object_ref (cls_item->file);
				}
			}
			i++;
		}
		while (ianjuta_iterable_next (iter, NULL) && i < max_label_items);
	}
	if (iter)
		g_object_unref (iter);
	
	if (cls_node->expansion_status == CLS_NODE_SEMI_EXPANDED &&
	    real_items_length > NODE_HALF_DISPLAY_ELEM_NUM)
	{
		g_string_append_printf (label, "|%s", NODE_SHOW_ALL_MEMBERS_STR);
	}
	
	g_string_append_printf (label, "}");
	agxset(cls_node->agnode, sym->index, label->str);
	
	/* set the margin for icons */
	if (!(sym = agfindattr(cls_node->graph->proto->n, "margin")))
		sym = agnodeattr(cls_node->graph, "margin", "0.11,0.055");
	agxset(cls_node->agnode, sym->index, "0.3,0.03");

	g_string_free (label, TRUE);

	return TRUE;
}

static gint
on_cls_node_item_compare (ClsNodeItem *a, ClsNodeItem *b)
{
	if (a->order > b->order) return 1;
	if (a->order < b->order) return -1;
	else return 0;
}

/*----------------------------------------------------------------------------
 * Draw an expanded node. Function which simplifies cls_inherit_draw_graph().
 */
static void
cls_node_draw_expanded (ClsNode *cls_node)
{
	GnomeCanvasItem *canvas_item, *text_item;
	gint item_height, j;
	GList *members, *member;

	g_return_if_fail (cls_node->sym_manager != NULL);
	g_return_if_fail (cls_node->expansion_status == CLS_NODE_SEMI_EXPANDED ||
	                  cls_node->expansion_status == CLS_NODE_FULL_EXPANDED);
		
	if (cls_node->canvas_group)
		gtk_object_destroy (GTK_OBJECT (cls_node->canvas_group));
	cls_node->canvas_group =
		gnome_canvas_item_new (gnome_canvas_root
		                       (GNOME_CANVAS (cls_node->canvas)),
		                       gnome_canvas_group_get_type (),
		                       NULL);
	cls_node->drawn_expansion_status = cls_node->expansion_status;
	members = g_hash_table_get_values (cls_node->members);
	members = g_list_sort (members, (GCompareFunc)on_cls_node_item_compare);

	if (cls_node->expansion_status == CLS_NODE_SEMI_EXPANDED)
		item_height = cls_node->height / (g_list_length (members) + 2);
	else
		item_height = cls_node->height / (g_list_length (members) + 1);

	/* Class title */
	canvas_item =
		gnome_canvas_item_new (GNOME_CANVAS_GROUP (cls_node->canvas_group),
		                       gnome_canvas_rect_get_type (),
		                       "x1", 0.0,
		                       "y1", 0.0,
		                       "x2", (gdouble) cls_node->width,
		                       "y2", (gdouble) item_height,
		                       "fill_color_gdk",
		                       &cls_node->plugin->style[STYLE_TITLE_BG],
		                       NULL);

	g_signal_connect (GTK_OBJECT (canvas_item), "event",
	                  G_CALLBACK (on_expanded_class_title_event),
					  cls_node);
	
	/* Arrow on the right of class title */
	create_canvas_arrow_item (GNOME_CANVAS_GROUP (cls_node->canvas_group),
	                          CLS_ARROW_DIR_DOWN,
	                          &cls_node->plugin->style[STYLE_TITLE_FG],
	                          cls_node->width - item_height,
	                          0, cls_node->width, item_height);
	/* Class title text */
	text_item = 
		gnome_canvas_item_new (GNOME_CANVAS_GROUP (cls_node->canvas_group),
			                   gnome_canvas_text_get_type (),
			                   "text", cls_node->sym_name,
			                   "justification", GTK_JUSTIFY_CENTER,
			                   "anchor", GTK_ANCHOR_CENTER,
			                   "x", (gdouble) 20.0,
			                   "y", (gdouble) (j + 0.5) * item_height,
			                   "fill_color_gdk",
			                   &cls_node->plugin->style[STYLE_TITLE_FG],
			                   "anchor", GTK_ANCHOR_W,
			                   NULL);
	g_object_set_data (G_OBJECT (canvas_item), "__text__", text_item);
	g_signal_connect (GTK_OBJECT (text_item), "event",
	                  G_CALLBACK (on_canvas_event_proxy),
					  canvas_item);
	
	/* Member items */
	j = 1;
	for (member = members; member; member = member->next) 
	{
		ClsNodeItem *node_item = (ClsNodeItem*) member->data;

		/* Member item background */
		node_item->canvas_node_item =
			gnome_canvas_item_new (GNOME_CANVAS_GROUP (cls_node->canvas_group),
				                   gnome_canvas_rect_get_type (),
				                   "x1", 0.0,
				                   "y1", (gdouble) j * item_height,
				                   "x2", (gdouble) cls_node->width,
				                   "y2", (gdouble) (j + 1) * item_height,
				                   "fill_color_gdk",
				                   &cls_node->plugin->style[STYLE_ITEM_BG],
				                   NULL);
		g_signal_connect (GTK_OBJECT (node_item->canvas_node_item),
			              "event",
						  G_CALLBACK (on_expanded_class_item_event),
						  node_item);
	
		/* Member item text */
		text_item =
			gnome_canvas_item_new (GNOME_CANVAS_GROUP (cls_node->canvas_group),
			                       gnome_canvas_text_get_type (),
			                       "text", node_item->label,
			                       "justification", GTK_JUSTIFY_CENTER,
			                       "anchor", GTK_ANCHOR_CENTER,
			                       "x", (gdouble) 20.0,
			                       "y", (gdouble) (j + 0.5) * item_height,
			                       "fill_color_gdk",
			                       &cls_node->plugin->style[STYLE_ITEM_FG],
			                       "anchor", GTK_ANCHOR_W,
			                       NULL);
		g_object_set_data (G_OBJECT (node_item->canvas_node_item),
		                   "__text__", text_item);
		g_signal_connect (GTK_OBJECT (text_item), "event",
		                  G_CALLBACK (on_canvas_event_proxy),
		                  node_item->canvas_node_item);

		/* Member item icon */
		if (node_item->icon)
			gnome_canvas_item_new (GNOME_CANVAS_GROUP (cls_node->canvas_group),
					               gnome_canvas_pixbuf_get_type(),
					               "x", 2.0,
					               "y",
					               (gdouble) (j + 0.5) * item_height - 8,
					               "pixbuf", node_item->icon,
					               NULL);
		if (node_item->order == 0 || j == 1) /* Variables and methods border */
				create_canvas_line_item (GNOME_CANVAS_GROUP (cls_node->canvas_group),
				                         &cls_node->plugin->style[STYLE_FG],
				                         0, j * item_height,
				                         cls_node->width, j * item_height);
		j++;
	}
	g_list_free (members);
	
	/* The last item for semi expanded item is "more" item */
	if (cls_node->expansion_status == CLS_NODE_SEMI_EXPANDED) 
	{
		/* More expand item background */
		canvas_item =
			gnome_canvas_item_new (GNOME_CANVAS_GROUP (cls_node->canvas_group),
				                   gnome_canvas_rect_get_type (),
				                   "x1", 0.0,
				                   "y1", (gdouble) j * item_height,
				                   "x2", (gdouble) cls_node->width,
				                   "y2", (gdouble) (j + 1) * item_height,
				                   "fill_color_gdk",
				                   &cls_node->plugin->style[STYLE_TITLE_BG],
				                   NULL);

		g_signal_connect (GTK_OBJECT (canvas_item), "event",
			              G_CALLBACK (on_expanded_class_more_event),
						  cls_node);
	
		/* More expand item text */
		text_item = 
			gnome_canvas_item_new (GNOME_CANVAS_GROUP (cls_node->canvas_group),
					               gnome_canvas_text_get_type (),
					               "text",  NODE_SHOW_ALL_MEMBERS_STR,
					               "justification", GTK_JUSTIFY_CENTER,
					               "anchor", GTK_ANCHOR_CENTER,
					               "x", (gdouble) 20.0,
					               "y", (gdouble) (j + 0.5) * item_height,
					               "fill_color_gdk",
					               &cls_node->plugin->style[STYLE_TITLE_FG],
					               "anchor", GTK_ANCHOR_W,
					               NULL);
		g_object_set_data (G_OBJECT (canvas_item), "__text__", text_item);
		g_signal_connect (GTK_OBJECT (text_item), "event",
			              G_CALLBACK (on_canvas_event_proxy),
			              canvas_item);

		create_canvas_line_item (GNOME_CANVAS_GROUP (cls_node->canvas_group),
		                         &cls_node->plugin->style[STYLE_FG],
		                         0, j * item_height,
		                         cls_node->width, j * item_height);
	}
	
	/* make the outline bounds */
	gnome_canvas_item_new (GNOME_CANVAS_GROUP (cls_node->canvas_group),
							gnome_canvas_rect_get_type (),
							"x1", (gdouble) 0.0,
							"y1", (gdouble) 0.0,
							"x2", (gdouble) cls_node->width,
							"y2", (gdouble) cls_node->height,
  						   "outline_color_gdk",
							&cls_node->plugin->style[STYLE_FG],
							"width_units", 1.0,
							NULL);
}

static void
cls_node_draw_collapsed (ClsNode *cls_node) 
{
	GnomeCanvasItem *item, *text_item;
	gdouble text_width_value;

	g_return_if_fail (cls_node->agnode != NULL);
	g_return_if_fail (cls_node->canvas);
	g_return_if_fail (cls_node->expansion_status == CLS_NODE_COLLAPSED);

	/* Clean up members */
	g_hash_table_remove_all (cls_node->members);

	/* Switch to collapsed canvas item */
	if (cls_node->canvas_group)
		gtk_object_destroy (GTK_OBJECT (cls_node->canvas_group));
	cls_node->canvas_group =
		gnome_canvas_item_new (gnome_canvas_root
		                       (GNOME_CANVAS (cls_node->canvas)),
		                       gnome_canvas_group_get_type (),
		                       NULL);
	cls_node->drawn_expansion_status = CLS_NODE_COLLAPSED;
	
	item =
		gnome_canvas_item_new (GNOME_CANVAS_GROUP (cls_node->canvas_group),
		                       gnome_canvas_rect_get_type (),
		                       "x1", (gdouble) 0.0,
		                       "y1", (gdouble) 0.0,
		                       "x2", (gdouble) cls_node->width,
		                       "y2", (gdouble) cls_node->height,
		                       "fill_color_gdk",
		                       &cls_node->plugin->style[STYLE_BG],
		                       "outline_color_gdk",
		                       &cls_node->plugin->style[STYLE_FG],
		                       "width_units", 1.0,
		                       NULL);
	g_signal_connect (GTK_OBJECT (item), "event",
					  G_CALLBACK (on_collapsed_class_event),
					  cls_node);

	/* --- text --- */
	text_item =
		gnome_canvas_item_new (GNOME_CANVAS_GROUP (cls_node->canvas_group),
		                       gnome_canvas_text_get_type (),
		                       "text", cls_node->sym_name,
		                       "justification", GTK_JUSTIFY_CENTER,
		                       "anchor", GTK_ANCHOR_CENTER,
		                       "x", (gdouble) 0.0,
		                       "y", (gdouble) cls_node->height/2,
		                       "fill_color_gdk",
		                       &cls_node->plugin->style[STYLE_FG],
		                       "anchor", GTK_ANCHOR_W,        
		                       NULL );
	g_object_set_data (G_OBJECT (item), "__text__", text_item);
	g_signal_connect (GTK_OBJECT (text_item), "event",
	                  G_CALLBACK (on_canvas_event_proxy), item);
	
	/* center the text in the node... */
	g_object_get (text_item, "text_width", &text_width_value, NULL);
						
	gnome_canvas_item_set (text_item,
	                       "x", (gdouble)((cls_node->width/2 - text_width_value/2)),
	                       NULL);
}

/* This function determins NODE_EDGE_ARROW_LENGTH long arrow destination
 * as a projection of (x1, y1) - (x2, y2) line.
 */
static gboolean
create_canvas_edge_arrow_ending (double x1, double y1, double x2, double y2,
                                 double *end_x, double *end_y)
{
	double x = x2 - x1;
	double y = y2 - y1;
	double h1 = sqrt(y * y + x * x);

	if (h1 == 0) /* Overlapping points, no direction! */
		return FALSE;
	
	*end_x = x2 + NODE_EDGE_ARROW_LENGTH * x / h1;
	*end_y = y2 + NODE_EDGE_ARROW_LENGTH * y / h1;
	return TRUE;
}

static void
cls_node_draw_edge (ClsNode *cls_node_to, ClsNodeEdge *cls_edge, ClsNode *cls_node_from)
{
	Agedge_t *edge;
	GnomeCanvasPathDef *path_def;
	GnomeCanvasPoints *points;
	gint i, num_points;
	
	path_def = gnome_canvas_path_def_new();
	edge = cls_edge->agedge;
	num_points = ED_spl(edge)->list->size;
	
	for (i = 0; i <  num_points - 1; i += 3)
	{
		/* go on with bezier curves. We can retrieve the info such
		 * as control points from the struct of the edge
		 */
		gnome_canvas_path_def_moveto (path_def,
		                              ((ED_spl(edge))->list->list[0+i]).x, 
		                              GRAPH_TO_CANVAS_Y (((ED_spl(edge))->list->list[0+i]).y));
		
		gnome_canvas_path_def_curveto (path_def, 
		                               ((ED_spl(edge))->list->list[1+i]).x, 
		                               GRAPH_TO_CANVAS_Y (((ED_spl(edge))->list->list[1+i]).y),
		                               ((ED_spl(edge))->list->list[2+i]).x, 
		                               GRAPH_TO_CANVAS_Y (((ED_spl(edge))->list->list[2+i]).y),
		                               ((ED_spl(edge))->list->list[3+i]).x, 
		                               GRAPH_TO_CANVAS_Y (((ED_spl(edge))->list->list[3+i]).y));
	}
	
	/* draw the path_def */
	if (cls_edge->canvas_line)
	{
		gnome_canvas_item_set (cls_edge->canvas_line,
		                       "bpath", path_def,
		                       NULL);
	}
	else
	{
		cls_edge->canvas_line =
			gnome_canvas_item_new (gnome_canvas_root
				                   (GNOME_CANVAS (cls_node_from->canvas)), 
				                   gnome_canvas_bpath_get_type(),
				                   "bpath", path_def,
				                   "outline_color_gdk",
				                   &cls_node_from->plugin->style[STYLE_FG],
				                   "width_pixels", 2,
				                   NULL);
	}
	gnome_canvas_path_def_unref (path_def);

    /* Draw arrow */
	
	/* let's draw a canvas_line with an arrow-end */
	points = gnome_canvas_points_new (2);

	/* Arrow end position */
	/* Sometimes, last 2 points overlap, resulting in failure to determine
	 * next point, track back until there is a usable pair of points.
	 */
	do
	{
		num_points--;
		
		/* starting point */
		points->coords[0] = ((ED_spl(edge))->list->list[num_points]).x;
		points->coords[1] =
			GRAPH_TO_CANVAS_Y (((ED_spl(edge))->list->list[num_points]).y);
	
	}
	while (num_points > 0 &&
	       !create_canvas_edge_arrow_ending (((ED_spl(edge))->list->list[num_points - 1]).x,
			                                 GRAPH_TO_CANVAS_Y (((ED_spl(edge))->list->list[num_points - 1]).y),
			                                 ((ED_spl(edge))->list->list[num_points]).x,
			                                 GRAPH_TO_CANVAS_Y (((ED_spl(edge))->list->list[num_points]).y),
			                                 &points->coords[2], &points->coords[3]));

	if (cls_edge->canvas_arrow)
	{
		gnome_canvas_item_set (cls_edge->canvas_arrow,
		                       "points", points,
					           NULL);
	}
	else
	{
		cls_edge->canvas_arrow =
			gnome_canvas_item_new (gnome_canvas_root
				                   (GNOME_CANVAS (cls_node_from->canvas)),
				                   gnome_canvas_line_get_type(),
				                   "points", points,
				                   "fill_color_gdk",
				                   &cls_node_from->plugin->style[STYLE_FG],
				                   "last_arrowhead", TRUE,
				                   "arrow_shape_a", 10.0,
				                   "arrow_shape_b", 10.0,
				                   "arrow_shape_c", 4.0,
				                   "width_units", 2.0,
				                   NULL);
	}
	gnome_canvas_points_unref (points);

}

/* Ensures that a new canvas item is created for this node and moved to right
 * position, or just moves an existing canvas item if already created.
 */
static void
cls_node_ensure_draw (gpointer klass_id, ClsNode *cls_node, ClsBox *bounding_box) 
{
	gdouble x, y;
	point node_pos;
#ifndef ND_coord_i
	pointf node_posf;
#endif
	
	/* get some infos from the node */
#ifdef ND_coord_i
	node_pos = ND_coord_i (cls_node->agnode);
#else
	node_posf = ND_coord (cls_node->agnode);
	PF2P (node_posf, node_pos);
#endif

	/* Determine node coords and size *in* canvas world coordinate system */
	cls_node->width = INCH_TO_PIXELS (ND_width (cls_node->agnode));
	cls_node->height = INCH_TO_PIXELS (ND_height (cls_node->agnode));
	cls_node->x1 = node_pos.x - cls_node->width/2;
	cls_node->y1 = GRAPH_TO_CANVAS_Y ((node_pos.y + cls_node->height/2));
	cls_node->x2 = node_pos.x + cls_node->width/2;
	cls_node->y2 = GRAPH_TO_CANVAS_Y ((node_pos.y - cls_node->height/2));

	if (cls_node->x1 < bounding_box->x1) bounding_box->x1 = cls_node->x1;
	if (cls_node->y1 < bounding_box->y1) bounding_box->y1 = cls_node->y1;
	if (cls_node->x2 > bounding_box->x2) bounding_box->x2 = cls_node->x2;
	if (cls_node->y2 > bounding_box->y2) bounding_box->y2 = cls_node->y2;

	if (cls_node->canvas_group == NULL ||
	    cls_node->drawn_expansion_status != cls_node->expansion_status)
	{
		if (cls_node->expansion_status == CLS_NODE_COLLAPSED)
			cls_node_draw_collapsed (cls_node);
		else
			cls_node_draw_expanded (cls_node);
	}
	/* Move the canvas item to right place */
	x = cls_node->x1;
	y = cls_node->y1;
	gnome_canvas_item_w2i (cls_node->canvas_group, &x, &y);
	gnome_canvas_item_move (cls_node->canvas_group, x, y);
	g_hash_table_foreach (cls_node->edges_to, (GHFunc) cls_node_draw_edge, cls_node);
}

/*----------------------------------------------------------------------------
 * draw the graph on the canvas. So nodes, edges, arrows, texts..
 */
void
cls_inherit_draw (AnjutaClassInheritance *plugin)
{
	ClsBox bounds;

	gvLayout (plugin->gvc, plugin->graph, "dot");

	/* set the size of the canvas. We need this to set the scrolling.. */
	bounds.x1 = 0;
	bounds.y1 = 0;
	bounds.x2 = CANVAS_MIN_SIZE;
	bounds.y2 = CANVAS_MIN_SIZE;

	g_hash_table_foreach (plugin->nodes, (GHFunc) cls_node_ensure_draw, &bounds);
	
	/* Request extra 20px along x and y for 10px margin around the canvas */
	gnome_canvas_set_scroll_region (GNOME_CANVAS (plugin->canvas),
	                                bounds.x1 - 10, bounds.y1 - 10,
	                                bounds.x2 + 10, bounds.y2 + 10);
	gvFreeLayout(plugin->gvc, plugin->graph);
}

static void
on_cls_node_mark_for_deletion (gpointer key, ClsNode *cls_node, gpointer data)
{
	cls_node->marked_for_deletion = TRUE;
}

static gboolean
on_cls_node_delete_marked (gpointer key, ClsNode *cls_node, gpointer data)
{
	if (!cls_node->marked_for_deletion) return FALSE;
	cls_node_unlink (cls_node);
	return TRUE;
}

/*----------------------------------------------------------------------------
 * update the internal graphviz graph-structure, then redraw the graph on the 
 * canvas
 */
void
cls_inherit_update (AnjutaClassInheritance *plugin)
{
	IAnjutaSymbolManager *sm;
	IAnjutaIterable *iter;
	IAnjutaSymbol *symbol;
	ClsNode *cls_node;

	g_return_if_fail (plugin != NULL);

	/* Mark all nodes for deletion. Selectively, they will be unmarked below */
	g_hash_table_foreach (plugin->nodes,
	                      (GHFunc) on_cls_node_mark_for_deletion,
	                      NULL);

	if (plugin->top_dir == NULL)
		goto cleanup;
	
	sm = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
									 IAnjutaSymbolManager, NULL);
	if (!sm)
		goto cleanup;
	
	/* Get all classes */	
	iter = ianjuta_symbol_manager_search (sm, IANJUTA_SYMBOL_TYPE_CLASS, 
										  TRUE,
										  IANJUTA_SYMBOL_FIELD_SIMPLE,
										  NULL, TRUE,
	                                      IANJUTA_SYMBOL_MANAGER_SEARCH_FS_PUBLIC, 
	    								  FALSE, -1, -1, NULL);
	if (!iter)
	{
		DEBUG_PRINT ("%s", "cls_inherit_update_graph (): search returned no items.");
		goto cleanup;
	}
	
	ianjuta_iterable_first (iter, NULL);
	if (ianjuta_iterable_get_length (iter, NULL) <= 0)
	{
		g_object_unref (iter);
		goto cleanup;
	}
	do 
	{
		gint klass_id;
		IAnjutaIterable *parents;
     
		/* a symbol representing a class */
		symbol = IANJUTA_SYMBOL (iter);

		/* get parents of the current class */
		parents = ianjuta_symbol_manager_get_class_parents (sm, symbol, 
															IANJUTA_SYMBOL_FIELD_SIMPLE, 
															NULL);

		/* if no parents are found then continue */
		if (parents == NULL || ianjuta_iterable_get_length (parents, NULL) <= 0)
		{
			/*DEBUG_PRINT ("ClassInheritance: no parents found for class %s",
						 ianjuta_symbol_get_name (symbol, NULL));*/
			continue;
		}

		if ((klass_id = ianjuta_symbol_get_id (symbol, NULL)) <= 0)
		{
			/*DEBUG_PRINT ("%s", "ClassInheritance: klass_id cannot be <= 0");*/
			continue;
		}
		
		cls_node = g_hash_table_lookup (plugin->nodes, GINT_TO_POINTER (klass_id));
		if (!cls_node)
		{
			cls_node = cls_inherit_create_node (plugin, symbol);
			g_hash_table_insert (plugin->nodes, GINT_TO_POINTER (klass_id), cls_node);
		}
		cls_node->marked_for_deletion = FALSE;

		/* Get parents */
		do 
		{
			gint parent_id;
			ClsNode *parent_node;
			IAnjutaSymbol *parent_symbol;
			parent_symbol = IANJUTA_SYMBOL (parents);
			parent_id = ianjuta_symbol_get_id (parent_symbol, NULL);
			
			parent_node = g_hash_table_lookup (plugin->nodes,
			                                   GINT_TO_POINTER (parent_id));
			if (!parent_node)
			{
				parent_node = cls_inherit_create_node (plugin, parent_symbol);
				g_hash_table_insert (plugin->nodes, GINT_TO_POINTER (parent_id),
				                     parent_node);
			}
			parent_node->marked_for_deletion = FALSE;
			cls_node_add_edge (parent_node, cls_node);
		} while (ianjuta_iterable_next (parents, NULL) == TRUE);
		g_object_unref (parents);

	} while (ianjuta_iterable_next (iter, NULL) == TRUE);

	g_object_unref (iter);

cleanup:
	
	/* Delete all marked nodes that did not get unmarked above. */
	g_hash_table_foreach_remove (plugin->nodes,
	                             (GHRFunc) on_cls_node_delete_marked,
	                             NULL);

	cls_inherit_draw (plugin);
}

void
cls_inherit_clear (AnjutaClassInheritance *plugin)
{
	g_hash_table_remove_all (plugin->nodes);
}

/*----------------------------------------------------------------------------
 * Perform a dot_cleanup and a graph closing. Call this function at the end of
 * call to draw_graph.
 */
void
cls_inherit_free (AnjutaClassInheritance *plugin)
{
	if (plugin->nodes)
		g_hash_table_destroy (plugin->nodes);
	plugin->nodes = NULL;

	/* Destroy graphics */
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
 * initialize the internal graphviz structure.
 */
static void
cls_inherit_graph_init (AnjutaClassInheritance *plugin, gchar* graph_label)
{
	Agsym_t *sym;
	gchar dpi_text[16];

	snprintf (dpi_text, 16, "%d", INCH_TO_PIXELS_CONVERSION_FACTOR);
	aginit ();
	plugin->graph = agopen (graph_label, AGDIGRAPH);
	plugin->gvc = gvContext();
	
	if (!(sym = agfindattr(plugin->graph->proto->n, "dpi")))
		sym = agraphattr(plugin->graph, "dpi", dpi_text);
	agxset(plugin->graph, sym->index, dpi_text);
}

void
cls_inherit_init (AnjutaClassInheritance *plugin)
{
	GtkWidget *s_window;

	/* Initialize graph layout engine */
	cls_inherit_graph_init (plugin, _(DEFAULT_GRAPH_NAME));
	
	s_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (s_window),
	                                GTK_POLICY_AUTOMATIC, 
	                                GTK_POLICY_AUTOMATIC);
	plugin->canvas = gnome_canvas_new ();
	gnome_canvas_set_scroll_region (GNOME_CANVAS (plugin->canvas),
	                                -CANVAS_MIN_SIZE/2, 
	                                -CANVAS_MIN_SIZE/2,
	                                CANVAS_MIN_SIZE/2,
	                                CANVAS_MIN_SIZE/2);
	gtk_container_add (GTK_CONTAINER (s_window), plugin->canvas);

	/* Initialize styles */
	gtk_widget_ensure_style (plugin->canvas);
	on_style_set (plugin->canvas, NULL, plugin);

	g_signal_connect (G_OBJECT (plugin->canvas), "event",
	                  G_CALLBACK (on_canvas_event),
	                  plugin);
	g_signal_connect (G_OBJECT (plugin->canvas), "style_set",
	                  G_CALLBACK (on_style_set), plugin);

	plugin->widget = gtk_vbox_new (FALSE, 2);
	
	/* --packing-- */
	/* vbox */
	gtk_box_pack_start (GTK_BOX (plugin->widget), s_window, TRUE, TRUE, TRUE);

	gtk_widget_show_all (plugin->widget);

	/* create new GList */
	plugin->nodes = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL,
	                                       (GDestroyNotify)cls_node_free);
	/* menu create */
	plugin->menu = gtk_menu_new ();

	GtkWidget *menu_item = gtk_menu_item_new_with_label (_("Update"));
	g_signal_connect (menu_item, "activate",
	                  (GCallback)on_update_menu_item_selected, plugin);

	/* set the user data on update selection */
	gtk_menu_shell_append (GTK_MENU_SHELL (plugin->menu), menu_item);
	gtk_widget_show_all (plugin->menu);

	plugin->update = menu_item;

	g_object_ref (plugin->menu);
	g_object_ref (plugin->update);
}
