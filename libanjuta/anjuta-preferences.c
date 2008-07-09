/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * preferences.c
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

/**
 * SECTION:anjuta-preferences
 * @short_description: Anjuta Prefereces system.
 * @see_also: #AnjutaPreferencesDialog
 * @stability: Unstable
 * @include: libanjuta/anjuta-preferences.h
 * 
 * #AnjutaPreferences is a way to let plugins register their preferences. There
 * are mainly two ways a plugin could register its preferences in Anjuta.
 * 
 * First is to not use #AnjutaPreferences at all. Simply register a
 * preferences page in #AnjutaPreferencesDialog using the function
 * anjuta_preferences_dialog_add_page(). The plugin should take
 * care of loading, saving and widgets synchronization of the
 * preferences values. It is particularly useful if the plugin
 * uses gconf system for its preferences. Also no "changed"
 * signal will be emitted from it.
 * 
 * Second is to use anjuta_preferences_add_page(), which will
 * automatically register the preferences keys and values from
 * a glade xml file. The glade xml file contains a preferences
 * page of the plugin. The widget names in the page are
 * given in a particular way (see anjuta_preferences_add_page()) to
 * let it know property key details. Loading, saving and
 * widget synchronization are automatically done. "changed" signal is
 * emitted when a preference is changed.
 * 
 * anjuta_preferences_register_all_properties_from_glade_xml() only registers
 * the preferences propery keys for automatic loading, saving and widget
 * syncrhronization, but does not add the page in preferences dialog. It
 * is useful if the plugin wants to show the preferences page somewhere else.
 * 
 * anjuta_preferences_register_property_from_string() is similar to 
 * anjuta_preferences_register_all_properties_from_glade_xml(), but it only
 * registers one property, the detail of which is given in its arguments.
 * anjuta_preferences_register_property_custom() is used to register a
 * property that uses a widget which is not supported by #AnjutaPreferences.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <glade/glade-parser.h>
#include <gconf/gconf-client.h>
#include <libgnomevfs/gnome-vfs.h>

#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/resources.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>

struct _AnjutaProperty
{
	GtkWidget                *object;
	gchar                    *key;
	gchar                    *default_value;
	guint                     flags;
	gint                     notify_id;
	GConfClient              *gclient;
	
	/* Set true if custom set/get to be used */
	gboolean                  custom;
	
	/* For inbuilt generic objects */
	AnjutaPropertyObjectType  object_type;
	AnjutaPropertyDataType    data_type;
	
	/* For custom objects */
	void    (*set_property) (AnjutaProperty *prop, const gchar *value);
	gchar * (*get_property) (AnjutaProperty *prop);
	
};

struct _AnjutaPreferencesPriv
{
	GConfClient         *gclient;
	GHashTable          *properties;
	GtkWidget           *prefs_dialog;
	AnjutaPluginManager *plugin_manager;
	gboolean             is_showing;
};

/* Internal structure for anjuta_preferences_foreach */
struct _AnjutaPreferencesForeachData
{
	AnjutaPreferences *pr;
	AnjutaPreferencesFilterType filter;
	AnjutaPreferencesCallback callback;
	gpointer callback_data;
};

#define PREFERENCE_PROPERTY_PREFIX "preferences_"
#define GCONF_KEY_PREFIX "/apps/anjuta/preferences"

static const gchar *
build_key (const gchar *key)
{
	static gchar buffer[1024];
	snprintf (buffer, 1024, "%s/%s", GCONF_KEY_PREFIX, key);
	return buffer;
}

/**
 * anjuta_preferences_get:
 * @pr: A #AnjutaPreferences object
 * @key: Property key
 *
 * Gets the value of @key as string. Returned string should be g_freed() when not
 * required.
 *
 * Return value: Key value as string or NULL if the key is not defined.
 */
#ifdef __GNUC__
inline
#endif
gchar *
anjuta_preferences_get (AnjutaPreferences *pr, const gchar *key)
{
	g_return_val_if_fail (ANJUTA_IS_PREFERENCES (pr), NULL);
	g_return_val_if_fail (key != NULL, NULL);

	return gconf_client_get_string (pr->priv->gclient, build_key (key), NULL);
}

/**
 * anjuta_preferences_get_list:
 * @pr: A #AnjutaPreferences object
 * @key: Property key
 * @list_type: Type of each list element
 *
 * Gets the list of @key.
 *
 * Return value: Key list or NULL if the key is not defined.
 */
#ifdef __GNUC__
inline
#endif
GSList *
anjuta_preferences_get_list (AnjutaPreferences *pr, const gchar *key,
                             GConfValueType list_type)
{
	g_return_val_if_fail (ANJUTA_IS_PREFERENCES (pr), NULL);
	g_return_val_if_fail (key != NULL, NULL);

	return gconf_client_get_list(pr->priv->gclient, build_key (key), list_type, NULL);
}

/**
 * anjuta_preferences_get_pair:
 * @pr: A #AnjutaPreferences object
 * @key: Property key
 * @car_type: Desired type of the pair's first field (car).
 * @cdr_type: Desired type of the pair's second field (cdr).
 * @car_retloc: Address of a return location for the car.
 * @cdr_retloc: Address of a return location for the cdr.
 *
 * Gets the pair of @key.
 *
 * Return value: TRUE or FALSE.
 */
#ifdef __GNUC__
inline
#endif
gboolean
anjuta_preferences_get_pair (AnjutaPreferences *pr, const gchar *key,
                             GConfValueType car_type, GConfValueType cdr_type,
                             gpointer car_retloc, gpointer cdr_retloc)
{
	g_return_val_if_fail (ANJUTA_IS_PREFERENCES (pr), FALSE);
	g_return_val_if_fail (key != NULL, FALSE);

	return gconf_client_get_pair(pr->priv->gclient, build_key (key),
                                 car_type, cdr_type,
                                 car_retloc, cdr_retloc, NULL);
}

/**
 * anjuta_preferences_get_int:
 * @pr: A #AnjutaPreferences object
 * @key: Property key
 *
 * Gets the value of @key as integer.
 *
 * Return value: Key value as integer or 0 if the key is not defined.
 */
#ifdef __GNUC__
inline
#endif
gint
anjuta_preferences_get_int (AnjutaPreferences *pr, const gchar *key)
{
	gint ret_val;
	GConfValue *value;
	
	g_return_val_if_fail (ANJUTA_IS_PREFERENCES (pr), 0);
	g_return_val_if_fail (key != NULL, 0);
	
	ret_val = 0;
	value = gconf_client_get (pr->priv->gclient, build_key (key), NULL);
	if (value)
	{
		switch (value->type)
		{
			case GCONF_VALUE_INT:
				ret_val = gconf_value_get_int (value);
				break;
			case GCONF_VALUE_BOOL:
				ret_val = gconf_value_get_bool (value);
				break;
			default:
				g_warning ("Invalid gconf type for key: %s", key);
		}
		gconf_value_free (value);
	}
	/* else
		g_warning ("The preference key %s is not defined", key); */
	return ret_val;
}

/**
 * anjuta_preferences_get_int_with_default:
 * @pr: A #AnjutaPreferences object
 * @key: Property key
 * @default_value: Default value to return if the key is not defined.
 *
 * Gets the value of @key as integer.
 *
 * Return value: Key value as integer or @default_value if the
 * key is not defined.
 */
