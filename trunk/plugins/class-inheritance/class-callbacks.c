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

void
on_toggled_menuitem_clicked (GtkCheckMenuItem *checkmenuitem,
							 gpointer data)
{
	NodeData *node;
	node = (NodeData*)data;
	
	if (node->name == NULL || g_str_equal (node->name, ""))
		return;
		
	if (node->anchored) {
		node->anchored = FALSE;
		
		/* remove the key from the hash table, if present */
		if (g_hash_table_lookup (node->plugin->expansion_node_list, node->name)) {
			g_hash_table_remove (node->plugin->expansion_node_list, node->name);
		}
	}
	else {
		NodeExpansionStatus *node_status;
		node->anchored = TRUE;
		
		node_status = g_new0 (NodeExpansionStatus, 1);
		node_status->name = g_strdup (node->name);
		/* set to half. This will display at least NODE_HALF_DISPLAY_ELEM_NUM.
		 * User will decide whether to show all elements or not. */
		node_status->expansion_status = NODE_HALF_EXPANDED;
		
		/* insert the class name to the hash_table */
		g_hash_table_insert (node->plugin->expansion_node_list, 
							g_strdup (node->name), 
							node_status);
	}
	
	class_inheritance_update_graph (node->plugin);
}

void
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

gint
on_nodedata_expanded_event (GnomeCanvasItem *item, GdkEvent *event, gpointer data)
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
		if (event->button.button == 1) {
			NodeExpansionStatus *node_status;
			if ( (node_status = (NodeExpansionStatus*)g_hash_table_lookup (plugin->expansion_node_list, 
																				nodedata->name)) == NULL) {
				break;
			}
			else				
				if (strcmp (nodedata->sub_item, NODE_SHOW_ALL_MEMBERS_STR) == 0) {
					node_status->expansion_status = NODE_FULL_EXPANDED;
					class_inheritance_update_graph (plugin);
				}
			else
				if (strcmp (nodedata->sub_item, NODE_SHOW_NORMAL_VIEW_STR) == 0) {
					g_hash_table_remove (plugin->expansion_node_list, nodedata->name);
					class_inheritance_update_graph (plugin);
				}
			else {		/* it's a class member. Take line && file of definition 
							 * and reach them */
				const gchar* file;
				gint line;
				
				file = g_object_get_data (G_OBJECT (item), "__file");
				line = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (item), "__line"));
				if (file) {
					/* Goto file line */
					IAnjutaDocumentManager *dm;
					dm = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
											 	IAnjutaDocumentManager, NULL);
					if (dm) {
						ianjuta_document_manager_goto_file_line (dm, file, line, NULL);
					}
				}
			}
		}
		break;
		
	case GDK_ENTER_NOTIFY:		/* mouse entered in item's area */
		gnome_canvas_item_set (nodedata->canvas_item,
							   "fill_color_gdk",
							   &plugin->canvas->style->base[GTK_STATE_PRELIGHT],
							   NULL);
		return TRUE;

	case GDK_LEAVE_NOTIFY:		/* mouse exited item's area */
		gnome_canvas_item_set (nodedata->canvas_item,
							   "fill_color_gdk",
							   &plugin->canvas->style->base[GTK_STATE_ACTIVE],
							   NULL);
		return TRUE;
	default:
		break;
	}
	
	return FALSE;
	
}


gint
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
		if (event->button.button == 1 && !nodedata->anchored)
		{
			class_inheritance_show_dynamic_class_popup_menu  (event, data);
		}
		break;
		
	case GDK_ENTER_NOTIFY:		/* mouse entered in item's area */
		/* Make the outline wide */
		gnome_canvas_item_set (nodedata->canvas_item,
							   "width_units", 2.5,
							   "fill_color_gdk",
							   &plugin->canvas->style->base[GTK_STATE_PRELIGHT],
							   "outline_color_gdk",
							   &plugin->canvas->style->text[GTK_STATE_PRELIGHT],
							   NULL);
		return TRUE;

	case GDK_LEAVE_NOTIFY:		/* mouse exited item's area */
		/* Make the outline thin */
		gnome_canvas_item_set (nodedata->canvas_item,
							   "width_units", 1.0,
							   "fill_color_gdk",
							   &plugin->canvas->style->base[GTK_STATE_NORMAL],
							   "outline_color_gdk",
							   &plugin->canvas->style->text[GTK_STATE_NORMAL],	
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
	class_inheritance_update_graph (plugin);
}

/*----------------------------------------------------------------------------
 * callback for theme/colors changes
 */
void
on_style_set (GtkWidget *widget, GtkStyle  *previous_style,
			  AnjutaClassInheritance *plugin)
{
	class_inheritance_update_graph (plugin);
}
