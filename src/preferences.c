/*
 * preferences.c
 * Copyright (C) 2000  Kh. Naba Kumar Singh
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gnome.h>
#include "resources.h"
#include "preferences.h"
#include "anjuta.h"
#include "commands.h"
#include "defaults.h"

extern gchar *format_style[];
extern gchar *hilite_style[];

static void
on_preferences_reset_default_yes_clicked (GtkButton * button,
					  gpointer user_data);

Preferences *
preferences_new ()
{
	Preferences *pr;
	pr = (Preferences *) malloc (sizeof (Preferences));
	if (pr)
	{
		gchar *propdir, *propfile, *str;

		pr->props_build_in = prop_set_new ();
		pr->props_global = prop_set_new ();
		pr->props_local = prop_set_new ();
		pr->props_session = prop_set_new ();
		pr->props = prop_set_new ();

		prop_clear (pr->props_build_in);
		prop_clear (pr->props_global);
		prop_clear (pr->props_local);
		prop_clear (pr->props_session);
		prop_clear (pr->props);

		prop_set_parent (pr->props_global, pr->props_build_in);
		prop_set_parent (pr->props_local, pr->props_global);
		prop_set_parent (pr->props_session, pr->props_local);
		prop_set_parent (pr->props, pr->props_session);

		/* Reading the build in default properties */
		prop_read_from_memory (pr->props_build_in, default_settings, strlen(default_settings), "");

		/* Default paths */
		str = g_strconcat (g_getenv("HOME"), "/Projects", NULL);
		prop_set_with_key (pr->props_build_in, "projects.directory", str);
		g_free (str);
		
		str = g_strconcat (g_getenv("HOME"), "/Tarballs", NULL);
		prop_set_with_key (pr->props_build_in, "tarballs.directory", str);
		g_free (str);

		str = g_strconcat (g_getenv("HOME"), "/Rpms", NULL);
		prop_set_with_key (pr->props_build_in, "rpms.directory", str);
		g_free (str);
		
		str = g_strconcat (g_getenv("HOME"), "/Tarballs", NULL);
		prop_set_with_key (pr->props_build_in, "srpms.directory", str);
		g_free (str);
		
		str = g_strdup (g_getenv("HOME"));
		prop_set_with_key (pr->props_build_in, "anjuta.home.directory", str);
		g_free (str);
		
		prop_set_with_key (pr->props_build_in, "anjuta.data.directory", app->dirs->data);
		prop_set_with_key (pr->props_build_in, "anjuta.pixmap.directory", app->dirs->pixmaps);
	
		/* Load the external configuration files */
		propdir = g_strconcat (app->dirs->data, "/properties/", NULL);
		propfile = g_strconcat (propdir, "anjuta.properties", NULL);
		
		if (file_is_readable (propfile) == FALSE)
		{
			anjuta_error
				(_("Cannot load Global defaults and configuration files:\n"
				 "%s.\n"
				 "This may result in improper behaviour or instabilities.\n"
				 "Anjuta will fall back to built in (limited) settings"),
				 propfile);
		}
		prop_read (pr->props_global, propfile, propdir);
		g_free (propfile);
		g_free (propdir);

		propdir = g_strconcat (app->dirs->home, "/.anjuta/", NULL);
		propfile = g_strconcat (propdir, "user.properties", NULL);
		
		/* Create user.properties file, if it doesn't exist */
		if (file_is_regular (propfile) == FALSE) {
			gchar* user_propfile = g_strconcat (app->dirs->data, "/properties/user.properties", NULL);
			copy_file (user_propfile, propfile, FALSE);
			g_free (user_propfile);
		}
		prop_read (pr->props_local, propfile, propdir);
		g_free (propdir);
		g_free (propfile);

		propdir = g_strconcat (app->dirs->home, "/.anjuta/", NULL);
		propfile = g_strconcat (propdir, "session.properties", NULL);
		prop_read (pr->props_session, propfile, propdir);
		g_free (propdir);
		g_free (propfile);

		/* A quick hack to fix the 'invisible' browser toolbar */
		str = prop_get(pr->props_session, "anjuta.version");
		if (str) {
			if (strcmp(str, VERSION) != 0)
				remove("~/.gnome/Anjuta");
			g_free (str);
		} else {
			remove("~/.gnome/Anjuta");
		}
		/* quick hack ends */
		
		preferences_set_build_options (pr);
		pr->current_style = NULL;
		pr->is_showing = FALSE;
		pr->win_pos_x = 100;
		pr->win_pos_y = 80;
		pr->save_prefs = NULL;
		create_preferences_gui (pr);
	}
	return pr;
}

