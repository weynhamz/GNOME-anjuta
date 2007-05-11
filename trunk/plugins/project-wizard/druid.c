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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#include <libanjuta/interfaces/ianjuta-wizard.h>

#include <gnome.h>
#include <libgnome/gnome-i18n.h>

#include <gtk/gtkactiongroup.h>

#include <string.h>

/*---------------------------------------------------------------------------*/

#define PROJECT_WIZARD_DIRECTORY PACKAGE_DATA_DIR"/project"
#define LOCAL_PROJECT_WIZARD_DIRECTORY "/.anjuta/project"
#define PIXMAP_APPWIZ_LOGO PACKAGE_DATA_DIR"/glade/applogo.png"
#define PIXMAP_APPWIZ_WATERMARK PACKAGE_DATA_DIR"/glade/appwizard.png"

/* Default property name useable in wizard file
 *---------------------------------------------------------------------------*/

#define ANJUTA_PROJECT_DIRECTORY_PROPERTY "AnjutaProjectDirectory"
#define USER_NAME_PROPERTY "UserName"
#define EMAIL_ADDRESS_PROPERTY "EmailAddress"

/* Widget and signal name found in glade file
 *---------------------------------------------------------------------------*/

#define NEW_PROJECT_DIALOG "druid_window"
#define PROJECT_SELECTION_LIST "project_list"
#define PROJECT_SELECTION_SCROLL_LIST "project_scroll_list"
#define PROJECT_SELECTION_FRAME "project_table"
#define PROJECT_SELECTION_BOOK "project_book"
#define PROJECT_SELECTION_BOOK_LABEL "project_book_label"
#define PROJECT_DESCRIPTION "project_description"
#define DRUID_WIDGET "druid"
#define DRUID_START_PAGE "start_page"
#define DRUID_SELECTION_PAGE "selection_page"
#define DRUID_PROPERTY_PAGE "property_page"
#define DRUID_PROPERTY_LABEL "property_label"
#define DRUID_PROPERTY_TABLE "property_table"
#define DRUID_FINISH_PAGE "finish_page"

#define DRUID_DELETE_SIGNAL "on_druid_delete"
#define DRUID_CANCEL_SIGNAL "on_druid_cancel"
#define DRUID_BACK_SIGNAL "on_druid_back"
#define DRUID_NEXT_SIGNAL "on_druid_next"
#define DRUID_FINISH_SIGNAL "on_druid_finish"
#define DRUID_PROJECT_SELECT_ICON_SIGNAL "on_druid_project_select_icon"
#define DRUID_PROJECT_SELECT_PAGE_SIGNAL "on_druid_project_map_page"

/*---------------------------------------------------------------------------*/

struct _NPWDruid
{
	GtkWidget* dialog;
	GtkNotebook* project_book;
	GMemChunk* pool;
	GnomeDruid* druid;
	const gchar* project_file;
	GnomeDruidPage* selection_page;
	GnomeDruidPageStandard* property_page;
	GtkLabel* property_label;
	GtkTable* property_table;
	GnomeDruidPage* finish_page;
	GtkTooltips *tooltips;
	NPWPlugin* plugin;
	
	guint page;
	GQueue* page_list;
	NPWValueHeap* values;
	NPWPageParser* parser;
	NPWHeaderList* header_list;
	NPWHeader* header;
	NPWAutogen* gen;
	gboolean busy;
};

typedef struct _NPWDruidAndTextBuffer
{
	NPWDruid* druid;
	GtkLabel* description;
	NPWHeader* header;
} NPWDruidAndTextBuffer;

typedef union _NPWDruidStuff
{
	NPWDruidAndTextBuffer a;
} NPWDruidStuff;


/* Create start and finish pages
 *---------------------------------------------------------------------------*/

