/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * preferences.c
 * Copyright (C) 2000 - 2003  Naba Kumar <naba@gnome.org>
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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
/*
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gnome.h>
#include <resources.h>
#include <utilities.h>
#include <style-editor.h>
#include <defaults.h>
#include <pixmaps.h>
*/

#include <libanjuta/resources.h>
#include <libanjuta/anjuta-preferences.h>
#include <glade/glade.h>
#include <glade/glade-parser.h>
#include "preferences.h"

#define GLADE_FILE_ANJUTA              PACKAGE_DATA_DIR"/glade/anjuta.glade"
#define PREFERENCE_PROPERTY_PREFIX     "preferences_"

/*
Function returns icon name *.png to certain preferences chapter
e.g. "General" corresponds to "preferences-general.png"
for future this maybe a map or something like 
but for now let's simply transform prefs_chapter to icon_name 
*/
static gchar*
get_preferences_icon_name (gchar* prefs_chapter)
{
    GString * st=g_string_new(prefs_chapter);
    st=g_string_ascii_down(st);
    st=g_string_prepend(st,"preferences-");
    st=g_string_append(st,".png");
    return st->str;
}

/*
utility function for previous one. The output structure completely 
describes widget that was specified in input by its name
02/04 by Vitaly<vvv@rfniias.ru>
*/
static GladeWidgetInfo*
glade_get_toplevel_by_name (GladeInterface *gi, gchar *name)
{
	int i;
	if(gi == NULL)
		return NULL;
	for(i = 0; i < gi->n_toplevels; i++)
	{
		if (!strcmp (gi->toplevels[i]->name, name))
		return gi->toplevels[i];
	}
	return NULL;
}

/*
input :
    yet created gladeinterface, the name of interested toplevel widget,
    and index of the latter child. 
out: 
    the name of the child

info:
    this provided to make glade extract main frame's names from toplevel 
    names to avoid mess when they are specified by hand
  
02/04 by Vitaly<vvv@rfniias.ru>
*/
static gchar*
glade_get_from_toplevel_child_name_nth(GladeInterface *gi,
									   gchar *toplevel_name, gint idx)
{
	GladeWidgetInfo *wi;
	gint dd = idx + 1;
    
	wi = glade_get_toplevel_by_name (gi, toplevel_name);
	if (wi == NULL)
		return NULL;
	if (idx > wi->children->child->n_children)
		return NULL;
	while (dd--)
	{
		wi = wi->children->child;
	}
	return wi->name;
}

static void
add_all_default_pages (AnjutaPreferences *pr)
{
	GladeXML *gxml;
	GList *node;
	
	GladeInterface *gi;

	gxml = glade_xml_new (GLADE_FILE_ANJUTA, NULL, NULL);
	gi = glade_parser_parse_file (GLADE_FILE_ANJUTA, (const gchar*) NULL);
	
	node = glade_xml_get_widget_prefix (gxml,"preferences_dialog");

	while (node)
	{
		const gchar *name;
		GtkWidget *widget = node->data;
		name = glade_get_widget_name (widget);
		if(!strstr (name, "terminal") && !strstr (name, "encodings"))
			if (strncmp (name, PREFERENCE_PROPERTY_PREFIX,
				strlen (PREFERENCE_PROPERTY_PREFIX)) == 0)
			{
				const gchar* MainFrame =
					glade_get_from_toplevel_child_name_nth (gi,
															(gchar*) name, 0);
				anjuta_preferences_add_page (pr, gxml, MainFrame,
							get_preferences_icon_name ((gchar*) MainFrame));
			}
		node = g_list_next (node);
	}
	g_object_unref (gxml);
}

void
anjuta_preferences_initialize (AnjutaPreferences *pr)
{
	// add_all_default_pages (pr);
	// anjuta_preferences_load_gconf(pr);
}
