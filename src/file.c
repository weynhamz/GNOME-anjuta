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

#include "anjuta.h"
#include "text_editor.h"
#include "file.h"


#define NEW_FILE_DIALOG "dialog.new.file"
#define NEW_FILE_ENTRY "new.file.entry"
#define NEW_FILE_TYPE "new.file.type"
#define NEW_FILE_TEMPLATE "new.file.template"
#define NEW_FILE_HEADER "new.file.header"
#define NEW_FILE_GPL "new.file.gpl"


static gboolean create_new_file_dialog(void);

static gchar *file_insert_c_gpl(void);

static gchar *file_insert_cpp_gpl(void);

static gchar *file_insert_py_gpl(void);

static gchar *file_insert_header_templ(TextEditor *te);

static gchar *file_insert_header_c( TextEditor *te);

/* Callbacks */
gboolean
on_new_file_cancelbutton_clicked(GtkWidget *window, GdkEvent *event,
			                     gboolean user_data);

gboolean
on_new_file_okbutton_clicked(GtkWidget *window, GdkEvent *event,
			                 gboolean user_data);

void
on_new_file_entry_changed (GtkEditable *entry, gpointer user_data);

void
on_new_file_type_changed (GtkOptionMenu   *optionmenu, gpointer user_data);


typedef struct _NewFileGUI
{
	GladeXML *xml;
	GtkWidget *dialog;
	gboolean showing;
} NewFileGUI;

typedef enum _Cmt
{
	CMT_C,
	CMT_CPP,
	CMT_PY
} Cmt;

typedef struct _NewfileType
{
	gchar *name;
	gchar *ext;
	gboolean header;
	gboolean gpl;
	gboolean template;
	Cmt type;
} NewfileType;

NewfileType new_file_type[] = {
	{N_("C Source File"), ".c", TRUE, TRUE, FALSE, CMT_C},
	{N_("C -C++ Header File"), ".h", TRUE, TRUE, TRUE, CMT_C},
	{N_("C++ Source File"), ".cpp", TRUE, TRUE, FALSE, CMT_C},
	{N_("C# Source File"), ".c#", FALSE, TRUE, FALSE, CMT_CPP},
	{N_("Java Source File"), ".java", FALSE, TRUE, FALSE, CMT_CPP},
	{N_("Perl Source File"), ".pl", FALSE, TRUE, FALSE, CMT_PY},
	{N_("Python Source File"), ".py", FALSE, TRUE, FALSE, CMT_PY},
	{N_("Shell Script File"), ".sh", FALSE, TRUE, FALSE, CMT_PY},
	{N_("Other"), NULL, FALSE, FALSE, FALSE, CMT_C}
};

NewFileGUI *nfg = NULL;


void
display_new_file(void)
{
	if (!nfg)
		if (! create_new_file_dialog())
			return;
	if (nfg && !(nfg->showing))
	{
		gtk_window_present (GTK_WINDOW (nfg->dialog));
		nfg->showing = TRUE;
	}		
}

//~ void
//~ clear_new_file(void)
//~ {
//~     Free nfg at Anjuta closing
//~ }