/* libGlade doesn't seem to set these parameters for the start and end page */
static void
npw_druid_fix_edge_pages (NPWDruid* this, GladeXML* xml)
{
	GdkColor bg_color = {0, 15616, 33280, 46848};
	GtkWidget* page;
	GdkPixbuf* pixbuf;

	/* Start page */
	page = glade_xml_get_widget (xml, DRUID_START_PAGE);
	gnome_druid_page_edge_set_bg_color (GNOME_DRUID_PAGE_EDGE (page), &bg_color);
	gnome_druid_page_edge_set_logo_bg_color (GNOME_DRUID_PAGE_EDGE (page), &bg_color);
	pixbuf = gdk_pixbuf_new_from_file (PIXMAP_APPWIZ_WATERMARK, NULL);
	gnome_druid_page_edge_set_watermark (GNOME_DRUID_PAGE_EDGE (page), pixbuf);
	g_object_unref (pixbuf);
	pixbuf = gdk_pixbuf_new_from_file (PIXMAP_APPWIZ_LOGO, NULL);
	gnome_druid_page_edge_set_logo (GNOME_DRUID_PAGE_EDGE (page), pixbuf);

	/* Finish page */
	page = glade_xml_get_widget (xml, DRUID_FINISH_PAGE);
	gnome_druid_page_edge_set_bg_color (GNOME_DRUID_PAGE_EDGE (page), &bg_color);
	gnome_druid_page_edge_set_logo_bg_color (GNOME_DRUID_PAGE_EDGE (page), &bg_color);
	gnome_druid_page_edge_set_logo (GNOME_DRUID_PAGE_EDGE (page), pixbuf);
	g_object_unref (pixbuf);
}

static void
cb_druid_add_summary_property (NPWProperty* property, gpointer user_data)
{
	GString* text = (GString*)user_data;

	if (npw_property_get_options (property) & NPW_SUMMARY_OPTION)
	{
		g_string_append (text, _(npw_property_get_label (property)));
		g_string_append (text, npw_property_get_value (property));
		g_string_append (text, "\n");
	}
}

/* Fill last page (summary) */
static void
npw_druid_fill_summary (NPWDruid* this)
{
	NPWPage* page;
	guint i;
	GString* text;

	text = g_string_new (_("Confim the following information:\n\n"));
	
	g_string_append (text,_("Project Type: "));
	g_string_append (text, _(npw_header_get_name (this->header)));
	g_string_append (text,"\n");

	for (i = 0; (page = g_queue_peek_nth (this->page_list, i)) != NULL; ++i)
	{
		npw_page_foreach_property (page, cb_druid_add_summary_property, text);
	}

	gnome_druid_page_edge_set_text (GNOME_DRUID_PAGE_EDGE (this->finish_page), text->str);
	g_string_free (text, TRUE);
}


/* Create project selection page
 *---------------------------------------------------------------------------*/

/* Display project information */
static void
on_druid_project_select_icon (GnomeIconList* iconlist, gint idx, GdkEvent* event, gpointer user_data)
{
	NPWHeader* header;
	NPWDruidAndTextBuffer* data = (NPWDruidAndTextBuffer *)user_data;

	header = (NPWHeader*)gnome_icon_list_get_icon_data (iconlist, idx);
	data->header = header;

	/* As the icon list is in Browse mode, this function is called after inserting the 
	 * first item but before setting the associated data, so header could be NULL */
	if (header != NULL)
	{
		gtk_label_set_text (data->description, _(npw_header_get_description (header)));
		data->druid->header = header;
	}
}

/* Reselect default icon in page */
static void
on_druid_project_select_page (GtkWidget* widget, gpointer user_data)
{
	/* NPWHeader* header; */
	NPWDruidAndTextBuffer* data = (NPWDruidAndTextBuffer *)user_data;

	g_return_if_fail (data->header != NULL);

	gtk_label_set_text (data->description, _(npw_header_get_description (data->header)));
	data->druid->header = data->header;
}

/* Add a project in the icon list */
static void
cb_druid_insert_project_icon (NPWHeader* header, gpointer user_data)
{
	gint idx;
	GnomeIconList* list = GNOME_ICON_LIST (user_data);

	idx = gnome_icon_list_append (list, npw_header_get_iconfile (header), _(npw_header_get_name (header)));
	gnome_icon_list_set_icon_data (list, idx, header);
}

