/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-ui.c
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
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * SECTION:anjuta-ui
 * @short_description: User Interface manager
 * @see_also: #GtkAction, #GtkActionEntry, #GtkToggleAction,
 *   #GtkToggleActionEntry, #GtkRadioAction, #GtkRadioActionEntry,
 *   #GtkActionGroup, #GtkUIManager
 * @stability: Unstable
 * @include: libanjuta/
 * 
 * #AnjutaUI subclasses #GtkUIManager, so you should really read #GtkUIManager
 * documentation first to know about Actions, UI merging and UI XML file
 * format. This documentation will cover only the relevent	APIs.
 * 
 * #AnjutaUI has its own methods for adding action groups, which is differnt
 * from #GtkUIManager methods. All #AnjutaPlugin based classes should use
 * these methods instead of #GtkUIManager methods. The reason is, in addition
 * to adding the actions and groups to the UI manager, it also resgisters
 * them for UI customization and accellerators editing. It also keeps
 * record of all actions.
 * 
 * An interesting side effect of this is that these
 * actions could be conveniently accessed or activated with
 * anjuta_ui_get_action() or anjuta_ui_activate_action_by_path(), without
 * the need of original action group object. This makes it is possible for
 * activating actions remotely from other plugins.
 * 
 * anjuta_ui_get_accel_editor() will return a widget containing the
 * UI customization and accellerators editor. All actions and action groups
 * are organized into a tree view, which should be added to a visible
 * container (e.g. a #GtkDialog based object) and displayed to users.
 * 
 * <note>
 * 	<para>
 * 		Any actions additions/removals using #GtkUIManager are not
 * 		registred with #AnjutaUI and hence their accellerators
 * 		cannot be edited. Nor will they be listed in UI manager
 * 		dialog. Hence, use #AnjutaUI methods whenever possible.
 * 	</para>
 * </note>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <glib/gi18n.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkiconfactory.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtktreestore.h>
#include <gtk/gtkcellrenderertoggle.h>
#include <gtk/gtkaccelmap.h>
#include <gtk/gtkcellrendererpixbuf.h>
#include <gtk/gtkimage.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkcellrendereraccel.h>

#include <libgnome/gnome-macros.h>

#include "resources.h"
#include "anjuta-ui.h"
#include "anjuta-debug.h"

struct _AnjutaUIPrivate {
	GtkIconFactory *icon_factory;
	GtkTreeModel *model;
	GHashTable *customizable_actions_hash;
	GHashTable *uncustomizable_actions_hash;
};

enum {
	COLUMN_PIXBUF,
	COLUMN_ACTION_LABEL,
	COLUMN_VISIBLE,
	COLUMN_SENSITIVE,
	COLUMN_ACTION,
	COLUMN_GROUP,
	N_COLUMNS
};

#if 0
static void
sensitivity_toggled (GtkCellRendererToggle *cell,
					 const gchar *path_str, GtkTreeModel *model)
{
	GtkTreePath *path;
	GtkTreeIter iter;
	GtkAction *action;
	gboolean sensitive;
	
	path = gtk_tree_path_new_from_string (path_str);
	gtk_tree_model_get_iter (model, &iter, path);
	
	gtk_tree_model_get (model, &iter,
						COLUMN_SENSITIVE, &sensitive,
						COLUMN_ACTION, &action, -1);
	g_object_set (G_OBJECT (action), "sensitive", !sensitive, NULL);
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
						COLUMN_SENSITIVE, !sensitive, -1);
	gtk_tree_path_free (path);
}
#endif

static void
visibility_toggled (GtkCellRendererToggle *cell,
					const gchar *path_str, GtkTreeModel *model)
{
	GtkTreePath *path;
	GtkTreeIter iter;
	GtkAction *action;
	gboolean visible;
	
	path = gtk_tree_path_new_from_string (path_str);
	gtk_tree_model_get_iter (model, &iter, path);
	
	gtk_tree_model_get (model, &iter,
						COLUMN_VISIBLE, &visible,
						COLUMN_ACTION, &action, -1);
	g_object_set (G_OBJECT (action), "visible", !visible, NULL);
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
						COLUMN_VISIBLE, !visible, -1);
	gtk_tree_path_free (path);
}

