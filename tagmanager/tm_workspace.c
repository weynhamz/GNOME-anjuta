/*
*
*   Copyright (c) 2001-2002, Biswapesh Chattopadhyay
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*/

#include "general.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <string.h>
#include <glob.h>
#include <sys/stat.h>

#include "tm_tag.h"
#include "tm_workspace.h"
#include "tm_project.h"


static TMWorkspace *theWorkspace = NULL;
guint workspace_class_id = 0;

static gboolean tm_create_workspace(void)
{
	char *file_name = g_strdup_printf("%s/anjuta_%ld.%d",
									  P_tmpdir, time (NULL), getpid());
#ifdef TM_DEBUG
	g_message("Workspace created: %s", file_name);
#endif

	workspace_class_id = tm_work_object_register(tm_workspace_free, tm_workspace_update
		  , tm_workspace_find_object);
	theWorkspace = g_new(TMWorkspace, 1);
	if (FALSE == tm_work_object_init(TM_WORK_OBJECT(theWorkspace),
		  workspace_class_id, file_name, TRUE))
	{
		g_free(file_name);
		g_free(theWorkspace);
		theWorkspace = NULL;
		g_warning("Failed to initialize workspace");
		return FALSE;
	}

	g_free(file_name);
	theWorkspace->global_tags = NULL;
	theWorkspace->work_objects = NULL;
	return TRUE;
}

void tm_workspace_free(gpointer workspace)
{
	guint i;

	if (workspace != theWorkspace)
		return;

#ifdef TM_DEBUG
	g_message("Workspace destroyed");
#endif

	if (theWorkspace)
	{
		if (theWorkspace->work_objects)
		{
			for (i=0; i < theWorkspace->work_objects->len; ++i)
				tm_work_object_free(theWorkspace->work_objects->pdata[i]);
			g_ptr_array_free(theWorkspace->work_objects, TRUE);
		}
		if (theWorkspace->global_tags)
		{
			for (i=0; i < theWorkspace->global_tags->len; ++i)
				tm_tag_free(theWorkspace->global_tags->pdata[i]);
			g_ptr_array_free(theWorkspace->global_tags, TRUE);
		}
		unlink(theWorkspace->work_object.file_name);
		tm_work_object_destroy(TM_WORK_OBJECT(theWorkspace));
		g_free(theWorkspace);
		theWorkspace = NULL;
	}
}

const TMWorkspace *tm_get_workspace(void)
{
	if (NULL == theWorkspace)
		tm_create_workspace();
	return theWorkspace;
}

gboolean tm_workspace_add_object(TMWorkObject *work_object)
{
	if (NULL == theWorkspace)
		tm_create_workspace();
	if (NULL == theWorkspace->work_objects)
		theWorkspace->work_objects = g_ptr_array_new();
	g_ptr_array_add(theWorkspace->work_objects, work_object);
	work_object->parent = TM_WORK_OBJECT(theWorkspace);
	return TRUE;
}

gboolean tm_workspace_remove_object(TMWorkObject *w, gboolean free)
{
	guint i;
	if ((NULL == theWorkspace) || (NULL == theWorkspace->work_objects)
		  || (NULL == w))
		return FALSE;
	for (i=0; i < theWorkspace->work_objects->len; ++i)
	{
		if (theWorkspace->work_objects->pdata[i] == w)
		{
			if (free)
				tm_work_object_free(w);
			g_ptr_array_remove_index_fast(theWorkspace->work_objects, i);
			tm_workspace_update(TM_WORK_OBJECT(theWorkspace), TRUE, FALSE, FALSE);
			return TRUE;
		}
	}
	return FALSE;
}

static GPtrArray* tm_workspace_load_tags (GPtrArray* array, const char *tags_file)
{
	FILE *fp;
	TMTag *tag;
	GPtrArray *tags;

	if (NULL == (fp = fopen(tags_file, "r")))
		return NULL;
	if (array)
		tags = array;
	else
		tags = g_ptr_array_new();
	while (NULL != (tag = tm_tag_new_from_file(NULL, fp)))
		g_ptr_array_add(tags, tag);
	fclose(fp);
	return tags;
}