/* Add a page in the project notebook */
static void
cb_druid_insert_project_page (NPWHeader* header, gpointer user_data)
{
	NPWDruid* this = (NPWDruid*)user_data;
	GladeXML* xml;
	GtkWidget* label;
	GtkWidget* frame;
	GnomeIconList* list;
	GtkLabel* desc;
	const gchar* category;
	NPWDruidAndTextBuffer* data;

	category = npw_header_get_category(header);
		
	/* Create new frame according to glade file */
	xml = glade_xml_new (GLADE_FILE, PROJECT_SELECTION_FRAME, NULL);
	g_return_if_fail (xml != NULL);

	frame = glade_xml_get_widget (xml, PROJECT_SELECTION_FRAME);
	list = GNOME_ICON_LIST (glade_xml_get_widget (xml, PROJECT_SELECTION_LIST));
	desc = GTK_LABEL (glade_xml_get_widget (xml, PROJECT_DESCRIPTION));

	npw_header_list_foreach_project_in (this->header_list, category, cb_druid_insert_project_icon, list);

	/* put druid address and corresponding text buffer in one structure 
	 * to pass them to a call back function as an unique pointer */
	data = g_chunk_new (NPWDruidAndTextBuffer, this->pool);
	data->druid = this;
	data->description = desc;
	data->header = (NPWHeader*)gnome_icon_list_get_icon_data (list, 0);
	glade_xml_signal_connect_data (xml, DRUID_PROJECT_SELECT_ICON_SIGNAL, GTK_SIGNAL_FUNC (on_druid_project_select_icon), data);
	glade_xml_signal_connect_data (xml, DRUID_PROJECT_SELECT_PAGE_SIGNAL, GTK_SIGNAL_FUNC (on_druid_project_select_page), data);

	g_object_unref (xml);

	/* Create label */
	xml = glade_xml_new (GLADE_FILE, PROJECT_SELECTION_BOOK_LABEL, NULL);
	g_return_if_fail (xml != NULL);
	label = glade_xml_get_widget (xml, PROJECT_SELECTION_BOOK_LABEL);
	g_object_unref (xml);
	gtk_label_set_text (GTK_LABEL(label), category);

	gtk_notebook_append_page (this->project_book, frame, label);
}

/* Fill project selection page */
static gboolean
npw_druid_fill_selection_page (NPWDruid* this)
{
	gboolean ok;
	gchar* local_dir;

	/* Create list of projects */
	if (this->header_list != NULL) npw_header_list_free (this->header_list);
	this->header_list = npw_header_list_new ();	

	/* Fill list with all project in directory */
	ok = npw_header_list_readdir (this->header_list, PROJECT_WIZARD_DIRECTORY);
	local_dir = g_build_filename (g_get_home_dir(), LOCAL_PROJECT_WIZARD_DIRECTORY, NULL);
	ok = npw_header_list_readdir (this->header_list, local_dir) || ok;
	g_free (local_dir);
	if (!ok)
	{
		anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (this->plugin)->shell),_("Unable to find any project template in %s"), PROJECT_WIZARD_DIRECTORY);		
		return FALSE;
	}

	/* Add all notebook page */
	gtk_notebook_remove_page(this->project_book, 0);	
	npw_header_list_foreach_category (this->header_list, cb_druid_insert_project_page, this);

	return TRUE;
}

/* Create properties pages
 *---------------------------------------------------------------------------*/

/* Group infomation need by the following call back function */
typedef struct _NPWDruidAddPropertyData
{
	NPWDruid* druid;
	guint row;
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
		gtk_table_resize (data->druid->property_table, data->row + 1, 2);
		label = gtk_label_new (_(npw_property_get_label (property)));
		gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
		gtk_misc_set_padding (GTK_MISC (label), 5, 5);
		gtk_table_attach (data->druid->property_table, label, 0, 1, data->row, data->row + 1,
			(GtkAttachOptions)(GTK_FILL), (GtkAttachOptions)(0), 0, 0);
		gtk_table_attach (data->druid->property_table, entry, 1, 2, data->row, data->row + 1,
			(GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(0), 0, 0);

		data->row++;
	}
};