#ifdef __GNUC__
inline
#endif
gint
anjuta_preferences_get_int_with_default (AnjutaPreferences *pr,
										 const gchar *key, gint default_value)
{
	gint ret_val;
	GConfValue *value;
	
	g_return_val_if_fail (ANJUTA_IS_PREFERENCES (pr), 0);
	g_return_val_if_fail (key != NULL, 0);
	
	ret_val = default_value;
	value = gconf_client_get (pr->priv->gclient, build_key (key), NULL);
	if (value)
	{
		switch (value->type)
		{
			case GCONF_VALUE_INT:
				ret_val = gconf_value_get_int (value);
				break;
			case GCONF_VALUE_BOOL:
				ret_val = gconf_value_get_bool (value);
				break;
			default:
				g_warning ("Invalid gconf type for key: %s", key);
		}
		gconf_value_free (value);
	}
	return ret_val;
}

/**
 * anjuta_preferences_default_get:
 * @pr: A #AnjutaPreferences object
 * @key: Property key
 *
 * Gets the default value of @key as string. The default value of the key
 * is the value defined in System defaults (generally installed during 
 * program installation). Returned value must be g_freed() when not required.
 *
 * Return value: Default key value as string or NULL if not defined.
 */
#ifdef __GNUC__
inline
#endif
gchar *
anjuta_preferences_default_get (AnjutaPreferences * pr, const gchar * key)
{
	GConfValue *val;
	gchar *str;
	GError *err = NULL;
	
	g_return_val_if_fail (ANJUTA_IS_PREFERENCES (pr), NULL);
	g_return_val_if_fail (key != NULL, NULL);
	
	val = gconf_client_get_default_from_schema (pr->priv->gclient, build_key (key), &err);
	if (err) {
		g_error_free (err);
		return NULL;
	}
	str = g_strdup (gconf_value_get_string (val));
	gconf_value_free (val);
	return str;
}

/**
 * anjuta_preferences_default_get_int:
 * @pr: A #AnjutaPreferences object
 * @key: Property key
 *
 * Gets the default value of @key as integer. The default value of the key
 * is the value defined in System defaults (generally installed during 
 * program installation).
 *
 * Return value: Default key value as integer or 0 if the key is not defined.
 */
#ifdef __GNUC__
inline
#endif
gint
anjuta_preferences_default_get_int (AnjutaPreferences *pr, const gchar *key)
{
	GConfValue *val;
	gint ret;
	GError *err = NULL;
	
	g_return_val_if_fail (ANJUTA_IS_PREFERENCES (pr), 0);
	g_return_val_if_fail (key != NULL, 0);
	val = gconf_client_get_default_from_schema (pr->priv->gclient, build_key (key), &err);
	if (err) {
		g_error_free (err);
		return 0;
	}
	ret = gconf_value_get_int (val);
	gconf_value_free (val);
	return ret;
}

/**
 * anjuta_preferences_set:
 * @pr: A #AnjutaPreferences object.
 * @key: Property key.
 * @value: Value of the key.
 *
 * Sets the value of @key in current session.
 */
#ifdef __GNUC__
inline
#endif
void
anjuta_preferences_set (AnjutaPreferences *pr, const gchar *key,
						const gchar *value)
{
	g_return_if_fail (ANJUTA_IS_PREFERENCES (pr));
	g_return_if_fail (key != NULL);

	if (value && (strlen (value) > 0))
	{
		gconf_client_set_string (pr->priv->gclient, build_key (key), value, NULL);
	}
	else
	{
		gconf_client_set_string (pr->priv->gclient, build_key (key), "", NULL);
	}
}

/**
 * anjuta_preferences_set_list:
 * @pr: A #AnjutaPreferences object.
 * @key: Property key.
 * @list_type: Type of each element.
 * @list: New value of the key.
 *
 * Sets a list in current session.
 */
#ifdef __GNUC__
inline
#endif
void
anjuta_preferences_set_list (AnjutaPreferences *pr, const gchar *key,
					         GConfValueType list_type, GSList *list)
{
	g_return_if_fail (ANJUTA_IS_PREFERENCES (pr));
	g_return_if_fail (key != NULL);

	gconf_client_set_list(pr->priv->gclient, build_key (key), 
		                      list_type, list, NULL);
}

/**
 * anjuta_preferences_set_pair:
 * @pr: A #AnjutaPreferences object.
 * @key: Property key.
 * @car_type: Type of the pair's first field (car).
 * @cdr_type: Type of the pair's second field (cdr).
 * @address_of_car: Address of the car.
 * @address_of_cdr: Address of the cdr.
 *
 */
#ifdef __GNUC__
inline
#endif
gboolean
anjuta_preferences_set_pair (AnjutaPreferences *pr, const gchar *key,
					         GConfValueType car_type, GConfValueType cdr_type,
                             gconstpointer address_of_car,
                             gconstpointer address_of_cdr)
{
	g_return_val_if_fail (ANJUTA_IS_PREFERENCES (pr), FALSE);
	g_return_val_if_fail (key != NULL, FALSE);

	return gconf_client_set_pair (pr->priv->gclient, build_key (key),
                                  car_type, cdr_type,
                                  address_of_car, address_of_cdr,
                                  NULL);
}

/**
 * anjuta_preferences_set_int:
 * @pr: A #AnjutaPreferences object.
 * @key: Property key.
 * @value: Integer value of the key.
 *
 * Sets the value of @key in current session.
 */
#ifdef __GNUC__
inline
#endif
void
anjuta_preferences_set_int (AnjutaPreferences *pr, const gchar *key,
							gint value)
{
	GConfValue *gvalue;
	
	g_return_if_fail (ANJUTA_IS_PREFERENCES (pr));
	g_return_if_fail (key != NULL);
	
	gvalue = gconf_client_get (pr->priv->gclient, build_key (key), NULL);
	if (gvalue)
	{
		switch (gvalue->type)
		{
			case GCONF_VALUE_BOOL:
				gconf_client_set_bool (pr->priv->gclient, build_key (key),
									   value, NULL);
				break;
			case GCONF_VALUE_INT:
				gconf_client_set_int (pr->priv->gclient, build_key (key),
									  value, NULL);
				break;
			default:
				g_warning ("Invalid gconf type for key: %s", key);
		}
		gconf_value_free (gvalue);
	}
	else
	{
		/* g_warning ("The preference key %s is not defined", key); */
		gconf_client_set_int (pr->priv->gclient, build_key (key),
							  value, NULL);
	}
}

static void
property_destroy (AnjutaProperty *property)
{
	g_return_if_fail (property);
	if (property->key) g_free (property->key);
	if (property->default_value) g_free (property->default_value);
	g_object_unref (property->object);
	gconf_client_notify_remove (property->gclient, property->notify_id);
	g_free (property);
}

/**
 * anjuta_property_get_widget:
 * @prop: an #AnjutaProperty reference
 *
 * Gets the widget associated with the property.
 * 
 * Returns: a #GtkWidget object associated with the property.
 */
GtkWidget*
anjuta_property_get_widget (AnjutaProperty *prop)
{
	return prop->object;
}