gboolean tm_workspace_load_global_tags(const char *tags_file)
{
	if (NULL == theWorkspace)
		tm_create_workspace();
	if (NULL == theWorkspace->global_tags)
	{
		theWorkspace->global_tags = tm_workspace_load_tags (NULL, tags_file);
		if (theWorkspace->global_tags)
			return TRUE;
		else
			return FALSE;
	}
	else
	{
		if (tm_workspace_load_tags (theWorkspace->global_tags, tags_file))
			return TRUE;
		else
			return FALSE;
	}
}

gboolean tm_workspace_create_global_tags(const char *pre_process, const char **includes
  , int includes_count, const char *tags_file)
{
	glob_t globbuf;
	int idx_inc;
	int idx_glob;
	char *command;
	guint i;
	FILE *fp;
	TMWorkObject *source_file;
	GPtrArray *tags_array;
	GList *includes_files = NULL;
	int list_len;
	int idx_main;
	int idx_sub;
	int remove_count = 0;
	char *temp_file = g_strdup_printf("%s/%d_%ld_1.cpp", P_tmpdir, getpid(), time(NULL));
	char *temp_file2 = g_strdup_printf("%s/%d_%ld_2.cpp", P_tmpdir, getpid(), time(NULL));
	TMTagAttrType sort_attrs[] = { tm_tag_attr_name_t, tm_tag_attr_scope_t
		, tm_tag_attr_type_t, 0};
	if (NULL == (fp = fopen(temp_file, "w")))
		return FALSE;
	
	globbuf.gl_offs = 0;
	for(idx_inc = 0; idx_inc < includes_count; idx_inc++)
	{
 		int dirty_len = strlen(includes[idx_inc]);
		char *clean_path = malloc(dirty_len - 1);
		strncpy(clean_path, includes[idx_inc] + 1, dirty_len - 1);
		clean_path[dirty_len - 2] = 0;

		//printf("[o][%s]\n", clean_path);
		glob(clean_path, 0, NULL, &globbuf);
		//printf("matches: %d\n", globbuf.gl_pathc);
		for(idx_glob = 0; idx_glob < globbuf.gl_pathc; idx_glob++)
		{
			includes_files = g_list_append(includes_files, strdup(globbuf.gl_pathv[idx_glob]));
			//printf(">>> %s\n", globbuf.gl_pathv[idx_glob]);
		}
		globfree(&globbuf);
		free(clean_path);
  	}


	/* Checks for duplicate file entries which would case trouble */
	{
		struct stat main_stat;
		struct stat sub_stat;

		remove_count = 0;
		
		list_len = g_list_length(includes_files);

		/* We look for files with the same inode */
		for(idx_main = 0; idx_main < list_len; idx_main++)
		{
//			printf("%d / %d\n", idx_main, list_len - 1);
			stat(g_list_nth_data(includes_files, idx_main), &main_stat);
			for(idx_sub = idx_main + 1; idx_sub < list_len; idx_sub++)
			{
				GList *element = NULL;
				
				stat(g_list_nth_data(includes_files, idx_sub), &sub_stat);
				
				
				if(main_stat.st_ino != sub_stat.st_ino)
					continue;
			
				/* Inodes match */
				
				element = g_list_nth(includes_files, idx_sub);
					
/*				printf("%s == %s\n", g_list_nth_data(includes_files, idx_main), 
										 g_list_nth_data(includes_files, idx_sub)); */
					
				/* We delete the duplicate entry from the list */
				includes_files = g_list_remove_link(includes_files, element);
				remove_count++;

				/* Don't forget to free the mallocs (we duplicated every string earlier!) */
				free(element->data);

				idx_sub--; /* Cause the inner loop not to move; good since we removed 
							   an element at the current position; we don't have to worry
							   about the outer loop because the inner loop always starts
							   after the outer loop's index */

				list_len = g_list_length(includes_files);
			}
		}
	}


	printf("writing out files to %s\n", temp_file);
	for(idx_main = 0; idx_main < g_list_length(includes_files); idx_main++)
	{
		char *str = g_strdup_printf("#include \"%s\"\n",
									(char*)g_list_nth_data(includes_files,
														   idx_main));
		int str_len = strlen(str);

		fwrite(str, str_len, 1, fp);

		free(str);
		free(g_list_nth(includes_files, idx_main) -> data);
	}
	
	fclose(fp);

	command = g_strdup_printf("%s %s >%s", pre_process, temp_file, temp_file2);
#ifdef TM_DEBUG
	g_message("Executing: %s", command);
#endif
	system(command);
	g_free(command);
	unlink(temp_file);
	g_free(temp_file);
	source_file = tm_source_file_new(temp_file2, TRUE);
	if (NULL == source_file)
	{
		unlink(temp_file2);
		return FALSE;
	}
	unlink(temp_file2);
	g_free(temp_file2);
	if ((NULL == source_file->tags_array) || (0 == source_file->tags_array->len))
	{
		tm_source_file_free(source_file);
		return FALSE;
	}
	
	/*
	tags_array = tm_tags_extract(source_file->tags_array, tm_tag_class_t |
	  tm_tag_typedef_t | tm_tag_prototype_t | tm_tag_enum_t | tm_tag_macro_with_arg_t);
	*/
	tags_array = tm_tags_extract(source_file->tags_array, tm_tag_max_t);
	
	if ((NULL == tags_array) || (0 == tags_array->len))
	{
		if (tags_array)
			g_ptr_array_free(tags_array, TRUE);
		tm_source_file_free(source_file);
		return FALSE;
	}
	if (FALSE == tm_tags_sort(tags_array, sort_attrs, TRUE))
	{
		tm_source_file_free(source_file);
		return FALSE;
	}
	if (NULL == (fp = fopen(tags_file, "w")))
	{
		tm_source_file_free(source_file);
		return FALSE;
	}
	for (i=0; i < tags_array->len; ++i)
	{
		tm_tag_write(TM_TAG(tags_array->pdata[i]), fp, (tm_tag_attr_type_t
		  | tm_tag_attr_scope_t | tm_tag_attr_arglist_t | tm_tag_attr_vartype_t | tm_tag_attr_pointer_t));
	}
	fclose(fp);
	tm_source_file_free(source_file);
	g_ptr_array_free(tags_array, TRUE);
	return TRUE;
}

