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

#include <glib/gdir.h>

#include <gnome.h>
#include <glib/gi18n.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-utils.h>

/*---------------------------------------------------------------------------*/

#define STRING_CHUNK_SIZE	256

/*---------------------------------------------------------------------------*/

struct _NPWPage
{
	GNode* list;
	GStringChunk* string_pool;
	GMemChunk* data_pool;
	GMemChunk* item_pool;
	NPWValueHeap* value;
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
	NPWPage* owner;
	GSList* item;
};

struct _NPWItem {
	const gchar* name;
	const gchar* label;
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
npw_property_new (NPWPage* owner)
{
	NPWProperty* this;

	g_return_val_if_fail (owner, NULL);

	this = g_chunk_new0(NPWProperty, owner->data_pool);
	this->owner = owner;
	this->type = NPW_UNKNOWN_PROPERTY;
	this->restriction = NPW_NO_RESTRICTION;
	this->item = NULL;
	/* value is set to NULL */
	g_node_append_data (owner->list, this);

	return this;
}

void
npw_property_free (NPWProperty* this)
{
	GNode* node;

	if (this->item != NULL)
	{
		g_slist_free (this->item);
	}
	node = g_node_find_child (this->owner->list, G_TRAVERSE_ALL, this);
	if (node != NULL)
	{
		g_node_destroy (node);
		/* Memory allocated in string pool and data pool is not free */
	}
}

void
npw_property_set_type (NPWProperty* this, NPWPropertyType type)
{
	this->type = type;
}

void
npw_property_set_string_type (NPWProperty* this, const gchar* type)
{
	npw_property_set_type (this, npw_property_type_from_string (type));
}


NPWPropertyType
npw_property_get_type (const NPWProperty* this)
{
	return this->type;
}

void
npw_property_set_restriction (NPWProperty* this, NPWPropertyRestriction restriction)
{
	this->restriction = restriction;
}

void
npw_property_set_string_restriction (NPWProperty* this, const gchar* restriction)
{
	npw_property_set_restriction (this, npw_property_restriction_from_string (restriction));
}

NPWPropertyRestriction
npw_property_get_restriction (const NPWProperty* this)
{
	return this->restriction;
}

gboolean
npw_property_is_valid_restriction (const NPWProperty* this)
{
	const gchar *value;

	switch (this->restriction)
	{
	case NPW_FILENAME_RESTRICTION:
		value = npw_property_get_value (this);

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
npw_property_set_name (NPWProperty* this, const gchar* name)
{
	this->value = npw_value_heap_find_value (this->owner->value, name);
}

const gchar*
npw_property_get_name (const NPWProperty* this)
{
	return npw_value_heap_get_name (this->owner->value, this->value);
}

void
npw_property_set_label (NPWProperty* this, const gchar* label)
{
	this->label = g_string_chunk_insert (this->owner->string_pool, label);
}

const gchar*
npw_property_get_label (const NPWProperty* this)
{
	return this->label;
}

void
npw_property_set_description (NPWProperty* this, const gchar* description)
{
	this->description = g_string_chunk_insert (this->owner->string_pool, description);
}

const gchar*
npw_property_get_description (const NPWProperty* this)
{
	return this->description;
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
npw_property_create_widget (NPWProperty* this)
{
	GtkWidget* widget = NULL;
	GtkWidget* entry;
	const gchar* value;

	value = npw_property_get_value (this);
	switch (this->type)
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
		if ((this->options & NPW_EXIST_SET_OPTION) && !(this->options & NPW_EXIST_OPTION))
		{
			GtkWidget *button;
			
			// Use an entry box and a browse button as GtkFileChooserButton
			// allow to select only existing file
			widget = gtk_hbox_new (FALSE, 3);
			
			entry = gtk_entry_new ();
			if (value) gtk_entry_set_text (GTK_ENTRY (entry), value);
			gtk_container_add (GTK_CONTAINER (widget), entry);
			
			button = gtk_button_new_from_stock (GTK_STOCK_OPEN);
			g_signal_connect (button, "clicked", G_CALLBACK (cb_browse_button_clicked), this);
			gtk_container_add (GTK_CONTAINER (widget), button);
			gtk_box_set_child_packing (GTK_BOX (widget), button, FALSE, TRUE, 0, GTK_PACK_END);
		}
		else
		{
			if (this->type == NPW_DIRECTORY_PROPERTY)
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
				gchar* uri = gnome_vfs_make_uri_from_input (value);
				gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (entry), uri);
				g_free (uri);
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
		for (node = this->item; node != NULL; node = node->next)
		{
			gtk_combo_box_append_text (GTK_COMBO_BOX (entry), _(((NPWItem *)node->data)->label));
			if ((value != NULL) && !get_value && (strcmp (value, ((NPWItem *)node->data)->name) == 0))
			{
				value = _(((NPWItem *)node->data)->label);
				get_value = TRUE;
			}
		}
		if (!(this->options & NPW_EDITABLE_OPTION))
		{
			gtk_editable_set_editable (GTK_EDITABLE (GTK_BIN (entry)->child), FALSE);
		}
		if (value) gtk_entry_set_text (GTK_ENTRY (GTK_BIN (entry)->child), value);
		break;
	}
	default:
		return NULL;
	}
	this->widget = entry;
	

	return widget == NULL ? entry : widget;
}

void
npw_property_set_widget (NPWProperty* this, GtkWidget* widget)
{
	this->widget = widget;
}

GtkWidget*
npw_property_get_widget (const NPWProperty* this)
{
	return this->widget;
}

void
npw_property_set_default (NPWProperty* this, const gchar* value)
{
	/* Check if the default property is valid */
	if (value && (this->options & NPW_EXIST_SET_OPTION) && !(this->options & NPW_EXIST_OPTION))
	{
		/* a file or directory with the same name shouldn't exist */
		if (g_file_test (value, G_FILE_TEST_EXISTS))
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
			this->defvalue = g_string_chunk_insert (this->owner->string_pool, buffer);
			g_free (buffer);

			return;
		}
	}
	/* This function could be used with value = defvalue to only check
	 * the default property */
	if (this->defvalue != value)
	{
		this->defvalue = (value == NULL) ? NULL : g_string_chunk_insert (this->owner->string_pool, value);
	}
}

static gboolean
npw_property_set_value_from_widget (NPWProperty* this, NPWValueTag tag)
{
	gchar* alloc_value = NULL;
	const gchar* value = NULL;
	gboolean ok;

	switch (this->type)
	{
	case NPW_INTEGER_PROPERTY:
		alloc_value = g_strdup_printf("%d", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (this->widget)));
		value = alloc_value;
		break;
	case NPW_BOOLEAN_PROPERTY:
		alloc_value = g_strdup_printf("%d", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (this->widget)));
		value = alloc_value;
		break;
	case NPW_STRING_PROPERTY:
		value = gtk_entry_get_text (GTK_ENTRY (this->widget));
		break;
	case NPW_DIRECTORY_PROPERTY:
	case NPW_FILE_PROPERTY:
		if ((this->options & NPW_EXIST_SET_OPTION) && !(this->options & NPW_EXIST_OPTION))
		{
			/* a GtkEntry is used in this case*/
			value = gtk_entry_get_text (GTK_ENTRY (this->widget));
			/* Expand ~ and environment variable */
			alloc_value = anjuta_util_shell_expand (value);
			value = alloc_value;
		}
		else
		{
			alloc_value = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (this->widget));
			value = alloc_value;
		}
		break;
	case NPW_ICON_PROPERTY:
		alloc_value = gnome_icon_entry_get_filename (GNOME_ICON_ENTRY (this->widget));
		value = alloc_value;
		break;
	case NPW_LIST_PROPERTY:
	{
		GSList* node;

		value = gtk_entry_get_text (GTK_ENTRY (GTK_BIN (this->widget)->child));
		for (node = this->item; node != NULL; node = node->next)
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
		value = this->defvalue;
		break;
	}

	/* Check and mark default value (will not be saved) */
	if ((value) && (this->defvalue) && (strcmp (value, this->defvalue) == 0))
	{
		tag |= NPW_DEFAULT_VALUE;
	}
	
	ok = npw_value_heap_set_value (this->owner->value, this->value, value, tag);
	if (alloc_value != NULL) g_free (alloc_value);

	return ok;
}

