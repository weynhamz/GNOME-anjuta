/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    anjuta-docman.c
    Copyright (C) 2003-2007 Naba Kumar <naba@gnome.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <libanjuta/resources.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-markable.h>
#include <libanjuta/interfaces/ianjuta-file-savable.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-editor-factory.h>

#include <gtk/gtkfilechooserdialog.h>
#include <libgnomevfs/gnome-vfs.h>

#include <gdl/gdl-icons.h>

#include "anjuta-docman.h"
#include "file_history.h"
#include "plugin.h"
#include "action-callbacks.h"
#include "editor-tooltips.h"

static gpointer parent_class;

enum
{
	DOC_ADDED,
	DOC_CHANGED,
	LAST_SIGNAL
};

/* Preference keys */
#define EDITOR_TABS_POS            "editor.tabs.pos"
#define EDITOR_TABS_HIDE           "editor.tabs.hide"
#define EDITOR_TABS_ORDERING       "editor.tabs.ordering"
#define EDITOR_TABS_RECENT_FIRST   "editor.tabs.recent.first"

typedef struct _AnjutaDocmanPage AnjutaDocmanPage;

struct _AnjutaDocmanPriv {
	DocmanPlugin *plugin;
	AnjutaPreferences *preferences;
	GList *pages;		/* list of AnjutaDocmanPage's */
	AnjutaDocmanPage *cur_page;
	IAnjutaDocument *current_document; /* normally == IANJUTA_DOCUMENT (cur_page->widget) */
	
	GtkWidget *fileselection;
	
	GtkWidget *popup_menu;	/* shared context-menu for main-notebook pages */
	gboolean tab_pressed;	/* flag for deferred notebook page re-arrangement */
	gboolean shutingdown;
};

struct _AnjutaDocmanPage {
	GtkWidget *widget;	/* notebook-page widget, a GTK_WIDGET (IAnjutaDocument*) */
	GtkWidget *box;		/* notebook-tab-label parent widget */
	GtkWidget *close_image;
	GtkWidget *close_button;
	GtkWidget *mime_icon;
	GtkWidget *label;
	GtkWidget *menu_label;	/* notebook page-switch menu-label */
	gboolean is_current;
};

static guint docman_signals[LAST_SIGNAL] = { 0 };

static void anjuta_docman_order_tabs (AnjutaDocman *docman);
static void anjuta_docman_update_page_label (AnjutaDocman *docman,
											 GtkWidget *doc_widget);
static void anjuta_docman_grab_text_focus (AnjutaDocman *docman);
static gboolean anjuta_docman_sort_pagelist (AnjutaDocman *docman);
static void on_notebook_switch_page (GtkNotebook *notebook,
									 GtkNotebookPage *page,
									 gint page_num, AnjutaDocman *docman);
static AnjutaDocmanPage *
anjuta_docman_get_page_for_document (AnjutaDocman *docman,
									IAnjutaDocument *doc);

static void
on_notebook_page_close_button_click (GtkButton* button,
									AnjutaDocman* docman)
{
	AnjutaDocmanPage *page;

	page = docman->priv->cur_page;
	if (page == NULL || page->close_button != GTK_WIDGET (button))
	{
		/* the close function works only on the current document */
		GList* node;
		for (node = docman->priv->pages; node != NULL; node = g_list_next (node))
		{
			IAnjutaDocument *doc;
			page = (AnjutaDocmanPage *) node->data;
			if (page->close_button == GTK_WIDGET (button))
			{
				doc = IANJUTA_DOCUMENT (page->widget);
				anjuta_docman_set_current_document (docman, doc);
				break;
			}
		}
		if (node == NULL)
			return;
	}

	if (page != NULL)
		on_close_file_activate (NULL, docman->priv->plugin);
}

static void
on_notebook_page_close_button_enter (GtkButton *button,
									 AnjutaDocmanPage *page)
{
	g_return_if_fail (page != NULL);
	
	gtk_widget_set_sensitive (page->close_image, TRUE);
}

static void
on_notebook_page_close_button_leave (GtkButton* button,
									 AnjutaDocmanPage *page)
{
	g_return_if_fail (page != NULL);
	
	if (! page->is_current)
		gtk_widget_set_sensitive (page->close_image,FALSE);
}

/* for managing deferred tab re-arrangement */
static gboolean
on_notebook_tab_btnpress (GtkWidget *wid, GdkEventButton *event, AnjutaDocman* docman)
{
	if (event->type == GDK_BUTTON_PRESS && event->button != 3)	/* right-click is for menu */
		docman->priv->tab_pressed = TRUE;

	return FALSE;
}

static gboolean
on_notebook_tab_btnrelease (GtkWidget *widget, GdkEventButton *event, AnjutaDocman* docman)
{
	docman->priv->tab_pressed = FALSE;

	if (anjuta_preferences_get_int (docman->priv->preferences, EDITOR_TABS_RECENT_FIRST))
	{
		GList *node;

		for (node = docman->priv->pages; node != NULL; node = g_list_next (node))
		{
			AnjutaDocmanPage *page;

			page = (AnjutaDocmanPage *)node->data;
			if (page->box == widget)
			{
				gtk_notebook_reorder_child (GTK_NOTEBOOK (docman), page->widget, 0);
				break;
			}
		}
	}

	return FALSE;
}

static void
on_notebook_page_reordered (GtkNotebook *notebook, GtkWidget *child,
							guint page_num, AnjutaDocman *docman)
{
	/* conform pagelist order */
	g_idle_add ((GSourceFunc) anjuta_docman_sort_pagelist, docman);
}

static gint
anjuta_docman_compare_pages (AnjutaDocmanPage *a, AnjutaDocmanPage *b, AnjutaDocman *docman)
{
	gint pa, pb;
	GtkNotebook *book;

	book = GTK_NOTEBOOK (docman);
	pa = gtk_notebook_page_num (book, a->widget);
	pb = gtk_notebook_page_num (book, b->widget);
	return ((pa < pb) ? 1:-1);
}

/* this is for idle or timer callback, as well as direct usage */
static gboolean
anjuta_docman_sort_pagelist (AnjutaDocman *docman)
{
	DEBUG_PRINT ("In function: anjuta_docman_sort_pagelist");
	if (docman->priv->pages != NULL && g_list_next (docman->priv->pages) != NULL)
		docman->priv->pages = g_list_sort_with_data (
								docman->priv->pages,
								(GCompareDataFunc) anjuta_docman_compare_pages,
								docman);
	return FALSE;
}

