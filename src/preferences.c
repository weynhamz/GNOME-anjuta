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
		gchar *propdir, *propfile, *internfile, *str;

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
		propdir = g_strconcat (app->dirs->data, "/", NULL);
		internfile =
			g_strconcat (propdir, "/internal.properties", NULL);
		propfile = g_strconcat (propdir, "/anjuta.properties", NULL);
		if (file_is_readable (internfile) == FALSE
		    || file_is_readable (propfile) == FALSE)
		{
			anjuta_error
				("Cannot load Global defaults and internal configuration files:\n"
				 "%s.\n%s.\n"
				 "This may result in improper behaviour or instabilities.\n"
				 "Anjuta will fall back to build in (limited) settings",
				 propfile, internfile);
		}
		prop_read (pr->props_global, propfile, propdir);
		g_free (internfile);
		g_free (propfile);
		g_free (propdir);

		propdir = g_strconcat (app->dirs->home, "/.anjuta", NULL);
		propfile = g_strconcat (propdir, "/users.properties", NULL);
		prop_read (pr->props_local, propfile, propdir);
		g_free (propdir);
		g_free (propfile);

		propdir = g_strconcat (app->dirs->home, "/.anjuta", NULL);
		propfile = g_strconcat (propdir, "/session.properties", NULL);
		prop_read (pr->props_session, propfile, propdir);
		g_free (propdir);
		g_free (propfile);

		preferences_set_build_options (pr);
		pr->current_style = NULL;
		pr->is_showing = FALSE;
		pr->win_pos_x = 100;
		pr->win_pos_y = 80;
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

		gtk_widget_unref (pr->widgets.paperselector);
		gtk_widget_unref (pr->widgets.pr_command_combo);
		gtk_widget_unref (pr->widgets.pr_command_entry);

		gtk_widget_unref (pr->widgets.format_style_combo);
		gtk_widget_unref (pr->widgets.custom_style_entry);
		gtk_widget_unref (pr->widgets.format_disable_check);
		gtk_widget_unref (pr->widgets.format_frame1);
		gtk_widget_unref (pr->widgets.format_frame2);

		gtk_widget_unref (pr->widgets.truncat_mesg_check);
		gtk_widget_unref (pr->widgets.mesg_first_spin);
		gtk_widget_unref (pr->widgets.mesg_last_spin);
		gtk_widget_unref (pr->widgets.tags_update_check);
		gtk_widget_unref (pr->widgets.no_tag_check);
		for (i = 0; i < 4; i++)
			gtk_widget_unref (pr->widgets.tag_pos_radio[i]);

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
	gchar *str;
	str = g_strdup_printf ("%d", value);
	prop_set_with_key (pr->props, key, str);
	g_free (str);
}

void
preferences_sync (Preferences * pr)
{
	gchar *str;
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
	str = preferences_get (pr, COMMAND_PRINT);
	gtk_entry_set_text (GTK_ENTRY (pr->widgets.pr_command_entry), str);
	g_free (str);

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
				      (pr->widgets.tags_update_check),
				      preferences_get_int (pr,
							   AUTOMATIC_TAGS_UPDATE));
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
	gtk_widget_hide (pr->widgets.window);
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
	fprintf (fp, "%s=%d\n", STRIP_TRAILING_SPACES,
		 preferences_get_int (pr, STRIP_TRAILING_SPACES));

	fprintf (fp, "%s=%d\n", FOLD_ON_OPEN,
		 preferences_get_int (pr, FOLD_ON_OPEN));
	fprintf (fp, "%s=%d\n", DISABLE_SYNTAX_HILIGHTING,
		 preferences_get_int (pr, DISABLE_SYNTAX_HILIGHTING));
	fprintf (fp, "%s=%d\n", SAVE_AUTOMATIC,
		 preferences_get_int (pr, SAVE_AUTOMATIC));
	fprintf (fp, "%s=%d\n", INDENT_AUTOMATIC,
		 preferences_get_int (pr, INDENT_AUTOMATIC));
	fprintf (fp, "%s=%d\n", USE_TABS, preferences_get_int (pr, USE_TABS));
	fprintf (fp, "%s=%d\n", BRACES_CHECK,
		 preferences_get_int (pr, BRACES_CHECK));

	fprintf (fp, "%s=%d\n", TAB_SIZE, preferences_get_int (pr, TAB_SIZE));
	fprintf (fp, "%s=%d\n", INDENT_SIZE,
		 preferences_get_int (pr, INDENT_SIZE));
	fprintf (fp, "%s=%d\n", AUTOSAVE_TIMER,
		 preferences_get_int (pr, AUTOSAVE_TIMER));
	fprintf (fp, "%s=%d\n", MARGIN_LINENUMBER_WIDTH,
		 preferences_get_int (pr, MARGIN_LINENUMBER_WIDTH));
	fprintf (fp, "%s=%d\n", SAVE_SESSION_TIMER,
		 preferences_get_int (pr, SAVE_SESSION_TIMER));

	/* Page 4 */
	str = preferences_get (pr, COMMAND_PRINT);
	fprintf (fp, "%s=%s\n", COMMAND_PRINT, str);
	g_free (str);

	/* Page 5 */
	fprintf (fp, "%s=%d\n", AUTOFORMAT_DISABLE,
		 preferences_get_int (pr, AUTOFORMAT_DISABLE));

	str = preferences_get (pr, AUTOFORMAT_CUSTOM_STYLE);
	fprintf (fp, "%s=%s\n", AUTOFORMAT_CUSTOM_STYLE, str);
	g_free (str);

	str = preferences_get (pr, AUTOFORMAT_STYLE);
	fprintf (fp, "%s=%s\n", AUTOFORMAT_STYLE, str);
	g_free (str);

	/* Page 6 */
	str = preferences_get (pr, EDITOR_TAG_POS);
	fprintf (fp, "%s=%s\n", EDITOR_TAG_POS, str);
	g_free (str);

	fprintf (fp, "%s=%d\n", EDITOR_TAG_HIDE,
		 preferences_get_int (pr, EDITOR_TAG_HIDE));
	fprintf (fp, "%s=%d\n", TRUNCAT_MESSAGES,
		 preferences_get_int (pr, TRUNCAT_MESSAGES));
	fprintf (fp, "%s=%d\n", TRUNCAT_MESG_FIRST,
		 preferences_get_int (pr, TRUNCAT_MESG_FIRST));
	fprintf (fp, "%s=%d\n", TRUNCAT_MESG_LAST,
		 preferences_get_int (pr, TRUNCAT_MESG_LAST));
	fprintf (fp, "%s=%d\n", AUTOMATIC_TAGS_UPDATE,
		 preferences_get_int (pr, AUTOMATIC_TAGS_UPDATE));
	if (pr->is_showing)
	{
		gdk_window_get_root_origin (pr->widgets.window->window,
					    &pr->win_pos_x, &pr->win_pos_y);
	}
	fprintf (fp, "preferences.win.pos.x=%d\n", pr->win_pos_x);
	fprintf (fp, "preferences.win.pos.y=%d\n", pr->win_pos_y);
	fprintf (fp, "text.zoom.factor=%d\n",
		 preferences_get_int (pr, "text.zoom.factor"));
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
