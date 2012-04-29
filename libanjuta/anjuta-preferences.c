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
 * @short_description: Anjuta Preferences system.
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
 * preferences values. This can be done using #GSettings bindings
 * for example.
 *
 * Second is to use anjuta_preferences_add_page(), which will
 * automatically register the preferences keys and values from
 * a glade xml file. The glade xml file contains a preferences
 * page of the plugin. The widget names in the page are
 * given in a particular way (see anjuta_preferences_add_page()) to
 * let it know property key details. The preference dialog will automatically
 * setup the bindings between GSettings and the widgets.
 *
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

struct _AnjutaPreferencesPriv
{
	GtkWidget           *prefs_dialog;
	AnjutaPluginManager *plugin_manager;

	gchar				*common_schema_id;
	GHashTable			*common_gsettings;
};

#define PREFERENCE_PROPERTY_PREFIX "preferences"

G_DEFINE_TYPE (AnjutaPreferences, anjuta_preferences, G_TYPE_OBJECT);

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
	GtkComboBox* combo = GTK_COMBO_BOX(user_data);

	return g_variant_new_string (gtk_combo_box_get_active_id (combo));
}

static gboolean
string_to_active (GValue* value, GVariant* variant, gpointer user_data)
{
	GtkComboBox* combo = GTK_COMBO_BOX(user_data);

	gtk_combo_box_set_active_id (combo, g_variant_get_string (variant, NULL));
	g_value_set_int (value, gtk_combo_box_get_active (combo));

	return g_value_get_int (value) >= 0;
}

static void
update_file_property (GtkWidget* widget, gpointer user_data)
{
	GtkFileChooser *chooser = GTK_FILE_CHOOSER (user_data);
	GSettings *gsettings = g_object_get_data (G_OBJECT (chooser), "anjuta-bind-gsettings");
	const gchar *key = g_object_get_data (G_OBJECT (chooser), "anjuta-bind-key");
	gchar* text_value = gtk_file_chooser_get_filename (chooser);

	g_settings_set_string (gsettings, key, text_value);

	g_free (text_value);
}

/**
 * anjuta_preferences_register_property:
 * @pr: a #AnjutaPreferences object
 * @settings: the #GSettings object associated with that property
 * @object: Widget to register
 * @key: Property key
 *
 * This registers only one widget. The widget could be shown elsewhere.
 * The widget needs to fulfill the properties described in
 * #anjuta_preferences_add_page documentation.
 *
 * Return value: TRUE if sucessful.
 */