void
preferences_destroy (Preferences * pr)
{
	gint i;
	if (pr)
	{
		prop_set_destroy (pr->props_global);
		prop_set_destroy (pr->props_local);
		prop_set_destroy (pr->props_session);
		prop_set_destroy (pr->props);

		gtk_widget_unref (pr->widgets.window);
		gtk_widget_unref (pr->widgets.notebook);

		gtk_widget_unref (pr->widgets.prj_dir_entry);
		gtk_widget_unref (pr->widgets.recent_prj_spin);
		gtk_widget_unref (pr->widgets.recent_files_spin);
		gtk_widget_unref (pr->widgets.combo_history_spin);

		gtk_widget_unref (pr->widgets.beep_check);
		gtk_widget_unref (pr->widgets.dialog_check);
		gtk_widget_unref (pr->widgets.lastprj_check);

		gtk_widget_unref (pr->widgets.build_keep_going_check);
		gtk_widget_unref (pr->widgets.build_silent_check);
		gtk_widget_unref (pr->widgets.build_debug_check);
		gtk_widget_unref (pr->widgets.build_warn_undef_check);
		gtk_widget_unref (pr->widgets.build_jobs_spin);
		
		gtk_widget_unref (pr->widgets.auto_save_check);
		gtk_widget_unref (pr->widgets.auto_indent_check);
		gtk_widget_unref (pr->widgets.tab_size_spin);
		gtk_widget_unref (pr->widgets.autosave_timer_spin);
		gtk_widget_unref (pr->widgets.autoindent_size_spin);
		gtk_widget_unref (pr->widgets.fore_colorpicker);
		gtk_widget_unref (pr->widgets.back_colorpicker);
		gtk_widget_unref (pr->widgets.caret_fore_color);
		gtk_widget_unref (pr->widgets.calltip_back_color);
		gtk_widget_unref (pr->widgets.selection_fore_color);
		gtk_widget_unref (pr->widgets.selection_back_color);

		gtk_widget_unref (pr->widgets.strip_spaces_check);
		gtk_widget_unref (pr->widgets.fold_on_open_check);

		gtk_widget_unref (pr->widgets.paper_selector);
		gtk_widget_unref (pr->widgets.print_header_check);
		gtk_widget_unref (pr->widgets.print_wrap_check);
		gtk_widget_unref (pr->widgets.print_linenum_count_spin);
		gtk_widget_unref (pr->widgets.print_landscape_check);
		gtk_widget_unref (pr->widgets.print_color_check);
		gtk_widget_unref (pr->widgets.margin_left_spin);
		gtk_widget_unref (pr->widgets.margin_right_spin);
		gtk_widget_unref (pr->widgets.margin_top_spin);
		gtk_widget_unref (pr->widgets.margin_bottom_spin);

		gtk_widget_unref (pr->widgets.format_style_combo);
		gtk_widget_unref (pr->widgets.custom_style_entry);
		gtk_widget_unref (pr->widgets.format_disable_check);
		gtk_widget_unref (pr->widgets.format_frame1);
		gtk_widget_unref (pr->widgets.format_frame2);

		gtk_widget_unref (pr->widgets.truncat_mesg_check);
		gtk_widget_unref (pr->widgets.mesg_first_spin);
		gtk_widget_unref (pr->widgets.mesg_last_spin);
		gtk_widget_unref (pr->widgets.tags_update_check);
		gtk_widget_unref (pr->widgets.build_symbols);
		gtk_widget_unref (pr->widgets.build_file_tree);
		gtk_widget_unref (pr->widgets.show_tooltips);
		gtk_widget_unref (pr->widgets.no_tag_check);
		gtk_widget_unref (pr->widgets.auto_indicator_check);
		for (i = 0; i < 4; i++)
			gtk_widget_unref (pr->widgets.tag_pos_radio[i]);

		gtk_widget_unref (pr->widgets.use_components);

		gtk_widget_unref (pr->widgets.name_entry);
		gtk_widget_unref (pr->widgets.email_entry);
		
		gtk_widget_destroy (pr->widgets.window);
		g_free (pr);
		pr = NULL;
	}
}


static gchar *
get_from_propset (PropsID props_id, gchar * key)
{
	return prop_get (props_id, key);
}

static gint
get_int_from_propset (PropsID props_id, gchar * key)
{
	return prop_get_int (props_id, key, 0);
}

static gint
get_int_from_propset_with_default (PropsID props_id, gchar * key, gint default_value)
{
	return prop_get_int (props_id, key, default_value);
}

gchar *
preferences_get (Preferences * p, gchar * key)
{
	return get_from_propset (p->props, key);
}

gint
preferences_get_int (Preferences * p, gchar * key)
{
	return get_int_from_propset (p->props, key);
}

gint
preferences_get_int_with_default (Preferences * p, gchar * key, gint default_value)
{
	return get_int_from_propset_with_default (p->props, key, default_value);
}

gchar *
preferences_default_get (Preferences * p, gchar * key)
{
	return get_from_propset (p->props_local, key);
}

gint
preferences_default_get_int (Preferences * p, gchar * key)
{
	return get_int_from_propset (p->props_local, key);
}

void
preferences_set (Preferences * pr, gchar * key, gchar * value)
{
	prop_set_with_key (pr->props, key, value);
}

void
preferences_set_int (Preferences * pr, gchar * key, gint value)
{
  prop_set_int_with_key(pr->props, key, value);
}

void
preferences_sync (Preferences * pr)
{
	gchar *str, *str2;
	gint i;
	gint8 r, g, b;

/* Page 0 */
	str = preferences_get (pr, PROJECTS_DIRECTORY);
	gtk_entry_set_text (GTK_ENTRY (pr->widgets.prj_dir_entry), str);
	g_free (str);

	str = preferences_get (pr, TARBALLS_DIRECTORY);
	gtk_entry_set_text (GTK_ENTRY (pr->widgets.tarballs_dir_entry), str);
	g_free (str);

	str = preferences_get (pr, RPMS_DIRECTORY);
	gtk_entry_set_text (GTK_ENTRY (pr->widgets.rpms_dir_entry), str);
	g_free (str);

	str = preferences_get (pr, SRPMS_DIRECTORY);
	gtk_entry_set_text (GTK_ENTRY (pr->widgets.srpms_dir_entry), str);
	g_free (str);

	gtk_spin_button_set_value (GTK_SPIN_BUTTON
				   (pr->widgets.recent_prj_spin),
				   preferences_get_int (pr,
							MAXIMUM_RECENT_PROJECTS));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON
				   (pr->widgets.recent_files_spin),
				   preferences_get_int (pr,
							MAXIMUM_RECENT_FILES));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON
				   (pr->widgets.combo_history_spin),
				   preferences_get_int (pr,
							MAXIMUM_COMBO_HISTORY));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (pr->widgets.dialog_check),
				      preferences_get_int (pr,
							   DIALOG_ON_BUILD_COMPLETE));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (pr->widgets.beep_check),
				      preferences_get_int (pr,
							   BEEP_ON_BUILD_COMPLETE));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (pr->widgets.lastprj_check),
				      preferences_get_int (pr,
							   RELOAD_LAST_PROJECT));
/* Page 1 */
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (pr->widgets.build_keep_going_check),
				      preferences_get_int (pr, BUILD_OPTION_KEEP_GOING));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (pr->widgets.build_debug_check),
				      preferences_get_int (pr, BUILD_OPTION_DEBUG));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (pr->widgets.build_silent_check),
				      preferences_get_int (pr, BUILD_OPTION_SILENT));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (pr->widgets.build_warn_undef_check),
				      preferences_get_int (pr, BUILD_OPTION_WARN_UNDEF));

		
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (pr->widgets.build_autosave_check),
				      preferences_get_int (pr, BUILD_OPTION_AUTOSAVE));
		 
	gtk_spin_button_set_value (GTK_SPIN_BUTTON
				   (pr->widgets.build_jobs_spin),
				   preferences_get_int (pr, BUILD_OPTION_JOBS));

