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
project_config_new (void)
{
	ProjectConfig *pc;
	gint i;

	pc = g_malloc (sizeof (ProjectConfig));
	if (!pc)
		return NULL;

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
	pc->extra_modules_before = NULL;
	pc->extra_modules_after = NULL;
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

	string_assign (&pc->extra_modules_before, NULL);
	string_assign (&pc->extra_modules_after, NULL);
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
	string_assign (&pc->extra_modules_before, NULL);
	string_assign (&pc->extra_modules_after, NULL);
	
	for (i=0; i < BUILD_FILE_END_MARK; i++)
		gtk_widget_unref (pc->widgets.disable_overwrite_check[i]);
	
	gtk_widget_unref (pc->widgets.description_text);
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

/* This puts the value in the struct into the widgets */
void
project_config_sync (ProjectConfig * pc)
{
	GtkWidget *entry;
	gint i;

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
	
	gtk_editable_delete_text (GTK_EDITABLE(pc->widgets.description_text), 0, -1);
	if (pc->description)
		gtk_text_insert (GTK_TEXT (pc->widgets.description_text), NULL, NULL, NULL, pc->description, -1);

	gtk_editable_delete_text (GTK_EDITABLE(pc->widgets.config_progs_text), 0, -1);
	if (pc->config_progs)
		gtk_text_insert (GTK_TEXT (pc->widgets.config_progs_text), NULL, NULL, NULL, pc->config_progs, -1);

	gtk_editable_delete_text (GTK_EDITABLE (pc->widgets.config_libs_text), 0, -1);
	if (pc->config_libs)
		gtk_text_insert (GTK_TEXT (pc->widgets.config_libs_text), NULL, NULL, NULL, pc->config_libs, -1);

	gtk_editable_delete_text (GTK_EDITABLE (pc->widgets.config_headers_text), 0, -1);
	if (pc->config_headers)
		gtk_text_insert (GTK_TEXT (pc->widgets.config_headers_text), NULL, NULL, NULL, pc->config_headers, -1);

	gtk_editable_delete_text (GTK_EDITABLE (pc->widgets.config_characteristics_text), 0, -1);
	if (pc->config_characteristics)
		gtk_text_insert (GTK_TEXT (pc->widgets.config_characteristics_text), NULL, NULL, NULL, pc->config_characteristics, -1);

	gtk_editable_delete_text (GTK_EDITABLE (pc->widgets.config_lib_funcs_text), 0, -1);
	if (pc->config_lib_funcs)
		gtk_text_insert (GTK_TEXT (pc->widgets.config_lib_funcs_text), NULL, NULL, NULL, pc->config_lib_funcs, -1);

	gtk_editable_delete_text (GTK_EDITABLE (pc->widgets.config_additional_text), 0, -1);
	if (pc->config_additional)
		gtk_text_insert (GTK_TEXT (pc->widgets.config_additional_text), NULL, NULL, NULL, pc->config_additional, -1);

	gtk_editable_delete_text (GTK_EDITABLE (pc->widgets.config_files_text), 0, -1);
	if (pc->config_files)
		gtk_text_insert (GTK_TEXT (pc->widgets.config_files_text), NULL, NULL, NULL, pc->config_files, -1);

	gtk_editable_delete_text (GTK_EDITABLE (pc->widgets.makefile_am_text), 0, -1);
	if (pc->makefile_am)
		gtk_text_insert (GTK_TEXT (pc->widgets.makefile_am_text), NULL, NULL, NULL, pc->makefile_am, -1);

	entry = pc->widgets.extra_modules_before_entry;
	if (pc->extra_modules_before)
		gtk_entry_set_text (GTK_ENTRY (entry), pc->extra_modules_before);
	else
		gtk_entry_set_text (GTK_ENTRY (entry), "");

	entry = pc->widgets.extra_modules_after_entry;
	if (pc->extra_modules_after)
		gtk_entry_set_text (GTK_ENTRY (entry), pc->extra_modules_after);
	else
		gtk_entry_set_text (GTK_ENTRY (entry), "");
}

/* This gets the value in the widgets into the struct */
void
project_config_get (ProjectConfig * pc)
{
	GtkWidget *entry;
	gint i;

	g_return_if_fail (pc != NULL);

	for (i = 0; i < BUILD_FILE_END_MARK; i++)
	{
		pc->disable_overwrite[i] = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
						      (pc->widgets.disable_overwrite_check[i]));
	}
	string_assign (&pc->description, NULL);
	pc->description =
		gtk_editable_get_chars (GTK_EDITABLE
					(pc->widgets.description_text), 0,
					-1);
	string_assign (&pc->config_progs, NULL);
	pc->config_progs =
		gtk_editable_get_chars (GTK_EDITABLE
					(pc->widgets.config_progs_text), 0,
					-1);
	string_assign (&pc->config_libs, NULL);
	pc->config_libs =
		gtk_editable_get_chars (GTK_EDITABLE
					(pc->widgets.config_libs_text), 0,
					-1);
	string_assign (&pc->config_headers, NULL);
	pc->config_headers =
		gtk_editable_get_chars (GTK_EDITABLE
					(pc->widgets.config_headers_text), 0,
					-1);
	string_assign (&pc->config_characteristics, NULL);
	pc->config_characteristics =
		gtk_editable_get_chars (GTK_EDITABLE
					(pc->widgets.
					 config_characteristics_text), 0, -1);
	string_assign (&pc->config_lib_funcs, NULL);
	pc->config_lib_funcs =
		gtk_editable_get_chars (GTK_EDITABLE
					(pc->widgets.config_lib_funcs_text),
					0, -1);
	string_assign (&pc->config_additional, NULL);
	pc->config_additional =
		gtk_editable_get_chars (GTK_EDITABLE
					(pc->widgets.config_additional_text),
					0, -1);
	string_assign (&pc->config_files, NULL);
	pc->config_files =
		gtk_editable_get_chars (GTK_EDITABLE
					(pc->widgets.config_files_text),
					0, -1);
	string_assign (&pc->makefile_am, NULL);
	pc->makefile_am =
		gtk_editable_get_chars (GTK_EDITABLE
					(pc->widgets.makefile_am_text),
					0, -1);

	entry = pc->widgets.extra_modules_before_entry;
	string_assign (&pc->extra_modules_before, NULL);
	pc->extra_modules_before =
		gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);

	entry = pc->widgets.extra_modules_after_entry;
	string_assign (&pc->extra_modules_after, NULL);
	pc->extra_modules_after =
		gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
}

