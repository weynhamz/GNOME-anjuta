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

#ifndef HTMLVIEW_H
#define HTMLVIEW_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include <devhelp/dh-html.h>

#include "plugin.h"

G_BEGIN_DECLS

#define HTML_TYPE_VIEW         (html_view_get_type ())
#define HTML_VIEW(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), HTML_TYPE_VIEW, HtmlView))
#define HTML_VIEW_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), HTML_TYPE_VIEW, HtmlViewClass))
#define HTML_IS_VIEW(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), HTML_TYPE_VIEW))
#define HTML_IS_VIEW_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), HTML_TYPE_VIEW))
#define HTML_VIEW_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), HTML_TYPE_VIEW, HtmlViewClass))

typedef struct _HtmlView HtmlView;
typedef struct _HtmlViewPrivate HtmlViewPrivate;
typedef struct _HtmlViewClass HtmlViewClass;

struct _HtmlView {
	GtkHBox parent;
	HtmlViewPrivate *priv;
};

struct _HtmlViewClass {
	GtkHBoxClass parent_class;
};

GType html_view_get_type(void);
GtkWidget *html_view_new(AnjutaDevhelp* devhelp);

DhHtml* html_view_get_dh_html(HtmlView* html_view);

G_END_DECLS

#endif /* HTMLVIEW_H */