/* Page 2 */

	/* Never hurts to use gtk_object_*_data as temp hash buffer */
	for (i = 0;; i += 2)
	{
		StyleData *sdata;

		if (hilite_style[i] == NULL)
			break;
		str = prop_get_expanded (pr->props, hilite_style[i + 1]);
		sdata = style_data_new_parse (str);
		if (str)
			g_free (str);

		style_data_set_item (sdata, hilite_style[i]);

		gtk_object_set_data_full (GTK_OBJECT (pr->widgets.window),
					  hilite_style[i], sdata,
					  (GtkDestroyNotify)
					  style_data_destroy);
	}
	pr->default_style =
		gtk_object_get_data (GTK_OBJECT (pr->widgets.window),
				     hilite_style[0]);
	pr->current_style = NULL;

	on_hilite_style_entry_changed (GTK_EDITABLE
				       (GTK_COMBO
					(pr->widgets.hilite_item_combo)->
					entry), pr);

	str = preferences_get (pr, CARET_FORE_COLOR);
	if(str)
	{
		ColorFromString(str, &r, &g, &b);
		gnome_color_picker_set_i8 (GNOME_COLOR_PICKER(pr->widgets.caret_fore_color), r, g, b, 0);
		g_free (str);
	}
	else
	{
		gnome_color_picker_set_i8 (GNOME_COLOR_PICKER(pr->widgets.caret_fore_color), 0, 0, 0, 0);
	}

	str = preferences_get (pr, CALLTIP_BACK_COLOR);
	if(str)
	{
		ColorFromString(str, &r, &g, &b);
		gnome_color_picker_set_i8 (GNOME_COLOR_PICKER(pr->widgets.calltip_back_color), r, g, b, 0);
		g_free (str);
	}
	else
	{
		ColorFromString("#E6D6B6", &r, &g, &b);
		gnome_color_picker_set_i8 (GNOME_COLOR_PICKER(pr->widgets.calltip_back_color), r, g, b, 0);
	}
	
	str = preferences_get (pr, SELECTION_FORE_COLOR);
	if(str)
	{
		ColorFromString(str, &r, &g, &b);
		gnome_color_picker_set_i8 (GNOME_COLOR_PICKER(pr->widgets.selection_fore_color), r, g, b, 0);
		g_free (str);
	}
	else
	{
		ColorFromString("#FFFFFF", &r, &g, &b);
		gnome_color_picker_set_i8 (GNOME_COLOR_PICKER(pr->widgets.selection_fore_color), r, g, b, 0);
	}
	
	str = preferences_get (pr, SELECTION_BACK_COLOR);
	if(str)
	{
		ColorFromString(str, &r, &g, &b);
		gnome_color_picker_set_i8 (GNOME_COLOR_PICKER(pr->widgets.selection_back_color), r, g, b, 0);
		g_free (str);
	}
	else
	{
		gnome_color_picker_set_i8 (GNOME_COLOR_PICKER(pr->widgets.selection_back_color), 0, 0, 0, 0);
	}

/* Page 3 */
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (pr->widgets.strip_spaces_check),
				      preferences_get_int (pr, STRIP_TRAILING_SPACES));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (pr->widgets.fold_on_open_check),
				      preferences_get_int (pr, FOLD_ON_OPEN));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (pr->widgets.disable_hilite_check),
				      preferences_get_int (pr,
							   DISABLE_SYNTAX_HILIGHTING));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (pr->widgets.auto_save_check),
				      preferences_get_int (pr,
							   SAVE_AUTOMATIC));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (pr->widgets.auto_indent_check),
				      preferences_get_int (pr,
							   INDENT_AUTOMATIC));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (pr->widgets.use_tabs_check),
				      preferences_get_int (pr, USE_TABS));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (pr->widgets.braces_check_check),
				      preferences_get_int (pr, BRACES_CHECK));
				      
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					(pr->widgets.dos_eol_check),
					preferences_get_int (pr, DOS_EOL_CHECK));
					
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					(pr->widgets.wrap_bookmarks_check),
					preferences_get_int (pr, WRAP_BOOKMARKS));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					(pr->widgets.indent_open_brace),
					preferences_get_int (pr, INDENT_OPENING));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					(pr->widgets.indent_close_brace),
					preferences_get_int (pr, INDENT_CLOSING));

	gtk_spin_button_set_value (GTK_SPIN_BUTTON
				   (pr->widgets.tab_size_spin),
				   preferences_get_int (pr, TAB_SIZE));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON
				   (pr->widgets.autoindent_size_spin),
				   preferences_get_int (pr, INDENT_SIZE));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON
				   (pr->widgets.autosave_timer_spin),
				   preferences_get_int (pr, AUTOSAVE_TIMER));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON
				   (pr->widgets.linenum_margin_width_spin),
				   preferences_get_int (pr,
							MARGIN_LINENUMBER_WIDTH));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON
				   (pr->widgets.session_timer_spin),
				   preferences_get_int (pr,
							SAVE_SESSION_TIMER));

