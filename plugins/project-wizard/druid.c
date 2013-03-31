/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    druid.c
    Copyright (C) 2004 Sebastien Granjoux

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

/*
 * All GUI functions
 *
 *---------------------------------------------------------------------------*/

#include <config.h>

#include "druid.h"

#include "plugin.h"
#include "header.h"
#include "property.h"
#include "parser.h"
#include "install.h"

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-autogen.h>
#include <libanjuta/interfaces/ianjuta-wizard.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <glib.h>
#include <gdk/gdk.h>

#include <gtk/gtk.h>

#include <string.h>

/*---------------------------------------------------------------------------*/

#define PROJECT_WIZARD_DIRECTORY PACKAGE_DATA_DIR "/templates"

/* Default property name useable in wizard file
 *---------------------------------------------------------------------------*/

#define ANJUTA_PROJECT_DIRECTORY_PROPERTY "AnjutaProjectDirectory"
#define USER_NAME_PROPERTY "UserName"
#define EMAIL_ADDRESS_PROPERTY "EmailAddress"
#define INDENT_WIDTH_PROPERTY "IndentWidth"
#define TAB_WIDTH_PROPERTY "TabWidth"
#define USE_TABS_PROPERTY "UseTabs"

/* Common editor preferences */
#define ANJUTA_PREF_SCHEMA_PREFIX "org.gnome.anjuta."


/* Widget and signal name found in glade file
 *---------------------------------------------------------------------------*/

#define GTK_BUILDER_UI_FILE PACKAGE_DATA_DIR "/glade/anjuta-project-wizard.ui"

#define NEW_PROJECT_DIALOG "druid_window"
#define PROJECT_LIST "project_list"
#define PROJECT_BOOK "project_book"
#define PROJECT_PAGE "project_page"
#define ERROR_PAGE "error_page"
#define ERROR_TITLE "error_title"
#define PROGRESS_PAGE "progress_page"
#define FINISH_PAGE "finish_page"
#define FINISH_TEXT "finish_text"
#define PROPERTY_PAGE "property_page"
#define PROPERTY_TITLE "property_title"
#define PROPERTY_TABLE "property_table"
#define ERROR_VBOX "error_vbox"
#define ERROR_ICON "error_icon"
#define ERROR_MESSAGE "error_message"
#define ERROR_DETAIL "error_detail"


#define PROJECT_PAGE_INDEX 0

/*---------------------------------------------------------------------------*/

struct _NPWDruid
{
	GtkWindow* window;

	GtkNotebook* project_book;
	GtkWidget *error_page;
	GtkWidget *error_title;
	GtkBox *error_vbox;
	GtkWidget *error_extra_widget;
	GtkImage *error_icon;
	GtkLabel *error_message;
	GtkWidget *error_detail;

	GtkWidget *project_page;
	GtkWidget *progress_page;
	GtkWidget *finish_page;
	GtkWidget *finish_text;

	const gchar* project_file;
	NPWPlugin* plugin;

	GQueue* page_list;

	GHashTable* values;
	NPWPageParser* parser;
	GList* header_list;
	NPWHeader* header;
	gboolean no_selection;
	AnjutaAutogen* gen;
	gboolean busy;
};

/* column of the icon view */
enum {
	PIXBUF_COLUMN,
	TEXT_COLUMN,
	DESC_COLUMN,
	DATA_COLUMN
};

/* Helper functon
 *---------------------------------------------------------------------------*/

/* Druid GUI functions
 *---------------------------------------------------------------------------*/

static void
npw_druid_set_busy (NPWDruid *druid, gboolean busy_state)
{
	if (druid->busy == busy_state)
		return;

	/* Set busy state */
	if (busy_state)
		anjuta_status_busy_push (anjuta_shell_get_status (ANJUTA_PLUGIN (druid->plugin)->shell, NULL));
	else
		anjuta_status_busy_pop (anjuta_shell_get_status (ANJUTA_PLUGIN (druid->plugin)->shell, NULL));
	druid->busy = busy_state;
}

/* Create error page
 *---------------------------------------------------------------------------*/

