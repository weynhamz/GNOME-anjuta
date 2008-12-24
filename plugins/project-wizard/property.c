/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    property.c
    Copyright (C) 2004 Sebastien Granjoux

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*
 * Project properties, used in middle pages
 *
 *---------------------------------------------------------------------------*/

#include <config.h>

#include "property.h"

#include <glib.h>

#include <gnome.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-utils.h>

/*---------------------------------------------------------------------------*/

struct _NPWPage
{
	GList* properties;
	GHashTable* values;
	gchar* name;
	gchar* label;
	gchar* description;
};

struct _NPWProperty {
	NPWPropertyType type;
	NPWPropertyType restriction;
	NPWPropertyOptions options;
	gchar* label;
	gchar* description;
	gchar* defvalue;
	NPWValue* value;
	GtkWidget* widget;
	GSList* items;
};

struct _NPWItem {
	gchar* name;
	gchar* label;
};

static const gchar* NPWPropertyTypeString[] = {
	"hidden",
	"boolean",
	"integer",
	"string",
	"list",
	"directory",
	"file",
	"icon"
};

static const gchar* NPWPropertyRestrictionString[] = {
	"filename"
};

/* Item object
 *---------------------------------------------------------------------------*/

static NPWItem*
npw_item_new (const gchar *name, const gchar *label)
{
	NPWItem *item;
	
	item = g_slice_new (NPWItem);
	item->name = g_strdup (name);
	item->label = g_strdup (label);
	
	return item;
}

static void
npw_item_free (NPWItem *item)
{
	g_free (item->name);
	g_free (item->label);
	g_slice_free (NPWItem, item);
}

/* Property object
 *---------------------------------------------------------------------------*/

static NPWPropertyType
npw_property_type_from_string (const gchar* type)
{
	gint i;

	for (i = 0; i < NPW_LAST_PROPERTY; i++)
	{
		if (strcmp (NPWPropertyTypeString[i], type) == 0)
		{
			return (NPWPropertyType)(i + 1);
		}
	}

	return NPW_UNKNOWN_PROPERTY;
}

static NPWPropertyRestriction
npw_property_restriction_from_string (const gchar* restriction)
{

	if (restriction != NULL)
	{
		gint i;

		for (i = 0; i < NPW_LAST_RESTRICTION; i++)
		{
			if (strcmp (NPWPropertyRestrictionString[i], restriction) == 0)
			{
				return (NPWPropertyRestriction)(i + 1);
			}
		}
	}	

	return NPW_NO_RESTRICTION;
}

NPWProperty*
npw_property_new (void)
{
	NPWProperty* prop;

	prop = g_slice_new0(NPWProperty);
	prop->type = NPW_UNKNOWN_PROPERTY;
	prop->restriction = NPW_NO_RESTRICTION;
	/* value is set to NULL */

	return prop;
}

void
npw_property_free (NPWProperty* prop)
{
	if (prop->items != NULL)
	{
		g_slist_foreach (prop->items, (GFunc)npw_item_free, NULL);
		g_slist_free (prop->items);
	}
	g_free (prop->label);
	g_free (prop->description);
	g_free (prop->defvalue);
	g_slice_free (NPWProperty, prop);
}

void
npw_property_set_type (NPWProperty* prop, NPWPropertyType type)
{
	prop->type = type;
}

void
npw_property_set_string_type (NPWProperty* prop, const gchar* type)
{
	npw_property_set_type (prop, npw_property_type_from_string (type));
}


NPWPropertyType
npw_property_get_type (const NPWProperty* prop)
{
	return prop->type;
}

void
npw_property_set_restriction (NPWProperty* prop, NPWPropertyRestriction restriction)
{
	prop->restriction = restriction;
}

void
npw_property_set_string_restriction (NPWProperty* prop, const gchar* restriction)
{
	npw_property_set_restriction (prop, npw_property_restriction_from_string (restriction));
}

NPWPropertyRestriction
npw_property_get_restriction (const NPWProperty* prop)
{
	return prop->restriction;
}

gboolean
npw_property_is_valid_restriction (const NPWProperty* prop)
{
	const gchar *value;

	switch (prop->restriction)
	{
	case NPW_FILENAME_RESTRICTION:
		value = npw_property_get_value (prop);

		/* First character should be letters, digit or '_' */
		if (value == NULL) return TRUE;
		if (!isalnum (*value) && (*value != '_'))
			return FALSE;

		/* Following characters should be letters, digit or '_'
		 * or '-' or '.' */
		for (value++; *value != '\0'; value++)
		{
			if (!isalnum (*value)
			    && (*value != '_')
			    && (*value != '-')
			    && (*value != '.'))
				return FALSE;
		}
		break;
	default:
		break;
	}

	return TRUE;
}

