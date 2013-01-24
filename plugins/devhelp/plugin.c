/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * plugin.c
 * Copyright (C) Johannes Schmid 2006 <jhs@cvs.gnome.org>
 * 
 * plugin.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * plugin.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with plugin.c.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include <config.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-tabber.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-help.h>

#include "plugin.h"

#ifndef DISABLE_EMBEDDED_DEVHELP

#ifdef HAVE_WEBKIT2
#include <webkit2/webkit2.h>
#else
#include <webkit/webkit.h>
#endif

#define ONLINE_API_DOCS "http://library.gnome.org/devel"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-devhelp.ui"

#else /* DISABLE_EMBEDDED_DEVHELP */

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-devhelp-simple.ui"

#endif /* DISABLE_EMBEDDED_DEVHELP */

static gpointer parent_class;

#ifndef DISABLE_EMBEDDED_DEVHELP

#define ANJUTA_PIXMAP_DEVHELP "anjuta-devhelp-plugin-48.png"
#define ANJUTA_PIXMAP_DEVHELP_SEARCH "anjuta-devhelp-search"
#define ANJUTA_PIXMAP_DEVHELP_VIEW "anjuta-devhelp-view"


#define ANJUTA_STOCK_DEVHELP "anjuta-devhelp"
#define ANJUTA_STOCK_DEVHELP_SEARCH "anjuta-devhelp-search"
#define ANJUTA_STOCK_DEVHELP_VIEW "anjuta-devhelp-view"

static void
register_stock_icons (AnjutaPlugin *plugin)
{
	static gboolean registered = FALSE;

	if (registered)
		return;
	registered = TRUE;

	/* Register stock icons */
	BEGIN_REGISTER_ICON (plugin);
	REGISTER_ICON (ANJUTA_PIXMAP_DEVHELP, ANJUTA_STOCK_DEVHELP);
	REGISTER_ICON_FULL (ANJUTA_PIXMAP_DEVHELP_VIEW, ANJUTA_STOCK_DEVHELP_VIEW);
	REGISTER_ICON_FULL (ANJUTA_PIXMAP_DEVHELP_SEARCH, ANJUTA_STOCK_DEVHELP_SEARCH);
	END_REGISTER_ICON
}

static void
devhelp_tree_link_selected_cb (GObject       *ignored, DhLink *link,
							   AnjutaDevhelp *widget)
{
	gchar *uri;
	
	anjuta_shell_present_widget (ANJUTA_PLUGIN (widget)->shell,
								 widget->present_widget, NULL);

	uri = dh_link_get_uri (link);
#ifdef HAVE_WEBKIT2
	webkit_web_view_load_uri (WEBKIT_WEB_VIEW (widget->view), uri);
#else
	webkit_web_view_open (WEBKIT_WEB_VIEW (widget->view), uri);
#endif
	g_free (uri);
	
	anjuta_devhelp_check_history (widget);
}

static void
devhelp_search_link_selected_cb (GObject  *ignored, DhLink *link,
								 AnjutaDevhelp *widget)
{
	gchar *uri;
	
	anjuta_shell_present_widget (ANJUTA_PLUGIN (widget)->shell,
								 widget->present_widget, NULL);

	uri = dh_link_get_uri (link);
#ifdef HAVE_WEBKIT2
	webkit_web_view_load_uri (WEBKIT_WEB_VIEW (widget->view), uri);
#else
	webkit_web_view_open (WEBKIT_WEB_VIEW (widget->view), uri);
#endif
	g_free (uri);
	
	anjuta_devhelp_check_history (widget);
}

static void
on_go_back_clicked (GtkWidget *widget, AnjutaDevhelp *plugin)
{
	anjuta_shell_present_widget (ANJUTA_PLUGIN (plugin)->shell,
								 plugin->present_widget, NULL);

	webkit_web_view_go_back (WEBKIT_WEB_VIEW (plugin->view));
	
	anjuta_devhelp_check_history (plugin);
}

static void
on_go_forward_clicked (GtkWidget *widget, AnjutaDevhelp *plugin)
{
	anjuta_shell_present_widget (ANJUTA_PLUGIN (plugin)->shell,
								 plugin->present_widget, NULL);

	webkit_web_view_go_forward (WEBKIT_WEB_VIEW (plugin->view));
	
	anjuta_devhelp_check_history (plugin);
}

