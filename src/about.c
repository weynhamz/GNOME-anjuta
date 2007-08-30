/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    about.c
    Copyright (C) 2002 Naba Kumar   <naba@gnome.org>

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gtk/gtkaboutdialog.h>
#include <libanjuta/anjuta-plugin-manager.h>

#include "about.h"

#define ANJUTA_PIXMAP_LOGO			"anjuta_logo.png"
#define ABOUT_AUTHORS				"AUTHORS"	
#define MAX_CAR 256
#define MAX_CREDIT 500

const gchar *authors[MAX_CREDIT];
const gchar *documenters[MAX_CREDIT];
gchar *translators;


static gchar*
about_read_line(FILE *fp)
{
	static gchar tpn[MAX_CAR];
	char *pt;
	char c;
	
	pt = tpn;
	while( ((c=getc(fp))!='\n') && (c!=EOF) && ((pt-tpn)<MAX_CAR) )
		*(pt++)=c;
	*pt = '\0';
	if ( c!=EOF)
		return tpn;
	else
		return NULL;	
}

static gchar*
about_read_developers(FILE *fp, gchar *line, gint *index, const gchar **tab)
{
	do
	{
		if (*index < MAX_CREDIT)
			tab[(*index)++] = g_strdup_printf("%s", line);
		if ( !(line = about_read_line(fp)))
			return NULL;
		line = g_strchomp(line);
	}
	while (!g_str_has_suffix(line, ":") );
	
	return line;	
}

static gchar*
read_documenters(FILE *fp, gchar *line, gint *index, const gchar **tab)
{
	do
	{
		if (*index < MAX_CREDIT)
			tab[(*index)++] = g_strdup_printf("%s", line);
		if ( !(line = about_read_line(fp)))
			return NULL;
		line = g_strchomp(line);
	}
	while ( !g_str_has_suffix(line, ":") );
	
	return line;	
}

static gchar*
read_translators(FILE *fp, gchar *line)
{
	gboolean found = FALSE;
	gchar *env_lang = getenv("LANG");
	
	do
	{
		if ( !(line = about_read_line(fp)))
			return NULL;
		
		line = g_strchug(line);
		if (!found && g_str_has_prefix(line, env_lang) )
		{
			found = TRUE;
			gchar *tmp = g_strdup(line + strlen(env_lang));
			tmp = g_strchug(tmp);
			translators = g_strconcat("\n\n", tmp, NULL);
			g_free(tmp);
		}
		line = g_strchomp(line);
	}
	while ( !g_str_has_suffix(line, ":") );
	
	return line;	
}

static void 
about_read_file(void)
{
	FILE *fp;
	gchar *line;
	gint i_auth = 0;
	gint i_doc = 0;

	fp = fopen(PACKAGE_DATA_DIR"/"ABOUT_AUTHORS, "r");
	
	g_return_if_fail (fp != NULL);
	line = about_read_line(fp);
	do
	{
		line = g_strchomp(line);
		if (g_str_has_suffix(line, "Developer:") ||
			g_str_has_suffix(line, "Developers:") ||
			g_str_has_suffix(line, "Contributors:") ||
			g_str_has_suffix(line, "Note:"))
		{
			line = about_read_developers(fp, line, &i_auth, authors);
		}
		else if (g_str_has_suffix(line, "Website:") ||
				 g_str_has_suffix(line, "Documenters:") )
		{
			line = read_documenters(fp, line, &i_doc, documenters);
		}
		else if (g_str_has_suffix(line, "Translators:")  )
		{
			line = read_translators(fp, line);
		}
		else
			line = about_read_line(fp);
	}
	while (line);
	fclose(fp);
}

static void
about_free_credit(void)
{
	gint i = 0;
	
	gchar** ptr = (gchar**) authors;
	for(i=0; ptr[i]; i++)
		g_free (ptr[i]);
	ptr = (gchar**) documenters;
	for(i=0; ptr[i]; i++)
		g_free (ptr[i]);

	g_free(translators);
}

