/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) 2000 Naba Kumar

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

#include <config.h>
#include <gtk/gtk.h>

#include <devhelp/dh-book-tree.h>
// #include <devhelp/dh-history.h>
#include <devhelp/dh-html.h>
#include <devhelp/dh-search.h>
#include <devhelp/dh-base.h>

#include <libanjuta/anjuta-shell.h>
#include <libanjuta/interfaces/ianjuta-help.h>

#include "plugin.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-devhelp.ui"

gpointer parent_class;

/* This is variable required from libdevhelp */
gchar *geometry = NULL;

struct _DevhelpPluginPriv {
	DhBase         *base;
	//	DhHistory      *history;
	DhHtml         *html;

	GtkWidget      *notebook;
	GtkWidget      *book_tree;
	GtkWidget      *search;
	GtkWidget      *html_view;
	GtkWidget      *browser_frame;
};

static void
activate_action (GtkAction *action, DevhelpPlugin *plugin)
{
	DevhelpPluginPriv *priv;
	const gchar  *name = gtk_action_get_name (action);
	
	priv = plugin->priv;
	if (strcmp (name, "ActionDevhelpBack") == 0) {
		dh_html_go_back (priv->html);
		// if (uri) {
		//	dh_html_open_uri (priv->html, uri);
		//	g_free (uri);
		//}
	}
	else if (strcmp (name, "ActionDevhelpForward") == 0) {
		dh_html_go_forward (priv->html);
		//if (uri) {
		//	dh_html_open_uri (priv->html, uri);
		//	g_free (uri);
		//}
	} else {
		g_message ("Unhandled action '%s'", name);
	}
}

static GtkActionEntry actions[] = {
	/* Go menu */
	{ "ActionDevhelpBack", GTK_STOCK_GO_BACK, NULL, NULL, NULL,
	  G_CALLBACK (activate_action)},
	{ "ActionDevhelpForward", GTK_STOCK_GO_FORWARD, NULL, NULL, NULL,
	  G_CALLBACK (activate_action)},
};

static gboolean 
open_url (DevhelpPlugin *plugin, const gchar *url)
{
	DevhelpPluginPriv *priv;
	
	g_return_val_if_fail (url != NULL, FALSE);

	priv = plugin->priv;

	dh_html_open_uri (priv->html, url);
	dh_book_tree_show_uri (DH_BOOK_TREE (priv->book_tree), url);

	return TRUE;
}

static void
location_changed_cb (DhHtml      *html,
					 const gchar *location,
					 DevhelpPlugin *plugin)
{
	//	window_check_history (window);
}

static void
link_selected_cb (GObject *ignored, DhLink *link, DevhelpPlugin *plugin)
{
	DevhelpPluginPriv   *priv;

	g_return_if_fail (link != NULL);
	
	priv = plugin->priv;

	if (open_url (plugin, link->uri)) {
		// dh_history_goto (priv->history, link->uri);
	}
}

static void
back_exists_changed_cb (DhHtml *history, 
						gboolean   exists,
						DevhelpPlugin  *plugin)
{
/*
	DevhelpPluginPriv *priv;
	EggAction *action;
		
	g_return_if_fail (DH_IS_HISTORY (history));
	
	priv = plugin->priv;
	
	action = egg_action_group_get_action (priv->action_group, 
					      "ActionDevhelpBack");
	g_object_set (action, "sensitive", exists, NULL);
*/
}

