/*
    about.c
    Copyright (C) 2002 Naba Kumar

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>

#include <gnome.h>
#include "pixmaps.h"
#include "resources.h"
#include "utilities.h"
#include "about.h"

static const char *authors[] = {
	"Founder and lead developer:",
	"Naba Kumar <naba@gnome.org>",
	"",
	"Developers:",
	"Biswapesh Chattopadhyay <biswapesh_chatterjee@tcscal.co.in>",
	"Johannes Schmid <johannes.schmid@gmx.de>",
	"Stephane Demurget <demurgets@free.fr>",
	"Hector Rivera Falu <misha@phreaker.net> [Website]", 
	"Andy Piper <andy.piper@freeuk.com> [Retired]",
	"",
	"Contributors:",
	"Jean-Noel Guiheneuf <jnoel@saudionline.com.sa>",
	"Timothee Besset <timo@qeradiant.com>",
	"Etay Meiri <etay-m@bezeqint.net>",
	"Dan Elphick <dre00r@ecs.soton.ac.uk>",
	"Michael Tindal <etherscape@paradoxpoint.com>",
	"Jakub Steiner <jimmac * ximian * com>",
	"Luca Bellonda <lbell@tsc4.com>",
	"Rick Patel <rikul@bellsouth.net>", 
	"Max Blagai <maximblagai@yahoo.com>", 
	"Venugopal Gummuluru <vgummuluru@yahoo.com>",
	"Archit Baweja <bighead@crosswinds.net>", 
	"Fatih Demir <kabalak@gtranslator.org>",
	"Martyn Bone <mbone@brightstar.u-net.com>",
	"Denis Boehme <boehme@synci.de>",
	"Gregory Schmitt <gregory.schmitt@free.fr>",
	"Yannick Koehler <yannick.koehler@colubris.com>", 
	"Giovanni Corriga <valkadesh@libero.it>",
	"Jens Georg <mail@jensgeorg.de>",
	"Dick Knol <dknol@gmx.net>",
	"Jason Williams <jason_williams@suth.com>",
	"Philip Van Hoof <freax@pandora.be>",
	"Vadim Berezniker <vadim@berezniker.com>",
	"Rob Bradford <robster@debian.org>",
	"Roel Vanhout <roel@stack.be>",
	"Roy Wood <roy.wood@filogix.com>",
	"Tina Hirsch <cevina@users.sourceforge.net>",
	"Jeroen van der Vegt <A.J.vanderVegt@ITS.TUDelft.nl>",
	"Todd Goyen <goyen@mbi-berlin.de>",
	"Nick Dowell <nixx@nixx.org.uk>",
	"Benke Laszlo <decike@freemail.hu>",
	"Pierre Sarrazin <sarrazip@sympatico.ca>",
	"Kelly Bergougnoux <three3@users.sourceforge.net>",
	"Dave Huseby <huseby@shockfusion.com>",
	"Sebastien Cote <cots01@gel.usherb.ca>",
	"Stephen Knight <steven.knight@unh.edu>",
	NULL
};

static const char *documentors[] = {
	"Naba Kumar <naba@gnome.org>",
	"Andy Piper <andy.piper@freeuk.com> [Retired]",
	"Ishan Chattopadhyaya <ichattopadhyaya@yahoo.com>",
	"Biswapesh Chattopadhyay <biswapesh_chatterjee@tcscal.co.in>",
	NULL
};

GtkWidget *
about_box_new ()
{
	GtkWidget *dialog;
	GdkPixbuf *pix;
	FILE *infile;
	char **names;
	GList *list;
	gboolean allocated = FALSE;
	
	infile = fopen (PACKAGE_DOC_DIR"/AUTHORS", "r");
	if (infile == NULL)
	{
		g_warning ("Can not open AUTHORS file for reading");
		names = (char **)authors;
	}
	else
	{
		GList *node;
		char *line;
		char *utfline;
		int count;
		
		list = NULL;
		while (!feof(infile) && !ferror (infile))
		{
			line = NULL;
			getline (&line, &count, infile);
			if (feof(infile) || ferror (infile))
				break;
			if (count > 0 && strstr (line, "---") == NULL)
			{
				if (line[strlen(line) - 1] == '\n')
					line[strlen(line) - 1] = '\0';
				if (line[0] == '\t')
					line[0] = ' ';
				utfline = anjuta_util_convert_to_utf8 (line);
				if (utfline != NULL)
					list = g_list_prepend (list, utfline);
			}
			g_free (line);
		}
		list = g_list_reverse (list);
		if (g_list_length(list) > 0)
		{
			names = g_new0 (char *, g_list_length (list) + 1);
			node = list;
			count = 0;
			while (node) {
				names[count++] = (char *) node->data;
				node = g_list_next (node);
			}
			names[count] = NULL;
			allocated = TRUE;
		}
		else
		{
			g_warning ("Can not parse AUTHORS file for reading");
			names = (char **)authors;
		}
	}
	pix = anjuta_res_get_pixbuf (ANJUTA_PIXMAP_LOGO);
	dialog = gnome_about_new ("Anjuta", VERSION, 
							  _("Copyright (c) Naba Kumar"),
							  _("Integrated Development Environment"),
							  (const char **)names, documentors, NULL, pix);
	if (allocated)
		glist_strings_free (list);
	return dialog;
}
