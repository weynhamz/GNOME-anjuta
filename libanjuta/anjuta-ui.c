/*
    anjuta-ui.c
    Copyright (C) 2003  Naba Kumar  <naba@gnome.org>

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

#include <stdio.h>
#include <string.h>
#include <libgnome/libgnome.h>
#include <libegg/menu/egg-toggle-action.h>
#include <libegg/treeviewutils/eggcellrendererkeys.h>

#include "pixmaps.h"
#include "resources.h"
#include "anjuta-ui.h"

struct _AnjutaUIPrivate {
	EggMenuMerge *merge;
	GtkIconFactory *icon_factory;
	GtkTreeModel *model;
	GHashTable *actions_hash;
};

enum {
	COLUMN_PIXBUF,
	COLUMN_ACTION,
	COLUMN_VISIBLE,
	COLUMN_SENSITIVE,
	COLUMN_DATA,
	COLUMN_GROUP,
	N_COLUMNS
};

#if 0
static gboolean
on_delete_event (AnjutaUI *ui, gpointer data)
{
	gtk_widget_hide (GTK_WIDGET (ui));
	return FALSE;
}
#endif

static void
sensitivity_toggled (GtkCellRendererToggle *cell,
					 const gchar *path_str, GtkTreeModel *model)
{
	GtkTreePath *path;
	GtkTreeIter iter;
	EggAction *action;
	gboolean sensitive;
	
	path = gtk_tree_path_new_from_string (path_str);
	gtk_tree_model_get_iter (model, &iter, path);
	
	gtk_tree_model_get (model, &iter,
						COLUMN_SENSITIVE, &sensitive,
						COLUMN_DATA, &action, -1);
	g_object_set (G_OBJECT (action), "sensitive", !sensitive, NULL);
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
						COLUMN_SENSITIVE, !sensitive, -1);
	gtk_tree_path_free (path);
}

static void
visibility_toggled (GtkCellRendererToggle *cell,
					const gchar *path_str, GtkTreeModel *model)
{
	GtkTreePath *path;
	GtkTreeIter iter;
	EggAction *action;
	gboolean visible;
	
	path = gtk_tree_path_new_from_string (path_str);
	gtk_tree_model_get_iter (model, &iter, path);
	
	gtk_tree_model_get (model, &iter,
						COLUMN_VISIBLE, &visible,
						COLUMN_DATA, &action, -1);
	g_object_set (G_OBJECT (action), "visible", !visible, NULL);
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
						COLUMN_VISIBLE, !visible, -1);
	gtk_tree_path_free (path);
}

static gchar*
get_action_label (EggAction *action)
{
	gchar *action_label;
	if (action->label && strlen (action->label))
	{
		gchar *s, *d;
		action_label = g_strdup (action->label);
		
		s = d = action_label;
		while (*s)
		{
			/* FIXME: May break with multibyte chars */
			if (*s == '_')
				s++;
			*d = *s; d++; s++;
		}
		*d = '\0';
	}
	else
		action_label = g_strdup (action->name);
	return action_label;
}

static gchar*
get_action_accel_path (EggAction *action, const gchar *group_name)
{
	gchar *accel_path;
	accel_path = g_strconcat ("<Actions>/", group_name,
							  "/", action->name, NULL);
	return accel_path;
}

static gchar*
get_action_accel (EggAction *action, const gchar *group_name)
{
	gchar *accel_path;
	gchar *accel_name;
	GtkAccelKey key;
	
	accel_path = get_action_accel_path (action, group_name);
	if ( gtk_accel_map_lookup_entry (accel_path, &key))
		accel_name = gtk_accelerator_name (key.accel_key, key.accel_mods);
	else
		accel_name = strdup ("");
	g_free (accel_path);
	return accel_name;
}

static void
accel_edited_callback (GtkCellRendererText *cell,
                       const char          *path_string,
                       guint                keyval,
                       GdkModifierType      mask,
                       guint                hardware_keycode,
                       gpointer             data)
{
	GtkTreeModel *model = (GtkTreeModel *)data;
	GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
	GtkTreeIter iter;
	EggAction *action;
	GError *err;
	gchar *str, *accel_path, *action_group;
	
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter,
						COLUMN_DATA, &action,
						COLUMN_GROUP, &action_group, -1);
	
	/* sanity check */
	if (action == NULL || action_group == NULL)
		return;
	
	accel_path = get_action_accel_path (action, action_group);
	if (accel_path) {
		gtk_accel_map_change_entry (accel_path, keyval, mask, TRUE);
		g_free (accel_path);
	}
	
	gtk_tree_path_free (path);
}

