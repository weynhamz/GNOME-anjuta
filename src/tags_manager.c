/*
    tags_manager.c
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

#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

#include <gnome.h>
#include "resources.h"
#include "anjuta.h"
#include "tags_manager.h"
#include "toolbar_callbacks.h"

#include "an_symbol_view.h"
#include "an_file_view.h"

TagsManager *
tags_manager_new (void)
{
	TagsManager *tm;
	tm = g_malloc (sizeof (TagsManager));
	if (tm)
	{
		tm->tag_items = NULL;
		tm->file_list = NULL;
		tm->update_file_list = NULL;
		tm->update_counter = 0;
		tm->update_in_progress = FALSE;
		tm->tmp = NULL;
		tm->cur_file = NULL;
		tm->menu_type = function_t;
		tm->freeze_count = 0;
		tm->block_count = 0;
		tm->pending_update = TRUE;
		tm->update_required_tags = FALSE;
		tm->update_required_mems = FALSE;
		tm->is_saved = TRUE;
	}
	return tm;
}

void
tags_manager_block_draw (TagsManager * tm)
{
	tm->block_count++;
}

void
tags_manager_unblock_draw (TagsManager * tm)
{
	tm->block_count--;
}

void
tags_manager_freeze (TagsManager * tm)
{
	tm->freeze_count++;
}

void
tags_manager_thaw (TagsManager * tm)
{
	tm->freeze_count--;
	if (tm->freeze_count < 0)
		tm->freeze_count = 0;
	if (tm->freeze_count == 0 && tm->pending_update)
		tags_manager_update_menu (tm);
}

void
tags_manager_clear (TagsManager * tm)
{
	TagItem *ti;
	TagFileInfo *file_info;
	GList *list;

	if (tm)
	{
		tags_manager_freeze (tm);
		list = tm->tag_items;
		while (list)
		{
			ti = list->data;
			if (ti->file)
				g_free (ti->file);
			if (ti->tag)
				g_free (ti->tag);
			g_free (ti);
			tm->update_required_tags = TRUE;
			tm->update_required_mems = TRUE;
			list = g_list_next (list);
		}
		list = tm->file_list;
		while (list)
		{
			file_info = list->data;
			if (file_info->filename)
				g_free (file_info->filename);
			g_free (file_info);
			tm->update_required_tags = TRUE;
			list = g_list_next (list);
		}
		list = tm->tmp;
		while (list)
		{
			g_free (list->data);
			list = g_list_next (list);
		}
		g_list_free (tm->tag_items);
		tm->tag_items = NULL;
		g_list_free (tm->file_list);
		tm->file_list = NULL;
		g_list_free (tm->tmp);
		tm->tmp = NULL;
		if (tm->cur_file)
		{
			g_free (tm->cur_file);
			tm->cur_file = NULL;
		}
		tags_manager_update_menu (tm);
		if (tm->update_file_list)
			glist_strings_free (tm->update_file_list);
		tm->update_file_list = NULL;
		tags_manager_thaw (tm);
	}
}

void
tags_manager_destroy (TagsManager * tm)
{
	if (tm)
	{
		tags_manager_clear (tm);
		g_free (tm);
	}
}

gboolean
tags_manager_save (TagsManager * tm)
{
	TagItem *ti;
	TagFileInfo *file_info;
	GList *list;
	FILE *stream;
	gchar *text;
	
	if (tm == NULL)
		return FALSE;
	if (app->project_dbase->project_is_open == FALSE)
		return FALSE;
	if (tm->is_saved == TRUE)
		return TRUE;

	text =
		g_strconcat (app->project_dbase->top_proj_dir, "/tags.cache",
			     NULL);
	stream = fopen (text, "w");
	g_free (text);
	if (stream == NULL)
		return FALSE;
	fprintf (stream,
		 "# **************************************************************************\n");
	fprintf (stream,
		 _
		 ("# *********** DO NOT EDIT OR RENAME THIS FILE *****************\n"));
	fprintf (stream,
		 _
		 ("# ******************* Created by  Anjuta ********************************\n"));
	fprintf (stream,
		 _
		 ("# ***************** Anjuta Tags cache file ********************\n"));
	fprintf (stream,
		 "# **************************************************************************\n\n");
	fprintf (stream, "Anjuta: %s\n", VERSION);

	if (fprintf (stream, "No of Tags: %u\n",
		     (unsigned) g_list_length (tm->tag_items)) < 1)
	{
		fclose (stream);
		return FALSE;
	}
	list = tm->tag_items;
	while (list)
	{
		gchar* relative_filename;
		
		ti = list->data;
		if (strncmp (ti->file,
			app->project_dbase->top_proj_dir,
			strlen (app->project_dbase->top_proj_dir)) != 0)
		{
			list = g_list_next (list);
			continue;
		}
		/* +1 is for skipping '/' */
		relative_filename = ti->file + strlen (app->project_dbase->top_proj_dir) + 1;
		if (fprintf
		    (stream, "%s %d %u %s\n", ti->tag, (int) ti->type,
		     (unsigned) ti->line, relative_filename) < 1)
		{
			fclose (stream);
			return FALSE;
		}
		list = g_list_next (list);
	}
	if (fprintf
	    (stream, "No of Tags Files: %u\n",
	     (unsigned) g_list_length (tm->file_list)) < 1)
	{
		fclose (stream);
		return FALSE;
	}
	list = tm->file_list;
	while (list)
	{
		gchar *fname;
		file_info = list->data;
		fname = file_info->filename;

		if (app->project_dbase->project_is_open == TRUE)
		{
			if (strncmp (app->project_dbase->top_proj_dir,
			     file_info->filename, strlen (app->project_dbase->top_proj_dir)) == 0)
			{
				fname =
					&(file_info->filename
					  [strlen
					   (app->project_dbase->
					    top_proj_dir) + 1]);
				if (fprintf
				    (stream, "%s %f\n", fname,
				     (float) file_info->update_time) < 1)
				{
					fclose (stream);
					return FALSE;
				}
			}
		}
		else
			if (fprintf
			    (stream, "%s %f\n", fname,
			     (float) file_info->update_time) < 1)
		{
			fclose (stream);
			return FALSE;
		}
		list = g_list_next (list);
	}
	tags_manager_update_menu (tm);
	fclose (stream);
	tm->is_saved = TRUE;
	return TRUE;
}

