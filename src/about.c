/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    about.c
    Copyright (C) 2002 Naba Kumar   <naba@gnome.org>

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

#include <gnome.h>
#include "about.h"

#define ANJUTA_PIXMAP_LOGO                "anjuta_logo2.png"

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
	
	pix = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"ANJUTA_PIXMAP_LOGO, NULL);
	dialog = gnome_about_new ("Anjuta", VERSION, 
							  _("Copyright (c) Naba Kumar"),
							  _("Integrated Development Environment"),
							  authors, documentors, NULL, pix);
	g_object_unref (pix);
#if 0
	/* GTK provides a new about dialog now, please activate
	when we swith to GTK 2.6 */
	dialog = gtk_about_dialog_new();
	gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(dialog), "Anjuta");
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), VERSION);
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog), 
		_("Copyright (c) Naba Kumar"));
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog),
		_("Integrated Development Environment");
	gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(dialog), 
		_("GNU General Public License"));
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog), "www.anjuta.org");
	gtk_about_dialog_set_icon(GTK_ABOUT_DIALOG(dialog), pix);
	
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dialog), authors);
	gtk_about_dialog_set_documenters(GTK_ABOUT_DIALOG(dialog), documentors);
	/* We should fill this!
	gtk_about_dialog_set_translator_credits(GTK_ABOUT_DIALOG(dialog), ???);
	gtk_about_dialog_set_artists(GTK_ABOUT_DIALOG(dialog), ???);*/
#endif
	
	return dialog;
}