static AnjutaPropertyObjectType
get_object_type_from_string (const gchar* object_type)
{
	if (strcmp (object_type, "entry") == 0)
		return ANJUTA_PROPERTY_OBJECT_TYPE_ENTRY;
	else if (strcmp (object_type, "combo") == 0)
		return ANJUTA_PROPERTY_OBJECT_TYPE_COMBO;
	else if (strcmp (object_type, "spin") == 0)
		return ANJUTA_PROPERTY_OBJECT_TYPE_SPIN;
	else if (strcmp (object_type, "toggle") == 0)
		return ANJUTA_PROPERTY_OBJECT_TYPE_TOGGLE;
	else if (strcmp (object_type, "text") == 0)
		return ANJUTA_PROPERTY_OBJECT_TYPE_TEXT;
	else if (strcmp (object_type, "color") == 0)
		return ANJUTA_PROPERTY_OBJECT_TYPE_COLOR;
	else if (strcmp (object_type, "font") == 0)
		return ANJUTA_PROPERTY_OBJECT_TYPE_FONT;
	else if (strcmp (object_type, "file") == 0)
		return ANJUTA_PROPERTY_OBJECT_TYPE_FILE;
	else if (strcmp (object_type, "folder") == 0)
		return ANJUTA_PROPERTY_OBJECT_TYPE_FOLDER;
	else
		return (AnjutaPropertyObjectType)(-1);
}

static AnjutaPropertyDataType
get_data_type_from_string (const gchar* data_type)
{
	if (strcmp (data_type, "bool") == 0)
		return ANJUTA_PROPERTY_DATA_TYPE_BOOL;
	else if (strcmp (data_type, "int") == 0)
		return ANJUTA_PROPERTY_DATA_TYPE_INT;
	else if (strcmp (data_type, "text") == 0)
		return ANJUTA_PROPERTY_DATA_TYPE_TEXT;
	else if (strcmp (data_type, "color") == 0)
		return ANJUTA_PROPERTY_DATA_TYPE_COLOR;
	else if (strcmp (data_type, "font") == 0)
		return ANJUTA_PROPERTY_DATA_TYPE_FONT;
	else
		return (AnjutaPropertyDataType)(-1);
}

static gchar*
get_property_value_as_string (AnjutaProperty *prop)
{
	gint  int_value;
	gchar** values;
	gchar *text_value = NULL;
	gchar *uri;
	
	if (prop->custom)
	{
		if (prop->get_property != NULL)
			return prop->get_property (prop);
		else
		{
			g_warning ("%s: Undefined get_property() for custom object",
					   prop->key);
			return NULL;
		}
	}
	switch (prop->object_type)
	{
	case ANJUTA_PROPERTY_OBJECT_TYPE_TOGGLE:
		int_value =
			gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prop->object));
		text_value = g_strdup_printf ("%d", int_value);
		break;
		
	case ANJUTA_PROPERTY_OBJECT_TYPE_SPIN:
		int_value =
			gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (prop->object));
		text_value = g_strdup_printf ("%d", int_value);
		break;
	
	case ANJUTA_PROPERTY_OBJECT_TYPE_ENTRY:
		text_value =
			gtk_editable_get_chars (GTK_EDITABLE (prop->object), 0, -1);
		break;
	case ANJUTA_PROPERTY_OBJECT_TYPE_COMBO:
		{
			gint idx;
			values = g_object_get_data(G_OBJECT(prop->object), "untranslated");
			idx = gtk_combo_box_get_active(GTK_COMBO_BOX(prop->object));
			if (values[idx] != NULL)
				text_value = g_strdup(values[idx]);
			break;
		}
	case ANJUTA_PROPERTY_OBJECT_TYPE_TEXT:
		{
			GtkTextBuffer *buffer;
			GtkTextIter start_iter;
			GtkTextIter end_iter;
			buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (prop->object));
			gtk_text_buffer_get_start_iter (buffer, &start_iter);
			gtk_text_buffer_get_end_iter (buffer, &end_iter);
			text_value =
				gtk_text_buffer_get_text (buffer, &start_iter, &end_iter, TRUE);
			break;
		}
	case ANJUTA_PROPERTY_OBJECT_TYPE_COLOR:
		{
			GdkColor color;
			gtk_color_button_get_color(GTK_COLOR_BUTTON (prop->object),
									   &color);
			text_value = anjuta_util_string_from_color (color.red,  color.green, color.blue);
		}
		break;
	case ANJUTA_PROPERTY_OBJECT_TYPE_FONT:
		{
			const gchar *font;
			font = gtk_font_button_get_font_name (GTK_FONT_BUTTON
													(prop->object));
			text_value = g_strdup (font);
		}
		break;
	case ANJUTA_PROPERTY_OBJECT_TYPE_FOLDER:
		text_value = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (prop->object));
		break;
	case ANJUTA_PROPERTY_OBJECT_TYPE_FILE:
		text_value = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (prop->object));
		break;
	}
	if (text_value && (strlen (text_value) == 0))
	{
		g_free (text_value);
		text_value = NULL;
	}
	return text_value;
}

static gint
get_property_value_as_int (AnjutaProperty *prop)
{
	gint  int_value;
	gchar *text_value;
	text_value = get_property_value_as_string (prop);
	int_value = atoi (text_value);
	g_free (text_value);	
	return int_value;
}

