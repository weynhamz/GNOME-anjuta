/*
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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "sourceview-prefs.h"
#include "sourceview-private.h"
#include "sourceview-provider.h"
#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/completion-providers/words/gtksourcecompletionwords.h>

#include <libanjuta/anjuta-debug.h>

#include <gconf/gconf-client.h>

#define REGISTER_NOTIFY(key, func) \
	notify_id = anjuta_preferences_notify_add (sv->priv->prefs, \
											   key, func, sv); \
	sv->priv->notify_ids = g_list_prepend (sv->priv->notify_ids, \
										   GUINT_TO_POINTER(notify_id));
/* Editor preferences */
#define HIGHLIGHT_SYNTAX           "sourceview.syntax.highlight"
#define HIGHLIGHT_CURRENT_LINE	   "sourceview.currentline.highlight"
#define USE_TABS                   "use.tabs"
#define HIGHLIGHT_BRACKETS         "sourceview.brackets.highlight"
#define TAB_SIZE                   "tabsize"
#define INDENT_SIZE                "indent.size"
#define AUTOCOMPLETION             "sourceview.autocomplete"

#define VIEW_LINENUMBERS           "margin.linenumber.visible"
#define VIEW_MARKS                 "margin.marker.visible"
#define VIEW_RIGHTMARGIN           "sourceview.rightmargin.visible"
#define VIEW_WHITE_SPACES          "view.whitespace"
#define VIEW_EOL                   "view.eol"
#define VIEW_LINE_WRAP             "view.line.wrap"
#define RIGHTMARGIN_POSITION       "sourceview.rightmargin.position"

#define COLOR_ERROR								 "messages.color.error"
#define COLOR_WARNING							 "messages.color.warning"


#define FONT_THEME "sourceview.font.use_theme"
#define FONT "sourceview.font"
#define DESKTOP_FIXED_FONT "/desktop/gnome/interface/monospace_font_name"


static void
on_notify_view_spaces (AnjutaPreferences* prefs,
                       const gchar* key,
                       gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);
	GtkSourceDrawSpacesFlags flags = 
		gtk_source_view_get_draw_spaces (GTK_SOURCE_VIEW (sv->priv->view));
	
	if (anjuta_preferences_get_bool (prefs, key))
		flags |= (GTK_SOURCE_DRAW_SPACES_SPACE | GTK_SOURCE_DRAW_SPACES_TAB);
	else
		flags &= ~(GTK_SOURCE_DRAW_SPACES_SPACE | GTK_SOURCE_DRAW_SPACES_TAB);
		
	gtk_source_view_set_draw_spaces (GTK_SOURCE_VIEW(sv->priv->view), 
																	 flags);
}

static void
on_notify_view_eol (AnjutaPreferences* prefs,
                    const gchar* key,
                    gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);
	GtkSourceDrawSpacesFlags flags = 
		gtk_source_view_get_draw_spaces (GTK_SOURCE_VIEW (sv->priv->view));
	
	if (anjuta_preferences_get_bool (prefs, key))
		flags |= GTK_SOURCE_DRAW_SPACES_NEWLINE;
	else
		flags &= ~GTK_SOURCE_DRAW_SPACES_NEWLINE;
		
	gtk_source_view_set_draw_spaces (GTK_SOURCE_VIEW(sv->priv->view), 
																	 flags);
}

static void
on_notify_line_wrap (AnjutaPreferences* prefs,
                           const gchar* key,
                           gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);
	
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (sv->priv->view),
	                             anjuta_preferences_get_bool (prefs, key) ? GTK_WRAP_WORD : GTK_WRAP_NONE);
}

static void
on_notify_disable_hilite (AnjutaPreferences* prefs,
                          const gchar* key,
                          gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);

	gtk_source_buffer_set_highlight_syntax(GTK_SOURCE_BUFFER(sv->priv->document),
	                                       anjuta_preferences_get_bool (prefs, key));

}