static gchar*
get_action_label (GtkAction *action)
{
	gchar *action_label = NULL;
	
	g_object_get (G_OBJECT (action), "label", &action_label, NULL);
	if (action_label && strlen (action_label))
	{
		gchar *s, *d;
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
		action_label = g_strdup (gtk_action_get_name (action));
	return action_label;
}

static void
accel_edited_callback (GtkCellRendererAccel *cell,
                       const char          *path_string,
                       guint                keyval,
                       GdkModifierType      mask,
                       guint                hardware_keycode,
                       gpointer             data)
{
	GtkTreeModel *model = (GtkTreeModel *)data;
	GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
	GtkTreeIter iter;
	GtkAction *action;
	const gchar *accel_path;
	
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter,
						COLUMN_ACTION, &action, -1);
	
	/* sanity check */
	if (action == NULL)
		return;
	
	accel_path = gtk_action_get_accel_path (action);
	if (accel_path) {
		gtk_accel_map_change_entry (accel_path, keyval, mask, TRUE);
	}
	
	gtk_tree_path_free (path);
}

static void
accel_cleared_callback (GtkCellRendererAccel *cell,
						const char *path_string,
						gpointer data)
{
	GtkTreeModel *model = (GtkTreeModel *)data;
	GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
	GtkTreeIter iter;
	GtkAction *action;
	const gchar *accel_path;
	
	
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter,
						COLUMN_ACTION, &action, -1);
	
	/* sanity check */
	if (action == NULL)
		return;
	
	accel_path = gtk_action_get_accel_path (action);
	if (accel_path) {
		gtk_accel_map_change_entry (accel_path, 0, 0, TRUE);
	}
	
	gtk_tree_path_free (path);
}
	
	

static gint
iter_compare_func (GtkTreeModel *model, GtkTreeIter *a,
				   GtkTreeIter *b, gpointer user_data)
{
	gchar *text_a;
	gchar *text_b;
	gint retval = 0;
	
	gtk_tree_model_get (model, a, COLUMN_ACTION_LABEL, &text_a, -1);
	gtk_tree_model_get (model, b, COLUMN_ACTION_LABEL, &text_b, -1);
	if (text_a == NULL && text_b == NULL) retval = 0;
	else if (text_a == NULL) retval = -1;
	else if (text_b == NULL) retval = 1;
	else retval =  strcasecmp (text_a, text_b);
	
	g_free(text_a);
	g_free(text_b);
	
	return retval;
}

static void
accel_set_func (GtkTreeViewColumn *tree_column,
                GtkCellRenderer   *cell,
                GtkTreeModel      *model,
                GtkTreeIter       *iter,
                gpointer           data)
{
	GtkAction *action;
	const gchar *accel_path;
	GtkAccelKey key;
	
	gtk_tree_model_get (model, iter,
						COLUMN_ACTION, &action, -1);
	if (action == NULL)
		g_object_set (G_OBJECT (cell), "visible", FALSE, NULL);
	else
	{
		if ((accel_path = gtk_action_get_accel_path (action)))
		{
			if (gtk_accel_map_lookup_entry (accel_path, &key))
			{
				g_object_set (G_OBJECT (cell), "visible", TRUE,
							  "accel-key", key.accel_key,
							  "accel-mods", key.accel_mods, NULL);
			}
			else
				g_object_set (G_OBJECT (cell), "visible", TRUE,
							  "accel-key", 0,
							  "accel-mods", 0, NULL);	
		}
	}
}

static void anjuta_ui_class_init (AnjutaUIClass *class);
static void anjuta_ui_instance_init (AnjutaUI *ui);

GNOME_CLASS_BOILERPLATE (AnjutaUI, anjuta_ui,
						 GtkUIManager, GTK_TYPE_UI_MANAGER);

