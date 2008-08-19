/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    utils.c
    Copyright (C) 2008 Sébastien Granjoux

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*
 * Miscellaneous utilities functions
 *---------------------------------------------------------------------------*/

#include <config.h>

#include "utils.h"

/* Constants
 *---------------------------------------------------------------------------*/

/* Helpers functions
 *---------------------------------------------------------------------------*/

#include <string.h>

gboolean
run_plugin_gtk_tree_model_find_string (GtkTreeModel *model, GtkTreeIter *parent,
		GtkTreeIter *iter, guint col, const gchar *value)

{
	gboolean valid;
	gboolean found = FALSE;

	g_return_val_if_fail (GTK_IS_TREE_MODEL (model), FALSE);
	g_return_val_if_fail (iter != NULL, FALSE);
	g_return_val_if_fail (value != NULL, FALSE);

	if (!parent)
	{
		valid = gtk_tree_model_get_iter_first (model, iter);
	}
	else 
	{
		valid = gtk_tree_model_iter_children (model, iter, parent);
	}

	while (valid)
	{
		gchar *mvalue;
		
		gtk_tree_model_get (model, iter, col, &mvalue, -1);
		found = (mvalue != NULL) && (strcmp (mvalue, value) == 0);
		g_free (mvalue);
		if (found) break;						   

		if (gtk_tree_model_iter_has_child (model, iter))
		{
			GtkTreeIter citer;
			
			found = run_plugin_gtk_tree_model_find_string (model, iter,
														   &citer, col, value);
			if (found)
			{
				*iter = citer;
				break;
			}
		}
		valid = gtk_tree_model_iter_next (model, iter);
	}

	return found;
}
