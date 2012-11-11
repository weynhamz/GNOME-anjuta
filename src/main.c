/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    main.c
    Copyright (C) 2000  Naba Kumar  <naba@gnome.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef ENABLE_NLS
#  include <locale.h>
#endif

#include <sys/stat.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <libxml/parser.h>
#include <libanjuta/anjuta-debug.h>

#include "anjuta-application.h"

int
main (int argc, char *argv[])
{
	AnjutaApplication* anjuta;
	gint status;

#ifdef ENABLE_NLS
	setlocale (LC_ALL, "");
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	g_set_application_name (_("Anjuta"));

	anjuta = anjuta_application_new ();

	status = g_application_run (G_APPLICATION (anjuta), argc, argv);

	if (anjuta_application_get_proper_shutdown (anjuta))
	{
		g_object_unref (anjuta);

		/* xmlCleanupParser must be called only one time in the application */
		xmlCleanupParser ();
	}

	return status;
}