static void
anjuta_ui_dispose (GObject *obj)
{
	AnjutaUI *ui = ANJUTA_UI (obj);

	if (ui->priv->model) {
		/* This will also release the refs on actions.
		 * Clear is necessary because following unref() might not actually
		 * finalize the model. It basically ensures all refs on actions
		 * are released irrespective of whether the model is finalized
		 * or not.
		 */
		gtk_tree_store_clear (GTK_TREE_STORE (ui->priv->model));
		
		g_object_unref (G_OBJECT (ui->priv->model));
		ui->priv->model = NULL;
	}
	if (ui->priv->customizable_actions_hash)
	{
		/* This will also release the refs on all action groups */
		g_hash_table_destroy (ui->priv->customizable_actions_hash);
		ui->priv->customizable_actions_hash = NULL;
	}
	if (ui->priv->uncustomizable_actions_hash)
	{
		/* This will also release the refs on all action groups */
		g_hash_table_destroy (ui->priv->uncustomizable_actions_hash);
		ui->priv->uncustomizable_actions_hash = NULL;
	}
	if (ui->priv->icon_factory) {
		g_object_unref (G_OBJECT (ui->priv->icon_factory));
		ui->priv->icon_factory = NULL;
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
anjuta_ui_class_init (AnjutaUIClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	parent_class = g_type_class_peek_parent (class);
	
	object_class->dispose = anjuta_ui_dispose;
	object_class->finalize = anjuta_ui_finalize;
}

static void
anjuta_ui_instance_init (AnjutaUI *ui)
{
	GtkTreeStore *store;
	
	/* Initialize member data */
	ui->priv = g_new0 (AnjutaUIPrivate, 1);
	ui->priv->customizable_actions_hash =
		g_hash_table_new_full (g_str_hash,
							   g_str_equal,
							   (GDestroyNotify) g_free,
							   NULL);
	ui->priv->uncustomizable_actions_hash =
		g_hash_table_new_full (g_str_hash,
							   g_str_equal,
							   (GDestroyNotify) g_free,
							   NULL);
	/* Create Icon factory */
	ui->priv->icon_factory = gtk_icon_factory_new ();
	gtk_icon_factory_add_default (ui->priv->icon_factory);
	
	/* Create Accel editor model */
	store = gtk_tree_store_new (N_COLUMNS,
								GDK_TYPE_PIXBUF,
								G_TYPE_STRING,
								G_TYPE_BOOLEAN,
								G_TYPE_BOOLEAN,
								G_TYPE_OBJECT,
								G_TYPE_STRING);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE(store), COLUMN_ACTION_LABEL,
									 iter_compare_func, NULL, NULL);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE(store),
										  COLUMN_ACTION_LABEL, GTK_SORT_ASCENDING);
	
	/* unreferenced in dispose() method. */
	ui->priv->model = GTK_TREE_MODEL (store);
}

/**
 * anjuta_ui_new:
 * 
 * Creates a new instance of #AnjutaUI.
 * 
 * Return value: A #AnjutaUI object
 */
AnjutaUI *
anjuta_ui_new (void)
{
	return g_object_new (ANJUTA_TYPE_UI, NULL);
}

/**
 * anjuta_ui_add_action_group_entries:
 * @ui: A #AnjutaUI object.
 * @action_group_name: Untranslated name of the action group.
 * @action_group_label: Translated label of the action group.
 * @entries: An array of action entries.
 * @num_entries: Number of elements in the action entries array.
 * @can_customize: If true the actions are customizable by user.
 * @translation_domain: The translation domain used to translated the entries.
 * It is usually the GETTEXT_PACKAGE macro in a project.
 * @user_data: User data to pass to action objects. This is the data that
 * will come as user_data in "activate" signal of the actions.
 * 
 * #GtkAction objects are created from the #GtkActionEntry structures and
 * added to the UI Manager. "activate" signal of #GtkAction is connected for
 * all the action objects using the callback in the entry structure and the
 * @user_data passed here.
 *
 * This group of actions are registered with the name @action_group_name
 * in #AnjutaUI. A #GtkAction object from this action group can be later
 * retrieved by anjuta_ui_get_action() using @action_group_name and action name.
 * @action_group_label is used as the display name for the action group in
 * UI manager dialog where action shortcuts are configured.
 * 
 * Return value: A #GtkActionGroup object holding all the action objects.
 */
GtkActionGroup*
anjuta_ui_add_action_group_entries (AnjutaUI *ui,
									const gchar *action_group_name,
									const gchar *action_group_label,
									GtkActionEntry *entries,
									gint num_entries,
									const gchar *translation_domain,
									gboolean can_customize,
									gpointer user_data)
{
	GtkActionGroup *action_group;
	
	g_return_val_if_fail (ANJUTA_IS_UI (ui), NULL);
	g_return_val_if_fail (action_group_name != NULL, NULL);
	g_return_val_if_fail (action_group_name != NULL, NULL);
	
	action_group = gtk_action_group_new (action_group_name);
	
	gtk_action_group_set_translation_domain (action_group, translation_domain);
	gtk_action_group_add_actions (action_group, entries, num_entries,
								  user_data);
	anjuta_ui_add_action_group (ui, action_group_name,
								action_group_label, action_group,
								can_customize);
	return action_group;
}

