/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/* Anjuta
 * Copyright 2000 Dave Camp
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.  
 */

#include <config.h>
#include "anjuta-plugin.h"

#include <string.h>

/*
#include <bonobo/bonobo-i18n.h>
#include <bonobo/bonobo-ui-util.h>
#include <bonobo/bonobo-window.h>
#include <libgnome/gnome-macros.h>
*/

#include <libanjuta/anjuta-marshal.h>

typedef struct 
{
	guint id;
	char *name;
	AnjutaPluginValueAdded added;
	AnjutaPluginValueRemoved removed;
	gboolean need_remove;
	gpointer user_data;
} Watch;

struct _AnjutaPluginPrivate {
	guint watch_num;

	int added_signal_id;
	int removed_signal_id;

	GList *watches;
};

enum {
	PROP_0,
	PROP_SHELL,
	PROP_UI,
	PROP_PREFS
};

static void anjuta_plugin_finalize (GObject         *object);
static void anjuta_plugin_class_init (AnjutaPluginClass     *class);

GNOME_CLASS_BOILERPLATE (AnjutaPlugin, anjuta_plugin, GObject, G_TYPE_OBJECT);

static void
destroy_watch (Watch *watch)
{
	g_free (watch->name);
	g_free (watch);
}

static void
anjuta_plugin_dispose (GObject *object)
{
	AnjutaPlugin *tool = ANJUTA_PLUGIN (object);
	
	if (tool->priv->watches) {
		GList *l;

		for (l = tool->priv->watches; l != NULL; l = l->next) {
			Watch *watch = (Watch *)l->data;

			if (watch->removed && watch->need_remove) {
				watch->removed (tool, 
						watch->name, 
						watch->user_data);
			}
			
			destroy_watch (watch);
		}
		g_list_free (tool->priv->watches);
		tool->priv->watches = NULL;
	}

	if (tool->shell) {
		g_object_unref (tool->shell);
		tool->shell = NULL;
	}
}

static void
anjuta_plugin_finalize (GObject *object) 
{
	AnjutaPlugin *tool = ANJUTA_PLUGIN (object);
/*	
	if (tool->uic) {
		g_warning ("UI not unmerged\n");
	}
*/
	g_free (tool->priv);
}

