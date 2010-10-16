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
#include <stdlib.h>

#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/resources.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>

/* AnjutaPreferences is a singleton */
static AnjutaPreferences* default_preferences = NULL;

struct _AnjutaProperty
{
	GtkWidget                *object;
	gchar                    *key;
	gchar                    *default_value;
	guint                     flags;
	gint                     notify_id;
	GSettings				 *gsettings;
	
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
	GHashTable          *properties;
	GtkWidget           *prefs_dialog;
	AnjutaPluginManager *plugin_manager;
	gboolean             is_showing;

	GHashTable			*notifications;
};

/* Internal structure for anjuta_preferences_foreach */
struct _AnjutaPreferencesForeachData
{
	AnjutaPreferences *pr;
	AnjutaPreferencesCallback callback;
	gpointer callback_data;
};

#define PREFERENCE_PROPERTY_PREFIX "preferences_"

G_DEFINE_TYPE (AnjutaPreferences, anjuta_preferences, G_TYPE_OBJECT);

static void
property_destroy (AnjutaProperty *property)
{
	g_return_if_fail (property);
	if (property->key) g_free (property->key);
	if (property->default_value) g_free (property->default_value);
	g_object_unref (property->object);
	g_object_unref (property->gsettings);
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

static GVariant*
string_to_gdkcolor (const GValue* value, const GVariantType* type, gpointer user_data)
{
	GdkColor* color = g_value_get_boxed (value);
	gchar* name = gdk_color_to_string (color);
	
	GVariant* variant = g_variant_new_string (name);
	g_free (name);
	
	return variant;
}

static gboolean
gdkcolor_to_string (GValue* value, GVariant* variant, gpointer user_data)
{
	GdkColor color;
	gdk_color_parse (g_variant_get_string (variant, NULL), &color);
	g_value_set_boxed (value, &color);
	return TRUE;
}

static GVariant*
active_to_string (const GValue* value, const GVariantType* type, gpointer user_data)
{
	AnjutaProperty* p = user_data;
	GtkComboBox* combo = GTK_COMBO_BOX(p->object);
	gint idx;
	gchar** values = g_object_get_data(G_OBJECT(combo), "untranslated");
	GVariant* variant;
	
	idx = gtk_combo_box_get_active(combo);

	variant = g_variant_new_string (values[idx]);
	
	return variant;
}

static gboolean
string_to_active (GValue* value, GVariant* variant, gpointer user_data)
{
	AnjutaProperty* p = user_data;
	GtkComboBox* combo = GTK_COMBO_BOX(p->object);
	const gchar* text_value = g_variant_get_string (variant, NULL);
	gchar** values = g_object_get_data(G_OBJECT(combo), "untranslated");
	int i;
	
	for (i=0; values[i] != NULL; i++)
	{
		if (strcmp(text_value, values[i]) == 0)
		{
			g_value_set_int (value, i);
			return TRUE;
		}
	}
		
	return FALSE;
}

static void
update_file_property (GtkWidget* widget, gpointer user_data)
{
	AnjutaProperty* p = user_data;
	gchar* text_value = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (p->object));
	
	g_settings_set_string (p->gsettings, p->key, text_value);
	
	g_free (text_value);
}