static gboolean
npw_druid_fill_property_page (NPWDruid* this, NPWPage* page)
{
	NPWDruidAddPropertyData data;
	PangoAttribute* attr;
	PangoAttrList* attr_list;
	
	/* Remove previous widgets */
	gtk_container_foreach (GTK_CONTAINER (this->property_table), cb_druid_destroy_widget, NULL);

	/* Update title	*/
	gnome_druid_page_standard_set_title (this->property_page, _(npw_page_get_label (page)));
	gtk_label_set_text (this->property_label, _(npw_page_get_description (page)));
	attr = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
	attr->start_index = 0;
	attr->end_index = G_MAXINT;
	attr_list = pango_attr_list_new ();
	pango_attr_list_insert (attr_list, attr);
	gtk_label_set_attributes (this->property_label, attr_list);
	pango_attr_list_unref (attr_list);
	
	/* Add new widget */
	data.druid = this;
	data.row = 0;
	npw_page_foreach_property (page, cb_druid_add_property, &data);

	/* Change page only if current is project selection page */
	gnome_druid_set_page (this->druid, GNOME_DRUID_PAGE (this->property_page));
	gtk_widget_show_all (this->dialog);

	return TRUE;
}

/* Handle page cache
 *---------------------------------------------------------------------------*/

static NPWPage*
npw_druid_add_new_page (NPWDruid* this)
{
	NPWPage* page;

	/* Get page in cache */
	page = g_queue_peek_nth (this->page_list, this->page);
	if (page == NULL)
	{
		/* Page not found in cache, create */
		page = npw_page_new (this->values);

		/* Add page in cache */
		g_queue_push_tail (this->page_list, page);
	}

	return page;
}

/* Remove all following page */

static void
npw_druid_remove_following_page (NPWDruid* this)
{
	NPWPage* page;

	for(;;)
	{
		page = g_queue_pop_nth (this->page_list, this->page);
		if (page == NULL) break;
		npw_page_free (page);
	}
}

/* Save properties
 *---------------------------------------------------------------------------*/

typedef struct _NPWSaveValidPropertyData
{
	GtkWindow *parent;
	gboolean next;
	gboolean modified;
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
			if (data->next == TRUE)
			{
				/* First error message */
				data->next = FALSE;

				/* Show error message. */
				anjuta_util_dialog_error (data->parent,
					_("Field \"%s\" is mandatory. Please enter it."),
					_(npw_property_get_label (property)));
			}
			npw_property_remove_value (property);
		}
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
			if (data->next == TRUE)
			{
				/* First error message */
				gboolean yes;

				yes = anjuta_util_dialog_boolean_question (data->parent, is_directory ? 
					_("Directory \"%s\" is not empty. Project creation could fail if some files "
					  "cannot be written. Do you want to continue?") :
					_("File \"%s\" already exists. Do you want to overwrite it ?"),
			       		value);

				if (!yes)
				{
					data->next = FALSE;
					npw_property_remove_value (property);
				}
			}
			else
			{
				/* value is not invalid */
				npw_property_remove_value (property);
			}
		}
	}
};

static void
cb_save_old_property (NPWProperty* property, gpointer user_data)
{
	if (npw_property_save_value_from_widget (property))
	{
		*(gboolean *)user_data = TRUE;
	}
};

/* Save values and check them, if it's possible to go to next page */

static gboolean
npw_druid_save_valid_values (NPWDruid* this)
{
	NPWPage* page;
	NPWSaveValidPropertyData data;

	page = g_queue_peek_nth (this->page_list, this->page - 1);
	data.modified = FALSE;
	data.next = TRUE;
	data.parent = GTK_WINDOW (this->dialog);
	npw_page_foreach_property (page, cb_save_valid_property, &data);

	if (data.modified) npw_druid_remove_following_page (this);

	return data.next;
}

/* Save current value, but do not check anything  */

static void
npw_druid_save_old_values (NPWDruid* this)
{
	NPWPage* page;
	gboolean modified;

	page = g_queue_peek_nth (this->page_list, this->page - 1);
	modified = FALSE;
	npw_page_foreach_property (page, cb_save_old_property, &modified);
}


