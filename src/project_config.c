/* 
    project_config.c
    Copyright (C) 2000  Kh. Naba Kumar Singh

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
#  include <config.h>
#endif

#include "anjuta.h"
#include "project_config.h"
#include "utilities.h"

static void create_project_config_gui (ProjectConfig * pc);
static gboolean on_close(GtkWidget *w, gpointer user_data);

ProjectConfig *
project_config_new (PropsID props)
{
	ProjectConfig *pc;
	gint i;

	pc = g_malloc (sizeof (ProjectConfig));
	if (!pc)
		return NULL;

	pc->props = props;
	for (i = 0; i < BUILD_FILE_END_MARK; i++)
	{
		pc->disable_overwrite[i] = FALSE;
	}
	pc->blocked = FALSE;
	pc->description = NULL;
	pc->config_progs = NULL;
	pc->config_libs = NULL;
	pc->config_headers = NULL;
	pc->config_characteristics = NULL;
	pc->config_lib_funcs = NULL;
	pc->config_additional = NULL;
	pc->config_files = NULL;
	pc->makefile_am = NULL;

	pc->is_showing = FALSE;
	pc->win_pos_x = 60;
	pc->win_pos_x = 60;
	pc->win_width = 500;
	pc->win_height = 300;
	create_project_config_gui (pc);
	return pc;
}

void
project_config_clear (ProjectConfig* pc)
{
	gint i;
	
	g_return_if_fail (pc != NULL);
	
	for (i=0; i < BUILD_FILE_END_MARK; i++)
		pc->disable_overwrite[i] = FALSE;
	string_assign (&pc->description, NULL);
	string_assign (&pc->config_progs, NULL);
	string_assign (&pc->config_libs, NULL);
	string_assign (&pc->config_headers, NULL);
	string_assign (&pc->config_characteristics, NULL);
	string_assign (&pc->config_lib_funcs, NULL);
	string_assign (&pc->config_additional, NULL);
	string_assign (&pc->config_files, NULL);
	string_assign (&pc->makefile_am, NULL);

	project_config_sync (pc);
}

void
project_config_destroy (ProjectConfig * pc)
{
	gint i;
	
	g_return_if_fail (pc != NULL);
	
	string_assign (&pc->description, NULL);
	string_assign (&pc->config_progs, NULL);
	string_assign (&pc->config_libs, NULL);
	string_assign (&pc->config_headers, NULL);
	string_assign (&pc->config_characteristics, NULL);
	string_assign (&pc->config_lib_funcs, NULL);
	string_assign (&pc->config_additional, NULL);
	
	for (i=0; i < BUILD_FILE_END_MARK; i++)
		gtk_widget_unref (pc->widgets.disable_overwrite_check[i]);
	
	gtk_widget_unref (pc->widgets.version_entry);
	gtk_widget_unref (pc->widgets.description_text);
	gtk_widget_unref (pc->widgets.ignore_entry);
	gtk_widget_unref (pc->widgets.config_progs_text);
	gtk_widget_unref (pc->widgets.config_libs_text);
	gtk_widget_unref (pc->widgets.config_headers_text);
	gtk_widget_unref (pc->widgets.config_characteristics_text);
	gtk_widget_unref (pc->widgets.config_lib_funcs_text);
	gtk_widget_unref (pc->widgets.config_additional_text);
	gtk_widget_unref (pc->widgets.config_files_text);
	gtk_widget_unref (pc->widgets.extra_modules_before_entry);
	gtk_widget_unref (pc->widgets.extra_modules_after_entry);
	gtk_widget_unref (pc->widgets.makefile_am_text);
	
	gtk_widget_destroy (pc->widgets.window);
	
	g_free (pc);
}

void
project_config_show (ProjectConfig * pc)
{
	g_return_if_fail (pc != NULL);
	if (pc->is_showing)
	{
		gdk_window_raise (pc->widgets.window->window);
		return;
	}
	project_config_sync (pc);
	gtk_widget_set_uposition (pc->widgets.window,
				  pc->win_pos_x, pc->win_pos_y);
	gtk_window_set_default_size (GTK_WINDOW
				     (pc->widgets.window),
				     pc->win_width, pc->win_height);
	gtk_widget_show (pc->widgets.window);
	pc->is_showing = TRUE;
}

void
project_config_hide (ProjectConfig * pc)
{
	g_return_if_fail (pc != NULL);
	if (pc->is_showing == FALSE)
		return;
	gdk_window_get_root_origin (pc->widgets.window->window,
				    &pc->win_pos_x, &pc->win_pos_y);
	gdk_window_get_size (pc->widgets.window->window, &pc->win_width,
			     &pc->win_height);
	pc->is_showing = FALSE;
}

static gchar *
get_text (GtkWidget *text_view)
{
	GtkTextBuffer *buffer;
	GtkTextIter start_iter;
	GtkTextIter end_iter;
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
	gtk_text_buffer_get_start_iter (buffer, &start_iter);
	gtk_text_buffer_get_end_iter (buffer, &end_iter);
	return gtk_text_buffer_get_text (buffer, &start_iter, &end_iter, TRUE);
}

static void
set_text (GtkWidget *text_view, const gchar *text)
{
	GtkTextBuffer *buffer;
	if (text)
	{
		buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
		gtk_text_buffer_set_text (buffer, text, -1);
	}
}

/* This puts the value in the struct into the widgets */
void
project_config_sync (ProjectConfig * pc)
{
	GtkWidget *entry;
	gint i;
	gchar *str;

	g_return_if_fail (pc != NULL);

	for (i = 0; i < BUILD_FILE_END_MARK; i++)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (pc->widgets.disable_overwrite_check[i]), pc->disable_overwrite[i]);
	}
	if (pc->blocked)
	{
		for (i = 0; i < BUILD_FILE_END_MARK; i++)
		{
			gtk_widget_set_sensitive (pc->widgets.disable_overwrite_check[i],
				FALSE);
		}
	}
	
	entry = pc->widgets.version_entry;
	str = prop_get(pc->props, "project.version");
	if (str) {
		gtk_entry_set_text (GTK_ENTRY (entry), str);
		g_free (str);
	}
	else
		gtk_entry_set_text (GTK_ENTRY (entry), "");

	set_text (pc->widgets.description_text, pc->description);

	entry = pc->widgets.ignore_entry;
	str = prop_get (pc->props, "project.excluded.modules");
	if (str) {
		gtk_entry_set_text (GTK_ENTRY (entry), str);
		g_free (str);
	}
	else
		gtk_entry_set_text (GTK_ENTRY (entry), "");
	
	set_text (pc->widgets.config_progs_text, pc->config_progs);
	set_text (pc->widgets.config_libs_text, pc->config_libs);
	set_text (pc->widgets.config_lib_funcs_text, pc->config_lib_funcs);
	set_text (pc->widgets.config_headers_text, pc->config_headers);
	set_text (pc->widgets.config_characteristics_text, pc->config_characteristics);
	set_text (pc->widgets.config_additional_text, pc->config_additional);
	set_text (pc->widgets.config_files_text, pc->config_files);
	set_text (pc->widgets.makefile_am_text, pc->makefile_am);
	
	entry = pc->widgets.extra_modules_before_entry;
	str = prop_get (pc->props, "project.config.extra.modules.before");
	if (str) {
		gtk_entry_set_text (GTK_ENTRY (entry), str);
		g_free (str);
	}
	else
		gtk_entry_set_text (GTK_ENTRY (entry), "");

	entry = pc->widgets.extra_modules_after_entry;
	str = prop_get (pc->props, "project.config.extra.modules.after");
	if (str) {
		gtk_entry_set_text (GTK_ENTRY (entry), str);
		g_free (str);
	}
	else
		gtk_entry_set_text (GTK_ENTRY (entry), "");
}