/**
 * anjuta_ui_add_toggle_action_group_entries:
 * @ui: A #AnjutaUI object.
 * @action_group_name: Untranslated name of the action group.
 * @action_group_label: Translated label of the action group.
 * @entries: An array of action entries.
 * @num_entries: Number of elements in the action entries array.
 * @translation_domain: The translation domain used to translated the entries.
 * It is usually the GETTEXT_PACKAGE macro in a project.
 * @user_data: User data to pass to action objects. This is the data that
 * will come as user_data in "activate" signal of the actions.
 * 
 * This is similar to anjuta_ui_add_action_group_entries(), except that
 * it adds #GtkToggleAction objects after creating them from the @entries.
 * 
 * Return value: A #GtkActionGroup object holding all the action objects.
 */
GtkActionGroup*
anjuta_ui_add_toggle_action_group_entries (AnjutaUI *ui,
									const gchar *action_group_name,
									const gchar *action_group_label,
									GtkToggleActionEntry *entries,
									gint num_entries,
									const gchar *translation_domain,
									gboolean can_customize,
									gpointer user_data)
{
	GtkActionGroup *action_group;
	
	g_return_val_if_fail (ANJUTA_IS_UI (ui), NULL);
	g_return_val_if_fail (action_group_name != NULL, NULL);
	g_return_val_if_fail (action_group_name != NULL, NULL);
	
	action_group = gtk_action_group_new (action_group_name);
	gtk_action_group_set_translation_domain (action_group, translation_domain);
	gtk_action_group_add_toggle_actions (action_group, entries, num_entries,
										 user_data);
	anjuta_ui_add_action_group (ui, action_group_name,
								action_group_label, action_group,
								can_customize);
	return action_group;
}

/**
 * anjuta_ui_add_action_group:
 * @ui: A #AnjutaUI object.
 * @action_group_name: Untranslated name of the action group.
 * @action_group_label: Translated label of the action group.
 * @action_group: #GtkActionGroup object to add.
 * 
 * This is similar to anjuta_ui_add_action_group_entries(), except that
 * it adds #GtkActionGroup object @action_group directly. All actions in this
 * group are automatically registered in #AnjutaUI and can be retrieved
 * normally with anjuta_ui_get_action().
 */
void
anjuta_ui_add_action_group (AnjutaUI *ui,
							const gchar *action_group_name,
							const gchar *action_group_label,
							GtkActionGroup *action_group,
							gboolean can_customize)
{
	GList *actions, *l;
	GtkTreeIter parent;
	GdkPixbuf *pixbuf;
	gint n_actions_added = 0;
	
	g_return_if_fail (ANJUTA_IS_UI (ui));
	g_return_if_fail (GTK_IS_ACTION_GROUP (action_group));
	g_return_if_fail (action_group_name != NULL);
	g_return_if_fail (action_group_name != NULL);

	gtk_ui_manager_insert_action_group (GTK_UI_MANAGER (ui), action_group, 0);
	
	if (can_customize)
	{
		g_hash_table_insert (ui->priv->customizable_actions_hash,
							g_strdup (action_group_name), action_group);
	}
	else
	{
		g_hash_table_insert (ui->priv->uncustomizable_actions_hash,
							g_strdup (action_group_name), action_group);
	}
	
	actions = gtk_action_group_list_actions (action_group);
	gtk_tree_store_append (GTK_TREE_STORE (ui->priv->model),
						   &parent, NULL);
	pixbuf = NULL;
	gtk_tree_store_set (GTK_TREE_STORE (ui->priv->model), &parent,
						COLUMN_PIXBUF, pixbuf,
						COLUMN_ACTION_LABEL, action_group_label,
						COLUMN_GROUP, action_group_name,
						-1);
	for (l = actions; l; l = l->next)
	{
		gchar *action_label;
		gchar *icon;
		guint signal_id;
		gint n_handlers;
		GtkTreeIter iter;
		GtkAction *action = l->data;
		
		if (!action)
			continue;
		
		signal_id = g_signal_lookup ("activate", GTK_TYPE_ACTION);
		n_handlers = g_signal_has_handler_pending (action, signal_id,
												   0, TRUE);
		if (n_handlers == 0)
			continue; /* The action element is not user configuration */
		
		n_actions_added++;
		
		gtk_tree_store_append (GTK_TREE_STORE (ui->priv->model),
							   &iter, &parent);
		action_label = get_action_label (action);
		g_object_get (G_OBJECT (action), "stock-id", &icon, NULL);
		if (icon)
		{
			GtkWidget *dummy = gtk_label_new ("Dummy");
			g_object_ref_sink(G_OBJECT(dummy));
			pixbuf = gtk_widget_render_icon (dummy, icon,
											 GTK_ICON_SIZE_MENU, NULL);
			if (pixbuf)
			{
				gtk_tree_store_set (GTK_TREE_STORE (ui->priv->model), &iter,
									COLUMN_PIXBUF, pixbuf,
									COLUMN_ACTION_LABEL, action_label,
									COLUMN_VISIBLE, gtk_action_get_visible (action),
									COLUMN_SENSITIVE, gtk_action_get_sensitive(action),
									COLUMN_ACTION, action,
									COLUMN_GROUP, action_group_name,
									-1);
				g_object_unref (G_OBJECT (pixbuf));
			}
			g_object_unref (dummy);
			g_free (icon);
		}
		else
		{
			gtk_tree_store_set (GTK_TREE_STORE (ui->priv->model), &iter,
								COLUMN_ACTION_LABEL, action_label,
								COLUMN_VISIBLE, gtk_action_get_visible (action),
								COLUMN_SENSITIVE, gtk_action_get_sensitive (action),
								COLUMN_ACTION, action,
								COLUMN_GROUP, action_group_name,
								-1);
		}
		g_free (action_label);
	}
	
	g_list_free(actions);
	
	/* If there are no actions in the group, removed the group node */
	if (n_actions_added == 0)
		gtk_tree_store_remove (GTK_TREE_STORE (ui->priv->model),
							   &parent);
}