/* Druid GUI functions
 *---------------------------------------------------------------------------*/

static void
npw_druid_set_busy (NPWDruid *this, gboolean busy_state)
{
	if (this->busy == busy_state)
		return;
	
	/* Set busy state */
	gnome_druid_set_buttons_sensitive (GNOME_DRUID (this->druid),
									   !busy_state, !busy_state,
									   !busy_state, TRUE);
	if (busy_state)
		anjuta_status_busy_push (anjuta_shell_get_status (ANJUTA_PLUGIN (this->plugin)->shell, NULL));
	else
		anjuta_status_busy_pop (anjuta_shell_get_status (ANJUTA_PLUGIN (this->plugin)->shell, NULL));
	this->busy = busy_state;
}

/* Druid call backs
 *---------------------------------------------------------------------------*/

static gboolean
on_druid_cancel (GtkWidget* window, NPWDruid* druid)
{
	DEBUG_PRINT ("Project wizard canceled");
	anjuta_plugin_deactivate (ANJUTA_PLUGIN (druid->plugin));
	npw_druid_free (druid);

	return TRUE;
}

static gboolean
on_druid_delete (GtkWidget* window, GdkEvent* event, NPWDruid* druid)
{
	DEBUG_PRINT ("Project wizard canceled");
	anjuta_plugin_deactivate (ANJUTA_PLUGIN (druid->plugin));
	npw_druid_free (druid);

	return TRUE;
}

static gboolean
on_project_wizard_key_press_event(GtkWidget *widget, GdkEventKey *event,
                               NPWDruid* druid)
{
	if (event->keyval == GDK_Escape)
	{
		npw_druid_free (druid);
		return TRUE;
	}
	return FALSE;
}

static void
on_druid_get_new_page (NPWAutogen* gen, gpointer data)
{
	NPWDruid* this = (NPWDruid *)data;
	NPWPage* page;

	page = g_queue_peek_nth (this->page_list, this->page - 1);

	if (npw_page_get_name (page) == NULL)
	{
		/* no page, display finish page */
		npw_druid_fill_summary (this);
		gnome_druid_set_page (this->druid, this->finish_page);
	}
	else
	{
		/* display property page */
		npw_druid_fill_property_page (this, page);
	}
	npw_druid_set_busy (this, FALSE);
}

static void
on_druid_parse_page (const gchar* output, gpointer data)
{
	NPWPageParser* parser = (NPWPageParser*)data;
	
	npw_page_parser_parse (parser, output, strlen (output), NULL);
}

static gboolean
on_druid_next (GnomeDruidPage* page, GtkWidget* widget, NPWDruid* this)
{
	/* Skip if busy */
	if (this->busy) return TRUE;
		
	/* Set busy */
	npw_druid_set_busy (this, TRUE);

	if (this->page == 0)
	{
		const gchar* new_project;

		/* Current is Select project page */
		new_project = npw_header_get_filename (this->header);
		if (this->project_file != new_project)
		{
			/* Change project */

			this->project_file = new_project;
			npw_druid_remove_following_page (this);
			npw_autogen_set_input_file (this->gen, this->project_file, "[+","+]");
		}
	}
	else
	{
		/* Current is one of the property page */
		if (!npw_druid_save_valid_values (this))
		{
			/* Error stay on this page */
			npw_druid_set_busy (this, FALSE);

			return TRUE;
		}
	}
	this->page++;

	if (g_queue_peek_nth (this->page_list, this->page - 1) == NULL)
	{
		/* Regenerate new page */
		if (this->parser != NULL)
			npw_page_parser_free (this->parser);
		this->parser = npw_page_parser_new (npw_druid_add_new_page (this), this->project_file, this->page - 1);
		npw_autogen_set_output_callback (this->gen, on_druid_parse_page, this->parser);
		npw_autogen_write_definition_file (this->gen, this->values);
		npw_autogen_execute (this->gen, on_druid_get_new_page, this, NULL);
	}
	else
	{
		/* Page is already in cache */
		on_druid_get_new_page (NULL, (gpointer)this);
	}

	return TRUE;
}