static gboolean
create_new_file_dialog(void)
{
	GtkWidget *optionmenu;
	GtkWidget *menu;
	GtkWidget *menuitem;
	gint i;

	nfg = g_new0(NewFileGUI, 1);
	if (NULL == (nfg->xml = glade_xml_new(GLADE_FILE_ANJUTA, NEW_FILE_DIALOG, NULL)))	
	{
		anjuta_error(_("Unable to build user interface for New File"));
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
	
	gtk_window_set_transient_for (GTK_WINDOW(nfg->dialog),
		                          GTK_WINDOW(app->widgets.window)); 

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
file_insert_text(gchar *txt, gint offset)
{
	TextEditor *te;
	gint caret;
	
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	caret = aneditor_command (te->editor_id, ANE_GETCURRENTPOS, -1, -1);

	aneditor_command (te->editor_id, ANE_INSERTTEXT, -1, (long)txt);
	
	if (offset < 0)
		aneditor_command (te->editor_id, ANE_GOTOPOS, caret + strlen(txt), -1);
	else
		aneditor_command (te->editor_id, ANE_GOTOPOS, caret + offset, -1);
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
	TextEditor *te;
	
	entry = glade_xml_get_widget(nfg->xml, NEW_FILE_ENTRY);
	name = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
	if ( strlen(name) > 0)
		anjuta_append_text_editor (NULL, name);
	else
		anjuta_append_text_editor (NULL, NULL);
	g_free(name);
	
	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return FALSE;

	checkbutton = glade_xml_get_widget(nfg->xml, NEW_FILE_HEADER);
	if (GTK_WIDGET_SENSITIVE(checkbutton) && 
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton)))
		file_insert_header();	
	
	checkbutton = glade_xml_get_widget(nfg->xml, NEW_FILE_GPL);
	if (GTK_WIDGET_SENSITIVE(checkbutton) && 
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton)))
	{
		optionmenu = glade_xml_get_widget(nfg->xml, NEW_FILE_TYPE);
		sel = gtk_option_menu_get_history(GTK_OPTION_MENU(optionmenu));
		switch (new_file_type[sel].type)
		{
			case CMT_C: 
				file_insert_c_gpl_notice();	
				break;
			case CMT_CPP:
				file_insert_cpp_gpl_notice();	
				break;
			case CMT_PY:
				file_insert_py_gpl_notice();
				break;
			default:
				g_warning("Not known type\n");
		}
	}
	
	checkbutton = glade_xml_get_widget(nfg->xml, NEW_FILE_TEMPLATE);
	if (GTK_WIDGET_SENSITIVE(checkbutton) && 
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton)))
		file_insert_header_template();
		
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
	widget = glade_xml_get_widget(nfg->xml, NEW_FILE_GPL);
	gtk_widget_set_sensitive(widget, new_file_type[sel].gpl);
	widget = glade_xml_get_widget(nfg->xml, NEW_FILE_TEMPLATE);
	gtk_widget_set_sensitive(widget, new_file_type[sel].template);
	
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


static gchar *
file_insert_c_gpl(void)
{
	gchar *GPLNotice =
	"/*\n"
	" *  This program is free software; you can redistribute it and/or modify\n"
	" *  it under the terms of the GNU General Public License as published by\n"
	" *  the Free Software Foundation; either version 2 of the License, or\n"
	" *  (at your option) any later version.\n"
	" *\n"
	" *  This program is distributed in the hope that it will be useful,\n"
	" *  but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	" *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
	" *  GNU Library General Public License for more details.\n"
	" *\n"
	" *  You should have received a copy of the GNU General Public License\n"
	" *  along with this program; if not, write to the Free Software\n"
	" *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.\n"
	" */\n"
	" \n";

	return  GPLNotice;
}


void
file_insert_c_gpl_notice(void)
{
	file_insert_text(file_insert_c_gpl(), -1);
}

static gchar *
file_insert_cpp_gpl(void)
{
	gchar *GPLNotice =
	"// This program is free software; you can redistribute it and/or modify\n"
	"// it under the terms of the GNU General Public License as published by\n"
	"// the Free Software Foundation; either version 2 of the License, or\n"
	"// (at your option) any later version.\n"
	"//\n"
	"// This program is distributed in the hope that it will be useful,\n"
	"// but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	"// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
	"// GNU Library General Public License for more details.\n"
	"//\n"
	"// You should have received a copy of the GNU General Public License\n"
	"// along with this program; if not, write to the Free Software\n"
	"// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.\n"
	"\n";

	return GPLNotice;
}

void
file_insert_cpp_gpl_notice(void)
{
	file_insert_text(file_insert_cpp_gpl(), -1);
}


static gchar *
file_insert_py_gpl(void)
{
	char *GPLNotice =
	"# This program is free software; you can redistribute it and/or modify\n"
	"# it under the terms of the GNU General Public License as published by\n"
	"# the Free Software Foundation; either version 2 of the License, or\n"
	"# (at your option) any later version.\n"
	"#\n"
	"# This program is distributed in the hope that it will be useful,\n"
	"# but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	"# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
	"# GNU Library General Public License for more details.\n"
	"#\n"
	"# You should have received a copy of the GNU General Public License\n"
	"# along with this program; if not, write to the Free Software\n"
	"# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.\n"
	"\n";
	return GPLNotice;
}