static gboolean
on_action_group_remove_hash (gpointer key, gpointer value, gpointer data)
{
	if (data == value)
		return TRUE;
	else
		return FALSE;
}

/**
 * anjuta_ui_remove_action_group:
 * @ui: A #AnjutaUI object
 * @action_group: #GtkActionGroup object to remove.
 *
 * Removes a previous added action group. All actions in this group are
 * also unregistered from UI manager.
 */
void
anjuta_ui_remove_action_group (AnjutaUI *ui, GtkActionGroup *action_group)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;

	g_return_if_fail (ANJUTA_IS_UI (ui));
		
	const gchar *name;
	name = gtk_action_group_get_name (action_group);
	model = ui->priv->model;
	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid)
	{
		gchar *group;
		const gchar *group_name;
		
		gtk_tree_model_get (model, &iter, COLUMN_GROUP, &group, -1);
		group_name = gtk_action_group_get_name (GTK_ACTION_GROUP (action_group));
		
		if (group_name == NULL || group == NULL)
		{
			valid = gtk_tree_model_iter_next (model, &iter);
			continue;
		}
		if (strcmp (group_name, group) == 0)
		{
			valid = gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);
		}
		else
			valid = gtk_tree_model_iter_next (model, &iter);
		g_free(group);
	}
	gtk_ui_manager_remove_action_group (GTK_UI_MANAGER (ui), action_group);
	
	g_hash_table_foreach_remove (ui->priv->customizable_actions_hash,
								 on_action_group_remove_hash, action_group);
	g_hash_table_foreach_remove (ui->priv->uncustomizable_actions_hash,
								 on_action_group_remove_hash, action_group);
}

/**
 * anjuta_ui_get_action:
 * @ui: This #AnjutaUI object
 * @action_group_name: Group name.
 * @action_name: Action name.
 * returns: A #GtkAction object
 *
 * Returns the action object with the name @action_name in @action_group_name.
 * Note that it will be only sucessully returned if the group has been added
 * using methods in #AnjutaUI.
 */
GtkAction*
anjuta_ui_get_action (AnjutaUI *ui, const gchar *action_group_name,
					  const gchar *action_name)
{
	GtkActionGroup *action_group;
	GtkAction *action;
	
	g_return_val_if_fail (ANJUTA_IS_UI (ui), NULL);
	
	action_group = g_hash_table_lookup (ui->priv->customizable_actions_hash,
										action_group_name);
	if (!action_group)
	{
		action_group = g_hash_table_lookup (ui->priv->uncustomizable_actions_hash,
											action_group_name);
	}
	if (GTK_IS_ACTION_GROUP (action_group) == FALSE)
	{
		g_warning ("Unable to find action group \"%s\"", action_group_name);
		return NULL;
	}
	action = gtk_action_group_get_action (action_group, action_name);
	if (GTK_IS_ACTION (action))
		return action;
	g_warning ("Unable to find action \"%s\" in group \"%s\"",
			  action_name, action_group_name);
	return NULL;
}