/* This gets the value in the widgets into the struct */
void
project_config_get (ProjectConfig * pc)
{
	GtkWidget *entry;
	gint i;
	gchar* str;

	g_return_if_fail (pc != NULL);

	for (i = 0; i < BUILD_FILE_END_MARK; i++)
	{
		pc->disable_overwrite[i] = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
						      (pc->widgets.disable_overwrite_check[i]));
	}

	entry = pc->widgets.version_entry;
	str = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
	if (str) {
		prop_set_with_key (pc->props, "project.version", str);
		g_free (str);
	} else {
		prop_set_with_key (pc->props, "project.version", "");
	}
	
	string_assign (&pc->description, NULL);
	pc->description = get_text (pc->widgets.description_text);
	
	entry = pc->widgets.ignore_entry;
	str = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
	if (str) {
		prop_set_with_key (pc->props, "project.excluded.modules", str);
		g_free (str);
	} else {
		prop_set_with_key (pc->props, "project.excluded.modules", "");
	}
	
	string_assign (&pc->config_progs, NULL);
	pc->config_progs = get_text (pc->widgets.config_progs_text);
	
	string_assign (&pc->config_libs, NULL);
	pc->config_libs = get_text (pc->widgets.config_libs_text);
	
	string_assign (&pc->config_headers, NULL);
	pc->config_headers = get_text (pc->widgets.config_headers_text);
	
	string_assign (&pc->config_characteristics, NULL);
	pc->config_characteristics = get_text (pc->widgets.config_characteristics_text);
	
	string_assign (&pc->config_lib_funcs, NULL);
	pc->config_lib_funcs = get_text (pc->widgets.config_lib_funcs_text);
	
	string_assign (&pc->config_additional, NULL);
	pc->config_additional = get_text (pc->widgets.config_additional_text);
	
	string_assign (&pc->config_files, NULL);
	pc->config_files = get_text (pc->widgets.config_files_text);
	
	string_assign (&pc->makefile_am, NULL);
	pc->makefile_am = get_text (pc->widgets.makefile_am_text);

	entry = pc->widgets.extra_modules_before_entry;
	str = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
	if (str) {
		prop_set_with_key (pc->props, "project.config.extra.modules.before", str);
		g_free (str);
	} else {
		prop_set_with_key (pc->props, "project.config.extra.modules.before", "");
	}

	entry = pc->widgets.extra_modules_after_entry;
	str = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
	if (str) {
		prop_set_with_key (pc->props, "project.config.extra.modules.after", str);
		g_free (str);
	} else {
		prop_set_with_key (pc->props, "project.config.extra.modules.after", "");
	}
}

