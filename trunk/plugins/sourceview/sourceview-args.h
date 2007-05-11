/***************************************************************************
 *            sourceview-args.h
 *
 *  Di Apr  4 17:09:23 2006
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

#ifndef SOURCEVIEW_ARGS_H
#define SOURCEVIEW_ARGS_H

#include <glib.h>
#include <glib-object.h>

#include <libanjuta/anjuta-plugin.h>
#include "tag-window.h"
#include "anjuta-view.h"

G_BEGIN_DECLS

#define SOURCEVIEW_TYPE_ARGS         (sourceview_args_get_type ())
#define SOURCEVIEW_ARGS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), SOURCEVIEW_TYPE_ARGS, SourceviewArgs))
#define SOURCEVIEW_ARGS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), SOURCEVIEW_TYPE_ARGS, SourceviewArgsClass))
#define SOURCEVIEW_IS_ARGS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), SOURCEVIEW_TYPE_ARGS))
#define SOURCEVIEW_IS_ARGS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), SOURCEVIEW_TYPE_ARGS))
#define SOURCEVIEW_ARGS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), SOURCEVIEW_TYPE_ARGS, SourceviewArgsClass))

typedef struct _SourceviewArgs SourceviewArgs;
typedef struct _SourceviewArgsPrivate SourceviewArgsPrivate;
typedef struct _SourceviewArgsClass SourceviewArgsClass;

struct _SourceviewArgs {
	TagWindow parent;
	SourceviewArgsPrivate *priv;
};

struct _SourceviewArgsClass {
	TagWindowClass parent_class;
	/* Add Signal Functions Here */
};

GType sourceview_args_get_type(void);
SourceviewArgs *sourceview_args_new(AnjutaPlugin* plugin, AnjutaView* view);

G_END_DECLS

#endif /* SOURCEVIEW_ARGS_H */
