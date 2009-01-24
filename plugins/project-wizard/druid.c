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
#include "autogen.h"

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/interfaces/ianjuta-wizard.h>

#include <gnome.h>
#include <glib/gi18n.h>

#include <gtk/gtk.h>

#include <string.h>

/*---------------------------------------------------------------------------*/

#define PROJECT_WIZARD_DIRECTORY PACKAGE_DATA_DIR"/project"

/* Default property name useable in wizard file
 *---------------------------------------------------------------------------*/

#define ANJUTA_PROJECT_DIRECTORY_PROPERTY "AnjutaProjectDirectory"
#define USER_NAME_PROPERTY "UserName"
#define EMAIL_ADDRESS_PROPERTY "EmailAddress"

/* Widget and signal name found in glade file
 *---------------------------------------------------------------------------*/

#define GTK_BUILDER_UI_FILE PACKAGE_DATA_DIR"/glade/anjuta-project-wizard.ui"

#define NEW_PROJECT_DIALOG "druid_window"
#define PROJECT_LIST "project_list"
#define PROJECT_BOOK "project_book"
#define PROPERTY_TABLE "property_table"
#define ERROR_ICON "error_icon"
#define ERROR_MESSAGE "error_message"
#define ERROR_DETAIL "error_detail"

#define PROJECT_PAGE 0
#define ERROR_PAGE 1
#define PROGRESS_PAGE 2
#define FINISH_PAGE 3
#define PROPERTY_PAGE 4

/*---------------------------------------------------------------------------*/

struct _NPWDruid
{
	GtkWindow* window;
	
	GtkNotebook* project_book;
	GtkImage *error_icon;
	GtkLabel *error_message;
	GtkWidget *error_detail;

	GtkTooltips *tooltips;
	
	const gchar* project_file;
	NPWPlugin* plugin;
	
	gint next_page;
	gint last_page;
	
	GQueue* page_list;
	GHashTable* values;
	NPWPageParser* parser;
	GList* header_list;
	NPWHeader* header;
	NPWAutogen* gen;
	gboolean busy;
};

/* column of the icon view */
enum {
	PIXBUF_COLUMN,
	TEXT_COLUMN,
	DESC_COLUMN,
	DATA_COLUMN
};

/* Create error page
 *---------------------------------------------------------------------------*/