static void
on_notify_highlight_current_line(AnjutaPreferences* prefs,
                                 const gchar* key,
                                 gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);

	gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(sv->priv->view),
	                                           anjuta_preferences_get_bool (prefs, key));
}

static void
on_notify_tab_size (AnjutaPreferences* prefs,
                    const gchar* key,
                    gpointer user_data)
{
	Sourceview *sv;

	sv = ANJUTA_SOURCEVIEW(user_data);

	g_return_if_fail(GTK_IS_SOURCE_VIEW(sv->priv->view));

	gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(sv->priv->view),
	                              anjuta_preferences_get_int (prefs, key));
}

static void
on_notify_use_tab_for_indentation (AnjutaPreferences* prefs,
                                   const gchar* key,
                                   gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);

	gtk_source_view_set_insert_spaces_instead_of_tabs(GTK_SOURCE_VIEW(sv->priv->view),
	                                                  !anjuta_preferences_get_bool (prefs, key));
}

static void
on_notify_braces_check (AnjutaPreferences* prefs,
                        const gchar* key,
                        gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);

	gtk_source_buffer_set_highlight_matching_brackets(GTK_SOURCE_BUFFER(sv->priv->document), 
	                                                  anjuta_preferences_get_bool (prefs, key));
}

static void
on_notify_autocompletion (AnjutaPreferences* prefs,
                         const gchar* key,
                         gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);
	GtkSourceCompletion* completion = gtk_source_view_get_completion(GTK_SOURCE_VIEW(sv->priv->view));

	if (anjuta_preferences_get_bool (prefs, key))
	{
		DEBUG_PRINT ("Register word completion provider");
		GtkSourceCompletionWords *prov_words;
		prov_words = gtk_source_completion_words_new (NULL, NULL);

		gtk_source_completion_words_register (prov_words,
		                                      gtk_text_view_get_buffer (GTK_TEXT_VIEW (sv->priv->view)));

		gtk_source_completion_add_provider (completion, 
		                                    GTK_SOURCE_COMPLETION_PROVIDER (prov_words), 
		                                    NULL);
	}
	else
	{
		GList* node;
		for (node = gtk_source_completion_get_providers(completion); node != NULL; node = g_list_next (node))
		{
			if (GTK_IS_SOURCE_COMPLETION_WORDS(node->data))
			{
				DEBUG_PRINT ("Unregister word completion provider");
				gtk_source_completion_words_unregister (GTK_SOURCE_COMPLETION_WORDS(node->data),
				                                        gtk_text_view_get_buffer (GTK_TEXT_VIEW (sv->priv->view)));
				gtk_source_completion_remove_provider(completion, GTK_SOURCE_COMPLETION_PROVIDER(node->data), NULL);
				break;
			}
		}
	}
}

static void
on_notify_view_marks (AnjutaPreferences* prefs,
                      const gchar* key,
                      gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);

	gtk_source_view_set_show_line_marks(GTK_SOURCE_VIEW(sv->priv->view), 
	                                    anjuta_preferences_get_bool (prefs, key));

}

static void
on_notify_view_linenums (AnjutaPreferences* prefs,
                         const gchar* key,
                         gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);
	
	gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(sv->priv->view), 
	                                      anjuta_preferences_get_bool (prefs, key));
	
}

static void
on_notify_view_right_margin (AnjutaPreferences* prefs,
                             const gchar* key,
                             gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);
	
	gtk_source_view_set_show_right_margin(GTK_SOURCE_VIEW(sv->priv->view), 
	                                      anjuta_preferences_get_bool (prefs, key));
}

static void
on_notify_right_margin_position (AnjutaPreferences* prefs,
                                 const gchar* key,
                                 gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);
	
	gtk_source_view_set_right_margin_position(GTK_SOURCE_VIEW(sv->priv->view), 
	                                          anjuta_preferences_get_bool (prefs, key));
	
}