static void
anjuta_docman_page_init (AnjutaDocman *docman, IAnjutaDocument *doc,
						 const gchar *uri, AnjutaDocmanPage *page)
{
	GtkWidget *close_button;
	GtkWidget *close_pixmap;
	GtkRcStyle *rcstyle;
	GtkWidget *label, *menu_label;
	GtkWidget *box;
	GtkWidget *event_hbox;
	GtkWidget *event_box;
#if !GTK_CHECK_VERSION (2,12,0)
	static GtkTooltips *tooltips = NULL;
#endif
	gint h, w;
	GdkColor color;
	const gchar *filename;
	gchar *uuri, *ruri;
	
	g_return_if_fail (IANJUTA_IS_DOCUMENT (doc));
	
	gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &w, &h);

	close_pixmap = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
	gtk_widget_show (close_pixmap);
	
	/* setup close button, zero out {x,y} thickness to get smallest possible size */
	close_button = gtk_button_new();
	gtk_button_set_focus_on_click (GTK_BUTTON (close_button), FALSE);
	gtk_container_add(GTK_CONTAINER(close_button), close_pixmap);
	gtk_button_set_relief(GTK_BUTTON(close_button), GTK_RELIEF_NONE);
	rcstyle = gtk_rc_style_new ();
	rcstyle->xthickness = rcstyle->ythickness = 0;
	gtk_widget_modify_style (close_button, rcstyle);
	g_object_unref (G_OBJECT (rcstyle));
	
	gtk_widget_set_size_request (close_button, w, h);
#if GTK_CHECK_VERSION (2,12,0)
	gtk_widget_set_tooltip_text (close_button, _("Close file"));
#else
	if (tooltips == NULL)
		tooltips = gtk_tooltips_new ();
	gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips), close_button, _("Close file"),
						  NULL);
#endif
	filename = ianjuta_document_get_filename (doc, NULL);
	label = gtk_label_new (filename);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_widget_show (label);

	menu_label = gtk_label_new (filename);
	gtk_widget_show (menu_label);
  	 
	color.red = 0;
	color.green = 0;
	color.blue = 0;
	
	gtk_widget_modify_fg (close_button, GTK_STATE_NORMAL, &color);
	gtk_widget_modify_fg (close_button, GTK_STATE_INSENSITIVE, &color);
	gtk_widget_modify_fg (close_button, GTK_STATE_ACTIVE, &color);
	gtk_widget_modify_fg (close_button, GTK_STATE_PRELIGHT, &color);
	gtk_widget_modify_fg (close_button, GTK_STATE_SELECTED, &color);
	gtk_widget_show(close_button);
	
	box = gtk_hbox_new (FALSE, 2);
	/* create our layout/event boxes */
	event_box = gtk_event_box_new();
	gtk_event_box_set_visible_window (GTK_EVENT_BOX (event_box), FALSE);

	event_hbox = gtk_hbox_new (FALSE, 2);	
	
	uuri = (uri) ? (gchar *)uri : ianjuta_file_get_uri (IANJUTA_FILE (doc), NULL);
	if (uuri != NULL)
	{
		/* add a nice mime-type icon if we can */
		GdlIcons *icons;
		GdkPixbuf *pixbuf;
		icons = gdl_icons_new (16);
		pixbuf = gdl_icons_get_uri_icon (icons, uuri);
		if (pixbuf != NULL)
		{
			GtkWidget *image;
			image = gtk_image_new_from_pixbuf (pixbuf);
			gtk_box_pack_start (GTK_BOX (event_hbox), image, FALSE, FALSE, 0);
			page->mime_icon = image;
			g_object_unref (G_OBJECT (pixbuf));
		}
		g_object_unref (G_OBJECT (icons));
		ruri = gnome_vfs_format_uri_for_display (uuri);
		if (uuri != uri)
			g_free (uuri);
		if (ruri != NULL)
		{
			/* set the tab-tooltip */
			gchar *tip;
#if !GTK_CHECK_VERSION (2,12,0)
			EditorTooltips *tooltips;
			tooltips = editor_tooltips_new ();	/*CHECKME is this ever cleaned ?*/
#endif
			tip = g_markup_printf_escaped ("<b>%s</b> %s", _("Path:"), ruri);
#if GTK_CHECK_VERSION (2,12,0)
			gtk_widget_set_tooltip_markup (event_box, tip);
#else
			editor_tooltips_set_tip (tooltips, event_box, tip, NULL);
#endif
			g_free (ruri);
			g_free (tip);
		}
	}
	
	gtk_box_pack_start (GTK_BOX (event_hbox), label, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (event_hbox), close_button, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (event_box), event_hbox);
	
	/* setup the data hierarchy */
	g_object_set_data (G_OBJECT (box), "event_box", event_box);
	
	/* pack our top-level layout box */
	gtk_box_pack_start (GTK_BOX (box), event_box, TRUE, TRUE, 0);
	
	/* show the widgets of the tab */
	gtk_widget_show_all(box);

	g_signal_connect (G_OBJECT (close_button), "clicked",
					  G_CALLBACK (on_notebook_page_close_button_click),
					  docman);
	g_signal_connect (G_OBJECT (close_button), "enter",
					  G_CALLBACK (on_notebook_page_close_button_enter),
					  page);
	g_signal_connect (G_OBJECT (close_button), "leave",
					  G_CALLBACK (on_notebook_page_close_button_leave),
					  page);
	g_signal_connect (G_OBJECT (box), "button-press-event",
					  G_CALLBACK (on_notebook_tab_btnpress),
					  docman);
	g_signal_connect (G_OBJECT (box), "button-release-event",
					  G_CALLBACK (on_notebook_tab_btnrelease),
					  docman);

	page->widget = GTK_WIDGET (doc);	/* this is also the notebook-page child widget */
	page->box = box;
	page->close_image = close_pixmap;
	page->close_button = close_button;
	page->label = label;
	page->menu_label = menu_label;

	gtk_widget_show (page->widget);
}

static AnjutaDocmanPage *
anjuta_docman_page_new (void)
{
	AnjutaDocmanPage *page;
	
	page = g_new0 (AnjutaDocmanPage, 1); /* don't try to survive a memory-crunch */
	return page;
}

