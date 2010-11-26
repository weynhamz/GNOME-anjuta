/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * plugin.h
 * Copyright (C) Johannes Schmid 2010 <jhs@gnome.org>
 * 
 * code-analyzer is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * code-analyzer is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _CODE_ANALYZER_H_
#define _CODE_ANALYZER_H_

#include <libanjuta/anjuta-plugin.h>

typedef struct _CodeAnalyzerPlugin CodeAnalyzerPlugin;
typedef struct _CodeAnalyzerPluginClass CodeAnalyzerPluginClass;

struct _CodeAnalyzerPlugin{
	AnjutaPlugin parent;

	GSettings* settings;
};

struct _CodeAnalyzerPluginClass{
	AnjutaPluginClass parent_class;
};

#endif
