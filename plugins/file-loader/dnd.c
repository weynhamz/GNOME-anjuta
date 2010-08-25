/*
 * dnd.c (c) Johannes Schmid, 2009
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

#include <gtk/gtk.h>
#include <glib.h>
#include <gdk/gdk.h>

#include <libanjuta/anjuta-utils.h>

#include "dnd.h"

#define TARGET_URI_LIST 100

void (* dnd_data_dropped) (GFile *uri, gpointer data) = NULL;

static GdkAtom
drag_get_uri_target (GtkWidget      *widget,
		     GdkDragContext *context)
{
	GdkAtom target;
	GtkTargetList *tl;
	
	tl = gtk_target_list_new (NULL, 0);
	gtk_target_list_add_uri_targets (tl, 0);
	
	target = gtk_drag_dest_find_target (widget, context, tl);
	gtk_target_list_unref (tl);
	
	return target;	
}

static gboolean
dnd_drag_drop (GtkWidget      *widget,
               GdkDragContext *context,
               gint            x,
               gint            y,
               guint           timestamp)
{
  gboolean result = FALSE;
  GdkAtom target;

  /* If this is a URL, just get the drag data */
  target = drag_get_uri_target (widget, context);

  if (target != GDK_NONE)
  {
		gtk_drag_get_data (widget, context, target, timestamp);
		result = TRUE;
  }
  
  return result;
}

/*
 * Callback for the drag_data_received signal, emitted whenever something is
 * dragged onto the widget.
 */
static void
drag_data_received_cb (GtkWidget *widget, GdkDragContext *context,
                       gint x, gint y, GtkSelectionData *data,
                       guint info, guint time, gpointer user_data)
{
	GSList* files;
	/* If this is an URL emit DROP_URIS, otherwise chain up the signal */
	if (info == TARGET_URI_LIST)
	{
		files = anjuta_util_drop_get_files (data);

		if (files != NULL)
		{
			GSList* node;
			for (node = files; node != NULL; node = g_slist_next(node))
			{
				GFile* file = node->data;
				dnd_data_dropped (file, user_data);
				g_object_unref (file);
			}
			g_slist_free (files);
			gtk_drag_finish (context, TRUE, FALSE, time);
		}
		gtk_drag_finish (context, FALSE, FALSE, time);
  }
}

/*
 * Initialize widget to start accepting "droppings" for the (NULL terminated)
 * list of mime types.
 */
void
dnd_drop_init (GtkWidget *widget,
               void (* data_dropped) (GFile* file, gpointer user_data),
               gpointer user_data)
{
  GtkTargetList* tl;
  GtkTargetEntry accepted[1];

  accepted[0].target = "application-x/anjuta";
  accepted[0].info = 0;
  accepted[0].flags = 0;
  
  /* Drag and drop support */	  	
  gtk_drag_dest_set (widget, GTK_DEST_DEFAULT_ALL, accepted, 1, GDK_ACTION_COPY);
  tl = gtk_drag_dest_get_target_list (widget);	
  gtk_target_list_add_uri_targets (tl, TARGET_URI_LIST);
  dnd_data_dropped = *data_dropped;
  g_signal_connect (G_OBJECT (widget), "drag_data_received", 
                    G_CALLBACK (drag_data_received_cb),
                    (gpointer) user_data);  	
  g_signal_connect (G_OBJECT (widget), "drag-drop",
                    G_CALLBACK (dnd_drag_drop), 
                    (gpointer) user_data);  
  return;
}

void
dnd_drop_finalize (GtkWidget *widget, gpointer user_data)
{
  g_signal_handlers_disconnect_by_func (G_OBJECT (widget),
                                        G_CALLBACK (drag_data_received_cb), user_data);
  g_signal_handlers_disconnect_by_func (G_OBJECT (widget),
                                        G_CALLBACK (dnd_drag_drop), user_data);
  dnd_data_dropped = NULL;
}
