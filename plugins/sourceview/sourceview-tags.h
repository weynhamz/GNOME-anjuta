/*
 * sourceview-tooltip.h (c) 2006 Johannes Schmid
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
#ifndef _SOURCEVIEW_TOOLTIP_H
#define _SOURCEVIEW_TOOLTIP_H

#include "anjuta-view.h"
#include "tag-window.h"
#include <gtk/gtkliststore.h>

gboolean sourceview_tags_update(AnjutaView* view, GtkListStore* store, gchar* current_word);

void sourceview_tags_destroy(void);

#endif /* _SOURCEVIEW_TOOLTIP_H */
 
