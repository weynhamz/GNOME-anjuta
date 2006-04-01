/*
 * sourceview-autocomplete.c (c) 2006 Johannes Schmid
 * Based on the Python-Code from Guillaume Chazarain
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#include "sourceview-autocomplete.h"
#include "sourceview-prefs.h"
#include "tag-window.h"
#include "config.h"

#include <libanjuta/interfaces/ianjuta-editor.h> 
#include <libanjuta/anjuta-debug.h>
#include <pcre.h>

/* Maximal found autocompletition words */
const gchar* REGEX = "\\s%s[\\w_*]*\\s";

static gboolean
find_duplicate(GSList* list, gchar* word)
{
	while(list)
	{
		if (strcmp(list->data, word) == 0)
			return TRUE;
		list = g_slist_next(list);
	}
	return FALSE;
}


/* Find all words which start with the current word an put them in a List
	Return NULL if no or more then five occurences were found */

static GSList*
get_completions(gchar* current_word, gchar* text, gint choices)
{
	pcre *re;
    GSList* words = NULL;
    gchar* regex;
    gint err_offset;
	const gchar* err;
	gint rc;
	int ovector[2];
	int num_matches = 0;

	/* Create regular expression */
	regex = g_strdup_printf(REGEX, current_word);
	re = pcre_compile(regex, 0, &err, &err_offset, NULL);
   	g_free(regex);
	
	if (NULL == re)
	{
		/* Compile failed - check error message */
		DEBUG_PRINT("Regex compile failed! %s at position %d", err, err_offset);
		return FALSE;
	}
	
	ovector[1] = 0;
	for (;;)
	{
		int options = 0;                 /* Normally no options */
		int start_offset = ovector[1];   /* Start at end of previous match */
		gchar* comp_word;
		int start, end;
		
		if (num_matches > choices)
			break;
		
 	 	if (ovector[0] == ovector[1])
    	{
    		if (ovector[0] == strlen(text)) 
    			break;
    		options = PCRE_NOTEMPTY | PCRE_ANCHORED;
    	}

  		/* Run the next matching operation */
		rc = pcre_exec(re, NULL, text, strlen(text), start_offset,
					   0, ovector, 2);

	 	if (rc == PCRE_ERROR_NOMATCH)
    	{
    		if (options == 0) 
    			break;
    		ovector[1] = start_offset + 1;
    		continue;
    	}

 		 if (rc < 0)
    	{
    		DEBUG_PRINT("Matching error %d\n", rc);
    		return FALSE;
    	}

		if (rc == 0)
    	{
    		break;
    	}
    	
    	start = ovector[0];
    	end = ovector[1];
    	
    	/* Minimum 2 chars (trailing and ending whitespace) */
    	if (start + 2 >= end)
    		continue;
    	
    	comp_word = g_new0(gchar, end - start + 1);
    	strncpy(comp_word, text + start + 1, end - start - 2);
    	if (!find_duplicate(words, comp_word) &&
    		strcmp(comp_word, current_word) != 0)
    	{
		    words = g_slist_append(words, comp_word);
			num_matches++;
		}
		else
			g_free(comp_word);
    }
	return words;
}

#define AUTOCOMPLETE_CHOICES "autocompleteword.choices"
#define PIXBUF PACKAGE_PIXMAPS_DIR"/marker-light.png"

gboolean
sourceview_autocomplete_update(AnjutaView* view, GtkListStore* store, gchar* current_word)
{
	GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	GtkTextIter start_iter;
	GtkTextIter end_iter;
	gchar* text;
	GSList* words;
	GSList* node;
	GdkPixbuf* pixbuf;
	
	gtk_text_buffer_get_iter_at_offset(buffer,
								   &start_iter, 0);
	gtk_text_buffer_get_iter_at_offset(buffer,
								   &end_iter, -1);
	text = gtk_text_buffer_get_text(buffer,
									&start_iter, &end_iter, FALSE);
	
	words = get_completions(current_word, text, 
		anjuta_preferences_get_int (sourceview_get_prefs(), AUTOCOMPLETE_CHOICES));
	if (words == NULL)
		return FALSE;
	
	gtk_list_store_clear(store);
	
	node = words;
	pixbuf = gdk_pixbuf_new_from_file (PIXBUF, NULL);
	while (node)
	{
		GtkTreeIter iter;
		gchar* word = node->data;
	 	gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, TAG_WINDOW_COLUMN_SHOW, word,
        												TAG_WINDOW_COLUMN_PIXBUF, pixbuf,
        												TAG_WINDOW_COLUMN_NAME, word, -1);
        g_free(word);
        node = g_slist_next(node);
     }
     g_object_unref(pixbuf);
     g_slist_free(words);
     return TRUE;
}
 