static void
set_property_value_as_string (AnjutaProperty *prop, const gchar *value)
{
	gint  int_value;
	char** values;
	gint i; 
	
	if (prop->custom)
	{
		if (prop->set_property != NULL)
		{
			prop->set_property (prop, value);
			return;
		}
		else
		{
			g_warning ("%s: Undefined set_property() for custom object",
					   prop->key);
			return;
		}
	}
	switch (prop->object_type)
	{
	case ANJUTA_PROPERTY_OBJECT_TYPE_TOGGLE:
		if (value) 
			int_value = atoi (value);
		else
			int_value = 0;
		
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prop->object),
		                              int_value);
		break;
		
	case ANJUTA_PROPERTY_OBJECT_TYPE_SPIN:
		if (value) 
			int_value = atoi (value);
		else
			int_value = 0;
		
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (prop->object), int_value);
		break;
	
	case ANJUTA_PROPERTY_OBJECT_TYPE_ENTRY:
		if (value)
			gtk_entry_set_text (GTK_ENTRY (prop->object), value);
		else
			gtk_entry_set_text (GTK_ENTRY (prop->object), "");
		break;
	case ANJUTA_PROPERTY_OBJECT_TYPE_COMBO:
		values = g_object_get_data(G_OBJECT(prop->object), "untranslated");
		if (value != NULL)
		{
			for (i=0; values[i] != NULL; i++)
			{
				if (strcmp(value, values[i]) == 0)
				{
					gtk_combo_box_set_active(GTK_COMBO_BOX(prop->object), i);
					break;
				}
			}
		}			
		break;		
	case ANJUTA_PROPERTY_OBJECT_TYPE_TEXT:
		{
			GtkTextBuffer *buffer;
			buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (prop->object));
			if (value)
				gtk_text_buffer_set_text (buffer, value, -1);
			else
				gtk_text_buffer_set_text (buffer, "", -1);
		}
		break;
		
	case ANJUTA_PROPERTY_OBJECT_TYPE_COLOR:
		{
			GdkColor color;
			
			if (value)
				anjuta_util_color_from_string (value, &color.red, &color.green, &color.blue);
				
			gtk_color_button_set_color(GTK_COLOR_BUTTON(prop->object), &color);
		}
		break;
		
	case ANJUTA_PROPERTY_OBJECT_TYPE_FONT:
		if (value)
		{
			/* If the font name is Xfont name, convert it into
			   Pango font description text -- Just take the family name :) */
			if (value[0] == '-')
			{
				/* Font is xfont name */
				gchar *font_name, *tmp;
				const gchar *end, *start;
				start = value;
				start = g_strstr_len (&start[1], strlen (&start[1]), "-");
				end = g_strstr_len (&start[1], strlen (&start[1]), "-");
				font_name = g_strndup (&start[1], end-start-1);
				tmp = font_name;
				
				/* Set font size to (arbitrary) 12 points */
				font_name = g_strconcat (tmp, " 12", NULL);
				g_free (tmp);
				
				/* DEBUG_PRINT ("Font set as: %s", font_name); */
				
				gtk_font_button_set_font_name (GTK_FONT_BUTTON
												 (prop->object), font_name);
				g_free (font_name);
			}
			else
			{				
				gtk_font_button_set_font_name (GTK_FONT_BUTTON
												 (prop->object), value);
			}
		}
		/* FIXME: Set a standard font as default.
		else
		{
			gnome_font_picker_set_font_name (GNOME_FONT_PICKER (prop->object),
											 "A standard font");
		}*/
		break;
		
	case ANJUTA_PROPERTY_OBJECT_TYPE_FOLDER:
		if (value)
		{
			gchar *old_folder;

			/* When the user change the folder, the
			 * current-folder-changed signal is emitted the
			 * gconf key is updated and this function is called.
			 * But setting the current folder here emits
			 * the current-folder-changed signal too.
			 * Moreover this signal is emitted asynchronously so
			 * it is not possible to block it here.
			 * 
			 * The solution here is to update the widget only
			 * if it is really needed.
			 */
			old_folder = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (prop->object));
			if ((old_folder == NULL) || strcmp (old_folder, value))
			{
				gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (prop->object), value);
			}
			g_free (old_folder);
		}
		break;
	case ANJUTA_PROPERTY_OBJECT_TYPE_FILE:
		if (value)
		{
			gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (prop->object),
															 value);
		}
		break;
	}
}

static void
set_property_value_as_int (AnjutaProperty *prop, gint value)
{
	gchar *text_value;
	text_value = g_strdup_printf ("%d", value);
	set_property_value_as_string (prop, text_value);
	g_free (text_value);	
}

static gboolean
update_property_on_event_str (GtkWidget *widget, GdkEvent *event,
							  gpointer user_data)
{
	AnjutaPreferences *pr;
	AnjutaProperty *p;
	gchar *val;
	
	pr = ANJUTA_PREFERENCES (g_object_get_data (G_OBJECT (widget),
												"AnjutaPreferences"));
	p = (AnjutaProperty *) user_data;
	val = get_property_value_as_string (p);
	anjuta_preferences_set (pr, p->key, val);
	g_free (val);
	return FALSE;
}

static void
update_property_on_change_str (GtkWidget *widget, gpointer user_data)
{
	AnjutaPreferences *pr;
	AnjutaProperty *p;
	gchar *val;

	pr = ANJUTA_PREFERENCES (g_object_get_data (G_OBJECT (widget),
												"AnjutaPreferences"));
	p = (AnjutaProperty *) user_data;
	val = get_property_value_as_string (p);
	anjuta_preferences_set (pr, p->key, val);
	g_free (val);
}

static gboolean
block_update_property_on_change_str (GtkWidget *widget, GdkEvent *event,
							  gpointer user_data)
{
	AnjutaProperty *p = (AnjutaProperty *) user_data;

	gtk_signal_handler_block_by_func (GTK_OBJECT(p->object), G_CALLBACK (update_property_on_change_str), p);
	return FALSE;
}

static gboolean
unblock_update_property_on_change_str (GtkWidget *widget, GdkEvent *event,
							  gpointer user_data)
{
	AnjutaProperty *p = (AnjutaProperty *) user_data;

	gtk_signal_handler_unblock_by_func (GTK_OBJECT(p->object), G_CALLBACK (update_property_on_change_str), p);
	return FALSE;
}

static void
update_property_on_change_int (GtkWidget *widget, gpointer user_data)
{
	AnjutaPreferences *pr;
	AnjutaProperty *p;
	gint val;
	
	pr = ANJUTA_PREFERENCES (g_object_get_data (G_OBJECT (widget),
												"AnjutaPreferences"));
	p = (AnjutaProperty *) user_data;
	val = get_property_value_as_int (p);
	anjuta_preferences_set_int (pr, p->key, val);
}

static void
update_property_on_change_color (GtkWidget *widget, gpointer user_data)
{
	AnjutaPreferences *pr;
	AnjutaProperty *p;
	gchar *val;
	
	pr = ANJUTA_PREFERENCES (g_object_get_data (G_OBJECT (widget),
												"AnjutaPreferences"));
	p = (AnjutaProperty *) user_data;
	val = get_property_value_as_string (p);
	anjuta_preferences_set (pr, p->key, val);
	g_free (val);
}

static void
update_property_on_change_font (GtkWidget *widget,
								gpointer user_data)
{
	AnjutaPreferences *pr;
	AnjutaProperty *p;
	gchar *val;
	
	pr = ANJUTA_PREFERENCES (g_object_get_data (G_OBJECT (widget),
												"AnjutaPreferences"));
	p = (AnjutaProperty *) user_data;
	val = get_property_value_as_string (p);
	anjuta_preferences_set (pr, p->key, val);
	g_free (val);
}

static void
unregister_preferences_key (GtkWidget *widget,
							gpointer user_data)
{
	AnjutaProperty *p;
	AnjutaPreferences *pr;
	gchar *key;
	
	p = (AnjutaProperty *) user_data;
	pr = g_object_get_data (G_OBJECT (widget),
							"AnjutaPreferences");
	key = g_strdup (p->key);
	
	g_hash_table_remove (pr->priv->properties, key);
	g_free (key);
}

static void
get_property (GConfClient *gclient, guint cnxt_id,
			  GConfEntry *entry, gpointer user_data)
{
	const gchar *key;
	GConfValue *value;
	
	AnjutaProperty *p = (AnjutaProperty *) user_data;
	key = gconf_entry_get_key (entry);
	value = gconf_entry_get_value (entry);
	/* DEBUG_PRINT ("Preference changed %s", key); */

	if (p->data_type == ANJUTA_PROPERTY_DATA_TYPE_BOOL)
	{
		gboolean gconf_value;
		
		gconf_value = gconf_value_get_bool (value);
		set_property_value_as_int (p, gconf_value);
	}
	else if (p->data_type == ANJUTA_PROPERTY_DATA_TYPE_INT)
	{
		int gconf_value;
		
		gconf_value = gconf_value_get_int (value);
		set_property_value_as_int (p, gconf_value);
	}
	else
	{
		const gchar *gconf_value;
		gconf_value = gconf_value_get_string (value);
		set_property_value_as_string (p, gconf_value);
	}
}

