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
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-help.h>

#include "plugin.h"

#ifndef DISABLE_EMBEDDED_DEVHELP

#include <devhelp/dh-book-tree.h>
#include <devhelp/dh-html.h>
#include <devhelp/dh-preferences.h>
#include <devhelp/dh-search.h>
#include <devhelp/dh-base.h>
#include "htmlview.h"
#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-devhelp.ui"

#else /* DISABLE_EMBEDDED_DEVHELP */

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-devhelp-simple.ui"

#endif /* DISABLE_EMBEDDED_DEVHELP */

static gpointer parent_class;

#ifndef DISABLE_EMBEDDED_DEVHELP

static void
devhelp_tree_link_selected_cb (GObject       *ignored,
			      DhLink        *link,
			      AnjutaDevhelp *widget)
{
	
	DhHtml       *html;

	anjuta_shell_present_widget (ANJUTA_PLUGIN (widget)->shell,
								 widget->htmlview, NULL);

	html = html_view_get_dh_html(HTML_VIEW(widget->htmlview));
	
	if (!DH_IS_HTML(html))
		return;
	
	dh_html_open_uri (html, link->uri);

	anjuta_devhelp_check_history (widget);
}

static void
devhelp_search_link_selected_cb (GObject  *ignored,
				DhLink   *link,
				 AnjutaDevhelp *widget)
{
	DhHtml       *html;

	anjuta_shell_present_widget (ANJUTA_PLUGIN (widget)->shell,
								 widget->htmlview, NULL);

	html = html_view_get_dh_html(HTML_VIEW(widget->htmlview));

	if (!DH_IS_HTML(html))
		return;

	dh_html_open_uri (html, link->uri);

	anjuta_devhelp_check_history (widget);
}

static void
on_go_back_activate (GtkAction *action, AnjutaDevhelp *plugin)
{
	DhHtml* html;

	html = html_view_get_dh_html(HTML_VIEW(plugin->htmlview));
	
	if (!DH_IS_HTML(html))
		return;
		
	dh_html_go_back(html);
}

static void
on_go_forward_activate (GtkAction *action, AnjutaDevhelp *plugin)
{
	DhHtml* html;

	html = html_view_get_dh_html(HTML_VIEW(plugin->htmlview));
	
	if (!DH_IS_HTML(html))
		return;
	
	dh_html_go_forward(html);
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
context_idle(AnjutaDevhelp* plugin)
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
		GTK_STOCK_HELP,
		N_("_Context Help"),
		"<shift>F1",
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
							const GValue *value, gpointer data)
{
	GtkAction *action;
	GObject *editor;
	AnjutaDevhelp* devhelp = ANJUTA_PLUGIN_DEVHELP (data);

	editor = g_value_get_object (value);
	devhelp->editor = IANJUTA_EDITOR(editor);
	action = gtk_action_group_get_action (devhelp->action_group,
										  "ActionHelpContext");
	g_object_set (action, "sensitive", TRUE, NULL);
}

static void
value_removed_current_editor (AnjutaPlugin *plugin,
							  const char *name, gpointer data)
{
	GtkAction *action;
	AnjutaDevhelp* devhelp = ANJUTA_PLUGIN_DEVHELP (data);	
		
	devhelp->editor = NULL;
	action = gtk_action_group_get_action (devhelp->action_group,
										  "ActionHelpContext");
	g_object_set (action, "sensitive", FALSE, NULL);
}

static gboolean
devhelp_activate (AnjutaPlugin *plugin)
{

	AnjutaUI *ui;
	AnjutaDevhelp *devhelp;
	GNode *books;
	GList *keywords;
	
	GtkWidget* books_sw;
	
	DEBUG_PRINT ("AnjutaDevhelp: Activating AnjutaDevhelp plugin ...");
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

	books = dh_base_get_book_tree (devhelp->base);
	keywords = dh_base_get_keywords (devhelp->base);
	
	books_sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (books_sw),
					GTK_POLICY_NEVER,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (books_sw),
					     GTK_SHADOW_IN);
	gtk_container_set_border_width (GTK_CONTAINER (books_sw), 2);
	
	devhelp->control_notebook = gtk_notebook_new();
	devhelp->book_tree = dh_book_tree_new(books);
	devhelp->search = dh_search_new(keywords);
	
	g_signal_connect (devhelp->book_tree,
			  "link-selected",
			  G_CALLBACK (devhelp_tree_link_selected_cb),
			  devhelp);
	g_signal_connect (devhelp->search,
			  "link-selected",
			  G_CALLBACK (devhelp_search_link_selected_cb),
			  devhelp);
	
	gtk_container_add(GTK_CONTAINER(books_sw), devhelp->book_tree);
	gtk_notebook_append_page(GTK_NOTEBOOK(devhelp->control_notebook), books_sw,
		gtk_label_new(_("Books")));
	gtk_notebook_append_page(GTK_NOTEBOOK(devhelp->control_notebook), devhelp->search,
		gtk_label_new(_("Search")));
	
	devhelp->htmlview = html_view_new(devhelp);	

	anjuta_shell_add_widget (plugin->shell, devhelp->control_notebook,
								 "AnjutaDevhelpIndex", _("Help"), GTK_STOCK_HELP,
								 ANJUTA_SHELL_PLACEMENT_LEFT, NULL);
	anjuta_shell_add_widget (plugin->shell, devhelp->htmlview,
								 "AnjutaDevhelpDisplay", _("Help display"),
								 GTK_STOCK_HELP,
								 ANJUTA_SHELL_PLACEMENT_CENTER, NULL);
								 