/* Saves everything except the scripts in prj file */
gboolean
project_config_save (ProjectConfig * pc, FILE* stream)
{
	gint i;
	gchar *str;
	
	g_return_val_if_fail (pc != NULL, FALSE);
	g_return_val_if_fail (stream != NULL, FALSE);

	if (fprintf (stream, "project.config.blocked=%d\n", (int) pc->blocked) <1)
		return FALSE;
	
	fprintf (stream, "project.config.disable.overwriting=");
	for (i = 0; i < BUILD_FILE_END_MARK; i++)
	{
		if (fprintf (stream, "%d ", (int) pc->disable_overwrite[i]) <1)
			return FALSE;
	}
	fprintf (stream, "\n\n");
	return TRUE;
}

void
project_config_set_disable_overwrite (ProjectConfig* pc, gint build_file, gboolean disable)
{
	g_return_if_fail (pc != NULL);
	g_return_if_fail (build_file < BUILD_FILE_END_MARK);
	
	pc->disable_overwrite[build_file]=disable;
	project_config_sync(pc);
}

void
project_config_set_disable_overwrite_all (ProjectConfig* pc, gboolean disable)
{
	gint i;
	
	g_return_if_fail (pc != NULL);

	for (i=0; i <BUILD_FILE_END_MARK; i++)
		pc->disable_overwrite[i]=disable;
	project_config_sync(pc);
}

/* Loads everything except the scripts. */
gboolean
project_config_load (ProjectConfig * pc, PropsID props)
{
	g_return_val_if_fail (pc != NULL, FALSE);
	
	/* Is the project configuration blocked? */
	pc->blocked = prop_get_int (props, "project.config.blocked", 0);
	
	if (pc->blocked == 0)
	{
		GList *list, *node;
		gint i;
		
		/* Do not call project_config_clear() */
		/* Because we have already loaded the scripts */
		list = glist_from_data (props, "project.config.disable.overwriting");
		node = list;
		i=0;
		while (node)
		{
			if (node->data)
			{
				if (i >= BUILD_FILE_END_MARK)
				{
					glist_strings_free (list);
					project_config_sync (pc);
					return FALSE;
				}
				if (sscanf (node->data, "%d", &pc->disable_overwrite[i]) <1)
				{
					glist_strings_free (list);
					project_config_sync (pc);
					return FALSE;
				}
			}
			i++;
			node = g_list_next (node);
		}
		if (i < BUILD_FILE_END_MARK)
		{
			glist_strings_free (list);
			project_config_sync (pc);
			return FALSE;
		}
		glist_strings_free (list);
	}
	else
	{
		int i;
		for (i = 0; i < BUILD_FILE_END_MARK; i++)
			pc->disable_overwrite[i] = 1;
	}
	project_config_sync (pc);
	return TRUE;
}