/* Fill dialog page */
static void
npw_druid_fill_error_page (NPWDruid* druid, GtkWidget *extra_widget, GtkMessageType type, const gchar *detail, const gchar *mesg,...)
{
	GtkAssistant *assistant;
	GtkWidget *page;
	va_list args;
	gchar *message;
	const gchar *stock_id = NULL;
	const gchar *title = NULL;

	assistant = GTK_ASSISTANT (druid->window);

	/* Add page to assistant after current one */
	page = druid->error_page;
	gtk_assistant_insert_page (assistant, page, gtk_assistant_get_current_page (assistant) + 1);

	/* Set dialog kind */
	switch (type)
	{
	case GTK_MESSAGE_INFO:
		stock_id = GTK_STOCK_DIALOG_INFO;
		title = _("Information");
		break;
	case GTK_MESSAGE_QUESTION:
		stock_id = GTK_STOCK_DIALOG_QUESTION;
		title = _("Warning");
		break;
	case GTK_MESSAGE_WARNING:
		stock_id = GTK_STOCK_DIALOG_WARNING;
		title = _("Warning");
		break;
	case GTK_MESSAGE_ERROR:
		stock_id = GTK_STOCK_DIALOG_ERROR;
		title = _("Error");
		break;
	case GTK_MESSAGE_OTHER:
		title = _("Message");
		break;
	default:
		g_warning ("Unknown GtkMessageType %u", type);
		break;
	}
	gtk_label_set_text (GTK_LABEL (druid->error_title), title);
	gtk_assistant_set_page_title (assistant, page, title);
	if (type == GTK_MESSAGE_ERROR)
	{
		gtk_assistant_set_page_type (assistant, page, GTK_ASSISTANT_PAGE_CONTENT);
		gtk_assistant_set_page_complete (assistant, page, FALSE);
	}
	else
	{
		gtk_assistant_set_page_type (assistant, page, GTK_ASSISTANT_PAGE_PROGRESS);
		gtk_assistant_set_page_complete (assistant, page, TRUE);
	}
	gtk_image_set_from_stock (druid->error_icon, stock_id, GTK_ICON_SIZE_DIALOG);

	/* Set message */
	va_start (args, mesg);
	message = g_strdup_vprintf (mesg, args);
	va_end (args);
	gtk_label_set_markup (druid->error_message, message);
	g_free (message);

	/* Set detail */
	if (detail == NULL)
	{
		gtk_widget_hide (druid->error_detail);
	}
	else
	{
		GtkLabel *label;

		gtk_widget_show (druid->error_detail);
		label = GTK_LABEL (gtk_bin_get_child (GTK_BIN (druid->error_detail)));
		gtk_label_set_text (label, detail);
	}

	if (druid->error_extra_widget)
		gtk_widget_destroy (druid->error_extra_widget);
	druid->error_extra_widget = NULL;

	/* Set extra widget */
	if (extra_widget)
	{
		gtk_box_pack_start (GTK_BOX (druid->error_vbox), extra_widget,
							FALSE, FALSE, 10);
		gtk_widget_show (extra_widget);
		druid->error_extra_widget = extra_widget;
	}
}

/* Create finish page
 *---------------------------------------------------------------------------*/

static void
cb_druid_add_summary_property (NPWProperty* property, gpointer user_data)
{
	GString* text = (GString*)user_data;

	if (npw_property_get_options (property) & NPW_SUMMARY_OPTION)
	{
		g_string_append_printf (text, "%s %s\n",
								npw_property_get_label (property),
								npw_property_get_value (property));
	}
}

/* Fill last page (summary) */
static void
npw_druid_fill_summary_page (NPWDruid* druid)
{
	NPWPage* page;
	guint i;
	GString* text;
	GtkLabel* label;

	text = g_string_new (NULL);
	g_string_append_printf (text, "<b>%s</b>\n\n", _("Confirm the following information:"));

	/* The project type is translated too, it is something like
	 * generic, GNOME applet, Makefile project... */
	g_string_append_printf (text, _("Project Type: %s\n"), npw_header_get_name (druid->header));

	for (i = 0; (page = g_queue_peek_nth (druid->page_list, i)) != NULL; ++i)
	{
		npw_page_foreach_property (page, (GFunc)cb_druid_add_summary_property, text);
	}

	label = GTK_LABEL (druid->finish_text);
	gtk_label_set_markup (label, text->str);
	g_string_free (text, TRUE);

	gtk_assistant_set_page_complete (GTK_ASSISTANT (druid->window), druid->finish_page, TRUE);
}

/* Create project selection page
 *---------------------------------------------------------------------------*/

/* Update selected project */
static void
on_druid_project_update_selected (GtkIconView* view, NPWDruid *druid)
{
	GList *selected;
	NPWHeader* header = NULL;

	/* No item can be selected when the view is mapped */
	selected = gtk_icon_view_get_selected_items (view);
	if (selected != NULL)
	{
		GtkTreeIter iter;
		GtkTreeModel *model;

		model = gtk_icon_view_get_model (view);
		if (gtk_tree_model_get_iter (model, &iter, (GtkTreePath *)selected->data))
		{
			gtk_tree_model_get	(model, &iter, DATA_COLUMN, &header, -1);
		}
		gtk_tree_path_free ((GtkTreePath *)selected->data);
		g_list_free (selected);
	}

	druid->header = header;
	gtk_assistant_set_page_complete (GTK_ASSISTANT (druid->window), druid->project_page, header != NULL);
}

/* Add a project in the icon list */
static void
cb_druid_insert_project_icon (gpointer data, gpointer user_data)
{
	NPWHeader *header = (NPWHeader *)data;
	GtkListStore* store = GTK_LIST_STORE (user_data);
	GtkTreeIter iter;
	GdkPixbuf *pixbuf;

	gtk_list_store_append (store, &iter);
	pixbuf = gdk_pixbuf_new_from_file (npw_header_get_iconfile (header), NULL);
	gtk_list_store_set (store, &iter, PIXBUF_COLUMN, pixbuf,
						TEXT_COLUMN, npw_header_get_name (header),
						DESC_COLUMN, npw_header_get_description (header),
						DATA_COLUMN, header,
						-1);

	g_object_unref (pixbuf);
}

