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

#ifndef ANJUTA_WINDOWS_DIALOG_H
#define ANJUTA_WINDOWS_DIALOG_H

#include <gtk/gtkdialog.h>
#include <properties.h>

#define ANJUTA_TYPE_WINDOWS_DIALOG        (anjuta_windows_dialog_get_type ())
#define ANJUTA_WINDOWS_DIALOG(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_WINDOWS_DIALOG, AnjutaWindowsDialog))
#define ANJUTA_WINDOWS_DIALOG_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), ANJUTA_TYPE_WINDOWS_DIALOG, AnjutaWindowsDialogClass))
#define ANJUTA_IS_WINDOWS_DIALOG(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_WINDOWS_DIALOG))
#define ANJUTA_IS_WINDOWS_DIALOG_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_WINDOWS_DIALOG))

typedef struct _AnjutaWindowsDialog        AnjutaWindowsDialog;
typedef struct _AnjutaWindowsDialogClass   AnjutaWindowsDialogClass;
typedef struct _AnjutaWindowsDialogPrivate AnjutaWindowsDialogPrivate;

struct _AnjutaWindowsDialog {
	GtkDialog parent;
	
	AnjutaWindowsDialogPrivate *priv;
};

struct _AnjutaWindowsDialogClass {
	GtkDialogClass parent;
};

GType anjuta_windows_dialog_get_type (void);

GtkWidget *anjuta_windows_dialog_new (PropsID props);

void anjuta_windows_register_window (AnjutaWindowsDialog *dlg, GtkWindow *win,
									 const gchar *name, const gchar *icon,
									 GtkWindow *parent);

void anjuta_windows_unregister_window (AnjutaWindowsDialog* dlg,
									   GtkWidget *win);

#endif