static void
anjuta_docman_page_destroy (AnjutaDocmanPage *page)
{
	if (page)
	{
		if (page->widget && GTK_IS_WIDGET (page->widget))
		/* CHECKME just unref in case other plugins hold ref on the page or its contents ? */
			gtk_widget_destroy (page->widget);
		if (page->box && GTK_IS_WIDGET (page->box))
			gtk_widget_destroy (page->box);	/* the tab-label parent-widget */
		g_free (page);
	}
}

static void
on_open_filesel_response (GtkDialog* dialog, gint id, AnjutaDocman *docman)
{
	gchar *uri;
// unused	gchar *entry_filename = NULL;
	int i;
	GSList * list;
	int elements;

	if (id != GTK_RESPONSE_ACCEPT)
	{
		gtk_widget_hide (docman->priv->fileselection);
		return;
	}
	
	list = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(dialog));
	if (list != NULL)
	{
		elements = g_slist_length(list);
		for (i = 0; i < elements; i++)
		{
			uri = g_slist_nth_data (list, i);
			if (uri)
			{
				anjuta_docman_goto_file_line (docman, uri, -1);
				g_free (uri);
			}
		}
		g_slist_free (list);
	
/*		if (entry_filename)
		{
			list = g_slist_remove(list, entry_filename);
			g_free(entry_filename);
		}
*/
	}
}

static GtkWidget*
create_file_open_dialog_gui (GtkWindow* parent, AnjutaDocman* docman)
{
	GtkWidget* dialog = 
		gtk_file_chooser_dialog_new (_("Open file"), 
									parent,
									GTK_FILE_CHOOSER_ACTION_OPEN,
									GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
									GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
									NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
	gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER(dialog), TRUE);
	g_signal_connect (G_OBJECT (dialog), "response",
					  G_CALLBACK (on_open_filesel_response), docman);
	g_signal_connect_swapped (G_OBJECT (dialog), "delete-event",
							  G_CALLBACK (gtk_widget_hide), dialog);
	return dialog;
}

static GtkWidget*
create_file_save_dialog_gui (GtkWindow* parent, AnjutaDocman* docman)
{
	GtkWidget* dialog = 
		gtk_file_chooser_dialog_new (_("Save file as"), 
									parent,
									GTK_FILE_CHOOSER_ACTION_SAVE,
									GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
									GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
									NULL); 
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
	return dialog;
}

void
anjuta_docman_open_file (AnjutaDocman *docman)
{
	if (!docman->priv->fileselection)
	{
		GtkWidget *parent;
		parent = gtk_widget_get_toplevel (GTK_WIDGET (docman));
		docman->priv->fileselection =
			create_file_open_dialog_gui(GTK_WINDOW (parent), docman);
	}
	if (GTK_WIDGET_VISIBLE (docman->priv->fileselection))
		gtk_window_present (GTK_WINDOW (docman->priv->fileselection));
	else
		gtk_widget_show (docman->priv->fileselection);
}

gboolean
anjuta_docman_save_document_as (AnjutaDocman *docman, IAnjutaDocument *doc,
							  GtkWidget *parent_window)
{
	gchar* uri;
	GnomeVFSURI* vfs_uri;
	gchar* file_uri;
	const gchar* filename;
	GtkWidget *parent;
	GtkWidget *dialog;
	gint response;
	gboolean file_saved = TRUE;
	
	g_return_val_if_fail (ANJUTA_IS_DOCMAN (docman), FALSE);
	g_return_val_if_fail (IANJUTA_IS_DOCUMENT (doc), FALSE);
	
	if (parent_window)
	{
		parent = parent_window;
	}
	else
	{
		parent = gtk_widget_get_toplevel (GTK_WIDGET (docman));
	}
	
	dialog = create_file_save_dialog_gui (GTK_WINDOW (parent), docman);
	
	if ((file_uri = ianjuta_file_get_uri (IANJUTA_FILE (doc), NULL)) != NULL)
	{
		gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (dialog), file_uri);
		g_free (file_uri);
	}
	else if ((filename = ianjuta_document_get_filename (doc, NULL)) != NULL)
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), filename);
	else
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), "");
	
	response = gtk_dialog_run (GTK_DIALOG (dialog));
	if (response != GTK_RESPONSE_ACCEPT)
	{
		gtk_widget_destroy (dialog);
		return FALSE;
	}
	
	uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
	vfs_uri = gnome_vfs_uri_new(uri);
	if (gnome_vfs_uri_exists(vfs_uri))
	{
		GtkWidget *msg_dialog;
		msg_dialog = gtk_message_dialog_new (GTK_WINDOW (dialog),
											 GTK_DIALOG_DESTROY_WITH_PARENT,
											 GTK_MESSAGE_QUESTION,
											 GTK_BUTTONS_NONE,
											 _("The file '%s' already exists.\n"
											 "Do you want to replace it with the"
											 " one you are saving?"),
											 uri);
		gtk_dialog_add_button (GTK_DIALOG (msg_dialog),
							   GTK_STOCK_CANCEL,
							   GTK_RESPONSE_CANCEL);
		anjuta_util_dialog_add_button (GTK_DIALOG (msg_dialog),
								  _("_Replace"),
								  GTK_STOCK_REFRESH,
								  GTK_RESPONSE_YES);
		if (gtk_dialog_run (GTK_DIALOG (msg_dialog)) == GTK_RESPONSE_YES)
			ianjuta_file_savable_save_as (IANJUTA_FILE_SAVABLE (doc), uri,
										  NULL);
		else
			file_saved = FALSE;
		gtk_widget_destroy (msg_dialog);
	}
	else
	{
		ianjuta_file_savable_save_as (IANJUTA_FILE_SAVABLE (doc), uri, NULL);
	}
	
	if (anjuta_preferences_get_int (ANJUTA_PREFERENCES (docman->priv->preferences),
									EDITOR_TABS_ORDERING))
		anjuta_docman_order_tabs (docman);

	gtk_widget_destroy (dialog);
	g_free (uri);
	gnome_vfs_uri_unref (vfs_uri);
	return file_saved;
}

