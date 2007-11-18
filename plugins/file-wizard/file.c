/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */

/***************************************************************************
 *            file.c
 *
 *  Sun Nov 30 17:46:54 2003
 *  Copyright  2003  Jean-Noel Guiheneuf
 *  jnoel@lotuscompounds.com
 ****************************************************************************/

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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include <time.h>

#include <gnome.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-macro.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>

#include "plugin.h"
#include "file.h"

#define GLADE_FILE_FILE PACKAGE_DATA_DIR"/glade/anjuta-file-wizard.glade"
#define NEW_FILE_DIALOG "dialog.new.file"
#define NEW_FILE_ENTRY "new.file.entry"
#define NEW_FILE_TYPE "new.file.type"
#define NEW_FILE_TEMPLATE "new.file.template"
#define NEW_FILE_HEADER "new.file.header"
#define NEW_FILE_LICENSE "new.file.license"
#define NEW_FILE_MENU_LICENSE "new.file.menu.license"
#define NEW_FILE_ADD_TO_PROJECT "add_to_project"
#define NEW_FILE_ADD_TO_REPOSITORY "add_to_repository"

typedef struct _NewFileGUI
{
	GladeXML *xml;
	GtkWidget *dialog;
	GtkWidget *add_to_project;
	GtkWidget *add_to_repository;
	gboolean showing;
	AnjutaFileWizardPlugin *plugin;
} NewFileGUI;


typedef struct _NewfileType
{
	gchar *name;
	gchar *ext;
	gboolean header;
	gboolean gpl;
	gboolean template;
	Cmt comment;
	Lge type;
} NewfileType;



NewfileType new_file_type[] = {
	{N_("C Source File"), ".c", TRUE, TRUE, FALSE, CMT_C, LGE_C},
	{N_("C/C++ Header File"), ".h", TRUE, TRUE, TRUE, CMT_C, LGE_HC},
	{N_("C++ Source File"), ".cxx", TRUE, TRUE, FALSE, CMT_CPP, LGE_CPLUS},
	{N_("C# Source File"), ".c#", TRUE, FALSE, FALSE, CMT_CPP, LGE_CSHARP},
	{N_("Java Source File"), ".java", TRUE, TRUE, FALSE, CMT_CPP, LGE_JAVA},
	{N_("Perl Source File"), ".pl", TRUE, TRUE, FALSE, CMT_P, LGE_PERL},
	{N_("Python Source File"), ".py", FALSE, TRUE, FALSE, CMT_P, LGE_PYTHON},
	{N_("Shell Script File"), ".sh", TRUE, TRUE, FALSE, CMT_P, LGE_SHELL},
	{N_("Other"), NULL, FALSE, FALSE, FALSE, CMT_C, LGE_C}
};


typedef enum _Lsc
{
	GPL,
	LGPL
} Lsc;

typedef struct _NewlicenseType
{
	gchar *name;
	Lsc type;
} NewlicenseType;

NewlicenseType new_license_type[] = {
	{N_("General Public License (GPL)"), GPL},
	{N_("Lesser General Public License (LGPL)"), LGPL}
};

static NewFileGUI *nfg = NULL;


static gboolean create_new_file_dialog(IAnjutaDocumentManager *docman);
static void insert_notice_gpl (IAnjutaMacro* macro, gint comment_type);
static void insert_notice_lgpl (IAnjutaMacro* macro, gint comment_type);
static void insert_notice(IAnjutaMacro* macro, gint license_type, gint comment_type);
static void insert_header(IAnjutaMacro* macro, gint source_type);

void
display_new_file(AnjutaFileWizardPlugin *plugin,
				 IAnjutaDocumentManager *docman)
{
	IAnjutaProjectManagerCapabilities caps =
		IANJUTA_PROJECT_MANAGER_CAN_ADD_NONE;
	
	if (!nfg)
		if (!create_new_file_dialog (docman))
			return;
	
	nfg->plugin = plugin;
	
	/* check whether we have a loaded project or not */
	if (plugin->top_dir) {
		IAnjutaProjectManager *manager =
			anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
										IAnjutaProjectManager, NULL);
       if (manager)
			caps = ianjuta_project_manager_get_capabilities (manager, NULL);
	}
	
	if ((caps & IANJUTA_PROJECT_MANAGER_CAN_ADD_SOURCE) == FALSE) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (nfg->add_to_project),
									  FALSE);
		gtk_widget_set_sensitive (nfg->add_to_project, FALSE);
	}
	else
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (nfg->add_to_project),
									  TRUE);
		gtk_widget_set_sensitive (nfg->add_to_project, TRUE);
	}
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (nfg->add_to_repository),
								  FALSE);
	/* FIXME: fix the problem with the repository add, then enable this check-button */
	gtk_widget_set_sensitive (nfg->add_to_repository, FALSE);
	
	if (nfg && !(nfg->showing))
	{
		gtk_window_present (GTK_WINDOW (nfg->dialog));
		nfg->showing = TRUE;
	}
}

