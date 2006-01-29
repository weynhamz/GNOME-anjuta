/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Jeffrey Stedfast <fejj@ximian.com>
 *
 *  Copyright 2003 Ximian, Inc. (www.ximian.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <popt.h>
#include <gnome.h>
#include <glade/glade.h>
#include <gconf/gconf.h>

#include "alleyoop.h"
#include "vgstrpool.h"


#ifndef POPT_TABLEEND
#define POPT_TABLEEND { NULL, '\0', 0, NULL, '\0', NULL, NULL }
#endif

static const struct poptOption options[] = {
	{ "version", 'v', POPT_ARG_NONE | POPT_ARGFLAG_ONEDASH, NULL, 'v',
	  N_("Print version and exit"), NULL },
	{ "include", 'I', POPT_ARG_STRING | POPT_ARGFLAG_ONEDASH, NULL, 'I',
	  N_("Add <dir> to the list of directories to search for source files"), "<dir>" },
	{ "recursive", 'R', POPT_ARG_STRING | POPT_ARGFLAG_ONEDASH, NULL, 'R',
	  N_("Recursively add <dir> and all subdirectories to the list of directories to search for source files"), "<dir>" },
	{ N_("tool"), '\0', POPT_ARG_STRING | POPT_ARGFLAG_ONEDASH, NULL, 's',
	  N_("Specify the default Valgrind tool to use"), "<name>" },
	POPT_TABLEEND
};

static gboolean
alleyoop_close (GtkWidget *widget, gpointer user_data)
{
	gtk_main_quit ();
	
	return FALSE;
}

static void
add_subdirs (GPtrArray *srcdir, GPtrArray *gc, const char *topsrcdir)
{
	struct dirent *dent;
	GPtrArray *subdirs;
	struct stat st;
	char *path;
	DIR *dir;
	int i;
	
	if (!(dir = opendir (topsrcdir)))
		return;
	
	subdirs = g_ptr_array_new ();
	
	while ((dent = readdir (dir))) {
		if (dent->d_name[0] == '.')
			continue;
		
		path = g_strdup_printf ("%s/%s", topsrcdir, dent->d_name);
		if (stat (path, &st) != -1 && S_ISDIR (st.st_mode)) {
			g_ptr_array_add (subdirs, path);
		} else {
			g_free (path);
		}
	}
	
	closedir (dir);
	
	for (i = 0; i < subdirs->len; i++) {
		path = subdirs->pdata[i];
		g_ptr_array_add (srcdir, path);
		g_ptr_array_add (gc, path);
		add_subdirs (srcdir, gc, path);
	}
	
	g_ptr_array_free (subdirs, TRUE);
}

int main (int argc, char **argv)
{
	const char *tool = NULL;
	GPtrArray *srcdir, *gc;
	GnomeProgram *program;
	GtkWidget *alleyoop;
	const char **args;
	const char *path;
	const char *name;
	char *srcdir_env;
	poptContext ctx;
	int i, rc = 0;
	
#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif
	
	program = gnome_program_init (PACKAGE, VERSION, LIBGNOMEUI_MODULE, argc, argv,
				      GNOME_PARAM_HUMAN_READABLE_NAME, _("Alleyoop"),
				      GNOME_PARAM_APP_DATADIR, PACKAGE_DATA_DIR,
				      GNOME_PARAM_POPT_FLAGS, POPT_CONTEXT_POSIXMEHARDER,
				      GNOME_PARAM_POPT_TABLE, options,
				      NULL);
	
	g_object_get ((GObject *) program, GNOME_PARAM_POPT_CONTEXT, &ctx, NULL);
	
	poptResetContext (ctx);
	
	gc = g_ptr_array_new ();
	srcdir = g_ptr_array_new ();
	while ((rc = poptGetNextOpt (ctx)) > 0) {
		switch (rc) {
		case 'v':
			fprintf (stdout, "%s version %s\n", PACKAGE, VERSION);
			
			for (i = 0; i < gc->len; i++)
				g_free (gc->pdata[i]);
			g_ptr_array_free (gc, TRUE);
			
			g_ptr_array_free (srcdir, TRUE);
			
			poptFreeContext (ctx);
			exit (0);
		case 'I':
			path = poptGetOptArg (ctx);
			g_ptr_array_add (srcdir, (char *) path);
			break;
		case 'R':
			path = poptGetOptArg (ctx);
			g_ptr_array_add (srcdir, (char *) path);
			add_subdirs (srcdir, gc, path);
			break;
		case 's':
			name = poptGetOptArg (ctx);
			if (!strcasecmp (name, "memcheck")) {
				/* default */
				tool = NULL;
			} else if (!strcasecmp (name, "addrcheck")) {
				tool = "addrcheck";
			} else if (!strcasecmp (name, "cachegrind")) {
				/*tool = "cachegrind";*/
				fprintf (stderr, "%s is currently an unsupported tool\n", name);
			} else if (!strcasecmp (name, "helgrind")) {
				/*tool = "helgrind";*/
				fprintf (stderr, "%s is currently an unsupported tool\n", name);
			} else {
				fprintf (stderr, "Unknown tool: %s\n", name);
				
				for (i = 0; i < gc->len; i++)
					g_free (gc->pdata[i]);
				g_ptr_array_free (gc, TRUE);
				
				g_ptr_array_free (srcdir, TRUE);
				
				poptFreeContext (ctx);
				exit (0);
			}
			break;
		}
	}
	
	args = poptGetArgs (ctx);
	
	if ((srcdir_env = getenv ("ALLEYOOP_INCLUDE_PATH"))) {
		/* add our environment to the list of srcdir paths */
		char *path, *p;
		
		path = srcdir_env = g_strdup (srcdir_env);
		do {
			if ((p = strchr (path, ':'))) {
				*p++ = '\0';
				if (path[0] != '\0')
					g_ptr_array_add (srcdir, path);
			} else {
				g_ptr_array_add (srcdir, path);
				break;
			}
			
			path = p;
		} while (path != NULL);
	}
	
	g_ptr_array_add (srcdir, NULL);
	
	glade_init ();
	vg_strpool_init ();
	alleyoop = alleyoop_new (tool, args, (const char **) srcdir->pdata);
	g_signal_connect (alleyoop, "delete-event", G_CALLBACK (alleyoop_close), NULL);
	gtk_widget_show (alleyoop);
	
	gtk_main ();
	
	for (i = 0; i < gc->len; i++)
		g_free (gc->pdata[i]);
	g_ptr_array_free (gc, TRUE);
	
	g_ptr_array_free (srcdir, TRUE);
	vg_strpool_shutdown ();
	g_free (srcdir_env);
	
	g_object_unref (program);
	
	poptFreeContext (ctx);
	
	return 0;
}