static gboolean
str_has_suffix (const char *haystack, const char *needle)
{
	const char *h, *n;

	if (needle == NULL) {
		return TRUE;
	}
	if (haystack == NULL) {
		return needle[0] == '\0';
	}
		
	/* Eat one character at a time. */
	h = haystack + strlen(haystack);
	n = needle + strlen(needle);
	do {
		if (n == needle) {
			return TRUE;
		}
		if (h == haystack) {
			return FALSE;
		}
	} while (*--h == *--n);
	return FALSE;
}

/* Merges a set of tags files. Note that the passed files are gzipped files */
gboolean tm_workspace_merge_global_tags (const gchar *output_file, 
										 GList *tag_files)
{
	int i;
	FILE *fp;
	GList *node;
	GPtrArray *merged_tags, *dedupped_tags;
	TMTagAttrType sort_attrs[] =
	{
		tm_tag_attr_name_t, tm_tag_attr_scope_t,
		tm_tag_attr_type_t, 0
	};
	
	if (!tag_files || !output_file)
		return FALSE;
	
	merged_tags = g_ptr_array_sized_new (15000);
	node = tag_files;
	while (node)
	{
		char *temp_file;
		
		if (str_has_suffix ((const char*) node->data, ".gz"))
		{
			char *command;
			temp_file = g_strdup_printf("%s/%d_%ld_1.anjutatags",
										P_tmpdir, getpid(), time(NULL));
			command = g_strdup_printf ("gunzip -c '%s' > %s",
									   (const char*) node->data,
									   temp_file);
			system(command);
			g_free (command);
			tm_workspace_load_tags (merged_tags, temp_file);
			unlink (temp_file);
			g_free (temp_file);
		}
		else
		{
			tm_workspace_load_tags (merged_tags, (const char*)node->data);
		}
		node = g_list_next (node);
	}
	if (merged_tags->len <= 0)
	{
		g_ptr_array_free (merged_tags, TRUE);
		return FALSE;
	}
	
	/* Sort the tags */
	dedupped_tags = tm_tags_extract(merged_tags, tm_tag_attr_max_t);

	if (FALSE == tm_tags_sort(dedupped_tags, sort_attrs, TRUE))
	{
		g_ptr_array_free (dedupped_tags, TRUE);
		tm_tags_array_free (merged_tags, TRUE);
		tm_tag_chunk_clean ();
		return FALSE;
	}
	
	/* Save the output */
	if (NULL == (fp = fopen(output_file, "w")))
	{
		g_ptr_array_free (dedupped_tags, TRUE);
		tm_tags_array_free (merged_tags, TRUE);
		tm_tag_chunk_clean ();
		return FALSE;
	}
	for (i=0; i < dedupped_tags->len; ++i)
	{
		tm_tag_write(TM_TAG(dedupped_tags->pdata[i]), fp,
					 (tm_tag_attr_type_t | tm_tag_attr_scope_t |
					  tm_tag_attr_arglist_t | tm_tag_attr_vartype_t |
					  tm_tag_attr_pointer_t));
	}
	fclose(fp);
	g_ptr_array_free (dedupped_tags, TRUE);
	tm_tags_array_free (merged_tags, TRUE);
	tm_tag_chunk_clean ();
	return TRUE;
}