/* Fill dialog page */
static void
npw_druid_fill_error_page (NPWDruid* druid, GtkMessageType type, const gchar *detail, const gchar *mesg,...)
{
	GtkAssistant *assistant;
	GtkWidget *page;
	va_list args;
	gchar *message;
	const gchar *stock_id = NULL;
	const gchar *title = NULL;
	
	assistant = GTK_ASSISTANT (druid->window);
	page = gtk_assistant_get_nth_page (assistant, ERROR_PAGE);
	
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
								_(npw_property_get_label (property)), 
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

	text = g_string_new (_("<b>Confirm the following information:</b>\n\n"));

	/* The project type is translated too, it is something like
	 * generic, GNOME applet, Makefile project... */
	g_string_append_printf (text, _("Project Type: %s\n"), _(npw_header_get_name (druid->header)));

	for (i = 0; (page = g_queue_peek_nth (druid->page_list, i)) != NULL; ++i)
	{
		npw_page_foreach_property (page, (GFunc)cb_druid_add_summary_property, text);
	}
	
	label = GTK_LABEL (gtk_assistant_get_nth_page (GTK_ASSISTANT (druid->window), FINISH_PAGE));
	gtk_label_set_markup (label, text->str);
	g_string_free (text, TRUE);
	
	gtk_assistant_set_page_complete (GTK_ASSISTANT (druid->window), GTK_WIDGET (label), TRUE);
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
	gtk_assistant_set_page_complete (GTK_ASSISTANT (druid->window),
									 gtk_assistant_get_nth_page (GTK_ASSISTANT (druid->window),PROJECT_PAGE),
									 header != NULL);
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
						TEXT_COLUMN, _(npw_header_get_name (header)),
						DESC_COLUMN, _(npw_header_get_description (header)),
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
	store = gtk_list_store_new (4, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
	g_list_foreach (template_list, cb_druid_insert_project_icon, store); 
	gtk_icon_view_set_model (view, GTK_TREE_MODEL (store));
	
	/* Connect signal to update dialog */
	g_signal_connect (G_OBJECT (view), "selection-changed", G_CALLBACK (on_druid_project_update_selected), druid);
	g_signal_connect (G_OBJECT (view), "map", G_CALLBACK (on_druid_project_update_selected), druid);

	/* Set new page label */
	assistant = GTK_WIDGET (gtk_builder_get_object (builder, NEW_PROJECT_DIALOG));
	book = GTK_NOTEBOOK (gtk_assistant_get_nth_page (GTK_ASSISTANT (assistant), PROJECT_PAGE));
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
npw_druid_fill_selection_page (NPWDruid* druid)
{
	gboolean ok;
	gchar* dir;
	const gchar * const * sys_dir;
	const gchar * user_dir;
 
	/* Remove all previous data */
	druid->project_book = GTK_NOTEBOOK (gtk_assistant_get_nth_page (GTK_ASSISTANT (druid->window), PROJECT_PAGE));
	gtk_notebook_remove_page(druid->project_book, 0);	
	npw_header_list_free (druid->header_list);	
	
	/* Create list of projects */
	druid->header_list = npw_header_list_new ();	
	
	/* Read project template in user directory,
	 * normally ~/.local/share/anjuta/project,
	 * the first template read override the others */
	dir = g_build_filename (g_get_user_data_dir (), "anjuta", "project", NULL);
	npw_header_list_readdir (&druid->header_list, dir);
	g_free (dir);
	
	/* Read project template in system directory */	
	for (sys_dir = g_get_system_data_dirs (); *sys_dir != NULL; sys_dir++)
	{
		dir = g_build_filename (*sys_dir, "anjuta", "project", NULL);
		npw_header_list_readdir (&druid->header_list, PROJECT_WIZARD_DIRECTORY);
		g_free (dir);
	}

	/* Read anjuta installation directory */
	npw_header_list_readdir (&druid->header_list, PROJECT_WIZARD_DIRECTORY);
	
	if (g_list_length (druid->header_list) == 0)
	{
		anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (druid->plugin)->shell),_("Unable to find any project template in %s"), PROJECT_WIZARD_DIRECTORY);		
		return FALSE;
	}

	/* Add all necessary notebook page */
	g_list_foreach (druid->header_list, cb_druid_insert_project_page, druid);

	gtk_widget_show_all (GTK_WIDGET (druid->project_book));
	
	return TRUE;
}

/* Create properties pages
 *---------------------------------------------------------------------------*/

/* Group infomation need by the following call back function */
typedef struct _NPWDruidAddPropertyData
{
	NPWDruid* druid;
	guint row;
	GtkTable *table;
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
			GtkTooltips *tooltips;
		
			tooltips = data->druid->tooltips;
			if (!tooltips)
			{
				tooltips = data->druid->tooltips = gtk_tooltips_new ();
				data->druid->tooltips = tooltips;
				g_object_ref (tooltips);
				gtk_object_sink (GTK_OBJECT (tooltips));
			}
			gtk_tooltips_set_tip (tooltips, entry, description, NULL);
		}

		/* Add label and entry */
		gtk_table_resize (data->table, data->row + 1, 2);
		label = gtk_label_new (_(npw_property_get_label (property)));
		gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
		gtk_misc_set_padding (GTK_MISC (label), 6, 6);
		gtk_table_attach (data->table, label, 0, 1, data->row, data->row + 1,
			(GtkAttachOptions)(GTK_FILL), (GtkAttachOptions)(0), 0, 0);
		gtk_table_attach (data->table, entry, 1, 2, data->row, data->row + 1,
			(GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(0), 0, 0);
		
		data->row++;
	}
};

