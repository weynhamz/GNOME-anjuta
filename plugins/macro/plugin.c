/* 
 * 	plugin.c (c) 2004 Johannes Schmid
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "plugin.h"
#include "macro-actions.h"
#include "macro-db.h"

#include <libanjuta/interfaces/ianjuta-macro.h>

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-macro.ui"
#define ICON_FILE "anjuta-macro.png"

static gpointer parent_class;

static GtkActionEntry actions_macro[] = {
	{
	"ActionMenuEditMacro",
	 NULL,
	 N_("Macros"),
	 NULL,
	 NULL,
	 NULL},
	 {
	 "ActionEditMacroInsert",
	 NULL,
	 N_("_Insert Macro ..."),
	 "<control>i",
	 N_("Insert a macro using a shortcut"),
	 G_CALLBACK (on_menu_insert_macro)},
	 {
	 "ActionEditMacroAdd",
	 NULL,
	 N_("_Add Macro ..."),
	 "<control>m",
	 N_("Add a macro"),
	 G_CALLBACK (on_menu_add_macro)},
	 {
	 "ActionEditMacroManager",
	 NULL,
	 N_("Macros ..."),
	 NULL,
	 N_("Add/Edit/Remove macros"),
	 G_CALLBACK (on_menu_manage_macro)}
};

static void
value_added_current_editor (AnjutaPlugin * plugin, const char *name,
			    const GValue * value, gpointer data)
{
	GObject *editor;
	GtkAction* macro_insert_action;
	AnjutaUI* ui = anjuta_shell_get_ui (plugin->shell, NULL);
	editor = g_value_get_object (value);

	MacroPlugin *macro_plugin = (MacroPlugin *) plugin;
 	macro_insert_action = 
		anjuta_ui_get_action (ui, "ActionGroupMacro", "ActionEditMacroInsert");

	if (editor != NULL)
	{
		g_object_set (G_OBJECT (macro_insert_action), "sensitive", TRUE, NULL);
		macro_plugin->current_editor = editor;
	}
	else
	{
		g_object_set (G_OBJECT (macro_insert_action), "sensitive", FALSE, NULL);
		macro_plugin->current_editor = NULL;
	}

}

static void
value_removed_current_editor (AnjutaPlugin * plugin,
			      const char *name, gpointer data)
{
	MacroPlugin *macro_plugin = (MacroPlugin *) plugin;

	macro_plugin->current_editor = NULL;

}

static gboolean
activate_plugin (AnjutaPlugin * plugin)
{
	//AnjutaPreferences *prefs;
	AnjutaUI *ui;
	MacroPlugin *macro_plugin;

#ifdef DEBUG
	g_message ("MacroPlugin: Activating Macro plugin ...");
#endif

	macro_plugin = (MacroPlugin *) plugin;
	ui = anjuta_shell_get_ui (plugin->shell, NULL);

	/* Add all our actions */
	anjuta_ui_add_action_group_entries (ui, "ActionGroupMacro",
					    _("Macro operations"),
					    actions_macro,
					    G_N_ELEMENTS (actions_macro),
					    plugin);
	macro_plugin->uiid = anjuta_ui_merge (ui, UI_FILE);

	macro_plugin->editor_watch_id =
		anjuta_plugin_add_watch (plugin,
					 "document_manager_current_editor",
					 value_added_current_editor,
					 value_removed_current_editor, NULL);
					 
	macro_plugin->macro_db = macro_db_new();
	
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin * plugin)
{
	AnjutaUI *ui = anjuta_shell_get_ui (plugin->shell, NULL);
#ifdef DEBUG
	g_message ("MacroPlugin: Deactivating Macro plugin ...");
#endif
	anjuta_ui_unmerge (ui, ((MacroPlugin *) plugin)->uiid);
	return TRUE;
}

static void
finalize (GObject * obj)
{
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (G_OBJECT (obj)));
}

static void
dispose (GObject * obj)
{
	MacroPlugin *plugin = (MacroPlugin *) obj;
	if (plugin->macro_dialog != NULL)
		g_object_unref (plugin->macro_dialog);
	g_object_unref(plugin->macro_db);
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (G_OBJECT (obj)));
}

static void
macro_plugin_instance_init (GObject * obj)
{
	MacroPlugin *plugin = (MacroPlugin *) obj;
	plugin->uiid = 0;
	plugin->current_editor = NULL;
}

static void
macro_plugin_class_init (GObjectClass * klass)
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
	klass->finalize = finalize;
}

static gboolean
match_keyword (MacroPlugin * plugin, GtkTreeIter * iter, const gchar *keyword)
{
	gchar *name;
	gint offset = 0;
	gint pos;
	
	gtk_tree_model_get(macro_db_get_model(plugin->macro_db), iter,
		MACRO_NAME, &name, -1);
	if ( name && strcmp(keyword, name) == 0)
	{
		const int CURRENT_POS = -1;
		gchar* text = macro_db_get_macro(plugin, plugin->macro_db, iter, &offset);
		if (plugin->current_editor != NULL && text != NULL)
		{
			pos = ianjuta_editor_get_position (IANJUTA_EDITOR(plugin->current_editor),
			                                   NULL);
			ianjuta_editor_insert (IANJUTA_EDITOR (plugin->current_editor),
					       CURRENT_POS, text, -1, NULL);
			ianjuta_editor_goto_position (IANJUTA_EDITOR(plugin->current_editor), 
			                              pos + offset, 
			                              NULL);
			g_free(text);
		}
		return TRUE;
	}
	return FALSE;
}

/* keyword : macro name  */

gboolean
macro_insert (MacroPlugin * plugin, const gchar *keyword)
{
	GtkTreeIter parent;
	GtkTreeIter cur_cat;
	GtkTreeModel *model = macro_db_get_model (plugin->macro_db);
	
	gtk_tree_model_get_iter_first (model, &parent);
	do
	{
		if (gtk_tree_model_iter_children (model, &cur_cat, &parent))
		{
		do
		{
			GtkTreeIter cur_macro;
			if (gtk_tree_model_iter_children
			    (model, &cur_macro, &cur_cat))
			{
				do
				{
					gboolean predefined;
					gtk_tree_model_get (model, &cur_macro,
						    MACRO_PREDEFINED,
						    &predefined, -1);
					if (predefined)
					{
						if (match_keyword (plugin, &cur_macro, keyword))
						{
							return TRUE;
						}
					}
				}
				while (gtk_tree_model_iter_next
				       (model, &cur_macro));
			}
			else
			{
				gboolean is_category;
				gtk_tree_model_get (model, &cur_cat,
						    MACRO_IS_CATEGORY,
						    &is_category, -1);
				if (is_category)
					continue;
				
				if (match_keyword (plugin, &cur_cat, keyword))
				{
					return TRUE;
				}
			}
		}
		while (gtk_tree_model_iter_next (model, &cur_cat));
		}
	}
	while (gtk_tree_model_iter_next (model, &parent));
	return TRUE;
}

static void 
ianjuta_macro_iface_insert(IAnjutaMacro* macro, const gchar* key, GError** err)
{
	MacroPlugin* plugin = (MacroPlugin*)macro;
	macro_insert(plugin, key);
}

/* Interface */
static void
ianjuta_macro_iface_init (IAnjutaMacroIface *iface)
{
	iface->insert = ianjuta_macro_iface_insert;
}

ANJUTA_PLUGIN_BEGIN (MacroPlugin, macro_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE (ianjuta_macro, IANJUTA_TYPE_MACRO);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (MacroPlugin, macro_plugin);
