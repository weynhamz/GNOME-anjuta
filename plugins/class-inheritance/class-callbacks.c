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
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <glib.h>
#include <gio/gio.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-symbol-manager.h>

#include "plugin.h"
#include "class-callbacks.h"
#include "class-inherit.h"

gint
on_canvas_event (GnomeCanvasItem *item, GdkEvent *event, gpointer data) 
{
	AnjutaClassInheritance *plugin;
	plugin = ANJUTA_PLUGIN_CLASS_INHERITANCE (data);
	
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

gint
on_canvas_event_proxy (GnomeCanvasItem *item, GdkEvent *event,
                       GnomeCanvasItem *proxy_item)
{
	gint ret;

	g_signal_emit_by_name (G_OBJECT (proxy_item), "event", event, &ret);
	return ret;
}

gint
on_expanded_class_title_event (GnomeCanvasItem *item, GdkEvent *event,
                               ClsNode *cls_node)
{
	GnomeCanvasItem *text_item;
	text_item = g_object_get_data (G_OBJECT (item), "__text__");
	switch (event->type)
	{
	case GDK_BUTTON_PRESS:		/* single click */
		if (event->button.button == 1 &&
		    cls_node->expansion_status != CLS_NODE_COLLAPSED &&
		    cls_node_collapse (cls_node))
		{
			cls_inherit_draw(cls_node->plugin);
			return TRUE;
		}
		break;
		
	case GDK_ENTER_NOTIFY:		/* mouse entered in title's area */
		gnome_canvas_item_set (item,
							   "fill_color_gdk",
							   &cls_node->plugin->style[STYLE_TITLE_PRELIGHT_BG],
							   NULL);
		gnome_canvas_item_set (text_item,
							   "fill_color_gdk",
							   &cls_node->plugin->style[STYLE_TITLE_PRELIGHT_FG],
							   NULL);
		return TRUE;

	case GDK_LEAVE_NOTIFY:		/* mouse exited title's area */
		gnome_canvas_item_set (item,
							   "fill_color_gdk",
							   &cls_node->plugin->style[STYLE_TITLE_BG],
							   NULL);
		gnome_canvas_item_set (text_item,
							   "fill_color_gdk",
							   &cls_node->plugin->style[STYLE_TITLE_FG],
							   NULL);
		return TRUE;
	default:
		break;
	}
	return FALSE;
}

static GnomeCanvasItem*
create_class_item_tooltip (ClsNode *cls_node, const gchar *tooltip_text)
{
	GnomeCanvasItem *group, *canvas_item, *text_item;
	gdouble text_width_value, text_height_value;

	group =
		gnome_canvas_item_new (gnome_canvas_root
			                   (GNOME_CANVAS (cls_node->canvas)),
			                   gnome_canvas_group_get_type (),
			                   NULL);
	
	text_item =
		gnome_canvas_item_new (GNOME_CANVAS_GROUP (group),
					           gnome_canvas_text_get_type (),
					           "text", tooltip_text,
					           "justification", GTK_JUSTIFY_LEFT,
					           "anchor", GTK_ANCHOR_CENTER,
					           "fill_color_gdk",
					           &cls_node->plugin->style[STYLE_ITEM_FG],
					           NULL);

	g_object_get (text_item, "text_width", &text_width_value,
	              "text_height", &text_height_value, NULL);

	gnome_canvas_item_set (text_item,
	                       "x", (gdouble) 10 + text_width_value/2,
				           "y", (gdouble) 10 + text_height_value/2,
	                       	NULL);
	/* Decoration */
	GnomeCanvasPoints *points = gnome_canvas_points_new (8);
	gint i = 0;
	points->coords[i++] = 0;
	points->coords[i++] = 0;
	
	points->coords[i++] = 30;
	points->coords[i++] = 0;

	points->coords[i++] = 40;
	points->coords[i++] = -10;

	points->coords[i++] = 50;
	points->coords[i++] = 0;

	points->coords[i++] = text_width_value + 20;
	points->coords[i++] = 0;

	points->coords[i++] = text_width_value + 20;
	points->coords[i++] = text_height_value + 20;

	points->coords[i++] = 0;
	points->coords[i++] = text_height_value + 20;
	
	points->coords[i++] = 0;
	points->coords[i++] = 0;

	/* background */
	canvas_item =
		gnome_canvas_item_new (GNOME_CANVAS_GROUP (group),
			                   gnome_canvas_polygon_get_type (),
			                   "points", points,
			                   "fill_color_gdk",
			                   &cls_node->plugin->style[STYLE_ITEM_BG],
			                   NULL);
	/* border */
	canvas_item =
		gnome_canvas_item_new (GNOME_CANVAS_GROUP (group),
	                       gnome_canvas_line_get_type (),
	                       "points", points,
	                       "fill_color_gdk",
	                       &cls_node->plugin->style[STYLE_ITEM_FG],
	                       NULL);
	/* shadow */
	canvas_item =
		gnome_canvas_item_new (GNOME_CANVAS_GROUP (group),
	                       gnome_canvas_polygon_get_type (),
	                       "points", points,
	                       "fill_color_gdk",
	                       &cls_node->plugin->style[STYLE_TITLE_BG],
	                       NULL);
	gnome_canvas_points_unref (points);

	/* Lower shadow */
	gnome_canvas_item_lower (canvas_item, 10);

	/* Offset shadow */
	gnome_canvas_item_move (canvas_item, 5, 5);

	/* Raise text */
	gnome_canvas_item_raise (text_item, 10);
	return group;
}

static gboolean
on_canvas_item_show_tooltip_timeout (ClsNodeItem *node_item)
{
	gchar *tooltip;
	gdouble x, y, x1, x2, y2;
	
	if (node_item->tooltip)
		gtk_object_destroy (GTK_OBJECT (node_item->tooltip));
	node_item->tooltip = NULL;
	
	if (node_item->args && strlen (node_item->args) > 2)
	{
		tooltip = g_strdup_printf (_("Args: %s"), node_item->args);

		node_item->tooltip =
			create_class_item_tooltip (node_item->cls_node, tooltip);
		g_free (tooltip);
	
		g_object_get (node_item->cls_node->canvas_group, "x", &x, "y", &y, NULL);
		g_object_get (node_item->canvas_node_item, "x1", &x1, "x2", &x2,
				      "y2", &y2, NULL);
		x = x + x1 + 25;
		y = y + y2 + 10;

		gnome_canvas_item_w2i (node_item->tooltip, &x, &y);
		gnome_canvas_item_move (node_item->tooltip, x, y);
		node_item->tooltip_timeout = 0;
	}
	return FALSE;
}

gint
on_expanded_class_item_event (GnomeCanvasItem *item, GdkEvent *event,
                              gpointer data)
{
	AnjutaClassInheritance *plugin;
	ClsNodeItem *node_item;
	GnomeCanvasItem *text_item;
	
	text_item = g_object_get_data (G_OBJECT (item), "__text__");
	
	node_item = (ClsNodeItem*)data;
	plugin = node_item->cls_node->plugin;

	switch (event->type)
	{
	case GDK_BUTTON_PRESS:		/* single click */
		if (event->button.button == 1)
		{
			/* Goto uri line */
			if (node_item->file) 
			{
				IAnjutaDocumentManager *dm;
				dm = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
				                                 IAnjutaDocumentManager, NULL);
				if (dm) 
					ianjuta_document_manager_goto_file_line (dm,
					                                         node_item->file,
					                                         node_item->line,
					                                         NULL);
			}
		}
		break;
		
	case GDK_ENTER_NOTIFY:		/* mouse entered in item's area */
		gnome_canvas_item_set (node_item->canvas_node_item,
							   "fill_color_gdk",
							   &node_item->cls_node->plugin->style[STYLE_ITEM_PRELIGHT_BG],
							   NULL);
		gnome_canvas_item_set (text_item,
							   "fill_color_gdk",
							   &node_item->cls_node->plugin->style[STYLE_ITEM_PRELIGHT_FG],
							   NULL);
		/* Show tooltip */
		if (!node_item->tooltip)
		{
			if (node_item->tooltip_timeout)
				g_source_remove (node_item->tooltip_timeout);
			node_item->tooltip_timeout =
				g_timeout_add (500,
				               (GSourceFunc)on_canvas_item_show_tooltip_timeout,
				               node_item);
		}
		return TRUE;

	case GDK_LEAVE_NOTIFY:		/* mouse exited item's area */
		gnome_canvas_item_set (node_item->canvas_node_item,
							   "fill_color_gdk",
							   &node_item->cls_node->plugin->style[STYLE_ITEM_BG],
							   NULL);
		gnome_canvas_item_set (text_item,
							   "fill_color_gdk",
							   &node_item->cls_node->plugin->style[STYLE_ITEM_FG],
							   NULL);
		/* Hide tooltip */
		if (node_item->tooltip_timeout)
			g_source_remove (node_item->tooltip_timeout);
		node_item->tooltip_timeout = 0;
		if (node_item->tooltip)
			gtk_object_destroy (GTK_OBJECT (node_item->tooltip));
		node_item->tooltip = NULL;
		
		return TRUE;
	default:
		break;
	}
	return FALSE;
}

