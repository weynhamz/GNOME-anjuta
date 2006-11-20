#include <gtk/gtk.h>
#include <string.h>
#include "main.h"

/* testing */
GdkRectangle rect;
GtkWidget *tipwindow;
gulong gtodo_timeout = 0; /* Variable name conflict during linking */
static gboolean mw_tooltip_timeout(GtkWidget *tv);
PangoLayout *layout = NULL;

gchar *get_tooltip_text()
{
	gchar *summary, *comment = NULL, *due_date = NULL,*priority = NULL, *retval = NULL, *category = NULL;
	GString *string;
	gint prio;
	guint64 id;
	GtkTreePath *path;
	GtkTreeIter iterp, iter;
	GTodoItem *item;
	int i = gtk_option_menu_get_history(GTK_OPTION_MENU(mw.option));
	string = g_string_new("");
	if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(mw.treeview), rect.x, rect.y, &path, NULL, NULL, NULL))
	{
		gtk_tree_model_get_iter(GTK_TREE_MODEL(mw.sortmodel), &iterp, path);	
		gtk_tree_model_sort_convert_iter_to_child_iter(GTK_TREE_MODEL_SORT(mw.sortmodel), &iter, &iterp);
		gtk_tree_model_get(GTK_TREE_MODEL(mw.list), &iter, 
				ID, &id,
				-1); 
		gtk_tree_path_free(path);
		item = gtodo_client_get_todo_item_from_id(cl, (guint32)id);
		if(item != NULL)
		{
			if(i == 0)
			{
				g_string_append_printf(string, "<i>%s:</i>\n",
					gtodo_todo_item_get_category(item)); 
			}
			
			if(gtodo_todo_item_get_summary(item) != NULL)
			{
				g_string_append_printf(string, _("<b>Summary:</b>\t%s"), 
						gtodo_todo_item_get_summary(item));
			}

			if(gtodo_todo_item_get_due_date(item) != NULL 
					&& gtodo_todo_item_get_due_time_houre(item) == -1){
			       	g_string_append_printf(string, _("\n<b>Due date:</b>\t%s"), 
						gtodo_todo_item_get_due_date_as_string(item));
			}
			else if(gtodo_todo_item_get_due_date(item) != NULL)
			{
				g_string_append_printf(string, _("\n<b>Due date:</b>\t%s at %02i:%02i") , 
						gtodo_todo_item_get_due_date_as_string(item), 
						gtodo_todo_item_get_due_time_houre(item), 
						gtodo_todo_item_get_due_time_minute(item));
			}

			if(gtodo_todo_item_get_priority(item) == 0)
			{
				g_string_append_printf(string, _("\n<b>Priority:</b>\t\t<span color=\"dark green\">%s</span>"), _("Low"));
			}
			else if(gtodo_todo_item_get_priority(item) == 1)
			{
				g_string_append_printf(string, _("\n<b>Priority:</b>\t\t%s"), _("Medium"));
			}
			else
			{
				g_string_append_printf(string, _("\n<b>Priority:</b>\t\t<span color=\"red\">%s</span>"), _("High"));
			}
			if(gtodo_todo_item_get_comment(item) != NULL && strlen(gtodo_todo_item_get_comment(item)))
			{
				g_string_append_printf(string, _("\n<b>Comment:</b>\t%s"), gtodo_todo_item_get_comment(item)); 
			}

			gtodo_todo_item_free(item);
		}
		/* free all the stuff */
		for(i=0;i < string->len;i++)
		{
			if(string->str[i] == '&')
			{
				g_string_insert(string, i+1, "amp;");
			}
		}
		retval = string->str;	
		g_string_free(string, FALSE);
		return retval;
	}
	else return g_strdup("oeps");
}



void mw_paint_tip(GtkWidget *widget, GdkEventExpose *event)
{
	GtkStyle *style;
	char *tooltiptext = get_tooltip_text();
	if(tooltiptext == NULL) tooltiptext = g_strdup("oeps");
	pango_layout_set_markup(layout, tooltiptext, strlen(tooltiptext));
	pango_layout_set_wrap(layout, PANGO_WRAP_WORD);
	pango_layout_set_width(layout, 300000);
	style = tipwindow->style;

	gtk_paint_flat_box (style, tipwindow->window, GTK_STATE_NORMAL, GTK_SHADOW_OUT,
			NULL, tipwindow, "tooltip", 0, 0, -1, -1);


	gtk_paint_layout (style, tipwindow->window, GTK_STATE_NORMAL, TRUE,
			NULL, tipwindow, "tooltip", 4, 4, layout);
	/*
	   g_object_unref(layout);
	   */
	g_free(tooltiptext);
	return;
}

