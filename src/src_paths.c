/*
    src_paths.c
    Copyright (C) 2000  Kh. Naba Kumar Singh

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gnome.h>
#include "anjuta.h"
#include "resources.h"
#include "src_paths.h"

SrcPaths *
src_paths_new (void)
{
	SrcPaths *co = g_malloc (sizeof (SrcPaths));
	if (co)
	{
		co->src_index = 0;
		co->is_showing = FALSE;
		co->win_pos_x = 100;
		co->win_pos_y = 100;
		create_src_paths_gui (co);
	}
	return co;
}

void
src_paths_destroy (SrcPaths * co)
{
	if (co)
	{
		gtk_widget_unref (co->widgets.window);

		gtk_widget_unref (co->widgets.src_clist);
		gtk_widget_unref (co->widgets.src_entry);
		gtk_widget_unref (co->widgets.src_add_b);
		gtk_widget_unref (co->widgets.src_update_b);
		gtk_widget_unref (co->widgets.src_remove_b);
		gtk_widget_unref (co->widgets.src_clear_b);

		if (co->widgets.window)
			gtk_widget_destroy (co->widgets.window);
		g_free (co);
	}
}

gboolean src_paths_save_yourself (SrcPaths * sp, FILE * stream)
{
	if (stream == NULL || sp == NULL)
		return FALSE;
	fprintf (stream, "source.paths.win.pos.x=%d\n", sp->win_pos_x);
	fprintf (stream, "source.paths.win.pos.y=%d\n", sp->win_pos_y);
	return TRUE;
}

gboolean src_paths_save (SrcPaths * co, FILE * f)
{
	gint length, i;
	gchar *text;

	g_return_val_if_fail (co != NULL, FALSE);
	g_return_val_if_fail (f != NULL, FALSE);

	fprintf (f, "project.source.paths=");
	length = g_list_length (GTK_CLIST (co->widgets.src_clist)->row_list);
	for (i = 0; i < length; i++)
	{
		gtk_clist_get_text (GTK_CLIST (co->widgets.src_clist), i, 0,
				    &text);
		fprintf (f, "\\\n\t%s", text);
		if (ferror (f))
			return FALSE;
	}
	fprintf (f, "\n\n");
	return TRUE;
}

gboolean src_paths_load_yourself (SrcPaths * sp, PropsID props)
{
	if (sp == NULL)
		return FALSE;
	sp->win_pos_x = prop_get_int (props, "source.paths.win.pos.x", 50);
	sp->win_pos_y = prop_get_int (props, "source.paths.win.pos.y", 50);
	return TRUE;
}

gboolean
src_paths_load (SrcPaths * co, PropsID props)
{
	GList *list, *node;
	gchar *dummy[1];

	g_return_val_if_fail (co != NULL, FALSE);

	gtk_clist_clear (GTK_CLIST (co->widgets.src_clist));
	list = glist_from_data (props, "project.source.paths");
	if (list == NULL)
		return TRUE;

	node = list;
	while (node)
	{
		if (node->data)
		{
			dummy[0] = node->data;
			gtk_clist_append (GTK_CLIST (co->widgets.src_clist),
					  dummy);
		}
		node = g_list_next (node);
	}
	gtk_entry_set_text (GTK_ENTRY (co->widgets.src_entry), "");
	return TRUE;
}

void
src_paths_get (SrcPaths * co)
{
	src_paths_show (co);
}

void
src_paths_show (SrcPaths * co)
{
	if (!co)
		return;
	src_paths_update_controls (co);

	if (co->is_showing)
	{
		gdk_window_raise (co->widgets.window->window);
		return;
	}
	gtk_widget_set_uposition (co->widgets.window, co->win_pos_x,
				  co->win_pos_y);
	gtk_widget_show (co->widgets.window);
	co->is_showing = TRUE;
}

void
src_paths_hide (SrcPaths * co)
{
	if (!co)
		return;
	if (co->is_showing == FALSE)
		return;
	gdk_window_get_root_origin (co->widgets.window->window,
				    &co->win_pos_x, &co->win_pos_y);
	co->is_showing = FALSE;
}

void
src_paths_update_controls (SrcPaths * co)
{
	gint length;

	length = g_list_length (GTK_CLIST (co->widgets.src_clist)->row_list);
	if (length < 2)
		gtk_widget_set_sensitive (co->widgets.src_clear_b, FALSE);
	else
		gtk_widget_set_sensitive (co->widgets.src_clear_b, TRUE);
	if (length < 1)
	{
		gtk_widget_set_sensitive (co->widgets.src_remove_b, FALSE);
		gtk_widget_set_sensitive (co->widgets.src_update_b, FALSE);
	}
	else
	{
		gtk_widget_set_sensitive (co->widgets.src_remove_b, TRUE);
		gtk_widget_set_sensitive (co->widgets.src_update_b, TRUE);
	}
}
