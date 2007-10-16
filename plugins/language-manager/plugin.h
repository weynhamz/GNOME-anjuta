/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * plugin.h
 * Copyright (C) Johannes Schmid 2007 <jhs@gnome.org>
 * 
 * plugin.h is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * plugin.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with plugin.h.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef _LANGUAGE_MANAGER_H_
#define _LANGUAGE_MANAGER_H_

#include <libanjuta/anjuta-plugin.h>

typedef struct _LanguageManager LanguageManager;
typedef struct _LanguageManagerClass LanguageManagerClass;

struct _LanguageManager{
	AnjutaPlugin parent;
	
	GHashTable* languages;
};

struct _LanguageManagerClass{
	AnjutaPluginClass parent_class;
};

extern GType
language_manager_get_type (GTypeModule *module);

#endif