/* Add a page in the project notebook */
static void
cb_druid_insert_project_page (gpointer value, gpointer user_data)
{
	NPWDruid* druid = (NPWDruid*)user_data;
	GtkBuilder *builder;
	GtkWidget* label;
	GtkWidget* child;
	GtkWidget* assistant;
	GtkIconView* view;
	GtkNotebook *book;
	GtkListStore *store;
	GList *template_list = (GList *)value;
	const gchar* category;

	category = npw_header_get_category ((NPWHeader *)template_list->data);

	/* Build another complete wizard dialog from the gtk builder file
	 * but keep only the project selection notebook page */
	builder = gtk_builder_new ();
	if (!gtk_builder_add_from_file (builder, GTK_BUILDER_UI_FILE, NULL))
	{
		g_warn_if_reached ();
		g_object_unref (builder);
		return;
	}

	/* Fill icon view */
	view = GTK_ICON_VIEW (gtk_builder_get_object (builder, PROJECT_LIST));
	gtk_icon_view_set_pixbuf_column (view, PIXBUF_COLUMN);
	gtk_icon_view_set_markup_column (view, TEXT_COLUMN);
	store = gtk_list_store_new (4, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
	g_list_foreach (template_list, cb_druid_insert_project_icon, store);
	gtk_icon_view_set_model (view, GTK_TREE_MODEL (store));

	/* Connect signal to update dialog */
	g_signal_connect (G_OBJECT (view), "selection-changed", G_CALLBACK (on_druid_project_update_selected), druid);
	g_signal_connect (G_OBJECT (view), "map", G_CALLBACK (on_druid_project_update_selected), druid);
	g_signal_connect_swapped (G_OBJECT (view), "item-activated", G_CALLBACK (gtk_assistant_next_page), druid->window);

	/* Set new page label */
	assistant = GTK_WIDGET (gtk_builder_get_object (builder, NEW_PROJECT_DIALOG));
	book = GTK_NOTEBOOK (gtk_builder_get_object (builder, PROJECT_BOOK));
	child = gtk_notebook_get_nth_page (book, 0);
	label = gtk_notebook_get_tab_label (book, child);
	gtk_label_set_text (GTK_LABEL(label), (const gchar *)category);

	/* Pick up the filled project selection page from the newly created dialog
	 * add it to the wizard project notebook and destroy the dialog
	 */
	gtk_notebook_remove_page (book, 0);
	gtk_notebook_append_page (druid->project_book, child, label);
	gtk_widget_destroy (assistant);

	g_object_unref (builder);
}

/* Fill project selection page */
static gboolean
npw_druid_fill_selection_page (NPWDruid* druid, GFile *templates)
{
	gchar* dir;
	const gchar * const * sys_dir;

	/* Remove all previous data */
	gtk_notebook_remove_page(druid->project_book, 0);
	npw_header_list_free (druid->header_list);
	anjuta_autogen_clear_library_path (druid->gen);

	/* Create list of projects */
	druid->header_list = npw_header_list_new ();

	if (templates != NULL)
	{
		if (g_file_query_file_type (templates, 0, NULL) == G_FILE_TYPE_DIRECTORY)
		{
			/* Read project template only in specified directory,
		 	 * other directories can still be used to get included
		 	 * files */
			gchar *directory = g_file_get_path (templates);

			npw_header_list_readdir (&druid->header_list, directory);
			anjuta_autogen_set_library_path (druid->gen, directory);
			g_free (directory);
		}
		else
		{
			/* templates is a file, so read only it as a project template */
			gchar *filename = g_file_get_path (templates);
			npw_header_list_read (&druid->header_list, filename);
			g_free (filename);
		}
	}

	dir = g_build_filename (g_get_user_data_dir (), "anjuta", "templates", NULL);
	if (templates == NULL)
	{
		/* Read project template in user directory,
		 * normally ~/.local/share/anjuta/templates,
	 	* the first template read override the others */
		npw_header_list_readdir (&druid->header_list, dir);
	}
	anjuta_autogen_set_library_path (druid->gen, dir);
	g_free (dir);

	for (sys_dir = g_get_system_data_dirs (); *sys_dir != NULL; sys_dir++)
	{
		dir = g_build_filename (*sys_dir, "anjuta", "templates", NULL);
		if (templates == NULL)
		{
			/* Read project template in system directory */
			npw_header_list_readdir (&druid->header_list, dir);
		}
		anjuta_autogen_set_library_path (druid->gen, dir);
		g_free (dir);
	}

	if (templates == NULL)
	{
		/* Read anjuta installation directory */
		npw_header_list_readdir (&druid->header_list, PROJECT_WIZARD_DIRECTORY);
	}
	anjuta_autogen_set_library_path (druid->gen, PROJECT_WIZARD_DIRECTORY);

	switch (g_list_length (druid->header_list))
	{
	case 0:
		anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (druid->plugin)->shell),_("Unable to find any project template in %s"), PROJECT_WIZARD_DIRECTORY);
		return FALSE;
	case 1:
		druid->header = (NPWHeader *)((GList *)druid->header_list->data)->data;
		druid->no_selection = TRUE;
		gtk_container_remove (GTK_CONTAINER (druid->window), druid->project_page);
		gtk_assistant_insert_page (GTK_ASSISTANT (druid->window), druid->progress_page, 0);
		npw_druid_set_busy (druid, FALSE);
		break;
	default:
		/* Add all necessary notebook page */
		druid->no_selection = FALSE;
		g_list_foreach (druid->header_list, cb_druid_insert_project_page, druid);
		gtk_widget_show_all (GTK_WIDGET (druid->project_book));
	}

	return TRUE;
}

/* Create properties pages
 *---------------------------------------------------------------------------*/

/* Group infomation need by the following call back function */
typedef struct _NPWDruidAddPropertyData
{
	NPWDruid* druid;
	guint row;
	GtkGrid *table;
	GtkWidget *first_entry;
} NPWDruidAddPropertyData;

static void
cb_druid_destroy_widget (GtkWidget* widget, gpointer user_data)
{
	gtk_widget_destroy (widget);
}