static gint
iter_compare_func (GtkTreeModel *model, GtkTreeIter *a,
				   GtkTreeIter *b, gpointer user_data)
{
	const gchar *text_a;
	const gchar *text_b;
	gint retval = 0;
	
	gtk_tree_model_get (model, a, COLUMN_ACTION, &text_a, -1);
	gtk_tree_model_get (model, b, COLUMN_ACTION, &text_b, -1);
	if (text_a == NULL && text_b == NULL) retval = 0;
	else if (text_a == NULL) retval = -1;
	else if (text_b == NULL) retval = 1;
	else retval =  strcasecmp (text_a, text_b);
	return retval;
}

static gboolean
binding_from_string (const char      *str,
                     guint           *accelerator_key,
                     GdkModifierType *accelerator_mods)
{
	EggVirtualModifierType virtual;
	
	g_return_val_if_fail (accelerator_key != NULL, FALSE);
	
	if (str == NULL || (str && strcmp (str, "disabled") == 0))
	{
		*accelerator_key = 0;
		*accelerator_mods = 0;
		return TRUE;
	}
	
	if (!egg_accelerator_parse_virtual (str, accelerator_key, &virtual))
		return FALSE;
	
	egg_keymap_resolve_virtual_modifiers (gdk_keymap_get_default (),
										  virtual,
										  accelerator_mods);
	
	/* Be sure the GTK accelerator system will be able to handle this
	* accelerator. Be sure to allow no-accelerator accels like F1.
	*/
	if ((*accelerator_mods & gtk_accelerator_get_default_mod_mask ()) == 0 &&
		*accelerator_mods != 0)
		return FALSE;
	
	if (*accelerator_key == 0)
		return FALSE;
	else
		return TRUE;
}

static void
accel_set_func (GtkTreeViewColumn *tree_column,
                GtkCellRenderer   *cell,
                GtkTreeModel      *model,
                GtkTreeIter       *iter,
                gpointer           data)
{
	EggAction *action;
	gchar *accel_name;
	gchar *group_name;
	guint keyval;
	GdkModifierType keymods;
	
	gtk_tree_model_get (model, iter,
						COLUMN_DATA, &action,
						COLUMN_GROUP, &group_name, -1);
	if (action == NULL)
		g_object_set (G_OBJECT (cell), "visible", FALSE, NULL);
	else
	{
		accel_name = get_action_accel (action, group_name);
		if (binding_from_string (accel_name, &keyval, &keymods))
		{
			g_object_set (G_OBJECT (cell), "visible", TRUE,
						  "accel_key", keyval,
						  "accel_mask", keymods, NULL);
		}
		else
			g_object_set (G_OBJECT (cell), "visible", TRUE,
						  "accel_key", 0,
						  "accel_mask", 0, NULL);
		g_free (accel_name);	
	}
}

static GtkWidget *
create_tree_view (AnjutaUI *ui)
{
	GtkWidget *tree_view, *sw;
	GtkTreeStore *store;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	
	store = gtk_tree_store_new (N_COLUMNS,
								GDK_TYPE_PIXBUF,
								G_TYPE_STRING,
								G_TYPE_BOOLEAN,
								G_TYPE_BOOLEAN,
								G_TYPE_POINTER,
								G_TYPE_STRING);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE(store), COLUMN_ACTION,
									 iter_compare_func, NULL, NULL);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE(store),
										  COLUMN_ACTION, GTK_SORT_ASCENDING);
	
	tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (tree_view), TRUE);
	
	/* Columns */
	column = gtk_tree_view_column_new ();
	// gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Action"));

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_add_attribute (column, renderer, "pixbuf",
										COLUMN_PIXBUF);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
										COLUMN_ACTION);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);
	gtk_tree_view_set_expander_column (GTK_TREE_VIEW (tree_view), column);
	gtk_tree_view_column_set_sort_column_id (column, 0);
	
	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (G_OBJECT (renderer), "toggled",
					  G_CALLBACK (visibility_toggled), store);
	column = gtk_tree_view_column_new_with_attributes (_("Visible"),
													   renderer,
													   "active",
													   COLUMN_VISIBLE,
													   NULL);
	// gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);
	
	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (G_OBJECT (renderer), "toggled",
					  G_CALLBACK (sensitivity_toggled), store);
	column = gtk_tree_view_column_new_with_attributes (_("Sensitive"),
													   renderer,
													   "active",
													   COLUMN_SENSITIVE,
													   NULL);
	// gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);
	
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Shortcut"));
	// gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);	
	renderer = g_object_new (EGG_TYPE_CELL_RENDERER_KEYS,
							"editable", TRUE,
							"accel_mode", EGG_CELL_RENDERER_KEYS_MODE_GTK,
							NULL);
	g_signal_connect (G_OBJECT (renderer), "keys_edited",
					  G_CALLBACK (accel_edited_callback),
					  store);
	g_object_set (G_OBJECT (renderer), "editable", TRUE, NULL);
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column, renderer, accel_set_func, NULL, NULL);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_DATA);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);
	
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (sw), tree_view);
	
	// unreferenced in destroy() method.
	ui->priv->model = GTK_TREE_MODEL (store);
	
	return sw;
}

