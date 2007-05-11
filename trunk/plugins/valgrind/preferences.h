/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * preferences.c
 *
 *  Copyright (C) Jeffrey Stedfast 2003 <fejj@ximian.com>
 *  Copyright (C) Ximian, Inc. 2003 (www.ximian.com)
 *  Copyright (C) Massimo Cora' 2006 <maxcvs@gmail.com> 
 * 
 * preferences.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2, or (at your option) any later version.
 * 
 * plugin.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with plugin.h.  See the file "COPYING".  If not,
 * write to:  The Free Software Foundation, Inc.,
 *            59 Temple Place - Suite 330,
 *            Boston,  MA  02111-1307, USA.
 */
 

#ifndef _PREFERENCES_VALGRIND_H
#define _PREFERENCES_VALGRIND_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gi18n.h>


G_BEGIN_DECLS

#define VALGRIND_TYPE_PLUGINPREFS         (valgrind_plugin_prefs_get_type ())
#define VALGRIND_PLUGINPREFS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), VALGRIND_TYPE_PLUGINPREFS, ValgrindPluginPrefs))
#define VALGRIND_PLUGINPREFS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), VALGRIND_TYPE_PLUGINPREFS, ValgrindPluginPrefsClass))
#define VALGRIND_IS_PLUGINPREFS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), VALGRIND_TYPE_PLUGINPREFS))
#define VALGRIND_IS_PLUGINPREFS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), VALGRIND_TYPE_PLUGINPREFS))
#define VALGRIND_PLUGINPREFS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), VALGRIND_TYPE_PLUGINPREFS, ValgrindPluginPrefsClass))

typedef struct _ValgrindPluginPrefsPriv ValgrindPluginPrefsPriv;

typedef struct {
	GObject parent;
	ValgrindPluginPrefsPriv *priv;
} ValgrindPluginPrefs;

typedef struct {
	GObjectClass parent_class;

} ValgrindPluginPrefsClass;

GType valgrind_plugin_prefs_get_type (void);
ValgrindPluginPrefs *valgrind_plugin_prefs_new (void);


GtkWidget *valgrind_plugin_prefs_get_anj_prefs (void);
GtkWidget *valgrind_plugin_prefs_get_general_widget (void);
GtkWidget *valgrind_plugin_prefs_get_memcheck_widget (void);
GtkWidget *valgrind_plugin_prefs_get_cachegrind_widget (void);
GtkWidget *valgrind_plugin_prefs_get_helgrind_widget (void);

GPtrArray *valgrind_plugin_prefs_create_argv (ValgrindPluginPrefs *val, const char *tool);

G_END_DECLS

#endif /* _PREFERENCES_VALGRIND_H */