static void
on_online_clicked (GtkWidget* widget, AnjutaDevhelp* plugin)
{
	anjuta_shell_present_widget (ANJUTA_PLUGIN (plugin)->shell,
								 plugin->present_widget, NULL);
#ifdef HAVE_WEBKIT2
	webkit_web_view_load_uri (WEBKIT_WEB_VIEW (plugin->view),
							  ONLINE_API_DOCS);
#else
	webkit_web_view_open (WEBKIT_WEB_VIEW (plugin->view),
						  ONLINE_API_DOCS);
#endif
	anjuta_devhelp_check_history (plugin);
}

static gboolean
api_reference_idle (AnjutaDevhelp* plugin)
{	
	gtk_notebook_set_current_page (GTK_NOTEBOOK (plugin->control_notebook), 0);
	anjuta_shell_present_widget (ANJUTA_PLUGIN (plugin)->shell,
								 plugin->control_notebook, NULL);
	return FALSE;
}

#else /* DISABLE_EMBEDDED_DEVHELP */

static gboolean
api_reference_idle (AnjutaDevhelp* plugin)
{
	ianjuta_help_search (IANJUTA_HELP (plugin), "", NULL);
	return FALSE;
}

#endif

static void
on_api_reference_activate (GtkAction *action, AnjutaDevhelp *plugin)
{
	g_idle_add((GSourceFunc) api_reference_idle, plugin);
}

static gboolean
context_idle (AnjutaDevhelp* plugin)
{
	IAnjutaEditor *editor;
	gchar *current_word;
		
	if(plugin->editor == NULL)
		return FALSE;

	editor = IANJUTA_EDITOR (plugin->editor);
	current_word = ianjuta_editor_get_current_word (editor, NULL);
	if (current_word)
	{
		ianjuta_help_search (IANJUTA_HELP (plugin), current_word, NULL);
		g_free (current_word);
	}
	return FALSE;
}

static void
on_context_help_activate (GtkAction *action, AnjutaDevhelp *plugin)
{
	g_idle_add((GSourceFunc) context_idle, plugin);
}

static void
on_search_help_activate (GtkAction *action, AnjutaDevhelp *plugin)
{
	GtkWindow *parent;
	gboolean status;
	gchar *search_term = NULL;
	
	parent = GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell);
	status =
		anjuta_util_dialog_input (parent, _("Search Help:"),
								  NULL, &search_term);
	if (status)
	{
		if (search_term && strlen (search_term) > 0)
		{
			ianjuta_help_search (IANJUTA_HELP (plugin), search_term, NULL);
		}
	}
	g_free (search_term);
}

/* Action definitions */
static GtkActionEntry actions[] = {

#ifndef DISABLE_EMBEDDED_DEVHELP

	/* Go menu */
	{
		"ActionMenuGoto",
		NULL,
		N_("_Go to"),
		NULL,
		NULL,
		NULL
	},
#endif /* DISABLE_EMBEDDED_DEVHELP */
	{
		"ActionHelpApi",
		NULL,
		N_("_API Reference"),
		NULL,
		N_("Browse API Pages"),
		G_CALLBACK (on_api_reference_activate)
	},
	{
		"ActionHelpContext",
#ifndef DISABLE_EMBEDDED_DEVHELP
		ANJUTA_STOCK_DEVHELP_VIEW,
#else
		NULL,
#endif /* DISABLE_EMBEDDED_DEVHELP */
		N_("_Context Help"),
		"<shift>F1",
		N_("Search help for the current word in the editor"),
		G_CALLBACK (on_context_help_activate)
	},
	{
		"ActionHelpSearch",
#ifndef DISABLE_EMBEDDED_DEVHELP
		ANJUTA_STOCK_DEVHELP_SEARCH,
#else
		NULL,
#endif
		N_("_Search Help"),
		NULL,
		N_("Search for a term in help"),
		G_CALLBACK (on_search_help_activate)
	}
};

/* Watches callbacks */

static void
value_added_current_editor (AnjutaPlugin *plugin, const gchar *name,
							const GValue *value, gpointer data)
{
	GtkAction *action;
	GObject *editor;
	AnjutaDevhelp* devhelp = ANJUTA_PLUGIN_DEVHELP (data);
	
	editor = g_value_get_object (value);
	if (!IANJUTA_IS_EDITOR (editor))
		return;
	devhelp->editor = IANJUTA_EDITOR(editor);
	action = gtk_action_group_get_action (devhelp->action_group,
										  "ActionHelpContext");
	g_object_set (action, "sensitive", TRUE, NULL);
}

static void
value_removed_current_editor (AnjutaPlugin *plugin,
							  const gchar *name, gpointer data)
{
	GtkAction *action;
	AnjutaDevhelp* devhelp = ANJUTA_PLUGIN_DEVHELP (data);	
		
	devhelp->editor = NULL;
	action = gtk_action_group_get_action (devhelp->action_group,
										  "ActionHelpContext");
	g_object_set (action, "sensitive", FALSE, NULL);
}