static void
cb_druid_add_property (NPWProperty* property, gpointer user_data)
{
	GtkWidget* label;
	GtkWidget* entry;
	NPWDruidAddPropertyData* data = (NPWDruidAddPropertyData *)user_data;
	const gchar* description;


	entry = npw_property_create_widget (property);
	if (entry != NULL)
	{
		/* Not hidden property */
		description = npw_property_get_description (property);

		/* Set description tooltip */
		if (description && (*description != '\0'))
		{
			gtk_widget_set_tooltip_text (entry, description);
		}

		label = gtk_label_new (npw_property_get_label (property));
		gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
		gtk_misc_set_padding (GTK_MISC (label), 6, 6);

		gtk_widget_set_hexpand (entry, TRUE);
		switch (npw_property_get_type (property))
		{
			case NPW_PACKAGE_PROPERTY:
				gtk_widget_set_vexpand (entry, TRUE);
				gtk_grid_attach (data->table, label, 0, data->row, 1, 1);
				gtk_grid_attach (data->table, entry, 0, data->row + 1, 1, 1);
				data->row += 2;
				break;
			case NPW_BOOLEAN_PROPERTY:
				gtk_widget_set_hexpand (entry, FALSE);
				/* Fall through */	
			default:
				/* Add label and entry */
				gtk_grid_attach (data->table, label, 0, data->row, 1, 1);
				gtk_grid_attach (data->table, entry, 1, data->row, 1, 1);
				data->row++;
		}

		/* Set first entry */
		if (data->first_entry == NULL) data->first_entry = entry;
	}
};

static void
npw_druid_fill_property_page (NPWDruid* druid, NPWPage* page)
{
	GtkWidget *widget;
	GList *children;
	GtkLabel *label;
	NPWDruidAddPropertyData data;

	/* Add page to assistant, after current page */
	widget = gtk_assistant_get_nth_page (GTK_ASSISTANT (druid->window), gtk_assistant_get_current_page (GTK_ASSISTANT (druid->window)) + 1);

	/* Remove previous widgets */
	gtk_container_foreach (GTK_CONTAINER (npw_page_get_widget (page)), cb_druid_destroy_widget, NULL);

	/* Update title	*/
	children = gtk_container_get_children (GTK_CONTAINER (widget));
	label = GTK_LABEL (g_list_nth_data (children, 0));
	g_list_free (children);
	gtk_label_set_text (label, npw_page_get_label (page));
	gtk_assistant_set_page_title (GTK_ASSISTANT (druid->window), widget, npw_page_get_label (page));

	/* Add new widget */
	data.druid = druid;
	data.row = 0;
	data.table = GTK_GRID (npw_page_get_widget (page));
	data.first_entry = NULL;
	npw_page_foreach_property (page, (GFunc)cb_druid_add_property, &data);

	/* Move focus on first entry */
	if (data.first_entry != NULL)
	{
		gtk_container_set_focus_child (GTK_CONTAINER (data.table), data.first_entry);
	}

	gtk_widget_show_all (widget);
}

/* Handle page cache
 *---------------------------------------------------------------------------*/

static NPWPage*
npw_druid_add_new_page (NPWDruid* druid)
{
	gint current;
	NPWPage* page;

	/* Get page in cache */
	current = gtk_assistant_get_current_page (GTK_ASSISTANT (druid->window));
	page = g_queue_peek_nth (druid->page_list, current - (druid->no_selection ? 0 : 1) + 1);

	if (page == NULL)
	{
		/* Page not found in cache, create */
		GtkBuilder *builder;
		GtkWidget *widget;
		GtkAssistantPageType type;
		GtkWidget *table;
		GtkAssistant *assistant;

		/* Build another complete wizard dialog from the gtk builder file
	 	* but keep only the property assistant page */
		builder = gtk_builder_new ();
		if (!gtk_builder_add_from_file (builder, GTK_BUILDER_UI_FILE, NULL))
		{
			g_warn_if_reached ();
			g_object_unref (builder);

			return NULL;
		}
		assistant = GTK_ASSISTANT (gtk_builder_get_object (builder, NEW_PROJECT_DIALOG));
		widget = GTK_WIDGET (gtk_builder_get_object (builder, PROPERTY_PAGE));
		table = GTK_WIDGET (gtk_builder_get_object (builder, PROPERTY_TABLE));

		type = gtk_assistant_get_page_type (assistant, widget);
		gtk_container_remove (GTK_CONTAINER (assistant), widget);
		gtk_assistant_insert_page (GTK_ASSISTANT (druid->window), widget, current + 1);
		gtk_assistant_set_page_type (GTK_ASSISTANT (druid->window), widget, type);
		gtk_assistant_set_page_complete (GTK_ASSISTANT (druid->window), widget, TRUE);
		gtk_widget_destroy (GTK_WIDGET (assistant));

		/* Builder get reference on all built widget, so unref it when all work is done */
		g_object_unref (builder);

		/* Create new page */
		page = npw_page_new (druid->values);
		npw_page_set_widget (page, table);

		/* Add page in cache */
		g_queue_push_tail (druid->page_list, page);
	}

	return page;
}

/* Remove all pages following current page except the summary page*/

static void
npw_druid_remove_following_page (NPWDruid* druid)
{
	gint current;

	current = gtk_assistant_get_current_page (GTK_ASSISTANT (druid->window));
	for(;;)
	{
		GtkWidget *widget;
		NPWPage* page;

		widget = gtk_assistant_get_nth_page (GTK_ASSISTANT (druid->window), current + 1);
		if (widget == druid->finish_page) break;

		gtk_container_remove (GTK_CONTAINER (druid->window), widget);

		page = g_queue_pop_nth (druid->page_list, current  - (druid->no_selection ? 0 : 1));
		if (page != NULL) npw_page_free (page);
	}
}

/* Save properties
 *---------------------------------------------------------------------------*/

