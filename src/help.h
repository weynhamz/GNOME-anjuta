 /*
  * help.h
  * Copyright (C) 2000  Kh. Naba Kumar Singh
  * 
  * This program is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation; either version 2 of the License, or
  * (at your option) any later version.
  * 
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  * 
  * You should have received a copy of the GNU General Public License
  * along with this program; if not, write to the Free Software
  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  */
#ifndef _ANJUTA_HELP_H_
#define _ANJUTA_HELP_H_

#include <gnome.h>
#include <glade/glade.h>

typedef struct
{
	GtkWidget* window;
	GtkWidget* entry;
	GtkWidget* combo;
	GtkWidget* gnome_radio;
	GtkWidget* man_radio;
	GtkWidget* info_radio;
} AnjutaHelpWidgets;

typedef struct
{
	GladeXML *gxml;
	AnjutaHelpWidgets widgets;
	GList* history;
	gboolean is_showing;
} AnjutaHelp;

AnjutaHelp* anjuta_help_new(void);
void anjuta_help_destroy(AnjutaHelp* help);
void anjuta_help_show(AnjutaHelp* help);
void anjuta_help_hide(AnjutaHelp* help);
gboolean anjuta_help_search(AnjutaHelp* help, const gchar* search_word);

#endif