gboolean
anjuta_docman_save_document (AnjutaDocman *docman, IAnjutaDocument *doc,
						   GtkWidget *parent_window)
{
	gchar *uri;
	gboolean ret;
	
	uri = ianjuta_file_get_uri (IANJUTA_FILE (doc), NULL);
	
	if (uri == NULL)
	{
		anjuta_docman_set_current_document (docman, doc);
		ret = anjuta_docman_save_document_as (docman, doc, parent_window);
	}
	else
	{
		/* TODO: Error checking */
		ianjuta_file_savable_save (IANJUTA_FILE_SAVABLE (doc), NULL);
		g_free (uri);
		ret = TRUE;
	}
	return ret;
}

static void
anjuta_docman_dispose (GObject *obj)
{
	AnjutaDocman *docman;
	GList *node;
	
	docman = ANJUTA_DOCMAN (obj);
	docman->priv->shutingdown = TRUE;
	
	DEBUG_PRINT ("Disposing AnjutaDocman object");
	if (docman->priv->popup_menu)
	{
		gtk_widget_destroy (docman->priv->popup_menu);
		docman->priv->popup_menu = NULL;
	}
	if (docman->priv->pages)
	{
		/* Destroy all page data (more than just the notebook-page-widgets) */
		GList *pages;
		
		g_signal_handlers_disconnect_by_func (G_OBJECT (docman),
											 (gpointer) on_notebook_switch_page,
											 (gpointer) docman);
		pages = docman->priv->pages; /*work with copy so var can be NULL'ed ASAP*/
		docman->priv->pages = NULL;
		for (node = pages; node != NULL; node = g_list_next (node))
		{
			/* this also tries to destroy any notebook-page-widgets, in case
			   they're not gone already
		 CHECKME at shutdown do we need "destroy" signals in case other plugins
		   hold refs on any page(s) or their contents ?
			*/
			anjuta_docman_page_destroy ((AnjutaDocmanPage *)node->data);
		}
		g_list_free (pages);
	}
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (G_OBJECT(obj)));
}

static void
anjuta_docman_finalize (GObject *obj)
{
	AnjutaDocman *docman;
	
	DEBUG_PRINT ("Finalising AnjutaDocman object");
	docman = ANJUTA_DOCMAN (obj);
	if (docman->priv)
	{
		if (docman->priv->fileselection)
			gtk_widget_destroy (docman->priv->fileselection);
		g_free (docman->priv);
		docman->priv = NULL;
	}
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (G_OBJECT(obj)));
}

static void
anjuta_docman_instance_init (AnjutaDocman *docman)
{
	docman->priv = g_new0 (AnjutaDocmanPriv, 1);
/*g_new0 NULL's all content
	docman->priv->popup_menu = NULL;
	docman->priv->popup_menu_det = NULL;
	docman->priv->fileselection = NULL;
*/
	gtk_notebook_popup_enable (GTK_NOTEBOOK (docman));
	gtk_notebook_set_scrollable (GTK_NOTEBOOK (docman), TRUE);
	g_signal_connect (G_OBJECT (docman), "switch-page",
					  G_CALLBACK (on_notebook_switch_page), docman);
	/* update pages-list after re-ordering (or deleting) */
	g_signal_connect (G_OBJECT (docman), "page-reordered",
						G_CALLBACK (on_notebook_page_reordered), docman);
}

static void
anjuta_docman_class_init (AnjutaDocmanClass *klass)
{
	static gboolean initialized;
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = anjuta_docman_finalize;
	object_class->dispose = anjuta_docman_dispose;
	
	if (!initialized)
	{
		initialized = TRUE;
		
		/* Signals */
	docman_signals [DOC_ADDED] =
		g_signal_new ("document-added",
			ANJUTA_TYPE_DOCMAN,
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (AnjutaDocmanClass, document_added),
			NULL, NULL,
			g_cclosure_marshal_VOID__OBJECT,
			G_TYPE_NONE,
			1,
			G_TYPE_OBJECT);
	docman_signals [DOC_CHANGED] =
		g_signal_new ("document-changed",
			ANJUTA_TYPE_DOCMAN,
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (AnjutaDocmanClass, document_changed),
			NULL, NULL,
			g_cclosure_marshal_VOID__OBJECT,
			G_TYPE_NONE,
			1,
			G_TYPE_OBJECT);
			
	}
}

GtkWidget*
anjuta_docman_new (DocmanPlugin* plugin, AnjutaPreferences *pref)
{

	GtkWidget *docman;
	docman = gtk_widget_new (ANJUTA_TYPE_DOCMAN, NULL);
	if (docman)
	{
		ANJUTA_DOCMAN (docman)->priv->plugin = plugin;
		ANJUTA_DOCMAN (docman)->priv->preferences = pref;
	}

	return docman;
}

/*! state flag for Ctrl-TAB */
static gboolean g_tabbing = FALSE;

static void
on_notebook_switch_page (GtkNotebook *notebook,
						 GtkNotebookPage *page,
						 gint page_num, AnjutaDocman *docman)
{
	if (!docman->priv->shutingdown)
	{
		GtkWidget *page_widget;
		
		page_widget = gtk_notebook_get_nth_page (notebook, page_num);
		anjuta_docman_set_current_document (docman, IANJUTA_DOCUMENT (page_widget));
		/* TTimo: reorder so that the most recently used files are
		 * at the beginning of the tab list
		 */
		if (!docman->priv->tab_pressed	/* after a tab-click, sorting is done upon release */
			&& !g_tabbing
			&& !anjuta_preferences_get_int (docman->priv->preferences, EDITOR_TABS_ORDERING)
			&& anjuta_preferences_get_int (docman->priv->preferences, EDITOR_TABS_RECENT_FIRST))
		{
			gtk_notebook_reorder_child (notebook, page_widget, 0);
		}
	}
}

static void
on_document_save_point (IAnjutaDocument *doc, gboolean entering,
						AnjutaDocman *docman)
{
	anjuta_docman_update_page_label (docman, GTK_WIDGET (doc));
}

