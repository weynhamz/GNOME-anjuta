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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
 * Project properties, used in middle pages
 *
 *---------------------------------------------------------------------------*/

#include <config.h>

#include "property.h"

#include <glib/gdir.h>

#include <gnome.h>
#include <libgnome/gnome-i18n.h>

#include <string.h>
#include <stdlib.h>

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

static const gchar* NPWPropertyTypeString[] = {"hidden",
       						"boolean",
					       	"integer",
					        "string",
						"list",
						"directory",
						"file",
						"icon"};

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

#if 0
static const gchar*
npw_property_string_from_type (NPWPropertyType type)
{
	if ((type > 0) && (type < NPW_LAST_PROPERTY))
	{
		return NPWPropertyTypeString[type - 1];
	}

	return NULL;
}
#endif

NPWProperty*
npw_property_new (NPWPage* owner)
{
	NPWProperty* this;

	g_return_val_if_fail (owner, NULL);

	this = g_chunk_new0(NPWProperty, owner->data_pool);
	this->owner = owner;
	this->type = NPW_UNKNOWN_PROPERTY;
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

GtkWidget*
npw_property_create_widget (NPWProperty* this)
{
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
		entry = gnome_file_entry_new (NULL, NULL);
		gnome_file_entry_set_directory_entry (GNOME_FILE_ENTRY (entry), TRUE);
		if (value) gnome_file_entry_set_filename (GNOME_FILE_ENTRY (entry), value);
		break;
	case NPW_FILE_PROPERTY:
		entry = gnome_file_entry_new (NULL, NULL);
		gnome_file_entry_set_directory_entry (GNOME_FILE_ENTRY (entry), FALSE);
		if (value) gnome_file_entry_set_filename (GNOME_FILE_ENTRY (entry), value);
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

	return entry;
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
		this->defvalue = value == NULL ? NULL : g_string_chunk_insert (this->owner->string_pool, value);
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
		alloc_value = gnome_file_entry_get_full_path (GNOME_FILE_ENTRY (this->widget), FALSE);
		value = alloc_value;
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
