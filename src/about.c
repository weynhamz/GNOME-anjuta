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

#include <gnome.h>
#include <libanjuta/pixmaps.h>
#include <libanjuta/resources.h>
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
	
	pix = anjuta_res_get_pixbuf (ANJUTA_PIXMAP_LOGO);
	dialog = gnome_about_new ("Anjuta", VERSION, 
							  _("Copyright (c) Naba Kumar"),
							  _("Integrated Development Environment"),
							  authors, documentors, NULL, pix);
	return dialog;
}