/* Page 4 */
	str = preferences_get(pr, PRINT_PAPER_SIZE);
	if (NULL == str)
		str = g_strdup(gnome_paper_name_default());
	gtk_entry_set_text (GTK_ENTRY (pr->widgets.paper_selector), str);
	g_free (str);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					(pr->widgets.print_header_check),
					preferences_get_int_with_default (pr, PRINT_HEADER, 1));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					(pr->widgets.print_wrap_check),
					preferences_get_int_with_default (pr, PRINT_WRAP, 1));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON
				   (pr->widgets.print_linenum_count_spin),
				   preferences_get_int_with_default (pr, PRINT_LINENUM_COUNT, 1));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					(pr->widgets.print_landscape_check),
					preferences_get_int_with_default (pr, PRINT_LANDSCAPE, 0));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					(pr->widgets.print_color_check),
					preferences_get_int_with_default (pr, PRINT_COLOR, 1));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON
				   (pr->widgets.margin_left_spin),
				   preferences_get_int_with_default (pr, PRINT_MARGIN_LEFT, 54));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON
				   (pr->widgets.margin_right_spin),
				   preferences_get_int_with_default (pr, PRINT_MARGIN_RIGHT, 54));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON
				   (pr->widgets.margin_top_spin),
				   preferences_get_int_with_default (pr, PRINT_MARGIN_TOP, 54));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON
				   (pr->widgets.margin_bottom_spin),
				   preferences_get_int_with_default (pr, PRINT_MARGIN_BOTTOM, 54));

/* Page 5 */
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (pr->widgets.format_disable_check),
				      preferences_get_int (pr,
							   AUTOFORMAT_DISABLE));

	str = preferences_get (pr, AUTOFORMAT_CUSTOM_STYLE);
	gtk_entry_set_text (GTK_ENTRY (pr->widgets.custom_style_entry), str);
	g_free (str);

	str = preferences_get (pr, AUTOFORMAT_STYLE);
	gtk_entry_set_text (GTK_ENTRY
			    (GTK_COMBO (pr->widgets.format_style_combo)->
			     entry), str);
	g_free (str);

/* Page 6 */
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (pr->widgets.truncat_mesg_check),
				      preferences_get_int (pr,
							   TRUNCAT_MESSAGES));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON
				   (pr->widgets.mesg_first_spin),
				   preferences_get_int (pr,
							TRUNCAT_MESG_FIRST));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON
				   (pr->widgets.mesg_last_spin),
				   preferences_get_int (pr,
							TRUNCAT_MESG_LAST));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (pr->widgets.auto_indicator_check),
				      preferences_get_int (pr,
							   MESSAGES_INDICATORS_AUTOMATIC));
	
	str = preferences_get (pr, MESSAGES_TAG_POS);
	if (str != NULL)
	{
		if (strcmp (str, "top") == 0)
		{
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
							  (pr->widgets.tag_pos_msg_radio[0]),
							  TRUE);
		}
		else if (strcmp (str, "left") == 0)
		{
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
							  (pr->widgets.tag_pos_msg_radio[2]),
							  TRUE);
		}
		else if (strcmp (str, "right") == 0)
		{
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
							  (pr->widgets.tag_pos_msg_radio[3]),
							  TRUE);
		}
		else
		{
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
							  (pr->widgets.tag_pos_msg_radio[1]),
							  TRUE);
		}
		g_free (str);
	}
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
							(pr->widgets.tag_pos_msg_radio[1]), TRUE);

	str = preferences_get(pr, MESSAGES_COLOR_ERROR);
	if (str)
	{
		ColorFromString(str, &r, &g, &b);
		gnome_color_picker_set_i8(GNOME_COLOR_PICKER(pr->widgets.color_picker[0]), r, g, b, 0);
		g_free(str);
	}
	else
		gnome_color_picker_set_i8(GNOME_COLOR_PICKER(pr->widgets.color_picker[0]), 255, 0, 0, 0);
	
		str = preferences_get(pr, MESSAGES_COLOR_WARNING);
	if (str)
	{
		ColorFromString(str, &r, &g, &b);
		gnome_color_picker_set_i8(GNOME_COLOR_PICKER(pr->widgets.color_picker[1]), r, g, b, 0);
		g_free(str);	
	}
	else
		gnome_color_picker_set_i8(GNOME_COLOR_PICKER(pr->widgets.color_picker[1]), 0, 127, 0, 0);

	str = preferences_get(pr, MESSAGES_COLOR_MESSAGES1);
	if (str)
	{
		ColorFromString(str, &r, &g, &b);
		gnome_color_picker_set_i8(GNOME_COLOR_PICKER(pr->widgets.color_picker[2]), r, g, b, 0);
		g_free(str);
	}
	else
		gnome_color_picker_set_i8(GNOME_COLOR_PICKER(pr->widgets.color_picker[2]), 0, 0, 255, 0);

	str = preferences_get(pr, MESSAGES_COLOR_MESSAGES2);
	if (str)
	{
		ColorFromString(str, &r, &g, &b);
		gnome_color_picker_set_i8(GNOME_COLOR_PICKER(pr->widgets.color_picker[3]), r, g, b, 0);
		g_free(str);
	}
	else
		gnome_color_picker_set_i8(GNOME_COLOR_PICKER(pr->widgets.color_picker[3]), 0, 0, 0, 0);

	
/* Page 7 */
	str = preferences_get (pr, EDITOR_TAG_POS);
	if (strcmp (str, "bottom") == 0)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (pr->widgets.tag_pos_radio[1]),
					      TRUE);
	}
	else if (strcmp (str, "left") == 0)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (pr->widgets.tag_pos_radio[2]),
					      TRUE);
	}
	else if (strcmp (str, "right") == 0)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (pr->widgets.tag_pos_radio[3]),
					      TRUE);
	}
	else
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (pr->widgets.tag_pos_radio[0]),
					      TRUE);
	}
	g_free (str);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (pr->widgets.no_tag_check),
				      preferences_get_int (pr,
							   EDITOR_TAG_HIDE));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pr->widgets.tabs_ordering), preferences_get_int (pr, EDITOR_TABS_ORDERING));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (pr->widgets.tags_update_check),
				      preferences_get_int (pr,
							   AUTOMATIC_TAGS_UPDATE));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (pr->widgets.build_symbols),
				      preferences_get_int (pr,
							   BUILD_SYMBOL_BROWSER));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (pr->widgets.build_file_tree),
				      preferences_get_int (pr,
							   BUILD_FILE_BROWSER));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (pr->widgets.show_tooltips),
				      preferences_get_int (pr,
							   SHOW_TOOLTIPS));

	/* Page CVS */
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pr->widgets.option_force_update),
			cvs_get_force_update (app->cvs));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pr->widgets.option_unified),
			cvs_get_unified_diff (app->cvs));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pr->widgets.option_context),
			cvs_get_context_diff (app->cvs));
	
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (pr->widgets.spin_compression),
			cvs_get_compression (app->cvs));
