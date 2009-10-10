/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* am-project-private.h
 *
 * Copyright (C) 2009  Sébastien Granjoux
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
 */

#ifndef _AM_PROJECT_PRIVATE_H_
#define _AM_PROJECT_PRIVATE_H_

#include "am-project.h"

G_BEGIN_DECLS

struct _AmpProperty {
	AnjutaToken *ac_init;				/* AC_INIT macro */
	gchar *name;
	gchar *version;
	gchar *bug_report;
	gchar *tarname;
	gchar *url;
};

struct _AmpProject {
	GObject         parent;

	/* uri of the project; this can be a full uri, even though we
	 * can only work with native local files */
	GFile			*root_file;

	/* project data */
	AnjutaTokenFile		*configure_file;		/* configure.in file */

	AmpProperty			*property;
	
	AmpGroup              *root_node;         	/* tree containing project data;
								 * each GNode's data is a
								 * AmpNode, and the root of
								 * the tree is the root group. */

	/* shortcut hash tables, mapping id -> GNode from the tree above */
	GHashTable		*groups;
	GHashTable		*files;
	GHashTable		*configs;		/* Config file from configure_file */
	
	GHashTable	*modules;
	
	/* project files monitors */
	GHashTable         *monitors;

	/* Keep list style */
	AnjutaTokenStyle *space_list;
	AnjutaTokenStyle *arg_list;
};

G_END_DECLS

#endif /* _AM_PROJECT_PRIVATE_H_ */
