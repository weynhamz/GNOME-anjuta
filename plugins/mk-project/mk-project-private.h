/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* mk-project-private.h
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

#ifndef _MK_PROJECT_PRIVATE_H_
#define _MK_PROJECT_PRIVATE_H_

#include "mk-project.h"

G_BEGIN_DECLS

struct _MkpProperty {
	AnjutaToken *ac_init;				/* AC_INIT macro */
	gchar *name;
	gchar *version;
	gchar *bug_report;
	gchar *tarname;
	gchar *url;
};

struct _MkpProject {
	GObject         parent;

	/* uri of the project; this can be a full uri, even though we
	 * can only work with native local files */
	GFile			*root_file;

	/* project data */
	AnjutaTokenFile		*make_file;		/* make file */

	MkpProperty			*property;
	
	MkpGroup              *root_node;         	/* tree containing project data;
								 * each GNode's data is a
								 * AmpNode, and the root of
								 * the tree is the root group. */

	/* shortcut hash tables, mapping id -> GNode from the tree above */
	GHashTable		*groups;
	GHashTable		*files;
	GHashTable		*variables;

	GHashTable		*rules;
	GHashTable		*suffix;
	
	/* project files monitors */
	GHashTable         *monitors;

	/* Keep list style */
	AnjutaTokenStyle *space_list;
	AnjutaTokenStyle *arg_list;
};

struct _MkpRule {
	gchar *name;
	const gchar *part;
	gboolean phony;
	gboolean pattern;
	GList *prerequisite;
	AnjutaToken *rule;
};

gchar *mkp_project_token_evaluate (MkpProject *project, AnjutaToken *token);

MkpTarget* mkp_target_new (const gchar *name, AnjutaProjectTargetType type);
void mkp_target_free (MkpTarget *node);
void mkp_target_add_token (MkpGroup *node, AnjutaToken *token);
MkpSource* mkp_source_new (GFile *file);

G_END_DECLS

#endif /* _MK_PROJECT_PRIVATE_H_ */