gint
on_expanded_class_more_event (GnomeCanvasItem *item, GdkEvent *event,
                              ClsNode *cls_node)
{
	GnomeCanvasItem *text_item;
	text_item = g_object_get_data (G_OBJECT (item), "__text__");
	switch (event->type)
	{
	case GDK_2BUTTON_PRESS:		/* double click */
		break;

	case GDK_BUTTON_PRESS:		/* single click */
		if (event->button.button == 1 &&
		    cls_node->expansion_status == CLS_NODE_SEMI_EXPANDED &&
		    cls_node_expand (cls_node, CLS_NODE_FULL_EXPANDED))
		{
			cls_inherit_draw(cls_node->plugin);
		}
		break;
		
	case GDK_ENTER_NOTIFY:		/* mouse entered in more's area */
		gnome_canvas_item_set (item,
							   "fill_color_gdk",
							   &cls_node->plugin->style[STYLE_TITLE_PRELIGHT_BG],
							   NULL);
		gnome_canvas_item_set (text_item,
							   "fill_color_gdk",
							   &cls_node->plugin->style[STYLE_TITLE_PRELIGHT_FG],
							   NULL);
		return TRUE;

	case GDK_LEAVE_NOTIFY:		/* mouse exited item's area */
		gnome_canvas_item_set (item,
							   "fill_color_gdk",
		                       &cls_node->plugin->style[STYLE_TITLE_BG],
							   NULL);
		gnome_canvas_item_set (text_item,
							   "fill_color_gdk",
							   &cls_node->plugin->style[STYLE_TITLE_FG],
							   NULL);
		return TRUE;
	default:
		break;
	}
	
	return FALSE;
}