static void
forward_exists_changed_cb (DhHtml *history, 
							gboolean   exists, 
							DevhelpPlugin  *plugin)
{
/*
	DevhelpPluginPriv *priv;
	EggAction *action;
		
	g_return_if_fail (DH_IS_HISTORY (history));
	
	priv = plugin->priv;
	action = egg_action_group_get_action (priv->action_group, 
					      "ActionDevhelpForward");
	g_object_set (action, "sensitive", exists, NULL);
*/
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	DevhelpPlugin *devhelp_plugin;
	DevhelpPluginPriv *priv;
	
	g_message ("DevhelpPlugin: Activating Devhelp plugin ...");
	devhelp_plugin = (DevhelpPlugin*) plugin;
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	priv = devhelp_plugin->priv;
	
	/* Add all our editor actions */
	anjuta_ui_add_action_group_entries (ui, "ActionGroupDevhelp",
										_("Devhelp navigation operations"),
										actions,
										G_N_ELEMENTS (actions),
										devhelp_plugin);
	devhelp_plugin->uiid = anjuta_ui_merge (ui, UI_FILE);
	
	anjuta_shell_add_widget (plugin->shell, priv->notebook,
							 "AnjutaDevhelpIndex", _("Help"), GTK_STOCK_HELP,
							 ANJUTA_SHELL_PLACEMENT_LEFT, NULL);
	anjuta_shell_add_widget (plugin->shell, priv->browser_frame,
							 "AnjutaDevhelpDisplay", _("Help display"),
							 GTK_STOCK_HELP,
							 ANJUTA_SHELL_PLACEMENT_CENTER, NULL);
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui = anjuta_shell_get_ui (plugin->shell, NULL);
	g_message ("DevhelpPlugin: Dectivating Devhelp plugin ...");
	anjuta_shell_remove_widget (plugin->shell, ((DevhelpPlugin*)plugin)->priv->browser_frame, NULL);
	anjuta_shell_remove_widget (plugin->shell, ((DevhelpPlugin*)plugin)->priv->notebook, NULL);
	anjuta_ui_unmerge (ui, ((DevhelpPlugin*)plugin)->uiid);
	return TRUE;
}

static void
dispose (GObject *obj)
{
	// DevhelpPlugin *plugin = (DevhelpPlugin*)obj;
}

static void
devhelp_plugin_instance_init (GObject *obj)
{
	GtkWidget    *html_sw;
	GtkWidget    *book_tree_sw;
	GNode        *contents_tree;
	GList        *keywords = NULL;
	// GError       *error = NULL;
	DevhelpPluginPriv *priv;
	DevhelpPlugin *plugin = (DevhelpPlugin*)obj;
	
	plugin->uiid = 0;
	plugin->priv = (DevhelpPluginPriv *) g_new0 (DevhelpPluginPriv, 1);
	priv = plugin->priv;
	
	g_message ("Intializing Devhelp plugin");
	
	/* Create plugin widgets */
	// priv->history = dh_history_new ();

	g_signal_connect (priv->html, "location-changed",
					  G_CALLBACK (location_changed_cb), obj);
	// g_signal_connect (priv->history, "forward_exists_changed",
	//				  G_CALLBACK (forward_exists_changed_cb), obj);
	// g_signal_connect (priv->history, "back_exists_changed",
	//				  G_CALLBACK (back_exists_changed_cb), obj);
	
	priv->base      = dh_base_new ();
	priv->html      = dh_html_new ();
	priv->html_view = dh_html_get_widget (priv->html);
	priv->notebook  = gtk_notebook_new ();
	
	html_sw         = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (html_sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	book_tree_sw      = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (book_tree_sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (book_tree_sw),
					     GTK_SHADOW_IN);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (html_sw),
					       priv->html_view);
	
	priv->browser_frame = gtk_frame_new (NULL);
	gtk_container_add (GTK_CONTAINER (priv->browser_frame), html_sw);
	gtk_frame_set_shadow_type (GTK_FRAME (priv->browser_frame), GTK_SHADOW_OUT);

	contents_tree = dh_base_get_book_tree (priv->base);
	keywords      = dh_base_get_keywords  (priv->base);
	
	if (contents_tree) {
		priv->book_tree = dh_book_tree_new (contents_tree);
	
		gtk_container_add (GTK_CONTAINER (book_tree_sw), 
				   priv->book_tree);

		gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook),
					  book_tree_sw,
					  gtk_label_new (_("Contents")));
		g_signal_connect (priv->book_tree, "link_selected", 
				  G_CALLBACK (link_selected_cb),
				  plugin);
	}
	
	if (keywords) {
		priv->search = dh_search_new (keywords);
		
		gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook),
					  priv->search,
					  gtk_label_new (_("Search")));

		g_signal_connect (priv->search, "link_selected",
				  G_CALLBACK (link_selected_cb),
				  plugin);
	}

	gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), 0);

 	g_signal_connect_swapped (priv->html, 
				  "uri_selected", 
				  G_CALLBACK (open_url),
				  plugin);
}

static void
devhelp_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
}

static void
ihelp_search (IAnjutaHelp *help, const gchar *query, GError **err)
{

}

static void
ihelp_iface_init(IAnjutaHelpIface *iface)
{
	iface->search = ihelp_search;
}

ANJUTA_PLUGIN_BEGIN (DevhelpPlugin, devhelp_plugin);
ANJUTA_INTERFACE (ihelp, IANJUTA_TYPE_HELP);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (DevhelpPlugin, devhelp_plugin);