TMWorkObject *tm_workspace_find_object(TMWorkObject *work_object, const char *file_name
  , gboolean name_only)
{
	TMWorkObject *w = NULL;
	guint i;

	if (work_object != TM_WORK_OBJECT(theWorkspace))
		return NULL;
	if ((NULL == theWorkspace) || (NULL == theWorkspace->work_objects)
		|| (0 == theWorkspace->work_objects->len))
		return NULL;
	for (i = 0; i < theWorkspace->work_objects->len; ++i)
	{
		if (NULL != (w = tm_work_object_find(TM_WORK_OBJECT(theWorkspace->work_objects->pdata[i])
			  , file_name, name_only)))
			return w;
	}
	return NULL;
}

void tm_workspace_recreate_tags_array(void)
{
	guint i, j;
	TMWorkObject *w;
	TMTagAttrType sort_attrs[] = { tm_tag_attr_name_t, tm_tag_attr_file_t
		, tm_tag_attr_scope_t, tm_tag_attr_type_t, 0};

#ifdef TM_DEBUG
	g_message("Recreating workspace tags array");
#endif

	if ((NULL == theWorkspace) || (NULL == theWorkspace->work_objects))
		return;
	if (NULL != theWorkspace->work_object.tags_array)
		g_ptr_array_set_size(theWorkspace->work_object.tags_array, 0);
	else
		theWorkspace->work_object.tags_array = g_ptr_array_new();

#ifdef TM_DEBUG
	g_message("Total %d objects", theWorkspace->work_objects->len);
#endif
	for (i=0; i < theWorkspace->work_objects->len; ++i)
	{
		w = TM_WORK_OBJECT(theWorkspace->work_objects->pdata[i]);
#ifdef TM_DEBUG
		g_message("Adding tags of %s", w->file_name);
#endif
		if ((NULL != w) && (NULL != w->tags_array) && (w->tags_array->len > 0))
		{
			for (j = 0; j < w->tags_array->len; ++j)
			{
				g_ptr_array_add(theWorkspace->work_object.tags_array,
					  w->tags_array->pdata[j]);
			}
		}
	}
#ifdef TM_DEBUG
	g_message("Total: %d tags", theWorkspace->work_object.tags_array->len);
#endif
	tm_tags_sort(theWorkspace->work_object.tags_array, sort_attrs, TRUE);
}