static void
register_callbacks (AnjutaPreferences *pr, AnjutaProperty *p)
{
	GConfClient *gclient;
	gchar *key_error_msg;
	
	gclient = pr->priv->gclient;
	g_object_set_data (G_OBJECT (p->object), "AnjutaPreferences", pr);
	switch (p->object_type) {
		case ANJUTA_PROPERTY_OBJECT_TYPE_ENTRY:
			g_signal_connect (G_OBJECT(p->object), "changed",
							  G_CALLBACK (update_property_on_change_str), p);
			g_signal_connect (G_OBJECT(p->object), "focus_out_event",
							  G_CALLBACK (update_property_on_event_str), p);
			g_signal_connect (G_OBJECT(p->object), "focus_out_event",
							  G_CALLBACK (unblock_update_property_on_change_str), p);
			g_signal_connect (G_OBJECT(p->object), "focus_in_event",
							  G_CALLBACK (block_update_property_on_change_str), p);
			break;
		case ANJUTA_PROPERTY_OBJECT_TYPE_SPIN:
			g_signal_connect (G_OBJECT(p->object), "value-changed",
							  G_CALLBACK (update_property_on_change_int), p);
			break;
		case ANJUTA_PROPERTY_OBJECT_TYPE_FONT:
			g_signal_connect (G_OBJECT(p->object), "font-set",
							  G_CALLBACK (update_property_on_change_font), p);
			break;
		case ANJUTA_PROPERTY_OBJECT_TYPE_TEXT:
			g_signal_connect (G_OBJECT(p->object), "focus_out_event",
							  G_CALLBACK (update_property_on_event_str), p);
			break;
		case ANJUTA_PROPERTY_OBJECT_TYPE_COMBO:
			g_signal_connect (G_OBJECT(p->object), "changed",
							  G_CALLBACK (update_property_on_change_str), p);
			break;
		case ANJUTA_PROPERTY_OBJECT_TYPE_TOGGLE:
			g_signal_connect (G_OBJECT(p->object), "toggled",
							  G_CALLBACK (update_property_on_change_int), p);
			break;
		case ANJUTA_PROPERTY_OBJECT_TYPE_COLOR:
			g_signal_connect (G_OBJECT(p->object), "color-set",
							  G_CALLBACK (update_property_on_change_color), p);
			break;
		case ANJUTA_PROPERTY_OBJECT_TYPE_FILE:
			g_signal_connect (G_OBJECT(p->object), "file-set",
							  G_CALLBACK (update_property_on_change_str), p);
			break;
		case ANJUTA_PROPERTY_OBJECT_TYPE_FOLDER:
			g_signal_connect (G_OBJECT(p->object), "current-folder-changed",
							  G_CALLBACK (update_property_on_change_str), p);
			break;
		default:
			break;
	}
	if (!gconf_valid_key (build_key (p->key), &key_error_msg))
	{
		g_warning ("Invalid key \"%s\": Error: \"%s\"", build_key (p->key),
				   key_error_msg);
		g_free (key_error_msg);
	}
	p->notify_id = gconf_client_notify_add (gclient, build_key (p->key),
											get_property, p, NULL, NULL);
	
	/* Connect to widget destroy signal so we can automatically unregister
	 * keys so there aren't any potential conflicts or references to 
	 * nonexistent widgets on subsequent uses of the prefs dialog. */
	g_signal_connect (G_OBJECT (p->object), "destroy",
					  G_CALLBACK (unregister_preferences_key),
					  p);
}

static gboolean
preferences_foreach_callback (gchar *key, struct _AnjutaProperty *p, 
							  struct _AnjutaPreferencesForeachData *data)
{	
	if (p->object_type != ANJUTA_PROPERTY_OBJECT_TYPE_COMBO)
	{
		if (data->filter == ANJUTA_PREFERENCES_FILTER_NONE)
			return data->callback (data->pr, key, data->callback_data);
		else if (p->flags & data->filter)
			return data->callback (data->pr, key, data->callback_data);
	}
	
	return TRUE;
}

static void
connect_prop_to_object (AnjutaPreferences *pr, AnjutaProperty *p)
{
	int gconf_value;
	gchar *value;
	
	if (p->data_type == ANJUTA_PROPERTY_DATA_TYPE_BOOL ||
		p->data_type == ANJUTA_PROPERTY_DATA_TYPE_INT)
	{	
		gconf_value = anjuta_preferences_get_int (pr, p->key);
		value = g_strdup_printf ("%d", gconf_value);
		set_property_value_as_string (p, value);
	} 
	else 
	{
		value = anjuta_preferences_get (pr, p->key);
		set_property_value_as_string (p, value);
		g_free (value);
	}
}


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
 * 
 * Return value: TRUE if sucessful.
 */
gboolean
anjuta_preferences_register_property_raw (AnjutaPreferences *pr,
										  GtkWidget *object,
										  const gchar *key,
										  const gchar *default_value,
										  guint flags,
										  AnjutaPropertyObjectType object_type,
										  AnjutaPropertyDataType  data_type)
{
	AnjutaProperty *p;
	GConfValue *value;
	
	g_return_val_if_fail (ANJUTA_IS_PREFERENCES (pr), FALSE);
	g_return_val_if_fail (GTK_IS_WIDGET (object), FALSE);
	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (strlen(key) > 0, FALSE);
	g_return_val_if_fail ((object_type != ANJUTA_PROPERTY_OBJECT_TYPE_COMBO) ||
				((default_value != NULL) &&
				 (*default_value != '\0')), FALSE);

	p = g_new0 (AnjutaProperty, 1);
	g_object_ref (object);
	p->object = object;
	p->object_type = object_type;
	p->data_type = data_type;
	p->key = g_strdup (key);
	p->gclient = pr->priv->gclient;
	
	value = gconf_client_get (pr->priv->gclient,
							  build_key (p->key), NULL);
	if (value)
	{
		/* Verify key type. Unset key if type mismatch. */
		if (!((value->type == GCONF_VALUE_BOOL &&
			   data_type == ANJUTA_PROPERTY_DATA_TYPE_BOOL) ||
			  (value->type == GCONF_VALUE_INT &&
			   data_type == ANJUTA_PROPERTY_DATA_TYPE_INT) ||
			  (value->type == GCONF_VALUE_STRING &&
			   data_type != ANJUTA_PROPERTY_DATA_TYPE_BOOL &&
			   data_type != ANJUTA_PROPERTY_DATA_TYPE_INT)))
		{
			gconf_client_unset (pr->priv->gclient, build_key (key), NULL);
		}
		gconf_value_free (value);
	}
	if (default_value)
	{
		p->default_value = g_strdup (default_value);
		if (strlen (default_value) > 0)
		{
			/* For combo, initialize the untranslated strings */
			if (object_type == ANJUTA_PROPERTY_OBJECT_TYPE_COMBO) 
			{
				gchar *old_value;
				gchar **vstr;
				
				vstr = g_strsplit (default_value, ",", 100);
				g_object_set_data(G_OBJECT(p->object), "untranslated",
									vstr);
				old_value = anjuta_preferences_get (pr, p->key);
				if (old_value == NULL && vstr[0])
				{
					/* DEBUG_PRINT ("Setting default pref value: %s = %s",
								 p->key, default_value); */
					anjuta_preferences_set (pr, p->key, vstr[0]);
				}
				if (old_value)
					g_free (old_value);
			}
			else if (p->data_type != ANJUTA_PROPERTY_DATA_TYPE_BOOL &&
					 p->data_type != ANJUTA_PROPERTY_DATA_TYPE_INT)
			{
				gchar *old_value;
				old_value = anjuta_preferences_get (pr, p->key);
				if (old_value == NULL)
				{
					/* DEBUG_PRINT ("Setting default pref value: %s = %s",
								 p->key, default_value);*/
					anjuta_preferences_set (pr, p->key, default_value);
				}
				if (old_value)
					g_free (old_value);
			}
			else
			{
				value = gconf_client_get (pr->priv->gclient,
										  build_key (p->key), NULL);
				if (value == NULL)
				{
					/* DEBUG_PRINT ("Setting default pref value: %s = %s",
								 p->key, default_value);*/
					if (p->data_type == ANJUTA_PROPERTY_DATA_TYPE_INT)
						gconf_client_set_int (pr->priv->gclient,
											  build_key (p->key),
											  atoi (default_value), NULL);
					else
						gconf_client_set_bool (pr->priv->gclient,
											   build_key (p->key),
											   atoi (default_value), NULL);
				}
				if (value)
					gconf_value_free (value);
			}
		}
	}
	p->flags = flags;
	p->custom = FALSE;
	p->set_property = NULL;
	p->get_property = NULL;
	
	g_hash_table_insert (pr->priv->properties, g_strdup (key), p);
	connect_prop_to_object (pr, p);
	register_callbacks (pr, p);
	return TRUE;
}