/**
 * anjuta_ui_activate_action_by_path:
 * @ui: This #AnjutaUI object
 * @action_path: Path of the action in the form "GroupName/ActionName"
 *
 * Activates the action represented by @action_path. The path is in the form
 * "ActionGroupName/ActionName". Note that it will only work if the group has
 * been added using methods in #AnjutaUI.
 */
void
anjuta_ui_activate_action_by_path (AnjutaUI *ui, const gchar *action_path)
{
	const gchar *action_group_name;
	const gchar *action_name;
	GtkAction *action;
	gchar **strv;
	
	g_return_if_fail (ANJUTA_IS_UI (ui));
	g_return_if_fail (action_path != NULL);
	
	strv = g_strsplit (action_path, "/", 2);
	action_group_name = strv[0];
	action_name = strv[1];
	
	g_return_if_fail (action_group_name != NULL && action_name != NULL);
	
	action = anjuta_ui_get_action (ui, action_group_name, action_name);
	if (action)
		gtk_action_activate (action);
	g_strfreev (strv);
}

/**
 * anjuta_ui_activate_action_by_group:
 * @ui: This #AnjutaUI object
 * @action_group: Action group.
 * @action_name: Action name.
 *
 * Activates the action @action_name in the #GtkActionGroup @action_group.
 * "ActionGroupName/ActionName". Note that it will only work if the group has
 * been added using methods in #AnjutaUI.
 */
void
anjuta_ui_activate_action_by_group (AnjutaUI *ui, GtkActionGroup *action_group,
									const gchar *action_name)
{
	GtkAction *action;
	
	g_return_if_fail (ANJUTA_IS_UI (ui));
	g_return_if_fail (action_group != NULL && action_name != NULL);
	
	action = gtk_action_group_get_action (action_group, action_name);
	if (GTK_IS_ACTION (action))
		gtk_action_activate (action);
}

/**
 * anjuta_ui_merge:
 * @ui: A #AnjutaUI object.
 * @ui_filename: UI file to merge into UI manager.
 *
 * Merges XML UI definition in @ui_filename. UI elements defined in the xml
 * are merged with existing UI elements in UI manager. The format of the
 * file content is the standard XML UI definition tree. For more detail,
 * read the documentation for #GtkUIManager.
 * 
 * Return value: Integer merge ID
 */
gint
anjuta_ui_merge (AnjutaUI *ui, const gchar *ui_filename)
{
	gint id;
	GError *err = NULL;
	
	g_return_val_if_fail (ANJUTA_IS_UI (ui), -1);
	g_return_val_if_fail (ui_filename != NULL, -1);
	id = gtk_ui_manager_add_ui_from_file(GTK_UI_MANAGER (ui),
										 ui_filename, &err);
#ifdef DEBUG
	{
		gchar *basename = g_path_get_basename (ui_filename);
		DEBUG_PRINT ("merged [%d] %s", id, basename);
		g_free(basename);
	}
#endif
	if (err != NULL)
		g_warning ("Could not merge [%s]: %s", ui_filename, err->message);
	return id;
}

/**
 * anjuta_ui_unmerge:
 * @ui: A #AnjutaUI object.
 * @id: Merge ID returned by anjuta_ui_merge().
 *
 * Unmerges UI with the ID value @id (returned by anjuta_ui_merge() when
 * it was merged. For more detail, read the documentation for #GtkUIManager.
 */
void
anjuta_ui_unmerge (AnjutaUI *ui, gint id)
{
	/* DEBUG_PRINT ("Menu unmerging %d", id); */
	g_return_if_fail (ANJUTA_IS_UI (ui));
	gtk_ui_manager_remove_ui(GTK_UI_MANAGER (ui), id);
}

/**
 * anjuta_ui_get_accel_group:
 * @ui: A #AnjutaUI object.
 * returns: A #GtkAccelGroup object.
 *
 * Returns the #GtkAccelGroup object associated with this UI manager.
 */