#ifdef USE_GLADEN	
	/* Page Components */
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (pr->widgets.use_components),
				      preferences_get_int (pr,
							   USE_COMPONENTS));
#else /* USE_GLADEN */
	/* Page Components */
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (pr->widgets.use_components), FALSE);
#endif /* USE_GLADEN */
	/* Page Identification */
	str = preferences_get (pr, IDENT_NAME);
	if (!str) {
		gchar *uname;
		uname = getenv("USERNAME");
		if (!uname)
			uname = getenv("USER");
		if (!uname)
			uname = "Unknown";
		str = g_strdup (uname);
	}
	gtk_entry_set_text (GTK_ENTRY (pr->widgets.name_entry), str);
	g_free(str);
	
	str2 = preferences_get (pr, IDENT_EMAIL);
	if (!str2)
	{
		str2 = getenv("HOSTNAME");
		if (!str2)
			str2 = "hostname.org";
		str = getenv("USERNAME");
		if (!str)
			str = getenv("USER");
		if (!str)
			str = "user";
		str2 = g_strconcat(str, "@", str2, NULL);
	}
	gtk_entry_set_text (GTK_ENTRY (pr->widgets.email_entry), str2);
	g_free(str2);

	/* Terminal */
	if (NULL == (str = preferences_get(pr, TERMINAL_FONT)))
		str = g_strdup(DEFAULT_ZVT_FONT);
	gnome_font_picker_set_font_name( (GnomeFontPicker *)
	  pr->widgets.term_font_fp, str);
	g_free(str);
	if (NULL == (str = preferences_get(pr, TERMINAL_WORDCLASS)))
		str = g_strdup(DEFAULT_ZVT_WORDCLASS);
	gtk_entry_set_text((GtkEntry *) pr->widgets.term_sel_by_word_e, str);
	g_free(str);
	i = preferences_get_int(pr, TERMINAL_SCROLLSIZE);
	if (i < DEFAULT_ZVT_SCROLLSIZE)
		i = DEFAULT_ZVT_SCROLLSIZE;
	else if (i > MAX_ZVT_SCROLLSIZE)
		i = MAX_ZVT_SCROLLSIZE;
	{
		float f = i;
		gtk_spin_button_set_value((GtkSpinButton *)
		 pr->widgets.term_scroll_buffer_sb, f);
	}
	if (NULL == (str = preferences_get(pr, TERMINAL_TERM)))
		str = g_strdup(DEFAULT_ZVT_TERM);
	gtk_entry_set_text((GtkEntry *) pr->widgets.term_type_e, str);
	g_free(str);
	i = preferences_get_int(pr, TERMINAL_SCROLL_KEY);
	gtk_toggle_button_set_active((GtkToggleButton *)
	  pr->widgets.term_scroll_on_key_cb, i ? TRUE : FALSE);
	i = preferences_get_int(pr, TERMINAL_SCROLL_OUTPUT);
	gtk_toggle_button_set_active((GtkToggleButton *)
	  pr->widgets.term_scroll_on_out_cb, i ? TRUE : FALSE);
	i = preferences_get_int(pr, TERMINAL_BLINK);
	gtk_toggle_button_set_active((GtkToggleButton *)
	  pr->widgets.term_blink_cursor_cb, i ? TRUE : FALSE);
	i = preferences_get_int(pr, TERMINAL_BELL);
	gtk_toggle_button_set_active((GtkToggleButton *)
	  pr->widgets.term_bell_cb, i ? TRUE : FALSE);
}

void
preferences_show (Preferences * pr)
{
	if (!pr)
		return;

	if (pr->is_showing)
	{
		gdk_window_raise (pr->widgets.window->window);
		return;
	}
	preferences_sync (pr);
	gtk_widget_set_uposition (pr->widgets.window, pr->win_pos_x,
				  pr->win_pos_y);
	gtk_widget_show (pr->widgets.window);
	pr->is_showing = TRUE;
}

void
preferences_reset_defaults (Preferences * pr)
{
	messagebox2 (GNOME_MESSAGE_BOX_QUESTION,
		     _("Are you sure you want to reset the preferences to\n"
		       "their default settings?"),
		     GNOME_STOCK_BUTTON_YES, GNOME_STOCK_BUTTON_NO,
		     GTK_SIGNAL_FUNC
		     (on_preferences_reset_default_yes_clicked), NULL, pr);
}

void
preferences_hide (Preferences * pr)
{
	if (!pr)
		return;
	if (pr->is_showing == FALSE)
		return;
	gdk_window_get_root_origin (pr->widgets.window->window,
				    &pr->win_pos_x, &pr->win_pos_y);
	pr->is_showing = FALSE;
}

