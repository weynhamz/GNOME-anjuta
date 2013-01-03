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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef SOURCEVIEW_PRIVATE_H
#define SOURCEVIEW_PRIVATE_H

#include "anjuta-view.h"

#include "assist-tip.h"
#include "sourceview-cell.h"
#include "sourceview-io.h"

#include <libanjuta/anjuta-plugin.h>
#include <glib.h>

struct SourceviewPrivate {
	/* GtkSouceView */
	AnjutaView* view;

	/* GtkSourceBuffer */
	GtkSourceBuffer* document;

	/* Highlight Tag */
	GtkTextTag *important_indic;
	GtkTextTag *warning_indic;
	GtkTextTag *critical_indic;

	/* IO */
	SourceviewIO* io;
	gboolean read_only;

	/* Preferences */
	GSettings* settings;
	GSettings* msgman_settings;
	GSettings* editor_settings;
	GList* notify_ids;

	/* Popup menu */
	GtkWidget* menu;

	/* Message area */
	GtkWidget* message_area;

	/* Editor window */
	GtkWidget* window;

	/* Goto line hack */
	gboolean loading;
	gint goto_line;

	/* Assist */
	AssistTip* assist_tip;

	/* Hover */
	gchar* tooltip;
	SourceviewCell* tooltip_cell;

	/* To recover the deleted text */
	gchar *deleted_text;

	/* Reload */
	GSList* reload_marks;
	gint reload_line;

	/* Plugin */
	AnjutaPlugin* plugin;
};

#endif
