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
#include "resources.h"
#include "preferences.h"
#include "preferences-dialog.h"
#include "anjuta.h"
//#include "commands.h"
#include "defaults.h"
#include "pixmaps.h"

struct _PreferencesPriv {
	GList     *properties;
	GtkWidget *dialog;
	gboolean   is_showing;
};

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

static AnjutaPropertyObjectType
get_object_type_from_string (const gchar* object_type)
{
	if (strcmp (object_type, "entry") == 0)
		return ANJUTA_PROPERTY_OBJECT_TYPE_ENTRY;
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
	
	switch (prop->object_type)
	{
	case ANJUTA_PROPERTY_OBJECT_TYPE_TOGGLE:
		if (value) 
			int_value = atoi (value);
		else
			int_value = atoi (prop->default_value);
		
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prop->object),
		                              int_value);
		break;
		
	case ANJUTA_PROPERTY_OBJECT_TYPE_SPIN:
		if (value) 
			int_value = atoi (value);
		else
			int_value = atoi (prop->default_value);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (prop->object), int_value);
		break;
	
	case ANJUTA_PROPERTY_OBJECT_TYPE_ENTRY:
		if (value)
			gtk_entry_set_text (GTK_ENTRY (prop->object), value);
		else
			gtk_entry_set_text (GTK_ENTRY (prop->object),
								(gchar*) prop->default_value);
		break;
	case ANJUTA_PROPERTY_OBJECT_TYPE_TEXT:
		{
			GtkTextBuffer *buffer;
			buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (prop->object));
			if (value)
				gtk_text_buffer_set_text (buffer, value, -1);
			else
				gtk_text_buffer_set_text (buffer, (gchar*) prop->default_value,
										  -1);
		}
		break;
	case ANJUTA_PROPERTY_OBJECT_TYPE_COLOR:
		{
			guint8 r, g, b;
			
			if (value)
				anjuta_util_color_from_string (value, &r, &g, &b);
			else if (prop->default_value)
				anjuta_util_color_from_string (prop->default_value,
											   &r, &g, &b);
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
				g_message ("Terminal font set as: %s", font_name);
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
		else
		{
			gnome_font_picker_set_font_name (GNOME_FONT_PICKER (prop->object),
											 prop->default_value);
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
save_property (Preferences *pr, AnjutaProperty *prop,
			   FILE *fp, PreferencesFilterType filter)
{
	gchar *value;
	gboolean return_value;

	return_value = 0;
	if ((filter != PREFERENCES_FILTER_NONE) && (prop->flags & filter))
	{
#ifdef DEBUG
		g_message ("Skipping (filtered) property '%s'", prop->key);
#endif
		value = prop_get (pr->props_session, prop->key);
	}
	else
	{
		value = prop_get (pr->props, prop->key);
	}
	if (value)
		return_value = fprintf (fp, "%s=%s\n", prop->key, value);
#ifdef DEBUG
	if (return_value <= 0)
		g_warning ("Error saving property '%s'", prop->key);
#endif
	g_free (value);
	return (return_value > 0);
}

gboolean
preferences_register_property_raw (Preferences *pr, GtkWidget *object,
								   AnjutaPropertyObjectType object_type,
								   AnjutaPropertyDataType  data_type,
								   const gchar *key, const gchar *default_value,
								   guint flags)
{
	AnjutaProperty *p;
	
	g_return_val_if_fail (pr, FALSE);
	g_return_val_if_fail (GTK_IS_WIDGET (object), FALSE);
	g_return_val_if_fail (key, FALSE);
	g_return_val_if_fail (strlen(key) > 0, FALSE);
	
	p = g_new0 (AnjutaProperty, 1);
	g_object_ref (object);
	p->object = object;
	p->object_type = object_type;
	p->data_type = data_type;
	p->key = g_strdup (key);
	if (default_value)
		p->default_value = g_strdup (default_value);
	p->flags = flags;
	
	pr->priv->properties = g_list_append (pr->priv->properties, p);
	return TRUE;
}

gboolean
preferences_register_property_from_string (Preferences *pr,
                                           GtkWidget *object,
                                           const gchar *property_description)
{
	gchar **fields;
	gchar *field;
	gint  n_fields;
	
	AnjutaPropertyObjectType object_type;
	AnjutaPropertyDataType data_type;
	gchar *key;
	gchar *default_value;
	gint flags;
	
	g_return_val_if_fail (pr, FALSE);
	g_return_val_if_fail ((GTK_IS_WIDGET (object)), FALSE);
	g_return_val_if_fail (property_description, FALSE);
	fields = g_strsplit (property_description, ":", 5);
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
	preferences_register_property_raw (pr, object, object_type,
                                       data_type, key, default_value, flags);
	g_strfreev (fields);
	return TRUE;
}

void
preferences_register_all_properties_from_glade_xml (Preferences* pr,
                                                    GladeXML *gxml)
{
	GList *widgets;
	GList *node;
	widgets = glade_xml_get_widget_prefix (gxml, "preferences_");
	node = widgets;
	while (node)
	{
		const gchar *name;
		GtkWidget *widget = node->data;
		name = glade_get_widget_name (widget);
		if (strncmp (name, PREFERENCE_PROPERTY_PREFIX,
                     strlen (PREFERENCE_PROPERTY_PREFIX)) == 0)
		{
			const gchar *property = &name[strlen (PREFERENCE_PROPERTY_PREFIX)];
			preferences_register_property_from_string (pr, widget, property);
		}
		node = g_list_next (node);
	}
}

inline gchar *
preferences_get (Preferences * p, gchar * key)
{
	return prop_get (p->props, key);
}

inline gint
preferences_get_int (Preferences * p, gchar * key)
{
	return prop_get_int (p->props, key, 0);
}

inline gint
preferences_get_int_with_default (Preferences * p,
								  gchar * key, gint default_value)
{
	return prop_get_int (p->props, key, default_value);
}

inline gchar *
preferences_default_get (Preferences * p, gchar * key)
{
	return prop_get (p->props_local, key);
}

inline gint
preferences_default_get_int (Preferences * p, gchar * key)
{
	return prop_get_int (p->props_local, key, 0);
}

inline void
preferences_set (Preferences * pr, gchar * key, gchar * value)
{
	prop_set_with_key (pr->props, key, value);
}

inline void
preferences_set_int (Preferences * pr, gchar * key, gint value)
{
	prop_set_int_with_key(pr->props, key, value);
}

static gboolean
preferences_objects_to_prop (Preferences *pr)
{
	AnjutaProperty *p;
	GList *node;
	node = pr->priv->properties;
	while (node)
	{
		gchar *value;
		p = node->data;
		value = get_property_value_as_string (p);
		if (value)
		{
			preferences_set (pr, p->key, value);
			g_free (value);
		}
		node = g_list_next (node);
	}
}

static void
on_preferences_dialog_response (GtkDialog *dialog,
                                gint response, gpointer user_data)
{
	Preferences *pr = (Preferences *) user_data;
	if (pr)
	{
		switch (response)
		{
		case GTK_RESPONSE_OK:
			preferences_hide (pr);
			/* Note: No break here */
		case GTK_RESPONSE_APPLY:
			preferences_objects_to_prop (pr);
			if (app->project_dbase->project_is_open)
				app->project_dbase->is_saved = FALSE;
			anjuta_apply_preferences ();
			break;
		case GTK_RESPONSE_CANCEL:
			preferences_hide (pr);
			break;
		}
	}
}

void
preferences_reset_defaults (Preferences * pr)
{
	GtkWidget *dlg;
	
	dlg = gtk_message_dialog_new (GTK_WINDOW (app->widgets.window),
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION,
			GTK_BUTTONS_YES_NO,
		     _("Are you sure you want to reset the preferences to\n"
		       "their default settings?"));
	if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_YES)
	{
		prop_clear (pr->props_session);
		prop_clear (pr->props);
	}
	gtk_widget_destroy (dlg);
}

/* Save excluding the filtered properties */
void
preferences_foreach (Preferences * pr, PreferencesFilterType filter,
					 PreferencesCallback callback, gpointer data)
{
	AnjutaProperty *p;
	GList *node;
	gboolean go_on = TRUE;
	
	node = pr->priv->properties;
	while (node && go_on)
	{
		p = node->data;
		if (filter == PREFERENCES_FILTER_NONE)
			go_on = callback (pr, p->key, data);
		else if (p->flags & filter)
			go_on = callback (pr, p->key, data);
		node = g_list_next (node);
	}
}

/* Save excluding the filtered properties */
gboolean
preferences_save_filtered (Preferences * pr, FILE * fp,
						   PreferencesFilterType filter)
{
	AnjutaProperty *p;
	GList *node;
	gboolean ret_val = TRUE;
	
	node = pr->priv->properties;
	while (node)
	{
		p = node->data;
		if (save_property (pr, p, fp, filter) == FALSE)
			ret_val = FALSE;
		node = g_list_next (node);
	}
	if (ret_val == FALSE)
		g_warning ("Error saving some preferences properties");
	return ret_val;
}

gboolean
preferences_save (Preferences *pr, FILE *fp)
{
	return preferences_save_filtered (pr, fp, PREFERENCES_FILTER_NONE);	
}

static gboolean
transfer_to_session (Preferences *pr, const gchar *key, gpointer data)
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

void
preferences_sync_to_session (Preferences *pr)
{
	preferences_foreach (pr, PREFERENCES_FILTER_NONE,
						 transfer_to_session, NULL);
}

void 
preferences_hide (Preferences *pr)
{
	g_return_if_fail (pr);
	gtk_widget_hide (GTK_WIDGET (pr->priv->dialog));
	pr->priv->is_showing = FALSE;
}

static gboolean
on_preferences_dialog_delete_event (GtkDialog *dialog,
									GdkEvent *event,
									gpointer user_data)
{
	Preferences *pr = (Preferences*) user_data;
	preferences_hide (pr);
	return TRUE;
}

static gboolean
preferences_prop_to_objects (Preferences *pr)
{
	AnjutaProperty *p;
	GList *node;
	node = pr->priv->properties;
	while (node)
	{
		gchar *value;
		p = node->data;
		value = preferences_get (pr, p->key);
		set_property_value_as_string (p, value);
		g_free (value);
		node = g_list_next (node);
	}
}

void
preferences_show (Preferences * pr)
{
	g_return_if_fail (pr);
	if (pr->priv->is_showing)
		return;
	preferences_prop_to_objects (pr);
	gtk_widget_show (pr->priv->dialog);
	pr->priv->is_showing = TRUE;
}

static void
on_style_editor_clicked (GtkWidget *button, Preferences *pr)
{
	g_signal_emit_by_name (app->widgets.menubar.settings.style_editor,
						   "activate", NULL);
}

void
preferences_add_page (Preferences* pr, GladeXML *gxml,
					  const char* glade_widget_name,
					  const gchar *icon_filename)
{
	GtkWidget *parent;
	GtkWidget *page;
	GdkPixbuf *pixbuf;
	
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
	anjuta_preferences_dialog_add_page (glade_widget_name, pixbuf, page);
	g_object_unref (page);
	preferences_register_all_properties_from_glade_xml (pr, gxml);
}

static void
add_all_default_pages (Preferences *pr)
{
	GtkWidget *button;
	GladeXML *gxml;
	
	gxml = glade_xml_new (GLADE_FILE_ANJUTA, "preferences_dialog_general", NULL);
	preferences_add_page (pr, gxml, "General", "preferences-general.png");
	g_object_unref (gxml);
	
	gxml = glade_xml_new (GLADE_FILE_ANJUTA, "preferences_dialog_editor", NULL);
	preferences_add_page (pr, gxml, "Editor", "preferences-editor.png");
	button = glade_xml_get_widget (gxml, "edit_syntax_highlighting");
	g_signal_connect (G_OBJECT (button), "clicked",
					  G_CALLBACK (on_style_editor_clicked), pr);
	g_object_unref (gxml);
	
	gxml = glade_xml_new (GLADE_FILE_ANJUTA, "preferences_dialog_indentation", NULL);
	preferences_add_page (pr, gxml, "Indentation", "preferences-indentation.png");
	g_object_unref (gxml);
	
	gxml = glade_xml_new (GLADE_FILE_ANJUTA, "preferences_dialog_build", NULL);
	preferences_add_page (pr, gxml, "Build", "preferences-build.png");
	g_object_unref (gxml);
	
	gxml = glade_xml_new (GLADE_FILE_ANJUTA, "preferences_dialog_print", NULL);
	preferences_add_page (pr, gxml, "Print", "preferences-print.png");
	g_object_unref (gxml);
	
	gxml = glade_xml_new (GLADE_FILE_ANJUTA, "preferences_dialog_messages", NULL);
	preferences_add_page (pr, gxml, "Messages", "preferences-messages.png");
	g_object_unref (gxml);
	
	gxml = glade_xml_new (GLADE_FILE_ANJUTA, "preferences_dialog_cvs", NULL);
	preferences_add_page (pr, gxml, "CVS", "preferences-cvs.png");
	g_object_unref (gxml);
	
	gxml = glade_xml_new (GLADE_FILE_ANJUTA, "preferences_dialog_terminal", NULL);
	preferences_add_page (pr, gxml, "Terminal", "preferences-terminal.png");
	g_object_unref (gxml);
}
			
static void
create_preferences_gui (Preferences * pr)
{
	GtkWidget *button;
	
	pr->priv->dialog = anjuta_preferences_dialog_get ();
	gtk_window_set_transient_for (GTK_WINDOW (pr->priv->dialog),
								  GTK_WINDOW (app->widgets.window));
	gtk_window_add_accel_group (GTK_WINDOW (pr->priv->dialog),
								app->accel_group);
	
	/* Add buttons: Cancel/Apply/Ok */
	gtk_dialog_add_button (GTK_DIALOG (pr->priv->dialog),
				       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (GTK_DIALOG (pr->priv->dialog),
				       GTK_STOCK_APPLY, GTK_RESPONSE_APPLY);
	gtk_dialog_add_button (GTK_DIALOG (pr->priv->dialog),
				       GTK_STOCK_OK, GTK_RESPONSE_OK);

	g_signal_connect (G_OBJECT (pr->priv->dialog), "delete_event",
					  G_CALLBACK (on_preferences_dialog_delete_event), pr);
	g_signal_connect (G_OBJECT (pr->priv->dialog), "response",
					  G_CALLBACK (on_preferences_dialog_response), pr);
	
	add_all_default_pages (pr);
}

Preferences *
preferences_new ()
{
	Preferences *pr;
	pr = (Preferences *) malloc (sizeof (Preferences));
	if (pr)
	{
		gchar *propdir, *propfile, *str;
		pr->priv = g_new0 (PreferencesPriv, 1);
		pr->priv->properties = NULL;
		
		pr->props_build_in = prop_set_new ();
		pr->props_global = prop_set_new ();
		pr->props_local = prop_set_new ();
		pr->props_session = prop_set_new ();
		pr->props = prop_set_new ();

		prop_clear (pr->props_build_in);
		prop_clear (pr->props_global);
		prop_clear (pr->props_local);
		prop_clear (pr->props_session);
		prop_clear (pr->props);

		prop_set_parent (pr->props_global, pr->props_build_in);
		prop_set_parent (pr->props_local, pr->props_global);
		prop_set_parent (pr->props_session, pr->props_local);
		prop_set_parent (pr->props, pr->props_session);

		/* Reading the build in default properties */
		prop_read_from_memory (pr->props_build_in,
                               default_settings, strlen(default_settings), "");

		/* Dynamic properties: Default paths */
		str = g_strconcat (g_getenv("HOME"), "/Projects", NULL);
		prop_set_with_key (pr->props_build_in, "projects.directory", str);
		g_free (str);
		
		str = g_strconcat (g_getenv("HOME"), "/Tarballs", NULL);
		prop_set_with_key (pr->props_build_in, "tarballs.directory", str);
		g_free (str);

		str = g_strconcat (g_getenv("HOME"), "/Rpms", NULL);
		prop_set_with_key (pr->props_build_in, "rpms.directory", str);
		g_free (str);
		
		str = g_strconcat (g_getenv("HOME"), "/Tarballs", NULL);
		prop_set_with_key (pr->props_build_in, "srpms.directory", str);
		g_free (str);
		
		str = g_strdup (g_getenv("HOME"));
		prop_set_with_key (pr->props_build_in, "anjuta.home.directory", str);
		g_free (str);
		
		prop_set_with_key (pr->props_build_in, "anjuta.data.directory",
		                   app->dirs->data);
		prop_set_with_key (pr->props_build_in, "anjuta.pixmap.directory",
		                   app->dirs->pixmaps);
	
		/* Load the external configuration files */
		propdir = g_strconcat (app->dirs->data, "/properties/", NULL);
		propfile = g_strconcat (propdir, "anjuta.properties", NULL);
		
		if (file_is_readable (propfile) == FALSE)
		{
			anjuta_error
				(_("Cannot load Global defaults and configuration files:\n"
				 "%s.\n"
				 "This may result in improper behaviour or instabilities.\n"
				 "Anjuta will fall back to built in (limited) settings"),
				 propfile);
		}
		prop_read (pr->props_global, propfile, propdir);
		g_free (propfile);
		g_free (propdir);

		propdir = g_strconcat (app->dirs->home, "/.anjuta/", NULL);
		propfile = g_strconcat (propdir, "user.properties", NULL);
		
		/* Create user.properties file, if it doesn't exist */
		if (file_is_regular (propfile) == FALSE) {
			gchar* user_propfile = g_strconcat (app->dirs->data,
						"/properties/user.properties", NULL);
			copy_file (user_propfile, propfile, FALSE);
			g_free (user_propfile);
		}
		prop_read (pr->props_local, propfile, propdir);
		g_free (propdir);
		g_free (propfile);

		propdir = g_strconcat (app->dirs->home, "/.anjuta/", NULL);
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
#warning "G2: Make sure build options are set properly here"
		//preferences_set_build_options (pr);
		pr->priv->is_showing = FALSE;
		create_preferences_gui (pr);
		// preferences_register_all_properties_from_glade_xml (pr, pr->priv->gxml);
		preferences_prop_to_objects (pr);
	}
	return pr;
}

void
preferences_destroy (Preferences * pr)
{
	gint i;
	if (pr)
	{
		prop_set_destroy (pr->props_global);
		prop_set_destroy (pr->props_local);
		prop_set_destroy (pr->props_session);
		prop_set_destroy (pr->props);
		g_list_foreach (pr->priv->properties, (GFunc)property_destroy, NULL);
		gtk_widget_destroy (pr->priv->dialog);
		g_free (pr);
		pr = NULL;
	}
}