gint
on_collapsed_class_event (GnomeCanvasItem *item, GdkEvent *event, gpointer data)
{
	AnjutaClassInheritance *plugin;
	ClsNode *cls_node;
	GnomeCanvasItem *text_item;
	text_item = g_object_get_data (G_OBJECT (item), "__text__");
	
	cls_node = (ClsNode*)data;
	plugin = cls_node->plugin;

	switch (event->type)
	{
	case GDK_BUTTON_PRESS:		/* single click */
		if (event->button.button == 1)
		{
			if (cls_node_expand (cls_node, CLS_NODE_SEMI_EXPANDED))
			{
			    cls_inherit_draw(plugin);
		    }
			return TRUE;
		}
		break;
		
	case GDK_ENTER_NOTIFY:		/* mouse entered in item's area */
		/* Make the outline wide */
		gnome_canvas_item_set (item,
							   "fill_color_gdk",
							   &cls_node->plugin->style[STYLE_ITEM_PRELIGHT_BG],
							   NULL);
		gnome_canvas_item_set (text_item,
							   "fill_color_gdk",
							   &cls_node->plugin->style[STYLE_ITEM_PRELIGHT_FG],
							   NULL);
		return TRUE;

	case GDK_LEAVE_NOTIFY:		/* mouse exited item's area */
		/* Make the outline thin */
		gnome_canvas_item_set (item,
							   "fill_color_gdk",
							   &cls_node->plugin->style[STYLE_BG],
							   NULL);
		gnome_canvas_item_set (text_item,
							   "fill_color_gdk",
							   &cls_node->plugin->style[STYLE_FG],
							   NULL);
		return TRUE;
	default:
		break;
	}
	
	return FALSE;
}

/*----------------------------------------------------------------------------
 * callback for the canvas' right-click menu - update button.
 */
void
on_update_menu_item_selected (GtkMenuItem *item,
							  AnjutaClassInheritance *plugin)
{
	cls_inherit_update (plugin);
}

/*----------------------------------------------------------------------------
 * callback for theme/colors changes
 */
void
on_style_set (GtkWidget *widget, GtkStyle  *previous_style,
			  AnjutaClassInheritance *plugin)
{
	GtkStyle *style = plugin->canvas->style;

	plugin->style[STYLE_BG] = style->base[GTK_STATE_NORMAL];
	plugin->style[STYLE_FG] = style->text[GTK_STATE_NORMAL];
	plugin->style[STYLE_TITLE_FG] = style->fg[GTK_STATE_ACTIVE];
	plugin->style[STYLE_TITLE_BG] = style->bg[GTK_STATE_ACTIVE];
	plugin->style[STYLE_TITLE_PRELIGHT_FG] = style->fg[GTK_STATE_PRELIGHT];
	plugin->style[STYLE_TITLE_PRELIGHT_BG] = style->bg[GTK_STATE_PRELIGHT];
	plugin->style[STYLE_ITEM_FG] = style->text[GTK_STATE_NORMAL];
	plugin->style[STYLE_ITEM_BG] = style->base[GTK_STATE_NORMAL];
	plugin->style[STYLE_ITEM_PRELIGHT_FG] = style->text[GTK_STATE_SELECTED];
	plugin->style[STYLE_ITEM_PRELIGHT_BG] = style->base[GTK_STATE_SELECTED];
	
	/* Use text background (normally white) for canvas background */
	style->bg[GTK_STATE_NORMAL] = plugin->style[STYLE_BG];

	/* FIXME: */
	/* cls_inherit_update (plugin); */
}
