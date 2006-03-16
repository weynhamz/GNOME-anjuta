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
#include "sourceview-private.h"

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

static gboolean
get_completions(Sourceview* sv)
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
	regex = g_strdup_printf(REGEX, sv->priv->ac->current_word);
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
		
		if (num_matches >= sv->priv->ac_choices)
			break;
		
 	 	if (ovector[0] == ovector[1])
    	{
    		if (ovector[0] == strlen(sv->priv->ac->text)) 
    			break;
    		options = PCRE_NOTEMPTY | PCRE_ANCHORED;
    	}

  		/* Run the next matching operation */
		rc = pcre_exec(re, NULL, sv->priv->ac->text, strlen(sv->priv->ac->text), start_offset,
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
    	strncpy(comp_word, sv->priv->ac->text + start + 1, end - start - 2);
    	if (!find_duplicate(words, comp_word) &&
    		strcmp(comp_word, sv->priv->ac->current_word) != 0)
    	{
		    words = g_slist_append(words, comp_word);
			num_matches++;
		}
		else
			g_free(comp_word);
    }
   	sv->priv->ac->completions = words;
   	if (sv->priv->ac->completions == NULL)
   		return FALSE;
   	else
	   	return TRUE;
}

/* Return a tuple containing the (x, y) position of the cursor */
static void
get_coordinates(Sourceview* sv, int* x, int* y)
{
	int xor, yor;
	GdkRectangle rect;	
	GdkWindow* window;
	GtkTextIter cursor;
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->document);
	GtkTextView* view = GTK_TEXT_VIEW(sv->priv->view);
	gtk_text_buffer_get_iter_at_mark(buffer, &cursor, gtk_text_buffer_get_insert(buffer)); 
	gtk_text_view_get_iter_location(view, &cursor, &rect);
	window = gtk_text_view_get_window(view, GTK_TEXT_WINDOW_TEXT);
	gtk_text_view_buffer_to_window_coords(view, GTK_TEXT_WINDOW_TEXT, 
		rect.x + rect.width, rect.y, x, y);
	
	gdk_window_get_origin(window, &xor, &yor);
	*x = *x + xor;
	*y = *y + yor;
}

/* Select to map callback of the Entry */

static void
select_to_map(GtkEntry* entry, GdkEvent* event, gchar* word)
{
	gtk_editable_select_region(GTK_EDITABLE(entry), strlen(word), -1);
}

/* Return an entry which contains all
	completion words */

static GtkWidget* 
make_entry_completion(Sourceview* sv)
{
	GtkWidget* entry;
	GtkEntryCompletion* completion;
	GtkListStore* list_store;	
	GSList* node;
	gchar* first_word;
	
	if (!get_completions(sv))
		return NULL;
	
	entry = gtk_entry_new();
	completion = gtk_entry_completion_new();
	list_store = gtk_list_store_new(1, G_TYPE_STRING);

	gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(list_store));
	gtk_entry_completion_set_text_column(completion, 0);
	first_word = g_strdup(sv->priv->ac->completions->data);
	
	node = sv->priv->ac->completions;
	 while (node != NULL)
    {
        GtkTreeIter iter;
        gtk_list_store_append(list_store, &iter);
        gtk_list_store_set(list_store, &iter, 0,node->data, -1);
        node = g_slist_next(node);
     }
    gtk_entry_set_completion(GTK_ENTRY(entry), completion);
	gtk_entry_set_text(GTK_ENTRY(entry), first_word);
	gtk_entry_completion_set_popup_completion(completion, TRUE);
	g_signal_connect_after(G_OBJECT(entry), "map-event", G_CALLBACK(select_to_map), 
		sv->priv->ac->current_word);
	
	gtk_widget_show_all(entry);
	return entry;
}

/* Destroy dialog */

static gboolean
idle_destroy(gpointer* data)
{
    GtkWidget* dialog = GTK_WIDGET(data);
	gtk_widget_destroy(dialog);
	return FALSE;
}

static void 
activate_entry(GtkEntry* entry, GtkDialog* dialog)
{
	gtk_dialog_response(dialog, GTK_RESPONSE_OK);
	g_idle_add((GSourceFunc)idle_destroy, dialog);
}

static void
deactivate_entry(GtkAccelGroup* group, GtkDialog* dialog)
{
	gtk_dialog_response(dialog, GTK_RESPONSE_CANCEL);
	g_idle_add((GSourceFunc)idle_destroy, dialog);
}

/* Return an undecorated dialog with the completion entry */

