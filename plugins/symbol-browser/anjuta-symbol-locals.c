/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-symbol-locals.c
 * Copyright (C) Naba Kumar 2007 <naba@gnome.org>
 * 
 * anjuta is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * anjuta is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with anjuta.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "anjuta-symbol-locals.h"

static GtkTreeViewClass* parent_class = NULL;

enum {
	PIXBUF_COLUMN,
	NAME_COLUMN,
	COLS_N
};

static void
anjuta_symbol_locals_init (AnjutaSymbolLocals *view)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;
	
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

	/* search through the tree interactively */
	gtk_tree_view_set_enable_search (GTK_TREE_VIEW (view), TRUE);
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (view), NAME_COLUMN);
	
	/* Columns */
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_sizing (column,
					 GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Symbol"));

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_add_attribute (column, renderer, "pixbuf",
					    PIXBUF_COLUMN);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
					    NAME_COLUMN);

	gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
	gtk_tree_view_set_expander_column (GTK_TREE_VIEW (view), column);
}

static void
anjuta_symbol_locals_finalize (GObject *object)
{
	/* TODO: Add deinitalization code here */

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
anjuta_symbol_locals_class_init (AnjutaSymbolLocalsClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	parent_class = GTK_TREE_VIEW_CLASS (g_type_class_peek_parent (klass));

	object_class->finalize = anjuta_symbol_locals_finalize;
}

GType
anjuta_symbol_locals_get_type (void)
{
	static GType our_type = 0;

	if(our_type == 0)
	{
		static const GTypeInfo our_info =
		{
			sizeof (AnjutaSymbolLocalsClass), /* class_size */
			(GBaseInitFunc) NULL, /* base_init */
			(GBaseFinalizeFunc) NULL, /* base_finalize */
			(GClassInitFunc) anjuta_symbol_locals_class_init, /* class_init */
			(GClassFinalizeFunc) NULL, /* class_finalize */
			NULL /* class_data */,
			sizeof (AnjutaSymbolLocals), /* instance_size */
			0, /* n_preallocs */
			(GInstanceInitFunc) anjuta_symbol_locals_init, /* instance_init */
			NULL /* value_table */
		};

		our_type = g_type_register_static (GTK_TYPE_TREE_VIEW, "AnjutaSymbolLocals",
		                                   &our_info, 0);
	}

	return our_type;
}

GtkWidget *
anjuta_symbol_locals_new (void)
{
	return GTK_WIDGET (g_object_new (ANJUTA_TYPE_SYMBOL_LOCALS, NULL));
}