static void
connect_objects (AnjutaPreferences *pr, GSettings* settings, AnjutaProperty *p)
{	
	g_object_set_data (G_OBJECT (p->object), "AnjutaPreferences", pr);
	switch (p->object_type) {
		case ANJUTA_PROPERTY_OBJECT_TYPE_ENTRY:
			g_settings_bind (settings, p->key, p->object, "text", 
			                 G_SETTINGS_BIND_DEFAULT);
			break;
		case ANJUTA_PROPERTY_OBJECT_TYPE_SPIN:
			g_settings_bind (settings, p->key, p->object, "value", 
			                 G_SETTINGS_BIND_DEFAULT);
			break;
		case ANJUTA_PROPERTY_OBJECT_TYPE_FONT:
			g_settings_bind (settings, p->key, p->object, "font-name", 
			                 G_SETTINGS_BIND_DEFAULT);
			break;
		case ANJUTA_PROPERTY_OBJECT_TYPE_COMBO:
			g_settings_bind_with_mapping (settings, p->key, 
									 p->object, "active",
									 G_SETTINGS_BIND_DEFAULT,
									 string_to_active,		
									 active_to_string,					 
									 p,
									 NULL);
			break;
		case ANJUTA_PROPERTY_OBJECT_TYPE_TOGGLE:
			g_settings_bind (settings, p->key, p->object, "active", 
			                 G_SETTINGS_BIND_DEFAULT);
			break;
		case ANJUTA_PROPERTY_OBJECT_TYPE_COLOR:
			g_settings_bind_with_mapping (settings, p->key, 
									 p->object, "color",
									 G_SETTINGS_BIND_DEFAULT,
									 gdkcolor_to_string,
									 string_to_gdkcolor,							 
									 p,
									 NULL);
									 
			break;
		case ANJUTA_PROPERTY_OBJECT_TYPE_FILE:
			/* Set initial value */
			{
				gchar* filename = g_settings_get_string (p->gsettings, p->key);
				gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (p->object),
											   filename);
				g_free (filename);
			}
			g_signal_connect (G_OBJECT(p->object), "file-set",
							  G_CALLBACK (update_file_property), p);
			break;
		case ANJUTA_PROPERTY_OBJECT_TYPE_FOLDER:
			/* Set initial value */
			{
				gchar* filename = g_settings_get_string (p->gsettings, p->key);
				gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (p->object),
											 		 filename);
				g_free (filename);
			}
			g_signal_connect (G_OBJECT(p->object), "current-folder-changed",
							  G_CALLBACK (update_file_property), p);
			break;
		default:
			break;
	}
}

/**
 * anjuta_preferences_register_property_raw:
 * @pr: a #AnjutaPreferences object
 * @settings: the #GSettings object associated with that property
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
                                          GSettings *settings,
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
	g_return_val_if_fail ((object_type != ANJUTA_PROPERTY_OBJECT_TYPE_COMBO) ||
				((default_value != NULL) &&
				 (*default_value != '\0')), FALSE);

	p = g_new0 (AnjutaProperty, 1);
	g_object_ref (object);
	p->object = object;
	p->object_type = object_type;
	p->data_type = data_type;
	p->key = g_strdup (key);
	p->gsettings = settings;
	g_object_ref (p->gsettings);
	
	p->flags = flags;
	p->custom = FALSE;
	p->set_property = NULL;
	p->get_property = NULL;
	
	if (default_value)
	{
		p->default_value = g_strdup (default_value);
		if (strlen (default_value) > 0)
		{
			/* For combo, initialize the untranslated strings */
			if (object_type == ANJUTA_PROPERTY_OBJECT_TYPE_COMBO)
			{
				gchar **vstr;

				vstr = g_strsplit (default_value, ",", 100);
				g_object_set_data_full (G_OBJECT(p->object), "untranslated",
									    vstr, (GDestroyNotify) g_strfreev);
			}
		}
	}
	
	g_hash_table_insert (pr->priv->properties, g_strdup (key), p);
	connect_objects (pr, settings, p);
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
                                                  GSettings* settings,
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
	anjuta_preferences_register_property_raw (pr, settings, object, key, default_value,
											  flags,  object_type,
											  data_type);
	g_strfreev (fields);
	return TRUE;
}

/**
 * anjuta_preferences_register_all_properties_from_builder_xml:
 * @pr: a #AnjutaPreferences Object
 * @builder: GtkBuilder object containing the properties widgets.
 * @parent: Parent widget in the builder object
 *
 * This will register all the properties names of the format described above
 * without considering the UI. Useful if you have the widgets shown elsewhere
 * but you want them to be part of preferences system.
 */