gboolean preferences_save_yourself (Preferences * pr, FILE * fp)
{
	gchar *str;
	gint i;

	/* Page 0 */
	str = preferences_get (pr, PROJECTS_DIRECTORY);
	fprintf (fp, "%s=%s\n", PROJECTS_DIRECTORY, str);
	g_free (str);

	str = preferences_get (pr, TARBALLS_DIRECTORY);
	fprintf (fp, "%s=%s\n", TARBALLS_DIRECTORY, str);
	g_free (str);

	str = preferences_get (pr, RPMS_DIRECTORY);
	fprintf (fp, "%s=%s\n", RPMS_DIRECTORY, str);
	g_free (str);

	str = preferences_get (pr, SRPMS_DIRECTORY);
	fprintf (fp, "%s=%s\n", SRPMS_DIRECTORY, str);
	g_free (str);

	fprintf (fp, "%s=%d\n", MAXIMUM_RECENT_PROJECTS,
		 preferences_get_int (pr, MAXIMUM_RECENT_PROJECTS));
	fprintf (fp, "%s=%d\n", MAXIMUM_RECENT_FILES,
		 preferences_get_int (pr, MAXIMUM_RECENT_FILES));
	fprintf (fp, "%s=%d\n", MAXIMUM_COMBO_HISTORY,
		 preferences_get_int (pr, MAXIMUM_COMBO_HISTORY));

	fprintf (fp, "%s=%d\n", DIALOG_ON_BUILD_COMPLETE,
		 preferences_get_int (pr, DIALOG_ON_BUILD_COMPLETE));
	fprintf (fp, "%s=%d\n", BEEP_ON_BUILD_COMPLETE,
		 preferences_get_int (pr, BEEP_ON_BUILD_COMPLETE));
	fprintf (fp, "%s=%d\n", RELOAD_LAST_PROJECT,
		 preferences_get_int (pr, RELOAD_LAST_PROJECT));

	/* Page 1 */
	fprintf (fp, "%s=%d\n", BUILD_OPTION_KEEP_GOING,
		 preferences_get_int (pr, BUILD_OPTION_KEEP_GOING));

	fprintf (fp, "%s=%d\n", BUILD_OPTION_DEBUG,
		 preferences_get_int (pr, BUILD_OPTION_DEBUG));

	fprintf (fp, "%s=%d\n", BUILD_OPTION_SILENT,
		 preferences_get_int (pr, BUILD_OPTION_SILENT));

	fprintf (fp, "%s=%d\n", BUILD_OPTION_WARN_UNDEF,
		 preferences_get_int (pr, BUILD_OPTION_WARN_UNDEF));

	fprintf (fp, "%s=%d\n", BUILD_OPTION_JOBS,
		 preferences_get_int (pr, BUILD_OPTION_JOBS));
	fprintf (fp, "%s=%d\n", BUILD_OPTION_AUTOSAVE,
		 preferences_get_int (pr, BUILD_OPTION_AUTOSAVE));
	
	/* Page 2 */
	for (i = 0;; i += 2)
	{
		if (hilite_style[i] == NULL)
			break;
		str = preferences_get (pr, hilite_style[i + 1]);
		if (str)
		{
			fprintf (fp, "%s=%s\n", hilite_style[i + 1], str);
			if (str)
				g_free (str);
		}
		else
			fprintf (fp, "%s=\n", hilite_style[i + 1]);
	}
	str = preferences_get (pr, CARET_FORE_COLOR);
	if(str)
	{
		fprintf (fp, "%s=%s\n", CARET_FORE_COLOR, str);
		g_free (str);
	}

	str = preferences_get (pr, CALLTIP_BACK_COLOR);
	if(str)
	{
		fprintf (fp, "%s=%s\n", CALLTIP_BACK_COLOR, str);
		g_free (str);
	}
	
	str = preferences_get (pr, SELECTION_FORE_COLOR);
	if(str)
	{
		fprintf (fp, "%s=%s\n", SELECTION_FORE_COLOR, str);
		g_free (str);
	}
	
	str = preferences_get (pr, SELECTION_BACK_COLOR);
	if(str)
	{
		fprintf (fp, "%s=%s\n", SELECTION_BACK_COLOR, str);
		g_free (str);
	}
	
	/* Page 3 */
	/* Save the settings from session if a project is open */
	/* Otherwise save the current settings */
	if (app->project_dbase->project_is_open)
	{
		fprintf (fp, "%s=%d\n", INDENT_AUTOMATIC,
				 prop_get_int (pr->props_session, INDENT_AUTOMATIC, 1));
		fprintf (fp, "%s=%d\n", USE_TABS,
				 prop_get_int (pr->props_session, USE_TABS, 1));
		fprintf (fp, "%s=%d\n", INDENT_OPENING,
				 prop_get_int (pr->props_session, INDENT_OPENING, 1));
		fprintf (fp, "%s=%d\n", INDENT_CLOSING,
				 prop_get_int (pr->props_session, INDENT_CLOSING, 1));
		fprintf (fp, "%s=%d\n", TAB_SIZE,
				 prop_get_int (pr->props_session, TAB_SIZE, 4));
		fprintf (fp, "%s=%d\n", INDENT_SIZE,
				 prop_get_int (pr->props_session, INDENT_SIZE, 4));
	}
	else
	{
		fprintf (fp, "%s=%d\n", INDENT_AUTOMATIC,
			 preferences_get_int (pr, INDENT_AUTOMATIC));
		fprintf (fp, "%s=%d\n", USE_TABS, preferences_get_int (pr, USE_TABS));
		fprintf (fp, "%s=%d\n", INDENT_OPENING,
			 preferences_get_int (pr, INDENT_OPENING));
		fprintf (fp, "%s=%d\n", INDENT_CLOSING,
			 preferences_get_int (pr, INDENT_CLOSING));
		fprintf (fp, "%s=%d\n", TAB_SIZE, preferences_get_int (pr, TAB_SIZE));
		fprintf (fp, "%s=%d\n", INDENT_SIZE,
			 preferences_get_int (pr, INDENT_SIZE));
	}
	
	fprintf (fp, "%s=%d\n", BRACES_CHECK,
		 preferences_get_int (pr, BRACES_CHECK));
	fprintf (fp, "%s=%d\n", STRIP_TRAILING_SPACES,
		 preferences_get_int (pr, STRIP_TRAILING_SPACES));
	fprintf (fp, "%s=%d\n", FOLD_ON_OPEN,
		 preferences_get_int (pr, FOLD_ON_OPEN));
	fprintf (fp, "%s=%d\n", DISABLE_SYNTAX_HILIGHTING,
		 preferences_get_int (pr, DISABLE_SYNTAX_HILIGHTING));
	fprintf (fp, "%s=%d\n", SAVE_AUTOMATIC,
		 preferences_get_int (pr, SAVE_AUTOMATIC));
	fprintf (fp, "%s=%d\n", DOS_EOL_CHECK, preferences_get_int(pr, DOS_EOL_CHECK));
	fprintf (fp, "%s=%d\n", WRAP_BOOKMARKS, preferences_get_int(pr, WRAP_BOOKMARKS));

	fprintf (fp, "%s=%d\n", AUTOSAVE_TIMER,
		 preferences_get_int (pr, AUTOSAVE_TIMER));
	fprintf (fp, "%s=%d\n", MARGIN_LINENUMBER_WIDTH,
		 preferences_get_int (pr, MARGIN_LINENUMBER_WIDTH));
	fprintf (fp, "%s=%d\n", SAVE_SESSION_TIMER,
		 preferences_get_int (pr, SAVE_SESSION_TIMER));

	/* Page 4 */
	str = preferences_get(pr, PRINT_PAPER_SIZE);
	fprintf(fp, "%s=%s\n", PRINT_PAPER_SIZE, str);
	g_free (str);
	fprintf (fp, "%s=%d\n", PRINT_HEADER,
		 preferences_get_int_with_default (pr, PRINT_HEADER, 1));
	fprintf (fp, "%s=%d\n", PRINT_WRAP,
		 preferences_get_int_with_default (pr, PRINT_WRAP, 1));
	fprintf (fp, "%s=%d\n", PRINT_COLOR,
		 preferences_get_int_with_default (pr, PRINT_COLOR, 1));
	fprintf (fp, "%s=%d\n", PRINT_LINENUM_COUNT,
		 preferences_get_int_with_default (pr, PRINT_LINENUM_COUNT, 1));
	fprintf (fp, "%s=%d\n", PRINT_LANDSCAPE,
		 preferences_get_int_with_default (pr, PRINT_LANDSCAPE, 0));
	fprintf (fp, "%s=%d\n", PRINT_MARGIN_LEFT,
		 preferences_get_int_with_default (pr, PRINT_MARGIN_LEFT, 54));
	fprintf (fp, "%s=%d\n", PRINT_MARGIN_RIGHT,
		 preferences_get_int_with_default (pr, PRINT_MARGIN_RIGHT, 54));
	fprintf (fp, "%s=%d\n", PRINT_MARGIN_TOP,
		 preferences_get_int_with_default (pr, PRINT_MARGIN_TOP, 54));
	fprintf (fp, "%s=%d\n", PRINT_MARGIN_BOTTOM,
		 preferences_get_int_with_default (pr, PRINT_MARGIN_BOTTOM, 54));

	/* Page 5 */
	/* Save the settings from session if a project is open */
	/* Otherwise save the current settings */
	if (app->project_dbase->project_is_open)
	{
		fprintf (fp, "%s=%d\n", AUTOFORMAT_DISABLE,
				 prop_get_int (pr->props_session, AUTOFORMAT_DISABLE, 0));
	
		str = prop_get (pr->props_session, AUTOFORMAT_CUSTOM_STYLE);
		if (str)
			fprintf (fp, "%s=%s\n", AUTOFORMAT_CUSTOM_STYLE, str);
		g_free (str);
	
		str = prop_get (pr->props_session, AUTOFORMAT_STYLE);
		if (str)
			fprintf (fp, "%s=%s\n", AUTOFORMAT_STYLE, str);
		g_free (str);
	}
	else
	{
		fprintf (fp, "%s=%d\n", AUTOFORMAT_DISABLE,
			 preferences_get_int (pr, AUTOFORMAT_DISABLE));
	
		str = preferences_get (pr, AUTOFORMAT_CUSTOM_STYLE);
		fprintf (fp, "%s=%s\n", AUTOFORMAT_CUSTOM_STYLE, str);
		g_free (str);
	
		str = preferences_get (pr, AUTOFORMAT_STYLE);
		fprintf (fp, "%s=%s\n", AUTOFORMAT_STYLE, str);
		g_free (str);
	}
	
	/* Page 6 */
	fprintf (fp, "%s=%d\n", TRUNCAT_MESSAGES,
		 preferences_get_int (pr, TRUNCAT_MESSAGES));
	fprintf (fp, "%s=%d\n", TRUNCAT_MESG_FIRST,
		 preferences_get_int (pr, TRUNCAT_MESG_FIRST));
	fprintf (fp, "%s=%d\n", TRUNCAT_MESG_LAST,
		 preferences_get_int (pr, TRUNCAT_MESG_LAST));
	
	str = preferences_get (pr, MESSAGES_TAG_POS);
	if (str) {
		fprintf (fp, "%s=%s\n", MESSAGES_TAG_POS, str);
		g_free (str);
	}

	str = preferences_get (pr, MESSAGES_COLOR_ERROR);
	fprintf (fp, "%s=%s\n", MESSAGES_COLOR_ERROR, str);
	g_free(str);
	
	str = preferences_get (pr, MESSAGES_COLOR_WARNING);
	fprintf (fp, "%s=%s\n", MESSAGES_COLOR_WARNING, str);
	g_free(str);

	
	str = preferences_get (pr, MESSAGES_COLOR_MESSAGES1);
	fprintf (fp, "%s=%s\n", MESSAGES_COLOR_MESSAGES1, str);
	g_free(str);

	str = preferences_get (pr, MESSAGES_COLOR_MESSAGES2);
	fprintf (fp, "%s=%s\n", MESSAGES_COLOR_MESSAGES2, str);
	g_free(str);

	fprintf (fp, "%s=%d\n", MESSAGES_INDICATORS_AUTOMATIC,
		 preferences_get_int (pr, MESSAGES_INDICATORS_AUTOMATIC));
	
	/* Page 7 */
	str = preferences_get (pr, EDITOR_TAG_POS);
	fprintf (fp, "%s=%s\n", EDITOR_TAG_POS, str);
	g_free (str);

	fprintf (fp, "%s=%d\n", EDITOR_TAG_HIDE,
		 preferences_get_int (pr, EDITOR_TAG_HIDE));
	fprintf (fp, "%s=%d\n", EDITOR_TABS_ORDERING,
		 preferences_get_int (pr, EDITOR_TABS_ORDERING));
	fprintf (fp, "%s=%d\n", AUTOMATIC_TAGS_UPDATE,
		 preferences_get_int (pr, AUTOMATIC_TAGS_UPDATE));
	fprintf (fp, "%s=%d\n", BUILD_SYMBOL_BROWSER,
		 preferences_get_int (pr, BUILD_SYMBOL_BROWSER));
	fprintf (fp, "%s=%d\n", BUILD_FILE_BROWSER,
		 preferences_get_int (pr, BUILD_FILE_BROWSER));
	fprintf (fp, "%s=%d\n", SHOW_TOOLTIPS,
		 preferences_get_int (pr, SHOW_TOOLTIPS));
#ifdef USE_GLADEN
	fprintf (fp, "%s=%d\n", USE_COMPONENTS,
		 preferences_get_int (pr, USE_COMPONENTS));
#else /* USE_GLADEN */
	fprintf (fp, "%s=%d\n", USE_COMPONENTS, 0);
#endif
	/* Miscellaneous */
	str = preferences_get(pr, CHARACTER_SET);
	if (str)
	{
		fprintf(fp, "%s=%s\n", CHARACTER_SET, str);
		g_free(str);
	}

	/* Terminal */
	if (NULL != (str = preferences_get(pr, TERMINAL_FONT)))
	{
		fprintf(fp, "%s=%s\n", TERMINAL_FONT, str);
		g_free(str);
	}
	if (NULL != (str = preferences_get(pr, TERMINAL_WORDCLASS)))
	{
		fprintf(fp, "%s=%s\n", TERMINAL_WORDCLASS, str);
		g_free(str);
	}
	if (NULL != (str = preferences_get(pr, TERMINAL_TERM)))
	{
		fprintf(fp, "%s=%s\n", TERMINAL_TERM, str);
		g_free(str);
	}
	fprintf(fp, "%s=%d\n", TERMINAL_BELL
	  , preferences_get_int(pr, TERMINAL_BELL));
	fprintf(fp, "%s=%d\n", TERMINAL_BLINK
	  , preferences_get_int(pr, TERMINAL_BLINK));
	fprintf(fp, "%s=%d\n", TERMINAL_SCROLLSIZE
	  , preferences_get_int_with_default(pr, TERMINAL_SCROLLSIZE
	  , DEFAULT_ZVT_SCROLLSIZE));
	fprintf(fp, "%s=%d\n", TERMINAL_SCROLL_KEY
	  , preferences_get_int_with_default(pr, TERMINAL_SCROLL_KEY, 1));
	fprintf(fp, "%s=%d\n", TERMINAL_SCROLL_OUTPUT
	  , preferences_get_int_with_default(pr, TERMINAL_SCROLL_OUTPUT, 1));

	if (pr->is_showing)
	{
		gdk_window_get_root_origin (pr->widgets.window->window,
					    &pr->win_pos_x, &pr->win_pos_y);
	}
	fprintf (fp, "preferences.win.pos.x=%d\n", pr->win_pos_x);
	fprintf (fp, "preferences.win.pos.y=%d\n", pr->win_pos_y);
	fprintf (fp, "%s=%d\n", TEXT_ZOOM_FACTOR, preferences_get_int(pr, TEXT_ZOOM_FACTOR));

	/* Identification */
	str = preferences_get (pr, IDENT_NAME);
	fprintf (fp, "%s=", IDENT_NAME);
	if (str)
	{
		fprintf (fp, "%s", str);
		g_free (str);
	}
	
	str = preferences_get (pr, IDENT_EMAIL);
	fprintf (fp, "\n%s=", IDENT_EMAIL);
	if (str)
	{
		fprintf (fp, "%s\n", str);
		g_free (str);
	}

	return TRUE;
}