/**
 * anjuta_preferences_register_property_custom:
 * @pr: a #AnjutaPreferences object.
 * @object: Object to register.
 * @key: Property key.
 * @default_value: Default value of the key.
 * @data_type: property data type.
 * @flags: Flags
 * @set_property: Set property to widget callback.
 * @get_property: Get property from widget callback.
 * 
 * This is meant for complex widgets which can not be set/get with the
 * standard object set/get methods. Custom set/get methods are passed for
 * the property to set/get the value to/from the widget.
 * 
 * Return value: TRUE if sucessful.
 */
gboolean
anjuta_preferences_register_property_custom (AnjutaPreferences *pr,
											 GtkWidget *object,
										     const gchar *key,
										     const gchar *default_value,
											 AnjutaPropertyDataType data_type,
										     guint flags,
		void    (*set_property) (AnjutaProperty *prop, const gchar *value),
		gchar * (*get_property) (AnjutaProperty *))
{
	AnjutaProperty *p;
	GConfValue *value;
	
	g_return_val_if_fail (ANJUTA_IS_PREFERENCES (pr), FALSE);
	g_return_val_if_fail (GTK_IS_WIDGET (object), FALSE);
	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (strlen(key) > 0, FALSE);
	
	p = g_new0 (AnjutaProperty, 1);
	g_object_ref (object);
	p->object = object;
	p->object_type = (AnjutaPropertyObjectType) 0;
	p->data_type = data_type;
	p->key = g_strdup (key);
	p->gclient = pr->priv->gclient;
	
	value = gconf_client_get (pr->priv->gclient,
							  build_key (p->key), NULL);
	if (value)
	{
		/* Verify key type. Unset key if type mismatch. */
		if (!((value->type == GCONF_VALUE_BOOL &&
			   data_type == ANJUTA_PROPERTY_DATA_TYPE_BOOL) ||
			  (value->type == GCONF_VALUE_INT &&
			   data_type == ANJUTA_PROPERTY_DATA_TYPE_INT) ||
			  (value->type == GCONF_VALUE_STRING &&
			   data_type != ANJUTA_PROPERTY_DATA_TYPE_BOOL &&
			   data_type != ANJUTA_PROPERTY_DATA_TYPE_INT)))
		{
			gconf_client_unset (pr->priv->gclient, build_key (key), NULL);
		}
		gconf_value_free (value);
	}
	if (default_value)
	{
		p->default_value = g_strdup (default_value);
		if (p->data_type != ANJUTA_PROPERTY_DATA_TYPE_BOOL &&
			p->data_type != ANJUTA_PROPERTY_DATA_TYPE_INT)
		{
			gchar *old_value;
			old_value = anjuta_preferences_get (pr, p->key);
			if (old_value == NULL)
			{
				/* DEBUG_PRINT ("Setting default pref value: %s = %s",
							 p->key, default_value); */
				anjuta_preferences_set (pr, p->key, default_value);
			}
			if (old_value)
				g_free (old_value);
		}
		else
		{
			value = gconf_client_get (pr->priv->gclient,
									  build_key (p->key), NULL);
			if (value == NULL)
			{
				/* DEBUG_PRINT ("Setting default pref value: %s = %s",
							 p->key, default_value);*/
				if (p->data_type == ANJUTA_PROPERTY_DATA_TYPE_INT)
					gconf_client_set_int (pr->priv->gclient,
										  build_key (p->key),
										  atoi (default_value), NULL);
				else
					gconf_client_set_bool (pr->priv->gclient,
										   build_key (p->key),
										   atoi (default_value), NULL);
			}
			if (value)
				gconf_value_free (value);
		}
	}
	p->custom = TRUE;
	p->flags = flags;
	p->set_property = set_property;
	p->get_property = get_property;

	g_hash_table_insert (pr->priv->properties, g_strdup (key), p);

	/* Connect to widget destroy signal so we can automatically unregister
	 * keys so there aren't any potential conflicts or references to 
	 * nonexistent widgets on subsequent uses of the prefs dialog. */
	g_object_set_data (G_OBJECT (p->object), "AnjutaPreferences", pr);
	g_signal_connect (G_OBJECT (p->object), "destroy",
					  G_CALLBACK (unregister_preferences_key),
					  p);
	return TRUE;
}

/**
 * anjuta_preferences_register_property_from_string:
 * @pr: a #AnjutaPreferences object
 * @object: Widget to register
 * @property_desc: Property description (see anjuta_preferences_add_page())
 *
 * This registers only one widget. The widget could be shown elsewhere.
 * the property_description should be of the form described before.
 * 
 * Return value: TRUE if sucessful.
 */
