/*
    tags_manager.h
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

#ifndef _TAGS_MANAGER_H_
#define _TAGS_MANAGER_H_

typedef enum _TagType TagType;
typedef struct _TagItem TagItem;
typedef struct _TagFileInfo TagFileInfo;
typedef struct _TagsManager TagsManager;

enum _TagType { function_t, class_t, struct_t, union_t, enum_t, variable_t, macro_t };

struct _TagItem
{
  TagType   type;
  gchar*       tag;
  gchar*        file;
  guint         line;
};

struct _TagFileInfo
{
   gchar* filename;
   time_t  update_time;
};

struct _TagsManager
{

/* All Private */

   GList *tag_items;
   GList *file_list;
   TagType  menu_type;
   gchar *cur_file;
   gint    freeze_count;
   gint    block_count;
   gboolean  pending_update;
   gboolean  update_required_tags;
   gboolean  update_required_mems;
   GList  *tmp;

   gboolean	update_in_progress;
   guint		update_counter;
   GList            *update_file_list;
   gboolean is_saved;
};

TagsManager*
tags_manager_new(void);

void
tags_manager_destroy(TagsManager*);

void
tags_manager_block_draw(TagsManager*);

void
tags_manager_unblock_draw(TagsManager*);

void
tags_manager_freeze(TagsManager*);

void
tags_manager_thaw(TagsManager*);

gboolean
tags_manager_save (TagsManager*);

gboolean
tags_manager_load (TagsManager*);

void
tags_manager_remove(TagsManager*, gchar*filename);

void
tags_manager_clear(TagsManager*);

gboolean
tags_manager_update(TagsManager*, gchar*filename);

void
tags_manager_get_tag_info(TagsManager*, gchar* tag, char** filename, guint* line);

GList*
tags_manager_get_function_list(TagsManager*, gchar* filename);

GList*
tags_manager_get_mem_func_list(TagsManager*, gchar* classname);

GList*
tags_manager_get_tag_list(TagsManager*, TagType t);

GList*
tags_manager_get_file_list(TagsManager*);

gint
tag_item_compare(gconstpointer a, gconstpointer b);

gint
tag_file_info_compare(gconstpointer a, gconstpointer b);

gboolean
tags_manager_check_update(TagsManager* tm, gchar* filename);

void
tags_manager_update_menu(TagsManager* tm);

void
tags_manager_set_filename(TagsManager* tm, gchar *fn);

gboolean
tags_manager_update_image(TagsManager* tm, GList *files);

gboolean
on_tags_manager_on_idle(gpointer data);

void
on_tags_manager_updt_img_cancel(gpointer data);


#endif