static void anjuta_ui_class_init (AnjutaUIClass *class);
static void anjuta_ui_instance_init (AnjutaUI *ui);

GNOME_CLASS_BOILERPLATE (AnjutaUI, 
						 anjuta_ui,
						 GtkDialog, GTK_TYPE_DIALOG);

static void
anjuta_ui_dispose (GObject *obj)
{
	AnjutaUI *ui = ANJUTA_UI (obj);

	if (ui->priv->model) {
		g_object_unref (G_OBJECT (ui->priv->model));
		ui->priv->model = NULL;
	}
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
anjuta_ui_finalize (GObject *obj)
{
	AnjutaUI *ui = ANJUTA_UI (obj);	
	g_free (ui->priv);
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

static void
anjuta_ui_close (GtkDialog *dialog)
{
	gtk_widget_hide (GTK_WIDGET (dialog));
}

static void
anjuta_ui_class_init (AnjutaUIClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (class);

	parent_class = g_type_class_peek_parent (class);
	
	object_class->dispose = anjuta_ui_dispose;
	object_class->finalize = anjuta_ui_finalize;

	// dialog_class->response = anjuta_preferences_dialog_response;
	dialog_class->close = anjuta_ui_close;
}

static void
anjuta_ui_instance_init (AnjutaUI *ui)
{
	GtkWidget *view;
	
	ui->priv = g_new0 (AnjutaUIPrivate, 1);
	ui->priv->merge = egg_menu_merge_new();
	ui->priv->actions_hash = g_hash_table_new_full (g_str_hash,
													g_str_equal,
													(GDestroyNotify) g_free,
													(GDestroyNotify) NULL);
	ui->priv->icon_factory = gtk_icon_factory_new ();
	gtk_icon_factory_add_default (ui->priv->icon_factory);
	
	gtk_window_set_default_size (GTK_WINDOW (ui), 500, 400);
	view = create_tree_view (ui);
	gtk_widget_show_all (view);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(ui)->vbox), view);
}

GtkWidget *
anjuta_ui_new (GtkWidget *ui_container,
			   GCallback add_widget_cb, GCallback remove_widget_cb)
{
	GtkWidget *widget;
	widget = gtk_widget_new (ANJUTA_TYPE_UI, 
				 "title", _("Anjuta User Interface"),
				 NULL);
	if (add_widget_cb)
		g_signal_connect (G_OBJECT (ANJUTA_UI (widget)->priv->merge),
						  "add_widget", G_CALLBACK (add_widget_cb),
						  ui_container);
	if (remove_widget_cb)
		g_signal_connect (G_OBJECT (ANJUTA_UI (widget)->priv->merge),
						  "remove_widget", G_CALLBACK (remove_widget_cb),
						  ui_container);
	return widget;
}

GtkIconFactory*
anjuta_ui_get_icon_factory (AnjutaUI *ui)
{
	g_return_val_if_fail (ANJUTA_IS_UI (ui), NULL);
	return ui->priv->icon_factory;
}

EggActionGroup*
anjuta_ui_add_action_group_entries (AnjutaUI *ui,
									const gchar *action_group_name,
									const gchar *action_group_label,
									EggActionGroupEntry *entries,
									gint num_entries)
{
	EggActionGroup *action_group;
	
	g_return_val_if_fail (ANJUTA_IS_UI (ui), NULL);
	g_return_val_if_fail (action_group_name != NULL, NULL);
	g_return_val_if_fail (action_group_name != NULL, NULL);
	
	action_group = egg_action_group_new (action_group_name);
	egg_action_group_add_actions (action_group, entries, num_entries);
	anjuta_ui_add_action_group (ui, action_group_name,
								action_group_label, action_group);
	// g_object_unref (action_group);
	return action_group;
}