static void
on_document_destroy (IAnjutaDocument *doc, AnjutaDocman *docman)
{
	AnjutaDocmanPage *page;
	gint page_num;
	
	g_signal_handlers_disconnect_by_func (G_OBJECT (doc),
										  G_CALLBACK (on_document_save_point),
										  docman);
	g_signal_handlers_disconnect_by_func (G_OBJECT (doc),
										  G_CALLBACK (on_document_destroy),
										  docman);

	page = anjuta_docman_get_page_for_document (docman, doc);
	docman->priv->pages = g_list_remove (docman->priv->pages, page);
	
	if (!docman->priv->shutingdown)
	{
		if (page == docman->priv->cur_page)
			docman->priv->cur_page = NULL;
		if (GTK_NOTEBOOK (docman)->children == NULL)
			anjuta_docman_set_current_document (docman, NULL);
		else
		{
			GtkWidget *page_widget;
			/* set a replacement active document */
			page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (docman));
			page_widget = gtk_notebook_get_nth_page (GTK_NOTEBOOK (docman), page_num);
			anjuta_docman_set_current_document (docman, IANJUTA_DOCUMENT (page_widget));
		}
	}
	anjuta_docman_page_destroy (page);
}

IAnjutaEditor *
anjuta_docman_add_editor (AnjutaDocman *docman, const gchar *uri,
						  const gchar *name)
{
	IAnjutaEditor *te;
	IAnjutaEditorFactory* factory;
	
	factory = anjuta_shell_get_interface (docman->shell, IAnjutaEditorFactory, NULL);
	
	if (name && uri)
		te = ianjuta_editor_factory_new_editor(factory, 
										   uri, name, NULL);
	else if (uri)
		te = ianjuta_editor_factory_new_editor(factory, 
										   uri, "", NULL);
	else if (name)
		te = ianjuta_editor_factory_new_editor(factory, 
										   "", name, NULL);
	else
		te = ianjuta_editor_factory_new_editor(factory, 
										   "", "", NULL);
	
	/* File cannot be loaded, texteditor brings up an error dialog */
	if (te != NULL)
	{
		if (IANJUTA_IS_EDITOR (te))
			ianjuta_editor_set_popup_menu (te, docman->priv->popup_menu, NULL);
		anjuta_docman_add_document (docman, IANJUTA_DOCUMENT (te), uri);
	}	
	return te;
}

void
anjuta_docman_add_document (AnjutaDocman *docman, IAnjutaDocument *doc,
							const gchar *uri)
{
	AnjutaDocmanPage *page;

	page = anjuta_docman_page_new ();
	anjuta_docman_page_init (docman, doc, uri, page);	/* NULL uri is ok */

	/* list order matches pages in book, initially at least */
	docman->priv->pages = g_list_prepend (docman->priv->pages, (gpointer)page);
	
	gtk_notebook_prepend_page_menu (GTK_NOTEBOOK (docman), page->widget,
									page->box, page->menu_label);
	gtk_notebook_set_tab_reorderable (GTK_NOTEBOOK (docman), page->widget, TRUE);

	g_object_ref (G_OBJECT (doc));	/* compensate for unref in cleanup processes */

	g_signal_connect (G_OBJECT (doc), "save_point",
					  G_CALLBACK (on_document_save_point), docman);
	g_signal_connect (G_OBJECT (doc), "destroy",
					  G_CALLBACK (on_document_destroy), docman);
				   
	g_signal_emit (G_OBJECT (docman), docman_signals[DOC_ADDED], 0, doc);
	anjuta_docman_set_current_document (docman, doc);
	anjuta_shell_present_widget (docman->shell, GTK_WIDGET (docman->priv->plugin->vbox), NULL);
}

void
anjuta_docman_remove_document (AnjutaDocman *docman, IAnjutaDocument *doc)
{
	AnjutaDocmanPage *page;

	if (!doc)
		doc = anjuta_docman_get_current_document (docman);

	if (!doc)
		return;

	/* removing child-widget unrefs that as well as all tab-related widgets
		CHECKME except
		page->close_button, its refcount to 5
		page->widget, its refcount to 1, & surviving object seems to be used elsewhere */
	gtk_container_remove (GTK_CONTAINER (docman), GTK_WIDGET (doc));

	page = anjuta_docman_get_page_for_document (docman, doc);
	if (page)
	{
		if (page == docman->priv->cur_page)
			docman->priv->cur_page = NULL;
		docman->priv->pages = g_list_remove (docman->priv->pages, (gpointer)page);
		g_free (page);
	}

	/*CHECKME need something like this if other code can ref the document ?
	if ((G_OBJECT (doc))->ref_count > 0)
	 	g_signal_emit (G_OBJECT (docman), docman_signals[DOC_REMOVED], 0, doc);
	*/
}

void
anjuta_docman_set_popup_menu (AnjutaDocman *docman, GtkWidget *menu)
{
	if (menu)
		g_object_ref (G_OBJECT (menu));
	if (docman->priv->popup_menu)
		gtk_widget_destroy (docman->priv->popup_menu);
	docman->priv->popup_menu = menu;
}


GtkWidget *
anjuta_docman_get_current_focus_widget (AnjutaDocman *docman)
{
	GtkWidget *widget;
	widget = gtk_widget_get_toplevel (GTK_WIDGET (docman));
	if (GTK_WIDGET_TOPLEVEL (widget) &&
		gtk_window_has_toplevel_focus (GTK_WINDOW (widget)))
	{
		return gtk_window_get_focus (GTK_WINDOW (widget));
	}
	return NULL;
}

static AnjutaDocmanPage *
anjuta_docman_get_page_for_document (AnjutaDocman *docman, IAnjutaDocument *doc)
{
	GList *node;
	node = docman->priv->pages;
	while (node)
	{
		AnjutaDocmanPage *page;

		page = node->data;
		g_assert (page);
		if (page->widget == GTK_WIDGET (doc))
			return page;
		node = g_list_next (node);
	}
	return NULL;
}

IAnjutaDocument *
anjuta_docman_get_current_document (AnjutaDocman *docman)
{
	return docman->priv->current_document;
}

