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
#include <devhelp/dh-html.h>
#include <devhelp/dh-search.h>
#include <devhelp/dh-base.h>

#include <libanjuta/anjuta-shell.h>
#include <libanjuta/interfaces/ianjuta-help.h>
#include <libanjuta/interfaces/ianjuta-editor.h>

#include "plugin.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-devhelp.ui"

gpointer parent_class;

/* This is variable required from libdevhelp */
gchar *geometry = NULL;

struct _DevhelpPluginPriv
{
	gint uiid;
	
	/* Devhelp widgets */
	DhBase         *base;
	DhHtml         *html;
	GtkActionGroup *action_group;
	GtkWidget      *notebook;
	GtkWidget      *book_tree;
	GtkWidget      *search;
	GtkWidget      *html_view;
	GtkWidget      *browser_frame;
	
	/* Watched values */
	gint editor_watch_id;
	GObject *editor;
};

/* Devhelp stuffs */
static void
devhelp_check_history (DevhelpPlugin *plugin)
{
	DevhelpPluginPriv *priv;
	GtkAction *action;
		
	priv = plugin->priv;
	
	if (priv->html)
	{
		action = gtk_action_group_get_action (priv->action_group, 
							  "ActionDevhelpForward");
		
		g_object_set (action, "sensitive", 
				  dh_html_can_go_forward (priv->html), NULL);
		action = gtk_action_group_get_action (priv->action_group,
							  "ActionDevhelpBack");
		g_object_set (action, "sensitive",
				  dh_html_can_go_back (priv->html), NULL);
	}
	else
	{
		action = gtk_action_group_get_action (priv->action_group, 
							  "ActionDevhelpForward");
		
		g_object_set (action, "sensitive", FALSE, NULL);
		action = gtk_action_group_get_action (priv->action_group,
							  "ActionDevhelpBack");
		g_object_set (action, "sensitive", FALSE, NULL);
	}
}

static gboolean 
devhelp_open_url (DevhelpPlugin *plugin, const gchar *url)
{
	DevhelpPluginPriv *priv;
	
	g_return_val_if_fail (url != NULL, FALSE);

	priv = plugin->priv;

	dh_html_open_uri (priv->html, url);
	dh_book_tree_show_uri (DH_BOOK_TREE (priv->book_tree), url);
	devhelp_check_history (plugin);

	anjuta_shell_present_widget (ANJUTA_PLUGIN (plugin)->shell,
								 priv->browser_frame, NULL);
	return TRUE;
}

static void
devhelp_location_changed_cb (DhHtml      *html,
					 const gchar *location,
					 DevhelpPlugin *plugin)
{
	devhelp_check_history (plugin);
	anjuta_shell_present_widget (ANJUTA_PLUGIN (plugin)->shell,
								 plugin->priv->browser_frame, NULL);
}

static void
devhelp_link_selected_cb (GObject *ignored, DhLink *link, DevhelpPlugin *plugin)
{
	DevhelpPluginPriv   *priv;

	g_return_if_fail (link != NULL);
	
	priv = plugin->priv;

	devhelp_open_url (plugin, link->uri);
}

/* The ugliest hack. When switching tabs, the selection and cursor is changed
 * for the tree view so the html content is changed. Block the signal during
 * switch.
 */
static void
devhelp_notebook_switch_page_cb (GtkWidget       *notebook,
						GtkNotebookPage *page,
						guint            page_num,
						DevhelpPlugin   *plugin)
{
	DevhelpPluginPriv *priv;
	priv = plugin->priv;

	g_signal_handlers_block_by_func (priv->book_tree, 
					 devhelp_link_selected_cb, plugin);
}

static void
devhelp_notebook_switch_page_after_cb (GtkWidget       *notebook,
							GtkNotebookPage *page,
							guint            page_num,
							DevhelpPlugin   *plugin)
{
	DevhelpPluginPriv *priv;
	priv = plugin->priv;
	priv = plugin->priv;
	
	g_signal_handlers_unblock_by_func (priv->book_tree, 
					   devhelp_link_selected_cb, plugin);
}