#ifndef DISABLE_EMBEDDED_DEVHELP

#ifdef HAVE_WEBKIT2
static void on_load_changed (WebKitWebView* web_view,
							 WebKitLoadEvent load_event, gpointer user_data)
{
	AnjutaDevhelp* devhelp = ANJUTA_PLUGIN_DEVHELP(user_data);

	if (load_event == WEBKIT_LOAD_FINISHED)
		anjuta_devhelp_check_history(devhelp);
}
#else
static void on_load_finished (GObject* view, GObject* frame, gpointer user_data)
{
	AnjutaDevhelp* devhelp = ANJUTA_PLUGIN_DEVHELP(user_data);
	anjuta_devhelp_check_history(devhelp);
}
#endif

#endif

static gboolean
devhelp_activate (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	AnjutaDevhelp *devhelp;

#ifndef DISABLE_EMBEDDED_DEVHELP
	static gboolean init = FALSE;
	GtkWidget *label;
	GtkWidget *books_sw;
	
	if (!init)
	{
		register_stock_icons (plugin);
		init = TRUE;
	}
#endif

	DEBUG_PRINT ("%s", "AnjutaDevhelp: Activating AnjutaDevhelp plugin ...");
	devhelp = ANJUTA_PLUGIN_DEVHELP (plugin);

	/* Add all UI actions and merge UI */
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	devhelp->action_group =
		anjuta_ui_add_action_group_entries (ui, "ActionGroupDevhelp",
											_("Help operations"),
											actions,
											G_N_ELEMENTS (actions),
											GETTEXT_PACKAGE, TRUE,
											plugin);
	devhelp->uiid = anjuta_ui_merge (ui, UI_FILE);

#ifndef DISABLE_EMBEDDED_DEVHELP
	devhelp->control_notebook = gtk_notebook_new ();
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (devhelp->control_notebook),
	                            FALSE);

	/* Tabs in grip */
	devhelp->tab_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);	

	gtk_box_pack_start (GTK_BOX (devhelp->tab_hbox),
	                    gtk_image_new_from_stock (ANJUTA_STOCK_DEVHELP_SEARCH,
	                                              GTK_ICON_SIZE_MENU),
	                    FALSE, FALSE, 0);
	label = gtk_label_new (_("API Browser"));
	devhelp->tabber = anjuta_tabber_new (GTK_NOTEBOOK(devhelp->control_notebook));
	
	
	gtk_box_pack_start (GTK_BOX (devhelp->tab_hbox),
	                    label,
	                    FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (devhelp->tab_hbox),
	                    devhelp->tabber,
	                    TRUE, TRUE, 5);

	label = gtk_label_new (_("Contents"));
	gtk_label_set_ellipsize (GTK_LABEL (label),
	                         PANGO_ELLIPSIZE_END);
	anjuta_tabber_add_tab (ANJUTA_TABBER (devhelp->tabber),
	                       label);
	
	label = gtk_label_new (_("Search"));
	gtk_label_set_ellipsize (GTK_LABEL (label),
	                         PANGO_ELLIPSIZE_END);
	anjuta_tabber_add_tab (ANJUTA_TABBER (devhelp->tabber),
	                       label);
	
	gtk_widget_show_all (devhelp->tab_hbox);

	/*
	 * Forward/back buttons
	 */
	devhelp->custom_label = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
	gtk_widget_show (devhelp->custom_label);

	gtk_box_pack_start (GTK_BOX (devhelp->custom_label), 
	                    gtk_image_new_from_stock (ANJUTA_STOCK_DEVHELP_VIEW,
	                                              GTK_ICON_SIZE_MENU),
	                    FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (devhelp->custom_label), gtk_label_new (_("API")),
	                    FALSE, FALSE, 0);	
	
	devhelp->go_back = gtk_button_new ();
	gtk_container_add (GTK_CONTAINER (devhelp->go_back),
	                   gtk_image_new_from_stock(GTK_STOCK_GO_BACK, GTK_ICON_SIZE_MENU));
	gtk_widget_set_sensitive (devhelp->go_back, FALSE);
	g_signal_connect (devhelp->go_back, "clicked",
	                  G_CALLBACK (on_go_back_clicked), devhelp);
	
	devhelp->go_forward = gtk_button_new ();
	gtk_container_add (GTK_CONTAINER (devhelp->go_forward),
	                   gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_MENU));
	gtk_widget_set_sensitive (devhelp->go_forward, FALSE);
	g_signal_connect (devhelp->go_forward, "clicked",
	                  G_CALLBACK (on_go_forward_clicked), devhelp);

	devhelp->online = gtk_button_new_with_label ("library.gnome.org");
	g_signal_connect (devhelp->online, "clicked",
	                  G_CALLBACK (on_online_clicked), devhelp);

	gtk_box_pack_start (GTK_BOX (devhelp->custom_label), devhelp->go_back,
	                    FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (devhelp->custom_label), devhelp->go_forward,
	                    FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (devhelp->custom_label), devhelp->online,
	                    FALSE, FALSE, 0);
	gtk_widget_show_all (devhelp->custom_label);
	
	/*
	 * Notebook
	 */
	books_sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (books_sw),
									GTK_POLICY_NEVER,
									GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (books_sw),
									     GTK_SHADOW_NONE);
	gtk_container_set_border_width (GTK_CONTAINER (books_sw), 2);

	devhelp->book_tree = dh_book_tree_new (devhelp->book_manager);

	devhelp->search = dh_search_new (devhelp->book_manager);
	gtk_widget_set_size_request (devhelp->search, 0, 0);
	
	g_signal_connect (devhelp->book_tree,
					  "link-selected",
					  G_CALLBACK (devhelp_tree_link_selected_cb),
					  devhelp);
	g_signal_connect (devhelp->search,
					  "link-selected",
					  G_CALLBACK (devhelp_search_link_selected_cb),
					  devhelp);
	
	gtk_container_add (GTK_CONTAINER (books_sw), devhelp->book_tree);
	gtk_notebook_append_page (GTK_NOTEBOOK (devhelp->control_notebook), books_sw,
							 NULL);
	gtk_notebook_append_page (GTK_NOTEBOOK (devhelp->control_notebook), devhelp->search,
							  NULL);
	
	gtk_widget_show_all (devhelp->control_notebook);
	
	/* View */
	devhelp->view = webkit_web_view_new ();
	gtk_widget_show (devhelp->view);
	
	// TODO: Show some good start page
