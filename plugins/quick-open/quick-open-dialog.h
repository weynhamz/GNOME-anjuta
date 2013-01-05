/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * quick-open-dialog.h
 * Copyright (C) 2013 Carl-Anton Ingmarsson <carlantoni@src.gnome.org>
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

#ifndef _QUICK_OPEN_DIALOG_H_
#define _QUICK_OPEN_DIALOG_H_

#include <gtk/gtk.h>
#include <libanjuta/interfaces/ianjuta-document.h>

G_BEGIN_DECLS

#define QUICK_TYPE_OPEN_DIALOG             (quick_open_dialog_get_type ())
#define QUICK_OPEN_DIALOG(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), QUICK_TYPE_OPEN_DIALOG, QuickOpenDialog))
#define QUICK_OPEN_DIALOG_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), QUICK_TYPE_OPEN_DIALOG, QuickOpenDialogClass))
#define QUICK_IS_OPEN_DIALOG(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), QUICK_TYPE_OPEN_DIALOG))
#define QUICK_IS_OPEN_DIALOG_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), QUICK_TYPE_OPEN_DIALOG))
#define QUICK_OPEN_DIALOG_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), QUICK_TYPE_OPEN_DIALOG, QuickOpenDialogClass))

typedef struct _QuickOpenDialogClass QuickOpenDialogClass;
typedef struct _QuickOpenDialog QuickOpenDialog;
typedef struct _QuickOpenDialogPrivate QuickOpenDialogPrivate;


struct _QuickOpenDialogClass
{
    GtkDialogClass parent_class;
};

struct _QuickOpenDialog
{
    GtkDialog parent_instance;

    QuickOpenDialogPrivate* priv;
};

GType            quick_open_dialog_get_type(void) G_GNUC_CONST;

QuickOpenDialog* quick_open_dialog_new(void);


void   quick_open_dialog_add_document      (QuickOpenDialog* self,
                                            IAnjutaDocument* document);

void   quick_open_dialog_remove_document   (QuickOpenDialog* self,
                                            IAnjutaDocument* document);

void   quick_open_dialog_set_project_root  (QuickOpenDialog* self,
                                            GFile* project_root);

void   quick_open_dialog_add_project_files (QuickOpenDialog* self,
                                            GSList* files);

void   quick_open_dialog_add_project_file  (QuickOpenDialog* self,
                                            GFile* file);

void   quick_open_dialog_add_project_file  (QuickOpenDialog* self, GFile* file);

GObject* quick_open_dialog_get_selected_object (QuickOpenDialog* self);

G_END_DECLS

#endif /* _QUICK_OPEN_DIALOG_H_ */
