/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gbf-am-config.c
 *
 * This file is part of the Gnome Build framework
 * Copyright (C) 2002  Gustavo Giráldez
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <glib/gi18n.h>

#include "gbf-am-config.h"

typedef struct _GbfAmConfigEntry  GbfAmConfigEntry;

struct _GbfAmConfigEntry {
	gchar            *key;
	GbfAmConfigValue *value;
};

struct _GbfAmConfigMapping {
	GList *pairs;
};


GbfAmConfigValue *
gbf_am_config_value_new (GbfAmValueType type)
{
	GbfAmConfigValue *new_value;

	g_return_val_if_fail (type != GBF_AM_TYPE_INVALID, NULL);
	
	new_value = g_new0 (GbfAmConfigValue, 1);
	new_value->type = type;

	switch (type) {
	    case GBF_AM_TYPE_STRING:
		    new_value->string = NULL;
		    break;
	    case GBF_AM_TYPE_MAPPING:
		    new_value->mapping = gbf_am_config_mapping_new ();
		    break;
	    case GBF_AM_TYPE_LIST:
		    new_value->list = NULL;
		    break;
	    default:
		    break;
	}
	
	return new_value;
}

void 
gbf_am_config_value_free (GbfAmConfigValue *value)
{
	if (value == NULL)
		return;
	
	switch (value->type) {
	    case GBF_AM_TYPE_STRING:
		    g_free (value->string);
		    value->string = NULL;
		    break;
	    case GBF_AM_TYPE_MAPPING:
		    gbf_am_config_mapping_destroy (value->mapping);
		    value->mapping = NULL;
		    break;
	    case GBF_AM_TYPE_LIST:
		    if (value->list) {
			    g_slist_foreach (value->list,
					     (GFunc) gbf_am_config_value_free,
					     NULL);
			    g_slist_free (value->list);
			    value->list = NULL;
		    }
		    break;
	    default:
		    g_warning ("%s", _("Invalid GbfAmConfigValue type"));
		    break;
	}
	g_free (value);
}

GbfAmConfigValue *
gbf_am_config_value_copy (const GbfAmConfigValue *source)
{
	GbfAmConfigValue *value;
	GSList *l;
	
	if (source == NULL)
		return NULL;
	
	value = gbf_am_config_value_new (source->type);
	
	switch (source->type) {
	    case GBF_AM_TYPE_STRING:
		    value->string = g_strdup (source->string);
		    break;
	    case GBF_AM_TYPE_MAPPING:
		    value->mapping = gbf_am_config_mapping_copy (source->mapping);
		    break;
	    case GBF_AM_TYPE_LIST:
		    value->list = NULL;
		    for (l = source->list; l; l = l->next) {
			    GbfAmConfigValue *new_value =
				    gbf_am_config_value_copy ((GbfAmConfigValue *)l->data);
			    value->list = g_slist_prepend (value->list, new_value);
		    }
		    value->list = g_slist_reverse (value->list);
		    break;
	    default:
		    g_warning ("%s", _("Invalid GbfAmConfigValue type"));
		    break;
	}

	return value;
}

void
gbf_am_config_value_set_string (GbfAmConfigValue *value,
				const gchar      *string)
{
	g_return_if_fail (value != NULL && value->type == GBF_AM_TYPE_STRING);

	if (value->string)
		g_free (value->string);
	
	value->string = g_strdup (string);
}

void
gbf_am_config_value_set_list (GbfAmConfigValue *value,
			      GSList           *list)
{
	GSList *l;
	
	g_return_if_fail (value != NULL && value->type == GBF_AM_TYPE_LIST);

	if (value->list) {
		g_slist_foreach (value->list, (GFunc) gbf_am_config_value_free, NULL);
		g_slist_free (value->list);
	}
	
	value->list = NULL;
	for (l = list; l; l = l->next) {
		GbfAmConfigValue *new_value = gbf_am_config_value_copy
			((GbfAmConfigValue *)l->data);
		value->list = g_slist_prepend (value->list, new_value);
	}

	value->list = g_slist_reverse (value->list);
}

void
gbf_am_config_value_set_list_nocopy (GbfAmConfigValue *value,
				     GSList           *list)
{
	g_return_if_fail (value != NULL && value->type == GBF_AM_TYPE_LIST);

	if (value->list) {
		g_slist_foreach (value->list, (GFunc) gbf_am_config_value_free, NULL);
		g_slist_free (value->list);
	}
	value->list = list;
}

void
gbf_am_config_value_set_mapping (GbfAmConfigValue   *value,
				 GbfAmConfigMapping *mapping)
{
	g_return_if_fail (value != NULL && value->type == GBF_AM_TYPE_MAPPING);

	gbf_am_config_mapping_destroy (value->mapping);

	value->mapping = mapping;
}

GbfAmConfigMapping *
gbf_am_config_mapping_new (void)
{
	GbfAmConfigMapping *new_map;

	new_map = g_new0 (GbfAmConfigMapping, 1);
	new_map->pairs = NULL;
	
	return new_map;
}