void
anjuta_docman_set_current_document (AnjutaDocman *docman, IAnjutaDocument *doc)
{
	AnjutaDocmanPage *page;
	IAnjutaDocument *defdoc;

	defdoc = docman->priv->current_document;
	if (defdoc == doc)
		return;

	if (doc != NULL)
	{
		page = anjuta_docman_get_page_for_document (docman, doc);
		/* proceed only if page data has been added before */
		if (page)
		{
			gint page_num;

			if (defdoc != NULL)
			{
				AnjutaDocmanPage *oldpage;
				oldpage = docman->priv->cur_page;
				if (oldpage)
				{
					oldpage->is_current = FALSE;
					if (oldpage->close_button != NULL)
					{
						gtk_widget_set_sensitive (oldpage->close_image, FALSE);
						if (oldpage->mime_icon)
							gtk_widget_set_sensitive (oldpage->mime_icon, FALSE);
					}
				}
			}

			docman->priv->current_document = doc;
			docman->priv->cur_page = page;

			page->is_current = TRUE;
			if (page->close_button != NULL)
			{
				gtk_widget_set_sensitive (page->close_image, TRUE);
				if (page->mime_icon)
					gtk_widget_set_sensitive (page->mime_icon, TRUE);
			}
			page_num = gtk_notebook_page_num (GTK_NOTEBOOK (docman),
											  GTK_WIDGET (doc));
			g_signal_handlers_block_by_func (G_OBJECT (docman),
											(gpointer) on_notebook_switch_page,
											(gpointer) docman);
			gtk_notebook_set_current_page (GTK_NOTEBOOK (docman), page_num);
			g_signal_handlers_unblock_by_func (G_OBJECT (docman),
											  (gpointer) on_notebook_switch_page,
											  (gpointer) docman);

			if (anjuta_preferences_get_int (ANJUTA_PREFERENCES (docman->priv->preferences),
											EDITOR_TABS_ORDERING))
				anjuta_docman_order_tabs (docman);

			gtk_widget_grab_focus (GTK_WIDGET (doc));
			anjuta_docman_grab_text_focus (docman);

			if (IANJUTA_IS_FILE (doc))
			{
				gchar *uri;
				uri = ianjuta_file_get_uri (IANJUTA_FILE (doc), NULL);
				if (uri)
				{
					gchar *hostname;
					gchar *filename;

					filename = g_filename_from_uri (uri, &hostname, NULL);
					if (hostname == NULL && filename != NULL)
					{
						gchar *dir;
						dir = g_path_get_dirname (filename);
						if (dir)
						{
							chdir (dir);	/* CHECKME why is CWD relevant at all ?
											Anything else might change CWD at any time */
							g_free (dir);
						}
					}
					g_free (hostname);
					g_free (filename);
					g_free (uri);
				}
			}
		}
	}
	else /* doc == NULL */
	{
		if (defdoc != NULL)
		{
			page = docman->priv->cur_page;
			if (page)
			{
				page->is_current = FALSE;
				if (page->close_button != NULL)
				{
					gtk_widget_set_sensitive (page->close_image, FALSE);
					if (page->mime_icon)
						gtk_widget_set_sensitive (page->mime_icon, FALSE);
				}
			}
		}
		docman->priv->current_document = NULL;
		docman->priv->cur_page = NULL;
	}

	if (doc == NULL || page != NULL)
		g_signal_emit (G_OBJECT (docman), docman_signals[DOC_CHANGED], 0, doc);
}

IAnjutaEditor *
anjuta_docman_goto_file_line (AnjutaDocman *docman, const gchar *fname, gint lineno)
{
	return anjuta_docman_goto_file_line_mark (docman, fname, lineno, FALSE);
}

IAnjutaEditor *
anjuta_docman_goto_file_line_mark (AnjutaDocman *docman, const gchar *fname,
								   gint line, gboolean mark)
{
	gchar *uri, *te_uri;
	GnomeVFSURI* vfs_uri;
	GList *node;
	const gchar *linenum;
	gint lineno;
	gboolean is_local_uri;
	gchar *normalized_path = NULL;
	
	IAnjutaDocument *doc;
	IAnjutaEditor *te;

	g_return_val_if_fail (fname, NULL);
	
	/* FIXME: */
	/* filename = anjuta_docman_get_full_filename (docman, fname); */
	vfs_uri = gnome_vfs_uri_new (fname);
	
	/* Extract linenum which comes as fragment identifier */
	linenum = gnome_vfs_uri_get_fragment_identifier (vfs_uri);
	if (linenum)
		lineno = atoi (linenum);
	else
		lineno = line;
	
	/* Restore URI without fragment identifier (linenum) */
	uri = gnome_vfs_uri_to_string (vfs_uri,
								   GNOME_VFS_URI_HIDE_FRAGMENT_IDENTIFIER);
	
	/* Get the normalized file path for comparision */
	is_local_uri = gnome_vfs_uri_is_local (vfs_uri);
	if (is_local_uri)
		normalized_path = realpath (gnome_vfs_uri_get_path (vfs_uri), NULL);
	if (normalized_path == NULL)
		normalized_path = g_strdup (uri);
	
	gnome_vfs_uri_unref (vfs_uri);
	/* g_free(filename); */

	g_return_val_if_fail (uri != NULL, NULL);
	
	node = docman->priv->pages;
	
	/* first, try to use a document that's already open */
	while (node)
	{
		AnjutaDocmanPage *page;
		
		page = (AnjutaDocmanPage *) node->data;
		doc = IANJUTA_DOCUMENT (page->widget);
		if (!IANJUTA_IS_EDITOR (doc))
			continue;

		te_uri = ianjuta_file_get_uri (IANJUTA_FILE (doc), NULL);
		if (te_uri)
		{
			gboolean te_is_local_uri;
			gchar *te_normalized_path;
		
			/* Get the normalized file path for comparision */
			vfs_uri = gnome_vfs_uri_new (te_uri);
			te_is_local_uri = gnome_vfs_uri_is_local (vfs_uri);
			if (te_is_local_uri)
				te_normalized_path = realpath (gnome_vfs_uri_get_path (vfs_uri),
											   NULL);
			else
				te_normalized_path = g_strdup (te_uri);
			gnome_vfs_uri_unref (vfs_uri);
		
			if (!normalized_path || !te_normalized_path)
			{
				DEBUG_PRINT ("Unexpected NULL path");
				g_free (te_uri);
				g_free (te_normalized_path);
				node = g_list_next (node);
				continue;
			}

			if (strcmp (normalized_path, te_normalized_path) == 0)
			{
				if (lineno >= 0)
				{
					ianjuta_editor_goto_line (IANJUTA_EDITOR (doc), lineno, NULL);
					if (mark)
					{
						ianjuta_markable_delete_all_markers (IANJUTA_MARKABLE (doc),
															IANJUTA_MARKABLE_LINEMARKER,
															NULL);
						ianjuta_markable_mark (IANJUTA_MARKABLE (doc), lineno,
											  IANJUTA_MARKABLE_LINEMARKER, NULL);
					}
				}
				anjuta_docman_present_notebook_page (docman, doc);
				an_file_history_push (te_uri, lineno);
				g_free (te_uri);
				g_free (te_normalized_path);
				g_free (uri);
				g_free (normalized_path);
				return IANJUTA_EDITOR (doc);
			}
			g_free (te_uri);
			g_free (te_normalized_path);
		}
		node = g_list_next (node);
	}
	/* no deal, open a new document */
	te = anjuta_docman_add_editor (docman, uri, NULL);
	if (te)
	{
		te_uri = ianjuta_file_get_uri (IANJUTA_FILE (te), NULL);
		an_file_history_push (te_uri, lineno);
		g_free (te_uri);
		if (lineno >= 0) 
		{
			ianjuta_editor_goto_line(te, lineno, NULL);
            if (mark)
  	        {
  	        	ianjuta_markable_mark(IANJUTA_MARKABLE(te), lineno,
  	            					IANJUTA_MARKABLE_LINEMARKER, NULL);
			}
		}
	}
	g_free (uri);
	g_free (normalized_path);
	return te ;
}

