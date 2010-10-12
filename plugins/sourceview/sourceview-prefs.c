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
	g_signal_connect (sv->priv->settings, "changed::" key, G_CALLBACK(func), sv);

#define PREF_SCHEMA "org.gnome.anjuta.sourceview"

/* Editor preferences */
#define HIGHLIGHT_SYNTAX           "sourceview-syntax-highlight"
#define HIGHLIGHT_CURRENT_LINE	   "sourceview-currentline-highlight"
#define USE_TABS                   "use-tabs"
#define HIGHLIGHT_BRACKETS         "sourceview-brackets-highlight"
#define TAB_SIZE                   "tabsize"
#define INDENT_SIZE                "indent-size"
#define AUTOCOMPLETION             "sourceview-autocomplete"

#define VIEW_LINENUMBERS           "margin-linenumber-visible"
#define VIEW_MARKS                 "margin-marker-visible"
#define VIEW_RIGHTMARGIN           "sourceview-rightmargin-visible"
#define VIEW_WHITE_SPACES          "docman-whitespace"
#define VIEW_EOL                   "docman-eol"
#define VIEW_LINE_WRAP             "docman-line-wrap"
#define RIGHTMARGIN_POSITION       "sourceview-rightmargin-position"

#define COLOR_ERROR							 "msgman-color-error"
#define COLOR_WARNING							 "msgman-color.warning"


#define FONT_THEME "sourceview-font-use-theme"
#define FONT "sourceview-font"
#define DESKTOP_FIXED_FONT "/desktop/gnome/interface/monospace_font_name"


static void
on_notify_view_spaces (GSettings* settings,
                       const gchar* key,
                       gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);
	GtkSourceDrawSpacesFlags flags = 
		gtk_source_view_get_draw_spaces (GTK_SOURCE_VIEW (sv->priv->view));
	
	if (g_settings_get_boolean (settings, key))
		flags |= (GTK_SOURCE_DRAW_SPACES_SPACE | GTK_SOURCE_DRAW_SPACES_TAB);
	else
		flags &= ~(GTK_SOURCE_DRAW_SPACES_SPACE | GTK_SOURCE_DRAW_SPACES_TAB);
		
	gtk_source_view_set_draw_spaces (GTK_SOURCE_VIEW(sv->priv->view), 
																	 flags);
}

static void
on_notify_view_eol (GSettings* settings,
                    const gchar* key,
                    gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);
	GtkSourceDrawSpacesFlags flags = 
		gtk_source_view_get_draw_spaces (GTK_SOURCE_VIEW (sv->priv->view));
	
	if (g_settings_get_boolean (settings, key))
		flags |= GTK_SOURCE_DRAW_SPACES_NEWLINE;
	else
		flags &= ~GTK_SOURCE_DRAW_SPACES_NEWLINE;
		
	gtk_source_view_set_draw_spaces (GTK_SOURCE_VIEW(sv->priv->view), 
																	 flags);
}

static void
on_notify_line_wrap (GSettings* settings,
                           const gchar* key,
                           gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);
	
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (sv->priv->view),
	                             g_settings_get_boolean (settings, key) ? GTK_WRAP_WORD : GTK_WRAP_NONE);
}

static void
on_notify_disable_hilite (GSettings* settings,
                          const gchar* key,
                          gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);

	gtk_source_buffer_set_highlight_syntax(GTK_SOURCE_BUFFER(sv->priv->document),
	                                       g_settings_get_boolean (settings, key));

}

static void
on_notify_highlight_current_line(GSettings* settings,
                                 const gchar* key,
                                 gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);

	gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(sv->priv->view),
	                                           g_settings_get_boolean (settings, key));
}

static void
on_notify_tab_size (GSettings* settings,
                    const gchar* key,
                    gpointer user_data)
{
	Sourceview *sv;

	sv = ANJUTA_SOURCEVIEW(user_data);

	g_return_if_fail(GTK_IS_SOURCE_VIEW(sv->priv->view));

	gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(sv->priv->view),
	                              g_settings_get_int (sv->priv->settings, key));
}

static void
on_notify_use_tab_for_indentation (GSettings* settings,
                                   const gchar* key,
                                   gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);

	gtk_source_view_set_insert_spaces_instead_of_tabs(GTK_SOURCE_VIEW(sv->priv->view),
	                                                  !g_settings_get_boolean (settings, key));
}