void
anjuta_ui_add_action_group (AnjutaUI *ui,
							const gchar *action_group_name,
							const gchar *action_group_label,
							EggActionGroup *action_group)
{
	GList *actions, *l;
	GtkTreeIter parent;
	GdkPixbuf *pixbuf;
	
	g_return_if_fail (ANJUTA_IS_UI (ui));
	g_return_if_fail (EGG_IS_ACTION_GROUP (action_group));
	g_return_if_fail (action_group_name != NULL);
	g_return_if_fail (action_group_name != NULL);
	
	egg_menu_merge_insert_action_group (ui->priv->merge, action_group, 0);
	g_hash_table_insert (ui->priv->actions_hash,
						g_strdup (action_group_name), action_group);
	actions = egg_action_group_list_actions (action_group);
	gtk_tree_store_append (GTK_TREE_STORE (ui->priv->model),
						   &parent, NULL);
	pixbuf = anjuta_res_get_pixbuf (ANJUTA_PIXMAP_CLOSED_FOLDER);
	gtk_tree_store_set (GTK_TREE_STORE (ui->priv->model), &parent,
						COLUMN_PIXBUF, pixbuf,
						COLUMN_ACTION, action_group_label,
						COLUMN_GROUP, action_group_name,
						-1);
	for (l = actions; l; l = l->next)
	{
		gchar *action_label;
		// gchar *accel_name;
		GtkTreeIter iter;
		GtkWidget *icon;
		EggAction *action = l->data;
		
		if (!action)
			continue;
		gtk_tree_store_append (GTK_TREE_STORE (ui->priv->model),
							   &iter, &parent);
		action_label = get_action_label (action);
		if (action->stock_id)
		{
			pixbuf = gtk_widget_render_icon (GTK_WIDGET (ui),
											 action->stock_id,
											 GTK_ICON_SIZE_MENU,
											 NULL);
			gtk_tree_store_set (GTK_TREE_STORE (ui->priv->model), &iter,
								COLUMN_PIXBUF, pixbuf,
								COLUMN_ACTION, action_label,
								COLUMN_VISIBLE, action->visible,
								COLUMN_SENSITIVE, action->sensitive,
								COLUMN_DATA, action,
								COLUMN_GROUP, action_group_name,
								-1);
			g_object_unref (G_OBJECT (pixbuf));
		}
		else
		{
			gtk_tree_store_set (GTK_TREE_STORE (ui->priv->model), &iter,
								COLUMN_ACTION, action_label,
								COLUMN_VISIBLE, action->visible,
								COLUMN_SENSITIVE, action->sensitive,
								COLUMN_DATA, action,
								COLUMN_GROUP, action_group_name,
								-1);
		}
		g_free (action_label);
	}
}

static gboolean
on_action_group_remove_hash (gpointer key, gpointer value, gpointer data)
{
	if (data == value)
	{
#ifdef DEBUG
		g_message ("Removing action group from hash: %s",
				   egg_action_group_get_name (EGG_ACTION_GROUP (data)));
#endif
		return TRUE;
	}
	else
		return FALSE;
}

void
anjuta_ui_remove_action_group (AnjutaUI *ui, EggActionGroup *action_group)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;

	g_return_if_fail (ANJUTA_IS_UI (ui));
		
	const gchar *name;
	name = egg_action_group_get_name (action_group);
	g_hash_table_foreach_remove (ui->priv->actions_hash,
								 on_action_group_remove_hash, action_group);
	
	model = ui->priv->model;
	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid)
	{
		GtkTreeIter parent;
		const gchar *group;
		const gchar *group_name;
		
		gtk_tree_model_get (model, &iter, COLUMN_GROUP, &group, -1);
		group_name = egg_action_group_get_name (EGG_ACTION_GROUP (action_group));
		
		g_message ("%s == %s", group, group_name);
		if (group_name == NULL || group == NULL)
		{
			valid = gtk_tree_model_iter_next (model, &iter);
			continue;
		}
		if (strcmp (group_name, group) == 0)
		{
#ifdef DEBUG
			g_message ("Removing action group from tree: %s", group);
#endif
			valid = gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);
		}
		else
			valid = gtk_tree_model_iter_next (model, &iter);
	}
	egg_menu_merge_remove_action_group (ui->priv->merge, action_group);
}