GtkAccelGroup*
anjuta_ui_get_accel_group (AnjutaUI *ui)
{
	g_return_val_if_fail (ANJUTA_IS_UI (ui), NULL);
	return gtk_ui_manager_get_accel_group (GTK_UI_MANAGER (ui));
}

/**
 * anjuta_ui_get_accel_editor:
 * @ui: A #AnjutaUI object.
 * returns: A #GtkWidget object.
 *
 * Creates an accel editor widget and returns it. It should be added to
 * container and displayed to users.
 *
 * Returns a #GtkWidget containing the editor.
 */
GtkWidget *
anjuta_ui_get_accel_editor (AnjutaUI *ui)
{
	GtkWidget *tree_view, *sw;
	GtkTreeStore *store;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	
	store = GTK_TREE_STORE (ui->priv->model);
	
	tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (tree_view), TRUE);
	
	/* Columns */
	column = gtk_tree_view_column_new ();
	/* gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE); */
	gtk_tree_view_column_set_title (column, _("Action"));

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_add_attribute (column, renderer, "pixbuf",
										COLUMN_PIXBUF);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
										COLUMN_ACTION_LABEL);
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
	/* gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE); */
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);
#if 0
	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (G_OBJECT (renderer), "toggled",
					  G_CALLBACK (sensitivity_toggled), store);
	column = gtk_tree_view_column_new_with_attributes (_("Sensitive"),
													   renderer,
													   "active",
													   COLUMN_SENSITIVE,
													   NULL);
	/* gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE); */
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);
#endif
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Shortcut"));
	/* gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);	*/
	renderer = g_object_new (GTK_TYPE_CELL_RENDERER_ACCEL,
							"editable", TRUE,
							NULL);
	g_signal_connect (G_OBJECT (renderer), "accel-edited",
					  G_CALLBACK (accel_edited_callback),
					  store);
	g_signal_connect (G_OBJECT (renderer), "accel-cleared",
					  G_CALLBACK (accel_cleared_callback),
					  store);
	g_object_set (G_OBJECT (renderer), "editable", TRUE, NULL);
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column, renderer, accel_set_func, NULL, NULL);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_ACTION);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);
	
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (sw), tree_view);
	gtk_widget_show_all (sw);
	return sw;
}

/**
 * anjuta_ui_get_icon_factory:
 * @ui: A #AnjutaUI object
 * 
 * This returns the IconFactory object. All icons should be registered using
 * this icon factory. Read the documentation for #GtkIconFactory on how to 
 * use it.
 *
 * Return value: The #GtkIconFactory object used by it
 */
GtkIconFactory*
anjuta_ui_get_icon_factory (AnjutaUI *ui)
{
	g_return_val_if_fail (ANJUTA_IS_UI (ui), NULL);
	return ui->priv->icon_factory;
}

/**
 * anjuta_ui_dump_tree:
 * @ui: A #AnjutaUI object.
 *
 * Dumps the current UI XML tree in STDOUT. Useful for debugging.
 */
void
anjuta_ui_dump_tree (AnjutaUI *ui)
{
	gchar *ui_str;
	
	g_return_if_fail (ANJUTA_IS_UI(ui));
	
	gtk_ui_manager_ensure_update (GTK_UI_MANAGER (ui));
	ui_str = gtk_ui_manager_get_ui (GTK_UI_MANAGER (ui));
	/* DEBUG_PRINT ("%s", ui_str); */
	g_free (ui_str);
}

/*
 * Accels
 */
static gchar *
anjuta_ui_get_accel_file (void)
{
	gchar *anjuta_dir = NULL;
	gchar *filename = NULL;

	anjuta_dir = anjuta_util_get_user_config_dir ();

	if (anjuta_dir != NULL)
		filename = g_build_filename (anjuta_dir, "anjuta-accels", NULL);
	
	g_free (anjuta_dir);

	return filename;
}

void
anjuta_ui_load_accels (void)
{
	gchar *filename;

	filename = anjuta_ui_get_accel_file ();
	if (filename != NULL)
	{
		gtk_accel_map_load (filename);
		g_free (filename);
	}
}

void
anjuta_ui_save_accels (void)
{
	gchar *filename;

	filename = anjuta_ui_get_accel_file ();
	if (filename != NULL)
	{
		gtk_accel_map_save (filename);
		g_free (filename);
	}
}