gboolean tm_workspace_update(TMWorkObject *workspace, gboolean force
  , gboolean recurse, gboolean __unused__ update_parent)
{
	guint i;
	gboolean update_tags = force;
	
#ifdef TM_DEBUG
	g_message("Updating workspace");
#endif

	if (workspace != TM_WORK_OBJECT(theWorkspace))
		return FALSE;
	if (NULL == theWorkspace)
		return TRUE;
	if ((recurse) && (theWorkspace->work_objects))
	{
		for (i=0; i < theWorkspace->work_objects->len; ++i)
		{
			if (TRUE == tm_work_object_update(TM_WORK_OBJECT(
				  theWorkspace->work_objects->pdata[i]), FALSE, TRUE, FALSE))
				update_tags = TRUE;
		}
	}
	if (update_tags)
		tm_workspace_recreate_tags_array();
	workspace->analyze_time = time(NULL);
	return update_tags;
}

void tm_workspace_dump(void)
{
	if (theWorkspace)
	{
#ifdef TM_DEBUG
		g_message("Dumping TagManager workspace tree..");
#endif
		tm_work_object_dump(TM_WORK_OBJECT(theWorkspace));
		if (theWorkspace->work_objects)
		{
			int i;
			for (i=0; i < theWorkspace->work_objects->len; ++i)
			{
				if (IS_TM_PROJECT(TM_WORK_OBJECT(theWorkspace->work_objects->pdata[i])))
					tm_project_dump(TM_PROJECT(theWorkspace->work_objects->pdata[i]));
				else
					tm_work_object_dump(TM_WORK_OBJECT(theWorkspace->work_objects->pdata[i]));
			}
		}
	}
}

static int fill_find_tags_array (GPtrArray *dst, const GPtrArray *src,
								 const char *name, int type, gboolean partial,
								 gboolean first)
{
	TMTag **match;
	int tagIter, count;
	size_t len;
	
	if ((!src) || (!dst) || (!name))
		return 0;
	
	if(first)
	{
		len = strlen(name);
		if(!len) return 0;
	}
	
	match = tm_tags_find (src, name, partial, &count);
	if (count && match && *match)
	{
		for (tagIter=0;tagIter<count;++tagIter)
		{
			if (type & match[tagIter]->type)
			{
				g_ptr_array_add(dst, match[tagIter]);
			}
			if(first)
			{
				if (partial)
				{
					if (0 != strncmp(match[tagIter]->name, name, len))
						break;
				}
				else
				{
					if (0 != strcmp(match[tagIter]->name, name))
						break;
				}
			}
		}
	}
	return dst->len;
}

const GPtrArray *tm_workspace_find(const char *name, int type, TMTagAttrType *attrs
 , gboolean partial)
{
	static GPtrArray *tags = NULL;
	
	if ((!theWorkspace) || (!name))
		return NULL;
	
	if (tags)
		g_ptr_array_set_size(tags, 0);
	else
		tags = g_ptr_array_new();

  fill_find_tags_array(tags, theWorkspace->work_object.tags_array,
                                              name, type, partial, TRUE);
  fill_find_tags_array(tags, theWorkspace->global_tags,
                                              name, type, partial, TRUE);
  
	tm_tags_sort(tags, attrs, TRUE);
	return tags;
}

const TMTag *tm_get_current_function (GPtrArray *file_tags, const gulong line)
{
	GPtrArray *const local = tm_tags_extract(file_tags, tm_tag_function_t);
	if (local && local->len)
	{
		int i;
		TMTag *tag, *function_tag = NULL;
		gulong function_line = 0;
		glong delta;
		
		for (i = 0; (i < local->len); ++i)
		{
			tag = TM_TAG(local->pdata[i]);
			delta = line - tag->atts.entry.line;
			if( delta >= 0 && delta < line - function_line )
			{
				function_tag = tag;
				function_line = tag->atts.entry.line;
			}
		}
		g_ptr_array_free (local, TRUE);
		return function_tag;
	}
	return NULL;
};

