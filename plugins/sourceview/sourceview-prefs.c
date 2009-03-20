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
#include <gtksourceview/gtksourceview.h>

#include <libanjuta/anjuta-debug.h>

#include <gconf/gconf-client.h>

#define REGISTER_NOTIFY(key, func, type) \
	notify_id = anjuta_preferences_notify_add_##type (sv->priv->prefs, \
																						       key, func, sv, NULL); \
	sv->priv->notify_ids = g_list_prepend (sv->priv->notify_ids, \
						 														GUINT_TO_POINTER(notify_id));
/* Editor preferences */
#define HIGHLIGHT_SYNTAX           "sourceview.syntax.highlight"
#define HIGHLIGHT_CURRENT_LINE	   "sourceview.currentline.highlight"
#define USE_TABS                   "use.tabs"
#define HIGHLIGHT_BRACKETS         "sourceview.brackets.highlight"
#define TAB_SIZE                   "tabsize"
#define INDENT_SIZE                "indent.size"

#define VIEW_LINENUMBERS           "margin.linenumber.visible"
#define VIEW_MARKS                 "margin.marker.visible"
#define VIEW_RIGHTMARGIN           "sourceview.rightmargin.visible"
#define VIEW_WHITE_SPACES          "view.whitespace"
#define VIEW_EOL                   "view.eol"
#define VIEW_LINE_WRAP             "view.line.wrap"
#define RIGHTMARGIN_POSITION       "sourceview.rightmargin.position"



#define FONT_THEME "sourceview.font.use_theme"
#define FONT "sourceview.font"
#define DESKTOP_FIXED_FONT "/desktop/gnome/interface/monospace_font_name"


static void
on_notify_view_spaces (AnjutaPreferences* prefs,
                       const gchar* key,
                       gboolean visible,
                       gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);
	GtkSourceDrawSpacesFlags flags = 
		gtk_source_view_get_draw_spaces (GTK_SOURCE_VIEW (sv->priv->view));
	
	if (visible)
		flags |= (GTK_SOURCE_DRAW_SPACES_SPACE | GTK_SOURCE_DRAW_SPACES_TAB);
	else
		flags &= ~(GTK_SOURCE_DRAW_SPACES_SPACE | GTK_SOURCE_DRAW_SPACES_TAB);
		
	gtk_source_view_set_draw_spaces (GTK_SOURCE_VIEW(sv->priv->view), 
																	 flags);
}

static void
on_notify_view_eol (AnjutaPreferences* prefs,
                    const gchar* key,
                    gboolean visible,
                    gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);
	GtkSourceDrawSpacesFlags flags = 
		gtk_source_view_get_draw_spaces (GTK_SOURCE_VIEW (sv->priv->view));
	
	if (visible)
		flags |= GTK_SOURCE_DRAW_SPACES_NEWLINE;
	else
		flags &= ~GTK_SOURCE_DRAW_SPACES_NEWLINE;
		
	gtk_source_view_set_draw_spaces (GTK_SOURCE_VIEW(sv->priv->view), 
																	 flags);
}

static void
on_notify_line_wrap (AnjutaPreferences* prefs,
                           const gchar* key,
                           gboolean line_wrap,
                           gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);
	
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (sv->priv->view),
															 line_wrap ? GTK_WRAP_WORD : GTK_WRAP_NONE);
}

static void
on_notify_disable_hilite (AnjutaPreferences* prefs,
                          const gchar* key,
                          gboolean highlight,
                          gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);

	gtk_source_buffer_set_highlight_syntax(GTK_SOURCE_BUFFER(sv->priv->document),
	                                       highlight);

}

static void
on_notify_highlight_current_line(AnjutaPreferences* prefs,
                                 const gchar* key,
                                 gboolean highlight_current_line,
                                 gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);

	gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(sv->priv->view),
	                                           highlight_current_line);
}

static void
on_notify_tab_size (AnjutaPreferences* prefs,
                    const gchar* key,
                    gint tab_size,
                    gpointer user_data)
{
	Sourceview *sv;

	sv = ANJUTA_SOURCEVIEW(user_data);

	g_return_if_fail(GTK_IS_SOURCE_VIEW(sv->priv->view));

	gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(sv->priv->view), tab_size);
}

static void
on_notify_use_tab_for_indentation (AnjutaPreferences* prefs,
                                   const gchar* key,
                                   gboolean use_tabs,
                                   gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);

	gtk_source_view_set_insert_spaces_instead_of_tabs(GTK_SOURCE_VIEW(sv->priv->view),
	                                                  !use_tabs);
}

static void
on_notify_braces_check (AnjutaPreferences* prefs,
                        const gchar* key,
                        gboolean braces_check,
                        gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);

	gtk_source_buffer_set_highlight_matching_brackets(GTK_SOURCE_BUFFER(sv->priv->document), 
	                                                  braces_check);
}

static void
on_notify_view_marks (AnjutaPreferences* prefs,
                      const gchar* key,
                      gboolean show_markers,
                      gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);

	gtk_source_view_set_show_line_marks(GTK_SOURCE_VIEW(sv->priv->view), 
	                                    show_markers);

}

static void
on_notify_view_linenums (AnjutaPreferences* prefs,
                         const gchar* key,
                         gboolean show_lineno,
                         gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);
	
	gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(sv->priv->view), 
																				show_lineno);
	
}