gboolean
tags_manager_load (TagsManager * tm)
{
	TagItem *ti;
	TagFileInfo *file_info;
	FILE *stream;
	int dummy_int;
	unsigned dummy_uint;
	float dummy_float;
	gchar *text;
	char tag_buff[512], file_buff[512];
	unsigned i, length;

	if (!tm)
		return FALSE;
	if (app->project_dbase->project_is_open == FALSE)
		return FALSE;

	text =
		g_strconcat (app->project_dbase->top_proj_dir, "/tags.cache",
			     NULL);
	stream = fopen (text, "r");
	g_free (text);
	if (stream == NULL)
		return FALSE;
	i = 0;
	while (i < 6)
	{
		char ch;
		if ((ch = fgetc (stream)) == EOF)
		{
			fclose (stream);
			return FALSE;
		}
		if (ch == '\n')
			i++;
	}
	if (fscanf (stream, "Anjuta: %s\n", tag_buff) < 1)
	{
		fclose (stream);
		return FALSE;
	}
	if (strcmp (tag_buff, VERSION) != 0)
	{
		fclose (stream);
		return FALSE;
	}

	if (fscanf (stream, "No of Tags: %u\n", &length) < 1)
	{
		fclose (stream);
		return FALSE;
	}
	tags_manager_freeze (tm);
	tags_manager_clear (tm);

	for (i = 0; i < length; i++)
	{
		if (fscanf (stream, "%s %d %u %s\n", tag_buff, &dummy_int,
		     &dummy_uint, file_buff) < 4)
		{
			tags_manager_thaw (tm);
			tags_manager_clear (tm);
			fclose (stream);
			return FALSE;
		}
		ti = g_malloc (sizeof (TagItem));
		ti->tag = g_strdup (tag_buff);
		if (strncmp (app->project_dbase->top_proj_dir,
		     file_buff, strlen (app->project_dbase->top_proj_dir)) == 0)
		{
			ti->file = g_strdup (file_buff);
		}
		else
		{
			ti->file = g_strconcat (app->project_dbase->top_proj_dir, "/", file_buff, NULL);
		}
		ti->type = (TagType) dummy_int;
		ti->line = dummy_uint;
		tm->tag_items = g_list_append (tm->tag_items, ti);
	}
	if (fscanf (stream, "No of Tags Files: %u\n", &length) < 1)
	{
		tags_manager_thaw (tm);
		tags_manager_clear (tm);
		fclose (stream);
		return FALSE;
	}
	for (i = 0; i < length; i++)
	{
		if (fscanf (stream, "%s %f\n", file_buff, &dummy_float) < 2)
		{
			tags_manager_thaw (tm);
			tags_manager_clear (tm);
			fclose (stream);
			return FALSE;
		}
		file_info = g_malloc (sizeof (TagFileInfo));
		file_info->filename = g_strconcat (app->project_dbase->top_proj_dir,
					     "/", file_buff, NULL);
		file_info->update_time = (time_t) dummy_float;
		tm->file_list = g_list_append (tm->file_list, file_info);
	}
	tm->update_required_tags = TRUE;
	tm->update_required_mems = TRUE;
	tags_manager_update_menu (tm);
	tags_manager_thaw (tm);
	fclose (stream);
	tm->is_saved = TRUE;
	return TRUE;
}

