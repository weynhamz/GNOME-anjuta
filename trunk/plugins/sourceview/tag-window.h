/***************************************************************************
 *            tag-window.h
 *
 *  Mi Mär 29 23:23:09 2006
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

#ifndef TAG_WINDOW_H
#define TAG_WINDOW_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtkentrycompletion.h>

G_BEGIN_DECLS

#define TAG_TYPE_WINDOW         (tag_window_get_type ())
#define TAG_WINDOW(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TAG_TYPE_WINDOW, TagWindow))
#define TAG_WINDOW_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TAG_TYPE_WINDOW, TagWindowClass))
#define TAG_IS_WINDOW(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TAG_TYPE_WINDOW))
#define TAG_IS_WINDOW_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TAG_TYPE_WINDOW))
#define TAG_WINDOW_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TAG_TYPE_WINDOW, TagWindowClass))

typedef struct _TagWindow TagWindow;
typedef struct _TagWindowPrivate TagWindowPrivate;
typedef struct _TagWindowClass TagWindowClass;

typedef enum
{
	TAG_WINDOW_KEY_CONTROL,
	TAG_WINDOW_KEY_UPDATE,
	TAG_WINDOW_KEY_SKIP
} TagWindowKeyPress;
	
struct _TagWindow {
	GtkWindow parent;
	TagWindowPrivate *priv;
};

struct _TagWindowClass {
	GtkWindowClass parent_class;
	/* Add Signal Functions Here */
	void (*selected)(GtkWidget* window, const gchar* tag);
	
	/* Virtual functions */
	gboolean (*update_tags)(TagWindow* tagwin, GtkWidget* view);
	gboolean (*filter_keypress)(TagWindow* tagwin, guint keyval);
	void (*move)(TagWindow* tagwin, GtkWidget* view);
};

GType tag_window_get_type(void);

gboolean tag_window_update(TagWindow* tagwin, GtkWidget* view);
TagWindowKeyPress tag_window_filter_keypress(TagWindow* tagwin, guint keyval);

gboolean tag_window_is_active(TagWindow* tagwin);

G_END_DECLS

#endif /* TAG_WINDOW_H */