static void
npw_druid_fill_property_page (NPWDruid* druid, NPWPage* page)
{
	GtkWidget *widget;
	NPWDruidAddPropertyData data;

	widget = gtk_assistant_get_nth_page (GTK_ASSISTANT (druid->window), druid->next_page);
	
	/* Remove previous widgets */
	gtk_container_foreach (GTK_CONTAINER (npw_page_get_widget (page)), cb_druid_destroy_widget, NULL);
 		
	/* Update title	*/
	gtk_assistant_set_page_title (GTK_ASSISTANT (druid->window), widget, _(npw_page_get_label (page)));

	/* Add new widget */
	data.druid = druid;
	data.row = 0;
	data.table = GTK_TABLE (npw_page_get_widget (page));
	npw_page_foreach_property (page, (GFunc)cb_druid_add_property, &data);
	
	gtk_widget_show_all (widget);
}

/* Handle page cache
 *---------------------------------------------------------------------------*/

static NPWPage*
npw_druid_add_new_page (NPWDruid* druid)
{
	NPWPage* page;

	/* Get page in cache */
	page = g_queue_peek_nth (druid->page_list, druid->next_page - PROPERTY_PAGE);
	if (page == NULL)
	{
		/* Page not found in cache, create */
		GtkBuilder *builder;
		GtkWidget *widget;
		GtkAssistantPageType type;
		GdkPixbuf *pixbuf;
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
		table = GTK_WIDGET (gtk_builder_get_object (builder, PROPERTY_TABLE));
		
		widget = gtk_assistant_get_nth_page (assistant, PROPERTY_PAGE);
		type = gtk_assistant_get_page_type (assistant, widget);
		pixbuf = gtk_assistant_get_page_header_image (assistant, widget);
		if (pixbuf) g_object_ref (pixbuf);
		gtk_container_remove (GTK_CONTAINER (assistant), widget);
		gtk_assistant_insert_page (GTK_ASSISTANT (druid->window), widget, druid->next_page);
		gtk_assistant_set_page_type (GTK_ASSISTANT (druid->window), widget, type);
		if (pixbuf != NULL)
		{	
			gtk_assistant_set_page_header_image (GTK_ASSISTANT (druid->window), widget, pixbuf);
			g_object_ref (pixbuf);
		}
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

/* Remove all following page */

static void
npw_druid_remove_following_page (NPWDruid* druid)
{
	NPWPage* page;
	gint num = druid->next_page;

	for(;;)
	{
		GtkWidget *widget;
		
		page = g_queue_pop_nth (druid->page_list, num - PROPERTY_PAGE);
		if (page == NULL) break;
		
		widget = gtk_assistant_get_nth_page (GTK_ASSISTANT (druid->window), num);
		gtk_container_remove (GTK_CONTAINER (druid->window), widget);
		
		npw_page_free (page);
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
									_(npw_property_get_label (property)));
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
									_("Field \"%s\" must start with a letter, a digit or an underscore and contains only letters, digits, underscore, minus and dot. Please fix it."),
									_(npw_property_get_label (property)));
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
	NPWPage* page;
	NPWSaveValidPropertyData data;
	gboolean ok = TRUE;

	page = g_queue_peek_nth (druid->page_list, druid->next_page - PROPERTY_PAGE - 1);
	data.modified = FALSE;
	data.parent = GTK_WINDOW (druid->window);
	data.error = g_string_new (NULL);
	data.warning = g_string_new (NULL);
	npw_page_foreach_property (page, (GFunc)cb_save_valid_property, &data);

	if (data.modified) npw_druid_remove_following_page (druid);
	
	if (data.error->len)
	{
		npw_druid_fill_error_page (druid,
								   GTK_MESSAGE_ERROR,
								   NULL,
									"<b>%s</b>\n\n%s",
									_("Invalid entry"),
									data.error->str);
		gtk_assistant_set_current_page (GTK_ASSISTANT (druid->window), ERROR_PAGE);
		ok = FALSE;
	}
	else if (data.warning->len)
	{
		npw_druid_fill_error_page (druid,
								   GTK_MESSAGE_WARNING,
								   NULL,
									"<b>%s</b>\n\n%s",
									_("Dubious entry"),
									data.warning->str);
		gtk_assistant_set_current_page (GTK_ASSISTANT (druid->window), ERROR_PAGE);
		ok = FALSE;
	}
	
	g_string_free (data.error, TRUE);
	g_string_free (data.warning, TRUE);

	return ok;
}

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
	if (event->keyval == GDK_Escape)
	{
		on_druid_cancel (GTK_ASSISTANT (widget), druid);
		return TRUE;
	}
	return FALSE;
}