gboolean
anjuta_preferences_register_property_from_string (AnjutaPreferences *pr,
												  GtkWidget *object,
												  const gchar *property_desc)
{
	gchar **fields;
	gint  n_fields;
	
	AnjutaPropertyObjectType object_type;
	AnjutaPropertyDataType data_type;
	gchar *key;
	gchar *default_value;
	gint flags;
	
	g_return_val_if_fail (ANJUTA_IS_PREFERENCES(pr), FALSE);
	g_return_val_if_fail ((GTK_IS_WIDGET (object)), FALSE);
	g_return_val_if_fail (property_desc != NULL, FALSE);
	
	fields = g_strsplit (property_desc, ":", 5);
	g_return_val_if_fail (fields, FALSE);
	for (n_fields = 0; fields[n_fields]; n_fields++);
	if (n_fields != 5)
	{
		g_strfreev (fields);
		return FALSE;
	}
	object_type = get_object_type_from_string (fields[0]);
	data_type = get_data_type_from_string (fields[1]);
	default_value = fields[2];
	flags = atoi (fields[3]);
	key = fields[4];
	if (object_type < 0)
	{
		g_warning ("Invalid property object type in property description");
		g_strfreev (fields);
		return FALSE;
	}
	if (data_type < 0)
	{
		g_warning ("Invalid property data type in property description");
		g_strfreev (fields);
		return FALSE;
	}
	anjuta_preferences_register_property_raw (pr, object, key, default_value,
											  flags,  object_type,
											  data_type);
	g_strfreev (fields);
	return TRUE;
}

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
void
anjuta_preferences_register_all_properties_from_glade_xml (AnjutaPreferences *pr,
														   GladeXML *gxml,
														   GtkWidget *parent)
{
	GList *widgets;
	GList *node;
	
	g_return_if_fail (ANJUTA_IS_PREFERENCES (pr));
	g_return_if_fail (gxml != NULL);
	
	widgets = glade_xml_get_widget_prefix (gxml, "preferences_");
	node = widgets;
	while (node)
	{
		const gchar *name;
		GtkWidget *widget, *p;
		gboolean cont_flag = FALSE;
		
		widget = node->data;
		
		p = gtk_widget_get_parent (widget);
		/* Added only if it's a desendend child of the parent */
		while (p != parent)
		{
			if (p == NULL)
			{
				cont_flag = TRUE;
				break;
			}
			p = gtk_widget_get_parent (p);
		}
		if (cont_flag == TRUE)
		{
			node = g_list_next (node);
			continue;
		}
		
		name = glade_get_widget_name (widget);
		if (strncmp (name, PREFERENCE_PROPERTY_PREFIX,
                     strlen (PREFERENCE_PROPERTY_PREFIX)) == 0)
		{
			const gchar *property = &name[strlen (PREFERENCE_PROPERTY_PREFIX)];
			anjuta_preferences_register_property_from_string (pr, widget,
															  property);
		}
		node = g_list_next (node);
	}
}

/**
 * anjuta_preferences_reset_defaults:
 * @pr: a #AnjutaPreferences object.
 *
 * Resets the default values into the keys
 */
void
anjuta_preferences_reset_defaults (AnjutaPreferences * pr)
{
	GtkWidget *dlg;
	
	g_return_if_fail (ANJUTA_IS_PREFERENCES (pr));
	
	dlg = gtk_message_dialog_new (GTK_WINDOW (pr),
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION,
			GTK_BUTTONS_NONE,
		     _("Are you sure you want to reset the preferences to\n"
		       "their default settings?"));
	gtk_dialog_add_button (GTK_DIALOG (dlg), GTK_STOCK_CANCEL,
						   GTK_RESPONSE_CANCEL);
	anjuta_util_dialog_add_button (GTK_DIALOG (dlg), _("_Reset"),
								   GTK_STOCK_REVERT_TO_SAVED,
								   GTK_RESPONSE_YES);
	if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_YES)
	{
		/* FIXME: Reset preferences to built-in default values. */
	}
	gtk_widget_destroy (dlg);
}

/**
 * anjuta_preferences_foreach:
 * @pr: A #AnjutaPreferences object.
 * @filter: Keys to filter out from the loop.
 * @callback: User callback function.
 * @data: User data passed to @callback
 *
 * Calls @callback function for each of the registered property keys. Keys
 * with matching @filter flags are left out of the loop. If @filter is
 * ANJUTA_PREFERENCES_FILTER_NONE, all properties are selected for the loop.
 */
void
anjuta_preferences_foreach (AnjutaPreferences *pr,
							AnjutaPreferencesFilterType filter,
							AnjutaPreferencesCallback callback,
							gpointer data)
{
	struct _AnjutaPreferencesForeachData foreach_data;
	
	foreach_data.pr = pr;
	foreach_data.filter = filter;
	foreach_data.callback = callback;
	foreach_data.callback_data = data;
	
	g_hash_table_find (pr->priv->properties, 
					   (GHRFunc) preferences_foreach_callback,
					   &foreach_data);

}

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
 *
 * <programlisting>
 *     preferences_OBJECTTYPE:DATATYPE:DEFAULT:FLAGS:PROPERTYKEY
 *     where,
 *       OBJECTTYPE is 'toggle', 'spin', 'entry', 'text', 'color', 'font' or 'file' .
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
 * </programlisting>
 *
 * All widgets having the above names in the gxml tree will be registered
 * and will become part of auto saving/loading. For example, refer to
 * anjuta preferences dialogs and study the widget names.
 */
void
anjuta_preferences_add_page (AnjutaPreferences* pr, GladeXML *gxml,
							 const gchar* glade_widget_name,
							 const gchar* title,
							 const gchar *icon_filename)
{
	GtkWidget *parent;
	GtkWidget *page;
	GdkPixbuf *pixbuf;
	gchar *image_path;
	
	g_return_if_fail (ANJUTA_IS_PREFERENCES (pr));
	g_return_if_fail (glade_widget_name != NULL);
	g_return_if_fail (icon_filename != NULL);
	
	page = glade_xml_get_widget (gxml, glade_widget_name);
	g_object_ref (page);
	g_return_if_fail (GTK_IS_WIDGET (page));
	parent = gtk_widget_get_parent (page);
	if (parent && GTK_IS_CONTAINER (parent))
	{
		if (GTK_IS_NOTEBOOK (parent))
		{
			gint page_num;
			
			page_num = gtk_notebook_page_num (GTK_NOTEBOOK (parent), page);
			gtk_notebook_remove_page (GTK_NOTEBOOK (parent), page_num);
		}
		else
		{
			gtk_container_remove (GTK_CONTAINER (parent), page);
		}
	}
	image_path = anjuta_res_get_pixmap_file (icon_filename);
	pixbuf = gdk_pixbuf_new_from_file (image_path, NULL);
	anjuta_preferences_dialog_add_page (ANJUTA_PREFERENCES_DIALOG (pr->priv->prefs_dialog),
										glade_widget_name, title, pixbuf, page);
	anjuta_preferences_register_all_properties_from_glade_xml (pr, gxml, page);
	g_object_unref (page);
	g_free (image_path);
	g_object_unref (pixbuf);
}

void
anjuta_preferences_remove_page (AnjutaPreferences *pr,
								const gchar *page_name)
{
	if (pr->priv->prefs_dialog)
	{
		anjuta_preferences_dialog_remove_page (ANJUTA_PREFERENCES_DIALOG (pr->priv->prefs_dialog),
											   page_name);
	}
}

static void 
on_preferences_dialog_destroyed (GtkWidget *preferencess_dialog,
							     AnjutaPreferences *pr)
{
	GList *plugins;
	GList *current_plugin;
	
	plugins = anjuta_plugin_manager_get_active_plugin_objects (pr->priv->plugin_manager);
	current_plugin = plugins;
	
	while (current_plugin)
	{
		if (IANJUTA_IS_PREFERENCES (current_plugin->data))
		{
			ianjuta_preferences_unmerge (IANJUTA_PREFERENCES (current_plugin->data),
								  		 pr, NULL);
		}
			
		current_plugin = g_list_next (current_plugin);
	
	}
	
	g_object_unref (pr->priv->prefs_dialog);
	
	g_list_free (plugins);
	pr->priv->prefs_dialog = NULL;
}

