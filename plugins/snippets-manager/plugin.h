/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.h
    Copyright (C) Dragos Dena 2010

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, 
	Boston, MA  02110-1301  USA
*/

#ifndef __SNIPPETS_MANAGER_PLUGIN_H__
#define __SNIPPETS_MANAGER_PLUGIN_H__

#include <config.h>
#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/interfaces/ianjuta-snippets-manager.h>
#include <libanjuta/interfaces/ianjuta-editor.h>

#include "snippets-editor.h"
#include "snippets-browser.h"
#include "snippets-db.h"
#include "snippets-interaction-interpreter.h"
#include "snippets-provider.h"


extern GType snippets_manager_plugin_get_type (GTypeModule *module);
#define ANJUTA_TYPE_PLUGIN_SNIPPETS_MANAGER         (snippets_manager_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_SNIPPETS_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_SNIPPETS_MANAGER, SnippetsManagerPlugin))
#define ANJUTA_PLUGIN_SNIPPETS_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_SNIPPETS_MANAGER, SnippetsManagerPluginClass))
#define ANJUTA_IS_PLUGIN_SNIPPETS_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_SNIPPETS_MANAGER))
#define ANJUTA_IS_PLUGIN_SNIPPETS_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_SNIPPETS_MANAGER))
#define ANJUTA_PLUGIN_SNIPPETS_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_SNIPPETS_MANAGER, SnippetsManagerPluginClass))


typedef struct _SnippetsManagerPlugin SnippetsManagerPlugin;
typedef struct _SnippetsManagerPluginClass SnippetsManagerPluginClass;

struct _SnippetsManagerPlugin
{
	AnjutaPlugin parent;

	/* Snippet Database. This is where snippets are loaded into memory.
	   Provides search functions. */
	SnippetsDB* snippets_db;
	
	/* Snippets Interaction Interpreter. This takes care of interacting with
	   the editor for inserting and live editing the snippets.  */
	SnippetsInteraction* snippets_interaction;
		
	/* GUI parts. */
	SnippetsBrowser* snippets_browser;

	SnippetsProvider *snippets_provider;
	
	/* Plug-in settings */
	gboolean overwrite_on_conflict;
	gboolean show_only_document_language_snippets;

	gint cur_editor_watch_id;

	/* The Menu UI */
	GtkActionGroup *action_group;
	gint uiid;

	gboolean browser_maximized;

};


struct _SnippetsManagerPluginClass
{
	AnjutaPluginClass parent_class;
};


/* To insert a snippet to the editor. */
gboolean
snippet_insert (SnippetsManagerPlugin * plugin, const gchar *keyword);


#endif /* __SNIPPETS_MANAGER_PLUGIN_H__ */
