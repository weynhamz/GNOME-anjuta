/***************************************************************************
 *            sourceview-cell.h
 *
 *  So Mai 21 14:44:13 2006
 *  Copyright  2006  Johannes Schmid
 *  jhs@cvs.gnome.org
 ***************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef SOURCEVIEW_CELL_H
#define SOURCEVIEW_CELL_H

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define SOURCEVIEW_TYPE_CELL         (sourceview_cell_get_type ())
#define SOURCEVIEW_CELL(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), SOURCEVIEW_TYPE_CELL, SourceviewCell))
#define SOURCEVIEW_CELL_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), SOURCEVIEW_TYPE_CELL, SourceviewCellClass))
#define SOURCEVIEW_IS_CELL(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), SOURCEVIEW_TYPE_CELL))
#define SOURCEVIEW_IS_CELL_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), SOURCEVIEW_TYPE_CELL))
#define SOURCEVIEW_CELL_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), SOURCEVIEW_TYPE_CELL, SourceviewCellClass))

typedef struct _SourceviewCell SourceviewCell;
typedef struct _SourceviewCellPrivate SourceviewCellPrivate;
typedef struct _SourceviewCellClass SourceviewCellClass;

struct _SourceviewCell {
	GObject parent;
	SourceviewCellPrivate *priv;
};

struct _SourceviewCellClass {
	GObjectClass parent_class;
	/* Add Signal Functions Here */
};

GType sourceview_cell_get_type(void);
SourceviewCell *sourceview_cell_new(GtkTextIter* iter, GtkTextView* view);

GtkTextIter* sourceview_cell_get_iter (SourceviewCell* cell);

G_END_DECLS

#endif /* SOURCEVIEW_CELL_H */