void
file_insert_py_gpl_notice(void)
{
	file_insert_text(file_insert_py_gpl(), -1);
}


static gchar *
file_insert_header_templ(TextEditor *te)
{
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

	i = strlen(te->filename);
	if ( g_strcasecmp((te->filename) + i - 2, ".h") == 0)
		name = g_strndup(te->filename, i - 2);
	else
	{
		sprintf(mesg, _("The file \"%s\" is not a header file."),
				te->filename);
		anjuta_warning (mesg);
		return NULL;
	}
	g_strup(name);  /* do not use with GTK2 */
	buffer = g_strconcat("#ifndef _", name, "_H\n#define _", name,
						header_template, name, "_H */\n", NULL);

	g_free(name);
	return buffer;
}

void
file_insert_header_template(void)
{
	TextEditor *te;
	gchar *header;

	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;

	header =  file_insert_header_templ(te);
	if (header == NULL)
		return;
	file_insert_text(header, 0);

	g_free(header);
}


static char *
file_insert_d_t(void)
{
	time_t cur_time = time(NULL);
	gchar *DateTime;

	DateTime = g_new(gchar, 100);
	sprintf(DateTime,ctime(&cur_time));
	return DateTime;
}                                                            ;

void
file_insert_date_time(void)
{
	TextEditor *te;
	gchar *DateTime;

	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;
	DateTime = file_insert_d_t();
	file_insert_text(DateTime, -1);

	g_free(DateTime);
}

static gchar *get_username(void)
{
	gchar *Username;

	Username =
		anjuta_preferences_get (ANJUTA_PREFERENCES (app->preferences),
								IDENT_NAME);
	if (!Username)
		Username = getenv("USERNAME");
	if (!Username)
		Username = getenv("USER");
	return Username;
}

void
file_insert_username(void)
{
	file_insert_text(get_username(), -1);
}


static gchar *file_insert_email(void)
{
	gchar *email;
	gchar *Username;

	email =
		anjuta_preferences_get (ANJUTA_PREFERENCES (app->preferences),
								IDENT_EMAIL);
	if (!email)
	{
		email = getenv("HOSTNAME");
		Username = getenv("USERNAME");
		if (!Username)
			Username = getenv("USER");
		email = g_strconcat(Username, "@", email, NULL);
	}
	return email;
}


static gchar *
file_insert_copyright(void)
{
	gchar *Username;
	gchar *copyright;
	gchar datetime[20];
	struct tm *lt;
	time_t cur_time = time(NULL);

	lt = localtime(&cur_time);
	strftime (datetime, 20, N_("%Y"), lt);
	Username = get_username();
	copyright = g_strconcat("Copyright  ", datetime, "  ", Username, NULL);

	return copyright;
}

static gchar *
file_insert_changelog(void)
{
	gchar *Username;
	gchar *email;
	gchar *CLEntry;
	gchar datetime[20];
	struct tm *lt;
	time_t cur_time = time(NULL);

	CLEntry = g_new(gchar, 200);
	lt = localtime(&cur_time);
	strftime (datetime, 20, N_("%Y-%m-%d"), lt);

	Username =  get_username();
	email = file_insert_email();
	sprintf(CLEntry,"%s  %s <%s>\n", datetime, Username, email);
	g_free(email);
  	
	return  CLEntry;
}

void
file_insert_changelog_entry(void)
{
	TextEditor *te;
	gchar *changelog;

	te = anjuta_get_current_text_editor ();
	if (te == NULL)
  	return;
	changelog = file_insert_changelog();
	file_insert_text(changelog, -1);

	g_free(changelog);
}