static void
remove_file (TagsManager * tm, gchar * filename)
{
	TagItem *ti;
	TagFileInfo *file_info;
	GList *list;

	tags_manager_freeze (tm);
	list = tm->file_list;
	while (list)
	{
		file_info = list->data;
		list = g_list_next (list);
		if (strcmp (file_info->filename, filename) == 0)
		{
			tm->file_list =
				g_list_remove (tm->file_list, file_info);
			if (file_info->filename)
				g_free (file_info->filename);
			g_free (file_info);
			tm->update_required_tags = TRUE;
		}
	}
	if (g_list_length (tm->file_list) == 0 && tm->cur_file)
	{
		g_free (tm->cur_file);
		tm->cur_file = NULL;
		tm->update_required_tags = TRUE;
		tm->update_required_mems = TRUE;
	}

	list = tm->tag_items;
	while (list)
	{
		ti = list->data;
		list = g_list_next (list);
		if (strcmp (ti->file, filename) == 0)
		{
			tm->tag_items = g_list_remove (tm->tag_items, ti);
			if (ti->file)
				g_free (ti->file);
			if (ti->tag)
				g_free (ti->tag);
			g_free (ti);
			tm->update_required_mems = TRUE;
		}
	}
	tags_manager_update_menu (tm);
	tags_manager_thaw (tm);
}

void
tags_manager_remove (TagsManager * tm, gchar * filename)
{
	gchar* newfname;
	gboolean swap_opened;

	swap_opened = FALSE;
	newfname = get_swapped_filename(filename);
	if (newfname)
	{
		GList *node;
		node = app->text_editor_list;
		while (node)
		{
			TextEditor *te;
			te = node->data;
			if (te->full_filename)
			{
				if (strcmp (te->full_filename, newfname)==0)
				{
					swap_opened = TRUE;
					break;
				}
			}
			node = g_list_next(node);
		}
	}
	if (swap_opened == FALSE)
	{
		tags_manager_freeze (tm);
		remove_file (tm, filename);
		if(newfname)
			remove_file (tm, newfname);
		tags_manager_thaw (tm);
	}
	if (newfname)
		g_free (newfname);
}