gboolean mw_tooltip_timeout(GtkWidget *tv)
{
	int scr_w,scr_h, w, h, x, y;
	char *tooltiptext = NULL;

	tooltiptext = get_tooltip_text();

	tipwindow = gtk_window_new(GTK_WINDOW_POPUP);
	tipwindow->parent = tv;
	gtk_widget_set_app_paintable(tipwindow, TRUE);
	gtk_window_set_resizable(GTK_WINDOW(tipwindow), FALSE);
	gtk_widget_set_name(tipwindow, "gtk-tooltips");
	g_signal_connect(G_OBJECT(tipwindow), "expose_event",
			G_CALLBACK(mw_paint_tip), NULL);
	gtk_widget_ensure_style (tipwindow);

	layout = gtk_widget_create_pango_layout (tipwindow, NULL);
	pango_layout_set_wrap(layout, PANGO_WRAP_WORD);
	pango_layout_set_width(layout, 300000);
	pango_layout_set_markup(layout, tooltiptext, strlen(tooltiptext));
	scr_w = gdk_screen_width();
	scr_h = gdk_screen_height();
	pango_layout_get_size (layout, &w, &h);
	w = PANGO_PIXELS(w) + 8;
	h = PANGO_PIXELS(h) + 8;

	gdk_window_get_pointer(NULL, &x, &y, NULL);
	if (GTK_WIDGET_NO_WINDOW(mw.vbox))
		y += mw.vbox->allocation.y;

	x -= ((w >> 1) + 4);

	if ((x + w) > scr_w)
		x -= (x + w) - scr_w;
	else if (x < 0)
		x = 0;

	if ((y + h + 4) > scr_h)
		y = y - h;
	else
		y = y + 6;
	/*
	   g_object_unref(layout);
	   */
	g_free(tooltiptext);
	gtk_widget_set_size_request(tipwindow, w, h);
	gtk_window_move(GTK_WINDOW(tipwindow), x, y);
	gtk_widget_show(tipwindow);

	return FALSE;
}

gboolean mw_motion_cb (GtkWidget *tv, GdkEventMotion *event, gpointer null)
{
	GtkTreePath *path;
	if(!gconf_client_get_bool(client, "/apps/gtodo/prefs/show-tooltip",NULL)) return FALSE;

	if(rect.y == 0 && rect.height == 0 && gtodo_timeout)
	{
		g_source_remove(gtodo_timeout);
		gtodo_timeout = 0;
		if (tipwindow) {
			gtk_widget_destroy(tipwindow);
			tipwindow = NULL;
		}
		return FALSE;

	}
	if (gtodo_timeout) {
		if (((int)event->y > rect.y) && (((int)event->y - rect.height) < rect.y))
			return FALSE;

		if(event->y == 0)
		{
			g_source_remove(gtodo_timeout);
			return FALSE;

		}
		/* We've left the cell.  Remove the timeout and create a new one below */
		if (tipwindow) {
			gtk_widget_destroy(tipwindow);
			tipwindow = NULL;
		}

		g_source_remove(gtodo_timeout);
	}

	if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tv), event->x, event->y, &path, NULL, NULL, NULL))
	{
		gtk_tree_view_get_cell_area(GTK_TREE_VIEW(tv), path, NULL, &rect);
		gtk_tree_path_free(path);
		if(rect.y != 0 && rect.height != 0) gtodo_timeout = g_timeout_add(500, (GSourceFunc)mw_tooltip_timeout, tv);
	}
	return FALSE;
}

void mw_leave_cb (GtkWidget *w, GdkEventCrossing *e, gpointer n)
{
	if (gtodo_timeout) {
		g_source_remove(gtodo_timeout);
		gtodo_timeout = 0;
	}
	if (tipwindow) {
		gtk_widget_destroy(tipwindow);
		g_object_unref(layout);
		tipwindow = NULL;
	}
}
