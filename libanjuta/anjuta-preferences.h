/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * preferences.h
 * Copyright (C) 2000 - 2003  Naba Kumar  <naba@gnome.org>
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
#ifndef _ANJUTA_PREFERENCES_H_
#define _ANJUTA_PREFERENCES_H_

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>

#include <libanjuta/anjuta-preferences-dialog.h>
#include <libanjuta/anjuta-plugin-manager.h>

G_BEGIN_DECLS

typedef struct _AnjutaProperty AnjutaProperty;

/* Get functions. Add more get functions for AnjutaProperty, if required */
/* Gets the widget associated with the property */
GtkWidget* anjuta_property_get_widget (AnjutaProperty *prop);

#define ANJUTA_TYPE_PREFERENCES        (anjuta_preferences_get_type ())
#define ANJUTA_PREFERENCES(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PREFERENCES, AnjutaPreferences))
#define ANJUTA_PREFERENCES_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), ANJUTA_TYPE_PREFERENCES, AnjutaPreferencesClass))
#define ANJUTA_IS_PREFERENCES(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PREFERENCES))
#define ANJUTA_IS_PREFERENCES_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PREFERENCES))

typedef struct _AnjutaPreferences        AnjutaPreferences;
typedef struct _AnjutaPreferencesClass   AnjutaPreferencesClass;
typedef struct _AnjutaPreferencesPriv    AnjutaPreferencesPriv;

struct _AnjutaPreferences
{
	GObject parent;

	/*< private >*/
	AnjutaPreferencesPriv *priv;
};

struct _AnjutaPreferencesClass
{
	GObjectClass parent;
};

typedef gboolean (*AnjutaPreferencesCallback) (AnjutaPreferences *pr,
                                               const gchar *key,
                                               gpointer data);

GType anjuta_preferences_get_type (void);

AnjutaPreferences *anjuta_preferences_new (AnjutaPluginManager *plugin_manager, const gchar *common_schema_id);
AnjutaPreferences *anjuta_preferences_default (void);

void anjuta_preferences_add_from_builder (AnjutaPreferences *pr,
                                          GtkBuilder *builder,
                                          GSettings *settings,
                                          const gchar *glade_widget_name,
                                          const gchar *stitle,
                                          const gchar *icon_filename);

void anjuta_preferences_remove_page (AnjutaPreferences *pr,
                                     const gchar *page_name);

/*
 * Registers all properties defined for widgets below the 'parent' widget
 * in the given gtkbuilder UI tree
 */
void anjuta_preferences_register_all_properties_from_builder_xml (AnjutaPreferences* pr,
                                                                  GtkBuilder *builder,
                                                                  GSettings *settings,
                                                                  GtkWidget *parent);
gboolean
anjuta_preferences_register_property (AnjutaPreferences *pr,
                                      GSettings *settings,
                                      GtkWidget *object,
                                      const gchar *key);

/* Dialog methods */
GtkWidget *anjuta_preferences_get_dialog (AnjutaPreferences *pr);
gboolean anjuta_preferences_is_dialog_created (AnjutaPreferences *pr);

G_END_DECLS

#endif