void
tags_manager_update_menu (TagsManager * tm)
{
	if (tm->block_count > 0)
		return;
	if (tm->freeze_count > 0)
	{
		tm->pending_update = TRUE;
		return;
	}
	gtk_signal_disconnect_by_func (GTK_OBJECT
				       (app->widgets.toolbar.
					tags_toolbar.tag_entry),
				       GTK_SIGNAL_FUNC
				       (on_tag_combo_entry_changed), NULL);
	gtk_signal_disconnect_by_func (GTK_OBJECT
				       (app->widgets.toolbar.
					tags_toolbar.member_entry),
				       GTK_SIGNAL_FUNC
				       (on_member_combo_entry_changed), NULL);
	if (tm->menu_type == function_t)
	{
		gtk_label_set_text (GTK_LABEL
				    (app->widgets.toolbar.tags_toolbar.
				     tag_label), _("File:"));
		gtk_label_set_text (GTK_LABEL
				    (app->widgets.toolbar.tags_toolbar.
				     member_label), _("Function:"));
		if (tm->update_required_tags)
		{
			gtk_combo_set_popdown_strings (GTK_COMBO
						       (app->widgets.toolbar.
							tags_toolbar.tag_combo),
						       tags_manager_get_file_list
						       (tm));
			tm->update_required_tags = FALSE;
		}
		if (tm->cur_file)
			gtk_entry_set_text (GTK_ENTRY
					    (app->widgets.toolbar.
					     tags_toolbar.tag_entry),
					    tm->cur_file);
		if (tm->update_required_mems)
		{
			gtk_combo_set_popdown_strings (GTK_COMBO
						       (app->widgets.toolbar.
							tags_toolbar.member_combo),
						       tags_manager_get_function_list
						       (tm,
							gtk_entry_get_text
							(GTK_ENTRY
							 (app->widgets.toolbar.tags_toolbar.tag_entry))));
			tm->update_required_mems = FALSE;
		}
		gtk_widget_show (app->widgets.toolbar.tags_toolbar.
				 member_combo);
		gtk_widget_show (app->widgets.toolbar.tags_toolbar.
				 member_label);
		gtk_widget_show (app->widgets.toolbar.tags_toolbar.toolbar);
	}
	else if (tm->menu_type == class_t)
	{
		gtk_label_set_text (GTK_LABEL
				    (app->widgets.toolbar.tags_toolbar.
				     tag_label), _("Class:"));
		gtk_label_set_text (GTK_LABEL
				    (app->widgets.toolbar.tags_toolbar.
				     member_label), _("Mem Func:"));
		if (tm->update_required_tags)
		{
			gtk_combo_set_popdown_strings (GTK_COMBO
						       (app->widgets.toolbar.
							tags_toolbar.tag_combo),
						       tags_manager_get_tag_list
						       (tm, class_t));
			tm->update_required_tags = FALSE;
		}
		if (tm->update_required_mems)
		{
			gtk_combo_set_popdown_strings (GTK_COMBO
						       (app->widgets.toolbar.
							tags_toolbar.member_combo),
						       tags_manager_get_mem_func_list
						       (tm,
							gtk_entry_get_text
							(GTK_ENTRY
							 (app->widgets.toolbar.tags_toolbar.tag_entry))));
			tm->update_required_mems = FALSE;
		}
		gtk_widget_show (app->widgets.toolbar.tags_toolbar.
				 member_combo);
		gtk_widget_show (app->widgets.toolbar.tags_toolbar.
				 member_label);
		gtk_widget_queue_resize (app->widgets.toolbar.tags_toolbar.toolbar);
	}
	else
	{
		if (tm->update_required_tags)
		{
			gtk_combo_set_popdown_strings (GTK_COMBO
						       (app->widgets.toolbar.
							tags_toolbar.tag_combo),
						       tags_manager_get_tag_list
						       (tm, tm->menu_type));
			tm->update_required_tags = FALSE;
		}
		gtk_label_set_text (GTK_LABEL
				    (app->widgets.toolbar.tags_toolbar.
				     tag_label), _("Tag:"));
		gtk_widget_hide (app->widgets.toolbar.tags_toolbar.
				 member_combo);
		gtk_widget_hide (app->widgets.toolbar.tags_toolbar.
				 member_label);
		gtk_widget_queue_resize (app->widgets.toolbar.tags_toolbar.toolbar);
	}
	gtk_signal_connect (GTK_OBJECT
			    (app->widgets.toolbar.tags_toolbar.tag_entry),
			    "changed",
			    GTK_SIGNAL_FUNC (on_tag_combo_entry_changed),
			    NULL);
	gtk_signal_connect (GTK_OBJECT
			    (app->widgets.toolbar.tags_toolbar.member_entry),
			    "changed",
			    GTK_SIGNAL_FUNC (on_member_combo_entry_changed),
			    NULL);
	tm->pending_update = FALSE;
}