/* Load the scripts from buffer. */
gint
project_config_load_scripts (ProjectConfig * pc, gchar* buff)
{
	gchar *start, *end, *pos;

	g_return_val_if_fail (pc != NULL, -1);
	g_return_val_if_fail (buff != NULL, -1);

	pos = buff;

	/* Do not call project_config_clear() */
	/* Because we might be loading into other things in project_config */

	start = strstr(pos, PROJECT_DESCRIPTION_START);
	if (!start) return -1;
	end = strstr(pos+strlen(PROJECT_DESCRIPTION_START), PROJECT_DESCRIPTION_END);
	if (!end) return -1;
	start += strlen (PROJECT_DESCRIPTION_START);
	string_assign (&pc->description, NULL);
	if (end > start)
		pc->description = g_strndup (start, end-start);
	pos = end+strlen(PROJECT_DESCRIPTION_END);
	
	start = strstr(pos, CONFIG_PROGS_START);
	if (!start) return -1;
	end = strstr(pos+strlen(CONFIG_PROGS_START), CONFIG_PROGS_END);
	if (!end) return -1;
	start += strlen (CONFIG_PROGS_START);
	string_assign (&pc->config_progs, NULL);
	if (end > start)
		pc->config_progs = g_strndup (start, end-start);
	pos = end+strlen(CONFIG_PROGS_END);
	
	start = strstr(pos, CONFIG_LIBS_START);
	if (!start) return -1;
	end = strstr(pos+strlen(CONFIG_LIBS_START), CONFIG_LIBS_END);
	if (!end) return -1;
	start += strlen (CONFIG_LIBS_START);
	string_assign (&pc->config_libs, NULL);
	if (end > start)
		pc->config_libs = g_strndup (start, end-start);
	pos = end+strlen(CONFIG_LIBS_END);
		
	start = strstr(pos, CONFIG_HEADERS_START);
	if (!start) return -1;
	end = strstr(pos+strlen(CONFIG_HEADERS_START), CONFIG_HEADERS_END);
	if (!end) return -1;
	start += strlen (CONFIG_HEADERS_START);
	string_assign (&pc->config_headers, NULL);
	if (end > start)
		pc->config_headers = g_strndup (start, end-start);
	pos = end+strlen(CONFIG_HEADERS_END);
	
	start = strstr(pos, CONFIG_CHARACTERISTICS_START);
	if (!start) return -1;
	end = strstr(pos+strlen(CONFIG_CHARACTERISTICS_START), CONFIG_CHARACTERISTICS_END);
	if (!end) return -1;
	start += strlen (CONFIG_CHARACTERISTICS_START);
	string_assign (&pc->config_characteristics, NULL);
	if (end > start)
		pc->config_characteristics = g_strndup (start, end-start);
	pos = end+strlen(CONFIG_CHARACTERISTICS_END);

	start = strstr(pos, CONFIG_LIB_FUNCS_START);
	if (!start) return -1;
	end = strstr(pos+strlen(CONFIG_LIB_FUNCS_START), CONFIG_LIB_FUNCS_END);
	if (!end) return -1;
	start += strlen (CONFIG_LIB_FUNCS_START);
	string_assign (&pc->config_lib_funcs, NULL);
	if (end > start)
		pc->config_lib_funcs = g_strndup (start, end-start);
	pos = end+strlen(CONFIG_LIB_FUNCS_END);

	start = strstr(pos, CONFIG_ADDITIONAL_START);
	if (!start) return -1;
	end = strstr(pos+strlen(CONFIG_ADDITIONAL_START), CONFIG_ADDITIONAL_END);
	if (!end) return -1;
	start += strlen (CONFIG_ADDITIONAL_START);
	string_assign (&pc->config_additional, NULL);
	if (end > start)
		pc->config_additional = g_strndup (start, end-start);
	pos = end+strlen(CONFIG_ADDITIONAL_END);

	start = strstr(pos, CONFIG_FILES_START);
	if (!start) return -1;
	end = strstr(pos+strlen(CONFIG_FILES_START), CONFIG_FILES_END);
	if (!end) return -1;
	start += strlen (CONFIG_FILES_START);
	string_assign (&pc->config_files, NULL);
	if (end > start)
		pc->config_files = g_strndup (start, end-start);
	pos = end+strlen(CONFIG_FILES_END);

	start = strstr(pos, MAKEFILE_AM_START);
	if (!start) return -1;
	end = strstr(pos+strlen(MAKEFILE_AM_START), MAKEFILE_AM_END);
	if (!end) return -1;
	start += strlen (MAKEFILE_AM_START);
	string_assign (&pc->makefile_am, NULL);
	if (end > start)
		pc->makefile_am = g_strndup (start, end-start);
	pos = end+strlen(MAKEFILE_AM_END);

	project_config_sync (pc);
	return (pos-buff);
}