typedef struct _NPWSaveValidPropertyData
{
	GtkWindow *parent;
	gboolean modified;
	GString *error;
	GString *warning;

} NPWSaveValidPropertyData;

static void
cb_save_valid_property (NPWProperty* property, gpointer user_data)
{
	NPWSaveValidPropertyData* data = (NPWSaveValidPropertyData *)user_data;
	const gchar* value;
	gboolean modified;

	/* Get value from widget */
	modified = npw_property_update_value_from_widget (property);
	if (modified) data->modified = modified;
       	value = npw_property_get_value (property);

	/* Check mandatory property */
	if (modified &&  (npw_property_get_options (property) & NPW_MANDATORY_OPTION))
	{
		if ((value == NULL) || (strlen (value) <= 0))
		{
			g_string_append_printf (data->error,
									_("\nField \"%s\" is mandatory. Please enter it."),
									npw_property_get_label (property));
			npw_property_remove_value (property);
		}
	}

	/* Check restricted property */
	if (modified && !npw_property_is_valid_restriction (property))
	{
		NPWPropertyRestriction restriction = npw_property_get_restriction (property);

		switch (restriction)
		{
		case NPW_FILENAME_RESTRICTION:
			g_string_append_printf (data->error,
									_("Field \"%s\" must contains only letters, digits or the following characters \"#$:%%+,.=@^_`~\". In addition you cannot have a leading dash. Please fix it."),
									npw_property_get_label (property));
			break;
		case NPW_DIRECTORY_RESTRICTION:
			g_string_append_printf (data->error,
									_("Field \"%s\" must contains only letters, digits, the following characters \"#$:%%+,.=@^_`~\" or directory separators. In addition you cannot have a leading dash. Please fix it."),
									npw_property_get_label (property));
			break;
		case NPW_PRINTABLE_RESTRICTION:
			g_string_append_printf (data->error,
									_("Field \"%s\" must contains only ASCII printable characters, no accentuated characters by example. Please fix it."),
									npw_property_get_label (property));
			break;
		default:
			g_string_append_printf (data->error,
									_("Unknown error."));

		}
		npw_property_remove_value (property);
	}

	/* Check exist property */
	if (modified && (npw_property_get_exist_option (property) == NPW_FALSE))
	{
		gboolean is_directory = npw_property_get_type (property) == NPW_DIRECTORY_PROPERTY;
		gboolean exist = (value != NULL) && g_file_test (value, G_FILE_TEST_EXISTS);
		/* Allow empty directory */
		if (exist && is_directory)
		{
			GDir* dir;

			dir = g_dir_open (value, 0, NULL);
			if (dir != NULL)
			{
				if (g_dir_read_name (dir) == NULL) exist = FALSE;
				g_dir_close (dir);
			}
		}

		if (exist)
		{
			g_string_append_printf (data->warning, is_directory ?
										_("Directory \"%s\" is not empty. Project creation could fail if some files "
										  "cannot be written. Do you want to continue?") :
											_("File \"%s\" already exists. Do you want to overwrite it?"),
										value);
		}
	}
};

/* Save values and check them, return TRUE if it's possible to go to
 * next page */

static gboolean
npw_druid_save_valid_values (NPWDruid* druid)
{
	gint current;
	NPWPage* page;
	NPWSaveValidPropertyData data;
	gboolean ok = TRUE;

	current = gtk_assistant_get_current_page (GTK_ASSISTANT (druid->window))  - (druid->no_selection ? 0 : 1) - 1;
	page = g_queue_peek_nth (druid->page_list, current);
	data.modified = FALSE;
	data.parent = GTK_WINDOW (druid->window);
	data.error = g_string_new (NULL);
	data.warning = g_string_new (NULL);
	npw_page_foreach_property (page, (GFunc)cb_save_valid_property, &data);

	if (data.modified) npw_druid_remove_following_page (druid);

	if (data.error->len)
	{
		npw_druid_fill_error_page (druid, NULL,
								   GTK_MESSAGE_ERROR,
								   NULL,
									"<b>%s</b>\n\n%s",
									_("Invalid entry"),
									data.error->str);
		ok = FALSE;
	}
	else if (data.warning->len)
	{
		npw_druid_fill_error_page (druid, NULL,
								   GTK_MESSAGE_WARNING,
								   NULL,
									"<b>%s</b>\n\n%s",
									_("Dubious entry"),
									data.warning->str);
		ok = FALSE;
	}

	g_string_free (data.error, TRUE);
	g_string_free (data.warning, TRUE);

	return ok;
}

/* Druid call backs
 *---------------------------------------------------------------------------*/

static void
on_druid_cancel (GtkAssistant* window, NPWDruid* druid)
{
	anjuta_plugin_deactivate (ANJUTA_PLUGIN (druid->plugin));
	npw_druid_free (druid);
}

static void
on_druid_close (GtkAssistant* window, NPWDruid* druid)
{
	anjuta_plugin_deactivate (ANJUTA_PLUGIN (druid->plugin));
	npw_druid_free (druid);
}

static gboolean
on_project_wizard_key_press_event(GtkWidget *widget, GdkEventKey *event,
                               NPWDruid* druid)
{
	if (event->keyval == GDK_KEY_Escape)
	{
		on_druid_cancel (GTK_ASSISTANT (widget), druid);
		return TRUE;
	}
	return FALSE;
}

