/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*  window.h
 *  Copyright (C) 2006 Armin Burgmeier
 *
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __CLASSGEN_WINDOW_H__
#define __CLASSGEN_WINDOW_H__

#include <plugins/project-wizard/values.h>

#include <gtk/gtkdialog.h>

#include <glib-object.h>

G_BEGIN_DECLS

#define CG_TYPE_WINDOW             (cg_window_get_type ())
#define CG_WINDOW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CG_TYPE_WINDOW, CgWindow))
#define CG_WINDOW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), CG_TYPE_WINDOW, CgWindowClass))
#define CG_IS_WINDOW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CG_TYPE_WINDOW))
#define CG_IS_WINDOW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), CG_TYPE_WINDOW))
#define CG_WINDOW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CG_TYPE_WINDOW, CgWindowClass))

typedef struct _CgWindowClass CgWindowClass;
typedef struct _CgWindow CgWindow;

/* I would like to subclass GtkWindow or GtkDialog directly, but its
 * glade that creates the window and not me. */

struct _CgWindowClass
{
	GObjectClass parent_class;
};

struct _CgWindow
{
	GObject parent_instance;
};

GType cg_window_get_type (void) G_GNUC_CONST;

CgWindow *cg_window_new (void);
GtkDialog *cg_window_get_dialog (CgWindow *window);
NPWValueHeap *cg_window_create_value_heap (CgWindow *window);

const gchar *cg_window_get_header_template (CgWindow *window);
const gchar *cg_window_get_header_file (CgWindow *window);
const gchar *cg_window_get_source_template (CgWindow *window);
const gchar *cg_window_get_source_file (CgWindow *window);

void cg_window_set_add_to_project (CgWindow *window,
                                   gboolean enable);
void cg_window_set_add_to_repository (CgWindow *window,
                                   gboolean enable);
gboolean cg_window_get_add_to_project (CgWindow *window);
gboolean cg_window_get_add_to_repository(CgWindow *window);

void cg_window_enable_add_to_project (CgWindow *window,
                                      gboolean enable);
void cg_window_enable_add_to_repository (CgWindow *window,
                                         gboolean enable);

void cg_window_set_author (CgWindow *window,
                           const gchar *author);

void cg_window_set_email (CgWindow *window,
                          const gchar *email);

G_END_DECLS

#endif /* __CLASSGEN_WINDOW_H__ */
