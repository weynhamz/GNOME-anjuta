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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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
#define DISABLE_SYNTAX_HILIGHTING  "disable.syntax.hilighting"
#define HIGHLIGHT_CURRENT_LINE	   "sourceview.highlightcurrentline"
#define USE_TABS                   "use.tabs"
#define BRACES_CHECK               "braces.check"
#define TAB_SIZE                   "tabsize"
#define TAB_INDENTS                "tab.indents"
#define BACKSPACE_UNINDENTS        "backspace.unindents"

#define VIEW_LINENUMBERS_MARGIN    "margin.linenumber.visible"
#define VIEW_MARKER_MARGIN         "margin.marker.visible"

#define COLOR_THEME "sourceview.color.use_theme"
#define COLOR_TEXT	"sourceview.color.text"
#define COLOR_BACKGROUND	"sourceview.color.background"
#define COLOR_SELECTED_TEXT	"sourceview.color.selected_text"
#define COLOR_SELECTION	"sourceview.color.selection"

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
	/* Usually we would use get_bool but anjuta_preferences saves bool as int 
		#409408
	*/
	if (value->type == GCONF_VALUE_BOOL)
		return gconf_value_get_bool (value);
	else
		return gconf_value_get_int(value);
}

static void
on_gconf_notify_disable_hilite (GConfClient *gclient, guint cnxn_id,
								GConfEntry *entry, gpointer user_data)
{
	Sourceview *sv;
	gboolean disable_highlight = get_bool(entry);
	sv = ANJUTA_SOURCEVIEW(user_data);
	
	gtk_source_buffer_set_highlight(GTK_SOURCE_BUFFER(sv->priv->document), !disable_highlight);
	
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
    
	gtk_source_view_set_tabs_width(GTK_SOURCE_VIEW(sv->priv->view), tab_size);
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
	
	gtk_source_buffer_set_check_brackets(GTK_SOURCE_BUFFER(sv->priv->document), 
										 braces_check);
}

static void
on_gconf_notify_view_markers (GConfClient *gclient, guint cnxn_id,
							  GConfEntry *entry, gpointer user_data)
{
	Sourceview *sv;
	gboolean show_markers = get_bool(entry);
	sv = ANJUTA_SOURCEVIEW(user_data);
	
	gtk_source_view_set_show_line_markers(GTK_SOURCE_VIEW(sv->priv->view), 
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
on_gconf_notify_color (GConfClient *gclient, guint cnxn_id,
							   GConfEntry *entry, gpointer user_data)
{
	Sourceview *sv;
	GdkColor *text, *background, *selected_text, *selection;
	AnjutaPreferences* prefs = sourceview_get_prefs();
	sv = ANJUTA_SOURCEVIEW(user_data);

	text = anjuta_util_convert_color(prefs, COLOR_TEXT);
	background = anjuta_util_convert_color(prefs, COLOR_BACKGROUND);
	selected_text = anjuta_util_convert_color(prefs, COLOR_SELECTED_TEXT);
	selection = anjuta_util_convert_color(prefs, COLOR_SELECTION);
	
	anjuta_view_set_colors(sv->priv->view, FALSE, background, text, selection, selected_text);
}

static void
on_gconf_notify_color_theme (GConfClient *gclient, guint cnxn_id,
							   GConfEntry *entry, gpointer user_data)
{
	Sourceview *sv;
	gboolean use_theme = get_bool(entry);
	sv = ANJUTA_SOURCEVIEW(user_data);
	
	if (use_theme)
	{
		anjuta_view_set_colors(sv->priv->view, TRUE, NULL, NULL, NULL, NULL);
	}
	else
		on_gconf_notify_color(NULL, 0, NULL, sv);
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
init_colors_and_fonts(Sourceview* sv)
{
	gboolean font_theme;
	gboolean color_theme;
	
	font_theme = anjuta_preferences_get_int(prefs, FONT_THEME);
	color_theme = anjuta_preferences_get_int(prefs, COLOR_THEME);
	
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
	
	if (!color_theme)
	{
		on_gconf_notify_color(NULL, 0, NULL, sv);
	}
}

static int
get_key(Sourceview* sv, const gchar* key)
{
	return anjuta_preferences_get_int (sv->priv->prefs, key);
}

void 
sourceview_prefs_init(Sourceview* sv)
{
	guint notify_id;
	
	prefs = sv->priv->prefs;
	
	/* Init */
	gtk_source_buffer_set_highlight(GTK_SOURCE_BUFFER(sv->priv->document), !get_key(sv, DISABLE_SYNTAX_HILIGHTING));
    	gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(sv->priv->view),
						   get_key(sv, HIGHLIGHT_CURRENT_LINE));
	gtk_source_view_set_tabs_width(GTK_SOURCE_VIEW(sv->priv->view), get_key(sv, TAB_SIZE));
	gtk_source_view_set_insert_spaces_instead_of_tabs(GTK_SOURCE_VIEW(sv->priv->view),
													  !get_key(sv, USE_TABS));
	gtk_source_buffer_set_check_brackets(GTK_SOURCE_BUFFER(sv->priv->document), 
										 get_key(sv, BRACES_CHECK));
	gtk_source_view_set_show_line_markers(GTK_SOURCE_VIEW(sv->priv->view), 
										 get_key(sv, VIEW_MARKER_MARGIN));
	gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(sv->priv->view), 
										 get_key(sv, VIEW_LINENUMBERS_MARGIN));
	
	init_colors_and_fonts(sv);
	
	/* Register gconf notifications */
	REGISTER_NOTIFY (TAB_SIZE, on_gconf_notify_tab_size);
	REGISTER_NOTIFY (USE_TABS, on_gconf_notify_use_tab_for_indentation);
	REGISTER_NOTIFY (DISABLE_SYNTAX_HILIGHTING, on_gconf_notify_disable_hilite);
	REGISTER_NOTIFY (HIGHLIGHT_CURRENT_LINE, on_gconf_notify_highlight_current_line);
	REGISTER_NOTIFY (BRACES_CHECK, on_gconf_notify_braces_check);
	REGISTER_NOTIFY (VIEW_MARKER_MARGIN, on_gconf_notify_view_markers);
	REGISTER_NOTIFY (VIEW_LINENUMBERS_MARGIN, on_gconf_notify_view_linenums);
	REGISTER_NOTIFY (COLOR_THEME, on_gconf_notify_color_theme);
	REGISTER_NOTIFY (COLOR_TEXT, on_gconf_notify_color);
	REGISTER_NOTIFY (COLOR_BACKGROUND, on_gconf_notify_color);
	REGISTER_NOTIFY (COLOR_SELECTED_TEXT, on_gconf_notify_color);
	REGISTER_NOTIFY (COLOR_SELECTION, on_gconf_notify_color);
	REGISTER_NOTIFY (FONT_THEME, on_gconf_notify_font_theme);
	REGISTER_NOTIFY (FONT, on_gconf_notify_font);
	
}

void sourceview_prefs_destroy(Sourceview* sv)
{
	AnjutaPreferences* prefs = sv->priv->prefs;
	GList* id;
	DEBUG_PRINT("Destroying preferences");
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
