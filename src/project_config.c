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

struct _ProjectConfigPriv
{
	/* Dialog widgets */
	GtkWidget* window;
	GtkWidget* disable_overwrite_check[BUILD_FILE_END_MARK];

	GtkWidget* version_entry;
	GtkWidget* target_entry;
	GtkWidget* gui_editor_entry;
	GtkWidget* description_text;
	GtkWidget* ignore_entry;
	GtkWidget* config_progs_text;
	GtkWidget* config_libs_text;
	GtkWidget* config_headers_text;
	GtkWidget* config_characteristics_text;
	GtkWidget* config_lib_funcs_text;
	GtkWidget* config_additional_text;
	GtkWidget* config_files_text;

	GtkWidget* extra_modules_before_entry;
	GtkWidget* extra_modules_after_entry;
	GtkWidget* makefile_am_text;
	
	/* Private member data */
	GladeXML *gxml;
	PropsID props;
	
	gboolean blocked;
	
	gboolean disable_overwrite[BUILD_FILE_END_MARK];
	gchar* description;
	gchar* config_progs;
	gchar* config_libs;
	gchar* config_headers;
	gchar* config_characteristics;
	gchar* config_lib_funcs;
	gchar* config_additional;
	gchar* config_files;
	gchar* makefile_am;

	gboolean is_showing;
	gint win_pos_x, win_pos_y, win_width, win_height;
};

static void create_project_config_gui (ProjectConfig * pc);
static gboolean on_close(GtkWidget *w, gpointer user_data);

ProjectConfig *
project_config_new (PropsID props)
{
	ProjectConfig *pc;
	gint i;

	pc = g_new0 (ProjectConfig, 1);
	pc->priv = g_new0 (ProjectConfigPriv, 1);

	pc->priv->props = props;
	for (i = 0; i < BUILD_FILE_END_MARK; i++)
	{
		pc->priv->disable_overwrite[i] = FALSE;
	}
	pc->priv->blocked = FALSE;
	pc->priv->description = NULL;
	pc->priv->config_progs = NULL;
	pc->priv->config_libs = NULL;
	pc->priv->config_headers = NULL;
	pc->priv->config_characteristics = NULL;
	pc->priv->config_lib_funcs = NULL;
	pc->priv->config_additional = NULL;
	pc->priv->config_files = NULL;
	pc->priv->makefile_am = NULL;

	pc->priv->is_showing = FALSE;
	pc->priv->win_pos_x = 60;
	pc->priv->win_pos_x = 60;
	pc->priv->win_width = 500;
	pc->priv->win_height = 300;
	create_project_config_gui (pc);
	return pc;
}

void
project_config_clear (ProjectConfig* pc)
{
	gint i;
	
	g_return_if_fail (pc != NULL);
	
	for (i=0; i < BUILD_FILE_END_MARK; i++)
		pc->priv->disable_overwrite[i] = FALSE;
	string_assign (&pc->priv->description, NULL);
	string_assign (&pc->priv->config_progs, NULL);
	string_assign (&pc->priv->config_libs, NULL);
	string_assign (&pc->priv->config_headers, NULL);
	string_assign (&pc->priv->config_characteristics, NULL);
	string_assign (&pc->priv->config_lib_funcs, NULL);
	string_assign (&pc->priv->config_additional, NULL);
	string_assign (&pc->priv->config_files, NULL);
	string_assign (&pc->priv->makefile_am, NULL);

	project_config_sync (pc);
}

void
project_config_destroy (ProjectConfig * pc)
{
	gint i;
	
	g_return_if_fail (pc != NULL);
	
	string_assign (&pc->priv->description, NULL);
	string_assign (&pc->priv->config_progs, NULL);
	string_assign (&pc->priv->config_libs, NULL);
	string_assign (&pc->priv->config_headers, NULL);
	string_assign (&pc->priv->config_characteristics, NULL);
	string_assign (&pc->priv->config_lib_funcs, NULL);
	string_assign (&pc->priv->config_additional, NULL);
	
	gtk_widget_destroy (pc->priv->window);
	g_object_unref (pc->priv->gxml);
	g_free (pc->priv);
	g_free (pc);
}