/* Save the scripts in project file. */
gboolean
project_config_save_scripts (ProjectConfig * pc, FILE* fp)
{
	fprintf (fp, PROJECT_DESCRIPTION_START);
	if (pc->description)
		fprintf (fp,"%s", pc->description);
	fprintf (fp, PROJECT_DESCRIPTION_END);
	if (ferror(fp))return FALSE;

	fprintf (fp, CONFIG_PROGS_START);
	if (pc->config_progs)
		fprintf (fp,"%s", pc->config_progs);
	fprintf (fp, CONFIG_PROGS_END);
	if (ferror(fp))return FALSE;
	
	fprintf (fp, CONFIG_LIBS_START);
	if (pc->config_libs)
		fprintf (fp,"%s", pc->config_libs);
	fprintf (fp, CONFIG_LIBS_END);
	if (ferror(fp))return FALSE;

	fprintf (fp, CONFIG_HEADERS_START);
	if (pc->config_headers)
		fprintf (fp,"%s", pc->config_headers);
	fprintf (fp, CONFIG_HEADERS_END);
	if (ferror(fp))return FALSE;

	fprintf (fp, CONFIG_CHARACTERISTICS_START);
	if (pc->config_characteristics)
		fprintf (fp,"%s", pc->config_characteristics);
	fprintf (fp, CONFIG_CHARACTERISTICS_END);
	if (ferror(fp))return FALSE;

	fprintf (fp, CONFIG_LIB_FUNCS_START);
	if (pc->config_lib_funcs)
		fprintf (fp,"%s", pc->config_lib_funcs);
	fprintf (fp, CONFIG_LIB_FUNCS_END);
	if (ferror(fp))return FALSE;

	fprintf (fp, CONFIG_ADDITIONAL_START);
	if (pc->config_additional)
		fprintf (fp,"%s", pc->config_additional);
	fprintf (fp, CONFIG_ADDITIONAL_END);
	if (ferror(fp))return FALSE;

	fprintf (fp, CONFIG_FILES_START);
	if (pc->config_files)
		fprintf (fp,"%s", pc->config_files);
	fprintf (fp, CONFIG_FILES_END);
	if (ferror(fp))return FALSE;

	fprintf (fp, MAKEFILE_AM_START);
	if (pc->makefile_am)
		fprintf (fp,"%s", pc->makefile_am);
	fprintf (fp, MAKEFILE_AM_END);
	if (ferror(fp))return FALSE;

	fprintf (fp, "\n");
	if (ferror(fp))return FALSE;

	return TRUE;
}

/* Write the scripts in a executable file. */
/* usualy in configur.in */
gboolean
project_config_write_scripts (ProjectConfig * pc, FILE* stream)
{
	g_return_val_if_fail (pc != NULL, FALSE);
	g_return_val_if_fail (stream != NULL, FALSE);

	fprintf (stream, "dnl Checks for programs.\n");
	if (pc->config_progs)
		if (fprintf (stream, "%s\n\n", pc->config_progs) <1)
			return FALSE;
	
	fprintf (stream, "dnl Checks for libraries.\n");
	if (pc->config_libs)
		if (fprintf (stream, "%s\n\n", pc->config_libs) <1)
			return FALSE;
	
	fprintf (stream, "dnl Checks for header files.\n");
	if (pc->config_headers)
		if (fprintf (stream, "%s\n\n", pc->config_headers) < 1)
			return FALSE;
	
	fprintf (stream, "dnl Checks for typedefs, structures, and compiler characteristics.\n");
	if (pc->config_characteristics)
		if (fprintf (stream, "%s\n\n", pc->config_characteristics) < 1)
			return FALSE;
	
	fprintf (stream, "dnl Checks for library functions.\n");
	if (pc->config_lib_funcs)
		if (fprintf (stream, "%s\n\n", pc->config_lib_funcs) < 1)
			return FALSE;

	fprintf (stream, "dnl Checks for Additional stuffs.\n");
	if (pc->config_additional)
		if (fprintf (stream, "%s\n\n", pc->config_additional) < 1)
			return FALSE;
	return TRUE;
}