static gchar *
file_insert_header_c( TextEditor *te)
{
 	gchar *buffer;
	gchar *tmp;
	gchar *star;
	gchar *copyright;
	gchar *email;

	star =  g_strnfill(75, '*');
	tmp = g_strdup(te->filename);
	buffer = g_strconcat("/", star, "\n *            ", tmp, "\n *\n", NULL);
	g_free(tmp);
	tmp = file_insert_d_t();
	buffer = g_strconcat( buffer, " *  ", tmp, NULL);
	g_free(tmp);
	copyright = file_insert_copyright();
	buffer = g_strconcat(buffer, " *  ", copyright, "\n", NULL);
	g_free(copyright);
	email = file_insert_email();
	buffer = g_strconcat(buffer, " *  ", email, "\n", NULL);
	g_free(email);
	buffer = g_strconcat(buffer, " ", star, "*/\n\n", NULL);
	g_free(star);

	return buffer;
}

void
file_insert_header(void)
{
	TextEditor *te;
	gchar *header;

	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;

	header = file_insert_header_c(te);
	file_insert_text(header, -1);

	g_free(header);
}

void
file_insert_switch_template(void)
{
	gchar *switch_template =
	"switch ()\n"
	"{\n"
	"\tcase  :\n"
	"\t\t;\n"
	"\t\tbreak;\n"
	"\tcase  :\n"
	"\t\t;\n"
	"\t\tbreak;\n"
	"\tdefaults:\n"
	"\t\t;\n"
	"\t\tbreak;\n"
	"}\n";

	file_insert_text(switch_template, 8);
}

void
file_insert_for_template(void)
{
	gchar *for_template =
	"for ( ; ; )\n"
	"{\n"
	"\t;\n"
	"}\n";

	file_insert_text(for_template, 5);
}

void
file_insert_while_template(void)
{
	gchar *while_template =
	"while ()\n"
	"{\n"
	"\t;\n"
	"}\n";

	file_insert_text(while_template, 7);
}

void
file_insert_ifelse_template(void)
{
	gchar *ifelse_template =
	"if ()\n"
	"{\n"
	"\t;\n"
	"}\n"
	"else\n"
	"{\n"
	"\t;\n"
	"}\n";

	file_insert_text(ifelse_template, 4);
}


void
file_insert_cvs_author(void)
{
	gchar *cvs_string_value = "Author";
	gchar *cvs_string;
	
	cvs_string = g_strconcat("$", cvs_string_value, "$\n", NULL);
	file_insert_text(cvs_string, -1);
}

void
file_insert_cvs_date(void)
{
	gchar *cvs_string_value = "Date";
	gchar *cvs_string;
	
	cvs_string = g_strconcat("$", cvs_string_value, "$\n", NULL);
	file_insert_text(cvs_string, -1);
}

void
file_insert_cvs_header(void)
{
	gchar *cvs_string_value = "Header";
	gchar *cvs_string;
	
	cvs_string = g_strconcat("$", cvs_string_value, "$\n", NULL);
	file_insert_text(cvs_string, -1);
}

void
file_insert_cvs_id(void)
{
	gchar *cvs_string_value = "Id";
	gchar *cvs_string;
	
	cvs_string = g_strconcat("$", cvs_string_value, "$\n", NULL);
	file_insert_text(cvs_string, -1);
}

void
file_insert_cvs_log(void)
{
	gchar *cvs_string_value = "Log";
	gchar *cvs_string;
	
	cvs_string = g_strconcat("$", cvs_string_value, "$\n", NULL);
	file_insert_text(cvs_string, -1);
}

void
file_insert_cvs_name(void)
{
	gchar *cvs_string_value = "Name";
	gchar *cvs_string;
	
	cvs_string = g_strconcat("$", cvs_string_value, "$\n", NULL);
	file_insert_text(cvs_string, -1);
}

void
file_insert_cvs_revision(void)
{
	gchar *cvs_string_value = "Revision";
	gchar *cvs_string;
	
	cvs_string = g_strconcat("$", cvs_string_value, "$\n", NULL);
	file_insert_text(cvs_string, -1);
}

void
file_insert_cvs_source(void)
{
	gchar *cvs_string_value = "Source";
	gchar *cvs_string;
	
	cvs_string = g_strconcat("$", cvs_string_value, "$\n", NULL);
	file_insert_text(cvs_string, -1);
}