void
npw_property_set_name (NPWProperty* prop, const gchar* name, NPWPage *page)
{
	prop->value = npw_value_heap_find_value (page->values, name);
}

const gchar*
npw_property_get_name (const NPWProperty* prop)
{
	return npw_value_get_name (prop->value);
}

void
npw_property_set_label (NPWProperty* prop, const gchar* label)
{
	prop->label = g_strdup (label);
}

const gchar*
npw_property_get_label (const NPWProperty* prop)
{
	return prop->label;
}

void
npw_property_set_description (NPWProperty* prop, const gchar* description)
{
	prop->description = g_strdup (description);
}

const gchar*
npw_property_get_description (const NPWProperty* prop)
{
	return prop->description;
}

static void
cb_boolean_button_toggled (GtkButton *button, gpointer data)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
		gtk_button_set_label (button, _("Yes"));
	else
		gtk_button_set_label (button, _("No"));
}

static void
cb_browse_button_clicked (GtkButton *button, NPWProperty* prop)
{
	GtkWidget *dialog;
	
	switch (prop->type)
	{
	case NPW_DIRECTORY_PROPERTY:
		dialog = gtk_file_chooser_dialog_new (_("Select directory"),
												 GTK_WINDOW (gtk_widget_get_ancestor (prop->widget, GTK_TYPE_WINDOW)),
												 GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
												 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				      							 GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				      							 NULL);
		break;
	case NPW_FILE_PROPERTY:
		dialog = gtk_file_chooser_dialog_new (_("Select file"),
												 GTK_WINDOW (gtk_widget_get_ancestor (prop->widget, GTK_TYPE_WINDOW)),
												 GTK_FILE_CHOOSER_ACTION_SAVE,
												 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				      							 GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
				      							 NULL);
		break;
	default:
		g_return_if_reached ();
	}

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
  	{
    	char *filename;

    	filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		gtk_entry_set_text (GTK_ENTRY (prop->widget), filename);
    	g_free (filename);
  	}
	gtk_widget_destroy (dialog);
}

GtkWidget*
npw_property_create_widget (NPWProperty* prop)
{
	GtkWidget* widget = NULL;
	GtkWidget* entry;
	const gchar* value;

	value = npw_property_get_value (prop);
	switch (prop->type)
	{
	case NPW_BOOLEAN_PROPERTY:
		entry = gtk_toggle_button_new_with_label (_("No"));
		g_signal_connect (G_OBJECT (entry), "toggled",
						  G_CALLBACK (cb_boolean_button_toggled), NULL);
		if (value)
		{
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (entry),
										  (gboolean)atoi (value));
		}
		break;
	case NPW_INTEGER_PROPERTY:
		entry = gtk_spin_button_new (NULL, 1, 0);
		if (value)
		{
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (entry), atoi (value));
		}
		break;
	case NPW_STRING_PROPERTY:
		entry = gtk_entry_new ();
		if (value) gtk_entry_set_text (GTK_ENTRY (entry), value);
		break;
	case NPW_DIRECTORY_PROPERTY:
	case NPW_FILE_PROPERTY:
		if ((prop->options & NPW_EXIST_SET_OPTION) && !(prop->options & NPW_EXIST_OPTION))
		{
			GtkWidget *button;
			
			// Use an entry box and a browse button as GtkFileChooserButton
			// allow to select only existing file
			widget = gtk_hbox_new (FALSE, 3);
			
			entry = gtk_entry_new ();
			if (value) gtk_entry_set_text (GTK_ENTRY (entry), value);
			gtk_container_add (GTK_CONTAINER (widget), entry);
			
			button = gtk_button_new_from_stock (GTK_STOCK_OPEN);
			g_signal_connect (button, "clicked", G_CALLBACK (cb_browse_button_clicked), prop);
			gtk_container_add (GTK_CONTAINER (widget), button);
			gtk_box_set_child_packing (GTK_BOX (widget), button, FALSE, TRUE, 0, GTK_PACK_END);
		}
		else
		{
			if (prop->type == NPW_DIRECTORY_PROPERTY)
			{
				entry = gtk_file_chooser_button_new (_("Choose directory"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
			}
			else
			{
				entry = gtk_file_chooser_button_new (_("Choose file"),
												 GTK_FILE_CHOOSER_ACTION_OPEN);
			}
				
			if (value)
			{
				GFile *file = g_file_parse_name (value);
				gchar* uri = g_file_get_uri (file);
				gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (entry), uri);
				g_free (uri);
				g_object_unref (file);
			}
		}
		break;
	case NPW_ICON_PROPERTY:
		entry = gnome_icon_entry_new("icon_choice", _("Icon choice"));
		if (value) gnome_icon_entry_set_filename (GNOME_ICON_ENTRY (entry), value);
		break;
	case NPW_LIST_PROPERTY:
	{
		GSList* node;
		gboolean get_value = FALSE;

		entry = gtk_combo_box_entry_new_text ();
		for (node = prop->items; node != NULL; node = node->next)
		{
			gtk_combo_box_append_text (GTK_COMBO_BOX (entry), _(((NPWItem *)node->data)->label));
			if ((value != NULL) && !get_value && (strcmp (value, ((NPWItem *)node->data)->name) == 0))
			{
				value = _(((NPWItem *)node->data)->label);
				get_value = TRUE;
			}
		}
		if (!(prop->options & NPW_EDITABLE_OPTION))
		{
			gtk_editable_set_editable (GTK_EDITABLE (GTK_BIN (entry)->child), FALSE);
		}
		if (value) gtk_entry_set_text (GTK_ENTRY (GTK_BIN (entry)->child), value);
		break;
	}
	default:
		return NULL;
	}
	prop->widget = entry;
	

	return widget == NULL ? entry : widget;
}

