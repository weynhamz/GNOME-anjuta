/***************************************************************************
 *            assist-window.h
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef ASSIST_WINDOW_H
#define ASSIST_WINDOW_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtkentrycompletion.h>
#include <gtk/gtktextview.h>

G_BEGIN_DECLS

#define ASSIST_TYPE_WINDOW         (assist_window_get_type ())
#define ASSIST_WINDOW(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ASSIST_TYPE_WINDOW, AssistWindow))
#define ASSIST_WINDOW_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), ASSIST_TYPE_WINDOW, AssistWindowClass))
#define ASSIST_IS_WINDOW(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ASSIST_TYPE_WINDOW))
#define ASSIST_IS_WINDOW_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ASSIST_TYPE_WINDOW))
#define ASSIST_WINDOW_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ASSIST_TYPE_WINDOW, AssistWindowClass))

typedef struct _AssistWindow AssistWindow;
typedef struct _AssistWindowPrivate AssistWindowPrivate;
typedef struct _AssistWindowClass AssistWindowClass;
	
struct _AssistWindow {
	GtkWindow parent;
	AssistWindowPrivate *priv;
};

struct _AssistWindowClass {
	GtkWindowClass parent_class;
	/* Add Signal Functions Here */
	void (*chosen)(gint selection);
	void (*cancel)();
};

GType assist_window_get_type(void);

void assist_window_update(AssistWindow* assistwin, GList* suggestions);
gboolean assist_window_filter_keypress(AssistWindow* assistwin, guint keyval);
void assist_window_move(AssistWindow* assist_win, gint offset);
const gchar* assist_window_get_trigger(AssistWindow* assist_win);
gint assist_window_get_position(AssistWindow* assist_win);
gboolean assist_window_is_active(AssistWindow* assist_win);

AssistWindow* assist_window_new(GtkTextView* view, gchar* trigger, gint position);


G_END_DECLS

#endif /* ASSIST_WINDOW_H */
