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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include <time.h>

#include <gnome.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-macro.h>

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

#define IDENT_NAME                 "ident.name"
#define IDENT_EMAIL                "ident.email"

static gboolean create_new_file_dialog(IAnjutaDocumentManager *docman);

static gchar *insert_header_templ(IAnjutaEditor *te);

typedef struct _NewFileGUI
{
	GladeXML *xml;
	GtkWidget *dialog;
	gboolean showing;
} NewFileGUI;


typedef struct _NewfileType
{
	gchar *name;
	gchar *ext;
	gboolean header;
	gboolean gpl;
	gboolean template;
	gchar *start_comment;
	gchar *mid_comment;
	gchar *end_comment;
	Cmt type;
} NewfileType;

NewfileType new_file_type[] = {
	{N_("C Source File"), ".c", TRUE, TRUE, FALSE, "/*\n", " * ", " */\n\n", CMT_C},
	{N_("C -C++ Header File"), ".h", TRUE, TRUE, TRUE, "/*\n", " * ", " */\n\n", CMT_HC},
	{N_("C++ Source File"), ".cxx", TRUE, TRUE, FALSE, "\n", "// ", "\n", CMT_CPLUS},
	{N_("C# Source File"), ".c#", TRUE, FALSE, FALSE, "\n", "// ", "\n", CMT_CSHARP},
	{N_("Java Source File"), ".java", TRUE, TRUE, FALSE, "\n", "// ", "\n", CMT_JAVA},
	{N_("Perl Source File"), ".pl", TRUE, TRUE, FALSE, "\n", "# ", " *\n", CMT_PERL},
	{N_("Python Source File"), ".py", FALSE, TRUE, FALSE, "\n", "# ", "\n", CMT_PYTHON},
	{N_("Shell Script File"), ".sh", TRUE, TRUE, FALSE, "\n", "#  ", "\n", CMT_SHELL},
	{N_("Other"), NULL, FALSE, FALSE, FALSE, "/*\n", " * ", " */\n\n", CMT_C}
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

void
display_new_file(IAnjutaDocumentManager *docman)
{	
	if (!nfg)
		if (! create_new_file_dialog(docman))
			return;
	if (nfg && !(nfg->showing))
	{
		gtk_window_present (GTK_WINDOW (nfg->dialog));
		nfg->showing = TRUE;
	}
}


/* Callback declarations */
gboolean on_new_file_cancelbutton_clicked(GtkWidget *window, GdkEvent *event,
										  gboolean user_data);
gboolean on_new_file_okbutton_clicked(GtkWidget *window, GdkEvent *event,
									  gboolean user_data);
void on_new_file_entry_changed (GtkEditable *entry, gpointer user_data);
void on_new_file_type_changed (GtkOptionMenu   *optionmenu, gpointer user_data);

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
	
	//gtk_window_set_transient_for (GTK_WINDOW(nfg->dialog),
	//	                          GTK_WINDOW(app)); 

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

//~ Offset<0 	:	Move cursor to the end of txt
//~ Offset >=0 	:	Move cursor + offset
void 
file_insert_text(IAnjutaEditor *te, gchar *txt, gint offset)
{
	gint caret;
	g_return_if_fail (IANJUTA_IS_EDITOR (te));

	caret = ianjuta_editor_get_position (te, NULL);
	ianjuta_editor_insert (te, -1, txt, -1, NULL);
	if (offset < 0)
		ianjuta_editor_goto_position (te, caret + strlen(txt), NULL);
	else
		ianjuta_editor_goto_position (te, caret + offset, NULL);
}

AnjutaPreferences *
get_preferences (AnjutaPlugin *plugin)
{
	return anjuta_shell_get_preferences (plugin->shell, NULL);
}

gboolean
on_new_file_okbutton_clicked(GtkWidget *window, GdkEvent *event,
			                 gboolean user_data)
{
	GtkWidget *entry;
	GtkWidget *checkbutton;
	GtkWidget *optionmenu;
	gchar *name;
	gint sel;
	gint license_type;
	gint source_type;
	IAnjutaEditor *te;
	IAnjutaDocumentManager *docman;
	GtkWidget *toplevel;
	IAnjutaMacro* macro;
	
	toplevel= gtk_widget_get_toplevel (window);
	docman = IANJUTA_DOCUMENT_MANAGER (g_object_get_data (G_OBJECT(toplevel),
										"IAnjutaDocumentManager"));
	macro = anjuta_shell_get_interface
		                      (ANJUTA_PLUGIN(docman)->shell, 
		                       IAnjutaMacro, NULL);
	entry = glade_xml_get_widget(nfg->xml, NEW_FILE_ENTRY);
	name = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
	if (strlen(name) > 0)
		te = ianjuta_document_manager_add_buffer (docman, name, "", NULL);
	else
		te = ianjuta_document_manager_add_buffer (docman, "", "", NULL);
	g_free(name);
	
	if (te == NULL)
		return FALSE;

	optionmenu = glade_xml_get_widget(nfg->xml, NEW_FILE_TYPE);
	source_type = gtk_option_menu_get_history(GTK_OPTION_MENU(optionmenu));
	
	checkbutton = glade_xml_get_widget(nfg->xml, NEW_FILE_LICENSE);
	if (GTK_WIDGET_SENSITIVE(checkbutton) && 
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton)))
	{
		optionmenu = glade_xml_get_widget(nfg->xml, NEW_FILE_MENU_LICENSE);
		sel = gtk_option_menu_get_history(GTK_OPTION_MENU(optionmenu));
		license_type =new_license_type[sel].type;
				
		insert_notice (te, source_type, license_type);
	}
	
	checkbutton = glade_xml_get_widget(nfg->xml, NEW_FILE_TEMPLATE);
	if (GTK_WIDGET_SENSITIVE(checkbutton) && 
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton)))
	{
		ianjuta_macro_insert(macro, "Header_h", NULL);
	}		
	
	checkbutton = glade_xml_get_widget(nfg->xml, NEW_FILE_HEADER);
	if (GTK_WIDGET_SENSITIVE(checkbutton) && 
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton)))
	{
		insert_header(macro, source_type);
	}
	
	gtk_widget_hide(nfg->dialog);
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
	
	if ( (last_length != 2) && ((length = strlen(name)) == 1) )
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
 