static void
devhelp_html_initialize (DevhelpPlugin *plugin)
{
	GtkWidget    *html_sw;
	GtkWidget    *book_tree_sw;
	GNode        *contents_tree;
	GList        *keywords = NULL;
	DevhelpPluginPriv *priv;
	
	priv = plugin->priv;
	
	if (priv->html)
		return;
	
	/* Create plugin widgets */
	priv->html      = dh_html_new ();
	g_signal_connect (priv->html, "location-changed",
					  G_CALLBACK (devhelp_location_changed_cb), plugin);
	
	priv->notebook  = gtk_notebook_new ();
	priv->html_view = dh_html_get_widget (priv->html);
	
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
	
	priv->browser_frame = html_sw;
	
	priv->base    = dh_base_new ();
	contents_tree = dh_base_get_book_tree (priv->base);
	keywords      = dh_base_get_keywords  (priv->base);
	
	if (keywords) {
		priv->search = dh_search_new (keywords);
		
		gtk_notebook_prepend_page (GTK_NOTEBOOK (priv->notebook),
					  priv->search,
					  gtk_label_new (_("Search")));

		g_signal_connect (priv->search, "link_selected",
				  G_CALLBACK (devhelp_link_selected_cb),
				  plugin);
	}

	if (contents_tree) {
		priv->book_tree = dh_book_tree_new (contents_tree);
	
		gtk_container_add (GTK_CONTAINER (book_tree_sw), 
				   priv->book_tree);

		gtk_notebook_prepend_page (GTK_NOTEBOOK (priv->notebook),
					  book_tree_sw,
					  gtk_label_new (_("Contents")));
		g_signal_connect (priv->book_tree, "link_selected", 
				  G_CALLBACK (devhelp_link_selected_cb),
				  plugin);
	}
	
	gtk_widget_show_all (priv->browser_frame);
	gtk_widget_show_all (priv->notebook);
	gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), 0);
	g_signal_connect (priv->notebook, "switch_page",
			  G_CALLBACK (devhelp_notebook_switch_page_cb),
			  plugin);

	g_signal_connect_after (priv->notebook, "switch_page",
				G_CALLBACK (devhelp_notebook_switch_page_after_cb),
				plugin);

	/* Add widgets to shell */
	anjuta_shell_add_widget (ANJUTA_PLUGIN (plugin)->shell, priv->notebook,
							 "AnjutaDevhelpIndex", _("Help"), GTK_STOCK_HELP,
							 ANJUTA_SHELL_PLACEMENT_LEFT, NULL);
	anjuta_shell_add_widget (ANJUTA_PLUGIN (plugin)->shell, priv->browser_frame,
							 "AnjutaDevhelpDisplay", _("Help display"),
							 GTK_STOCK_HELP,
							 ANJUTA_SHELL_PLACEMENT_CENTER, NULL);
/*
 	g_signal_connect_swapped (priv->html, 
				  "uri_selected", 
				  G_CALLBACK (open_url),
				  plugin);
*/
#if 0
	gtk_widget_show_all (GTK_WIDGET (widget));

	/* Make sure that the HTML widget is realized before trying to 
	 * clear it. Solves bug #147343.
	 */
	while (g_main_context_pending (NULL)) {
		g_main_context_iteration (NULL, FALSE);
	}

	dh_html_clear (plugin->priv->html);
#endif
}

/* Action callbacks */
static void
on_go_back_activate (GtkAction *action, DevhelpPlugin *plugin)
{
	devhelp_html_initialize (plugin);
	dh_html_go_back (plugin->priv->html);
}

static void
on_go_forward_activate (GtkAction *action, DevhelpPlugin *plugin)
{
	devhelp_html_initialize (plugin);
	dh_html_go_forward (plugin->priv->html);
}