static gboolean
update_using_ctags (TagsManager * tm, gchar * filename)
{
	TagItem *ti;
	TagFileInfo *file_info;
	int stdout_pipe[2], pid;
	FILE *so_file;
	int status;
	gchar buffer[FILE_BUFFER_SIZE];
	gchar tag_buff[512], type_buff[256], file_buff[512];
	guint line;

	/*
	if (tags_manager_check_update (tm, filename) == TRUE)
		return TRUE;
	*/
	if (anjuta_is_installed ("ctags", FALSE) == FALSE)
		return FALSE;

	if (app->project_dbase->project_is_open == TRUE)
		tm->is_saved = FALSE;

	tags_manager_freeze (tm);
	remove_file (tm, filename);

	pipe (stdout_pipe);
	if ((pid = fork ()) == 0)
	{
		close (1);
		close (2);	// no error output on screen.
		dup (stdout_pipe[1]);
		close (stdout_pipe[0]);
		execlp ("ctags", "ctags", "-x", "--c-types=+C", filename,
			NULL);
		g_error (_("Cannot execute ctags"));
	}
	close (stdout_pipe[1]);
	if (pid < 0)
	{
		anjuta_system_error (errno, _("Cannot fork to execute ctags.\nIn %s:%d"),
					 __FILE__, __LINE__);
		close (stdout_pipe[0]);
		return FALSE;
	}
	so_file = fdopen (stdout_pipe[0], "r");
	if (so_file == NULL)
	{
		tags_manager_thaw (tm);
		return FALSE;
	}
	while (fgets(buffer, FILE_BUFFER_SIZE, so_file))
	{
		if (ferror(so_file) || feof(so_file))
			break;
		if (sscanf(buffer, "%s %s %d %s", tag_buff, type_buff, &line, file_buff) != 4)
			continue;
		ti = g_malloc (sizeof (TagItem));
		if (strcmp (type_buff, "function") == 0)
			ti->type = function_t;
		else if (strcmp (type_buff, "class") == 0)
			ti->type = class_t;
		else if (strcmp (type_buff, "struct") == 0)
			ti->type = struct_t;
		else if (strcmp (type_buff, "union") == 0)
			ti->type = union_t;
		else if (strcmp (type_buff, "enum") == 0)
			ti->type = enum_t;
		else if (strcmp (type_buff, "variable") == 0)
			ti->type = variable_t;
		else if (strcmp (type_buff, "macro") == 0)
			ti->type = macro_t;
		else
		{
			g_free (ti);
			continue;
		}
		ti->file = g_strdup (file_buff);
		ti->tag = g_strdup (tag_buff);
		ti->line = line;
		tm->tag_items =
			g_list_insert_sorted (tm->tag_items, ti,
					      tag_item_compare);
	}
	waitpid (pid, &status, 0);
	file_info = g_malloc (sizeof (TagFileInfo));
	file_info->filename = g_strdup (filename);
	file_info->update_time = time (NULL);
	tm->file_list =
		g_list_insert_sorted (tm->file_list, file_info,
				      tag_file_info_compare);
	tm->update_required_tags = TRUE;
	tm->update_required_mems = TRUE;
	fclose (so_file);
	tags_manager_update_menu (tm);
	tags_manager_thaw (tm);
	return TRUE;
}

gboolean
tags_manager_update (TagsManager * tm, gchar * filename)
{
	gchar* newfname;
	gboolean ret;

	tags_manager_freeze (tm);
	if (app->project_dbase->project_is_open == FALSE)
	{
		newfname = get_swapped_filename (filename);
		if (newfname)
		{
			update_using_ctags (tm, newfname);
			g_free (newfname);
		}
	}
	ret = update_using_ctags (tm, filename);
	tags_manager_thaw(tm);
	return ret;
}

gboolean
tags_manager_update_image (TagsManager * tm, GList * files)
{
	GList* node;
	
	if (tm == NULL)
		return FALSE;
	if (files == NULL)
		return TRUE;
	if (tm->update_in_progress)
		return FALSE;

	/*
	if (g_list_length (files) == g_list_length (tm->file_list))
		return TRUE;
	tags_manager_clear (tm);
	*/
	tags_manager_freeze (tm);
	tm->update_file_list = NULL;
	node = files;

	for (node = files; node; node = g_list_next (node)) {
		gchar *fn = anjuta_get_full_filename (node->data);

		if (!fn)
			continue;

		if (tags_manager_check_update (tm, fn) == FALSE)
			tm->update_file_list = 
				g_list_append(tm->update_file_list, g_strdup(node->data));

		g_free (fn);
	}

	if (tm->update_file_list)
	{
		tm->update_counter = 0;
		tm->update_in_progress = TRUE;
	
		anjuta_init_progress (_("Synchronizing and updating tags image ... please wait"),
			g_list_length (files), on_tags_manager_updt_img_cancel, tm);
	
		gtk_idle_add (on_tags_manager_on_idle, tm);
	}
	else
	{
		tags_manager_thaw(tm);
		tm_project_update(app->project_dbase->tm_project, FALSE
		  , TRUE, TRUE);

		sv_populate();
		fv_populate();
	}

	return TRUE;
}