static void
on_druid_get_new_page (NPWAutogen* gen, gpointer data)
{
	NPWDruid* druid = (NPWDruid *)data;
	NPWPage* page;

	page = g_queue_peek_nth (druid->page_list, druid->next_page - PROPERTY_PAGE);

	if (npw_page_get_name (page) == NULL)
	{
		/* no page, display finish page */
		npw_druid_fill_summary_page (druid);
		
		gtk_assistant_set_current_page (GTK_ASSISTANT (druid->window), FINISH_PAGE);
	}
	else
	{
		/* display property page */
		npw_druid_fill_property_page (druid, page);

		gtk_assistant_set_current_page (GTK_ASSISTANT (druid->window), druid->next_page);
	}
}

static void
on_druid_parse_page (const gchar* output, gpointer data)
{
	NPWPageParser* parser = (NPWPageParser*)data;
	
	npw_page_parser_parse (parser, output, strlen (output), NULL);
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
		g_list_free (missing_programs);
	}
	
	if (missing_packages)
	{
		gchar *missing_pkgs;
		missing_pkgs = anjuta_util_glist_strings_join (missing_packages,
													   ", ");
		g_string_append_printf (missing_message,
								_("\nMissing packages: %s."), missing_pkgs);
		g_free (missing_pkgs);
		g_list_free (missing_packages);
	}
	
	if (missing_message)
	{
		g_string_prepend (missing_message, _(
		 "Some important programs or development packages required to build "
		 "this project are missing. Please make sure they are "
		 "installed properly before generating the project.\n"));

		npw_druid_fill_error_page (druid,
								   GTK_MESSAGE_WARNING,
								  _("The missing programs are usually part of some distrubution "
									"packages and can be searched in your Application Manager. "
									"Similarly, the development packages are contained in special "
									"packages that your distribution provide to allow development "
									"of projects based on them. They usually end with -dev or "
									"-devel suffix in package names and can be found by searching "
									"in your Application Manager."),
									"<b>%s</b>\n\n%s",
									_("Missing components"),
									missing_message->str);
		g_string_free (missing_message, TRUE);
		
		gtk_assistant_set_current_page (GTK_ASSISTANT (druid->window), ERROR_PAGE);
	}
	
	return !missing_message;
}

/* This function is called to compute the next page AND to determine the status
 * of last button: it does not mean that the user go to the next page. 
 * We need this information to generate the next page just before displaying it.
 * So a progress page is inserted between all pages of the wizard. We use the
 * prepare signal handler of this page to compute the next page and switch to
 * it using gtk_assistant_set_current_page(). */
static gint
on_druid_next (gint page, gpointer user_data)
{
	return page == FINISH_PAGE ? -1 : PROGRESS_PAGE;
}