static void
on_notify_font (AnjutaPreferences* prefs,
                const gchar* key,
                gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);
	gchar* font = anjuta_preferences_get (prefs, key);
		
	if (font != NULL)
		anjuta_view_set_font(sv->priv->view, FALSE,
		                     font);
	g_free (font);
}

static void
on_notify_font_theme (AnjutaPreferences* prefs,
                      const gchar* key,
                      gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);
	
	if (anjuta_preferences_get_bool (prefs, key))
	{
		GConfClient *gclient = gconf_client_get_default ();
		gchar *desktop_fixed_font;
		desktop_fixed_font =
			gconf_client_get_string (gclient, DESKTOP_FIXED_FONT, NULL);
		if (desktop_fixed_font)
			anjuta_view_set_font(sv->priv->view, FALSE, desktop_fixed_font);
		else
			anjuta_view_set_font(sv->priv->view, TRUE, NULL);
		g_free (desktop_fixed_font);
	}
	else
	{
		gchar* font = anjuta_preferences_get (prefs, FONT);
		if (font != NULL)
			anjuta_view_set_font(sv->priv->view, FALSE, font);
		g_free (font);
	}
}

/* Preferences notifications */
static void
on_notify_indic_colors (AnjutaPreferences* prefs,
                        const gchar *key,
                        gpointer user_data)
{
	char* error_color =
		anjuta_preferences_get (anjuta_preferences_default(),
		                        "messages.color.error");
	char* warning_color =
		anjuta_preferences_get (anjuta_preferences_default(),
		                        "messages.color.warning");
	Sourceview* sv = ANJUTA_SOURCEVIEW (user_data);

	g_object_set (sv->priv->warning_indic, "foreground", warning_color, NULL);
	g_object_set (sv->priv->critical_indic, "foreground", error_color, NULL);

	g_free (error_color);
	g_free (warning_color);
}

static void
init_fonts(Sourceview* sv)
{
	gboolean font_theme;
	
	font_theme = anjuta_preferences_get_bool (sv->priv->prefs, FONT_THEME);
	
	if (!font_theme)
	{
		gchar* font = anjuta_preferences_get (sv->priv->prefs, FONT);
		if (font != NULL)
			anjuta_view_set_font(sv->priv->view, FALSE, font);
		g_free (font);
	}
	else
	{
		GConfClient *gclient;
		gchar *desktop_fixed_font;
		
		gclient = gconf_client_get_default ();
		desktop_fixed_font =
			gconf_client_get_string (gclient, DESKTOP_FIXED_FONT, NULL);
		if (desktop_fixed_font)
			anjuta_view_set_font(sv->priv->view, FALSE, desktop_fixed_font);
		else
			anjuta_view_set_font(sv->priv->view, TRUE, NULL);
		g_free (desktop_fixed_font);
		g_object_unref (gclient);
	}
}

static int
get_key_int(Sourceview* sv, const gchar* key)
{
	return anjuta_preferences_get_int (sv->priv->prefs, key);
}

static int
get_key_bool(Sourceview* sv, const gchar* key)
{
	return anjuta_preferences_get_bool (sv->priv->prefs, key);
}

