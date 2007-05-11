/***************************************************************************
 *            sourceview-scope.h
 *
 *  So Apr  2 10:56:47 2006
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

#ifndef SOURCEVIEW_SCOPE_H
#define SOURCEVIEW_SCOPE_H

#include <glib.h>
#include <glib-object.h>

#include <libanjuta/anjuta-plugin.h>

#include "tag-window.h"
#include "anjuta-view.h"

G_BEGIN_DECLS

#define SOURCEVIEW_TYPE_SCOPE         (sourceview_scope_get_type ())
#define SOURCEVIEW_SCOPE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), SOURCEVIEW_TYPE_SCOPE, SourceviewScope))
#define SOURCEVIEW_SCOPE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), SOURCEVIEW_TYPE_SCOPE, SourceviewScopeClass))
#define SOURCEVIEW_IS_SCOPE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), SOURCEVIEW_TYPE_Scope))
#define SOURCEVIEW_IS_SCOPE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), SOURCEVIEW_TYPE_Scope))
#define SOURCEVIEW_SCOPE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), SOURCEVIEW_TYPE_TAGS, SourceviewScopeClass))

typedef struct _SourceviewScope SourceviewScope;
typedef struct _SourceviewScopePrivate SourceviewScopePrivate;
typedef struct _SourceviewScopeClass SourceviewScopeClass;

struct _SourceviewScope {
	TagWindow parent;
	
	SourceviewScopePrivate *priv;
};

struct _SourceviewScopeClass {
	TagWindowClass parent_class;
	/* Add Signal Functions Here */
};

GType sourceview_scope_get_type(void);
SourceviewScope *sourceview_scope_new(AnjutaPlugin* plugin, AnjutaView* view);

G_END_DECLS

#endif /* SOURCEVIEW_TAGS_H */