static gchar*
get_real_path (const gchar *file_name)
{
	if (file_name)
	{
		gchar path[PATH_MAX+1];
		memset (path, '\0', PATH_MAX+1);
		realpath (file_name, path);
		return g_strdup (path);
	}
	else
		return NULL;
}

gchar *
anjuta_docman_get_full_filename (AnjutaDocman *docman, const gchar *fn)
{
	IAnjutaDocument *doc;
	GList *node;
	gchar *real_path;
	gchar *fname;
	
	g_return_val_if_fail (fn, NULL);
	real_path = get_real_path (fn);
	
	/* If it is full and absolute path, there is no need to 
	go further, even if the file is not found*/
	if (fn[0] == '/')
	{
		return real_path;
	}
	
	/* First, check if we can get the file straightaway */
	if (g_file_test (real_path, G_FILE_TEST_IS_REGULAR))
		return real_path;
	g_free(real_path);

	/* Get the name part of the file */
	fname = g_path_get_basename (fn);
	
	/* Next, check if the current text editor buffer matches this name */
	if (NULL != (doc = anjuta_docman_get_current_document (docman)))
	{
		if (strcmp(ianjuta_document_get_filename(doc, NULL), fname) == 0)
		{
			g_free (fname);
			return ianjuta_file_get_uri (IANJUTA_FILE (doc), NULL);
		}
	}
	/* Next, see if the name matches any of the opened files */
	for (node = docman->priv->pages; node != NULL; node = g_list_next (node))
	{
		AnjutaDocmanPage *page;

		page = (AnjutaDocmanPage *) node->data;
		doc = IANJUTA_DOCUMENT (page->widget);
		if (strcmp (fname, ianjuta_document_get_filename (doc, NULL)) == 0)
		{
			g_free (fname);
			return ianjuta_file_get_uri (IANJUTA_FILE (doc), NULL);
		}
	}
	g_free (fname);
	return NULL;
}

void
anjuta_docman_present_notebook_page (AnjutaDocman *docman, IAnjutaDocument *doc)
{
	GList *node;

	if (!doc)
		return;

	node = docman->priv->pages;

	while (node)
	{
		AnjutaDocmanPage* page;
		page = (AnjutaDocmanPage *)node->data;
		if (page && IANJUTA_DOCUMENT (page->widget) == doc)
		{
			gint curindx;
			curindx = gtk_notebook_page_num (GTK_NOTEBOOK (docman), page->widget);
			if (!(curindx == -1 || curindx == gtk_notebook_get_current_page (GTK_NOTEBOOK (docman))))
				gtk_notebook_set_current_page (GTK_NOTEBOOK (docman), curindx);
/* this is done by the page-switch cb
			if (!page->is_current)
			{
				anjuta_docman_set_current_document (docman, doc);
			}
*/
			break;
		}
		node = g_list_next (node);
	}
}

static void
anjuta_docman_update_page_label (AnjutaDocman *docman, GtkWidget *page_widget)
{
	AnjutaDocmanPage *page;
	gchar *basename;
	gchar *uri;
	IAnjutaDocument *doc;
	const gchar* doc_filename;
	gchar* dirty_char;
	gchar* label;
	
	doc = IANJUTA_DOCUMENT (page_widget);
	if (doc == NULL)
		return;

	page = anjuta_docman_get_page_for_document (docman, doc);
	if (!page || page->label == NULL || page->menu_label == NULL)
		return;
	
	if (!ianjuta_file_savable_is_dirty(IANJUTA_FILE_SAVABLE (doc), NULL))
	{
		dirty_char = NULL;
	}
	else
	{
		dirty_char = "*";
	}
	
	uri = ianjuta_file_get_uri (IANJUTA_FILE (doc), NULL);
	if (uri)
	{
		basename = g_path_get_basename (uri);
		label = g_strconcat(basename, dirty_char, NULL);
		gtk_label_set_text (GTK_LABEL (page->label), label);
		gtk_label_set_text (GTK_LABEL (page->menu_label), label);
		g_free (label);
		g_free (basename);
		g_free (uri);
	}
	else if ((doc_filename = ianjuta_document_get_filename (doc, NULL)) != NULL)
	{
		label = g_strconcat (doc_filename, dirty_char, NULL);
		gtk_label_set_text (GTK_LABEL (page->label), label);
		gtk_label_set_text (GTK_LABEL (page->menu_label), label);
		g_free (label);
	}
}

static void
anjuta_docman_grab_text_focus (AnjutaDocman *docman)
{
	anjuta_shell_present_widget (docman->shell, 
								 GTK_WIDGET (docman->priv->plugin->vbox), NULL);
}

void
anjuta_docman_delete_all_markers (AnjutaDocman *docman, gint marker)
{
	GList *node;

	for (node = docman->priv->pages; node != NULL; node = g_list_next (node))
	{
		AnjutaDocmanPage *page;

		page = (AnjutaDocmanPage *) node->data;
		if (IANJUTA_IS_EDITOR (page->widget))
		{
			IAnjutaEditor* te;

			te = IANJUTA_EDITOR (page->widget);
			ianjuta_markable_delete_all_markers (IANJUTA_MARKABLE (te), marker, NULL);
		}
	}
}