void
npw_property_set_widget (NPWProperty* prop, GtkWidget* widget)
{
	prop->widget = widget;
}

GtkWidget*
npw_property_get_widget (const NPWProperty* prop)
{
	return prop->widget;
}

void
npw_property_set_default (NPWProperty* prop, const gchar* value)
{
	/* Check if the default property is valid */
	if (value && (prop->options & NPW_EXIST_SET_OPTION) && !(prop->options & NPW_EXIST_OPTION))
	{
		gchar *expand_value = anjuta_util_shell_expand (value);
		/* a file or directory with the same name shouldn't exist */
		if (g_file_test (expand_value, G_FILE_TEST_EXISTS))
		{
			char* buffer;
			guint i;

			/* Allocate memory for the string and a decimal number */
			buffer = g_new (char, strlen(value) + 8);
			/* Give up after 1000000 tries */
			for (i = 1; i < 1000000; i++)
			{
				sprintf(buffer,"%s%d",value, i);
				if (!g_file_test (buffer, G_FILE_TEST_EXISTS)) break;
			}
			prop->defvalue = buffer;
			g_free (expand_value);

			return;
		}
		g_free (expand_value);
	}
	/* This function could be used with value = defvalue to only check
	 * the default property */
	if (prop->defvalue != value)
	{
		prop->defvalue = (value == NULL) ? NULL : g_strdup (value);
	}
}

static gboolean
npw_property_set_value_from_widget (NPWProperty* prop, NPWValueTag tag)
{
	gchar* alloc_value = NULL;
	const gchar* value = NULL;
	gboolean ok;

	switch (prop->type)
	{
	case NPW_INTEGER_PROPERTY:
		alloc_value = g_strdup_printf("%d", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (prop->widget)));
		value = alloc_value;
		break;
	case NPW_BOOLEAN_PROPERTY:
		alloc_value = g_strdup_printf("%d", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prop->widget)));
		value = alloc_value;
		break;
	case NPW_STRING_PROPERTY:
		value = gtk_entry_get_text (GTK_ENTRY (prop->widget));
		break;
	case NPW_DIRECTORY_PROPERTY:
	case NPW_FILE_PROPERTY:
		if ((prop->options & NPW_EXIST_SET_OPTION) && !(prop->options & NPW_EXIST_OPTION))
		{
			/* a GtkEntry is used in this case*/
			value = gtk_entry_get_text (GTK_ENTRY (prop->widget));
			/* Expand ~ and environment variable */
			alloc_value = anjuta_util_shell_expand (value);
			value = alloc_value;
		}
		else
		{
			alloc_value = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (prop->widget));
			value = alloc_value;
		}
		break;
	case NPW_ICON_PROPERTY:
		alloc_value = gnome_icon_entry_get_filename (GNOME_ICON_ENTRY (prop->widget));
		value = alloc_value;
		break;
	case NPW_LIST_PROPERTY:
	{
		GSList* node;

		value = gtk_entry_get_text (GTK_ENTRY (GTK_BIN (prop->widget)->child));
		for (node = prop->items; node != NULL; node = node->next)
		{
			if (strcmp (value, _(((NPWItem *)node->data)->label)) == 0)
			{
				value = ((NPWItem *)node->data)->name;
				break;
			}
		}
		break;
	}
	default:
		/* Hidden property */
		value = prop->defvalue;
		break;
	}

	/* Check and mark default value (will not be saved) */
	if ((value) && (prop->defvalue) && (strcmp (value, prop->defvalue) == 0))
	{
		tag |= NPW_DEFAULT_VALUE;
	}
	
	ok = npw_value_set_value (prop->value, value, tag);
	if (alloc_value != NULL) g_free (alloc_value);

	return ok;
}