gboolean
project_config_save_yourself (ProjectConfig * pc, FILE* stream)
{
	g_return_val_if_fail (pc != NULL, FALSE);

	if (pc->is_showing)
	{
		gdk_window_get_root_origin (pc->widgets.window->window,
					    &pc->win_pos_x, &pc->win_pos_y);
		gdk_window_get_size (pc->widgets.window->window,
				     &pc->win_width, &pc->win_height);
	}
	fprintf (stream, "project.config.win.pos.x=%d\n", pc->win_pos_x);
	fprintf (stream, "project.config.win.pos.y=%d\n", pc->win_pos_y);
	fprintf (stream, "project.config.win.width=%d\n", pc->win_width);
	fprintf (stream, "project.config.win.height=%d\n", pc->win_height);
	return TRUE;
}

gboolean
project_config_load_yourself (ProjectConfig * pc, PropsID props)
{
	g_return_val_if_fail (pc != NULL, FALSE);
	pc->win_pos_x = prop_get_int (props, "project.config.win.pos.x", 60);
	pc->win_pos_y = prop_get_int (props, "project.config.win.pos.y", 60);
	pc->win_width = prop_get_int (props, "project.config.win.width", 500);
	pc->win_height = prop_get_int (props, "project.config.win.height", 300);
	return TRUE;
}

static void
on_response (GtkButton *button, gint response, gpointer user_data)
{
	ProjectConfig* pc;
	pc = (ProjectConfig *)user_data;
	g_return_if_fail (pc);
	switch (response)
	{
	case GTK_RESPONSE_OK:
		gtk_dialog_close(GTK_DIALOG(pc->widgets.window));
		/* Note: No break here */
	case GTK_RESPONSE_APPLY:
		project_config_get (pc);
		if (app->project_dbase->project_is_open)
			app->project_dbase->is_saved = FALSE;
		break;
	case GTK_RESPONSE_CANCEL:
		gtk_dialog_close(GTK_DIALOG(pc->widgets.window));
		break;
	}
}

static gboolean
on_close(GtkWidget *w, gpointer user_data)
{
	ProjectConfig *pc = (ProjectConfig *)user_data;
	project_config_hide(pc);
	return FALSE;
}

static void
create_project_config_gui (ProjectConfig * pc)
{
	gint i;
	
	pc->widgets.window = glade_xml_get_widget (app->gxml, "project_config_dialog");
	g_signal_connect (G_OBJECT (pc->widgets.window), "clicked",
			    G_CALLBACK (on_response), pc);
	g_signal_connect (G_OBJECT (pc->widgets.window), "close",
				G_CALLBACK (on_close), pc);

	for(i=0; i< BUILD_FILE_END_MARK; i++)
	{
		gchar *key;
		key = g_strdup_printf ("project_config_build_file_%d", i);
		pc->widgets.disable_overwrite_check[i] =
			glade_xml_get_widget (app->gxml, key);
	}
	pc->widgets.version_entry = glade_xml_get_widget (app->gxml, "project_config_version");
	pc->widgets.description_text = glade_xml_get_widget (app->gxml, "project_config_description");
	pc->widgets.ignore_entry = glade_xml_get_widget (app->gxml, "project_config_ignore");
	pc->widgets.config_progs_text = glade_xml_get_widget (app->gxml, "project_config_programs");
	pc->widgets.config_libs_text = glade_xml_get_widget (app->gxml, "project_config_libraries");
	pc->widgets.config_headers_text = glade_xml_get_widget (app->gxml, "project_config_headers");
	pc->widgets.config_characteristics_text = glade_xml_get_widget (app->gxml, "project_config_compiler");
	pc->widgets.config_lib_funcs_text = glade_xml_get_widget (app->gxml, "project_config_functions");
	pc->widgets.config_additional_text = glade_xml_get_widget (app->gxml, "project_config_additional");
	pc->widgets.config_files_text = glade_xml_get_widget (app->gxml, "project_config_outputs");
	pc->widgets.extra_modules_before_entry = glade_xml_get_widget (app->gxml, "project_config_modules_before");
	pc->widgets.extra_modules_after_entry = glade_xml_get_widget (app->gxml, "project_config_modules_after");
	pc->widgets.makefile_am_text = glade_xml_get_widget (app->gxml, "project_config_makefile_am");
}