static gboolean
create_new_file_dialog(IAnjutaDocumentManager *docman)
{
	GtkWidget *optionmenu;
	GtkWidget *menu;
	GtkWidget *menuitem;
	gint i;

	nfg = g_new0(NewFileGUI, 1);
	if (NULL == (nfg->xml = glade_xml_new(GLADE_FILE_FILE, NEW_FILE_DIALOG, NULL)))	
	{
		anjuta_util_dialog_error(NULL, _("Unable to build user interface for New File"));
		g_free(nfg);
		nfg = NULL;
		return FALSE;
	}
	nfg->dialog = glade_xml_get_widget(nfg->xml, NEW_FILE_DIALOG);
	nfg->add_to_project = glade_xml_get_widget (nfg->xml, NEW_FILE_ADD_TO_PROJECT);
	nfg->add_to_repository = glade_xml_get_widget (nfg->xml, NEW_FILE_ADD_TO_REPOSITORY);
	nfg->showing = FALSE;
	
	optionmenu = glade_xml_get_widget(nfg->xml, NEW_FILE_TYPE);
	menu = gtk_menu_new();
	for (i=0; i < (sizeof(new_file_type) / sizeof(NewfileType)); i++)
	{
		menuitem = gtk_menu_item_new_with_label(new_file_type[i].name);
		gtk_menu_append(GTK_MENU(menu), menuitem);
		gtk_widget_show(menuitem);
	}
	gtk_option_menu_set_menu(GTK_OPTION_MENU(optionmenu), menu);
	
	optionmenu = glade_xml_get_widget(nfg->xml, NEW_FILE_MENU_LICENSE);
	menu = gtk_menu_new();
	for (i=0; i < (sizeof(new_license_type) / sizeof(NewlicenseType)); i++)
	{
		menuitem = gtk_menu_item_new_with_label(new_license_type[i].name);
		gtk_menu_append(GTK_MENU(menu), menuitem);
		gtk_widget_show(menuitem);
	}
	gtk_option_menu_set_menu(GTK_OPTION_MENU(optionmenu), menu);
	
	g_object_set_data (G_OBJECT (nfg->dialog), "IAnjutaDocumentManager", docman);
	glade_xml_signal_autoconnect(nfg->xml);
	gtk_signal_emit_by_name(GTK_OBJECT (optionmenu), "changed");
	
	return TRUE;
}

gboolean
on_new_file_cancelbutton_clicked(GtkWidget *window, GdkEvent *event,
			                     gboolean user_data)
{
	if (nfg->showing)
	{
		gtk_widget_hide(nfg->dialog);
		nfg->showing = FALSE;
	}
	return TRUE;
}

static gboolean
confirm_file_overwrite (AnjutaPlugin* plugin, const gchar *uri)
{
	GnomeVFSURI *vfs_uri;
	gboolean ret = TRUE;
	
	vfs_uri = gnome_vfs_uri_new (uri);
	if (gnome_vfs_uri_exists (vfs_uri))
	{
		GtkWidget *dialog;
		gint res;
		dialog = gtk_message_dialog_new (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
										 GTK_DIALOG_DESTROY_WITH_PARENT,
										 GTK_MESSAGE_QUESTION,
										 GTK_BUTTONS_NONE,
										 _("The file '%s' already exists.\n"
										   "Do you want to replace it with the "
										   "one you are saving?"),
										 uri);
		gtk_dialog_add_button (GTK_DIALOG(dialog),
							   GTK_STOCK_CANCEL,
							   GTK_RESPONSE_CANCEL);
		anjuta_util_dialog_add_button (GTK_DIALOG (dialog),
								  _("_Replace"),
								  GTK_STOCK_REFRESH,
								  GTK_RESPONSE_YES);
		res = gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		if (res != GTK_RESPONSE_YES)
			ret = FALSE;
	}
	gnome_vfs_uri_unref (vfs_uri);
	return ret;
}