static void
on_api_reference_activate (GtkAction * action, DevhelpPlugin *dh_plugin)
{
	devhelp_html_initialize (dh_plugin);
	gtk_notebook_set_current_page (GTK_NOTEBOOK (dh_plugin->priv->notebook), 0);
	anjuta_shell_present_widget (ANJUTA_PLUGIN (dh_plugin)->shell,
								 dh_plugin->priv->notebook, NULL);
}

static void
on_context_help_activate (GtkAction * action, DevhelpPlugin *dh_plugin)
{
	IAnjutaEditor *editor;
	gchar *current_word;
	
	if(dh_plugin->priv->editor == NULL)
		return;

	if (IANJUTA_IS_EDITOR (dh_plugin->priv->editor) == FALSE)
	{
		g_warning ("Current Editor does not support IAnjutaEditor interface");
		return;
	}
	editor = IANJUTA_EDITOR (dh_plugin->priv->editor);
	current_word = ianjuta_editor_get_current_word (editor, NULL);
	if (current_word)
	{
		devhelp_html_initialize (dh_plugin);
		ianjuta_help_search (IANJUTA_HELP (dh_plugin), current_word, NULL);
		g_free (current_word);
	}
}

static void
on_search_help_activate (GtkAction * action, DevhelpPlugin *dh_plugin)
{
	GtkWindow *parent;
	gboolean status;
	gchar *search_term = NULL;
	
	parent = GTK_WINDOW (ANJUTA_PLUGIN (dh_plugin)->shell);
	status =
		anjuta_util_dialog_input (parent, _("Search Help:"), &search_term);
	if (status)
	{
		if (search_term && strlen (search_term) > 0)
		{
			devhelp_html_initialize (dh_plugin);
			ianjuta_help_search (IANJUTA_HELP (dh_plugin), search_term, NULL);
		}
	}
	g_free (search_term);
}

/* Action definitions */
static GtkActionEntry actions[] = {
	/* Go menu */
	{
		"ActionMenuGoto",
		NULL,
		N_("_Goto"),
		NULL,
		NULL,
		NULL
	},
	{
		"ActionDevhelpBack",
		GTK_STOCK_GO_BACK,
		N_("Previous Help"),
		NULL,
		N_("Go to previous help page"),
		G_CALLBACK (on_go_back_activate)
	},
	{
		"ActionDevhelpForward",
		GTK_STOCK_GO_FORWARD,
		N_("Next Help"),
		NULL,
		N_("Go to next help page"),
		G_CALLBACK (on_go_forward_activate)
	},
	{
		"ActionHelpApi",
		NULL,
		N_("_API references"),
		NULL,
		N_("Browse API Pages"),
		G_CALLBACK (on_api_reference_activate)
	},
	{
		"ActionHelpContext",
		GTK_STOCK_HELP,
		N_("_Context Help"),
		"<control>h",
		N_("Search help for the current word in the editor"),
		G_CALLBACK (on_context_help_activate)
	},
	{
		"ActionHelpSearch",
		NULL,
		N_("_Search Help"),
		NULL,
		N_("Search for a term in help"),
		G_CALLBACK (on_search_help_activate)
	}
};

/* Watches callbacks */

static void
value_added_current_editor (AnjutaPlugin *plugin, const char *name,
							const GValue *value, gpointer dh_plugin)
{
	GtkAction *action;
	GObject *editor;
	DevhelpPluginPriv *priv;
	
	priv = ((DevhelpPlugin*)dh_plugin)->priv;
	editor = g_value_get_object (value);
	priv->editor = editor;
	action = gtk_action_group_get_action (priv->action_group,
										  "ActionHelpContext");
	g_object_set (action, "sensitive", TRUE, NULL);
}

