/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#ifndef __EDITOR_TOOLTIPS_H__
#define __EDITOR_TOOLTIPS_H__

#include <gtk/gtkwidget.h>


#ifdef __cplusplus
extern "C" {
#endif				/* __cplusplus */


#define EDITOR_TYPE_TOOLTIPS                  (editor_tooltips_get_type ())
#define EDITOR_TOOLTIPS(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EDITOR_TYPE_TOOLTIPS, EditorTooltips))
#define EDITOR_TOOLTIPS_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), EDITOR_TYPE_TOOLTIPS, EditorTooltipsClass))
#define EDITOR_IS_TOOLTIPS(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EDITOR_TYPE_TOOLTIPS))
#define EDITOR_IS_TOOLTIPS_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), EDITOR_TYPE_TOOLTIPS))
#define EDITOR_TOOLTIPS_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), EDITOR_TYPE_TOOLTIPS, EditorTooltipsClass))


typedef struct _EditorTooltips EditorTooltips;
typedef struct _EditorTooltipsClass EditorTooltipsClass;
typedef struct _EditorTooltipsData EditorTooltipsData;

struct _EditorTooltipsData {
	EditorTooltips *tooltips;
	GtkWidget *widget;
	gchar *tip_text;
	gchar *tip_private;
};

struct _EditorTooltips {
	GtkObject parent_instance;

	GtkWidget *tip_window;
	GtkWidget *tip_label;
	EditorTooltipsData *active_tips_data;
	GList *tips_data_list;

	guint delay:30;
	guint enabled:1;
	guint have_grab:1;
	guint use_sticky_delay:1;
	gint timer_tag;
	GTimeVal last_popdown;
};

struct _EditorTooltipsClass {
	GtkObjectClass parent_class;

	/* Padding for future expansion */
	void (*_gtk_reserved1) (void);
	void (*_gtk_reserved2) (void);
	void (*_gtk_reserved3) (void);
	void (*_gtk_reserved4) (void);
};

GType editor_tooltips_get_type (void) G_GNUC_CONST;
EditorTooltips *editor_tooltips_new (void);

void editor_tooltips_enable (EditorTooltips * tooltips);
void editor_tooltips_disable (EditorTooltips * tooltips);

void editor_tooltips_set_tip (EditorTooltips * tooltips,
			     GtkWidget * widget,
			     const gchar * tip_text,
			     const gchar * tip_private);

EditorTooltipsData *editor_tooltips_data_get (GtkWidget * widget);
void editor_tooltips_force_window (EditorTooltips * tooltips);

void _editor_tooltips_toggle_keyboard_mode (GtkWidget * widget);

#ifdef __cplusplus

}
#endif				/* __cplusplus */
#endif				/* __EDITOR_TOOLTIPS_H__ */
