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

#include <libxml/parser.h>

#include "about.h"

#define ANJUTA_PIXMAP_LOGO			"anjuta_logo2.png"
#define ABOUT_AUTHORS				"AUTHORS.xml"						
#define MAX_CREDIT 500

const gchar *authors[MAX_CREDIT];
const gchar *documenters[MAX_CREDIT];
gchar *translators;


static gboolean
about_parse_xml_file (xmlDocPtr * doc, xmlNodePtr * cur, const gchar * filename)
{
	*doc = xmlParseFile (filename);

	if (*doc == NULL)
		return FALSE;
	*cur = xmlDocGetRootElement (*doc);

	if (*cur == NULL)
	{
		xmlFreeDoc (*doc);
		return FALSE;
	}

	if (xmlStrcmp ((*cur)->name, (const xmlChar *) "credit"))
	{
		xmlFreeDoc (*doc);
		return FALSE;
	}
	return TRUE;
}
	
static gint
about_read_authors (xmlDocPtr doc, xmlNodePtr cur, gint index, const gchar **tab)
{
	xmlChar *title;
	
	title = xmlGetProp(cur, "_title");
	tab[index++] = g_strdup_printf("%s",title);
	xmlFree(title);
	
	cur = cur->xmlChildrenNode;
	while (cur != NULL && (index < MAX_CREDIT - 3))
	{
		if ((!xmlStrcmp (cur->name, (const xmlChar *) "author")))
		{
			xmlChar *name;
			xmlChar *email;
			xmlChar *country;
			
			name = xmlGetProp(cur, "_name");
			email = xmlGetProp(cur, "_email");
			country = xmlGetProp(cur, "_country");		
			tab[index] = g_strdup_printf("%s  ", name);
			xmlFree(name);
			if (email)
				tab[index] = g_strconcat(tab[index], "<", email, ">    ", NULL);
			xmlFree(email);
			if (country)
				tab[index] = g_strconcat(tab[index], "(", country, ")", NULL);
			xmlFree(country);
			index++;
		}
		cur = cur->next;
	}
	tab[index++] = g_strdup_printf(" ");
	return index;
}

static gint
about_read_note (xmlDocPtr doc, xmlNodePtr cur, gint index, const gchar **tab)
{
	xmlChar *title;
	
	title = xmlGetProp(cur, "_title");
	tab[index++] = g_strdup_printf("%s",title);
	xmlFree(title);
	
	cur = cur->xmlChildrenNode;
	while ((cur != NULL) && (index < MAX_CREDIT - 3))
	{
		if ((!xmlStrcmp (cur->name, (const xmlChar *) "ntxt")))
		{
			xmlChar *text = xmlGetProp(cur, "_text");
			tab[index++] = g_strdup_printf("%s", text);
			xmlFree(text);
		}
		cur = cur->next;
	}
	tab[index++] = g_strdup_printf(" ");
	return index;
}

static void
about_read_translators (xmlDocPtr doc, xmlNodePtr cur)
{	
	gchar *env_lang = getenv("LANG");
	
	cur = cur->xmlChildrenNode;
	while (cur != NULL)
	{
		if ((!xmlStrcmp (cur->name, (const xmlChar *) "author")))
		{
			xmlChar *name;
			xmlChar *email;
			xmlChar *lang;
			
			lang = xmlGetProp(cur, "_lang");
			if (lang && (strcmp (env_lang, lang) == 0) )
			{
				name = xmlGetProp(cur, "_name");
				translators = g_strdup_printf("\n\n%s  ", name);
				xmlFree(name);
				email = xmlGetProp(cur, "_email");
				if (email)
					translators = g_strconcat(translators, "<", email, ">", NULL);
				xmlFree(email);
				xmlFree(lang);
				return;
			}
			xmlFree(lang);
		}
		cur = cur->next;
	}
}

static void
about_read_contrib (xmlDocPtr doc, xmlNodePtr cur)
{
	gint i_auth = 0;
	gint i_doc = 0;
	
	cur = cur->xmlChildrenNode;
	while (cur != NULL)
	{
		if ((!xmlStrcmp (cur->name, (const xmlChar *) "founder")) ||
			(!xmlStrcmp (cur->name, (const xmlChar *) "present_developers")) ||
			(!xmlStrcmp (cur->name, (const xmlChar *) "past_developers")) ||
			(!xmlStrcmp (cur->name, (const xmlChar *) "contributors"))  )
		{
			i_auth = about_read_authors (doc, cur, i_auth, authors);
		}
		
		else if ((!xmlStrcmp (cur->name, (const xmlChar *) "documenters")) ||
				(!xmlStrcmp (cur->name, (const xmlChar *) "website")))
			i_doc = about_read_authors (doc, cur, i_doc, documenters);
		else if ((!xmlStrcmp (cur->name, (const xmlChar *) "translators")))
			about_read_translators (doc, cur);
		else if ((!xmlStrcmp (cur->name, (const xmlChar *) "note")))
			i_auth = about_read_note (doc, cur, i_auth, authors);
		
		cur = cur->next;
	}
	authors[i_auth] = NULL;
	documenters[i_doc] = NULL;
}

static void
about_read_authors_file(void)
{
	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;
	
	if (about_parse_xml_file (&doc, &cur, PACKAGE_DOC_DIR"/"ABOUT_AUTHORS))
		about_read_contrib (doc, cur);
}

static void
about_free_credit(void)
{
	gint i;
	
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
	about_read_authors_file();

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