static void
on_druid_prepare (GtkAssistant* assistant, GtkWidget *page, NPWDruid* druid)
{
	gint current_page = gtk_assistant_get_current_page (assistant);
	
	if (current_page == PROGRESS_PAGE)
	{
		/* Generate the next page */

		if (druid->next_page == PROPERTY_PAGE)
		{
			const gchar* new_project;

			new_project = npw_header_get_filename (druid->header);
			
			if (druid->project_file != new_project)
			{	
				if (druid->last_page != ERROR_PAGE)
				/* Check if necessary programs for this project is installed */
				if (!check_and_warn_missing (druid))
				{   
					return;
				}   
				
				/* Change project */
				druid->project_file = new_project;
				npw_druid_remove_following_page (druid);
				npw_autogen_set_input_file (druid->gen, druid->project_file, "[+","+]");
				
			}
		}
		else
		{
			if (!npw_druid_save_valid_values (druid))
			{
				/* Display error */
				gtk_assistant_set_current_page (assistant, ERROR_PAGE);
				
				return;
			}
		}

		if (g_queue_peek_nth (druid->page_list, druid->next_page - PROPERTY_PAGE) == NULL)
		{
			/* Regenerate new page */
			gtk_assistant_set_page_complete (assistant, page, FALSE);
			if (druid->parser != NULL)
				npw_page_parser_free (druid->parser);
			druid->parser = npw_page_parser_new (npw_druid_add_new_page (druid), druid->project_file, druid->next_page - PROPERTY_PAGE);
			
			npw_autogen_set_output_callback (druid->gen, on_druid_parse_page, druid->parser);
			npw_autogen_write_definition_file (druid->gen, druid->values);
			npw_autogen_execute (druid->gen, on_druid_get_new_page, druid, NULL);
		}
		else
		{
			/* Page is already in cache */
			on_druid_get_new_page (NULL, (gpointer)druid);
		}
	}
	else if (current_page == ERROR_PAGE)
	{
		druid->last_page = ERROR_PAGE;
	}
	else if (current_page == PROJECT_PAGE)
	{
		druid->last_page = current_page;
		druid->next_page = PROPERTY_PAGE;
	}
	else if (current_page == FINISH_PAGE)
	{
		npw_druid_set_busy (druid, FALSE);
	}
	else if (current_page >= PROPERTY_PAGE)
	{
		npw_druid_set_busy (druid, FALSE);
		druid->last_page = current_page;
		druid->next_page = current_page + 1;
	}
	
}

static void
on_druid_finish (GtkAssistant* assistant, NPWDruid* druid)
{
	NPWInstall* inst;

	inst = npw_install_new (druid->plugin);
	npw_install_set_property (inst, druid->values);
	npw_install_set_wizard_file (inst, npw_header_get_filename (druid->header));
	npw_install_launch (inst);
}

static GtkWidget*
npw_druid_create_assistant (NPWDruid* druid)
{
	AnjutaShell *shell;
	GtkBuilder *builder;
	GtkAssistant *assistant;
	GtkWidget *page;
	
	g_return_val_if_fail (druid->window == NULL, NULL);
	
	shell = ANJUTA_PLUGIN (druid->plugin)->shell;

	/* Create GtkAssistant using GtkBuilder, glade doesn't seem to work*/
	builder = gtk_builder_new ();
	if (!gtk_builder_add_from_file (builder, GTK_BUILDER_UI_FILE, NULL))
	{
		anjuta_util_dialog_error (GTK_WINDOW (shell),
								  _("Unable to build project assistant user interface reading %s."), GTK_BUILDER_UI_FILE);
		return NULL;
	}
	assistant = GTK_ASSISTANT (gtk_builder_get_object (builder, NEW_PROJECT_DIALOG));
	druid->window = GTK_WINDOW (assistant);
	druid->project_book = GTK_NOTEBOOK (gtk_builder_get_object (builder, PROJECT_BOOK));
	druid->error_icon = GTK_IMAGE (gtk_builder_get_object (builder, ERROR_ICON));
	druid->error_message = GTK_LABEL (gtk_builder_get_object (builder, ERROR_MESSAGE));
	druid->error_detail = GTK_WIDGET (gtk_builder_get_object (builder, ERROR_DETAIL));
	gtk_window_set_transient_for (GTK_WINDOW (assistant), GTK_WINDOW (shell));
	g_object_unref (builder);

	/* Connect assistant signals */
	gtk_assistant_set_forward_page_func (assistant, on_druid_next, druid, NULL);
	g_signal_connect (G_OBJECT (assistant), "prepare", G_CALLBACK (on_druid_prepare), druid);
	g_signal_connect (G_OBJECT (assistant), "apply", G_CALLBACK (on_druid_finish), druid);
	g_signal_connect (G_OBJECT (assistant), "cancel", G_CALLBACK (on_druid_cancel), druid);
	g_signal_connect (G_OBJECT (assistant), "close", G_CALLBACK (on_druid_close), druid);
	g_signal_connect(G_OBJECT(assistant), "key-press-event", G_CALLBACK(on_project_wizard_key_press_event), druid);

	/* Setup project selection page */
	if (!npw_druid_fill_selection_page (druid))
	{
		return NULL;
	}

	/* Use progress page to stop the flow */
	/*page = gtk_assistant_get_nth_page (assistant, PROGRESS_PAGE);
	gtk_assistant_set_page_complete (assistant, page, FALSE);*/
	
	/* Remove property page, will be created later as needed */
	page = gtk_assistant_get_nth_page (assistant, PROPERTY_PAGE);
	gtk_container_remove (GTK_CONTAINER (assistant), page);
	
	/* Add dialog widget to anjuta status. */
	anjuta_status_add_widget (anjuta_shell_get_status (shell, NULL), GTK_WIDGET (assistant));
	
	gtk_widget_show_all (GTK_WIDGET (assistant));
	
	return GTK_WIDGET(assistant);
}