#endif /* DISABLE_EMBEDDED_DEVHELP */

	/* Add watches */
	devhelp->editor_watch_id = 
		anjuta_plugin_add_watch (plugin, "document_manager_current_editor",
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

	DEBUG_PRINT ("AnjutaDevhelp: Dectivating AnjutaDevhelp plugin ...");


	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	anjuta_ui_unmerge (ui, devhelp->uiid);
	
#ifndef DISABLE_EMBEDDED_DEVHELP

	/* Remove widgets */
	anjuta_shell_remove_widget(plugin->shell, devhelp->htmlview, NULL);
	anjuta_shell_remove_widget(plugin->shell, devhelp->control_notebook, NULL);	

#endif /* DISABLE_EMBEDDED_DEVHELP */
	
	/* Remove action group */
	anjuta_ui_remove_action_group (ui, devhelp->action_group);
	
	/* Remove watch */
	anjuta_plugin_remove_watch (plugin, devhelp->editor_watch_id, TRUE);
	
	return TRUE;
}

#ifndef DISABLE_EMBEDDED_DEVHELP

void anjuta_devhelp_check_history(AnjutaDevhelp* devhelp)
{
	GtkAction* action_forward;
	GtkAction* action_back;
	DhHtml* html = html_view_get_dh_html(HTML_VIEW(devhelp->htmlview));
	
	action_forward = gtk_action_group_get_action (devhelp->action_group,
										  "ActionDevhelpForward");
	action_back = gtk_action_group_get_action (devhelp->action_group,
										  "ActionDevhelpBack");
	if (html != NULL)
	{
		g_object_set (action_forward, "sensitive", dh_html_can_go_forward (html) , NULL);
		g_object_set (action_back, "sensitive", dh_html_can_go_back (html) , NULL);
	}
}

#endif /* DISABLE_EMBEDDED_DEVHELP */

#if 0
static void
devhelp_finalize (GObject *obj)
{
	/* Finalization codes here */
	AnjutaDevhelp *plugin = ANJUTA_PLUGIN_DEVHELP (obj);
	
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

static void
devhelp_dispose (GObject *obj)
{
	AnjutaDevhelp* devhelp = ANJUTA_PLUGIN_DEVHELP (obj);
	
	/* Destroy devhelp - seems not to work... */
	// g_object_unref(G_OBJECT(devhelp->base));

	/* Disposition codes */
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}
#endif

static void
devhelp_instance_init (GObject *obj)
{
	AnjutaDevhelp *plugin = ANJUTA_PLUGIN_DEVHELP (obj);
	
#ifndef DISABLE_EMBEDDED_DEVHELP

	/* Create devhelp */
	plugin->base = dh_base_new ();

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
#if 0
	klass->finalize = devhelp_finalize;
	klass->dispose = devhelp_dispose;
#endif
}

#ifndef DISABLE_EMBEDDED_DEVHELP

static void
ihelp_search (IAnjutaHelp *help, const gchar *query, GError **err)
{
	AnjutaDevhelp *plugin;
	
	plugin = ANJUTA_PLUGIN_DEVHELP (help);
	
	anjuta_shell_present_widget (ANJUTA_PLUGIN (plugin)->shell,
								 plugin->control_notebook, NULL);
	
	dh_search_set_search_string (DH_SEARCH (plugin->search), query);
	gtk_notebook_set_current_page (GTK_NOTEBOOK (plugin->control_notebook), 1);
}

#else /* DISABLE_EMBEDDED_DEVHELP */

static void
ihelp_search (IAnjutaHelp *help, const gchar *query, GError **err)
{
	AnjutaDevhelp *plugin;
	
	plugin = ANJUTA_PLUGIN_DEVHELP (help);
	
	if (!anjuta_util_prog_is_installed ("devhelp", TRUE))
	{
		return;
	}
	
	if(query && strlen (query) > 0)
	{
		fprintf(stderr, "Word is %s\n", query);
		if(fork()==0)
		{
			execlp("devhelp", "devhelp", "-s", query, NULL);
			g_warning (_("Cannot execute command: \"%s\""), "devhelp");
			_exit(1);
		}
	}
	else
	{
		if(fork()==0)
		{
			execlp("devhelp", "devhelp", NULL);
			g_warning (_("Cannot execute command: \"%s\""), "devhelp");
			_exit(1);
		}
	}
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
