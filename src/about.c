/*
 *   about.c
 *   Copyright (C) 2000  Kh. Naba Kumar Singh
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gnome.h>
#include "pixmaps.h"
#include "utilities.h"
#include "resources.h"
#include "about.h"

GtkWidget*
create_about_gui ()
{
	const gchar *authors[] = {
		"Naba Kumar <kh_naba@users.sourceforge.net>",
		"Stéphane Démurget <demurgets@free.fr>",
		"Andy Piper <andy.piper@freeuk.com>",
		"Hector Rivera Falu <misha@phreaker.net>", 
		"Luca Bellonda <lbell@tsc4.com>",
		"Rick Patel <rikul@bellsouth.net>", 
		"Johannes Schmid <johannes.schmid@gmx.de>",
		"Max Blagai <maximblagai@yahoo.com>", 
		"Venugopal Gummuluru <vgummuluru@yahoo.com>",
		"Archit Baweja <bighead@crosswinds.net>", 
		"Fatih Demir <kabalak@gtranslator.org>",
		"Martyn Bone <mbone@brightstar.u-net.com>",
		"Denis Boehme <boehme@synci.de>",
		"Gregory Schmitt <gregory.schmitt@free.fr>",
		"Yannick Koehler <yannick.koehler@colubris.com>", 
		"Giovanni Corriga <valkadesh@libero.it>",
		"Dan Elphick <dre00r@ecs.soton.ac.uk>",
		"Jens Georg <mail@jensgeorg.de>",
		"Biswapesh Chattopadhyay <biswapesh_chatterjee@tcscal.co.in>",
		NULL
	};
	GtkWidget *about_gui;
	GtkWidget *vbox1;
	GtkWidget *href1;
	gchar* about_imgfile;

	about_imgfile = anjuta_res_get_pixmap_file (ANJUTA_PIXMAP_LOGO);
	about_gui = gnome_about_new ("Anjuta", VERSION,
		_("Copyright (c) 1999-2001 Naba Kumar"),
		authors,
		NULL, about_imgfile);
	if(about_imgfile)
		g_free (about_imgfile);
	gtk_window_set_modal (GTK_WINDOW (about_gui), FALSE);
	gtk_window_set_position (GTK_WINDOW (about_gui), GTK_WIN_POS_CENTER);
	gtk_window_set_wmclass (GTK_WINDOW (about_gui), "about", "Anjuta");
	
	vbox1 = GNOME_DIALOG (about_gui)->vbox;

	href1 = gnome_href_new ("http://anjuta.sourceforge.net/", _("Anjuta Home Site"));
	gtk_widget_show (href1);
	gtk_box_pack_start (GTK_BOX (vbox1), href1, FALSE, FALSE, 0);
	gtk_box_reorder_child (GTK_BOX (vbox1), href1, 1);
	gtk_box_set_spacing (GTK_BOX (vbox1), 2);

	return about_gui;
}