static int find_scope_members_tags(const GPtrArray *all, GPtrArray *tags,
	    			const langType langJava, const char *name, const char *filename)
{
  GPtrArray *local = g_ptr_array_new();
  unsigned int i;
  TMTag *tag;
  size_t len = strlen(name);
  for (i = 0; (i < all->len); ++i)
  {
	  tag = TM_TAG(all->pdata[i]);
    if(filename && tag->atts.entry.file &&
       0 != strcmp(filename, tag->atts.entry.file->work_object.short_name))
    {
      continue;
    }
    if(tag && tag->atts.entry.scope && tag->atts.entry.scope[0] != '\0')
    {
	    if(0 == strncmp(name, tag->atts.entry.scope, len))
      {
	      g_ptr_array_add(local, tag);
	    }
    }
  }
  if(local->len > 0)
  {
    unsigned int j;
    TMTag *tag2;
    char backup;
	  char *s_backup = NULL;
	  char *var_type = NULL;
	  char *scope;
    for (i = 0; (i < local->len); ++i)
    {
      tag = TM_TAG(local->pdata[i]);
      scope = tag->atts.entry.scope;
      if(scope && 0 == strcmp(name, scope))
      {
        g_ptr_array_add(tags, tag);
	      continue;
	    }
      s_backup = NULL;
      j = 0;/* someone could write better code :P */
      while(scope)
	    {
		    if(s_backup)
        {
          backup = s_backup[0];
          s_backup[0] = '\0';
          if(0 == strcmp(name, tag->atts.entry.scope))
		      {
            j = local->len;
            s_backup[0] = backup;
			      break;
		      }
        }
        if(tag->atts.entry.file && tag->atts.entry.file->lang == langJava)
        {
          scope = strrchr(tag->atts.entry.scope, '.');
          if(scope) var_type = scope + 1;
        }
        else
        {
          scope = strrchr(tag->atts.entry.scope, ':');
          if(scope) 
          {
            var_type = scope + 1;
            scope--;
          }
        }
        if(s_backup)
        {
          s_backup[0] = backup;
        }
		    if(scope)
        {
          if(s_backup)
		      {
			      backup = s_backup[0];
			      s_backup[0] = '\0';
			    }
          for (j = 0; (j < local->len); ++j)
          {
            if(i == j) continue;
            tag2 = TM_TAG(local->pdata[j]);
            if(tag2->atts.entry.var_type &&
               0 == strcmp(var_type, tag2->atts.entry.var_type))
            {
              break;
	          }
          }
          if(s_backup) s_backup[0] = backup;
        }
        if(j < local->len)
        {  
          break;
        }
        s_backup = scope;
      }
      if(j == local->len)
      {
        g_ptr_array_add(tags, tag);
      }
    }      
  }
  g_ptr_array_free(local, TRUE);
  return (int)tags->len;
}