gboolean
on_tags_manager_on_idle (gpointer data)
{
	gchar *fn;
	TagsManager *tm = data;
	fn = NULL;
	if (tm == NULL)
		goto error;
	if (tm->update_in_progress == FALSE)
		goto error;

	if (tm->update_counter >= g_list_length (tm->update_file_list))
	{
		tags_manager_thaw (tm);
		anjuta_done_progress (_("Updating tags image completed"));
		glist_strings_free (tm->update_file_list);
		tm->update_in_progress = FALSE;
		tm->update_file_list = NULL;
		tags_manager_save(tm);
		tm_project_update(app->project_dbase->tm_project, FALSE
		  , TRUE, TRUE);

		sv_populate();
		fv_populate();

		return FALSE;
	}
	if (app->project_dbase->project_is_open == FALSE)
		goto error;

	fn =
		anjuta_get_full_filename (g_list_nth_data
					  (tm->update_file_list,
					   tm->update_counter));
	if (!fn)
		goto error;

	if (tags_manager_update (tm, fn) == FALSE)
		goto error;

	g_free (fn);
	tm->update_counter++;
	anjuta_set_progress (tm->update_counter);

	return TRUE;

      error:
	if (fn)
		g_free (fn);
	tags_manager_thaw (tm);
	anjuta_done_progress (_("Synchronization of tags image failed"));
	glist_strings_free (tm->update_file_list);
	tm->update_file_list = NULL;
	tm->update_in_progress = FALSE;

	return FALSE;
}

void
on_tags_manager_updt_img_cancel (gpointer data)
{
	TagsManager *tm = data;
	if (tm == NULL)
		return;
	if (tm->update_in_progress == FALSE)
		return;
	tm->update_counter = 0;
	tags_manager_thaw (tm);
	glist_strings_free (tm->update_file_list);
	tm->update_file_list = NULL;
	tm->update_in_progress = FALSE;
}

gint
tag_item_compare (gconstpointer a, gconstpointer b)
{
	return strcmp (((TagItem *) a)->tag, ((TagItem *) b)->tag);
}

gint
tag_file_info_compare (gconstpointer a, gconstpointer b)
{
	return strcmp (extract_filename (((TagFileInfo *) a)->filename),
		       extract_filename (((TagFileInfo *) b)->filename));
}

gboolean
tags_manager_check_update (TagsManager * tm, gchar * filename)
{
	struct stat f_st;
	TagFileInfo *file_info;
	GList *list;

	list = tm->file_list;
	while (list)
	{
		file_info = list->data;
		if (strcmp (file_info->filename, filename) == 0)
		{
			if (stat (filename, &f_st) != 0)
				return FALSE;
			if (f_st.st_mtime > file_info->update_time)
				return FALSE;
			else
				return TRUE;
		}
		list = g_list_next (list);
	}
	return FALSE;
}

void
tags_manager_get_tag_info (TagsManager * tm, gchar * tag, char **filename,
			   guint * line)
{
	TagItem *ti;
	GList *list;

	list = tm->tag_items;
	while (list)
	{
		ti = list->data;
		if (strcmp (ti->tag, tag) == 0)
		{
			*filename = g_strdup (ti->file);
			*line = ti->line;
			return;
		}
		list = g_list_next (list);
	}
	*filename = NULL;
	*line = 0;
}