void
insert_notice(IAnjutaEditor *te, gint comment_type, gint license_type)
{
	gchar *gpl_notice = 
		"This program is free software; you can redistribute it and/or modify\n"
		"it under the terms of the GNU General Public License as published by\n"
		"the Free Software Foundation; either version 2 of the License, or\n"
		"(at your option) any later version.\n\n"
		"This program is distributed in the hope that it will be useful,\n"
		"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
		"GNU Library General Public License for more details.\n\n"
		"You should have received a copy of the GNU General Public License\n"
		"along with this program; if not, write to the Free Software\n"
		"Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.\n";
	char *lgpl_notice = 
		"This program is free software; you can redistribute it and/or modify\n"
		"it under the terms of the GNU Lesser General Public License as published\n"
		"by the Free Software Foundation; either version 2.1 of the License, or\n"
		"(at your option) any later version.\n\n"
		"This program is distributed in the hope that it will be useful,\n"
		"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n"
		"Lesser General Public License for more details.\n\n"
		"You should have received a copy of the GNU Lesser General Public\n"
		"License along with this program; if not, write to the Free Software\n"
		"Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.\n";
			
	gchar *notice;
	gchar *tmp;
	gchar *start;
	gchar *buffer;

	switch (license_type)
		{
			case GPL: 
				notice = gpl_notice;
				break;
			case LGPL:
				notice = lgpl_notice;
				break;
			default:
				g_warning ("License not known\n");
		}
	
	buffer = g_strdup(new_file_type[comment_type].start_comment);

	start = notice;
	while(*notice++)
	{
		if (*notice == '\n')
		{
			tmp = g_strndup(start, notice - start +1);
			buffer = g_strconcat(buffer, new_file_type[comment_type].mid_comment, tmp, NULL);
			g_free(tmp);
			start = notice + 1;
		}	
	}
	buffer = g_strconcat(buffer, new_file_type[comment_type].end_comment, NULL);

	file_insert_text(te, buffer, -1);
	g_free(buffer);
}


static gchar *
insert_header_templ(IAnjutaEditor *te)
{
	GtkWidget *parent;
	gchar *header_template =
	"_H\n"
	"\n"
	"#ifdef __cplusplus\n"
	"extern \"C\"\n"
	"{\n"
	"#endif\n"
	"\n\n\n"
	"#ifdef __cplusplus\n"
	"}\n"
	"#endif\n"
	"\n"
	"#endif /* _";
	gchar *buffer;
	gchar *name = NULL;
	gchar mesg[256];
	gint i;
	const gchar *filename;
	
	filename = ianjuta_editor_get_filename (IANJUTA_EDITOR (te), NULL);	
	i = strlen(filename);
	if ( g_strcasecmp((filename) + i - 2, ".h") == 0)
		name = g_strndup(filename, i - 2);
	else
	{
		parent = gtk_widget_get_toplevel (GTK_WIDGET (te));
		sprintf(mesg, _("The file \"%s\" is not a header file."),
				filename);
// FIXME	anjuta_util_dialog_warning (GTK_WINDOW (te), mesg);
		g_warning("%s\n", mesg);
		return NULL;
	}
	name = g_ascii_strup(name, -1);
	buffer = g_strconcat("#ifndef _", name, "_H\n#define _", name,
						header_template, name, "_H */\n", NULL);

	g_free (name);
	return buffer;
}

void
insert_header_template(IAnjutaEditor *te)
{
	gchar *header;

	header =  insert_header_templ(te);
	if (header == NULL)
		return;
	file_insert_text(te, header, 0);

	g_free(header);
}


static char *
insert_d_t(void)
{
	time_t cur_time = time(NULL);
	gchar *DateTime;

	DateTime = g_new(gchar, 100);
	sprintf(DateTime,ctime(&cur_time));
	return DateTime;
}                                                            ;

void
insert_date_time(IAnjutaEditor *te)
{
	gchar *DateTime;
	DateTime = insert_d_t();
	file_insert_text(te, DateTime, -1);

	g_free(DateTime);
}

void
insert_header(IAnjutaMacro* macro, gint source_type)
{
	switch (source_type)
	{
		case  CMT_C: case CMT_HC:
			ianjuta_macro_insert(macro, "Header_c", NULL);
			break;
		case  CMT_CPLUS: case CMT_JAVA:
			ianjuta_macro_insert(macro, "Header_cpp", NULL);
			break;
		case  CMT_CSHARP:
			ianjuta_macro_insert(macro, "Header_csharp", NULL);
			break;
		case CMT_PERL:
			ianjuta_macro_insert(macro, "Header_perl", NULL);
			break;
		case CMT_SHELL:
			ianjuta_macro_insert(macro, "Header_shell", NULL);
			break;
		default:
			break;
	}
}
