/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * plugin.h
 * Copyright (C) Naba Kumar  <naba@gnome.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _SYMBOL_BROWSER_PLUGIN_H_
#define _SYMBOL_BROWSER_PLUGIN_H_

#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/interfaces/ianjuta-message-manager.h>

G_BEGIN_DECLS

extern GType symbol_browser_plugin_get_type (GluePlugin *plugin);
#define ANJUTA_TYPE_PLUGIN_SYMBOL_BROWSER         (symbol_browser_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_SYMBOL_BROWSER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_SYMBOL_BROWSER, SymbolBrowserPlugin))
#define ANJUTA_PLUGIN_SYMBOL_BROWSER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_SYMBOL_BROWSER, SymbolBrowserPluginClass))
#define ANJUTA_IS_PLUGIN_SYMBOL_BROWSER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_SYMBOL_BROWSER))
#define ANJUTA_IS_PLUGIN_SYMBOL_BROWSER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_SYMBOL_BROWSER))
#define ANJUTA_PLUGIN_SYMBOL_BROWSER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_SYMBOL_BROWSER, SymbolBrowserPluginClass))

typedef struct _SymbolBrowserPlugin SymbolBrowserPlugin;
typedef struct _SymbolBrowserPluginClass SymbolBrowserPluginClass;

struct _SymbolBrowserPlugin{
	AnjutaPlugin parent;
	
	AnjutaUI *ui;
	AnjutaPreferences *prefs;
	
	GtkWidget *sw;             /* symbol main window [gtk_notebook] */
	GtkWidget *sv;             /* symbol view scrolledwindow */
	GtkWidget *sv_tree;        /* anjuta_symbol_view */
	GtkWidget *sv_tab_label;
	GtkWidget *ss;             /* symbol search */
	GtkWidget *ss_tab_label;
	GtkWidget *pref_tree_view; /* Preferences treeview */
	
	GtkActionGroup *action_group;
	GtkActionGroup *popup_action_group;
	GtkActionGroup *action_group_nav;
	gint merge_id;
	gchar *project_root_uri;
	GObject *current_editor;
	guint root_watch_id;
	guint editor_watch_id;
	GHashTable *editor_connected;
	GList *gconf_notify_ids;
	
	IAnjutaMessageView* mesg_view;
};

struct _SymbolBrowserPluginClass{
	AnjutaPluginClass parent_class;
};

G_END_DECLS

#endif