static void
on_notify_braces_check (GSettings* settings,
                        const gchar* key,
                        gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);

	gtk_source_buffer_set_highlight_matching_brackets(GTK_SOURCE_BUFFER(sv->priv->document), 
	                                                  g_settings_get_boolean (settings, key));
}

static void
on_notify_autocompletion (GSettings* settings,
                         const gchar* key,
                         gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);
	GtkSourceCompletion* completion = gtk_source_view_get_completion(GTK_SOURCE_VIEW(sv->priv->view));

	if (g_settings_get_boolean (settings, key))
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
on_notify_view_marks (GSettings* settings,
                      const gchar* key,
                      gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);

	gtk_source_view_set_show_line_marks(GTK_SOURCE_VIEW(sv->priv->view), 
	                                    g_settings_get_boolean (settings, key));

}

static void
on_notify_view_linenums (GSettings* settings,
                         const gchar* key,
                         gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);
	
	gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(sv->priv->view), 
	                                      g_settings_get_boolean (settings, key));
	
}

static void
on_notify_view_right_margin (GSettings* settings,
                             const gchar* key,
                             gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);
	
	gtk_source_view_set_show_right_margin(GTK_SOURCE_VIEW(sv->priv->view), 
	                                      g_settings_get_boolean (settings, key));
}

static void
on_notify_right_margin_position (GSettings* settings,
                                 const gchar* key,
                                 gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);
	
	gtk_source_view_set_right_margin_position(GTK_SOURCE_VIEW(sv->priv->view), 
	                                          g_settings_get_boolean (settings, key));
	
}

static void
on_notify_font (GSettings* settings,
                const gchar* key,
                gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);
	gchar* font = g_settings_get_string (settings, key);

	anjuta_view_set_font(sv->priv->view, FALSE,
	                     font);
	g_free (font);
}

static void
on_notify_font_theme (GSettings* settings,
                      const gchar* key,
                      gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);
	
	if (g_settings_get_boolean (settings, key))
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
		gchar* font = g_settings_get_string (settings, FONT);
		anjuta_view_set_font(sv->priv->view, FALSE, font);
		g_free (font);
	}
}

/* Preferences notifications */
static void
on_notify_indic_colors (GSettings* settings,
                        const gchar *key,
                        gpointer user_data)
{
	char* error_color =
		 g_settings_get_string (settings,
		                        COLOR_ERROR);
	char* warning_color =
		 g_settings_get_string (settings,
		                        COLOR_WARNING);
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
	
	font_theme = g_settings_get_boolean (sv->priv->settings, FONT_THEME);
	
	if (!font_theme)
	{
		gchar* font = g_settings_get_string (sv->priv->settings, FONT);
		anjuta_view_set_font(sv->priv->view, FALSE, font);
		g_free (font);
	}
	else
	{
		/* FIXME: Get font from GSettings */	
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
	return g_settings_get_int (sv->priv->settings, key);
}

static int
get_key_bool(Sourceview* sv, const gchar* key)
{
	return g_settings_get_int (sv->priv->settings, key);
}

#define PREF_SCHEMA_MSGMAN "org.gnome.anjuta.message-manager"

void 
sourceview_prefs_init(Sourceview* sv)
{
	GtkSourceDrawSpacesFlags flags = 0;
	GSettings* msg_settings = g_settings_new (PREF_SCHEMA_MSGMAN);
	/* We create a new GSettings object here because if we used the one from
	 * the editor might be destroyed while the plugin is still alive
	 */
	sv->priv->settings = g_settings_new (PREF_SCHEMA);
	
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

	on_notify_autocompletion(sv->priv->settings, AUTOCOMPLETION, sv);
  
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

	g_signal_connect (msg_settings, "changed::" COLOR_ERROR,
	                  G_CALLBACK (on_notify_indic_colors), sv);
	g_signal_connect (msg_settings, "changed::" COLOR_WARNING,
	                  G_CALLBACK (on_notify_indic_colors), sv);	
}

void sourceview_prefs_destroy(Sourceview* sv)
{
	g_object_unref (sv->priv->settings);
}