void 
gbf_am_config_mapping_destroy (GbfAmConfigMapping *mapping)
{
	GbfAmConfigEntry *entry;
	GList            *lp;

	if (mapping == NULL)
		return;

	for (lp = mapping->pairs; lp; lp = lp->next) {
		entry = (GbfAmConfigEntry *) lp->data;
		gbf_am_config_value_free (entry->value);
		g_free (entry->key);
		g_free (entry);
	}
	g_list_free (mapping->pairs);
	g_free (mapping);
}

GbfAmConfigMapping *
gbf_am_config_mapping_copy (const GbfAmConfigMapping *source)
{
	GbfAmConfigMapping *new_map;
	GList              *lp;

	if (source == NULL)
		return NULL;
	
	new_map = g_new0 (GbfAmConfigMapping, 1);
	new_map->pairs = NULL;
	
	for (lp = source->pairs; lp; lp = lp->next) {
		GbfAmConfigEntry *new_entry, *entry;

		entry = (GbfAmConfigEntry *) lp->data;
		if (entry == NULL)
			continue;
		
		new_entry = g_new0 (GbfAmConfigEntry, 1);
		new_entry->key = g_strdup (entry->key);
		new_entry->value = gbf_am_config_value_copy (entry->value);
		new_map->pairs = g_list_prepend (new_map->pairs, new_entry);
	}

	return new_map;
}

GbfAmConfigValue * 
gbf_am_config_mapping_lookup (GbfAmConfigMapping *mapping,
			      const gchar        *key)
{
	GbfAmConfigEntry *entry = NULL;
	GList            *lp;

	g_return_val_if_fail (mapping != NULL && key != NULL, NULL);

	for (lp = mapping->pairs; lp; lp = lp->next) {
		entry = (GbfAmConfigEntry *) lp->data;
		if (!strcmp (entry->key, key))
			break;
	}
	
	return (lp ? entry->value : NULL);
}

gboolean 
gbf_am_config_mapping_insert (GbfAmConfigMapping *mapping,
			      const gchar        *key,
			      GbfAmConfigValue   *value)
{
	GbfAmConfigEntry *entry = NULL;
	GList            *lp;
	gboolean          insert = TRUE;
	
	g_return_val_if_fail (mapping != NULL && key != NULL, FALSE);

	for (lp = mapping->pairs; lp; lp = lp->next) {
		entry = (GbfAmConfigEntry *) lp->data;
		if (!strcmp (entry->key, key)) {
			insert = FALSE;
			break;
		}
	}
	
	if (insert) {
		GbfAmConfigEntry *new_entry;

		new_entry = g_new0 (GbfAmConfigEntry, 1);
		new_entry->key = g_strdup (key);
		new_entry->value = value;
		mapping->pairs = g_list_prepend (mapping->pairs, new_entry);
	}
	
	return insert;
}

gboolean 
gbf_am_config_mapping_update (GbfAmConfigMapping *mapping,
			      const gchar        *key,
			      GbfAmConfigValue   *value)
{
	GbfAmConfigEntry *entry = NULL;
	GList            *lp;
	gboolean         update = TRUE;
	gboolean 	 insert = TRUE;
	
	g_return_val_if_fail (mapping != NULL && key != NULL, FALSE);

	for (lp = mapping->pairs; lp; lp = lp->next) {
		entry = (GbfAmConfigEntry *) lp->data;
		if (!strcmp (entry->key, key)) {
			if (entry->value != value)
			{
				gbf_am_config_value_free (entry->value);
				entry->value = value;
				update = TRUE;	
			}
			insert = FALSE;
			break;
		}
	}
	
	if (insert) {
		GbfAmConfigEntry *new_entry;

		new_entry = g_new0 (GbfAmConfigEntry, 1);
		new_entry->key = g_strdup (key);
		new_entry->value = value;
		mapping->pairs = g_list_prepend (mapping->pairs, new_entry);
	}
	
	return update;
}

gboolean 
gbf_am_config_mapping_remove (GbfAmConfigMapping *mapping,
			      const gchar        *key)
{
	GbfAmConfigEntry *entry = NULL;
	GList            *lp;
	gboolean          remove = FALSE;
	
	g_return_val_if_fail (mapping != NULL && key != NULL, FALSE);

	for (lp = mapping->pairs; lp; lp = lp->next) {
		entry = (GbfAmConfigEntry *) lp->data;
		if (!strcmp (entry->key, key)) {
			remove = TRUE;
			break;
		}
	}
	
	if (remove) {
		gbf_am_config_value_free (entry->value);
		g_free (entry->key);
		g_free (entry);
		mapping->pairs = g_list_delete_link (mapping->pairs, lp);
	}
	
	return remove;
}

void
gbf_am_config_mapping_foreach (GbfAmConfigMapping *mapping,
			       GbfAmConfigMappingFunc callback,
			       gpointer user_data)
{
	GbfAmConfigEntry *entry = NULL;
	GList            *lp;
	
	g_return_if_fail (mapping != NULL && callback != NULL);

	for (lp = mapping->pairs; lp; lp = lp->next) {
		entry = (GbfAmConfigEntry *) lp->data;
		callback (entry->key, entry->value, user_data);
	}
}