GtkWidget *
about_box_new ()
{
	GtkWidget *dialog;
	GdkPixbuf *pix;
	
	/*  Parse AUTHORS file  */
	about_read_file();
	
	pix = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"ANJUTA_PIXMAP_LOGO,
									NULL);
	
	dialog = gtk_about_dialog_new();
	gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(dialog), "Anjuta");
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), VERSION);
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog), 
		_("Copyright (c) Naba Kumar"));
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog),
		_("Integrated Development Environment"));
	gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(dialog), 
		NULL);
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog), "www.anjuta.org");
	gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(dialog), pix);
	
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dialog), authors);
	gtk_about_dialog_set_documenters(GTK_ABOUT_DIALOG(dialog), documenters);
	gtk_about_dialog_set_translator_credits(GTK_ABOUT_DIALOG(dialog), translators);
	/* We should fill this!
	gtk_about_dialog_set_artists(GTK_ABOUT_DIALOG(dialog), ???);*/
	/* Free authors, documenters, translators */
	about_free_credit();
	g_object_unref (pix);
	return dialog;
}

static void
on_about_plugin_activate (GtkMenuItem *item, AnjutaPluginDescription *desc)
{
	gchar *name = NULL;
	gchar *authors = NULL;
	gchar **authors_v = NULL;
	gchar *icon = NULL;
	gchar *d = NULL;
	GdkPixbuf *pix = NULL;
	GtkWidget *dialog;
	
	anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
										  "Name", &name);
	anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
										  "Description", &d);
	anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
										  "Icon", &icon);
	anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
										  "Authors", &authors);
	if (icon)
	{
		gchar *path = g_build_filename (PACKAGE_PIXMAPS_DIR, icon, NULL);
		pix = gdk_pixbuf_new_from_file (path, NULL);
		g_free (path);
	}
	if (authors)
	{
		authors_v = g_strsplit(authors, ",", -1);
	}
	/*dialog = gnome_about_new (name, VERSION, _("Anjuta plugin"), d,
							  authors_v, NULL, NULL, pix);*/
	dialog = gtk_about_dialog_new();
	gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(dialog), name);
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), VERSION);
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog), 
		_("Anjuta Plugin"));
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog),d);
	gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(dialog), pix);
	
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dialog),
								 (const gchar **)authors_v);
	
	gtk_widget_show (dialog);
	g_object_unref (pix);
	g_strfreev (authors_v);
	g_free (name);
	g_free (d);
	g_free (authors);
	g_free (icon);
}

void
about_create_plugins_submenu (AnjutaShell *shell, GtkWidget *menuitem)
{
	GtkWidget *submenu;
	GList *plugin_descs, *node;
	
	g_return_if_fail (ANJUTA_IS_SHELL (shell));
	g_return_if_fail (GTK_IS_MENU_ITEM (menuitem));
	
	submenu = gtk_menu_new ();
	gtk_widget_show (submenu);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), submenu);
	
	plugin_descs =
		anjuta_plugin_manager_query (anjuta_shell_get_plugin_manager (shell, NULL),
									 NULL, NULL, NULL, NULL);
	node = plugin_descs;
	while (node)
	{
		gchar *label;
		GtkWidget *item;
		AnjutaPluginDescription *desc = node->data;
		if (anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
												  "Name", &label))
		{
			gchar *authors;
			if (anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
													  "Authors", &authors))
			{
				item = gtk_menu_item_new_with_label (label);
				gtk_widget_show (item);
				g_signal_connect (G_OBJECT (item), "activate",
								  G_CALLBACK (on_about_plugin_activate),
								  desc);
				gtk_menu_shell_append (GTK_MENU_SHELL (submenu), item);
				g_free (authors);
			}
			g_free (label);
		}
		node = g_list_next (node);
	}
}
