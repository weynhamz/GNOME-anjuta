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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef _ANJUTA_PREFERENCES_H_
#define _ANJUTA_PREFERENCES_H_

#include <gnome.h>
#include <glade/glade.h>

#include <libanjuta/properties.h>
#include <libanjuta/anjuta-preferences-dialog.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
	ANJUTA_PROPERTY_OBJECT_TYPE_TOGGLE,
	ANJUTA_PROPERTY_OBJECT_TYPE_SPIN,
	ANJUTA_PROPERTY_OBJECT_TYPE_ENTRY,
	ANJUTA_PROPERTY_OBJECT_TYPE_MENU,
	ANJUTA_PROPERTY_OBJECT_TYPE_TEXT,
	ANJUTA_PROPERTY_OBJECT_TYPE_COLOR,
	ANJUTA_PROPERTY_OBJECT_TYPE_FONT
} AnjutaPropertyObjectType;

typedef enum
{
	ANJUTA_PROPERTY_DATA_TYPE_BOOL,
	ANJUTA_PROPERTY_DATA_TYPE_INT,
	ANJUTA_PROPERTY_DATA_TYPE_TEXT,
	ANJUTA_PROPERTY_DATA_TYPE_COLOR,
	ANJUTA_PROPERTY_DATA_TYPE_FONT
} AnjutaPropertyDataType;

typedef enum
{
	ANJUTA_PREFERENCES_FILTER_NONE = 0,
	ANJUTA_PREFERENCES_FILTER_PROJECT = 1
} AnjutaPreferencesFilterType;

typedef struct _AnjutaProperty AnjutaProperty;

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

/* AnjutaPreferences:
 * @parent: Parent
 * @props_build_in: Properties object holding buildin values.
 * @props_global: Properties object holding global values.
 * 
 * This is a test description
 */
struct _AnjutaPreferences
{
	AnjutaPreferencesDialog parent;
	
	/*< public >*/
	
	/* Built in values */
	PropsID props_built_in;
	
	/* System values */
	PropsID props_global;
	
	/* User values */ 
	PropsID props_local;
	
	/* Session values */
	PropsID props_session;
	
	/* Instance values */
	PropsID props;
	
	/*< private >*/
	AnjutaPreferencesPriv *priv;
};

struct _AnjutaPreferencesClass
{
	AnjutaPreferencesDialogClass parent;
	void (*changed) (GtkWidget *pref);
};

typedef gboolean (*AnjutaPreferencesCallback) (AnjutaPreferences *pr,
											   const gchar *key,
											   gpointer data);

GType anjuta_preferences_get_type (void);

/**
 * anjuta_preferences_new:
 * 
 * Creates a new #AnjutaPreferences object
 */
GtkWidget *anjuta_preferences_new (void);

/**
 * anjuta_preferences_add_page:
 * @pr: a #AnjutaPreferences object
 * @gxml: #GladeXML object containing the preferences page
 * @glade_widget_name: Page widget name (as give with glade interface editor).
 * The widget will be searched with the given name and detached
 * (that is, removed from the container, if present) from it's parent.
 * @icon_filename: File name (of the form filename.png) of the icon representing
 * the preference page.
 * 
 * Add a page to the preferences sytem.
 * gxml is the GladeXML object of the glade dialog containing the page widget.
 * The glade dialog will contain the layout of the preferences widgets.
 * The widgets which are preference widgets (e.g. toggle button) should have
 * widget names of the form:
 *              preferences_OBJECTTYPE:DATATYPE:DEFAULT:FLAGS:PROPERTYKEY
 *     where,
 *       OBJECTTYPE is 'toggle', 'spin', 'entry', 'text', 'color' or 'font'.
 *       DATATYPE   is 'bool', 'int', 'float', 'text', 'color' or 'font'.
 *       DEFAULT    is the default value (in the appropriate format). The format
 *                     for color is '#XXXXXX' representing RGB value and for
 *                     font, it is the pango font description.
 *       FLAGS      is any flag associated with the property. Currently it
 *                     has only two values -- 0 and 1. For normal preference
 *                     property which is saved/retrieved globally, the flag = 0.
 *                     For preference property which is also saved/retrieved
 *                     along with the project, the flag = 1.
 *       PROPERTYKEY is the property key. e.g - 'tab.size'.
 *
 * All widgets having the above names in the gxml tree will be registered
 * and will become part of auto saving/loading. For example, refer to
 * anjuta preferences dialogs and study the widget names.
 *
 * Returns: void
 */
void anjuta_preferences_add_page (AnjutaPreferences* pr, GladeXML *gxml,
								  const char* glade_widget_name,
								  const gchar *icon_filename);

