/***************************************************************************
 *            sourceview-autocomplete.h
 *
 *  Mo Apr  3 00:46:28 2006
 *  Copyright  2006  Johannes Schmid
 *  jhs@gnome.org
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef SOURCEVIEW_AUTOCOMPLETE_H
#define SOURCEVIEW_AUTOCOMPLETE_H

#include <glib.h>
#include <glib-object.h>

#include "tag-window.h"

G_BEGIN_DECLS

#define SOURCEVIEW_TYPE_AUTOCOMPLETE         (sourceview_autocomplete_get_type ())
#define SOURCEVIEW_AUTOCOMPLETE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), SOURCEVIEW_TYPE_AUTOCOMPLETE, SourceviewAutocomplete))
#define SOURCEVIEW_AUTOCOMPLETE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), SOURCEVIEW_TYPE_AUTOCOMPLETE, SourceviewAutocompleteClass))
#define SOURCEVIEW_IS_AUTOCOMPLETE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), SOURCEVIEW_TYPE_AUTOCOMPLETE))
#define SOURCEVIEW_IS_AUTOCOMPLETE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), SOURCEVIEW_TYPE_AUTOCOMPLETE))
#define SOURCEVIEW_AUTOCOMPLETE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), SOURCEVIEW_TYPE_AUTOCOMPLETE, SourceviewAutocompleteClass))

typedef struct _SourceviewAutocomplete SourceviewAutocomplete;
typedef struct _SourceviewAutocompletePrivate SourceviewAutocompletePrivate;
typedef struct _SourceviewAutocompleteClass SourceviewAutocompleteClass;

struct _SourceviewAutocomplete {
	TagWindow parent;
	SourceviewAutocompletePrivate *priv;
};

struct _SourceviewAutocompleteClass {
	TagWindowClass parent_class;
	/* Add Signal Functions Here */
};

GType sourceview_autocomplete_get_type(void);
SourceviewAutocomplete *sourceview_autocomplete_new(void);

G_END_DECLS

#endif /* SOURCEVIEW_AUTOCOMPLETE_H */