void
project_config_show (ProjectConfig * pc)
{
	g_return_if_fail (pc != NULL);
	if (pc->priv->is_showing)
	{
		gdk_window_raise (pc->priv->window->window);
		return;
	}
	project_config_sync (pc);
	gtk_widget_set_uposition (pc->priv->window,
				  pc->priv->win_pos_x, pc->priv->win_pos_y);
	gtk_window_set_default_size (GTK_WINDOW
				     (pc->priv->window),
				     pc->priv->win_width, pc->priv->win_height);
	gtk_widget_show (pc->priv->window);
	pc->priv->is_showing = TRUE;
}

void
project_config_hide (ProjectConfig * pc)
{
	g_return_if_fail (pc != NULL);
	if (pc->priv->is_showing == FALSE)
		return;
	gdk_window_get_root_origin (pc->priv->window->window,
				    &pc->priv->win_pos_x, &pc->priv->win_pos_y);
	gdk_window_get_size (pc->priv->window->window, &pc->priv->win_width,
			     &pc->priv->win_height);
	gtk_widget_hide (pc->priv->window);
	pc->priv->is_showing = FALSE;
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
					      (pc->priv->disable_overwrite_check[i]), pc->priv->disable_overwrite[i]);
	}
	if (pc->priv->blocked)
	{
		for (i = 0; i < BUILD_FILE_END_MARK; i++)
		{
			gtk_widget_set_sensitive (pc->priv->disable_overwrite_check[i],
				FALSE);
		}
	}
	
	entry = pc->priv->version_entry;
	str = prop_get(pc->priv->props, "project.version");
	if (str) {
		gtk_entry_set_text (GTK_ENTRY (entry), str);
		g_free (str);
	}
	else
		gtk_entry_set_text (GTK_ENTRY (entry), "");

	entry = pc->priv->target_entry;
	str = prop_get(pc->priv->props, "project.source.target");
	if (str) {
		gtk_entry_set_text (GTK_ENTRY (entry), str);
		g_free (str);
	}
	else
		gtk_entry_set_text (GTK_ENTRY (entry), "");
	
	entry = pc->priv->gui_editor_entry;
	str = prop_get(pc->priv->props, "project.gui.command");
	if (str) {
		gtk_entry_set_text (GTK_ENTRY (entry), str);
		g_free (str);
	}
	else
		gtk_entry_set_text (GTK_ENTRY (entry), "");
	
	set_text (pc->priv->description_text, pc->priv->description);

	entry = pc->priv->ignore_entry;
	str = prop_get (pc->priv->props, "project.excluded.modules");
	if (str) {
		gtk_entry_set_text (GTK_ENTRY (entry), str);
		g_free (str);
	}
	else
		gtk_entry_set_text (GTK_ENTRY (entry), "");
	
	set_text (pc->priv->config_progs_text, pc->priv->config_progs);
	set_text (pc->priv->config_libs_text, pc->priv->config_libs);
	set_text (pc->priv->config_lib_funcs_text, pc->priv->config_lib_funcs);
	set_text (pc->priv->config_headers_text, pc->priv->config_headers);
	set_text (pc->priv->config_characteristics_text, pc->priv->config_characteristics);
	set_text (pc->priv->config_additional_text, pc->priv->config_additional);
	set_text (pc->priv->config_files_text, pc->priv->config_files);
	set_text (pc->priv->makefile_am_text, pc->priv->makefile_am);
	
	entry = pc->priv->extra_modules_before_entry;
	str = prop_get (pc->priv->props, "project.config.extra.modules.before");
	if (str) {
		gtk_entry_set_text (GTK_ENTRY (entry), str);
		g_free (str);
	}
	else
		gtk_entry_set_text (GTK_ENTRY (entry), "");

	entry = pc->priv->extra_modules_after_entry;
	str = prop_get (pc->priv->props, "project.config.extra.modules.after");
	if (str) {
		gtk_entry_set_text (GTK_ENTRY (entry), str);
		g_free (str);
	}
	else
		gtk_entry_set_text (GTK_ENTRY (entry), "");
}

gchar *
project_config_get_description (ProjectConfig *pc)
{
	return pc->priv->description;
}

void
project_config_set_description (ProjectConfig *pc, const gchar *desc)
{
	if (desc) {
		if (pc->priv->description)
			g_free (pc->priv->description);
		pc->priv->description = g_strdup (desc);
	}
}

gchar *
project_config_get_config_files (ProjectConfig *pc)
{
	return pc->priv->config_files;
}

