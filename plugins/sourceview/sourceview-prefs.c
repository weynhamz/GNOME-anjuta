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

static AnjutaPreferences* prefs = NULL;

#define REGISTER_NOTIFY(key, func) \
	notify_id = anjuta_preferences_notify_add (sv->priv->prefs, \
																						key, func, sv, NULL); \
	sv->priv->gconf_notify_ids = g_list_prepend (sv->priv->gconf_notify_ids, \
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

static int
get_int(GConfEntry* entry)
{
	GConfValue* value = gconf_entry_get_value(entry);
	return gconf_value_get_int(value);
}

static gboolean
get_bool(GConfEntry* entry)
{
	GConfValue* value = gconf_entry_get_value(entry);
	/* The value might be deleted */
	if (!value)
		return FALSE;
	/* Usually we would use get_bool but anjuta_preferences saves bool as int 
		#409408 */
	if (value->type == GCONF_VALUE_BOOL)
		return gconf_value_get_bool (value);
	else
		return gconf_value_get_int(value);
}

static void
on_gconf_notify_view_spaces (GConfClient *gclient, guint cnxn_id,
														 GConfEntry *entry, gpointer user_data)
{
	Sourceview *sv;
	gboolean visible = get_bool(entry);
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
on_gconf_notify_view_eols (GConfClient *gclient, guint cnxn_id,
													 GConfEntry *entry, gpointer user_data)
{
	Sourceview *sv;
	gboolean visible = get_bool(entry);
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
on_gconf_notify_line_wrap (GConfClient *gclient, guint cnxn_id,
													 GConfEntry *entry, gpointer user_data)
{
	Sourceview *sv;
	gboolean line_wrap = get_bool(entry);
	sv = ANJUTA_SOURCEVIEW(user_data);
	
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (sv->priv->view),
															 line_wrap ? GTK_WRAP_WORD : GTK_WRAP_NONE);
}

static void
on_gconf_notify_disable_hilite (GConfClient *gclient, guint cnxn_id,
																GConfEntry *entry, gpointer user_data)
{
	Sourceview *sv;
	gboolean highlight = get_bool(entry);
	sv = ANJUTA_SOURCEVIEW(user_data);
	
	gtk_source_buffer_set_highlight_syntax(GTK_SOURCE_BUFFER(sv->priv->document),
																				 highlight);
	
}

static void
on_gconf_notify_highlight_current_line (GConfClient *gclient, guint cnxn_id,
																				GConfEntry *entry, gpointer user_data)
{
	Sourceview *sv;
	gboolean highlight_current_line = get_bool(entry);
	sv = ANJUTA_SOURCEVIEW(user_data);
	
	gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(sv->priv->view),
																						 highlight_current_line);
}

static void
on_gconf_notify_tab_size (GConfClient *gclient, guint cnxn_id,
													GConfEntry *entry, gpointer user_data)
{
	Sourceview *sv;
	gint tab_size = get_int(entry);
	
	sv = ANJUTA_SOURCEVIEW(user_data);
	
	g_return_if_fail(GTK_IS_SOURCE_VIEW(sv->priv->view));
	
	gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(sv->priv->view), tab_size);
}

static void
on_gconf_notify_use_tab_for_indentation (GConfClient *gclient, guint cnxn_id,
																				 GConfEntry *entry, gpointer user_data)
{
	Sourceview *sv;
	gboolean use_tabs = get_bool(entry);
	sv = ANJUTA_SOURCEVIEW(user_data);
	
	gtk_source_view_set_insert_spaces_instead_of_tabs(GTK_SOURCE_VIEW(sv->priv->view),
																										!use_tabs);
}

static void
on_gconf_notify_braces_check (GConfClient *gclient, guint cnxn_id,
															GConfEntry *entry, gpointer user_data)
{
	Sourceview *sv;
	gboolean braces_check = get_bool(entry);
	sv = ANJUTA_SOURCEVIEW(user_data);
	
	gtk_source_buffer_set_highlight_matching_brackets(GTK_SOURCE_BUFFER(sv->priv->document), 
																										braces_check);
}