gboolean
npw_property_update_value_from_widget (NPWProperty* this)
{
	return npw_property_set_value_from_widget (this, NPW_VALID_VALUE);
}

gboolean
npw_property_save_value_from_widget (NPWProperty* this)
{
	return npw_property_set_value_from_widget (this, NPW_OLD_VALUE);
}

gboolean
npw_property_remove_value (NPWProperty* this)
{
	return npw_value_heap_set_value (this->owner->value, this->value, NULL, NPW_EMPTY_VALUE);
}

const char*
npw_property_get_value (const NPWProperty* this)
{
	NPWValueTag tag;

	tag = npw_value_heap_get_tag (this->owner->value, this->value);
	if ((tag == NPW_EMPTY_VALUE) || (tag & NPW_DEFAULT_VALUE))
	{
		return this->defvalue;
	}
	else
	{
		/* Only value entered by user could replace default value */
		return npw_value_heap_get_value (this->owner->value, this->value);
	}	
}

gboolean
npw_property_add_list_item (NPWProperty* this, const gchar* name, const gchar* label)
{
	NPWItem* item;

	item  = g_chunk_new (NPWItem, this->owner->item_pool);
	item->name = g_string_chunk_insert (this->owner->string_pool, name);
	item->label = g_string_chunk_insert (this->owner->string_pool, label);

	this->item = g_slist_append (this->item, item);

	return TRUE;
}