gboolean
on_new_file_okbutton_clicked(GtkWidget *window, GdkEvent *event,
			                 gboolean user_data)
{
	GtkWidget *entry;
	GtkWidget *checkbutton;
	GtkWidget *optionmenu;
	const gchar *name;
	gint sel;
	gint license_type;
	gint comment_type;
	gint source_type;
	IAnjutaDocumentManager *docman;
	GtkWidget *toplevel;
	IAnjutaMacro* macro;
	IAnjutaEditor *te = NULL;
	
	toplevel= gtk_widget_get_toplevel (window);
	docman = IANJUTA_DOCUMENT_MANAGER (g_object_get_data (G_OBJECT(toplevel),
										"IAnjutaDocumentManager"));
	macro = anjuta_shell_get_interface (ANJUTA_PLUGIN(docman)->shell, 
		                       IAnjutaMacro, NULL);
	entry = glade_xml_get_widget(nfg->xml, NEW_FILE_ENTRY);
	name = gtk_entry_get_text(GTK_ENTRY(entry));
	
	if (nfg->plugin->top_dir &&
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (nfg->add_to_project)))
	{
		IAnjutaProjectManager *pm;
		GnomeVFSHandle *vfs_write;
		gchar* file_uri;
		
		pm = anjuta_shell_get_interface (ANJUTA_PLUGIN(docman)->shell, 
										 IAnjutaProjectManager, NULL);
		g_return_val_if_fail (pm != NULL, FALSE);
		
		file_uri = ianjuta_project_manager_add_source (pm, name, "", NULL);
		if (!file_uri)
			return FALSE;
		
		/* Create empty file */
		if (!confirm_file_overwrite (ANJUTA_PLUGIN (nfg->plugin), file_uri) ||
			gnome_vfs_create (&vfs_write, file_uri, GNOME_VFS_OPEN_WRITE,
							  FALSE, 0664) != GNOME_VFS_OK ||
			gnome_vfs_close(vfs_write) != GNOME_VFS_OK)
		{
			g_free (file_uri);
			return FALSE;
		}
		ianjuta_file_open (IANJUTA_FILE (docman), file_uri, NULL);
		g_free (file_uri);
	}
	else
	{
		if (name && strlen (name) > 0)
			te = ianjuta_document_manager_add_buffer (docman, name, "", NULL);
		else
			te = ianjuta_document_manager_add_buffer (docman, "", "", NULL);
		if (te == NULL)
			return FALSE;
	}
	
	optionmenu = glade_xml_get_widget(nfg->xml, NEW_FILE_TYPE);
	source_type = gtk_option_menu_get_history(GTK_OPTION_MENU(optionmenu));
	
	checkbutton = glade_xml_get_widget(nfg->xml, NEW_FILE_HEADER);
	if (GTK_WIDGET_SENSITIVE(checkbutton) && 
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton)))
	{
		insert_header(macro, source_type);
	}
		
	checkbutton = glade_xml_get_widget(nfg->xml, NEW_FILE_LICENSE);
	if (GTK_WIDGET_SENSITIVE(checkbutton) && 
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton)))
	{
		optionmenu = glade_xml_get_widget(nfg->xml, NEW_FILE_MENU_LICENSE);
		sel = gtk_option_menu_get_history(GTK_OPTION_MENU(optionmenu));
		license_type = new_license_type[sel].type;
		comment_type = new_file_type[source_type].comment;
		                                  
		insert_notice(macro, license_type, comment_type);		
	}
	
	checkbutton = glade_xml_get_widget(nfg->xml, NEW_FILE_TEMPLATE);
	if (GTK_WIDGET_SENSITIVE(checkbutton) && 
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton)))
	{
		ianjuta_macro_insert(macro, "Header_h", NULL);
	}
	
	gtk_widget_hide (nfg->dialog);
	nfg->showing = FALSE;
	
	return TRUE;
}