static void
on_druid_get_new_page (AnjutaAutogen* gen, gpointer data)
{
	NPWDruid* druid = (NPWDruid *)data;
	gint current;
	NPWPage* page;

	current = gtk_assistant_get_current_page (GTK_ASSISTANT (druid->window));
	page = g_queue_peek_nth (druid->page_list, current - (druid->no_selection ? 0 : 1));

	if (npw_page_get_name (page) == NULL)
	{
		/* no page, display finish page */
		npw_druid_fill_summary_page (druid);

		page = g_queue_pop_nth (druid->page_list, current - (druid->no_selection ? 0 : 1));
		if (page != NULL) npw_page_free (page);
		gtk_container_remove (GTK_CONTAINER (druid->window), gtk_assistant_get_nth_page (GTK_ASSISTANT (druid->window), current + 1));
		gtk_assistant_set_current_page (GTK_ASSISTANT (druid->window), current + 1);
	}
	else
	{
		/* display property page */
		npw_druid_fill_property_page (druid, page);

		gtk_assistant_set_current_page (GTK_ASSISTANT (druid->window), current + 1);
	}
}

static void
on_druid_parse_page (const gchar* output, gpointer data)
{
	GError *error = NULL;
	NPWPageParser* parser = (NPWPageParser*)data;

	npw_page_parser_parse (parser, output, strlen (output), &error);

	if (error)
	{
		g_warning ("Parser error: %s", error->message);
		g_error_free (error);
	}
}

static void
strip_package_version_info (gpointer data, gpointer user_data)
{
	gchar * const pkg = (gchar *) data;

	if (!data)
		return;
	g_strdelimit (pkg, " ", '\0');
}

static void
on_install_button_clicked (GtkWidget *button, NPWDruid *druid)
{
	GList *missing_programs, *missing_packages;
	GList *missing_files = NULL;
	GList *node;


	missing_programs = npw_header_check_required_programs (druid->header);
	missing_packages = npw_header_check_required_packages (druid->header);

	anjuta_util_glist_strings_prefix (missing_programs, "/usr/bin/");

	/* Search for "pkgconfig(pkg_name)" */
	g_list_foreach (missing_packages, (GFunc) strip_package_version_info,
					NULL);
	missing_files = g_list_concat (missing_files, missing_programs);

	for (node = missing_packages; node != NULL; node = g_list_next (missing_packages))
	{
		gchar* pk_pkg_config_string = g_strdup_printf ("pkgconfig(%s)", (gchar*) node->data);
		missing_files = g_list_append (missing_files, pk_pkg_config_string);
	}
	g_list_free (missing_packages);

	if (missing_files)
	{
		gchar * missing_names = NULL;

		missing_names = anjuta_util_glist_strings_join (missing_files, ", ");
		anjuta_util_install_files (missing_names);

		if (missing_names)
			g_free (missing_names);
		anjuta_util_glist_strings_free (missing_files);
	}
}

static gboolean
check_and_warn_missing (NPWDruid *druid)
{
	GList *missing_programs, *missing_packages;
	GString *missing_message = NULL;

	missing_programs = npw_header_check_required_programs (druid->header);
	missing_packages = npw_header_check_required_packages (druid->header);

	if (missing_programs || missing_packages)
	{
		missing_message = g_string_new (NULL);
	}

	if (missing_programs)
	{
		gchar *missing_progs;
		missing_progs = anjuta_util_glist_strings_join (missing_programs,
														", ");
		g_string_append_printf (missing_message,
								_("\nMissing programs: %s."), missing_progs);
		g_free (missing_progs);
		anjuta_util_glist_strings_free (missing_programs);
	}

	if (missing_packages)
	{
		gchar *missing_pkgs;
		missing_pkgs = anjuta_util_glist_strings_join (missing_packages,
													   ", ");
		g_string_append_printf (missing_message,
								_("\nMissing packages: %s."), missing_pkgs);
		g_free (missing_pkgs);
		anjuta_util_glist_strings_free (missing_packages);
	}

	if (missing_message)
	{
		GtkWidget *hbox, *install_button;
		g_string_prepend (missing_message, _(
		 "Some important programs or development packages required to build "
		 "this project are missing. Please make sure they are "
		 "installed properly before generating the project.\n"));

		hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_widget_show (hbox);

#ifdef ENABLE_PACKAGEKIT
		install_button =
			gtk_button_new_with_label (_("Install missing packages"));
		gtk_box_pack_end (GTK_BOX (hbox), install_button, FALSE, FALSE, 10);
		g_signal_connect (install_button, "clicked",
						  G_CALLBACK (on_install_button_clicked), druid);
		gtk_widget_show (install_button);
#endif

		npw_druid_fill_error_page (druid, hbox,
								   GTK_MESSAGE_WARNING,
								   /* Translators: Application Manager is the program used to install
								    * new application like apt on Ubuntu, yum on Fedora, zypper on
								    * OpenSuSE and emerge on Gentoo */
								  _("The missing programs are usually part of some distribution "
									"packages and can be searched for in your Application Manager. "
									"Similarly, the development packages are contained in special "
									"packages that your distribution provides to allow development "
									"of projects based on them. They usually end with a \"-dev\" or "
									"\"-devel\" suffix in package names and can be found by searching "
									"in your Application Manager."),
									"<b>%s</b>\n\n%s",
									_("Missing components"),
									missing_message->str);
		g_string_free (missing_message, TRUE);
	}

	return !missing_message;
}