void
npw_property_set_mandatory_option (NPWProperty* this, gboolean value)
{
	if (value)
	{
		this->options |= NPW_MANDATORY_OPTION;
	}
	else
	{
		this->options &= ~NPW_MANDATORY_OPTION;
	}
}

void
npw_property_set_summary_option (NPWProperty* this, gboolean value)
{
	if (value)
	{
		this->options |= NPW_SUMMARY_OPTION;
	}
	else
	{
		this->options &= ~NPW_SUMMARY_OPTION;
	}
}

void
npw_property_set_editable_option (NPWProperty* this, gboolean value)
{
	if (value)
	{
		this->options |= NPW_EDITABLE_OPTION;
	}
	else
	{
		this->options &= ~NPW_EDITABLE_OPTION;
	}
}

NPWPropertyOptions
npw_property_get_options (const NPWProperty* this)
{
	return this->options;
}

void
npw_property_set_exist_option (NPWProperty* this, NPWPropertyBooleanValue value)
{
	switch (value)
	{
	case NPW_TRUE:
		this->options |= NPW_EXIST_OPTION | NPW_EXIST_SET_OPTION;
		break;
	case NPW_FALSE:
		this->options &= ~NPW_EXIST_OPTION;
		this->options |= NPW_EXIST_SET_OPTION;
		npw_property_set_default (this, this->defvalue);
		break;
	case NPW_DEFAULT:
		this->options &= ~(NPW_EXIST_OPTION | NPW_EXIST_SET_OPTION);
		break;
	}
}

NPWPropertyBooleanValue
npw_property_get_exist_option (const NPWProperty* this)
{
	return this->options & NPW_EXIST_SET_OPTION ? (this->options & NPW_EXIST_OPTION ? NPW_TRUE : NPW_FALSE) : NPW_DEFAULT;
}

/* Page object = list of properties
 *---------------------------------------------------------------------------*/

NPWPage*
npw_page_new (NPWValueHeap* value)
{
	NPWPage* this;

	this = g_new0(NPWPage, 1);
	this->string_pool = g_string_chunk_new (STRING_CHUNK_SIZE);
	this->data_pool = g_mem_chunk_new ("property pool", sizeof (NPWProperty), STRING_CHUNK_SIZE * sizeof (NPWProperty) / 4, G_ALLOC_ONLY);
	this->item_pool = g_mem_chunk_new ("item pool", sizeof (NPWItem), STRING_CHUNK_SIZE * sizeof (NPWItem) / 4, G_ALLOC_ONLY);
	this->list = g_node_new (NULL);
	this->value = value;

	return this;
}

void
npw_page_free (NPWPage* this)
{
	g_return_if_fail (this != NULL);

	g_string_chunk_free (this->string_pool);
	g_mem_chunk_destroy (this->data_pool);
	g_mem_chunk_destroy (this->item_pool);
	g_node_destroy (this->list);
	g_free (this);
}

void
npw_page_set_name (NPWPage* this, const gchar* name)
{
	this->name = g_string_chunk_insert (this->string_pool, name);
}

const gchar*
npw_page_get_name (const NPWPage* this)
{
	return this->name;
}

void
npw_page_set_label (NPWPage* this, const gchar* label)
{
	this->label = g_string_chunk_insert (this->string_pool, label);
}

const gchar*
npw_page_get_label (const NPWPage* this)
{
	return this->label;
}

void
npw_page_set_description (NPWPage* this, const gchar* description)
{
	this->description = g_string_chunk_insert (this->string_pool, description);
}

const gchar*
npw_page_get_description (const NPWPage* this)
{
	return this->description;
}

typedef struct _PageForeachPropertyData
{
	NPWPropertyForeachFunc func;
	gpointer data;
} PageForeachPropertyData;

static void
cb_page_foreach_property (GNode* node, gpointer data)
{
	PageForeachPropertyData* d = (PageForeachPropertyData *)data;

	(d->func)((NPWProperty*)node->data, d->data);
}

void
npw_page_foreach_property (const NPWPage* this, NPWPropertyForeachFunc func, gpointer data)
{
	PageForeachPropertyData d;

	d.func = func;
	d.data = data;
	g_node_children_foreach (this->list, G_TRAVERSE_LEAFS, cb_page_foreach_property, &d);
}
