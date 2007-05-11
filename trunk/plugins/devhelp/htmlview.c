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

#include "htmlview.h"

#include <devhelp/dh-book-tree.h>
#include <devhelp/dh-html.h>
#include <devhelp/dh-preferences.h>
#include <devhelp/dh-search.h>
#include <devhelp/dh-base.h>

static void html_view_class_init(HtmlViewClass *klass);
static void html_view_init(HtmlView *sp);
static void html_view_finalize(GObject *object);

struct _HtmlViewPrivate {
	DhHtml* html;
	AnjutaDevhelp* devhelp;
	gchar* uri;
	guint idle_realize;
};

G_DEFINE_TYPE(HtmlView, html_view, GTK_TYPE_HBOX)

static gboolean
devhelp_html_open_uri_cb (DhHtml      *html,
			 const gchar *uri,
			 AnjutaDevhelp    *widget)
{
#ifdef HAVE_OLD_DEVHELP
	dh_book_tree_show_uri (DH_BOOK_TREE (widget->book_tree), uri);
#else
	dh_book_tree_select_uri (DH_BOOK_TREE (widget->book_tree), uri);
#endif
	return FALSE;
}

static void
devhelp_html_location_changed_cb (DhHtml      *html,
				 const gchar *location,
				 AnjutaDevhelp    *widget)
{	
	anjuta_devhelp_check_history (widget);
}

static gboolean
html_view_create_html(HtmlView* html_view)
{
	GtkWidget* view;
	HtmlViewPrivate* priv = html_view->priv;

	priv->html = dh_html_new();
	
	if (!priv->html || !DH_IS_HTML(priv->html))	
		return TRUE; /* I think the idea is to wait until we get a widget? */
	
	view = dh_html_get_widget(priv->html);
	gtk_box_pack_start(GTK_BOX(html_view), dh_html_get_widget(priv->html), TRUE, TRUE, 1);
	
	g_signal_connect (priv->html, "open-uri",
			  G_CALLBACK (devhelp_html_open_uri_cb),
			  priv->devhelp);
	g_signal_connect (priv->html, "location-changed",
			  G_CALLBACK (devhelp_html_location_changed_cb),
			  priv->devhelp);
	
	/* Hack to get GtkMozEmbed to work properly. */
	gtk_widget_realize (view);

	if (priv->uri)
		dh_html_open_uri(priv->html, priv->uri);
	else
		dh_html_clear(priv->html);
	
	gtk_widget_show (view);
	
	return FALSE;
}

static void
html_view_realize(GtkWidget* widget)
{
	HtmlView* html_view = HTML_VIEW(widget);
	
	if (html_view->priv->idle_realize == 0)
	{
		html_view->priv->idle_realize =
			g_idle_add((GSourceFunc) html_view_create_html, html_view);
	}	
	(* GTK_WIDGET_CLASS (html_view_parent_class)->realize)(widget);
}

static void
html_view_unrealize(GtkWidget* widget)
{
	HtmlView* html_view = HTML_VIEW(widget);
	
	if (html_view->priv->idle_realize > 0)
	{
		g_source_remove (html_view->priv->idle_realize);
		html_view->priv->idle_realize = 0;
	}
	
	if (html_view->priv->html != NULL)
	{
		g_free(html_view->priv->uri);
		html_view->priv->uri = dh_html_get_location(html_view->priv->html);
	}
	else
		html_view->priv->uri = NULL;
	
	if (gtk_container_get_children(GTK_CONTAINER(html_view)))
	{
		gtk_container_remove(GTK_CONTAINER(widget), dh_html_get_widget(html_view->priv->html));
		html_view->priv->html = NULL;
	}

	(* GTK_WIDGET_CLASS (html_view_parent_class)->unrealize)(widget);
}

static void
html_view_class_init(HtmlViewClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS(klass);

	object_class->finalize = html_view_finalize;
	
	widget_class->realize = html_view_realize;
	widget_class->unrealize = html_view_unrealize;
}

static void
html_view_init(HtmlView *obj)
{
	obj->priv = g_new0(HtmlViewPrivate, 1);
	gtk_widget_show(GTK_WIDGET(obj));
}

static void
html_view_finalize(GObject *object)
{
	HtmlView *cobj;
	cobj = HTML_VIEW(object);
	
	/* Free private members, etc. */
	if (cobj->priv->html)
		gtk_widget_destroy(dh_html_get_widget(cobj->priv->html));
	g_free (cobj->priv->uri);	
	g_free(cobj->priv);
	G_OBJECT_CLASS(html_view_parent_class)->finalize(object);
}

GtkWidget*
html_view_new(AnjutaDevhelp* devhelp)
{
	HtmlView *obj;
	
	obj = HTML_VIEW(g_object_new(HTML_TYPE_VIEW, NULL));
	
	obj->priv->devhelp = devhelp;
	
	return GTK_WIDGET(obj);
}

DhHtml* html_view_get_dh_html(HtmlView* view)
{
	return view->priv->html;
}