static const gchar*
get_word_replacement(Sourceview* sv)
{
	GtkWidget* entry = make_entry_completion(sv);
	GtkWidget* completion_dialog;
	gint x, y;
	gint response;
	GtkAccelGroup* accel_group;
	guint accel_key;
	GdkModifierType mod_type;
	
	if (entry == NULL)
		return NULL;
	get_coordinates(sv, &x, &y);

	completion_dialog = gtk_dialog_new_with_buttons(
		"gedit completion",
		NULL,
		GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT|GTK_DIALOG_NO_SEPARATOR,
		NULL);
	gtk_window_set_decorated(GTK_WINDOW(completion_dialog), FALSE);
	gtk_window_move(GTK_WINDOW(completion_dialog), x, y);
	gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(completion_dialog)->vbox), entry);
	gtk_dialog_set_default_response(GTK_DIALOG(completion_dialog), GTK_RESPONSE_OK);
	g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(activate_entry), completion_dialog);
	
	accel_group = gtk_accel_group_new();
	gtk_accelerator_parse("Escape", &accel_key, &mod_type);
	gtk_accel_group_connect(accel_group, accel_key, mod_type, GTK_ACCEL_VISIBLE, 
		g_cclosure_new_object(G_CALLBACK(deactivate_entry), G_OBJECT(completion_dialog)));
	
	gtk_window_add_accel_group(GTK_WINDOW(completion_dialog), accel_group);
	
	response = gtk_dialog_run(GTK_DIALOG(completion_dialog));
	if (response == GTK_RESPONSE_OK)
		return gtk_entry_get_text(GTK_ENTRY(entry));
	else
		return NULL;
}

/* Main autocomplete function. Insert the selected word in the buffer */

void sourceview_autocomplete(Sourceview* sv)
{
	GtkTextIter start_iter, end_iter;   
	GtkTextBuffer * buffer = GTK_TEXT_BUFFER(sv->priv->document);
	GtkTextIter cursor_iter, *word_iter;
	const gchar* replacement;
	
	/* Init */
	sv->priv->ac = g_new0(SourceviewAutocomplete, 1);
	sv->priv->ac->current_word = sourceview_autocomplete_get_current_word(buffer);
	 if (sv->priv->ac->current_word == NULL && !strlen(sv->priv->ac->current_word))
     {
     	return;
     }
	
	/* Get whole buffer text */ 
    gtk_text_buffer_get_start_iter(buffer, &start_iter);
    gtk_text_buffer_get_end_iter(buffer, &end_iter);    
    sv->priv->ac->text = gtk_text_buffer_get_text(buffer, &start_iter, &end_iter, FALSE);
    
    replacement = get_word_replacement(sv);

	if (replacement == NULL)
	{
		g_free(sv->priv->ac->text);
		g_free(sv->priv->ac->current_word);
		return;
	}
	gtk_text_buffer_get_iter_at_mark(buffer, &cursor_iter, 
								 gtk_text_buffer_get_insert(buffer));
	word_iter = gtk_text_iter_copy(&cursor_iter);
	gtk_text_iter_set_line_index(word_iter, 
		gtk_text_iter_get_line_index(&cursor_iter) - strlen(sv->priv->ac->current_word));
	gtk_text_buffer_delete(buffer, word_iter, &cursor_iter);
	gtk_text_buffer_insert_at_cursor(buffer, replacement, 
		strlen(replacement));
		
	/* Clean up */
	{
		GSList* node = sv->priv->ac->completions;
		while (node != NULL)
		{
			g_free(node->data);
			node = g_slist_next(node); 
		}
		g_free(sv->priv->ac->text);
		g_free(sv->priv->ac->current_word);
		g_slist_free(sv->priv->ac->completions);
		gtk_text_iter_free(word_iter);
	}
}

#define WORD_REGEX "[^ \\t]+$"

gchar*
sourceview_autocomplete_get_current_word(GtkTextBuffer* buffer)
{
	pcre *re;
    gint err_offset;
	const gchar* err;
	gint rc, start, end;
	int ovector[2];
	gchar* line, *word;
	GtkTextIter *line_iter, cursor_iter;
	gtk_text_buffer_get_iter_at_mark(buffer, &cursor_iter, 
								 gtk_text_buffer_get_insert(buffer));
	line_iter = gtk_text_iter_copy(&cursor_iter);
	gtk_text_iter_set_line_offset(line_iter, 0);
	line = gtk_text_buffer_get_text(buffer, line_iter, &cursor_iter, FALSE);
	gtk_text_iter_free(line_iter);

	/* Create regular expression */
	re = pcre_compile(WORD_REGEX, 0, &err, &err_offset, NULL);
	
	if (NULL == re)
	{
		/* Compile failed - check error message */
		DEBUG_PRINT("Regex compile failed! %s at position %d", err, err_offset);
		return FALSE;
	}
	
	/* Run the next matching operation */
	rc = pcre_exec(re, NULL, line, strlen(line), 0,
					   0, ovector, 2);

	 if (rc == PCRE_ERROR_NOMATCH)
    {
    	DEBUG_PRINT("No match");
    	return NULL;
    }
 	 else if (rc < 0)
    {
    	DEBUG_PRINT("Matching error %d\n", rc);
    	return NULL;
    }
	else if (rc == 0)
    {
    	DEBUG_PRINT("ovector too small");
    	return NULL;
    }
    start = ovector[0];
    end = ovector[1];
    	    	
    word = g_new0(gchar, end - start + 1);
    strncpy(word, line + start, end - start);
  	
  	return word;
}
 