gboolean preferences_load_yourself (Preferences * pr, PropsID props)
{
	pr->win_pos_x = prop_get_int (props, "preferences.win.pos.x", 100);
	pr->win_pos_y = prop_get_int (props, "preferences.win.pos.y", 80);
	return TRUE;
}

static void
on_preferences_reset_default_yes_clicked (GtkButton * button,
					  gpointer user_data)
{
	Preferences *pr = (Preferences *) user_data;
	if (!pr)
		return;
	prop_clear (pr->props_session);
	prop_clear (pr->props);
}

/* Return must be freed */
gchar *
preferences_get_format_opts (Preferences * p)
{
	int i;
	gchar *style, *custom;

	style = prop_get (p->props, "autoformat.style");
	if (!style)
		style = g_strdup ("Style of Kangleipak");

	custom = prop_get (p->props, "autoformat.custom.style");
	if (!custom)
		custom = g_strdup ("");

	if (strcmp (format_style[0], style) == 0)
	{
		g_free (style);
		return custom;
	}
	for (i = 0;; i += 2)
	{
		if (format_style[i] == NULL)
		{
			g_free (style);
			return custom;
		}
		if (strcmp (format_style[i], style) == 0)
		{
			g_free (style);
			g_free (custom);
			return g_strdup (format_style[i + 1]);
		}
	}

	/* Yes, this won't reach */
	g_free (style);
	return custom;
}