static void
anjuta_plugin_get_property (GObject *object,
			  guint param_id,
			  GValue *value,
			  GParamSpec *pspec)
{
	AnjutaPlugin *tool = ANJUTA_PLUGIN (object);
	
	switch (param_id) {
	case PROP_SHELL:
		g_value_set_object (value, tool->shell);
		break;
	case PROP_UI:
	  g_value_set_object (value, tool->ui);
	  break;
	case PROP_PREFS:
	  g_value_set_object (value, tool->prefs);
	  break;
	default :
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
anjuta_plugin_set_property (GObject *object,
			  guint param_id,
			  const GValue *value,
			  GParamSpec *pspec)
{
	AnjutaPlugin *tool = ANJUTA_PLUGIN (object);
	
	switch (param_id) {
	case PROP_SHELL:
		g_return_if_fail (tool->shell == NULL);
		tool->shell = g_value_get_object (value);
		g_object_ref (tool->shell);
		
		ANJUTA_PLUGIN_GET_CLASS (object)->shell_set (tool);

		g_object_notify (object, "shell");
		break;
	case PROP_UI:
	  g_return_if_fail (tool->ui == NULL);
	  tool->ui = g_value_get_object (value);
	  g_object_ref (tool->ui);

	  ANJUTA_PLUGIN_GET_CLASS (object)->ui_set (tool);
	  
	  g_object_notify (object, "ui");
	  break;
	case PROP_PREFS:
	  g_return_if_fail (tool->prefs == NULL);
	  tool->prefs = g_value_get_object (value);
	  g_object_ref (tool->prefs);

	  ANJUTA_PLUGIN_GET_CLASS (object)->prefs_set (tool);

	  g_object_notify (object, "prefs");
	  break;
	default :
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}
        
static void
anjuta_plugin_class_init (AnjutaPluginClass *class) 
{
	GObjectClass *object_class = (GObjectClass*) class;
	parent_class = g_type_class_peek_parent (class);
    
	object_class->dispose = anjuta_plugin_dispose;
	object_class->finalize = anjuta_plugin_finalize;
	object_class->get_property = anjuta_plugin_get_property;
	object_class->set_property = anjuta_plugin_set_property;

	class->shutdown = NULL;

	g_object_class_install_property
		(object_class,
		 PROP_SHELL,
		 g_param_spec_object ("shell",
				      _("Anjuta Shell"),
				      _("Anjuta shell that will contain the tool"),
				      ANJUTA_TYPE_SHELL,
				      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property
		(object_class,
		 PROP_UI,
		 g_param_spec_object ("ui",
				      _("Anjuta UI"),
				      _("Anjuta UI that will contain Menus and toolbars"),
				      ANJUTA_TYPE_UI,
				      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property
		(object_class,
		 PROP_PREFS,
		 g_param_spec_object ("prefs",
				      _("Anjuta Preferences"),
				      _("Anjuta preferences that will contain preferences"),
				      ANJUTA_TYPE_PREFERENCES,
				      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
anjuta_plugin_instance_init (AnjutaPlugin *tool)
{
	tool->priv = g_new0 (AnjutaPluginPrivate, 1);
}

#if 0
void
anjuta_plugin_merge_ui (AnjutaPlugin *tool, 
		      const char *name, 
		      const char *datadir, 
		      const char *xmlfile, 
		      BonoboUIVerb *verbs,
		      gpointer user_data)
{
	BonoboWindow *window;
	BonoboUIContainer *container;

	if (tool->uic) {
		anjuta_plugin_unmerge_ui (tool);
	}
	
	g_return_if_fail (tool->shell != NULL);
	
	window = BONOBO_WINDOW (tool->shell);
	container = bonobo_window_get_ui_container (window);
	
	tool->uic = bonobo_ui_component_new (name);
	bonobo_ui_component_set_container (tool->uic, 
					   BONOBO_OBJREF (container), 
					   NULL);
	bonobo_ui_util_set_ui (tool->uic, datadir, xmlfile, name, NULL);
	bonobo_ui_component_add_verb_list_with_data (tool->uic, 
						     verbs,
						     user_data);
}

void
anjuta_plugin_unmerge_ui (AnjutaPlugin *tool)
{
	if (tool->uic) {
		bonobo_ui_component_unset_container (tool->uic, NULL);
		bonobo_object_unref (tool->uic);
		tool->uic = NULL;
	}
}

#endif

static void
value_added_cb (AnjutaShell *shell,
		const char *name,
		const GValue *value,
		gpointer user_data)
{
	AnjutaPlugin *tool = ANJUTA_PLUGIN (user_data);
	GList *l;
	
	for (l = tool->priv->watches; l != NULL; l = l->next) {
		Watch *watch = (Watch *)l->data;
		if (!strcmp (watch->name, name)) {
			if (watch->added) {
				watch->added (tool, 
					      name, 
					      value, 
					      watch->user_data);
			}
			
			watch->need_remove = TRUE;
		}
	}
}

static void
value_removed_cb (AnjutaShell *shell,
		  const char *name,
		  gpointer user_data)
{
	AnjutaPlugin *tool = ANJUTA_PLUGIN (user_data);
	GList *l;

	for (l = tool->priv->watches; l != NULL; l = l->next) {
		Watch *watch = (Watch *)l->data;
		if (!strcmp (watch->name, name)) {
			if (watch->removed) {
				watch->removed (tool, name, watch->user_data);
			}
			if (!watch->need_remove) {
				g_warning ("watch->need_remove FALSE when it should be TRUE");
			}
			
			watch->need_remove = FALSE;
		}
	}
}

guint
anjuta_plugin_add_watch (AnjutaPlugin *tool, 
		       const char *name,
		       AnjutaPluginValueAdded added,
		       AnjutaPluginValueRemoved removed,
		       gpointer user_data)
{
	Watch *watch;
	GValue value = {0, };
	GError *error = NULL;

	g_return_val_if_fail (tool != NULL, -1);
	g_return_val_if_fail (ANJUTA_IS_PLUGIN (tool), -1);
	g_return_val_if_fail (name != NULL, -1);

	watch = g_new0 (Watch, 1);
	
	watch->id = ++tool->priv->watch_num;
	watch->name = g_strdup (name);
	watch->added = added;
	watch->removed = removed;
	watch->need_remove = FALSE;
	watch->user_data = user_data;

	tool->priv->watches = g_list_prepend (tool->priv->watches,
					      watch);

	anjuta_shell_get_value (tool->shell, name, &value, &error);
	if (!error) {
		if (added) {
			watch->added (tool, name, &value, user_data);
		}
		
		watch->need_remove = TRUE;
	}

	if (!tool->priv->added_signal_id) {
		tool->priv->added_signal_id = 
			g_signal_connect (tool->shell,
					  "value_added",
					  G_CALLBACK (value_added_cb),
					  tool);

		tool->priv->added_signal_id = 
			g_signal_connect (tool->shell,
					  "value_removed",
					  G_CALLBACK (value_removed_cb),
					  tool);
	}

	return watch->id;
}

void
anjuta_plugin_remove_watch (AnjutaPlugin *tool, guint id, gboolean send_remove)
{
	GList *l;
	Watch *watch = NULL;
	
	g_return_if_fail (tool != NULL);
	g_return_if_fail (ANJUTA_IS_PLUGIN (tool));

	for (l = tool->priv->watches; l != NULL; l = l->next) {
		watch = l->data;
		
		if (watch->id == id) {
			break;
		}
	}

	if (!watch) {
		g_warning ("Attempted to remove non-existant watch %d\n", id);
		return;
	}

	if (send_remove && watch->need_remove && watch->removed) {
		watch->removed (tool, watch->name, watch->user_data);
	}
	
	g_list_remove (tool->priv->watches, watch);
	destroy_watch (watch);
}