static void
value_removed_current_editor (AnjutaPlugin *plugin,
							  const char *name, gpointer dh_plugin)
{
	GtkAction *action;
	DevhelpPluginPriv *priv;
	
	priv = ((DevhelpPlugin*)dh_plugin)->priv;
	priv->editor = NULL;
	action = gtk_action_group_get_action (priv->action_group,
										  "ActionHelpContext");
	g_object_set (action, "sensitive", FALSE, NULL);
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	DevhelpPlugin *devhelp_plugin;
	DevhelpPluginPriv *priv;
	GtkAction *action;
	
	g_message ("DevhelpPlugin: Activating Devhelp plugin ...");
	devhelp_plugin = (DevhelpPlugin*) plugin;
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	priv = devhelp_plugin->priv;
	
	/* Add action group */
	priv->action_group = 
		anjuta_ui_add_action_group_entries (ui, "ActionGroupDevhelp",
										_("Devhelp navigation operations"),
										actions,
										G_N_ELEMENTS (actions),
										devhelp_plugin);
	action = anjuta_ui_get_action (ui, "ActionGroupDevhelp",
								   "ActionHelpContext");
	g_object_set (G_OBJECT (action), "short-label", _("Help"), NULL);
	
	/* Add UI */
	priv->uiid = anjuta_ui_merge (ui, UI_FILE);
	
	action = gtk_action_group_get_action (priv->action_group,
										  "ActionHelpContext");
	g_object_set (action, "sensitive", FALSE, NULL);

	if (priv->html)
	{
		/* Add widgets */
		anjuta_shell_add_widget (plugin->shell, priv->notebook,
								 "AnjutaDevhelpIndex", _("Help"), GTK_STOCK_HELP,
								 ANJUTA_SHELL_PLACEMENT_LEFT, NULL);
		anjuta_shell_add_widget (plugin->shell, priv->browser_frame,
								 "AnjutaDevhelpDisplay", _("Help display"),
								 GTK_STOCK_HELP,
								 ANJUTA_SHELL_PLACEMENT_CENTER, NULL);
	}
	
	/* FIXME:
	 * We can delay devhelp initialization (hence making the initial startup
	 * faster) by commenting the following line. However, if that is done,
	 * the window layout gets screwed up when the widgets are added later.
	 */
	devhelp_html_initialize (devhelp_plugin);
	
	/* Add watches */
	priv->editor_watch_id = 
		anjuta_plugin_add_watch (plugin, "document_manager_current_editor",
								 value_added_current_editor,
								 value_removed_current_editor,
								 plugin);
	devhelp_check_history (devhelp_plugin);
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	DevhelpPluginPriv *priv;
	
	priv = ((DevhelpPlugin*)plugin)->priv;
	
	AnjutaUI *ui = anjuta_shell_get_ui (plugin->shell, NULL);
	g_message ("DevhelpPlugin: Dectivating Devhelp plugin ...");
	
	if (priv->html)
	{
		/* Remove widgets */
		anjuta_shell_remove_widget (plugin->shell, priv->browser_frame, NULL);
		anjuta_shell_remove_widget (plugin->shell, priv->notebook, NULL);
	}
	
	/* Remove UI */
	anjuta_ui_unmerge (ui, priv->uiid);
	
	/* Remove action group */
	anjuta_ui_remove_action_group (ui, priv->action_group);
	
	/* Remove watch */
	anjuta_plugin_remove_watch (plugin, priv->editor_watch_id, TRUE);
	
	return TRUE;
}

static void
dispose (GObject *obj)
{
	// DevhelpPlugin *plugin = (DevhelpPlugin*)obj;
	/* Devhelp widgets should be destroyed */
}

static void
devhelp_plugin_instance_init (GObject *obj)
{
	DevhelpPluginPriv *priv;
	DevhelpPlugin *plugin = (DevhelpPlugin*)obj;
	
	plugin->priv = (DevhelpPluginPriv *) g_new0 (DevhelpPluginPriv, 1);
	priv = plugin->priv;
	
	g_message ("Intializing Devhelp plugin");
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
	DevhelpPluginPriv *priv;
	
	priv = ((DevhelpPlugin*)help)->priv;
	dh_search_set_search_string (DH_SEARCH (priv->search), query);
	gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), 1);
	anjuta_shell_present_widget (ANJUTA_PLUGIN (help)->shell,
								 priv->notebook, NULL);
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
