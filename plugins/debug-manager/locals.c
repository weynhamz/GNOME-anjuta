/*
    locals.c
    Copyright (C) 2000  Kh. Naba Kumar Singh

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any laterdversion.

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

#include "locals.h"

#include "debug_tree.h"

struct _Locals
{
	IAnjutaDebugger *debugger;
	GtkWidget *main_w;
	DebugTree *debug_tree;
	AnjutaPlugin *plugin;
};

static void
locals_updated (const GList *list, gpointer user_data)
{
	Locals *locals = (Locals*) user_data;
	
	g_return_if_fail (locals != NULL);

	if (g_list_length ((GList*)list) < 1)
		return;
	
	debug_tree_remove_all (locals->debug_tree);
	debug_tree_add_watch_list (locals->debug_tree, list, TRUE);	
	debug_tree_update_all(locals->debug_tree,FALSE);
}

/* Public functions
 *---------------------------------------------------------------------------*/

void locals_update (Locals *locals)
{
	ianjuta_debugger_list_local (locals->debugger, locals_updated, locals, NULL);
}

void
locals_clear (Locals *l)
{
	g_return_if_fail (l != NULL);
	debug_tree_remove_all (l->debug_tree);
}

void
locals_connect (Locals *l, IAnjutaDebugger *debugger)
{
	l->debugger = debugger;
	debug_tree_connect (l->debug_tree, debugger);
	
	if (l->main_w == NULL)
	{
		/* Create local window */
		GtkWidget *main_w;
		
		main_w = gtk_scrolled_window_new (NULL, NULL);
		gtk_widget_show (main_w);		
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (main_w),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (main_w),
										 GTK_SHADOW_IN);
		gtk_container_add (GTK_CONTAINER (main_w), debug_tree_get_tree_widget (l->debug_tree));
		gtk_widget_show_all (main_w);
		l->main_w = main_w;

		anjuta_shell_add_widget (l->plugin->shell,
							 l->main_w,
							 "AnjutaDebuggerLocals", _("Locals"),
							 "gdb-locals-icon", ANJUTA_SHELL_PLACEMENT_BOTTOM,
							 NULL);
	}
	
}

/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

Locals *
locals_new (AnjutaPlugin *plugin, IAnjutaDebugger* debugger)
{
	DebugTree *debug_tree;

	Locals *locals = g_new0 (Locals, 1);

	debug_tree = debug_tree_new (plugin, FALSE);

	locals->debugger = debugger;
	locals->debug_tree = debug_tree;
	locals->plugin = plugin;
	debug_tree_connect (locals->debug_tree, locals->debugger);
	
	return locals;
}

void
locals_free (Locals *l)
{
	g_return_if_fail (l != NULL);
	
	debug_tree_free (l->debug_tree);
	if (l->main_w != NULL)
		gtk_widget_destroy (GTK_WIDGET (l->main_w));
	g_free (l);
}