void
preferences_set_build_options(Preferences* p)
{
	gint x;
	gchar *str, *tmp;
	
	str = g_strdup ("");
	g_return_if_fail (p != NULL);
	
	x = preferences_get_int (p, BUILD_OPTION_KEEP_GOING);
	if (x)
	{
		tmp = str;
		str = g_strconcat (tmp, "--keep-going ", NULL);
		g_free (tmp);
	}
	else
	{
		tmp = str;
		str = g_strconcat (tmp, "--no-keep-going ", NULL);
		g_free (tmp);
	}

	/* 
	This produces too much output. Anjuta finds it
	hard to manage it
	*/
#if 0
	x = preferences_get_int (p, BUILD_OPTION_DEBUG);
	if (x)
	{
		tmp = str;
		str = g_strconcat (tmp, "--debug ", NULL);
	}
#endif
	
	x = preferences_get_int (p, BUILD_OPTION_SILENT);
	if (x)
	{
		tmp = str;
		str = g_strconcat (tmp, "--silent ", NULL);
		g_free (tmp);
	}

	x = preferences_get_int (p, BUILD_OPTION_WARN_UNDEF);
	if (x)
	{
		tmp = str;
		str = g_strconcat (tmp, "--warn-undefined-variables ", NULL);
		g_free (tmp);
	}

	x = preferences_get_int (p, BUILD_OPTION_JOBS);
	if (x)
	{
		tmp = str;
		str = g_strdup_printf ("%s --jobs=%d", tmp, x);
		g_free (tmp);
	}
	prop_set_with_key (p->props, "anjuta.make.options", str);
	g_free (str);
}