/**
 * anjuta_preferences_register_all_properties_from_glade_xml:
 * @pr: a #AnjutaPreferences Object
 * @gxml: GladeXML object containing the properties widgets.
 * @parent: Parent widget in the gxml object
 *
 * This will register all the properties names of the format described above
 * without considering the UI. Useful if you have the widgets shown elsewhere
 * but you want them to be part of preferences system.
 */
void anjuta_preferences_register_all_properties_from_glade_xml (AnjutaPreferences* pr,
																GladeXML *gxml,
																GtkWidget *parent);
/**
 * anjuta_preferences_register_property_from_string:
 * @pr: a #AnjutaPreferences object
 * @object: Widget to register
 * @property_desc: Property description (see anjuta_preferences_add_pag())
 *
 * This registers only one widget. The widget could be shown elsewhere.
 * the property_description should be of the form described before.
 */
gboolean
anjuta_preferences_register_property_from_string (AnjutaPreferences *pr,
												  GtkWidget *object,
												  const gchar *property_desc);

/**
 * anjuta_preferences_register_property_raw:
 * @pr: a #AnjutaPreferences object
 * @object: Widget to register
 * @key: Property key
 * @default_value: Default value of the key
 * @flags: Flags
 * @object_type: Object type of widget
 * @data_type: Data type of the property
 *
 * This also registers only one widget, but instead of supplying the property
 * parameters as a single parsable string (as done in previous method), it
 * takes them separately.
 */
gboolean
anjuta_preferences_register_property_raw (AnjutaPreferences *pr, GtkWidget *object,
										  const gchar *key,
										  const gchar *default_value,
										  guint flags,
										  AnjutaPropertyObjectType object_type,
										  AnjutaPropertyDataType  data_type);

/**
 * anjuta_preferences_register_property_custom:
 * @pr: a #AnjutaPreferences object.
 * @object: Object to register.
 * @key: Property key.
 * @default_value: Default value of the key.
 * @flags: Flags
 *
 * This is meant for complex widgets which can not be set/get with the
 * standard object set/get methods. Custom set/get methods are passed for
 * the property to set/get the value to/from the widget.
 */
gboolean
anjuta_preferences_register_property_custom (AnjutaPreferences *pr,
											 GtkWidget *object,
										     const gchar *key,
										     const gchar *default_value,
										     guint flags,
		void    (*set_property) (AnjutaProperty *prop, const gchar *value),
		gchar * (*get_property) (AnjutaProperty *));

/**
 * anjuta_preferences_reset_defaults:
 * @pr: a #AnjutaPreferences object.
 *
 * Resets the default values into the keys
 */
void anjuta_preferences_reset_defaults (AnjutaPreferences *pr);

/**
 * anjuta_preferences_save:
 * @p: a #AnjutaPreferences object.
 * @stream: File stream where the preferences will be saved.
 *
 * Save and (Loading is done in _new())
 */
gboolean anjuta_preferences_save (AnjutaPreferences * p, FILE * stream);

/* Save excluding the filtered properties. This will save only those
 * properties which DOES NOT have the flags set to values given by the filter.
 */
gboolean anjuta_preferences_save_filtered (AnjutaPreferences * p, FILE * stream,
										   AnjutaPreferencesFilterType filter);

/* Calls the callback function for each of the properties with the flags
 * matching with the given filter 
 */
void anjuta_preferences_foreach (AnjutaPreferences * pr,
								 AnjutaPreferencesFilterType filter,
								 AnjutaPreferencesCallback callback,
								 gpointer data);

/* This will transfer all the properties values from the main
properties database to the parent session properties database */
void anjuta_preferences_sync_to_session (AnjutaPreferences *pr);

/* Sets the value (string) of a key */
void anjuta_preferences_set (AnjutaPreferences *, gchar * key, gchar * value);

/* Sets the value (int) of a key */
void anjuta_preferences_set_int (AnjutaPreferences *, gchar * key, gint value);

/* Gets the value (string) of a key */
/* Must free the return string */
gchar * anjuta_preferences_get (AnjutaPreferences * p, gchar * key);

/* Gets the value (int) of a key. If not found, 0 is returned */
gint anjuta_preferences_get_int (AnjutaPreferences * p, gchar * key);

/* Gets the value (int) of a key. If not found, the default_value is returned */
gint anjuta_preferences_get_int_with_default (AnjutaPreferences * p,
											  gchar *key, gint default_value);

gchar * anjuta_preferences_default_get (AnjutaPreferences * p, gchar * key);

/* Gets the value (int) of a key */
gint anjuta_preferences_default_get_int (AnjutaPreferences * p, gchar * key);

#ifdef __cplusplus
};
#endif

#endif
