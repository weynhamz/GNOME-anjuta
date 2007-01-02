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

#include <libanjuta/anjuta-debug.h>

struct _Locals
{
	IAnjutaDebugger *debugger;
	GtkWidget *main_w;
	DebugTree *debug_tree;
	AnjutaPlugin *plugin;
};

static void
locals_updated (const gpointer data, gpointer user_data, GError *error)
{
	const GList *list = (const GList *)data;
	Locals *locals = (Locals*) user_data;
	
	g_return_if_fail (locals != NULL);

	if (error != NULL)
		return;
	
	if (g_list_length ((GList*)list) < 1)
		return;

	printf ("replace list\n");
	debug_tree_replace_list (locals->debug_tree, list);
	printf ("update all\n");
	debug_tree_update_all(locals->debug_tree);
	printf ("replace list end\n");
}

/* Private functions
 *---------------------------------------------------------------------------*/

static void
create_locals_gui (Locals *l)
{
	if (l->debug_tree == NULL)
	{
		l->debug_tree = debug_tree_new (l->plugin);
		debug_tree_connect (l->debug_tree, l->debugger);
	}

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

static void
destroy_locals_gui (Locals *l)
{
	if (l->debug_tree != NULL)
	{
		debug_tree_free (l->debug_tree);
		l->debug_tree = NULL;
	}
	if (l->main_w != NULL)
	{
		gtk_widget_destroy (GTK_WIDGET (l->main_w));
		l->main_w = NULL;
	}
}

/* Private functions
 *---------------------------------------------------------------------------*/

static void locals_update (Locals *locals)
{
	ianjuta_debugger_list_local (locals->debugger, locals_updated, locals, NULL);
}

static void
locals_clear (Locals *l)
{
	g_return_if_fail (l != NULL);
	debug_tree_remove_all (l->debug_tree);
}

static void
on_program_stopped (Locals *l)
{
	locals_update (l);
}

static void
on_debugger_started (Locals *l)
{
	create_locals_gui (l);
}

static void
on_debugger_stopped (Locals *l)
{
	locals_clear (l);
	destroy_locals_gui (l);
}

/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

Locals *
locals_new (AnjutaPlugin *plugin, IAnjutaDebugger* debugger)
{
	DebugTree *debug_tree;

	Locals *locals = g_new0 (Locals, 1);

	debug_tree = debug_tree_new (plugin);

	locals->debugger = debugger;
	if (debugger != NULL) g_object_ref (debugger);
	locals->plugin = plugin;

	g_signal_connect_swapped (locals->debugger, "debugger-started", G_CALLBACK (on_debugger_started), locals);
	g_signal_connect_swapped (locals->debugger, "debugger-stopped", G_CALLBACK (on_debugger_stopped), locals);
	g_signal_connect_swapped (locals->debugger, "program-stopped", G_CALLBACK (on_program_stopped), locals);
	
	return locals;
}

void
locals_free (Locals *l)
{
	g_return_if_fail (l != NULL);

	/* Destroy gui */
	destroy_locals_gui (l);
	
	/* Disconnect from debugger */
	if (l->debugger != NULL)
	{	
		g_signal_handlers_disconnect_by_func (l->debugger, G_CALLBACK (on_debugger_started), l);
		g_signal_handlers_disconnect_by_func (l->debugger, G_CALLBACK (on_debugger_stopped), l);
		g_signal_handlers_disconnect_by_func (l->debugger, G_CALLBACK (on_program_stopped), l);
		g_object_unref (l->debugger);	
	}

	g_free (l);
}