static gboolean
on_druid_back (GnomeDruidPage* dpage, GtkWidget* widget, NPWDruid* druid)
{
	/* Skip if busy */
	if (druid->busy) return TRUE;

	g_return_val_if_fail (druid->page > 0, TRUE);

	npw_druid_save_old_values (druid);

	druid->page--;
	if (druid->page == 0)
	{
		/* Go back to project selection page */
		gnome_druid_set_page (druid->druid, druid->selection_page);
	}
	else
	{
		NPWPage* page;

		page = g_queue_peek_nth (druid->page_list, druid->page - 1);
		/* Create property page */
		npw_druid_fill_property_page (druid, page);
	}

	return TRUE;
}

static void
on_druid_finish (GnomeDruidPage* page, GtkWidget* widget, NPWDruid* druid)
{
	NPWInstall* inst;

	inst = npw_install_new (druid->plugin);
	npw_install_set_property (inst, druid->values);
	npw_install_set_wizard_file (inst, npw_header_get_filename (druid->header));
	npw_install_launch (inst);

	npw_druid_free (druid);
}

static void
npw_druid_connect_all_signal (NPWDruid* this, GladeXML* xml)
{
	glade_xml_signal_connect_data (xml, DRUID_DELETE_SIGNAL, GTK_SIGNAL_FUNC (on_druid_delete), this);
	glade_xml_signal_connect_data (xml, DRUID_CANCEL_SIGNAL, GTK_SIGNAL_FUNC (on_druid_cancel), this);
	glade_xml_signal_connect_data (xml, DRUID_FINISH_SIGNAL, GTK_SIGNAL_FUNC (on_druid_finish), this);
	glade_xml_signal_connect_data (xml, DRUID_NEXT_SIGNAL, GTK_SIGNAL_FUNC (on_druid_next), this);
	glade_xml_signal_connect_data (xml, DRUID_BACK_SIGNAL, GTK_SIGNAL_FUNC (on_druid_back), this);
}

/* Add default property
 *---------------------------------------------------------------------------*/

static void
npw_druid_add_default_property (NPWDruid* this)
{
	NPWValue* value;
	gchar* s;
	/* gchar* email; */
	AnjutaPreferences* pref;

	pref = anjuta_shell_get_preferences (ANJUTA_PLUGIN (this->plugin)->shell, NULL);

	/* Add default base project directory */
	value = npw_value_heap_find_value (this->values, ANJUTA_PROJECT_DIRECTORY_PROPERTY);
	s = anjuta_preferences_get (pref, "anjuta.project.directory");
	npw_value_heap_set_value (this->values, value, s, NPW_VALID_VALUE);
	g_free (s);
	
	/* Add user name */
	value = npw_value_heap_find_value (this->values, USER_NAME_PROPERTY);
	s = anjuta_preferences_get (pref, "anjuta.user.name");
	if (!s || strlen(s) == 0)
	{
		s = (gchar *)g_get_real_name();
		npw_value_heap_set_value (this->values, value, s, NPW_VALID_VALUE);
	}
	else
	{
		npw_value_heap_set_value (this->values, value, s, NPW_VALID_VALUE);
		g_free (s);
	}
	/* Add Email address */
	value = npw_value_heap_find_value (this->values, EMAIL_ADDRESS_PROPERTY);
	s = anjuta_preferences_get (pref, "anjuta.user.email");
	/* If Email address not defined in Preferences */
	if (!s || strlen(s) == 0)
	{
		if (!(s = getenv("USERNAME")) || strlen(s) == 0)
			s = getenv("USER");
		s = g_strconcat(s, "@", getenv("HOSTNAME"), NULL);
	}
	npw_value_heap_set_value (this->values, value, s, NPW_VALID_VALUE);
	g_free (s);
}

/* Druid public functions
 *---------------------------------------------------------------------------*/