/* Saves everything except the scripts in prj file */
gboolean
project_config_save (ProjectConfig * pc, FILE* stream)
{
	gint i;

	g_return_val_if_fail (pc != NULL, FALSE);
	g_return_val_if_fail (stream != NULL, FALSE);

	if (fprintf (stream, "project.config.blocked=%d\n\n", (int) pc->blocked) <1)
		return FALSE;
	
	fprintf (stream, "project.config.disable.overwriting=");
	for (i = 0; i < BUILD_FILE_END_MARK; i++)
	{
		if (fprintf (stream, "%d ", (int) pc->disable_overwrite[i]) <1)
			return FALSE;
	}
	fprintf (stream, "\n");
	
	if (pc->extra_modules_before)
	{
		if (fprintf (stream, "project.config.extra.modules.before=%s\n", pc->extra_modules_before) <1)
			return FALSE;
	}
	else
		fprintf (stream, "project.config.extra.modules.before=\n");

	if (pc->extra_modules_after)
	{
		if (fprintf (stream, "project.config.extra.modules.after=%s\n", pc->extra_modules_after) <1)
			return FALSE;
	}
	else
		fprintf (stream, "project.config.extra.modules.after=\n");
	fprintf (stream, "\n");
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
	pc->extra_modules_before = prop_get (props, "project.config.extra.modules.before");
	pc->extra_modules_after = prop_get (props, "project.config.extra.modules.after");
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
on_apply_clicked                       (GtkButton       *button,
                                        gpointer         user_data)
{
	ProjectConfig* pc;
	pc = user_data;
	project_config_get (pc);
	if (app->project_dbase->project_is_open)
		app->project_dbase->is_saved = FALSE;
}

static void
on_ok_clicked                          (GtkButton       *button,
                                        gpointer         user_data)
{
	ProjectConfig* pc;
	pc = (ProjectConfig *)user_data;
	on_apply_clicked (button, pc);
	if (NULL != pc)
		gnome_dialog_close(GNOME_DIALOG(pc->widgets.window));
}

static void
on_cancel_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
	ProjectConfig* pc;
	pc = (ProjectConfig *)user_data;
	if (NULL != pc)
		gnome_dialog_close(GNOME_DIALOG(pc->widgets.window));
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
	GtkWidget *dialog1;
	GtkWidget *dialog_vbox1;
	GtkWidget *notebook1;
	GtkWidget *frame1;
	GtkWidget *checkbutton[BUILD_FILE_END_MARK];
	GtkWidget *label8;
	GtkWidget *vbox1;
	GtkWidget *notebook2;
	GtkWidget *vbox0;
	GtkWidget *frame0;
	GtkWidget *scrolledwindow0;
	GtkWidget *text0;
	GtkWidget *label0;
	GtkWidget *frame2;
	GtkWidget *scrolledwindow1;
	GtkWidget *text1;
	GtkWidget *label2;
	GtkWidget *frame4;
	GtkWidget *scrolledwindow2;
	GtkWidget *text2;
	GtkWidget *label3;
	GtkWidget *frame5;
	GtkWidget *scrolledwindow3;
	GtkWidget *text3;
	GtkWidget *label4;
	GtkWidget *frame6;
	GtkWidget *scrolledwindow4;
	GtkWidget *text4;
	GtkWidget *label5;
	GtkWidget *frame7;
	GtkWidget *scrolledwindow5;
	GtkWidget *text5;
	GtkWidget *label6;
	GtkWidget *frame8;
	GtkWidget *scrolledwindow6;
	GtkWidget *text6;
	GtkWidget *label10;
	GtkWidget *frame10;
	GtkWidget *scrolledwindow8;
	GtkWidget *text8;
	GtkWidget *label15;
	GtkWidget *frame9;
	GtkWidget *scrolledwindow7;
	GtkWidget *text7;
	GtkWidget *label14;
	GtkWidget *hbox1;
	GtkWidget *button5;
	GtkWidget *button4;
	GtkWidget *label7;
	GtkWidget *vbox2;
	GtkWidget *label12;
	GtkWidget *eventbox1;
	GtkWidget *entry2;
	GtkWidget *hseparator1;
	GtkWidget *label13;
	GtkWidget *eventbox2;
	GtkWidget *entry3;
	GtkWidget *label9;
	GtkWidget *dialog_action_area1;
	GtkWidget *button1;
	GtkWidget *button2;
	GtkWidget *button3;
	gint i;

	dialog1 = gnome_dialog_new (_("Project configuration"), NULL);
	gtk_window_set_transient_for (GTK_WINDOW(dialog1), GTK_WINDOW(app->widgets.window));
	gtk_window_set_policy (GTK_WINDOW (dialog1), FALSE, TRUE, FALSE);
	gtk_window_set_wmclass (GTK_WINDOW (dialog1), "proj_conf", "Anjuta");
	gnome_dialog_close_hides(GNOME_DIALOG(dialog1), TRUE);

	dialog_vbox1 = GNOME_DIALOG (dialog1)->vbox;
	gtk_widget_show (dialog_vbox1);

	notebook1 = gtk_notebook_new ();
	gtk_widget_show (notebook1);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1), notebook1, TRUE, TRUE, 0);

	frame0 = gtk_frame_new (_("Project Description"));
	gtk_widget_show (frame0);
	gtk_container_add (GTK_CONTAINER (notebook1), frame0);
	gtk_container_set_border_width (GTK_CONTAINER (frame0), 5);

	vbox0 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox0);
	gtk_container_add (GTK_CONTAINER (frame0), vbox0);

	scrolledwindow0 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow0);
	gtk_container_add (GTK_CONTAINER (vbox0), scrolledwindow0);
	gtk_container_set_border_width (GTK_CONTAINER (scrolledwindow0), 5);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow0),
					GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	text0 = gtk_text_new (NULL, NULL);
	gtk_widget_show (text0);
	gtk_container_add (GTK_CONTAINER (scrolledwindow0), text0);
	gtk_text_set_editable (GTK_TEXT (text0), TRUE);

	label0 = gtk_label_new (_("General"));
	gtk_widget_show (label0);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1),
				    gtk_notebook_get_nth_page (GTK_NOTEBOOK
							       (notebook1),
							       0), label0);

	frame1 = gtk_frame_new (_("Disable overwriting Project files"));
	gtk_widget_show (frame1);
	gtk_container_add (GTK_CONTAINER (notebook1), frame1);
	gtk_container_set_border_width (GTK_CONTAINER (frame1), 5);

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox1);
	gtk_container_add (GTK_CONTAINER (frame1), vbox1);

	checkbutton[0] =
		gtk_check_button_new_with_label (_("Project configure.in"));
	gtk_widget_show (checkbutton[0]);
	gtk_box_pack_start (GTK_BOX (vbox1), checkbutton[0], FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (checkbutton[0]), 5);

	checkbutton[1] =
		gtk_check_button_new_with_label (_("Top level Makefile.am"));
	gtk_widget_show (checkbutton[1]);
	gtk_box_pack_start (GTK_BOX (vbox1), checkbutton[1], FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (checkbutton[1]), 5);

	checkbutton[2] =
		gtk_check_button_new_with_label (_("Source module Makefile.am"));
	gtk_widget_show (checkbutton[2]);
	gtk_box_pack_start (GTK_BOX (vbox1), checkbutton[2], FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (checkbutton[2]), 5);

	checkbutton[3] =
		gtk_check_button_new_with_label (_("Include module Makefile.am"));
	gtk_widget_show (checkbutton[3]);
	gtk_box_pack_start (GTK_BOX (vbox1), checkbutton[3], FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (checkbutton[3]), 5);

	checkbutton[4] =
		gtk_check_button_new_with_label (_("Help module Makefile.am"));
	gtk_widget_show (checkbutton[4]);
	gtk_box_pack_start (GTK_BOX (vbox1), checkbutton[4], FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (checkbutton[4]), 5);

	checkbutton[5] =
		gtk_check_button_new_with_label (_("Pixmap module Makefile.am"));
	gtk_widget_show (checkbutton[5]);
	gtk_box_pack_start (GTK_BOX (vbox1), checkbutton[5], FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (checkbutton[5]), 5);

	checkbutton[6] = gtk_check_button_new_with_label (_("Data module Makefile.am"));
	gtk_widget_show (checkbutton[6]);
	gtk_box_pack_start (GTK_BOX (vbox1), checkbutton[6], FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (checkbutton[6]), 5);

	checkbutton[7] = gtk_check_button_new_with_label (_("Doc module Makefile.am"));
	gtk_widget_show (checkbutton[7]);
	gtk_box_pack_start (GTK_BOX (vbox1), checkbutton[7], FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (checkbutton[7]), 5);

	checkbutton[8] = gtk_check_button_new_with_label (_("Po module (translation) Makefile.am"));
	gtk_widget_show (checkbutton[8]);
	gtk_box_pack_start (GTK_BOX (vbox1), checkbutton[8], FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (checkbutton[8]), 5);

	label8 = gtk_label_new (_("Build files"));
	gtk_widget_show (label8);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1),
				    gtk_notebook_get_nth_page (GTK_NOTEBOOK
							       (notebook1),
							       1), label8);

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox1);
	gtk_container_add (GTK_CONTAINER (notebook1), vbox1);

	notebook2 = gtk_notebook_new ();
	gtk_widget_show (notebook2);
	gtk_box_pack_start (GTK_BOX (vbox1), notebook2, TRUE, TRUE, 5);
	gtk_notebook_set_scrollable (GTK_NOTEBOOK (notebook2), TRUE);
	gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook2), GTK_POS_LEFT);
	gtk_notebook_popup_enable (GTK_NOTEBOOK (notebook2));

	frame2 = gtk_frame_new (_("Check for programs in config.in"));
	gtk_widget_show (frame2);
	gtk_container_add (GTK_CONTAINER (notebook2), frame2);
	gtk_container_set_border_width (GTK_CONTAINER (frame2), 5);

	scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow1);
	gtk_container_add (GTK_CONTAINER (frame2), scrolledwindow1);
	gtk_container_set_border_width (GTK_CONTAINER (scrolledwindow1), 5);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1),
					GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	text1 = gtk_text_new (NULL, NULL);
	gtk_widget_show (text1);
	gtk_container_add (GTK_CONTAINER (scrolledwindow1), text1);
	gtk_text_set_editable (GTK_TEXT (text1), TRUE);

	label2 = gtk_label_new (_("Program checks"));
	gtk_widget_show (label2);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook2),
				    gtk_notebook_get_nth_page (GTK_NOTEBOOK
							       (notebook2),
							       0), label2);

	frame4 = gtk_frame_new (_("Check for libraries in config.in"));
	gtk_widget_show (frame4);
	gtk_container_add (GTK_CONTAINER (notebook2), frame4);
	gtk_container_set_border_width (GTK_CONTAINER (frame4), 5);

	scrolledwindow2 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow2);
	gtk_container_add (GTK_CONTAINER (frame4), scrolledwindow2);
	gtk_container_set_border_width (GTK_CONTAINER (scrolledwindow2), 5);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow2),
					GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	text2 = gtk_text_new (NULL, NULL);
	gtk_widget_show (text2);
	gtk_container_add (GTK_CONTAINER (scrolledwindow2), text2);
	gtk_text_set_editable (GTK_TEXT (text2), TRUE);

	label3 = gtk_label_new (_("Library checks"));
	gtk_widget_show (label3);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook2),
				    gtk_notebook_get_nth_page (GTK_NOTEBOOK
							       (notebook2),
							       1), label3);

	frame5 =
		gtk_frame_new (_
			       ("Check for header files in config.in"));
	gtk_widget_show (frame5);
	gtk_container_add (GTK_CONTAINER (notebook2), frame5);
	gtk_container_set_border_width (GTK_CONTAINER (frame5), 5);

	scrolledwindow3 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow3);
	gtk_container_add (GTK_CONTAINER (frame5), scrolledwindow3);
	gtk_container_set_border_width (GTK_CONTAINER (scrolledwindow3), 5);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow3),
					GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	text3 = gtk_text_new (NULL, NULL);
	gtk_widget_show (text3);
	gtk_container_add (GTK_CONTAINER (scrolledwindow3), text3);
	gtk_text_set_editable (GTK_TEXT (text3), TRUE);

	label4 = gtk_label_new (_("Header checks"));
	gtk_widget_show (label4);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook2),
				    gtk_notebook_get_nth_page (GTK_NOTEBOOK
							       (notebook2),
							       2), label4);

	frame6 =
		gtk_frame_new (_
			       ("Checks for typedefs, structures, and compiler characteristics"));
	gtk_widget_show (frame6);
	gtk_container_add (GTK_CONTAINER (notebook2), frame6);
	gtk_container_set_border_width (GTK_CONTAINER (frame6), 5);

	scrolledwindow4 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow4);
	gtk_container_add (GTK_CONTAINER (frame6), scrolledwindow4);
	gtk_container_set_border_width (GTK_CONTAINER (scrolledwindow4), 5);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow4),
					GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	text4 = gtk_text_new (NULL, NULL);
	gtk_widget_show (text4);
	gtk_container_add (GTK_CONTAINER (scrolledwindow4), text4);
	gtk_text_set_editable (GTK_TEXT (text4), TRUE);

	label5 = gtk_label_new (_("Characteristics check"));
	gtk_widget_show (label5);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook2),
				    gtk_notebook_get_nth_page (GTK_NOTEBOOK
							       (notebook2),
							       3), label5);

	frame7 =
		gtk_frame_new (_
			       ("Check for library functions in config.in"));
	gtk_widget_show (frame7);
	gtk_container_add (GTK_CONTAINER (notebook2), frame7);
	gtk_container_set_border_width (GTK_CONTAINER (frame7), 5);

	scrolledwindow5 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow5);
	gtk_container_add (GTK_CONTAINER (frame7), scrolledwindow5);
	gtk_container_set_border_width (GTK_CONTAINER (scrolledwindow5), 5);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow5),
					GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	text5 = gtk_text_new (NULL, NULL);
	gtk_widget_show (text5);
	gtk_container_add (GTK_CONTAINER (scrolledwindow5), text5);
	gtk_text_set_editable (GTK_TEXT (text5), TRUE);

	label6 = gtk_label_new (_("Function checks"));
	gtk_widget_show (label6);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook2),
				    gtk_notebook_get_nth_page (GTK_NOTEBOOK
							       (notebook2),
							       4), label6);

	frame8 = gtk_frame_new (_("Additional scripts in config.in"));
	gtk_widget_show (frame8);
	gtk_container_add (GTK_CONTAINER (notebook2), frame8);
	gtk_container_set_border_width (GTK_CONTAINER (frame8), 5);

	scrolledwindow6 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow6);
	gtk_container_add (GTK_CONTAINER (frame8), scrolledwindow6);
	gtk_container_set_border_width (GTK_CONTAINER (scrolledwindow6), 5);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow6),
					GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	text6 = gtk_text_new (NULL, NULL);
	gtk_widget_show (text6);
	gtk_container_add (GTK_CONTAINER (scrolledwindow6), text6);
	gtk_text_set_editable (GTK_TEXT (text6), TRUE);

	label10 = gtk_label_new (_("Additional script"));
	gtk_widget_show (label10);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook2),
				    gtk_notebook_get_nth_page (GTK_NOTEBOOK
							       (notebook2),
							       5), label10);

	frame10 = gtk_frame_new (_("Files to configure"));
	gtk_widget_show (frame10);
	gtk_container_add (GTK_CONTAINER (notebook2), frame10);
	gtk_container_set_border_width (GTK_CONTAINER (frame10), 5);

	scrolledwindow8 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow8);
	gtk_container_add (GTK_CONTAINER (frame10), scrolledwindow8);
	gtk_container_set_border_width (GTK_CONTAINER (scrolledwindow8), 5);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow8),
					GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	text8 = gtk_text_new (NULL, NULL);
	gtk_widget_show (text8);
	gtk_container_add (GTK_CONTAINER (scrolledwindow8), text8);
	gtk_text_set_editable (GTK_TEXT (text8), TRUE);

	label15 = gtk_label_new (_("Files to configure"));
	gtk_widget_show (label15);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook2),
				    gtk_notebook_get_nth_page (GTK_NOTEBOOK
							       (notebook2), 6), label15);

	hbox1 = gtk_hbox_new (TRUE, 0);
	/* gtk_widget_show (hbox1); */
	gtk_box_pack_start (GTK_BOX (vbox1), hbox1, FALSE, FALSE, 0);

	button5 = gnome_stock_button (GNOME_STOCK_BUTTON_HELP);
	gtk_widget_show (button5);
	gtk_box_pack_start (GTK_BOX (hbox1), button5, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (button5), 5);

	button4 = gtk_button_new_with_label (_("Auto scan the project"));
	gtk_widget_show (button4);
	gtk_box_pack_start (GTK_BOX (hbox1), button4, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (button4), 5);

	label7 = gtk_label_new (_("Configuration scripts"));
	gtk_widget_show (label7);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1),
				    gtk_notebook_get_nth_page (GTK_NOTEBOOK
							       (notebook1),
							       2), label7);

	vbox2 = gtk_vbox_new (FALSE, 5);
	gtk_widget_show (vbox2);
	gtk_container_add (GTK_CONTAINER (notebook1), vbox2);
	gtk_container_set_border_width (GTK_CONTAINER (vbox2), 5);

	label12 =
		gtk_label_new (_
			       ("Extra modules to be built before source module:"));
	gtk_widget_show (label12);
	gtk_misc_set_alignment (GTK_MISC (label12), 0, -1);
	gtk_box_pack_start (GTK_BOX (vbox2), label12, FALSE, FALSE, 0);

	eventbox1 = gtk_event_box_new ();
	gtk_widget_show (eventbox1);
	gtk_box_pack_start (GTK_BOX (vbox2), eventbox1, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (eventbox1), 5);

	entry2 = gtk_entry_new ();
	gtk_widget_show (entry2);
	gtk_container_add (GTK_CONTAINER (eventbox1), entry2);

	hseparator1 = gtk_hseparator_new ();
	gtk_widget_show (hseparator1);
	gtk_box_pack_start (GTK_BOX (vbox2), hseparator1, FALSE, TRUE, 0);

	label13 =
		gtk_label_new (_
			       ("Extra modules to be built after source module:"));
	gtk_widget_show (label13);
	gtk_misc_set_alignment (GTK_MISC (label13), 0, -1);
	gtk_box_pack_start (GTK_BOX (vbox2), label13, FALSE, FALSE, 0);

	eventbox2 = gtk_event_box_new ();
	gtk_widget_show (eventbox2);
	gtk_box_pack_start (GTK_BOX (vbox2), eventbox2, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (eventbox2), 5);

	entry3 = gtk_entry_new ();
	gtk_widget_show (entry3);
	gtk_container_add (GTK_CONTAINER (eventbox2), entry3);

	label9 = gtk_label_new (_("Modules"));
	gtk_widget_show (label9);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1),
				    gtk_notebook_get_nth_page (GTK_NOTEBOOK
							       (notebook1),
							       3), label9);
	frame9 = gtk_frame_new (_("Extra scripts at the end of top level Makefile.am"));
	gtk_widget_show (frame9);
	gtk_container_add (GTK_CONTAINER (notebook1), frame9);
	gtk_container_set_border_width (GTK_CONTAINER (frame9), 5);

	scrolledwindow7 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow7);
	gtk_container_add (GTK_CONTAINER (frame9), scrolledwindow7);
	gtk_container_set_border_width (GTK_CONTAINER (scrolledwindow7), 5);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow7),
					GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	text7 = gtk_text_new (NULL, NULL);
	gtk_widget_show (text7);
	gtk_container_add (GTK_CONTAINER (scrolledwindow7), text7);
	gtk_text_set_editable (GTK_TEXT (text7), TRUE);

	label14 = gtk_label_new (_("Makefile.am"));
	gtk_widget_show (label14);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1),
				    gtk_notebook_get_nth_page (GTK_NOTEBOOK
							       (notebook1),  4), label14);

	dialog_action_area1 = GNOME_DIALOG (dialog1)->action_area;
	gtk_widget_show (dialog_action_area1);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1),
				   GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area1), 8);

	gnome_dialog_append_button (GNOME_DIALOG (dialog1),
				    GNOME_STOCK_BUTTON_OK);
	button1 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
	gtk_widget_show (button1);
	GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);

	gnome_dialog_append_button (GNOME_DIALOG (dialog1),
				    GNOME_STOCK_BUTTON_APPLY);
	button2 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
	gtk_widget_show (button2);
	GTK_WIDGET_SET_FLAGS (button2, GTK_CAN_DEFAULT);

	gnome_dialog_append_button (GNOME_DIALOG (dialog1),
				    GNOME_STOCK_BUTTON_CANCEL);
	button3 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
	gtk_widget_show (button3);
	GTK_WIDGET_SET_FLAGS (button3, GTK_CAN_DEFAULT);

	gtk_signal_connect (GTK_OBJECT (button1), "clicked",
			    GTK_SIGNAL_FUNC (on_ok_clicked), pc);
	gtk_signal_connect (GTK_OBJECT (button2), "clicked",
			    GTK_SIGNAL_FUNC (on_apply_clicked), pc);
	gtk_signal_connect (GTK_OBJECT (button3), "clicked",
			    GTK_SIGNAL_FUNC (on_cancel_clicked), pc);
	gtk_signal_connect (GTK_OBJECT (dialog1), "close",
				GTK_SIGNAL_FUNC (on_close), pc);

	pc->widgets.window = dialog1;
	for(i=0; i< BUILD_FILE_END_MARK; i++)
		pc->widgets.disable_overwrite_check[i] = checkbutton[i];
	pc->widgets.description_text = text0;
	pc->widgets.config_progs_text = text1;
	pc->widgets.config_libs_text = text2;
	pc->widgets.config_headers_text = text3;
	pc->widgets.config_characteristics_text = text4;
	pc->widgets.config_lib_funcs_text = text5;
	pc->widgets.config_additional_text = text6;
	pc->widgets.config_files_text = text8;
	pc->widgets.extra_modules_before_entry = entry2;
	pc->widgets.extra_modules_after_entry = entry3;
	pc->widgets.makefile_am_text = text7;

	gtk_widget_ref (pc->widgets.window);
	for (i=0; i < BUILD_FILE_END_MARK; i++)
		gtk_widget_ref (pc->widgets.disable_overwrite_check[i]);
	gtk_widget_ref (pc->widgets.description_text);
	gtk_widget_ref (pc->widgets.config_progs_text);
	gtk_widget_ref (pc->widgets.config_libs_text);
	gtk_widget_ref (pc->widgets.config_headers_text);
	gtk_widget_ref (pc->widgets.config_characteristics_text);
	gtk_widget_ref (pc->widgets.config_lib_funcs_text);
	gtk_widget_ref (pc->widgets.config_additional_text);
	gtk_widget_ref (pc->widgets.config_files_text);
	gtk_widget_ref (pc->widgets.extra_modules_before_entry);
	gtk_widget_ref (pc->widgets.extra_modules_after_entry);
	gtk_widget_ref (pc->widgets.makefile_am_text);
}