const GPtrArray *tm_workspace_find_scope_members(const GPtrArray *file_tags, const char *name,
												 gboolean search_global)
{
	static GPtrArray *tags = NULL;
	GPtrArray *local = NULL;
	char *new_name = (char *)name;
	char *filename = NULL;
	int found = 0, del = 0;
	static langType langJava = -1;
	TMTag *tag = NULL;	
	
	//FIX ME
	//langJava = getNamedLanguage ("Java");
	
	g_return_val_if_fail((theWorkspace && name && name[0] != '\0'), NULL);
	
	if (!tags)
		tags = g_ptr_array_new();
	
	while(1)
	{
		const GPtrArray *tags2;
		int found = 0, types = (tm_tag_class_t | tm_tag_namespace_t |
								tm_tag_struct_t | tm_tag_typedef_t |
								tm_tag_union_t | tm_tag_enum_t);
		
		if(file_tags)
		{
			g_ptr_array_set_size(tags, 0);
			found = fill_find_tags_array (tags, file_tags,
										  new_name, types, FALSE, FALSE);
		}
		if(found)
		{
			tags2 = tags;
		}
		else
		{
			TMTagAttrType attrs[] =
			{
				tm_tag_attr_name_t, tm_tag_attr_type_t,
				tm_tag_attr_none_t
			};
			tags2 = tm_workspace_find(new_name, types, attrs, FALSE);
		}
		
		if( (tags2) && (tags2->len == 1) && (tag = TM_TAG(tags2->pdata[0])) )
		{
			if(tag->type == tm_tag_typedef_t  && tag->atts.entry.var_type
			   && tag->atts.entry.var_type[0] != '\0')
			{
				new_name = tag->atts.entry.var_type;
				continue;
			}
			filename = (tag->atts.entry.file ?
						tag->atts.entry.file->work_object.short_name : NULL);
			if(tag->atts.entry.scope && tag->atts.entry.scope[0] != '\0')
			{
				del = 1;
				if(tag->atts.entry.file && tag->atts.entry.file->lang == langJava)
				{
					new_name = g_strdup_printf("%s.%s",tag->atts.entry.scope, new_name);
				}
				else
				{
					new_name = g_strdup_printf("%s::%s",tag->atts.entry.scope, new_name);
				}
			}
			break;
		}
		else
		{
			return NULL;
		}
	}
	
	g_ptr_array_set_size(tags, 0);
	
	if(tag && tag->atts.entry.file)
	{
		local = tm_tags_extract(tag->atts.entry.file->work_object.tags_array,
								(tm_tag_function_t|tm_tag_member_t|
								 tm_tag_method_t|tm_tag_enumerator_t));
	}
	else
	{
		local = tm_tags_extract(theWorkspace->work_object.tags_array,
								(tm_tag_function_t|tm_tag_member_t|
								 tm_tag_method_t|tm_tag_enumerator_t));
	}
	if (local)
	{
		found = find_scope_members_tags(local, tags, langJava, new_name, filename);
		g_ptr_array_free(local, TRUE);
	}
	if(!found && search_global)
	{
	    GPtrArray *global = tm_tags_extract(theWorkspace->global_tags,
	       (tm_tag_member_t|tm_tag_method_t|tm_tag_function_t|tm_tag_enumerator_t));
	    if (global)
	    {
			find_scope_members_tags(global, tags, langJava, new_name, filename);
			g_ptr_array_free(global, TRUE);
	    }
	}
	if(del)
	{
	    g_free(new_name);
	}
	return tags;
}

const GPtrArray *tm_workspace_get_parents(const gchar *name)
{
	static TMTagAttrType type[] = { tm_tag_attr_name_t, tm_tag_attr_none_t };
	static GPtrArray *parents = NULL;
	const GPtrArray *matches;
	guint i = 0;
	guint j;
	gchar **klasses;
	gchar **klass;
	TMTag *tag;

	g_return_val_if_fail(name && isalpha(*name),NULL);

	if (NULL == parents)
		parents = g_ptr_array_new();
	else
		g_ptr_array_set_size(parents, 0);
	matches = tm_workspace_find(name, tm_tag_class_t, type, FALSE);
	if ((NULL == matches) || (0 == matches->len))
		return NULL;
	g_ptr_array_add(parents, matches->pdata[0]);
	while (i < parents->len)
	{
		tag = TM_TAG(parents->pdata[i]);
		if ((NULL != tag->atts.entry.inheritance) && (isalpha(tag->atts.entry.inheritance[0])))
		{
			klasses = g_strsplit(tag->atts.entry.inheritance, ",", 10);
			for (klass = klasses; (NULL != *klass); ++ klass)
			{
				for (j=0; j < parents->len; ++j)
				{
					if (0 == strcmp(*klass, TM_TAG(parents->pdata[j])->name))
						break;
				}
				if (parents->len == j)
				{
					matches = tm_workspace_find(*klass, tm_tag_class_t, type, FALSE);
					if ((NULL != matches) && (0 < matches->len))
						g_ptr_array_add(parents, matches->pdata[0]);
				}
			}
			g_strfreev(klasses);
		}
		++ i;
	}
	return parents;
}