#ifdef HAVE_WEBKIT2
	webkit_web_view_load_uri (WEBKIT_WEB_VIEW (devhelp->view), "about:blank");
	g_signal_connect(G_OBJECT (devhelp->view), "load-changed",
					 G_CALLBACK (on_load_changed), devhelp);

	devhelp->present_widget = devhelp->view;
#else
	webkit_web_view_open (WEBKIT_WEB_VIEW (devhelp->view), "about:blank");
	g_signal_connect(G_OBJECT (devhelp->view), "load-finished", 
					 G_CALLBACK (on_load_finished), devhelp);

	devhelp->view_sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (devhelp->view_sw),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	gtk_container_set_border_width (GTK_CONTAINER (devhelp->view_sw), 5);
	gtk_widget_show (devhelp->view_sw);
	gtk_container_add (GTK_CONTAINER (devhelp->view_sw), devhelp->view);

	devhelp->present_widget = devhelp->view_sw;
#endif

	/* This is the window that show the html help text */
	anjuta_shell_add_widget_custom (plugin->shell, devhelp->present_widget,
							 "AnjutaDevhelpDisplay", _("API"),
							 ANJUTA_STOCK_DEVHELP_VIEW, devhelp->custom_label,
							 ANJUTA_SHELL_PLACEMENT_CENTER, NULL);

	anjuta_shell_add_widget_custom (plugin->shell, devhelp->control_notebook,
	                                "AnjutaDevhelpIndex", _("API Browser"), ANJUTA_STOCK_DEVHELP_SEARCH,
	                                devhelp->tab_hbox,
	                                ANJUTA_SHELL_PLACEMENT_LEFT, NULL);

#endif /* DISABLE_EMBEDDED_DEVHELP */

	/* Add watches */
	devhelp->editor_watch_id = 
		anjuta_plugin_add_watch (plugin,  IANJUTA_DOCUMENT_MANAGER_CURRENT_DOCUMENT,
								 value_added_current_editor,
								 value_removed_current_editor,
								 devhelp);

	return TRUE;
}