NPWDruid*
npw_druid_new (NPWPlugin* plugin)
{
	GladeXML* xml;
	/* GtkContainer* project_scroll; */
	NPWDruid* this;

	/* Skip if already created */
	if (plugin->druid != NULL) return plugin->druid;

	/* Check if autogen is present */
	if (!npw_check_autogen())
	{
		anjuta_util_dialog_error (NULL, _("Could not find autogen version 5, please install the autogen package. You can get it from http://autogen.sourceforge.net"));
		return NULL;
	}	

	this = g_new0(NPWDruid, 1);
	xml = glade_xml_new (GLADE_FILE, NEW_PROJECT_DIALOG, NULL);
	if ((this == NULL) || (xml == NULL))
	{
		anjuta_util_dialog_error (NULL, _("Unable to build project wizard user interface"));
		g_free (this);

		return NULL;
	}
	this->pool = g_mem_chunk_new ("druid pool", sizeof(NPWDruidStuff), sizeof(NPWDruidStuff) * 20, G_ALLOC_ONLY);
	
	/* Get reference on all useful widget */
	this->dialog = glade_xml_get_widget (xml, NEW_PROJECT_DIALOG);
	gtk_window_set_transient_for (GTK_WINDOW (this->dialog),
								  GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell));
	this->tooltips = NULL;
	/* add GtkIconView in the program as it's not handled by libglade */
	this->druid = GNOME_DRUID (glade_xml_get_widget (xml, DRUID_WIDGET));
	this->selection_page = GNOME_DRUID_PAGE (glade_xml_get_widget (xml, DRUID_SELECTION_PAGE));
	this->project_book = GTK_NOTEBOOK (glade_xml_get_widget (xml, PROJECT_SELECTION_BOOK));
	this->property_page = GNOME_DRUID_PAGE_STANDARD (glade_xml_get_widget (xml, DRUID_PROPERTY_PAGE));
	this->property_label = GTK_LABEL (glade_xml_get_widget (xml, DRUID_PROPERTY_LABEL));
	this->property_table = GTK_TABLE (glade_xml_get_widget (xml, DRUID_PROPERTY_TABLE));
	this->finish_page = GNOME_DRUID_PAGE (glade_xml_get_widget (xml, DRUID_FINISH_PAGE));
	this->page = 0;
	this->project_file = NULL;
	this->busy = FALSE;
	this->page_list = g_queue_new ();
	this->values = npw_value_heap_new ();
	this->gen = npw_autogen_new ();
	this->plugin = plugin;

	npw_druid_fix_edge_pages (this, xml);
	npw_druid_connect_all_signal (this, xml);
	g_object_unref (xml);

	if (!npw_druid_fill_selection_page (this))
	{
		/* No project available */
		npw_druid_free (this);

		return NULL;
	}

	/* Add dialog widget to anjuta status. */
	anjuta_status_add_widget (anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell, NULL), this->dialog);

	/* Needed by GnomeDruid widget */
	gtk_widget_show_all (this->dialog);

	g_signal_connect(G_OBJECT(this->dialog), "key-press-event",
			G_CALLBACK(on_project_wizard_key_press_event), this);
	
	plugin->druid = this;
	npw_druid_add_default_property (this);

	return this;
}

void
npw_druid_free (NPWDruid* this)
{
	/* NPWPage* page; */

	g_return_if_fail (this != NULL);

	if (this->tooltips)
	{
		g_object_unref (this->tooltips);
		this->tooltips = NULL;
	}
	/* Delete page list */
	this->page = 0;
	npw_druid_remove_following_page (this);
	g_queue_free (this->page_list);
	npw_value_heap_free (this->values);
	npw_autogen_free (this->gen);
	if (this->parser != NULL) npw_page_parser_free (this->parser);
	g_mem_chunk_destroy(this->pool);
	npw_header_list_free (this->header_list);
	gtk_widget_destroy (this->dialog);
	this->plugin->druid = NULL;
	g_free (this);
}

void
npw_druid_show (NPWDruid* this)
{
	g_return_if_fail (this);

	/* Display dialog box */
	if (this->dialog) gtk_window_present (GTK_WINDOW (this->dialog));
}
