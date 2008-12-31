/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gbf-mkfile-config.h
 *
 * This file is part of the Gnome Build framework
 * Copyright (C) 2005  Eric Greveson
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
 * Author: Eric Greveson
 * Based on the Autotools GBF backend (libgbf-am) by
 *   JP Rosevear
 *   Dave Camp
 *   Naba Kumar
 *   Gustavo Giráldez
 */

#ifndef __GBF_MKFILE_CONFIG_H__
#define __GBF_MKFILE_CONFIG_H__

#include <glib.h>

/* config data structures */
typedef enum {
	GBF_MKFILE_TYPE_INVALID,
	GBF_MKFILE_TYPE_STRING,
	GBF_MKFILE_TYPE_MAPPING,
	GBF_MKFILE_TYPE_LIST
} GbfMkfileValueType;

typedef struct _GbfMkfileConfigValue    GbfMkfileConfigValue;
typedef struct _GbfMkfileConfigMapping  GbfMkfileConfigMapping;

struct _GbfMkfileConfigValue {
	GbfMkfileValueType type;
	gchar               *string;
	GbfMkfileConfigMapping  *mapping;
	GSList              *list;
};

typedef void (*GbfMkfileConfigMappingFunc) (const gchar *key,
					GbfMkfileConfigValue *value,
					gpointer user_data);
/* ---------- public interface */

GbfMkfileConfigValue *gbf_mkfile_config_value_new         (GbfMkfileValueType      type);
void                  gbf_mkfile_config_value_free        (GbfMkfileConfigValue   *value);
GbfMkfileConfigValue *gbf_mkfile_config_value_copy        (const GbfMkfileConfigValue *source);

void                  gbf_mkfile_config_value_set_string      (GbfMkfileConfigValue   *value,
						 const gchar        *string);
void                  gbf_mkfile_config_value_set_list        (GbfMkfileConfigValue   *value,
							 GSList             *list);
void                  gbf_mkfile_config_value_set_list_nocopy (GbfMkfileConfigValue   *value,
							 GSList             *list);
void                  gbf_mkfile_config_value_set_mapping     (GbfMkfileConfigValue   *value,
							 GbfMkfileConfigMapping *mapping);

#define gbf_mkfile_config_value_get_string(x)  ((const gchar *)(((GbfMkfileConfigValue *)x)->string))
#define gbf_mkfile_config_value_get_list(x)    ((GSList *)(((GbfMkfileConfigValue *)x)->list))
#define gbf_mkfile_config_value_get_mapping(x) ((GbfMkfileConfigMapping *)(((GbfMkfileConfigValue *)x)->mapping))

GbfMkfileConfigMapping *gbf_mkfile_config_mapping_new           (void);
void                    gbf_mkfile_config_mapping_destroy       (GbfMkfileConfigMapping *mapping);
GbfMkfileConfigMapping *gbf_mkfile_config_mapping_copy          (const GbfMkfileConfigMapping *source);
GbfMkfileConfigValue   *gbf_mkfile_config_mapping_lookup        (GbfMkfileConfigMapping *mapping,
							 const gchar        *key);
gboolean                gbf_mkfile_config_mapping_insert        (GbfMkfileConfigMapping *mapping,
							 const gchar        *key,
							 GbfMkfileConfigValue   *value);
gboolean                gbf_mkfile_config_mapping_remove        (GbfMkfileConfigMapping *mapping,
							 const gchar        *key);
void                    gbf_mkfile_config_mapping_foreach       (GbfMkfileConfigMapping *mapping,
							 GbfMkfileConfigMappingFunc callback,
							 gpointer user_data);

#endif /* __GBF_MKFILE_CONFIG_H__ */