gint
anjuta_ui_merge (AnjutaUI *ui, const gchar *ui_filename)
{
	gint id;
	GError *err = NULL;
	
	g_return_val_if_fail (ANJUTA_IS_UI (ui), -1);
	
	id = egg_menu_merge_add_ui_from_file(ui->priv->merge, ui_filename, &err);
#ifdef DEBUG
	g_message("merging [%d] %s", id, ui_filename);
#endif
	if (err != NULL)
		g_warning ("Could not merge [%s]: %s", ui_filename, err->message);
	return id;
}

void
anjuta_ui_unmerge (AnjutaUI *ui, gint id)
{
#ifdef DEBUG
	g_message("Menu unmerging %d", id);
#endif
	g_return_if_fail (ANJUTA_IS_UI (ui));
	egg_menu_merge_remove_ui(ui->priv->merge, id);
}

static gchar *node_type_names[] = {
  "UNDECIDED",
  "ROOT",
  "MENUBAR",
  "MENU",
  "TOOLBAR",
  "MENU_PLACEHOLDER",
  "TOOLBAR_PLACEHOLDER",
  "POPUPS",
  "MENUITEM",
  "TOOLITEM",
  "SEPARATOR"
};

static void
print_node (EggMenuMerge *merge, GNode *node, gint indent_level)
{
	gint i;
	EggMenuMergeNode *mnode;
	GNode *child;
	gchar *action_label = NULL;
	
	mnode = node->data;
	
	if (mnode->action)
		g_object_get (mnode->action, "label", &action_label, NULL);
	
	for (i = 0; i < indent_level; i++)
	printf("  ");
	printf("%s (%s): action_name=%s action_label=%s\n", mnode->name,
	node_type_names[mnode->type],
	g_quark_to_string(mnode->action_name), action_label);
	
	g_free(action_label);
	
	for (child = node->children; child != NULL; child = child->next)
		print_node(merge, child, indent_level + 1);
}

GtkAccelGroup*
anjuta_ui_get_accel_group (AnjutaUI *ui)
{
	g_return_val_if_fail (ANJUTA_IS_UI (ui), NULL);
	return ui->priv->merge->accel_group;
}

EggMenuMerge*
anjuta_ui_get_menu_merge (AnjutaUI *ui)
{
	g_return_val_if_fail (ANJUTA_IS_UI (ui), NULL);
	return ui->priv->merge;
}

EggAction*
anjuta_ui_get_action (AnjutaUI *ui, const gchar *action_group_name,
					  const gchar *action_name)
{
	EggActionGroup *action_group;
	EggAction *action;
	
	g_return_val_if_fail (ANJUTA_IS_UI (ui), NULL);
	
	action_group = g_hash_table_lookup (ui->priv->actions_hash,
										action_group_name);
	if (EGG_IS_ACTION_GROUP (action_group) == FALSE)
	{
		g_warning ("Unable to find action group \"%s\"", action_group_name);
		return NULL;
	}
	action = egg_action_group_get_action (action_group, action_name);
	if (EGG_IS_ACTION (action))
		return action;
	g_warning ("Unable to find action \"%s\" in group \"%s\"",
			  action_name, action_group_name);
	return NULL;
}

void
anjuta_ui_activate_action_by_path (AnjutaUI *ui, gchar *action_path)
{
	const gchar *action_group_name;
	const gchar *action_name;
	EggAction *action;
	gchar **strv;
	
	g_return_if_fail (ANJUTA_IS_UI (ui));
	g_return_if_fail (action_path != NULL);
	
	strv = g_strsplit (action_path, "/", 2);
	action_group_name = strv[0];
	action_name = strv[1];
	
	g_return_if_fail (action_group_name != NULL && action_name != NULL);
	
	action = anjuta_ui_get_action (ui, action_group_name, action_name);
	if (action)
		egg_action_activate (action);
	g_strfreev (strv);
}

void
anjuta_ui_activate_action_by_group (AnjutaUI *ui, EggActionGroup *action_group,
									const gchar *action_name)
{
	EggAction *action;
	
	g_return_if_fail (ANJUTA_IS_UI (ui));
	g_return_if_fail (action_group != NULL && action_name != NULL);
	
	action = egg_action_group_get_action (action_group, action_name);
	if (EGG_IS_ACTION (action))
		egg_action_activate (action);
}

void
anjuta_ui_dump_tree (AnjutaUI *ui)
{
	g_return_if_fail (ANJUTA_IS_UI(ui));
	
	egg_menu_merge_ensure_update (ui->priv->merge);
	print_node (ui->priv->merge, ui->priv->merge->root_node, 0);
}
