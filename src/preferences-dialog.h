/* Anjuta
 * 
 * Copyright (C) 2002 Dave Camp
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef ANJUTA_PREFERENCES_DIALOG_H
#define ANJUTA_PREFERENCES_DIALOG_H

#include <gtk/gtkdialog.h>

#define ANJUTA_TYPE_PREFERENCES_DIALOG        (anjuta_preferences_dialog_get_type ())
#define ANJUTA_PREFERENCES_DIALOG(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PREFERENCES_DIALOG, AnjutaPreferencesDialog))
#define ANJUTA_PREFERENCES_DIALOG_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), ANJUTA_TYPE_PREFERENCES_DIALOG, AnjutaPreferencesDialogClass))
#define ANJUTA_IS_PREFERENCES_DIALOG(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PREFERENCES_DIALOG))
#define ANJUTA_IS_PREFERENCES_DIALOG_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PREFERENCES_DIALOG))

typedef struct _AnjutaPreferencesDialog        AnjutaPreferencesDialog;
typedef struct _AnjutaPreferencesDialogClass   AnjutaPreferencesDialogClass;
typedef struct _AnjutaPreferencesDialogPrivate AnjutaPreferencesDialogPrivate;

struct _AnjutaPreferencesDialog {
	GtkDialog parent;
	
	AnjutaPreferencesDialogPrivate *priv;
};

struct _AnjutaPreferencesDialogClass {
	GtkDialogClass parent;
};

GType anjuta_preferences_dialog_get_type (void);

GtkWidget *anjuta_preferences_dialog_get (void);

void anjuta_preferences_dialog_add_page (const char *name,
										 GdkPixbuf *icon,
										 GtkWidget *page);
void anjuta_preferences_dialog_remove_page (const char *name);

#endif