gchar *
project_config_get_makefile_am_content (ProjectConfig *pc)
{
	return pc->priv->makefile_am;
}

gboolean
project_config_get_overwrite_disabled (ProjectConfig *pc, int file_id)
{
	return pc->priv->disable_overwrite[file_id];
}

gboolean
project_config_is_blocked (ProjectConfig *pc)
{
	return pc->priv->blocked;
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
		pc->priv->disable_overwrite[i] = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
						      (pc->priv->disable_overwrite_check[i]));
	}

	entry = pc->priv->version_entry;
	str = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
	if (str) {
		prop_set_with_key (pc->priv->props, "project.version", str);
		g_free (str);
	} else {
		prop_set_with_key (pc->priv->props, "project.version", "");
	}
	
	entry = pc->priv->target_entry;
	str = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
	if (str) {
		prop_set_with_key (pc->priv->props, "project.source.target", str);
		g_free (str);
	} else {
		prop_set_with_key (pc->priv->props, "project.source.target", "");
	}
	
	entry = pc->priv->gui_editor_entry;
	str = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
	if (str) {
		prop_set_with_key (pc->priv->props, "project.gui.command", str);
		g_free (str);
	} else {
		prop_set_with_key (pc->priv->props, "project.gui.command", "");
	}
	
	string_assign (&pc->priv->description, NULL);
	pc->priv->description = get_text (pc->priv->description_text);
	
	entry = pc->priv->ignore_entry;
	str = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
	if (str) {
		prop_set_with_key (pc->priv->props, "project.excluded.modules", str);
		g_free (str);
	} else {
		prop_set_with_key (pc->priv->props, "project.excluded.modules", "");
	}
	
	string_assign (&pc->priv->config_progs, NULL);
	pc->priv->config_progs = get_text (pc->priv->config_progs_text);
	
	string_assign (&pc->priv->config_libs, NULL);
	pc->priv->config_libs = get_text (pc->priv->config_libs_text);
	
	string_assign (&pc->priv->config_headers, NULL);
	pc->priv->config_headers = get_text (pc->priv->config_headers_text);
	
	string_assign (&pc->priv->config_characteristics, NULL);
	pc->priv->config_characteristics = get_text (pc->priv->config_characteristics_text);
	
	string_assign (&pc->priv->config_lib_funcs, NULL);
	pc->priv->config_lib_funcs = get_text (pc->priv->config_lib_funcs_text);
	
	string_assign (&pc->priv->config_additional, NULL);
	pc->priv->config_additional = get_text (pc->priv->config_additional_text);
	
	string_assign (&pc->priv->config_files, NULL);
	pc->priv->config_files = get_text (pc->priv->config_files_text);
	
	string_assign (&pc->priv->makefile_am, NULL);
	pc->priv->makefile_am = get_text (pc->priv->makefile_am_text);

	entry = pc->priv->extra_modules_before_entry;
	str = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
	if (str) {
		prop_set_with_key (pc->priv->props, "project.config.extra.modules.before", str);
		g_free (str);
	} else {
		prop_set_with_key (pc->priv->props, "project.config.extra.modules.before", "");
	}

	entry = pc->priv->extra_modules_after_entry;
	str = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
	if (str) {
		prop_set_with_key (pc->priv->props, "project.config.extra.modules.after", str);
		g_free (str);
	} else {
		prop_set_with_key (pc->priv->props, "project.config.extra.modules.after", "");
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

	if (fprintf (stream, "project.config.blocked=%d\n", (int) pc->priv->blocked) <1)
		return FALSE;
	
	fprintf (stream, "project.config.disable.overwriting=");
	for (i = 0; i < BUILD_FILE_END_MARK; i++)
	{
		if (fprintf (stream, "%d ", (int) pc->priv->disable_overwrite[i]) <1)
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
	
	pc->priv->disable_overwrite[build_file]=disable;
	project_config_sync(pc);
}

void
project_config_set_disable_overwrite_all (ProjectConfig* pc, gboolean disable)
{
	gint i;
	
	g_return_if_fail (pc != NULL);

	for (i=0; i <BUILD_FILE_END_MARK; i++)
		pc->priv->disable_overwrite[i]=disable;
	project_config_sync(pc);
}

/* Loads everything except the scripts. */
gboolean
project_config_load (ProjectConfig * pc, PropsID props)
{
	g_return_val_if_fail (pc != NULL, FALSE);
	
	/* Is the project configuration blocked? */
	pc->priv->blocked = prop_get_int (props, "project.config.blocked", 0);
	
	if (pc->priv->blocked == 0)
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
				if (sscanf (node->data, "%d", &pc->priv->disable_overwrite[i]) <1)
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
			pc->priv->disable_overwrite[i] = 1;
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
	string_assign (&pc->priv->description, NULL);
	if (end > start)
		pc->priv->description = g_strndup (start, end-start);
	pos = end+strlen(PROJECT_DESCRIPTION_END);
	
	start = strstr(pos, CONFIG_PROGS_START);
	if (!start) return -1;
	end = strstr(pos+strlen(CONFIG_PROGS_START), CONFIG_PROGS_END);
	if (!end) return -1;
	start += strlen (CONFIG_PROGS_START);
	string_assign (&pc->priv->config_progs, NULL);
	if (end > start)
		pc->priv->config_progs = g_strndup (start, end-start);
	pos = end+strlen(CONFIG_PROGS_END);
	
	start = strstr(pos, CONFIG_LIBS_START);
	if (!start) return -1;
	end = strstr(pos+strlen(CONFIG_LIBS_START), CONFIG_LIBS_END);
	if (!end) return -1;
	start += strlen (CONFIG_LIBS_START);
	string_assign (&pc->priv->config_libs, NULL);
	if (end > start)
		pc->priv->config_libs = g_strndup (start, end-start);
	pos = end+strlen(CONFIG_LIBS_END);
		
	start = strstr(pos, CONFIG_HEADERS_START);
	if (!start) return -1;
	end = strstr(pos+strlen(CONFIG_HEADERS_START), CONFIG_HEADERS_END);
	if (!end) return -1;
	start += strlen (CONFIG_HEADERS_START);
	string_assign (&pc->priv->config_headers, NULL);
	if (end > start)
		pc->priv->config_headers = g_strndup (start, end-start);
	pos = end+strlen(CONFIG_HEADERS_END);
	
	start = strstr(pos, CONFIG_CHARACTERISTICS_START);
	if (!start) return -1;
	end = strstr(pos+strlen(CONFIG_CHARACTERISTICS_START), CONFIG_CHARACTERISTICS_END);
	if (!end) return -1;
	start += strlen (CONFIG_CHARACTERISTICS_START);
	string_assign (&pc->priv->config_characteristics, NULL);
	if (end > start)
		pc->priv->config_characteristics = g_strndup (start, end-start);
	pos = end+strlen(CONFIG_CHARACTERISTICS_END);

	start = strstr(pos, CONFIG_LIB_FUNCS_START);
	if (!start) return -1;
	end = strstr(pos+strlen(CONFIG_LIB_FUNCS_START), CONFIG_LIB_FUNCS_END);
	if (!end) return -1;
	start += strlen (CONFIG_LIB_FUNCS_START);
	string_assign (&pc->priv->config_lib_funcs, NULL);
	if (end > start)
		pc->priv->config_lib_funcs = g_strndup (start, end-start);
	pos = end+strlen(CONFIG_LIB_FUNCS_END);

	start = strstr(pos, CONFIG_ADDITIONAL_START);
	if (!start) return -1;
	end = strstr(pos+strlen(CONFIG_ADDITIONAL_START), CONFIG_ADDITIONAL_END);
	if (!end) return -1;
	start += strlen (CONFIG_ADDITIONAL_START);
	string_assign (&pc->priv->config_additional, NULL);
	if (end > start)
		pc->priv->config_additional = g_strndup (start, end-start);
	pos = end+strlen(CONFIG_ADDITIONAL_END);

	start = strstr(pos, CONFIG_FILES_START);
	if (!start) return -1;
	end = strstr(pos+strlen(CONFIG_FILES_START), CONFIG_FILES_END);
	if (!end) return -1;
	start += strlen (CONFIG_FILES_START);
	string_assign (&pc->priv->config_files, NULL);
	if (end > start)
		pc->priv->config_files = g_strndup (start, end-start);
	pos = end+strlen(CONFIG_FILES_END);

	start = strstr(pos, MAKEFILE_AM_START);
	if (!start) return -1;
	end = strstr(pos+strlen(MAKEFILE_AM_START), MAKEFILE_AM_END);
	if (!end) return -1;
	start += strlen (MAKEFILE_AM_START);
	string_assign (&pc->priv->makefile_am, NULL);
	if (end > start)
		pc->priv->makefile_am = g_strndup (start, end-start);
	pos = end+strlen(MAKEFILE_AM_END);

	project_config_sync (pc);
	return (pos-buff);
}

/* Save the scripts in project file. */
gboolean
project_config_save_scripts (ProjectConfig * pc, FILE* fp)
{
	fprintf (fp, PROJECT_DESCRIPTION_START);
	if (pc->priv->description)
		fprintf (fp,"%s", pc->priv->description);
	fprintf (fp, PROJECT_DESCRIPTION_END);
	if (ferror(fp))return FALSE;

	fprintf (fp, CONFIG_PROGS_START);
	if (pc->priv->config_progs)
		fprintf (fp,"%s", pc->priv->config_progs);
	fprintf (fp, CONFIG_PROGS_END);
	if (ferror(fp))return FALSE;
	
	fprintf (fp, CONFIG_LIBS_START);
	if (pc->priv->config_libs)
		fprintf (fp,"%s", pc->priv->config_libs);
	fprintf (fp, CONFIG_LIBS_END);
	if (ferror(fp))return FALSE;

	fprintf (fp, CONFIG_HEADERS_START);
	if (pc->priv->config_headers)
		fprintf (fp,"%s", pc->priv->config_headers);
	fprintf (fp, CONFIG_HEADERS_END);
	if (ferror(fp))return FALSE;

	fprintf (fp, CONFIG_CHARACTERISTICS_START);
	if (pc->priv->config_characteristics)
		fprintf (fp,"%s", pc->priv->config_characteristics);
	fprintf (fp, CONFIG_CHARACTERISTICS_END);
	if (ferror(fp))return FALSE;

	fprintf (fp, CONFIG_LIB_FUNCS_START);
	if (pc->priv->config_lib_funcs)
		fprintf (fp,"%s", pc->priv->config_lib_funcs);
	fprintf (fp, CONFIG_LIB_FUNCS_END);
	if (ferror(fp))return FALSE;

	fprintf (fp, CONFIG_ADDITIONAL_START);
	if (pc->priv->config_additional)
		fprintf (fp,"%s", pc->priv->config_additional);
	fprintf (fp, CONFIG_ADDITIONAL_END);
	if (ferror(fp))return FALSE;

	fprintf (fp, CONFIG_FILES_START);
	if (pc->priv->config_files)
		fprintf (fp,"%s", pc->priv->config_files);
	fprintf (fp, CONFIG_FILES_END);
	if (ferror(fp))return FALSE;

	fprintf (fp, MAKEFILE_AM_START);
	if (pc->priv->makefile_am)
		fprintf (fp,"%s", pc->priv->makefile_am);
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
	if (pc->priv->config_progs)
		if (fprintf (stream, "%s\n\n", pc->priv->config_progs) <1)
			return FALSE;
	
	fprintf (stream, "dnl Checks for libraries.\n");
	if (pc->priv->config_libs)
		if (fprintf (stream, "%s\n\n", pc->priv->config_libs) <1)
			return FALSE;
	
	fprintf (stream, "dnl Checks for header files.\n");
	if (pc->priv->config_headers)
		if (fprintf (stream, "%s\n\n", pc->priv->config_headers) < 1)
			return FALSE;
	
	fprintf (stream, "dnl Checks for typedefs, structures, and compiler characteristics.\n");
	if (pc->priv->config_characteristics)
		if (fprintf (stream, "%s\n\n", pc->priv->config_characteristics) < 1)
			return FALSE;
	
	fprintf (stream, "dnl Checks for library functions.\n");
	if (pc->priv->config_lib_funcs)
		if (fprintf (stream, "%s\n\n", pc->priv->config_lib_funcs) < 1)
			return FALSE;

	fprintf (stream, "dnl Checks for Additional stuffs.\n");
	if (pc->priv->config_additional)
		if (fprintf (stream, "%s\n\n", pc->priv->config_additional) < 1)
			return FALSE;
	return TRUE;
}

gboolean
project_config_save_yourself (ProjectConfig * pc, FILE* stream)
{
	g_return_val_if_fail (pc != NULL, FALSE);

	if (pc->priv->is_showing)
	{
		gdk_window_get_root_origin (pc->priv->window->window,
					    &pc->priv->win_pos_x, &pc->priv->win_pos_y);
		gdk_window_get_size (pc->priv->window->window,
				     &pc->priv->win_width, &pc->priv->win_height);
	}
	fprintf (stream, "project.config.win.pos.x=%d\n", pc->priv->win_pos_x);
	fprintf (stream, "project.config.win.pos.y=%d\n", pc->priv->win_pos_y);
	fprintf (stream, "project.config.win.width=%d\n", pc->priv->win_width);
	fprintf (stream, "project.config.win.height=%d\n", pc->priv->win_height);
	return TRUE;
}

gboolean
project_config_load_yourself (ProjectConfig * pc, PropsID props)
{
	g_return_val_if_fail (pc != NULL, FALSE);
	pc->priv->win_pos_x = prop_get_int (props, "project.config.win.pos.x", 60);
	pc->priv->win_pos_y = prop_get_int (props, "project.config.win.pos.y", 60);
	pc->priv->win_width = prop_get_int (props, "project.config.win.width", 500);
	pc->priv->win_height = prop_get_int (props, "project.config.win.height", 300);
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
		project_config_hide (pc);
		/* Note: No break here */
	case GTK_RESPONSE_APPLY:
		project_config_get (pc);
		if (app->project_dbase->project_is_open)
			app->project_dbase->is_saved = FALSE;
		break;
	case GTK_RESPONSE_CANCEL:
		project_config_hide (pc);
		break;
	}
}