static void
on_notify_view_right_margin (AnjutaPreferences* prefs,
                             const gchar* key,
                             gboolean show_margin,
                             gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);
	
	gtk_source_view_set_show_right_margin(GTK_SOURCE_VIEW(sv->priv->view), 
																				show_margin);
}

static void
on_notify_right_margin_position (AnjutaPreferences* prefs,
                                 const gchar* key,
                                 int pos,
                                 gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);
	
	gtk_source_view_set_right_margin_position(GTK_SOURCE_VIEW(sv->priv->view), 
																						pos);
	
}

static void
on_notify_font (AnjutaPreferences* prefs,
                const gchar* key,
                const gchar* font,
                gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);
		
	if (font != NULL)
		anjuta_view_set_font(sv->priv->view, FALSE, font);
}

static void
on_notify_font_theme (AnjutaPreferences* prefs,
                      const gchar* key,
                      gboolean use_theme,
                      gpointer user_data)
{
	Sourceview *sv;
	sv = ANJUTA_SOURCEVIEW(user_data);
	
	if (use_theme)
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
get_key_int(Sourceview* sv, const gchar* key, gint default_value)
{
	return anjuta_preferences_get_int_with_default (sv->priv->prefs, key, default_value);
}

static int
get_key_bool(Sourceview* sv, const gchar* key, gboolean default_value)
{
	return anjuta_preferences_get_bool_with_default (sv->priv->prefs, key, default_value);
}

void 
sourceview_prefs_init(Sourceview* sv)
{
	guint notify_id;
	GtkSourceDrawSpacesFlags flags = 0;
	
	/* Init */
	gtk_source_buffer_set_highlight_syntax(GTK_SOURCE_BUFFER(sv->priv->document), get_key_bool(sv, HIGHLIGHT_SYNTAX, TRUE));
	gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(sv->priv->view),
																						 get_key_bool(sv, HIGHLIGHT_CURRENT_LINE, FALSE));
	gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(sv->priv->view), get_key_int(sv, TAB_SIZE, 4));
	gtk_source_view_set_indent_width(GTK_SOURCE_VIEW(sv->priv->view), -1); /* Same as tab width */
	gtk_source_view_set_insert_spaces_instead_of_tabs(GTK_SOURCE_VIEW(sv->priv->view),
																										!get_key_bool(sv, USE_TABS, FALSE));
	gtk_source_buffer_set_highlight_matching_brackets(GTK_SOURCE_BUFFER(sv->priv->document), 
																										get_key_bool(sv, HIGHLIGHT_BRACKETS, FALSE));
	gtk_source_view_set_show_line_marks(GTK_SOURCE_VIEW(sv->priv->view), 
																			get_key_bool(sv, VIEW_MARKS, TRUE));
	gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(sv->priv->view), 
																				get_key_bool(sv, VIEW_LINENUMBERS, TRUE));
	gtk_source_view_set_show_right_margin(GTK_SOURCE_VIEW(sv->priv->view), 
																				get_key_bool(sv, VIEW_RIGHTMARGIN, TRUE));
	gtk_source_view_set_right_margin_position(GTK_SOURCE_VIEW(sv->priv->view), 
																						get_key_int(sv, RIGHTMARGIN_POSITION, 80));
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (sv->priv->view),
															 get_key_bool (sv, VIEW_EOL, FALSE) ? GTK_WRAP_WORD : GTK_WRAP_NONE);

	
	if (get_key_bool (sv, VIEW_WHITE_SPACES, FALSE))
		flags |= (GTK_SOURCE_DRAW_SPACES_SPACE | GTK_SOURCE_DRAW_SPACES_TAB);
	if (get_key_bool (sv, VIEW_EOL, FALSE))
		flags |= GTK_SOURCE_DRAW_SPACES_NEWLINE;
	
	gtk_source_view_set_draw_spaces (GTK_SOURCE_VIEW (sv->priv->view),
																	 flags);
	
	init_fonts(sv);
	
	/* Register gconf notifications */
	REGISTER_NOTIFY (TAB_SIZE, on_notify_tab_size, int);
	REGISTER_NOTIFY (USE_TABS, on_notify_use_tab_for_indentation, bool);
	REGISTER_NOTIFY (HIGHLIGHT_SYNTAX, on_notify_disable_hilite, bool);
	REGISTER_NOTIFY (HIGHLIGHT_CURRENT_LINE, on_notify_highlight_current_line, bool);
	REGISTER_NOTIFY (HIGHLIGHT_BRACKETS, on_notify_braces_check, bool);
	REGISTER_NOTIFY (VIEW_MARKS, on_notify_view_marks, bool);
	REGISTER_NOTIFY (VIEW_LINENUMBERS, on_notify_view_linenums, bool);
	REGISTER_NOTIFY (VIEW_WHITE_SPACES, on_notify_view_spaces, bool);		
	REGISTER_NOTIFY (VIEW_EOL, on_notify_view_eol, bool);		  
	REGISTER_NOTIFY (VIEW_LINE_WRAP, on_notify_line_wrap, bool);		  
	REGISTER_NOTIFY (VIEW_RIGHTMARGIN, on_notify_view_right_margin, bool);
	REGISTER_NOTIFY (RIGHTMARGIN_POSITION, on_notify_right_margin_position, int);
	REGISTER_NOTIFY (FONT_THEME, on_notify_font_theme, bool);
	REGISTER_NOTIFY (FONT, on_notify_font, string);	
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