void
anjuta_docman_delete_all_indicators (AnjutaDocman *docman)
{
	GList *node;

	for (node = docman->priv->pages; node; node = g_list_next (node))
	{
		AnjutaDocmanPage *page;

		page = (AnjutaDocmanPage *) node->data;
		if (IANJUTA_IS_EDITOR (page->widget))
		{
			gchar *uri;

			uri = ianjuta_file_get_uri (IANJUTA_FILE (page->widget), NULL);
			if (uri)
			{
				g_free (uri);
				ianjuta_markable_unmark (IANJUTA_MARKABLE (page->widget), -1, -1, NULL);
			}
		}
	}
}

/* Saves a file to keep it synchronized with external programs */
void 
anjuta_docman_save_file_if_modified (AnjutaDocman *docman, const gchar *szFullPath)
{
	IAnjutaDocument *doc;

	g_return_if_fail (szFullPath != NULL);

	doc = anjuta_docman_get_document_for_path (docman, szFullPath);
	if (doc)
	{
		if(ianjuta_file_savable_is_dirty (IANJUTA_FILE_SAVABLE (doc), NULL))
		{
			ianjuta_file_savable_save (IANJUTA_FILE_SAVABLE (doc), NULL);
		}
	}
}

/* If an external program changed the file, we must reload it */
void 
anjuta_docman_reload_file (AnjutaDocman *docman, const gchar *szFullPath)
{
	IAnjutaDocument *doc;

	g_return_if_fail (szFullPath != NULL);

	doc = anjuta_docman_get_document_for_path (docman, szFullPath);
	if (doc && IANJUTA_IS_EDITOR (doc))
	{
		IAnjutaEditor *te;
		te = IANJUTA_EDITOR (doc);
		glong nNowPos = ianjuta_editor_get_lineno (te, NULL);
		ianjuta_file_open (IANJUTA_FILE (doc), szFullPath, NULL);
		ianjuta_editor_goto_line (te, nNowPos, NULL);
	}
}

typedef struct _order_struct order_struct;
struct _order_struct
{
	const gchar *m_label;
	GtkWidget *m_widget;
};

static int
do_ordertab1 (const void *a, const void *b)
{
	order_struct aos,bos;
	aos = *(order_struct*)a;
	bos = *(order_struct*)b;
	return (g_strcasecmp (aos.m_label, bos.m_label)); /* need g_utf8_collate() ? */
}

static void
anjuta_docman_order_tabs (AnjutaDocman *docman)
{
	gint i, num_pages;
	GtkWidget *page_widget;
	order_struct *tab_labels;
	GtkNotebook *notebook;

	notebook = GTK_NOTEBOOK (docman);

	num_pages = gtk_notebook_get_n_pages (notebook);
	if (num_pages < 2)
		return;
	tab_labels = g_new0 (order_struct, num_pages);
	for (i = 0; i < num_pages; i++)
	{
/*new0 NULL'ed things already
		if((widget = gtk_notebook_get_nth_page (notebook, i)) == NULL)
		{
			tab_labels[i].m_label = NULL;
			tab_labels[i].m_widget = NULL;
		}
		else
*/
		if ((page_widget = gtk_notebook_get_nth_page (notebook, i)) != NULL)
		{
			tab_labels[i].m_widget = page_widget; /* CHECKME needed ? */
			tab_labels[i].m_label = ianjuta_document_get_filename (
											IANJUTA_DOCUMENT (page_widget), NULL);
		}
	}
	qsort (tab_labels, num_pages, sizeof(order_struct), do_ordertab1);
	g_signal_handlers_block_by_func (G_OBJECT (notebook),
									(gpointer) on_notebook_page_reordered,
									(gpointer) docman);
	for (i = 0; i < num_pages; i++)
		gtk_notebook_reorder_child (notebook, tab_labels[i].m_widget, i);
	g_signal_handlers_unblock_by_func (G_OBJECT (notebook),
									  (gpointer) on_notebook_page_reordered,
									  (gpointer) docman);
	g_free (tab_labels);
	/* adjust pagelist order */
	g_idle_add ((GSourceFunc) anjuta_docman_sort_pagelist, docman);
}


gboolean
anjuta_docman_set_editor_properties (AnjutaDocman *docman)
{
	/* FIXME:
	TextEditor *te = IANJUTA_EDITOR (docman->priv->current_document);
	if (te)
	{
		gchar *word;
		// FIXME: anjuta_set_file_properties (app->current_text_editor->uri);
		word = text_editor_get_current_word (te);
		prop_set_with_key (te->props_base, "current.file.selection", word?word:"");
		if (word)
			g_free(word);
		prop_set_int_with_key (te->props_base, "current.file.lineno",
			text_editor_get_current_lineno (te));
		return TRUE;
	}
	else
		return FALSE; */
	return TRUE;
}

IAnjutaDocument *
anjuta_docman_get_document_for_path (AnjutaDocman *docman, const gchar *file_path)
{
	GList *node;
	
	g_return_val_if_fail (file_path != NULL, NULL);
		
	for (node = docman->priv->pages; node != NULL; node = g_list_next (node))
	{
		AnjutaDocmanPage *page;
		page = (AnjutaDocmanPage *) node->data;

		if (page && page->widget && IANJUTA_IS_DOCUMENT (page->widget))
		{
			IAnjutaDocument *doc;
			gchar *uri;

			doc = IANJUTA_DOCUMENT (page->widget);
			uri = ianjuta_file_get_uri (IANJUTA_FILE (doc), NULL);
			if (uri)
			{
				if (0 == strcmp (file_path, uri))
				{
					g_free (uri);
					return doc;
				}
				g_free (uri);
			}
		}
	}
	return NULL;
}

GList*
anjuta_docman_get_all_doc_widgets (AnjutaDocman *docman)
{
	GList *wids;
	GList *node;
	
	wids = NULL;
	for (node = docman->priv->pages; node != NULL; node = g_list_next (node))
	{
		AnjutaDocmanPage *page;
		page = (AnjutaDocmanPage *) node->data;
		if (page && page->widget)
			wids = g_list_prepend (wids, page->widget);
	}
	if (wids)
		wids = g_list_reverse (wids);
	return wids;
}

ANJUTA_TYPE_BEGIN(AnjutaDocman, anjuta_docman, GTK_TYPE_NOTEBOOK);
ANJUTA_TYPE_END;
