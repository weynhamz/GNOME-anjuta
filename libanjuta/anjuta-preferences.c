/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * preferences.c
 * Copyright (C) 2000 - 2003  Naba Kumar <naba@gnome.org>
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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gnome.h>
#include <glade/glade-parser.h>

// #include <style-editor.h>

#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/resources.h>
#include <libanjuta/defaults.h>
#include <libanjuta/pixmaps.h>
#include <libanjuta/properties.h>

struct _AnjutaProperty
{
	GtkWidget                *object;
	gchar                    *key;
	gchar                    *default_value;
	guint                     flags;
	
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
	GList     *properties;
	gboolean   is_showing;
};

/** 
 * AnjutaPreferences::changed:
 * @pref: The #AnjutaPreferences object which emitted the signal.
 * 
 * This signal is emitted when preferences is changed. Note that for
 * now, it is only emitted when 'Apply' is clicked on the preferences
 * dialog. In future, it is meant to be emitted when any set value
 * is called on a preference key (and if value is different from the
 *	old value.
 */
enum
{
	CHANGED,
	SIGNALS_END
};

static guint anjuta_preferences_signals[SIGNALS_END] = { 0 };

#define PREFERENCE_PROPERTY_PREFIX "preferences_"

static void
property_destroy (AnjutaProperty *property)
{
	g_return_if_fail (property);
	if (property->key) g_free (property->key);
	if (property->default_value) g_free (property->key);
	g_object_unref (G_OBJECT (property->object));
	g_free (property);
}

/* Get functions. Add more get functions for AnjutaProperty, if required */
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
	else if (strcmp (object_type, "menu") == 0)
		return ANJUTA_PROPERTY_OBJECT_TYPE_MENU;
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
	gchar *text_value;
	gchar** values;
	
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
	case ANJUTA_PROPERTY_OBJECT_TYPE_MENU:
		{
			gint idx;
			values = g_object_get_data(G_OBJECT(prop->object), "untranslated");
			idx = gtk_option_menu_get_history(GTK_OPTION_MENU(prop->object));
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
			guint8 r, g, b, a;
			gnome_color_picker_get_i8 (GNOME_COLOR_PICKER (prop->object),
									   &r, &g, &b, &a);
			text_value = anjuta_util_string_from_color (r, g, b);
		}
		break;
	case ANJUTA_PROPERTY_OBJECT_TYPE_FONT:
		{
			const gchar *font;
			font = gnome_font_picker_get_font_name (GNOME_FONT_PICKER
													(prop->object));
			text_value = g_strdup (font);
		}
		break;
	}
	if (text_value && (strlen (text_value) == 0))
	{
		g_free (text_value);
		text_value = NULL;
	}
	return text_value;
}

/*
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
*/
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
	case ANJUTA_PROPERTY_OBJECT_TYPE_MENU:
		values = g_object_get_data(G_OBJECT(prop->object), "untranslated");
		if (value != NULL)
		{
			for (i=0; values[i] != NULL; i++)
			{
				if (strcmp(value, values[i]) == 0)
				{
					gtk_option_menu_set_history(GTK_OPTION_MENU(prop->object), i);
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
			guint8 r, g, b;
			
			if (value)
				anjuta_util_color_from_string (value, &r, &g, &b);
			else
				r = g = b = 0;
			
			gnome_color_picker_set_i8 (GNOME_COLOR_PICKER (prop->object),
									   r, g, b, 8);
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
#ifdef DEBUG
				g_message ("Font set as: %s", font_name);
#endif
				gnome_font_picker_set_font_name (GNOME_FONT_PICKER
												 (prop->object), font_name);
				g_free (font_name);
			}
			else
			{				
				gnome_font_picker_set_font_name (GNOME_FONT_PICKER
												 (prop->object), value);
			}
		}
		/* else
		{
			gnome_font_picker_set_font_name (GNOME_FONT_PICKER (prop->object),
											 "A standard font");
		}*/
		break;
	}
}

/*
static void
set_property_value_as_int (AnjutaProperty *prop, gint value)
{
	gchar *text_value;
	text_value = g_strdup_printf ("%d", value);
	set_property_value_as_string (prop, text_value);
	g_free (text_value);	
}
*/