void
anjuta_preferences_register_all_properties_from_builder_xml (AnjutaPreferences *pr,
                                                             GtkBuilder *builder,
                                                             GSettings *settings,
                                                             GtkWidget *parent)
{
	GSList *widgets;
	GSList *node;
	
	g_return_if_fail (ANJUTA_IS_PREFERENCES (pr));
	g_return_if_fail (builder != NULL);
	
	widgets = gtk_builder_get_objects (builder);
	for (node = widgets; node != NULL; node = g_slist_next (node))
	{
		const gchar *name;
		const gchar *property;
		GtkWidget *widget, *p;
		gboolean cont_flag = FALSE;

		if (!GTK_IS_WIDGET (node->data) || !GTK_IS_BUILDABLE (node->data))
			continue;
		
		widget = node->data;
		name = gtk_buildable_get_name (GTK_BUILDABLE (widget));

		if (!g_str_has_prefix (name, PREFERENCE_PROPERTY_PREFIX))
			continue;
		
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
		if (cont_flag)
			continue;

		property = &name[strlen (PREFERENCE_PROPERTY_PREFIX)];
		anjuta_preferences_register_property_from_string (pr, settings, widget,
		                                                  property);
	}
}

/**
 * anjuta_preferences_add_page:
 * @pr: a #AnjutaPreferences object
 * @builder: #GtkBuilder object containing the preferences page
 * @settings: the #GSettings object associated with that page
 * @gwidget_name: Page widget name (as give with glade interface editor).
 * The widget will be searched with the given name and detached
 * (that is, removed from the container, if present) from it's parent.
 * @icon_filename: File name (of the form filename.png) of the icon representing
 * the preference page.
 * 
 * Add a page to the preferences sytem.
 * builder is the GtkBuilder object of the dialog containing the page widget.
 * The dialog will contain the layout of the preferences widgets.
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
anjuta_preferences_add_from_builder (AnjutaPreferences *pr, 
                                     GtkBuilder *builder,
                                     GSettings *settings,
                                     const gchar *widget_name,
                                     const gchar *title,
                                     const gchar *icon_filename)
{
	GtkWidget *parent;
	GtkWidget *page;
	GdkPixbuf *pixbuf;
	gchar *image_path;
	
	g_return_if_fail (ANJUTA_IS_PREFERENCES (pr));
	g_return_if_fail (widget_name != NULL);
	g_return_if_fail (icon_filename != NULL);
	
	page = GTK_WIDGET(gtk_builder_get_object (builder, widget_name));
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
										widget_name, title, pixbuf, page);
	anjuta_preferences_register_all_properties_from_builder_xml (pr, builder, settings, page);
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


/*
 * anjuta_preferences_get_dialog:
 * @pr: AnjutaPreferences object
 *
 * Returns: The preference dialog. Creates the dialog if it doesn't exist
 */
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

/*
 * anjuta_preferences_is_dialog_created:
 * @pr: AnjutaPreferences
 *
 * Returns: Whether the preference dialog was already created
 */
gboolean
anjuta_preferences_is_dialog_created (AnjutaPreferences *pr)
{
	return (pr->priv->prefs_dialog != NULL);
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
anjuta_preferences_init (AnjutaPreferences *pr)
{
	pr->priv = g_new0 (AnjutaPreferencesPriv, 1);
	
	pr->priv->properties = g_hash_table_new_full (g_str_hash, g_str_equal,
												  g_free, 
												  (GDestroyNotify) property_destroy);
	pr->priv->notifications = g_hash_table_new_full (g_int_hash,
	                                                 g_int_equal,
	                                                 NULL, 
	                                                 g_free);
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
 * @plugin_manager: #AnjutaPluginManager to be used
 * 
 * Creates a new #AnjutaPreferences object
 * 
 * Return value: A #AnjutaPreferences object.
 */
AnjutaPreferences *
anjuta_preferences_new (AnjutaPluginManager *plugin_manager)
{
	AnjutaPreferences *pr;

	if (!default_preferences)
	{
		pr = g_object_new (ANJUTA_TYPE_PREFERENCES, NULL);
		pr->priv->plugin_manager = g_object_ref (plugin_manager);
		default_preferences = pr;
		return pr;
	}
	else
		return default_preferences;
	
}

/**
 * anjuta_preferences_default:
 * 
 * Get the default instace of anjuta preferences
 * 
 * Return value: A #AnjutaPreferences object.
 */
AnjutaPreferences *anjuta_preferences_default ()
{
	return default_preferences;
}