static void
on_gconf_notify_view_marks (GConfClient *gclient, guint cnxn_id,
														GConfEntry *entry, gpointer user_data)
{
	Sourceview *sv;
	gboolean show_markers = get_bool(entry);
	sv = ANJUTA_SOURCEVIEW(user_data);
	
	gtk_source_view_set_show_line_marks(GTK_SOURCE_VIEW(sv->priv->view), 
																			show_markers);
	
}

static void
on_gconf_notify_view_linenums (GConfClient *gclient, guint cnxn_id,
															 GConfEntry *entry, gpointer user_data)
{
	Sourceview *sv;
	gboolean show_lineno = get_bool(entry);
	sv = ANJUTA_SOURCEVIEW(user_data);
	
	gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(sv->priv->view), 
																				show_lineno);
	
}

static void
on_gconf_notify_view_right_margin (GConfClient *gclient, guint cnxn_id,
																	 GConfEntry *entry, gpointer user_data)
{
	Sourceview *sv;
	gboolean show_margin = get_bool(entry);
	sv = ANJUTA_SOURCEVIEW(user_data);
	
	gtk_source_view_set_show_right_margin(GTK_SOURCE_VIEW(sv->priv->view), 
																				show_margin);
	
}

static void
on_gconf_notify_right_margin_position (GConfClient *gclient, guint cnxn_id,
																			 GConfEntry *entry, gpointer user_data)
{
	Sourceview *sv;
	gboolean pos = get_bool(entry);
	sv = ANJUTA_SOURCEVIEW(user_data);
	
	gtk_source_view_set_right_margin_position(GTK_SOURCE_VIEW(sv->priv->view), 
																						pos);
	
}

static void
on_gconf_notify_font (GConfClient *gclient, guint cnxn_id,
											GConfEntry *entry, gpointer user_data)
{
	Sourceview *sv;
	gchar* font;
	AnjutaPreferences* prefs = sourceview_get_prefs();
	sv = ANJUTA_SOURCEVIEW(user_data);
	
	font = anjuta_preferences_get(prefs, FONT);
	
	if (font != NULL)
		anjuta_view_set_font(sv->priv->view, FALSE, font);
	g_free (font);
}