static gboolean
save_property (AnjutaPreferences *pr, AnjutaProperty *prop,
			   FILE *stream, AnjutaPreferencesFilterType filter)
{
	gchar *value;
	gboolean return_value;

	return_value = 0;
	
	if ((filter != ANJUTA_PREFERENCES_FILTER_NONE) && (prop->flags & filter))
	{
		value = prop_get (pr->props_session, prop->key);
	}
	else
	{
		value = prop_get (pr->props, prop->key);
	}
	if (value)
		return_value = fprintf (stream, "%s=%s\n", prop->key, value);
	else
		return_value = fprintf (stream, "%s=\n", prop->key);
#ifdef DEBUG
	if (return_value <= 0)
		g_warning ("Error saving property '%s'", prop->key);
#endif
	g_free (value);
	return (return_value > 0);
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
	
	g_return_val_if_fail (ANJUTA_IS_PREFERENCES (pr), FALSE);
	g_return_val_if_fail (GTK_IS_WIDGET (object), FALSE);
	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (strlen(key) > 0, FALSE);
	
	p = g_new0 (AnjutaProperty, 1);
	g_object_ref (object);
	p->object = object;
	p->object_type = object_type;
	p->data_type = data_type;
	p->key = g_strdup (key);
	if (default_value)
	{
		p->default_value = g_strdup (default_value);
		if (strlen (default_value) > 0)
		{
			/* For menu, initialize the untranslated strings */
			if (object_type == ANJUTA_PROPERTY_OBJECT_TYPE_MENU) 
			{
				gchar **vstr;
				vstr = g_strsplit (default_value, ",", 100);
				g_object_set_data(G_OBJECT(p->object), "untranslated",
									vstr);
			}
			prop_set_with_key (pr->props_built_in, key, default_value);
		}
	}
	p->flags = flags;
	p->custom = FALSE;
	p->set_property = NULL;
	p->get_property = NULL;
	pr->priv->properties = g_list_append (pr->priv->properties, p);
	return TRUE;
}

/**
 * anjuta_preferences_register_property_custom:
 * @pr: a #AnjutaPreferences object.
 * @object: Object to register.
 * @key: Property key.
 * @default_value: Default value of the key.
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
										     guint flags,
		void    (*set_property) (AnjutaProperty *prop, const gchar *value),
		gchar * (*get_property) (AnjutaProperty *))
{
	AnjutaProperty *p;
	
	g_return_val_if_fail (ANJUTA_IS_PREFERENCES (pr), FALSE);
	g_return_val_if_fail (GTK_IS_WIDGET (object), FALSE);
	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (strlen(key) > 0, FALSE);
	
	p = g_new0 (AnjutaProperty, 1);
	g_object_ref (object);
	p->object = object;
	p->object_type = (AnjutaPropertyObjectType) 0;
	p->data_type = (AnjutaPropertyDataType) 0;
	p->key = g_strdup (key);
	if (default_value)
	{
		p->default_value = g_strdup (default_value);
		if (strlen (default_value) > 0)
			prop_set_with_key (pr->props_built_in, key, default_value);
	}
	p->custom = TRUE;
	p->flags = flags;
	p->set_property = set_property;
	p->get_property = get_property;
	
	pr->priv->properties = g_list_append (pr->priv->properties, p);
	return TRUE;
}

/**
 * anjuta_preferences_register_property_from_string:
 * @pr: a #AnjutaPreferences object
 * @object: Widget to register
 * @property_desc: Property description (see anjuta_preferences_add_pag())
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
 * anjuta_preferences_get:
 * @pr: A #AnjutaPreferences object
 * @key: Property key
 *
 * Gets the value of @key as string. Returned string should be g_freed() when not
 * required.
 *
 * Return value: Key value as string or NULL if the key is not defined.
 */
