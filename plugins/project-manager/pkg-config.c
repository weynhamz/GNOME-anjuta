/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/* pkg-config.c
 *
 * Copyright (C) 2005  Naba Kumar
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "pkg-config.h"

#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

GtkListStore *
packages_get_pkgconfig_list (void)
{
	GtkListStore *store;
	GtkTreeIter iter;
	gchar line[1024];
	gchar *tmpfile, *pkg_cmd;
	FILE *pkg_fd;
	
	store = gtk_list_store_new (N_PKG_COLUMNS, G_TYPE_STRING,
				    G_TYPE_STRING);
	
	/* Now get all the pkg-config info */
	tmpfile = g_strdup_printf ("%s%cpkgmodules--%d", g_get_tmp_dir (),
				   G_DIR_SEPARATOR, getpid());
	pkg_cmd = g_strconcat ("pkg-config --list-all 2>/dev/null | sort > ",
			       tmpfile, NULL);
	if (system (pkg_cmd) == -1)
		return store;
	pkg_fd = fopen (tmpfile, "r");
	if (!pkg_fd) {
		g_warning ("Can not open %s for reading", tmpfile);
		g_free (tmpfile);
		return store;
	}
	while (fgets (line, 1024, pkg_fd)) {
		gchar *name_end;
		gchar *desc_start;
		gchar *description;
		gchar *name;
		
		if (line[0] == '\0')
			continue;
		
		name_end = line;
		while (!isspace(*name_end))
			name_end++;
		desc_start = name_end;
		while (isspace(*desc_start))
			desc_start++;
		
		name = g_strndup (line, name_end-line);
		description = g_strndup (desc_start, strlen (desc_start)-1);
		
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    COL_PKG_PACKAGE, name,
				    COL_PKG_DESCRIPTION, description,
				    -1);
	}
	fclose (pkg_fd);
	unlink (tmpfile);
	g_free (tmpfile);
	return store;
}