GtkWidget *
anjuta_preferences_get_dialog (AnjutaPreferences *pr)
{
	GList *plugins;
	GList *current_plugin;
	
	if (pr->priv->prefs_dialog)
		return pr->priv->prefs_dialog;
	else
	{
		pr->priv->prefs_dialog = anjuta_preferences_dialog_new ();
		
		g_signal_connect (G_OBJECT (pr->priv->prefs_dialog), "destroy",
						  G_CALLBACK (on_preferences_dialog_destroyed),
						  pr);

		plugins = anjuta_plugin_manager_get_active_plugin_objects (pr->priv->plugin_manager);
		current_plugin = plugins;
		
		while (current_plugin)
		{
			if (IANJUTA_IS_PREFERENCES (current_plugin->data))
			{
				ianjuta_preferences_merge (IANJUTA_PREFERENCES (current_plugin->data),
								  		   pr, NULL);
			}
			
			current_plugin = g_list_next (current_plugin);
		}
	
		g_list_free (plugins);
		
		return g_object_ref_sink (pr->priv->prefs_dialog);
	}
}

gboolean
anjuta_preferences_is_dialog_created (AnjutaPreferences *pr)
{
	return (pr->priv->prefs_dialog != NULL);
}

static void anjuta_preferences_class_init    (AnjutaPreferencesClass *class);
static void anjuta_preferences_instance_init (AnjutaPreferences      *pr);

GType
anjuta_preferences_get_type ()
{
	static GType obj_type = 0;
	
	if (!obj_type)
	{
		static const GTypeInfo obj_info = 
		{
			sizeof (AnjutaPreferencesClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) anjuta_preferences_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,           /* class_data */
			sizeof (AnjutaPreferencesClass),
			0,              /* n_preallocs */
			(GInstanceInitFunc) anjuta_preferences_instance_init,
			NULL            /* value_table */
		};
		obj_type = g_type_register_static (G_TYPE_OBJECT,
		                                   "AnjutaPreferences", &obj_info, 0);
	}
	return obj_type;
}

static void
anjuta_preferences_dispose (GObject *obj)
{
	AnjutaPreferences *pr = ANJUTA_PREFERENCES (obj);
	
	if (pr->priv->properties)
	{
		/* This will release the refs on property objects */
		g_hash_table_destroy (pr->priv->properties);
		pr->priv->properties = NULL;
	}
}

static void
anjuta_preferences_instance_init (AnjutaPreferences *pr)
{
	pr->priv = g_new0 (AnjutaPreferencesPriv, 1);
	
	pr->priv->properties = g_hash_table_new_full (g_str_hash, g_str_equal,
												  g_free, 
												  (GDestroyNotify) property_destroy);
	
	pr->priv->gclient = gconf_client_get_default();
	gconf_client_add_dir (pr->priv->gclient, GCONF_KEY_PREFIX,
						  GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);

}

static void
anjuta_preferences_finalize (GObject *obj)
{
	AnjutaPreferences *pr = ANJUTA_PREFERENCES (obj);
	
	if (pr->priv->prefs_dialog)
		gtk_widget_destroy (pr->priv->prefs_dialog);
	
	g_object_unref (pr->priv->plugin_manager);
	g_free (pr->priv);
}

static void
anjuta_preferences_class_init (AnjutaPreferencesClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	
	object_class->dispose = anjuta_preferences_dispose;
	object_class->finalize = anjuta_preferences_finalize;
}

/**
 * anjuta_preferences_new:
 * 
 * Creates a new #AnjutaPreferences object
 * 
 * Return value: A #AnjutaPreferences object.
 */
AnjutaPreferences *
anjuta_preferences_new (AnjutaPluginManager *plugin_manager)
{
	AnjutaPreferences *pr;
	
	pr = g_object_new (ANJUTA_TYPE_PREFERENCES, NULL);
	pr->priv->plugin_manager = g_object_ref (plugin_manager);
	
	return pr;
	
}

/**
 * anjuta_preferences_notify_add:
 * @pr: A #AnjutaPreferences object.
 * @key: Key to monitor.
 * @func: User callback function.
 * @data: User data passed to @func
 * @destroy_notify: Destroy notify function - called when notify is removed.
 *
 * This is similar to gconf_client_notify_add(), except that the key is not
 * given as full path. Only anjuta preference key is given. The key prefix
 * is added internally.
 *
 * Return value: Notify ID.
 */
guint
anjuta_preferences_notify_add (AnjutaPreferences *pr,
							   const gchar *key,
							   GConfClientNotifyFunc func,
							   gpointer data,
							   GFreeFunc destroy_notify)
{
	return gconf_client_notify_add (pr->priv->gclient,
									build_key (key),
									func, data, destroy_notify, NULL);
}

/**
 * anjuta_preferences_notify_remove:
 * @pr: A #AnjutaPreferences object.
 * @notify_id: Notify ID returned by anjuta_preferences_notify_add().
 *
 * Removes the notify callback added with anjuta_preferences_notify_add().
 */
void
anjuta_preferences_notify_remove (AnjutaPreferences *pr, guint notify_id)
{
	gconf_client_notify_remove (pr->priv->gclient, notify_id);
}

/**
 * anjuta_preferences_get_prefix:
 * @pr: A #AnjutaPreferences object.
 *
 * Returns the gconf key prefix used by anjuta to store its preferences.
 *
 * Return value: preferences keys prefix.
 */
const gchar*
anjuta_preferences_get_prefix (AnjutaPreferences *pr)
{
	return PREFERENCE_PROPERTY_PREFIX;
}

/**
 * anjuta_preferences_dir_exists:
 * @pr: A #AnjutaPreferences object.
 * @dir: Directory to checkfor.
 *
 * Returns TRUE if dir exists.
 */
#ifdef __GNUC__
inline
#endif
gboolean
anjuta_preferences_dir_exists (AnjutaPreferences *pr, const gchar *dir)
{
	g_return_val_if_fail (ANJUTA_IS_PREFERENCES (pr), FALSE);
	g_return_val_if_fail (dir != NULL, FALSE);

	return gconf_client_dir_exists(pr->priv->gclient, build_key (dir), NULL);
}

/**
 * anjuta_preferences_add_dir:
 * @pr: A #AnjutaPreferences object.
 * @dir: Directory to add to the list.
 * @preload: Degree of preload.
 *
 * Add a directory to the list of directories the GConfClient.
 */
#ifdef __GNUC__
inline
#endif
void
anjuta_preferences_add_dir (AnjutaPreferences *pr, const gchar *dir, 
                               GConfClientPreloadType preload)
{
	g_return_if_fail (ANJUTA_IS_PREFERENCES (pr));
	g_return_if_fail (dir != NULL);

	gconf_client_add_dir(pr->priv->gclient, build_key (dir), 
	                     preload, NULL);
}

/**
 * anjuta_preferences_remove_dir:
 * @pr: A #AnjutaPreferences object.
 * @dir: Directory to remove from the list.
 *
 * Remove a directory from the list of directories.
 */
#ifdef __GNUC__
inline
#endif
void
anjuta_preferences_remove_dir (AnjutaPreferences *pr, const gchar *dir)
{
	g_return_if_fail (ANJUTA_IS_PREFERENCES (pr));
	g_return_if_fail (dir != NULL);

	gconf_client_remove_dir(pr->priv->gclient, build_key (dir), NULL);
}
