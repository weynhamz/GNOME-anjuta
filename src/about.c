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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gnome.h>
#include "about.h"

#define ANJUTA_PIXMAP_LOGO			"anjuta_logo2.png"
#define ABOUT_AUTHORS				"AUTHORS"	
#define MAX_CAR 256
#define MAX_CREDIT 500

const gchar *authors[MAX_CREDIT];
const gchar *documenters[MAX_CREDIT];
gchar *translators;


gchar *about_read_line(FILE *fp)
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
// A modifier	
	if ( (fp = fopen(PACKAGE_DOC_DIR"/"ABOUT_AUTHORS, "r")) )
	{
		line = about_read_line(fp);
		do
		{
			line = g_strchomp(line);
			if (g_str_has_suffix(line, "Developer:") || g_str_has_suffix(line, "Developers:") ||
				g_str_has_suffix(line, "Contributors:") || g_str_has_suffix(line, "Note:"))
			{
				line = about_read_developers(fp, line, &i_auth, authors);
			}
			else if (g_str_has_suffix(line, "Website:") || g_str_has_suffix(line, "Documenters:") )
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
	
	/*  Parse AUTHOR.xml file  */
	about_read_file();
	
	pix = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"ANJUTA_PIXMAP_LOGO, NULL);
	dialog = gnome_about_new ("Anjuta", VERSION, 
							  _("Copyright (c) Naba Kumar"),
							  _("Integrated Development Environment"),
							  authors, documenters, translators, pix);
	g_object_unref (pix);
#if 0
	/* GTK provides a new about dialog now, please activate
	when we swith to GTK 2.6 */
	dialog = gtk_about_dialog_new();
	gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(dialog), "Anjuta");
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), VERSION);
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog), 
		_("Copyright (c) Naba Kumar"));
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog),
		_("Integrated Development Environment");
	gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(dialog), 
		_("GNU General Public License"));
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog), "www.anjuta.org");
	gtk_about_dialog_set_icon(GTK_ABOUT_DIALOG(dialog), pix);
	
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dialog), authors);
	gtk_about_dialog_set_documenters(GTK_ABOUT_DIALOG(dialog), documentors);
	/* We should fill this!
	gtk_about_dialog_set_translator_credits(GTK_ABOUT_DIALOG(dialog), ???);
	gtk_about_dialog_set_artists(GTK_ABOUT_DIALOG(dialog), ???);*/
#endif
	/* Free authors, documenters, translators */
	about_free_credit();
	return dialog;
}
