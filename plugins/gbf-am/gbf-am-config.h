/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gbf-am-config.h
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef __GBF_AM_CONFIG_H__
#define __GBF_AM_CONFIG_H__

#include <glib.h>

/* config data structures */
typedef enum {
	GBF_AM_TYPE_INVALID,
	GBF_AM_TYPE_STRING,
	GBF_AM_TYPE_MAPPING,
	GBF_AM_TYPE_LIST
} GbfAmValueType;

typedef struct _GbfAmConfigValue    GbfAmConfigValue;
typedef struct _GbfAmConfigMapping  GbfAmConfigMapping;

struct _GbfAmConfigValue {
	GbfAmValueType type;
	gchar               *string;
	GbfAmConfigMapping  *mapping;
	GSList              *list;
};

typedef void (*GbfAmConfigMappingFunc) (const gchar *key,
					GbfAmConfigValue *value,
					gpointer user_data);
/* ---------- public interface */

GbfAmConfigValue   *gbf_am_config_value_new             (GbfAmValueType      type);
void                gbf_am_config_value_free            (GbfAmConfigValue   *value);
GbfAmConfigValue   *gbf_am_config_value_copy            (const GbfAmConfigValue *source);

void                gbf_am_config_value_set_string      (GbfAmConfigValue   *value,
							 const gchar        *string);
void                gbf_am_config_value_set_list        (GbfAmConfigValue   *value,
							 GSList             *list);
void                gbf_am_config_value_set_list_nocopy (GbfAmConfigValue   *value,
							 GSList             *list);
void                gbf_am_config_value_set_mapping     (GbfAmConfigValue   *value,
							 GbfAmConfigMapping *mapping);

#define gbf_am_config_value_get_string(x)  ((const gchar *)(((GbfAmConfigValue *)x)->string))
#define gbf_am_config_value_get_list(x)    ((GSList *)(((GbfAmConfigValue *)x)->list))
#define gbf_am_config_value_get_mapping(x) ((GbfAmConfigMapping *)(((GbfAmConfigValue *)x)->mapping))

GbfAmConfigMapping *gbf_am_config_mapping_new           (void);
void                gbf_am_config_mapping_destroy       (GbfAmConfigMapping *mapping);
GbfAmConfigMapping *gbf_am_config_mapping_copy          (const GbfAmConfigMapping *source);
GbfAmConfigValue   *gbf_am_config_mapping_lookup        (GbfAmConfigMapping *mapping,
							 const gchar        *key);
gboolean            gbf_am_config_mapping_insert        (GbfAmConfigMapping *mapping,
							 const gchar        *key,
							 GbfAmConfigValue   *value);
gboolean            gbf_am_config_mapping_update        (GbfAmConfigMapping *mapping,
							 const gchar        *key,
							 GbfAmConfigValue   *value);
gboolean            gbf_am_config_mapping_remove        (GbfAmConfigMapping *mapping,
							 const gchar        *key);
void                gbf_am_config_mapping_foreach       (GbfAmConfigMapping *mapping,
							 GbfAmConfigMappingFunc callback,
							 gpointer user_data);

#endif /* __GBF_AM_CONFIG_H__ */