void 
sourceview_prefs_init(Sourceview* sv)
{
	guint notify_id;
	GtkSourceDrawSpacesFlags flags = 0;
	
	/* Init */
	gtk_source_buffer_set_highlight_syntax(GTK_SOURCE_BUFFER(sv->priv->document), 
	                                       get_key_bool(sv, HIGHLIGHT_SYNTAX));
	gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(sv->priv->view),
	                                           get_key_bool(sv, HIGHLIGHT_CURRENT_LINE));
	gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(sv->priv->view), 
	                              get_key_int(sv, TAB_SIZE));
	gtk_source_view_set_indent_width(GTK_SOURCE_VIEW(sv->priv->view), -1); /* Same as tab width */
	gtk_source_view_set_insert_spaces_instead_of_tabs(GTK_SOURCE_VIEW(sv->priv->view),
	                                                  !get_key_bool(sv, USE_TABS));
	gtk_source_buffer_set_highlight_matching_brackets(GTK_SOURCE_BUFFER(sv->priv->document), 
	                                                  get_key_bool(sv, HIGHLIGHT_BRACKETS));
	gtk_source_view_set_show_line_marks(GTK_SOURCE_VIEW(sv->priv->view), 
	                                    get_key_bool(sv, VIEW_MARKS));
	gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(sv->priv->view), 
	                                      get_key_bool(sv, VIEW_LINENUMBERS));
	gtk_source_view_set_show_right_margin(GTK_SOURCE_VIEW(sv->priv->view), 
	                                      get_key_bool(sv, VIEW_RIGHTMARGIN));
	gtk_source_view_set_right_margin_position(GTK_SOURCE_VIEW(sv->priv->view), 
	                                          get_key_int(sv, RIGHTMARGIN_POSITION));
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (sv->priv->view),
	                             get_key_bool (sv, VIEW_EOL) ? GTK_WRAP_WORD : GTK_WRAP_NONE);


	if (get_key_bool (sv, VIEW_WHITE_SPACES))
		flags |= (GTK_SOURCE_DRAW_SPACES_SPACE | GTK_SOURCE_DRAW_SPACES_TAB);
	if (get_key_bool (sv, VIEW_EOL))
		flags |= GTK_SOURCE_DRAW_SPACES_NEWLINE;

	gtk_source_view_set_draw_spaces (GTK_SOURCE_VIEW (sv->priv->view),
	                                 flags);

	init_fonts(sv);

	on_notify_autocompletion(sv->priv->prefs, AUTOCOMPLETION, sv);
  
	/* Register gconf notifications */
	REGISTER_NOTIFY (TAB_SIZE, on_notify_tab_size);
	REGISTER_NOTIFY (USE_TABS, on_notify_use_tab_for_indentation);
	REGISTER_NOTIFY (HIGHLIGHT_SYNTAX, on_notify_disable_hilite);
	REGISTER_NOTIFY (HIGHLIGHT_CURRENT_LINE, on_notify_highlight_current_line);
	REGISTER_NOTIFY (HIGHLIGHT_BRACKETS, on_notify_braces_check);
	REGISTER_NOTIFY (AUTOCOMPLETION, on_notify_autocompletion);
	REGISTER_NOTIFY (VIEW_MARKS, on_notify_view_marks);
	REGISTER_NOTIFY (VIEW_LINENUMBERS, on_notify_view_linenums);
	REGISTER_NOTIFY (VIEW_WHITE_SPACES, on_notify_view_spaces);
	REGISTER_NOTIFY (VIEW_EOL, on_notify_view_eol);  
	REGISTER_NOTIFY (VIEW_LINE_WRAP, on_notify_line_wrap);
	REGISTER_NOTIFY (VIEW_RIGHTMARGIN, on_notify_view_right_margin);
	REGISTER_NOTIFY (RIGHTMARGIN_POSITION, on_notify_right_margin_position);
	REGISTER_NOTIFY (FONT_THEME, on_notify_font_theme);
	REGISTER_NOTIFY (FONT, on_notify_font);
	REGISTER_NOTIFY (COLOR_ERROR, on_notify_indic_colors);
	REGISTER_NOTIFY (COLOR_WARNING, on_notify_indic_colors);
}

void sourceview_prefs_destroy(Sourceview* sv)
{
	AnjutaPreferences* prefs = sv->priv->prefs;
	GList* id;
	DEBUG_PRINT("%s", "Destroying preferences");
	for (id = sv->priv->notify_ids; id != NULL; id = id->next)
	{
		anjuta_preferences_notify_remove(prefs,GPOINTER_TO_UINT(id->data));
	}
	g_list_free(sv->priv->notify_ids);
}
