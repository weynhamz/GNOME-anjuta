/*
    locals.c
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

#include "locals.h"

#include "debug_tree.h"

struct _Locals
{
	Debugger *debugger;
	GtkWidget *main_w;
	DebugTree *debug_tree;
};

static void
locals_clear (Locals *l)
{
	g_return_if_fail (l != NULL);

	debug_tree_clear (l->debug_tree);
}

static void
locals_update (Debugger *debugger, const GDBMIValue *mi_results,
			   const GList *cli_results, gpointer user_data)
{
	Locals *locals = (Locals*) user_data;
	g_return_if_fail (locals != NULL);

	if (g_list_length ((GList*)cli_results) < 1)
		return;

	debug_tree_parse_variables (locals->debug_tree, cli_results);
}

static void
on_program_stopped (Debugger *debugger, GDBMIValue *mi_results, Locals *locals)
{
	debugger_command (locals->debugger, "info locals", TRUE,
					  locals_update, locals);
}

Locals *
locals_create (Debugger *debugger)
{
	DebugTree *debug_tree;
	GtkWidget *main_w;

	Locals *locals = g_malloc (sizeof (Locals));

	debug_tree = debug_tree_create (debugger);

	main_w = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (main_w),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (main_w),
										 GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (main_w), debug_tree->tree);
	gtk_widget_show_all (main_w);

	g_object_ref (debugger);
	g_signal_connect (debugger, "program-stopped",
					  G_CALLBACK (on_program_stopped), locals);
	g_signal_connect_swapped (debugger, "program-exited",
							  G_CALLBACK (locals_clear), locals);

	locals->debugger = debugger;
	locals->main_w = main_w;
	locals->debug_tree = debug_tree;

	return locals;
}

GtkWidget *
locals_get_main_widget (Locals *l)
{
	g_return_val_if_fail (l != NULL, NULL);

	return l->main_w;
}

void
locals_destroy (Locals *l)
{
	g_return_if_fail (l != NULL);
	
	g_signal_handlers_disconnect_by_func (l->debugger,
										  G_CALLBACK (on_program_stopped), l);
	g_signal_handlers_disconnect_by_func (l->debugger,
										  G_CALLBACK (locals_update), l);
	g_object_unref (l->debugger);

	debug_tree_destroy (l->debug_tree);
	gtk_widget_destroy (GTK_WIDGET (l->main_w));
	g_free (l);
}