GList *
tags_manager_get_tag_list (TagsManager * tm, TagType t)
{
	TagItem *ti;
	GList *list;

	list = tm->tmp;
	while (list)
	{
		g_free (list->data);
		list = g_list_next (list);
	}
	g_list_free (tm->tmp);
	tm->tmp = NULL;

	list = tm->tag_items;
	while (list)
	{
		ti = list->data;
		if (ti->type == t)
			tm->tmp =
				g_list_insert_sorted (tm->tmp,
						      g_strdup (ti->tag),
						      compare_string_func);
		list = g_list_next (list);
	}
	if (tm->tmp == NULL)
		tm->tmp = g_list_append (tm->tmp, g_strdup (""));
	return tm->tmp;
}

GList *
tags_manager_get_file_list (TagsManager * tm)
{
	TagFileInfo *file_info;
	GList *list;

	list = tm->tmp;
	while (list)
	{
		g_free (list->data);
		list = g_list_next (list);
	}
	g_list_free (tm->tmp);
	tm->tmp = NULL;

	list = tm->file_list;
	while (list)
	{
		file_info = list->data;
		switch (get_file_ext_type (file_info->filename))
		{
		case FILE_TYPE_C:
		case FILE_TYPE_CPP:
			tm->tmp = g_list_insert_sorted (tm->tmp,
							g_strdup
							(extract_filename
							 (file_info->
							  filename)),
							compare_string_func);
			break;
		default:
		}
		list = g_list_next (list);
	}

	if (tm->tmp == NULL)
		tm->tmp = g_list_append (tm->tmp, g_strdup (""));
	return tm->tmp;
}

GList *
tags_manager_get_function_list (TagsManager * tm, gchar * filename)
{
	TagItem *ti;
	GList *list;

	list = tm->tmp;
	while (list)
	{
		g_free (list->data);
		list = g_list_next (list);
	}
	g_list_free (tm->tmp);
	tm->tmp = NULL;

	list = tm->tag_items;
	while (list)
	{
		ti = list->data;
		if (ti->type == function_t
		    && (strcmp (extract_filename (ti->file), filename) == 0))
			tm->tmp =
				g_list_insert_sorted (tm->tmp,
						      g_strdup (ti->tag),
						      compare_string_func);
		list = g_list_next (list);
	}

	if (tm->tmp == NULL)
		tm->tmp = g_list_append (tm->tmp, g_strdup (""));
	return tm->tmp;
}

GList *
tags_manager_get_mem_func_list (TagsManager * tm, gchar * classname)
{
	TagItem *ti;
	gchar *buff;
	GList *list;

	list = tm->tmp;
	while (list)
	{
		g_free (list->data);
		list = g_list_next (list);
	}
	g_list_free (tm->tmp);
	tm->tmp = NULL;

	list = tm->tag_items;
	while (list)
	{
		ti = list->data;
		if (ti->type == function_t)
		{
			buff = g_strconcat (classname, "::", NULL);
			if (strncmp
			    (buff, ti->tag,
			     strlen (buff) * sizeof (char)) == 0)
				tm->tmp =
					g_list_insert_sorted (tm->tmp,
							      g_strdup (
									(ti->
									 tag)
									+
									strlen
									(buff)
									*
									sizeof
									(char)),
							      compare_string_func);
			g_free (buff);
		}
		list = g_list_next (list);
	}

	if (tm->tmp == NULL)
		tm->tmp = g_list_append (tm->tmp, g_strdup (""));
	return tm->tmp;
}

void
tags_manager_set_filename (TagsManager * tm, gchar * fn)
{
	TagFileInfo *file_info;
	GList *list;

	if (tm && fn)
	{
		if (tm->cur_file)
		{
#ifdef DEBUG
			g_message("Files are %s and %s", tm->cur_file, fn);
#endif
			if (strcmp (tm->cur_file, extract_filename (fn)) == 0)
				return;
		}

		list = tm->file_list;
		while (list)
		{
			file_info = list->data;
			list = g_list_next (list);
			if (strcmp (file_info->filename, fn) == 0)
			{
				switch (get_file_ext_type
					(file_info->filename))
				{
				case FILE_TYPE_C:
				case FILE_TYPE_CPP:
					if (tm->cur_file)
						g_free (tm->cur_file);
					tm->cur_file =
						g_strdup (extract_filename
							  (fn));
					if (tm->menu_type == function_t)
					{
						tm->update_required_mems =
							TRUE;
						tags_manager_update_menu (tm);
					}
					return;
				default:
				}
				return;
			}
		}
	}
}