static gboolean
devhelp_deactivate (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	AnjutaDevhelp* devhelp = ANJUTA_PLUGIN_DEVHELP (plugin);

	DEBUG_PRINT ("%s", "AnjutaDevhelp: Dectivating AnjutaDevhelp plugin ...");


	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	anjuta_ui_unmerge (ui, devhelp->uiid);
	
#ifndef DISABLE_EMBEDDED_DEVHELP

	/* Remove widgets */
	anjuta_shell_remove_widget(plugin->shell, devhelp->present_widget, NULL);
	anjuta_shell_remove_widget(plugin->shell, devhelp->control_notebook, NULL);	

#endif /* DISABLE_EMBEDDED_DEVHELP */
	
	/* Remove action group */
	anjuta_ui_remove_action_group (ui, devhelp->action_group);
	
	/* Remove watch */
	anjuta_plugin_remove_watch (plugin, devhelp->editor_watch_id, TRUE);
	
	return TRUE;
}

#ifndef DISABLE_EMBEDDED_DEVHELP

void
anjuta_devhelp_check_history (AnjutaDevhelp* devhelp)
{
	gtk_widget_set_sensitive (devhelp->go_forward, webkit_web_view_can_go_forward (WEBKIT_WEB_VIEW (devhelp->view)));

	gtk_widget_set_sensitive (devhelp->go_back, webkit_web_view_can_go_back (WEBKIT_WEB_VIEW (devhelp->view)));
}

#endif /* DISABLE_EMBEDDED_DEVHELP */

static void
devhelp_finalize (GObject *obj)
{
	DEBUG_PRINT ("%s", "Finalising Devhelp plugin");

	/* Finalization codes here */
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
devhelp_dispose (GObject *obj)
{
	DEBUG_PRINT ("%s", "Disposing Devhelp plugin");
	
#ifndef DISABLE_EMBEDDED_DEVHELP
	AnjutaDevhelp* devhelp = ANJUTA_PLUGIN_DEVHELP (obj);

	g_clear_object (&devhelp->book_manager);
#endif /* DISABLE_EMBEDDED_DEVHELP */

	/* Disposition codes */
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
devhelp_instance_init (GObject *obj)
{
	AnjutaDevhelp *plugin = ANJUTA_PLUGIN_DEVHELP (obj);
	
#ifndef DISABLE_EMBEDDED_DEVHELP

	/* Initialize Devhelp support */
	dh_init ();

	/* Create devhelp */
	plugin->book_manager = dh_book_manager_new ();
	dh_book_manager_populate (plugin->book_manager);

#endif /* DISABLE_EMBEDDED_DEVHELP */
	
	plugin->uiid = 0;
}

static void
devhelp_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = devhelp_activate;
	plugin_class->deactivate = devhelp_deactivate;
	
	klass->finalize = devhelp_finalize;
	klass->dispose = devhelp_dispose;
}

#ifndef DISABLE_EMBEDDED_DEVHELP

static void
ihelp_search (IAnjutaHelp *help, const gchar *query, GError **err)
{
	AnjutaDevhelp *plugin;
	
	plugin = ANJUTA_PLUGIN_DEVHELP (help);
	
	anjuta_shell_present_widget (ANJUTA_PLUGIN (plugin)->shell,
								 plugin->control_notebook, NULL);
	
	dh_search_set_search_string (DH_SEARCH (plugin->search), query, NULL);
	gtk_notebook_set_current_page (GTK_NOTEBOOK (plugin->control_notebook), 1);
}

#else /* DISABLE_EMBEDDED_DEVHELP */

 
/*
 * Set the DISPLAY variable, to be use by g_spawn_async.
 */
static void
set_environment (gpointer display)
{
	g_setenv ("DISPLAY", display, TRUE);
}


static void
ihelp_search (IAnjutaHelp *help, const gchar *query, GError **err)
{
	gchar *cmd[4];
	gchar *display;
	gboolean retval;
	
	if (!anjuta_util_prog_is_installed ("devhelp", TRUE))
	{
		return;
	}
	
	cmd[0] = "devhelp";
	
	if (query && strlen (query) > 0)
	{
		cmd[1] = "-s";
		cmd[2] = (gchar *)query;
		cmd[3] = NULL;	
	}
	else
	{
		cmd[1] = NULL;
	}

	display = gdk_screen_make_display_name (gdk_screen_get_default ());

	retval = g_spawn_async (NULL, /* working directory */
				cmd,
				NULL, /* envp */
				G_SPAWN_SEARCH_PATH,
				set_environment,
				&display,
				NULL,
				NULL);

	g_free (display);
}

#endif /* DISABLE_EMBEDDED_DEVHELP */

static void
ihelp_iface_init(IAnjutaHelpIface *iface)
{
	iface->search = ihelp_search;
}

ANJUTA_PLUGIN_BEGIN (AnjutaDevhelp, devhelp);
ANJUTA_PLUGIN_ADD_INTERFACE (ihelp, IANJUTA_TYPE_HELP);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (AnjutaDevhelp, devhelp);