gboolean
anjuta_preferences_register_property (AnjutaPreferences *pr,
                                      GSettings *settings,
                                      GtkWidget *object,
                                      const gchar *key)
{
	gboolean ok = TRUE;

	g_return_val_if_fail (ANJUTA_IS_PREFERENCES (pr), FALSE);
	g_return_val_if_fail (GTK_IS_WIDGET (object), FALSE);
	g_return_val_if_fail (strlen(key) > 0, FALSE);

	/* Start with the most specialized widget as a GtkSpinButton
	 * is a GtkEntry too */
	if (GTK_IS_COLOR_BUTTON (object))
	{
		g_settings_bind_with_mapping (settings, key,
								 object, "color",
								 G_SETTINGS_BIND_DEFAULT,
								 gdkcolor_to_string,
								 string_to_gdkcolor,
								 object,
								 NULL);
	}
	else if (GTK_IS_FONT_BUTTON (object))
	{
		g_settings_bind (settings, key, object, "font-name",
		                 G_SETTINGS_BIND_DEFAULT);
	}
	else if (GTK_IS_SPIN_BUTTON (object))
	{
		g_settings_bind (settings, key, object, "value",
		                 G_SETTINGS_BIND_DEFAULT);
	}
	else if (GTK_IS_FILE_CHOOSER_BUTTON (object))
	{
		gchar *filename;

		switch (gtk_file_chooser_get_action (GTK_FILE_CHOOSER (object)))
		{
		case GTK_FILE_CHOOSER_ACTION_OPEN:
		case GTK_FILE_CHOOSER_ACTION_SAVE:
			filename = g_settings_get_string (settings, key);
			gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (object),
										 		 filename);
			g_free (filename);
			g_object_set_data_full (G_OBJECT (object), "anjuta-bind-gsettings", g_object_ref (settings), (GDestroyNotify)g_object_unref);
			g_object_set_data_full (G_OBJECT (object), "anjuta-bind-key", g_strdup (key), (GDestroyNotify)g_free);
			g_signal_connect (G_OBJECT (object), "file-set",
							  G_CALLBACK (update_file_property), object);
			break;
		case GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER:
		case GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER:
			filename = g_settings_get_string (settings, key);
			gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (object),
			                                     filename);
			g_free (filename);
			g_object_set_data_full (G_OBJECT (object), "anjuta-bind-gsettings", g_object_ref (settings), (GDestroyNotify)g_object_unref);
			g_object_set_data_full (G_OBJECT (object), "anjuta-bind-key", g_strdup (key), (GDestroyNotify)g_free);
			g_signal_connect (G_OBJECT(object), "current-folder-changed",
							  G_CALLBACK (update_file_property), object);
			break;
		default:
			ok = FALSE;
		}
	}
	else if (GTK_IS_COMBO_BOX (object))
	{
		g_settings_bind_with_mapping (settings, key,
								 object, "active",
								 G_SETTINGS_BIND_DEFAULT,
								 string_to_active,
								 active_to_string,
								 object,
								 NULL);
	}
	else if (GTK_IS_CHECK_BUTTON (object))
	{
		g_settings_bind (settings, key, object, "active",
		                 G_SETTINGS_BIND_DEFAULT);
	}
	else if (GTK_IS_ENTRY (object))
	{
		g_settings_bind (settings, key, object, "text",
		                 G_SETTINGS_BIND_DEFAULT);
	}
	else
	{
		ok = FALSE;
	}

	return ok;
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
		const gchar *key;
		const gchar *ptr;
		GtkWidget *widget, *p;
		gboolean cont_flag = FALSE;
		GSettings *key_settings = settings;

		if (!GTK_IS_WIDGET (node->data) || !GTK_IS_BUILDABLE (node->data))
			continue;

		widget = node->data;
		name = gtk_buildable_get_name (GTK_BUILDABLE (widget));

		if (!g_str_has_prefix (name, PREFERENCE_PROPERTY_PREFIX))
			continue;

		/* Only ':' is needed between "preferences" and the key name but accept
		 * '_' and additional fields separated by ':' to work with the old
		 * widget naming scheme. */
		key = &name[strlen (PREFERENCE_PROPERTY_PREFIX)];
		if ((*key != '_') && (*key != ':'))
			continue;

		for (ptr = ++key; *ptr != '\0'; ptr++)
		{
			if (*ptr == ':') key = ptr + 1;
		}
		if (*key == '\0') continue;

		/* Check if we need to use common settings */
		if (*key == '.')
		{
			const gchar *id = strrchr (key, '.');
			GString *schema_id;

			schema_id = g_string_new (pr->priv->common_schema_id);
			if (key != id)
			{
				g_string_append_len (schema_id, key, id - key);
			}
			key = id + 1;

			key_settings = (GSettings *)g_hash_table_lookup (pr->priv->common_gsettings, schema_id->str);
			if (key_settings == NULL)
			{
				key_settings = g_settings_new (schema_id->str);
				g_hash_table_insert (pr->priv->common_gsettings, schema_id->str, key_settings);
				g_string_free (schema_id, FALSE);
			}
			else
			{
				g_string_free (schema_id, TRUE);
			}
		}

		/* Added only if it's a descendant child of the parent */
		p = gtk_widget_get_parent (widget);
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

		if (!anjuta_preferences_register_property (pr, key_settings, widget, key))
		{
			g_critical ("Invalid preference widget named %s, check anjuta_preferences_add_page function documentation.", name);
		}
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
 *     preferences(_.*)?:(.SCHEMAID)?PROPERTYKEY
 *     where,
 *       SCHEMAID if present the key will be added not in the page settings but
 *                in common settings using SCHEMAID as a suffix.
 *       PROPERTYKEY is the property key. e.g - 'tab-size'.
 * </programlisting>
 *
 * The widget must derivated from one of the following Gtk widget:
 *		* GtkColorButton
 * 		* GtkFontButton
 * 		* GtkSpinButton
 *		* GtkFileChooserButton
 * 		* GtkComboBox
 *		* GtkCheckButton
 * 		* GtkEntry
 *
 * In addition, the model of a GtkComboBox must have an id column.
 * It is the value of this column which is saved as preference.
 *
 * All widgets having the above names in the gxml tree will be registered
 * and will become part of auto saving/loading. For example, refer to
 * anjuta preferences dialogs and study the widget names.
 *
 * Older versions of Anjuta use more fields in the widget name. They do not
 * need an id column in GtkComboBox model and could work with a custom widget.
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

	if (pr->priv->common_gsettings)
	{
		g_hash_table_destroy (pr->priv->common_gsettings);
		pr->priv->common_gsettings = NULL;
	}
	g_free (pr->priv->common_schema_id);
	pr->priv->common_schema_id = NULL;
}

static void
anjuta_preferences_init (AnjutaPreferences *pr)
{
	pr->priv = g_new0 (AnjutaPreferencesPriv, 1);
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
 * @common_schema_id: Common schema id used for key starting with .
 *
 * Creates a new #AnjutaPreferences object
 *
 * Return value: A #AnjutaPreferences object.
 */
AnjutaPreferences *
anjuta_preferences_new (AnjutaPluginManager *plugin_manager, const gchar *common_schema_id)
{
	AnjutaPreferences *pr;

	if (!default_preferences)
	{
		pr = g_object_new (ANJUTA_TYPE_PREFERENCES, NULL);
		pr->priv->plugin_manager = g_object_ref (plugin_manager);
		pr->priv->common_schema_id = g_strdup (common_schema_id);
		pr->priv->common_gsettings = g_hash_table_new_full (g_str_hash, g_str_equal,
		                                                    g_free,
		                                                    (GDestroyNotify) g_object_unref);
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