static void
on_gconf_notify_font_theme (GConfClient *gclient, guint cnxn_id,
														GConfEntry *entry, gpointer user_data)
{
	Sourceview *sv;
	gboolean use_theme = get_bool(entry);
	sv = ANJUTA_SOURCEVIEW(user_data);
	
	if (use_theme)
	{
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
		on_gconf_notify_font(NULL, 0, NULL, sv);
}

static void
init_fonts(Sourceview* sv)
{
	gboolean font_theme;
	
	font_theme = anjuta_preferences_get_int(prefs, FONT_THEME);
	
	if (!font_theme)
	{
		on_gconf_notify_font(NULL, 0, NULL, sv);
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
get_key(Sourceview* sv, const gchar* key, gint default_value)
{
	return anjuta_preferences_get_int_with_default (sv->priv->prefs, key, default_value);
}

void 
sourceview_prefs_init(Sourceview* sv)
{
	guint notify_id;
	GtkSourceDrawSpacesFlags flags = 0;
	
	prefs = sv->priv->prefs;
	
	/* Init */
	gtk_source_buffer_set_highlight_syntax(GTK_SOURCE_BUFFER(sv->priv->document), get_key(sv, HIGHLIGHT_SYNTAX, TRUE));
	gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(sv->priv->view),
																						 get_key(sv, HIGHLIGHT_CURRENT_LINE, FALSE));
	gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(sv->priv->view), get_key(sv, TAB_SIZE, 4));
	gtk_source_view_set_indent_width(GTK_SOURCE_VIEW(sv->priv->view), -1); /* Same as tab width */
	gtk_source_view_set_insert_spaces_instead_of_tabs(GTK_SOURCE_VIEW(sv->priv->view),
																										!get_key(sv, USE_TABS, FALSE));
	gtk_source_buffer_set_highlight_matching_brackets(GTK_SOURCE_BUFFER(sv->priv->document), 
																										get_key(sv, HIGHLIGHT_BRACKETS, FALSE));
	gtk_source_view_set_show_line_marks(GTK_SOURCE_VIEW(sv->priv->view), 
																			get_key(sv, VIEW_MARKS, TRUE));
	gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(sv->priv->view), 
																				get_key(sv, VIEW_LINENUMBERS, TRUE));
	gtk_source_view_set_show_right_margin(GTK_SOURCE_VIEW(sv->priv->view), 
																				get_key(sv, VIEW_RIGHTMARGIN, TRUE));
	gtk_source_view_set_right_margin_position(GTK_SOURCE_VIEW(sv->priv->view), 
																						get_key(sv, RIGHTMARGIN_POSITION, 80));
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (sv->priv->view),
															 get_key (sv, VIEW_EOL, FALSE) ? GTK_WRAP_WORD : GTK_WRAP_NONE);

	
	if (get_key (sv, VIEW_WHITE_SPACES, FALSE))
		flags |= (GTK_SOURCE_DRAW_SPACES_SPACE | GTK_SOURCE_DRAW_SPACES_TAB);
	if (get_key (sv, VIEW_EOL, FALSE))
		flags |= GTK_SOURCE_DRAW_SPACES_NEWLINE;
	
	gtk_source_view_set_draw_spaces (GTK_SOURCE_VIEW (sv->priv->view),
																	 flags);
	
	init_fonts(sv);
	
	/* Register gconf notifications */
	REGISTER_NOTIFY (TAB_SIZE, on_gconf_notify_tab_size);
	REGISTER_NOTIFY (USE_TABS, on_gconf_notify_use_tab_for_indentation);
	REGISTER_NOTIFY (HIGHLIGHT_SYNTAX, on_gconf_notify_disable_hilite);
	REGISTER_NOTIFY (HIGHLIGHT_CURRENT_LINE, on_gconf_notify_highlight_current_line);
	REGISTER_NOTIFY (HIGHLIGHT_BRACKETS, on_gconf_notify_braces_check);
	REGISTER_NOTIFY (VIEW_MARKS, on_gconf_notify_view_marks);
	REGISTER_NOTIFY (VIEW_LINENUMBERS, on_gconf_notify_view_linenums);
	REGISTER_NOTIFY (VIEW_RIGHTMARGIN, on_gconf_notify_view_right_margin);
	REGISTER_NOTIFY (VIEW_WHITE_SPACES, on_gconf_notify_view_spaces);		
	REGISTER_NOTIFY (VIEW_EOL, on_gconf_notify_view_eols);		  
	REGISTER_NOTIFY (VIEW_LINE_WRAP, on_gconf_notify_line_wrap);		  
	REGISTER_NOTIFY (RIGHTMARGIN_POSITION, on_gconf_notify_right_margin_position);
	REGISTER_NOTIFY (FONT_THEME, on_gconf_notify_font_theme);
	REGISTER_NOTIFY (FONT, on_gconf_notify_font);	
}

void sourceview_prefs_destroy(Sourceview* sv)
{
	AnjutaPreferences* prefs = sv->priv->prefs;
	GList* id;
	DEBUG_PRINT("%s", "Destroying preferences");
	for (id = sv->priv->gconf_notify_ids; id != NULL; id = id->next)
	{
		anjuta_preferences_notify_remove(prefs,GPOINTER_TO_UINT(id->data));
	}
	g_list_free(sv->priv->gconf_notify_ids);
}


AnjutaPreferences*
sourceview_get_prefs()
{
	return prefs;
}