gboolean
npw_property_update_value_from_widget (NPWProperty* prop)
{
	return npw_property_set_value_from_widget (prop, NPW_VALID_VALUE);
}

gboolean
npw_property_save_value_from_widget (NPWProperty* prop)
{
	return npw_property_set_value_from_widget (prop, NPW_OLD_VALUE);
}

gboolean
npw_property_remove_value (NPWProperty* prop)
{
	return npw_value_set_value (prop->value, NULL, NPW_EMPTY_VALUE);
}

const char*
npw_property_get_value (const NPWProperty* prop)
{
	NPWValueTag tag;

	tag = npw_value_get_tag (prop->value);
	if ((tag == NPW_EMPTY_VALUE) || (tag & NPW_DEFAULT_VALUE))
	{
		return prop->defvalue;
	}
	else
	{
		/* Only value entered by user could replace default value */
		return npw_value_get_value (prop->value);
	}	
}

gboolean
npw_property_add_list_item (NPWProperty* prop, const gchar* name, const gchar* label)
{
	NPWItem* item;

	item = npw_item_new (name, label);
	prop->items = g_slist_append (prop->items, item);

	return TRUE;
}

void
npw_property_set_mandatory_option (NPWProperty* prop, gboolean value)
{
	if (value)
	{
		prop->options |= NPW_MANDATORY_OPTION;
	}
	else
	{
		prop->options &= ~NPW_MANDATORY_OPTION;
	}
}

void
npw_property_set_summary_option (NPWProperty* prop, gboolean value)
{
	if (value)
	{
		prop->options |= NPW_SUMMARY_OPTION;
	}
	else
	{
		prop->options &= ~NPW_SUMMARY_OPTION;
	}
}

void
npw_property_set_editable_option (NPWProperty* prop, gboolean value)
{
	if (value)
	{
		prop->options |= NPW_EDITABLE_OPTION;
	}
	else
	{
		prop->options &= ~NPW_EDITABLE_OPTION;
	}
}

NPWPropertyOptions
npw_property_get_options (const NPWProperty* prop)
{
	return prop->options;
}

void
npw_property_set_exist_option (NPWProperty* prop, NPWPropertyBooleanValue value)
{
	switch (value)
	{
	case NPW_TRUE:
		prop->options |= NPW_EXIST_OPTION | NPW_EXIST_SET_OPTION;
		break;
	case NPW_FALSE:
		prop->options &= ~NPW_EXIST_OPTION;
		prop->options |= NPW_EXIST_SET_OPTION;
		npw_property_set_default (prop, prop->defvalue);
		break;
	case NPW_DEFAULT:
		prop->options &= ~(NPW_EXIST_OPTION | NPW_EXIST_SET_OPTION);
		break;
	}
}

NPWPropertyBooleanValue
npw_property_get_exist_option (const NPWProperty* prop)
{
	return prop->options & NPW_EXIST_SET_OPTION ? (prop->options & NPW_EXIST_OPTION ? NPW_TRUE : NPW_FALSE) : NPW_DEFAULT;
}

/* Page object = list of properties
 *---------------------------------------------------------------------------*/

NPWPage*
npw_page_new (GHashTable* value)
{
	NPWPage* page;

	page = g_new0(NPWPage, 1);
	page->values = value;

	return page;
}

void
npw_page_free (NPWPage* page)
{
	g_return_if_fail (page != NULL);

	g_free (page->name);
	g_free (page->label);
	g_free (page->description);
	g_list_foreach (page->properties, (GFunc)npw_property_free, NULL);
	g_list_free (page->properties);
	g_free (page);
}

void
npw_page_set_name (NPWPage* page, const gchar* name)
{
	page->name = g_strdup (name);
}

const gchar*
npw_page_get_name (const NPWPage* page)
{
	return page->name;
}

void
npw_page_set_label (NPWPage* page, const gchar* label)
{
	page->label = g_strdup (label);
}

const gchar*
npw_page_get_label (const NPWPage* page)
{
	return page->label;
}

void
npw_page_set_description (NPWPage* page, const gchar* description)
{
	page->description = g_strdup (description);
}

const gchar*
npw_page_get_description (const NPWPage* page)
{
	return page->description;
}

void
npw_page_foreach_property (const NPWPage* page, GFunc func, gpointer data)
{
	g_list_foreach (page->properties, func, data);
}

void
npw_page_add_property (NPWPage* page, NPWProperty *prop)
{
	page->properties = g_list_append (page->properties, prop);
}