void
on_new_file_entry_changed (GtkEditable *entry, gpointer user_data)
{
	char *name;
	gint sel;
	static gint last_length = 0;
	gint length;
	GtkWidget *optionmenu;
	
	name = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
	length = strlen(name);
	
	if (last_length != 2 && length == 1)
	{
		optionmenu = glade_xml_get_widget(nfg->xml, NEW_FILE_TYPE);
		sel = gtk_option_menu_get_history(GTK_OPTION_MENU(optionmenu));
		name = g_strconcat (name, new_file_type[sel].ext, NULL);
		gtk_entry_set_text (GTK_ENTRY(entry), name);
	}
	last_length = length;
	
	g_free(name);
}

void
on_new_file_type_changed (GtkOptionMenu   *optionmenu, gpointer user_data)
{
	gint sel;
	char *name, *tmp;
	GtkWidget *widget;
	GtkWidget *entry;
	
	sel = gtk_option_menu_get_history(optionmenu);
	
	widget = glade_xml_get_widget(nfg->xml, NEW_FILE_HEADER);
	gtk_widget_set_sensitive(widget, new_file_type[sel].header);
	widget = glade_xml_get_widget(nfg->xml, NEW_FILE_LICENSE);
	gtk_widget_set_sensitive(widget, new_file_type[sel].gpl);
	widget = glade_xml_get_widget(nfg->xml, NEW_FILE_TEMPLATE);
	gtk_widget_set_sensitive(widget, new_file_type[sel].template);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
	
	entry = glade_xml_get_widget(nfg->xml, NEW_FILE_ENTRY);
	name = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
	if (strlen(name) > 0)
	{
		tmp = strrchr(name, '.');
		if (tmp)
			name = g_strndup(name, tmp - name);
		name =  g_strconcat (name, new_file_type[sel].ext, NULL);
		gtk_entry_set_text (GTK_ENTRY(entry), name);
	}
	g_free(name);
}

void
on_new_file_license_toggled(GtkToggleButton *button, gpointer user_data)
{
	GtkWidget *widget;
	
	widget = glade_xml_get_widget(nfg->xml, NEW_FILE_MENU_LICENSE);
	gtk_widget_set_sensitive(widget, gtk_toggle_button_get_active(button));
}
 
static void
insert_notice_gpl (IAnjutaMacro* macro, gint comment_type)
{
	switch (comment_type)
	{
		case CMT_C:
			ianjuta_macro_insert(macro, "/* GPL */", NULL);
			break;
		case CMT_CPP:
			ianjuta_macro_insert(macro, "// GPL", NULL);;
			break;
		case CMT_P:
			ianjuta_macro_insert(macro, "# GPL", NULL);;
			break;
		default:
			ianjuta_macro_insert(macro, "/* GPL */", NULL);;
			break;
	}
}

static void
insert_notice_lgpl (IAnjutaMacro* macro, gint comment_type)
{
	switch (comment_type)
	{
		case CMT_C:
			ianjuta_macro_insert(macro, "/* LGPL */", NULL);
			break;
		case CMT_CPP:
			ianjuta_macro_insert(macro, "// LGPL", NULL);;
			break;
		case CMT_P:
			ianjuta_macro_insert(macro, "# LGPL", NULL);;
			break;
		default:
			ianjuta_macro_insert(macro, "/* LGPL */", NULL);;
			break;
	}
}
static void
insert_notice(IAnjutaMacro* macro, gint license_type, gint comment_type)
{
	switch (license_type)
	{
		case GPL:
			insert_notice_gpl(macro, comment_type);
			break;
		case LGPL :
			insert_notice_lgpl(macro, comment_type);
			break;
		default:
			;
			break;
	}	
}

static void
insert_header(IAnjutaMacro* macro, gint source_type)
{
	switch (source_type)
	{
		case  LGE_C: case LGE_HC:
			ianjuta_macro_insert(macro, "Header_c", NULL);
			break;
		case  LGE_CPLUS: case LGE_JAVA:
			ianjuta_macro_insert(macro, "Header_cpp", NULL);
			break;
		case  LGE_CSHARP:
			ianjuta_macro_insert(macro, "Header_csharp", NULL);
			break;
		case LGE_PERL:
			ianjuta_macro_insert(macro, "Header_perl", NULL);
			break;
		case LGE_SHELL:
			ianjuta_macro_insert(macro, "Header_shell", NULL);
			break;
		default:
			break;
	}
}