/* Add default property
 *---------------------------------------------------------------------------*/

static void
npw_druid_add_default_property (NPWDruid* druid)
{
	NPWValue* value;
	gchar* s;
	/* gchar* email; */
	AnjutaPreferences* pref;

	pref = anjuta_shell_get_preferences (ANJUTA_PLUGIN (druid->plugin)->shell, NULL);

	/* Add default base project directory */
	value = npw_value_heap_find_value (druid->values, ANJUTA_PROJECT_DIRECTORY_PROPERTY);
	s = anjuta_preferences_get (pref, "anjuta.project.directory");
	npw_value_set_value (value, s == NULL ? "~" : s, NPW_VALID_VALUE);
	g_free (s);
	
	/* Add user name */
	value = npw_value_heap_find_value (druid->values, USER_NAME_PROPERTY);
	s = anjuta_preferences_get (pref, "anjuta.user.name");
	if (!s || strlen(s) == 0)
	{
		s = (gchar *)g_get_real_name();
		npw_value_set_value (value, s, NPW_VALID_VALUE);
	}
	else
	{
		npw_value_set_value (value, s, NPW_VALID_VALUE);
		g_free (s);
	}
	/* Add Email address */
	value = npw_value_heap_find_value (druid->values, EMAIL_ADDRESS_PROPERTY);
	s = anjuta_preferences_get (pref, "anjuta.user.email");
	/* If Email address not defined in Preferences */
	if (!s || strlen(s) == 0)
	{
		if (!(s = getenv("USERNAME")) || strlen(s) == 0)
			s = getenv("USER");
		s = g_strconcat(s, "@", getenv("HOSTNAME"), NULL);
	}
	npw_value_set_value (value, s, NPW_VALID_VALUE);
	g_free (s);
}

/* Druid public functions
 *---------------------------------------------------------------------------*/

NPWDruid*
npw_druid_new (NPWPlugin* plugin)
{
	NPWDruid* druid;

	/* Check if autogen is present */
	if (!npw_check_autogen())
	{
		anjuta_util_dialog_error (NULL, _("Could not find autogen version 5, please install the autogen package. You can get it from http://autogen.sourceforge.net"));
		return NULL;
	}	

	druid = g_new0(NPWDruid, 1);
	druid->plugin = plugin;
	druid->tooltips = NULL;
	druid->project_file = NULL;
	druid->busy = FALSE;
	druid->page_list = g_queue_new ();
	druid->values = npw_value_heap_new ();
	druid->gen = npw_autogen_new ();
	druid->plugin = plugin;
	
	if (npw_druid_create_assistant (druid) == NULL)
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
	/* NPWPage* page; */

	g_return_if_fail (druid != NULL);

	if (druid->tooltips)
	{
		g_object_unref (druid->tooltips);
		druid->tooltips = NULL;
	}
	
	/* Delete page list */
	druid->next_page = PROPERTY_PAGE;
	npw_druid_remove_following_page (druid);
	g_queue_free (druid->page_list);
	npw_value_heap_free (druid->values);
	npw_autogen_free (druid->gen);
	if (druid->parser != NULL) npw_page_parser_free (druid->parser);
	npw_header_list_free (druid->header_list);
	gtk_widget_destroy (GTK_WIDGET (druid->window));
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