static gboolean
on_delete_event (GtkWidget *w, GdkEvent *event, gpointer user_data)
{
	ProjectConfig *pc = (ProjectConfig *)user_data;
	project_config_hide(pc);
	return TRUE;
}

static void
create_project_config_gui (ProjectConfig * pc)
{
	gint i;
	
	pc->priv->gxml = glade_xml_new (GLADE_FILE_ANJUTA, "project_config_dialog", NULL);
	glade_xml_signal_autoconnect (pc->priv->gxml);
	pc->priv->window = glade_xml_get_widget (pc->priv->gxml, "project_config_dialog");
	gtk_widget_hide (pc->priv->window);
	g_signal_connect (G_OBJECT (pc->priv->window), "response",
			    G_CALLBACK (on_response), pc);
	g_signal_connect (G_OBJECT (pc->priv->window), "delete_event",
				G_CALLBACK (on_delete_event), pc);

	for(i=0; i< BUILD_FILE_END_MARK; i++)
	{
		gchar *key;
		key = g_strdup_printf ("project_config_build_file_%d", i);
		pc->priv->disable_overwrite_check[i] =
			glade_xml_get_widget (pc->priv->gxml, key);
	}
	pc->priv->version_entry = glade_xml_get_widget (pc->priv->gxml, "project_config_version");
	pc->priv->target_entry = glade_xml_get_widget (pc->priv->gxml, "project_config_target");
	pc->priv->gui_editor_entry = glade_xml_get_widget (pc->priv->gxml, "project_config_gui_editor");
	pc->priv->description_text = glade_xml_get_widget (pc->priv->gxml, "project_config_description");
	pc->priv->ignore_entry = glade_xml_get_widget (pc->priv->gxml, "project_config_ignore");
	pc->priv->config_progs_text = glade_xml_get_widget (pc->priv->gxml, "project_config_programs");
	pc->priv->config_libs_text = glade_xml_get_widget (pc->priv->gxml, "project_config_libraries");
	pc->priv->config_headers_text = glade_xml_get_widget (pc->priv->gxml, "project_config_headers");
	pc->priv->config_characteristics_text = glade_xml_get_widget (pc->priv->gxml, "project_config_compiler");
	pc->priv->config_lib_funcs_text = glade_xml_get_widget (pc->priv->gxml, "project_config_functions");
	pc->priv->config_additional_text = glade_xml_get_widget (pc->priv->gxml, "project_config_additional");
	pc->priv->config_files_text = glade_xml_get_widget (pc->priv->gxml, "project_config_outputs");
	pc->priv->extra_modules_before_entry = glade_xml_get_widget (pc->priv->gxml, "project_config_modules_before");
	pc->priv->extra_modules_after_entry = glade_xml_get_widget (pc->priv->gxml, "project_config_modules_after");
	pc->priv->makefile_am_text = glade_xml_get_widget (pc->priv->gxml, "project_config_makefile_am");
}