static void
on_druid_real_prepare (GtkAssistant* assistant, GtkWidget *page, NPWDruid* druid)
{
	if (page == druid->progress_page)
	{
		gint previous;
		gboolean last_warning;

		previous = gtk_assistant_get_current_page (assistant) - 1;
		last_warning = gtk_assistant_get_nth_page (assistant, previous) == druid->error_page;
		if (last_warning)
		{
			/* Remove warning page */
			gtk_container_remove (GTK_CONTAINER (assistant), druid->error_page);
			previous--;
		}
		if (druid->no_selection) previous++;

		/* Generate the next page */
		if (previous == PROJECT_PAGE_INDEX)
		{
			const gchar* new_project;

			new_project = npw_header_get_filename (druid->header);

			if (druid->project_file != new_project)
			{
				npw_druid_remove_following_page (druid);

				/* Check if necessary programs for this project is installed */
				if (!last_warning && !check_and_warn_missing (druid))
				{
					gtk_assistant_set_current_page (assistant, gtk_assistant_get_current_page (assistant) + 1);
					return;
				}

				/* Change project */
				druid->project_file = new_project;
				anjuta_autogen_set_input_file (druid->gen, druid->project_file, "[+","+]");

			}
		}
		else
		{
			if (!npw_druid_save_valid_values (druid))
			{
				/* Display error */
				gtk_assistant_set_current_page (assistant, gtk_assistant_get_current_page (assistant) + 1);

				return;
			}
		}

		if (g_queue_peek_nth (druid->page_list, previous) == NULL)
		{
			/* Regenerate new page */
			gtk_assistant_set_page_complete (assistant, page, FALSE);
			if (druid->parser != NULL)
				npw_page_parser_free (druid->parser);
			druid->parser = npw_page_parser_new (npw_druid_add_new_page (druid), druid->project_file, previous);

			anjuta_autogen_set_output_callback (druid->gen, on_druid_parse_page, druid->parser, NULL);
			anjuta_autogen_write_definition_file (druid->gen, druid->values, NULL);
			anjuta_autogen_execute (druid->gen, on_druid_get_new_page, druid, NULL);
		}
		else
		{
			/* Page is already in cache, change the page to display it */
			on_druid_get_new_page (NULL, druid);
		}
	}
	else if (page == druid->finish_page)
	{
		npw_druid_set_busy (druid, FALSE);
		gtk_container_remove (GTK_CONTAINER (assistant), druid->error_page);
		gtk_container_remove (GTK_CONTAINER (assistant), druid->progress_page);
	}
	else
	{
		npw_druid_set_busy (druid, FALSE);

		if (page != druid->error_page) gtk_container_remove (GTK_CONTAINER (assistant), druid->error_page);

		/* Move progress page */
		gtk_container_remove (GTK_CONTAINER (assistant), druid->progress_page);
		gtk_assistant_insert_page (assistant, druid->progress_page, gtk_assistant_get_current_page (assistant) + 1);
		gtk_assistant_set_page_title (assistant, druid->progress_page, "...");
	}
}

static gboolean
on_druid_delayed_prepare (gpointer data)
{
	NPWDruid *druid = (NPWDruid *)data;
	GtkAssistant *assistant;
	GtkWidget *page;

	assistant = GTK_ASSISTANT (druid->window);
	page = gtk_assistant_get_nth_page (assistant, gtk_assistant_get_current_page (assistant));
	on_druid_real_prepare (assistant, page, druid);

	return FALSE;
}

static void
on_druid_prepare (GtkAssistant* assistant, GtkWidget *page, NPWDruid* druid)
{
	/* The page change is delayed because in the latest version of
	 * GtkAssistant, the page switch is not completely done when
	 * the signal is called. A page change in the signal handler
	 * will be partialy overwritten */
	g_idle_add (on_druid_delayed_prepare, druid);
}


static void
on_druid_finish (GtkAssistant* assistant, NPWDruid* druid)
{
	NPWInstall* inst;
	GList *path;

	inst = npw_install_new (druid->plugin);
	npw_install_set_property (inst, druid->values);
	npw_install_set_wizard_file (inst, npw_header_get_filename (druid->header));
	for (path = g_list_last (anjuta_autogen_get_library_paths (druid->gen)); path != NULL; path = g_list_previous (path))
	{
		npw_install_set_library_path (inst, (const gchar *)path->data);
	}
	npw_install_launch (inst);
}

