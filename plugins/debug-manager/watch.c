/* 
 * watch.c Copyright (C) 2000 Kh. Naba Kumar Singh
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along with 
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple 
 * Place, Suite 330, Boston, MA 02111-1307 USA */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "watch.h"

#include "debug_tree.h"

/* Type
 *---------------------------------------------------------------------------*/

struct _ExprWatch
{
	AnjutaPlugin *plugin;
	
	GtkWidget *scrolledwindow;
	DebugTree *debug_tree;	
	IAnjutaDebugger *debugger;
	
	/* Menu action */
	GtkActionGroup *action_group;
};

static void
create_expr_watch_gui (ExprWatch * ew)
{
	ew->debug_tree = debug_tree_new (ew->plugin, TRUE);
	ew->scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (ew->scrolledwindow);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (ew->scrolledwindow),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (ew->scrolledwindow),
										 GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (ew->scrolledwindow), debug_tree_get_tree_widget (ew->debug_tree));
	gtk_widget_show_all (ew->scrolledwindow);
}

/* Public function
 *---------------------------------------------------------------------------*/

void
expr_watch_cmd_queqe (ExprWatch *ew)
{
	debug_tree_update_all (ew->debug_tree, FALSE);
}

void
expr_watch_connect (ExprWatch *ew, IAnjutaDebugger *debugger)
{
	ew->debugger = debugger;
	debug_tree_connect (ew->debug_tree, debugger);
}

/* Callback for saving session
 *---------------------------------------------------------------------------*/

static void
on_session_save (AnjutaShell *shell, AnjutaSessionPhase phase, AnjutaSession *session, ExprWatch *ew)
{
	GList *list;

	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;

	list = debug_tree_get_full_watch_list (ew->debug_tree);
	if (list != NULL)
		anjuta_session_set_string_list (session, "Debugger", "Watch", list);
}

static void
on_session_load (AnjutaShell *shell, AnjutaSessionPhase phase, AnjutaSession *session, ExprWatch *ew)
{
	GList *list;

	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;

	/* debug_tree_remove_all (ew->debug_tree); */
	list = anjuta_session_get_string_list (session, "Debugger", "Watch");
	if (list != NULL)
		debug_tree_add_full_watch_list (ew->debug_tree, list);
}

/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

ExprWatch *
expr_watch_new (AnjutaPlugin *plugin, IAnjutaDebugger *debugger)
{
	ExprWatch *ew;

	ew = g_new0 (ExprWatch, 1);
	ew->plugin = plugin;
	create_expr_watch_gui (ew);
	g_object_ref (debugger);
	ew->debugger = debugger;

	/* Connect to Load and Save event */
	g_signal_connect (plugin->shell, "save-session",
					  G_CALLBACK (on_session_save), ew);
    	g_signal_connect (plugin->shell, "load-session",
					  G_CALLBACK (on_session_load), ew);
	
	/* Add watch window */
	anjuta_shell_add_widget (plugin->shell,
							 ew->scrolledwindow,
                             "AnjutaDebuggerWatch", _("Watches"),
                             "gdb-watch-icon", ANJUTA_SHELL_PLACEMENT_BOTTOM,
                              NULL);

	
	return ew;
}

void
expr_watch_destroy (ExprWatch * ew)
{
	g_return_if_fail (ew != NULL);
	
	/* Disconnect from Load and Save event */
	g_signal_handlers_disconnect_by_func (ew->plugin->shell, G_CALLBACK (on_session_save), ew);
	g_signal_handlers_disconnect_by_func (ew->plugin->shell, G_CALLBACK (on_session_load), ew);

	debug_tree_free (ew->debug_tree);
	gtk_widget_destroy (ew->scrolledwindow);
	g_free (ew);
}
