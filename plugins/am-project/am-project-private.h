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
#include "command-queue.h"

G_BEGIN_DECLS

typedef enum {
	AM_PROPERTY_IN_CONFIGURE = 1 << 0,
	AM_PROPERTY_IN_MAKEFILE = 1 << 1
} AmpPropertyFlag;


struct _AmpProperty {
	AnjutaProjectProperty base;
	gint token_type;
	gint position;
	const gchar *suffix;
	AmpPropertyFlag flags;
	AnjutaToken *token;
};

struct _AmpProject {
	AmpRootNode base;

	/* GFile corresponding to root configure */
	GFile *configure;
	AnjutaTokenFile *configure_file;
	AnjutaToken *configure_token;

	/* File monitor */
	GFileMonitor *monitor;
	
	AnjutaToken			*ac_init;
	AnjutaToken			*args;

	/* Project file list */
	GList	   *files;

	/* shortcut hash tables, mapping id -> GNode from the tree above */
	GHashTable		*groups;
	GHashTable		*configs;		/* Config file from configure_file */

	/* Number of not loaded node */
	gint				loading;
	
	/* Keep list style */
	AnjutaTokenStyle *ac_space_list;
	AnjutaTokenStyle *am_space_list;
	AnjutaTokenStyle *arg_list;

	/* Command queue */
	PmCommandQueue *queue;
};

typedef struct _AmpNodeInfo AmpNodeInfo;

struct _AmpNodeInfo {
	AnjutaProjectNodeInfo base;
	AnjutaTokenType token;
	const gchar *prefix;
	const gchar *install;
};


G_END_DECLS

#endif /* _AM_PROJECT_PRIVATE_H_ */
