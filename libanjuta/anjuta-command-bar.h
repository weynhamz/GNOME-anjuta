/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * git-shell-test
 * Copyright (C) James Liggett 2009 <jrliggett@cox.net>
 * 
 * git-shell-test is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * git-shell-test is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _ANJUTA_COMMAND_BAR_H_
#define _ANJUTA_COMMAND_BAR_H_

#include <glib-object.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_BEGIN_DECLS

#define ANJUTA_TYPE_COMMAND_BAR             (anjuta_command_bar_get_type ())
#define ANJUTA_COMMAND_BAR(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_COMMAND_BAR, AnjutaCommandBar))
#define ANJUTA_COMMAND_BAR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_COMMAND_BAR, AnjutaCommandBarClass))
#define ANJUTA_IS_COMMAND_BAR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_COMMAND_BAR))
#define ANJUTA_IS_COMMAND_BAR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_COMMAND_BAR))
#define ANJUTA_COMMAND_BAR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_COMMAND_BAR, AnjutaCommandBarClass))

typedef struct _AnjutaCommandBarClass AnjutaCommandBarClass;
typedef struct _AnjutaCommandBar AnjutaCommandBar;
typedef struct _AnjutaCommandBarPriv AnjutaCommandBarPriv;

/**
 * AnjutaCommandBarEntryType:
 * @ANJUTA_COMMAND_BAR_ENTRY_FRAME: This entry should create a frame in the 
 *									action bar. The entry's action name and 
 *									callback are ignored.
 * @ANJUTA_COMMAND_BAR_ENTRY_BUTTON: This entry adds a button to the action bar,
 *									 either to the last frame to appear in the 
 *									 entry list before this entry, or to the top
 *									 of the bar if no frames were previously 
 *									 added.
 *
 * Specifies if the entry corresponds to a frame or a button.
 * Buttons are added to the last frame that appears before the button entry
 */
typedef enum
{
	ANJUTA_COMMAND_BAR_ENTRY_FRAME,
	ANJUTA_COMMAND_BAR_ENTRY_BUTTON
} AnjutaCommandBarEntryType;

/**
 * AnjutaCommandBarEntry:
 * @type: The type of action
 * @action_name: The name of the action for this entry
 * @label: The display label for this entry
 * @stock_icon: The stock icon to display for this entry
 * @callback: Function to call when this entry's action is activated
 *
 * AnjutaCommandBarEntry is used to add a set of frames and actions to a command
 * bar.
 */
typedef struct
{
	AnjutaCommandBarEntryType type;
	const gchar *action_name;
	const gchar *label;
	const gchar *tooltip;
	const gchar *stock_icon;
	GCallback callback;
} AnjutaCommandBarEntry;

struct _AnjutaCommandBarClass
{
	GtkNotebookClass parent_class;
};

struct _AnjutaCommandBar
{
	GtkNotebook parent_instance;

	AnjutaCommandBarPriv *priv;
};

GType anjuta_command_bar_get_type (void) G_GNUC_CONST;
GtkWidget *anjuta_command_bar_new (void);
void anjuta_command_bar_add_action_group (AnjutaCommandBar *self, 
                                          const gchar *group_name, 
                                          const AnjutaCommandBarEntry *entries,
                                          int num_entries,
                                          gpointer user_data);
void anjuta_command_bar_remove_action_group (AnjutaCommandBar *self,
                                             const gchar *group_name);
void anjuta_command_bar_show_action_group (AnjutaCommandBar *self,
                                           const gchar *group_name);
GtkActionGroup *anjuta_command_bar_get_action_group (AnjutaCommandBar *self,
                                                     const gchar *group_name);
GtkAction *anjuta_command_bar_get_action (AnjutaCommandBar *self,
                                          const gchar *group_name, 
                                          const gchar *action_name);

G_END_DECLS

#endif /* _ANJUTA_COMMAND_BAR_H_ */