inline gchar *
anjuta_preferences_get (AnjutaPreferences *pr, const gchar *key)
{
	g_return_val_if_fail (ANJUTA_IS_PREFERENCES (pr), NULL);
	g_return_val_if_fail (key != NULL, NULL);
	return prop_get (pr->props, key);
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
inline gint
anjuta_preferences_get_int (AnjutaPreferences *pr, const gchar *key)
{
	g_return_val_if_fail (ANJUTA_IS_PREFERENCES (pr), 0);
	g_return_val_if_fail (key != NULL, 0);
	return prop_get_int (pr->props, key, 0);
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
inline gint
anjuta_preferences_get_int_with_default (AnjutaPreferences *pr,
										 const gchar *key, gint default_value)
{
	g_return_val_if_fail (ANJUTA_IS_PREFERENCES (pr), 0);
	g_return_val_if_fail (key != NULL, 0);
	return prop_get_int (pr->props, key, default_value);
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
inline gchar *
anjuta_preferences_default_get (AnjutaPreferences * pr, const gchar * key)
{
	g_return_val_if_fail (ANJUTA_IS_PREFERENCES (pr), NULL);
	g_return_val_if_fail (key != NULL, NULL);
	return prop_get (pr->props_local, key);
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
inline gint
anjuta_preferences_default_get_int (AnjutaPreferences *pr, const gchar *key)
{
	g_return_val_if_fail (ANJUTA_IS_PREFERENCES (pr), 0);
	g_return_val_if_fail (key != NULL, 0);
	return prop_get_int (pr->props_local, key, 0);
}

/**
 * anjuta_preferences_set:
 * @pr: A #AnjutaPreferences object.
 * @key: Property key.
 * @value: Value of the key.
 *
 * Sets the value of @key in current session.
 */
inline void
anjuta_preferences_set (AnjutaPreferences *pr, const gchar *key,
						const gchar *value)
{
	g_return_if_fail (ANJUTA_IS_PREFERENCES (pr));
	g_return_if_fail (key != NULL);

	if (value && (strlen (value) > 0))
		prop_set_with_key (pr->props, key, value);
	else
		prop_set_with_key (pr->props, key, "");
}

/**
 * anjuta_preferences_set_int:
 * @pr: A #AnjutaPreferences object.
 * @key: Property key.
 * @value: Integer value of the key.
 *
 * Sets the value of @key in current session.
 */
inline void
anjuta_preferences_set_int (AnjutaPreferences *pr, const gchar *key,
							gint value)
{
	g_return_if_fail (ANJUTA_IS_PREFERENCES (pr));
	g_return_if_fail (key != NULL);
	prop_set_int_with_key(pr->props, key, value);
}

static gboolean
preferences_objects_to_prop (AnjutaPreferences *pr)
{
	AnjutaProperty *p;
	GList *node;
	node = pr->priv->properties;
	while (node)
	{
		gchar *value;
		p = node->data;
		value = get_property_value_as_string (p);
		anjuta_preferences_set (pr, p->key, value);
		g_free (value);
		node = g_list_next (node);
	}
	return TRUE;
}

static void
on_preferences_dialog_response (GtkDialog *dialog,
                                gint response, gpointer user_data)
{
	AnjutaPreferences *pr = (AnjutaPreferences *) user_data;
	if (pr)
	{
		switch (response)
		{
		case GTK_RESPONSE_OK:
			gtk_widget_hide (GTK_WIDGET (pr));
			/* Note: No break here */
		case GTK_RESPONSE_APPLY:
			preferences_objects_to_prop (pr);
			g_signal_emit (G_OBJECT (pr),
						   anjuta_preferences_signals[CHANGED], 0);
			break;
		case GTK_RESPONSE_CANCEL:
			gtk_widget_hide (GTK_WIDGET (pr));
			break;
		}
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
		prop_clear (pr->props_session);
		prop_clear (pr->props);
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
	AnjutaProperty *p;
	GList *node;
	gboolean go_on = TRUE;
	
	g_return_if_fail (ANJUTA_IS_PREFERENCES (pr));
	g_return_if_fail (callback != NULL);
	
	node = pr->priv->properties;
	while (node && go_on)
	{
		p = node->data;
		if (p->object_type != ANJUTA_PROPERTY_OBJECT_TYPE_MENU)
		{
			if (filter == ANJUTA_PREFERENCES_FILTER_NONE)
				go_on = callback (pr, p->key, data);
			else if (p->flags & filter)
				go_on = callback (pr, p->key, data);
		}
		node = g_list_next (node);
	}
}

/**
 * anjuta_preferences_save_filtered:
 * @pr: A #AnjutaPreferences object.
 * @stream: File stream.
 * @filter: Keys to filter out from saving.
 * 
 * Similar to anjuta_preferences_save(), except that property keys with
 * with matching @filter are not saved. If @filter is
 * ANJUTA_PREFERENCES_FILTER_NONE, all properties are saved.
 *
 * Return value: TRUE if sucessful. FALSE if error occured during save.
 */
gboolean
anjuta_preferences_save_filtered (AnjutaPreferences *pr, FILE *stream,
								  AnjutaPreferencesFilterType filter)
{
	AnjutaProperty *p;
	GList *node;
	gboolean ret_val = TRUE;
	
	g_return_val_if_fail (ANJUTA_IS_PREFERENCES (pr), FALSE);
	g_return_val_if_fail (stream != NULL, FALSE);
	
	node = pr->priv->properties;
	while (node)
	{
		p = node->data;
		if (save_property (pr, p, stream, filter) == FALSE)
			ret_val = FALSE;
		node = g_list_next (node);
	}
	if (ret_val == FALSE)
		g_warning ("Error saving some preferences properties");
	return ret_val;
}

/**
 * anjuta_preferences_save:
 * @pr: a #AnjutaPreferences object.
 * @stream: File stream where the preferences will be saved.
 *
 * Saves all properties in the given FILE stream. Saved format is
 * key=value, separated by line breaks.
 *
 * Return value: TRUE if sucessful
 */
gboolean
anjuta_preferences_save (AnjutaPreferences *pr, FILE *stream)
{
	return anjuta_preferences_save_filtered (pr, stream,
											 ANJUTA_PREFERENCES_FILTER_NONE);	
}

static gboolean
transfer_to_session (AnjutaPreferences *pr, const gchar *key, gpointer data)
{
	gchar *value;
	g_return_val_if_fail (key, TRUE);
	value = prop_get (pr->props, key);
	if (value)
	{
		prop_set_with_key (pr->props_session, key, value);
		g_free (value);
	}
	return TRUE;
}

/**
 * anjuta_preferences_sync_to_session:
 * @pr: a #AnjutaPreferences object.
 *
 * All property values are transfered to session level from current level
 * (eg. project level).
 */
void
anjuta_preferences_sync_to_session (AnjutaPreferences *pr)
{
	g_return_if_fail (ANJUTA_IS_PREFERENCES (pr));
	
	anjuta_preferences_foreach (pr, ANJUTA_PREFERENCES_FILTER_NONE,
								transfer_to_session, NULL);
}

static gboolean
on_preferences_dialog_delete_event (GtkDialog *dialog,
									GdkEvent *event,
									gpointer user_data)
{
	gtk_widget_hide (GTK_WIDGET(dialog));
	return TRUE;
}

static gboolean
preferences_prop_to_objects (AnjutaPreferences *pr)
{
	AnjutaProperty *p;
	GList *node;
	node = pr->priv->properties;
	while (node)
	{
		gchar *value;
		p = node->data;
			value = anjuta_preferences_get (pr, p->key);
			set_property_value_as_string (p, value);
			g_free (value);
		node = g_list_next (node);
	}
	return TRUE;
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
 * </programlisting>
 *
 * All widgets having the above names in the gxml tree will be registered
 * and will become part of auto saving/loading. For example, refer to
 * anjuta preferences dialogs and study the widget names.
 */
void
anjuta_preferences_add_page (AnjutaPreferences* pr, GladeXML *gxml,
							 const char* glade_widget_name,
							 const gchar *icon_filename)
{
	GtkWidget *parent;
	GtkWidget *page;
	GdkPixbuf *pixbuf;
	
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
	pixbuf = anjuta_res_get_pixbuf (icon_filename);
	anjuta_preferences_dialog_add_page (ANJUTA_PREFERENCES_DIALOG (pr),
										glade_widget_name, pixbuf, page);
	anjuta_preferences_register_all_properties_from_glade_xml (pr, gxml, page);
	g_object_unref (page);
}

static void anjuta_preferences_class_init    (AnjutaPreferencesClass *class);
static void anjuta_preferences_instance_init (AnjutaPreferences      *pr);

static AnjutaPreferencesDialogClass *parent_class;

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
		obj_type = g_type_register_static (ANJUTA_TYPE_PREFERENCES_DIALOG,
		                                   "AnjutaPreferences", &obj_info, 0);
	}
	return obj_type;
}

static void
anjuta_preferences_dispose (GObject *obj)
{
	AnjutaPreferences *pr = ANJUTA_PREFERENCES (obj);

	prop_set_destroy (pr->props_global);
	prop_set_destroy (pr->props_local);
	prop_set_destroy (pr->props_session);
	prop_set_destroy (pr->props);
	
	g_list_foreach (pr->priv->properties, (GFunc)property_destroy, NULL);
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
anjuta_preferences_instance_init (AnjutaPreferences *pr)
{
	gchar *propdir, *propfile, *str;
	
	pr->priv = g_new0(AnjutaPreferencesPriv, 1);
	pr->priv->properties = NULL;
	
	g_message ("Initializing AP Instance");
	
	pr->props_built_in = prop_set_new ();
	pr->props_global = prop_set_new ();
	pr->props_local = prop_set_new ();
	pr->props_session = prop_set_new ();
	pr->props = prop_set_new ();

	prop_clear (pr->props_built_in);
	prop_clear (pr->props_global);
	prop_clear (pr->props_local);
	prop_clear (pr->props_session);
	prop_clear (pr->props);

	prop_set_parent (pr->props_global, pr->props_built_in);
	prop_set_parent (pr->props_local, pr->props_global);
	prop_set_parent (pr->props_session, pr->props_local);
	prop_set_parent (pr->props, pr->props_session);

	/* Reading the build in default properties */
	prop_read_from_memory (pr->props_built_in,
						   default_settings, strlen(default_settings), "");

	/* Dynamic properties: Default paths */
	str = g_strconcat (g_getenv("HOME"), "/Projects", NULL);
	prop_set_with_key (pr->props_built_in, "projects.directory", str);
	g_free (str);
	
	str = g_strconcat (g_getenv("HOME"), "/Tarballs", NULL);
	prop_set_with_key (pr->props_built_in, "tarballs.directory", str);
	g_free (str);

	str = g_strconcat (g_getenv("HOME"), "/Rpms", NULL);
	prop_set_with_key (pr->props_built_in, "rpms.directory", str);
	g_free (str);
	
	str = g_strconcat (g_getenv("HOME"), "/Tarballs", NULL);
	prop_set_with_key (pr->props_built_in, "srpms.directory", str);
	g_free (str);
	
	str = g_strdup (g_getenv("HOME"));
	prop_set_with_key (pr->props_built_in, "anjuta.home.directory", str);
	g_free (str);
	
	prop_set_with_key (pr->props_built_in, "anjuta.data.directory",
					   PACKAGE_DATA_DIR);
	prop_set_with_key (pr->props_built_in, "anjuta.pixmap.directory",
					   PACKAGE_PIXMAPS_DIR);

	/* Load the external configuration files */
	propdir = g_strconcat (PACKAGE_DATA_DIR, "/properties/", NULL);
	propfile = g_strconcat (propdir, "anjuta.properties", NULL);
	
	if (g_file_test (propfile, G_FILE_TEST_EXISTS) == FALSE)
	{
		anjuta_util_dialog_error (GTK_WINDOW (pr),
			_("Cannot load Global defaults and configuration files:\n"
			 "%s.\n"
			 "This may result in improper behaviour or instabilities.\n"
			 "Anjuta will fall back to built in (limited) settings"),
			 propfile);
	}
	prop_read (pr->props_global, propfile, propdir);
	g_free (propfile);
	g_free (propdir);

	propdir = g_strconcat (g_get_home_dir(), "/.anjuta" PREF_SUFFIX "/", NULL);
	propfile = g_strconcat (propdir, "user.properties", NULL);
	
	/* Create user.properties file, if it doesn't exist */
	if (g_file_test (propfile, G_FILE_TEST_EXISTS) == FALSE) {
		gchar* user_propfile = g_strconcat (PACKAGE_DATA_DIR,
					"/properties/user.properties", NULL);
		anjuta_util_copy_file (user_propfile, propfile, FALSE);
		g_free (user_propfile);
	}
	prop_read (pr->props_local, propfile, propdir);
	g_free (propdir);
	g_free (propfile);

	propdir = g_strconcat (g_get_home_dir(), "/.anjuta" PREF_SUFFIX "/", NULL);
	propfile = g_strconcat (propdir, "session.properties", NULL);
	prop_read (pr->props_session, propfile, propdir);
	g_free (propdir);
	g_free (propfile);

	/* A quick hack to fix the 'invisible' browser toolbar */
	str = prop_get(pr->props_session, "anjuta.version");
	if (str) {
		if (strcmp(str, VERSION) != 0)
			remove("~/.gnome/Anjuta");
		g_free (str);
	} else {
		remove("~/.gnome/Anjuta");
	}
	/* quick hack ends */
	
	pr->priv->is_showing = FALSE;
	
	
	/* Add buttons: Cancel/Apply/Ok */
	gtk_dialog_add_button (GTK_DIALOG (pr),
				       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (GTK_DIALOG (pr),
				       GTK_STOCK_APPLY, GTK_RESPONSE_APPLY);
	gtk_dialog_add_button (GTK_DIALOG (pr),
				       GTK_STOCK_OK, GTK_RESPONSE_OK);

	g_signal_connect (G_OBJECT (pr), "delete_event",
					  G_CALLBACK (on_preferences_dialog_delete_event), pr);
	g_signal_connect (G_OBJECT (pr), "response",
					  G_CALLBACK (on_preferences_dialog_response), pr);
}

static void
anjuta_preferences_finalize (GObject *obj)
{
	AnjutaPreferences *dlg = ANJUTA_PREFERENCES (obj);	
	g_free (dlg->priv);
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

static void
anjuta_preferences_hide (GtkWidget *w)
{
	AnjutaPreferences *pr = ANJUTA_PREFERENCES (w);	
	pr->priv->is_showing = FALSE;
	GNOME_CALL_PARENT (GTK_WIDGET_CLASS, hide, (w));
}

static void
anjuta_preferences_show (GtkWidget * w)
{
	AnjutaPreferences *pr = ANJUTA_PREFERENCES (w);
	if (pr->priv->is_showing)
		return;
	preferences_prop_to_objects (ANJUTA_PREFERENCES (pr));
	pr->priv->is_showing = TRUE;
	GNOME_CALL_PARENT (GTK_WIDGET_CLASS, show, (w));
}

static void
anjuta_preferences_close (GtkDialog *obj)
{
	AnjutaPreferences *pr = ANJUTA_PREFERENCES (obj);	
	gtk_widget_hide (GTK_WIDGET (pr));
}

static void
anjuta_preferences_class_init (AnjutaPreferencesClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
	GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (class);
	
	parent_class = g_type_class_peek_parent (class);
	
	anjuta_preferences_signals[CHANGED] =
		g_signal_new ("changed",
					G_TYPE_FROM_CLASS (object_class),
					G_SIGNAL_RUN_FIRST,
					G_STRUCT_OFFSET (AnjutaPreferencesClass, changed),
					NULL, NULL,
					g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	
	object_class->dispose = anjuta_preferences_dispose;
	object_class->finalize = anjuta_preferences_finalize;

	widget_class->hide = anjuta_preferences_hide;
	widget_class->show = anjuta_preferences_show;
	
	// dialog_class->response = anjuta_preferences_response;
	dialog_class->close = anjuta_preferences_close;
}

/**
 * anjuta_preferences_new:
 * 
 * Creates a new #AnjutaPreferences object
 * 
 * Return value: A #AnjutaPreferences object.
 */
GtkWidget *
anjuta_preferences_new ()
{
	static GtkWidget *widget = NULL;

	if (!widget) {
		widget = gtk_widget_new (ANJUTA_TYPE_PREFERENCES,
					 "title", _("Anjuta Preferences"),
					 NULL);
	}
	return widget;
}
