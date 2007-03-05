/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#ifndef SOURCEVIEW_PRIVATE_H
#define SOURCEVIEW_PRIVATE_H

#include "anjuta-view.h"
#include "anjuta-document.h"
#include "sourceview-tags.h"
#include "sourceview-scope.h"
#include "sourceview-args.h"
#include "sourceview-autocomplete.h"

#include <libanjuta/anjuta-plugin.h>

struct SourceviewPrivate {
	/* GtkSouceView */
	AnjutaView* view;
	
	/* GtkSourceBuffer */
	AnjutaDocument* document;
	
	/* Filename */
	gchar* filename;
	
	/* Markers */
	GList* markers;
	gint marker_count;
	
	/* Highlight Tag */
	GtkTextTag *important_indic;
	GtkTextTag *warning_indic;
	GtkTextTag *critical_indic;
	
	/* VFS Monitor */
	GnomeVFSMonitorHandle* monitor;
	gchar* last_saved_content;
	gint file_modified_timer;
	gboolean monitor_delay;
	GtkWidget *file_modified_widget;
	
	/* Preferences */
	AnjutaPreferences* prefs;
	GList* gconf_notify_ids;
	
	/* Popup menu */
	GtkWidget* menu;
	
	/* Bookmarks */
	GList* bookmarks;
	GList* cur_bmark;
	
	/* Tag Windows */
	SourceviewTags* tag_window;
	SourceviewAutocomplete* autocomplete;
	SourceviewScope* scope;
	SourceviewArgs* args;
	
	/* Goto line hack */
	gboolean loading;
	gboolean saving;
	gint goto_line;
	
	/* Plugin */
	AnjutaPlugin* plugin;
};

#endif