static GtkWidget*
npw_druid_create_assistant (NPWDruid* druid, GFile *templates)
{
	AnjutaShell *shell;
	GtkBuilder *builder;
	GError* error = NULL;
	GtkAssistant *assistant;
	GtkWidget *property_page;

	g_return_val_if_fail (druid->window == NULL, NULL);

	shell = ANJUTA_PLUGIN (druid->plugin)->shell;

	/* Create GtkAssistant using GtkBuilder, glade doesn't seem to work*/
	builder = gtk_builder_new ();
	if (!gtk_builder_add_from_file (builder, GTK_BUILDER_UI_FILE, &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
		return NULL;
	}
	anjuta_util_builder_get_objects (builder,
	                                 NEW_PROJECT_DIALOG, &assistant,
	                                 PROJECT_BOOK, &druid->project_book,
	                                 ERROR_VBOX, &druid->error_vbox,
	                                 ERROR_TITLE, &druid->error_title,
	                                 ERROR_ICON, &druid->error_icon,
	                                 ERROR_MESSAGE, &druid->error_message,
	                                 ERROR_DETAIL, &druid->error_detail,
	                                 PROJECT_PAGE, &druid->project_page,
	                                 ERROR_PAGE, &druid->error_page,
	                                 PROGRESS_PAGE, &druid->progress_page,
	                                 FINISH_PAGE, &druid->finish_page,
	                                 FINISH_TEXT, &druid->finish_text,
	                                 PROPERTY_PAGE, &property_page,
	                                 NULL);
	druid->window = GTK_WINDOW (assistant);
	gtk_window_set_transient_for (GTK_WINDOW (assistant), GTK_WINDOW (shell));
	g_object_unref (builder);

	/* Connect assistant signals */
	g_signal_connect (G_OBJECT (assistant), "prepare", G_CALLBACK (on_druid_prepare), druid);
	g_signal_connect (G_OBJECT (assistant), "apply", G_CALLBACK (on_druid_finish), druid);
	g_signal_connect (G_OBJECT (assistant), "cancel", G_CALLBACK (on_druid_cancel), druid);
	g_signal_connect (G_OBJECT (assistant), "close", G_CALLBACK (on_druid_close), druid);
	g_signal_connect(G_OBJECT(assistant), "key-press-event", G_CALLBACK(on_project_wizard_key_press_event), druid);

	/* Remove property page, will be created later as needed */
	gtk_container_remove (GTK_CONTAINER (assistant), property_page);
	/* Remove error page, could be needed later so keep a ref */
	g_object_ref (druid->error_page);
	gtk_container_remove (GTK_CONTAINER (assistant), druid->error_page);
	/* Remove progress page, could be needed later so keep a ref */
	g_object_ref (druid->progress_page);
	gtk_container_remove (GTK_CONTAINER (assistant), druid->progress_page);

	/* Setup project selection page */
	if (!npw_druid_fill_selection_page (druid, templates))
	{
		return NULL;
	}

	/* Add dialog widget to anjuta status. */
	anjuta_status_add_widget (anjuta_shell_get_status (shell, NULL), GTK_WIDGET (assistant));

	gtk_window_set_default_size (GTK_WINDOW (assistant),
	                             600, 500);

	gtk_widget_show_all (GTK_WIDGET (assistant));

	return GTK_WIDGET(assistant);
}

/* Add default property
 *---------------------------------------------------------------------------*/

static void
npw_druid_add_default_property (NPWDruid* druid)
{
	gchar* s;
	GSettings *settings;
	gboolean flag;
	gint i;

	/* Add default base project directory */
	g_hash_table_insert (druid->values, g_strdup (ANJUTA_PROJECT_DIRECTORY_PROPERTY), g_strdup (g_get_home_dir()));

	/* Add user name */
	g_hash_table_insert (druid->values, g_strdup (USER_NAME_PROPERTY), g_strdup (g_get_real_name()));

	/* Add Email address */
	/* FIXME: We need a default way for the mail */
	s = anjuta_util_get_user_mail();
	g_hash_table_insert (druid->values, g_strdup (EMAIL_ADDRESS_PROPERTY), s);

	/* Add use-tabs property */
	settings = g_settings_new (ANJUTA_PREF_SCHEMA_PREFIX IANJUTA_EDITOR_PREF_SCHEMA);
	flag = g_settings_get_boolean (settings, IANJUTA_EDITOR_USE_TABS_KEY);
	g_hash_table_insert (druid->values, g_strdup (USE_TABS_PROPERTY), g_strdup (flag ? "1" : "0"));

	/* Add tab-width property */
	i = g_settings_get_int (settings, IANJUTA_EDITOR_TAB_WIDTH_KEY);
	g_hash_table_insert (druid->values, g_strdup (TAB_WIDTH_PROPERTY), g_strdup_printf("%d", i));

	/* Add indent-width property */
	i = g_settings_get_int (settings, IANJUTA_EDITOR_INDENT_WIDTH_KEY);
	g_hash_table_insert (druid->values, g_strdup (INDENT_WIDTH_PROPERTY), g_strdup_printf("%d", i));
	g_object_unref (settings);
}

/* Druid public functions
 *---------------------------------------------------------------------------*/

NPWDruid*
npw_druid_new (NPWPlugin* plugin, GFile *templates)
{
	NPWDruid* druid;

	/* Check if autogen is present */
	if (!anjuta_check_autogen())
	{
		anjuta_util_dialog_error (NULL, _("Could not find autogen version 5; please install the autogen package. You can get it from http://autogen.sourceforge.net."));
		return NULL;
	}

	druid = g_new0(NPWDruid, 1);
	druid->plugin = plugin;
	druid->project_file = NULL;
	druid->busy = FALSE;
	druid->no_selection = FALSE;
	druid->page_list = g_queue_new ();
	druid->values = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify)g_free);
	druid->gen = anjuta_autogen_new ();
	plugin->druid = druid;
	druid->error_extra_widget = NULL;

	if (npw_druid_create_assistant (druid, templates) == NULL)
	{
		npw_druid_free (druid);

		return NULL;
	}

	npw_druid_add_default_property (druid);

	return druid;
}

void
npw_druid_free (NPWDruid* druid)
{
	NPWPage* page;

	g_return_if_fail (druid != NULL);

	/* Delete page list */

	while ((page = (NPWPage *)g_queue_pop_head (druid->page_list)) != NULL)
	{
		npw_page_free (page);
	}
	g_queue_free (druid->page_list);
	g_hash_table_destroy (druid->values);
	g_object_unref (G_OBJECT (druid->gen));
	if (druid->parser != NULL) npw_page_parser_free (druid->parser);
	npw_header_list_free (druid->header_list);
	/* Destroy project notebook first to avoid a critical warning */
	gtk_widget_destroy (GTK_WIDGET (druid->project_book));
	gtk_widget_destroy (GTK_WIDGET (druid->window));
	g_object_unref (druid->error_page);
	g_object_unref (druid->progress_page);
	druid->plugin->druid = NULL;
	g_free (druid);
}

void
npw_druid_show (NPWDruid* druid)
{
	g_return_if_fail (druid != NULL);

	/* Display dialog box */
	if (druid->window) gtk_window_present (druid->window);
}
