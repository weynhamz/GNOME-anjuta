/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * pkg-config-chooser
 * Copyright (C) Johannes Schmid 2010 <jhs@gnome.org>
 * 
 * pkg-config-chooser is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * pkg-config-chooser is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _ANJUTA_PKG_CONFIG_CHOOSER_H_
#define _ANJUTA_PKG_CONFIG_CHOOSER_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define ANJUTA_TYPE_PKG_CONFIG_CHOOSER             (anjuta_pkg_config_chooser_get_type ())
#define ANJUTA_PKG_CONFIG_CHOOSER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_PKG_CONFIG_CHOOSER, AnjutaPkgConfigChooser))
#define ANJUTA_PKG_CONFIG_CHOOSER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_PKG_CONFIG_CHOOSER, AnjutaPkgConfigChooserClass))
#define ANJUTA_IS_PKG_CONFIG_CHOOSER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_PKG_CONFIG_CHOOSER))
#define ANJUTA_IS_PKG_CONFIG_CHOOSER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_PKG_CONFIG_CHOOSER))
#define ANJUTA_PKG_CONFIG_CHOOSER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_PKG_CONFIG_CHOOSER, AnjutaPkgConfigChooserClass))

typedef struct _AnjutaPkgConfigChooserClass AnjutaPkgConfigChooserClass;
typedef struct _AnjutaPkgConfigChooser AnjutaPkgConfigChooser;
typedef struct _AnjutaPkgConfigChooserPrivate AnjutaPkgConfigChooserPrivate;

struct _AnjutaPkgConfigChooserClass
{
	GtkTreeViewClass parent_class;
	
	/* Signals */
	void(* package_activated) (AnjutaPkgConfigChooser *self, const gchar* package);
	void(* package_deactivated) (AnjutaPkgConfigChooser *self, const gchar* package);
};

struct _AnjutaPkgConfigChooser
{
	GtkTreeView parent_instance;

	AnjutaPkgConfigChooserPrivate* priv;
};

GType anjuta_pkg_config_chooser_get_type (void) G_GNUC_CONST;
GtkWidget* anjuta_pkg_config_chooser_new (void);

GList* anjuta_pkg_config_chooser_get_active_packages (AnjutaPkgConfigChooser* chooser);
void anjuta_pkg_config_chooser_set_active_packages (AnjutaPkgConfigChooser* chooser, GList* packages);
void anjuta_pkg_config_chooser_show_active_only (AnjutaPkgConfigChooser* chooser, gboolean show_selected);
void anjuta_pkg_config_chooser_show_active_column (AnjutaPkgConfigChooser* chooser, gboolean show_column);

gchar* anjuta_pkg_config_chooser_get_selected_package (AnjutaPkgConfigChooser* chooser);


G_END_DECLS

#endif /* _ANJUTA_PKG_CONFIG_CHOOSER_H_ */
