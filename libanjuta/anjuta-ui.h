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
#ifndef _ANJUTA_UI_H_
#define _ANJUTA_UI_H_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

/* Usage Notes:
 * 1) Any object which added any action or action group is responsible
 * for removing them when done (for example, a plugin object).
 *
 * 2) Any object which merged a UI is responsible for unmerging it
 * when done with it (for example, a plugin object).
 *
 * 3) Avoid using EggMenuMerge object gotten by anjuta_ui_get_menu_merge(),
 * because AnjutaUI keeps track of all actions/action-groups added to or
 * removed from it and accordingly updates the required UI interfaces.
 * Use the EggMenuMerge object only to do things not doable by AnjutaUI.
 */
#include <gtk/gtkdialog.h>
#include <gtk/gtkaccelgroup.h>
#include <gtk/gtkuimanager.h>

#define ANJUTA_TYPE_UI        (anjuta_ui_get_type ())
#define ANJUTA_UI(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_UI, AnjutaUI))
#define ANJUTA_UI_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), ANJUTA_TYPE_UI, AnjutaUIClass))
#define ANJUTA_IS_UI(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_UI))
#define ANJUTA_IS_UI_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_UI))

typedef struct _AnjutaUI        AnjutaUI;
typedef struct _AnjutaUIClass   AnjutaUIClass;
typedef struct _AnjutaUIPrivate AnjutaUIPrivate;

struct _AnjutaUI {
	GtkDialog parent;
	
	AnjutaUIPrivate *priv;
};

struct _AnjutaUIClass {
	GtkDialogClass parent;
};

GType anjuta_ui_get_type (void);

/* Creates a new AnjutaUI object */
GtkWidget* anjuta_ui_new (GtkWidget *ui_container,
						  GCallback add_widget_cb,
						  GCallback remove_widget_cb);

/* Gets the icon factory */
GtkIconFactory* anjuta_ui_get_icon_factory (AnjutaUI* ui);

/* Adds a group of Action entries with the give group name.
 * Caller does not get a reference to the returned ActionGroup. Use it
 * as reference ID to remove the action-group later (after which the object
 * will no longer be valid).
 */
GtkActionGroup* anjuta_ui_add_action_group_entries (AnjutaUI *ui,
											const gchar *action_group_name,
											const gchar *action_group_label,
											GtkActionEntry *entries,
											gint num_entries,
											gpointer user_data);
GtkActionGroup* anjuta_ui_add_toggle_action_group_entries (AnjutaUI *ui,
											const gchar *action_group_name,
											const gchar *action_group_label,
											GtkToggleActionEntry *entries,
											gint num_entries,
											gpointer user_data);
void anjuta_ui_add_action_group (AnjutaUI *ui,
								 const gchar *action_group_name,
								 const gchar *action_group_label,
								 GtkActionGroup *action_group);

/* Removes the group of Actions */
void anjuta_ui_remove_action_group (AnjutaUI *ui, GtkActionGroup *action_group);

/* Merges the given UI description file (written in xml)
   Returns an id representing it */
gint anjuta_ui_merge (AnjutaUI *ui, const gchar *ui_filename);

/* Unmerges the give merge id */
void anjuta_ui_unmerge (AnjutaUI *ui, gint id);

/* Get accel group associated with UI */
GtkAccelGroup* anjuta_ui_get_accel_group (AnjutaUI *ui);

/* Warning: do not use it directly, except for things AnjutaUI 
 * is not capable of doing.
 */
GtkUIManager* anjuta_ui_get_menu_merge (AnjutaUI *ui);

/* Get the action object from the given group with the given name */
GtkAction * anjuta_ui_get_action (AnjutaUI *ui,
								  const gchar *action_group_name,
								  const gchar *action_name);

/* Activates (calls the action callback) the action given by the action
 * path. Path is given by "ActionGroupName/ActionName".
 */
void anjuta_ui_activate_action_by_path (AnjutaUI *ui,
										const gchar *action_path);

/* Activates the action in the given Action group object with the given
 * action name.
 */
void anjuta_ui_activate_action_by_group (AnjutaUI *ui,
										 GtkActionGroup *action_group,
										 const gchar *action_name);

/* Dump the whole tree in STDOUT. Useful for debugging */
void anjuta_ui_dump_tree (AnjutaUI *ui);

#endif
