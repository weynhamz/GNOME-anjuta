#include <glib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "libgtodo.h"
#include <libgnomevfs/gnome-vfs.h>

#ifdef ENABLE_NLS
#include <libintl.h>
#include <locale.h>
#define _(String) gettext (String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else 
#define _(String) String
#endif

#ifdef DEBUG
short int debug = 1;
#else
short int debug = 0;
#endif

typedef struct _GTodoCategory{
	gchar *name;
	gint id;
} GTodoCategory;

/* should not be used by the user. internal function */	
GTodoItem * gtodo_client_get_todo_item_from_xml_ptr(GTodoClient *cl, xmlNodePtr node);

/* this checks if the xml backend file exists.. not to be used by the user */
int check_item_changed(GnomeVFSMonitorHandle *handle, const gchar *uri, const gchar *info, GnomeVFSMonitorEventType event, GTodoClient *cl);

int gtodo_client_check_file(GTodoClient *cl, GError **error);


/* Function that creates an empty todo item. WARNING Don't use this when adding a todo item to the list.*/
/* Use gtodo_client_create_new_todo_item instead */
/* note.. id is equal to the time created...  this should be unique. (what is the change on 2 pc's its created at the vary same second )*/

GTodoItem * gtodo_client_create_empty_todo_item()
{
	GTodoItem *item = g_malloc(sizeof(GTodoItem));		
	if(item == NULL) return NULL;
	item->id = 0;
	item->notify = FALSE;
	item->last_edited = 0;
	item->start = NULL;
	item->stop = NULL;
	item->due = NULL;
	item->done = FALSE;
	item->category = NULL;
	item->priority = GTODO_PRIORITY_MEDIUM;
	item->summary = NULL;
	item->comment = NULL;
	item->due_time[GTODO_DUE_TIME_HOURE] = -1;
	item->due_time[GTODO_DUE_TIME_MINUTE] = 0;
	return item;
}

/* create a new unique todo item */
/* use this to add an todo item */
GTodoItem * gtodo_client_create_new_todo_item(GTodoClient *cl)
{
	GTodoItem *item = gtodo_client_create_empty_todo_item();
	/* give an nice "random" id */
	item->id = (GTime)time(NULL);
	/* set the start time */
	item->start = g_date_new();
	g_date_set_time(item->start, (GTime)item->id);
	return item;
}


/* free's an GTodoItem */
void gtodo_todo_item_free(GTodoItem *item)
{
	if(item->start != NULL) g_date_free(item->start);
	if(item->stop != NULL) g_date_free(item->stop);
	if(item->due != NULL) g_date_free(item->due);
	if(item->category != NULL) g_free(item->category);
	if(item->summary != NULL) g_free(item->summary);
	if(item->comment != NULL) g_free(item->comment);
	g_free(item);
}


/* get the id from an todo item in guint32 (its an GTime, but a gtime is an gint32)..*/
/* I made it a guint32 because there is no negative time here */
guint32 gtodo_todo_item_get_id(GTodoItem *item)
{
	return (guint32 )item->id;
}

/* set the notification flag for this todo item. */    
void gtodo_todo_item_set_notify(GTodoItem *item, gboolean notify)
{
	item->notify = notify;
}
/* get the statis of the notification flag */
gboolean gtodo_todo_item_get_notify(GTodoItem *item)    
{
	return item->notify;
}

/* get the priority. see enumeration in libgtodo.h for possible return values */
int gtodo_todo_item_get_priority(GTodoItem *item)
{
	return item->priority;
}

/* set the priority, for possible value's look @ enumeration in libgtodo.c */
void gtodo_todo_item_set_priority(GTodoItem *item, int priority)
{
	item->priority = priority;
}

/* get the summary, the returned value shouldnt be freed.*/
/* I return an empty string when there is no value. I am afraid this is wrong now */

char *gtodo_todo_item_get_summary(GTodoItem *item)
{
	if(item->summary == NULL) return "";
	return item->summary;
}

/* set the summary, also setting to NULL, or changing is allowed */
void gtodo_todo_item_set_summary(GTodoItem *item, gchar *summary)
{
	if(summary == NULL)
	{
		if(item->summary != NULL) g_free(item->summary);
		item->summary = NULL;
	}
	else
	{
		GString *string;
		int i;
		string = g_string_new(summary);
		for(i=0;i < string->len;i++)
		{
			if(string->str[i] == '&')
			{
				g_string_insert(string, i+1, "amp;");
			}
		}
		if(item->summary != NULL) g_free(item->summary);
		item->summary = string->str;
		g_string_free(string,FALSE);
	}
}

/* get the category, the returned value shouldnt be freeed.*/
char *gtodo_todo_item_get_category(GTodoItem *item)
{
	return item->category;
}

/* set the category or changing is allowed */
/* FIXME if category exists, if not.. create it. */
void gtodo_todo_item_set_category(GTodoItem *item, gchar *category)
{
	if(category == NULL) return;
	/* setting to NULL is bad.. because the GTodoItem is invalid then */
	/*	{
		if(item->category != NULL) g_free(item->category);
		item->category = NULL;
		}
		*/    else
	{
		if(item->category != NULL) g_free(item->category);
		item->category = g_strdup(category);
	}
}

/* get the comment, this can be a multi line string */
char *gtodo_todo_item_get_comment(GTodoItem *item)
{
	if(item->comment == NULL) return "";
	return item->comment;
}

/* set the comment, setting to NULL is allowed, or changing */
void gtodo_todo_item_set_comment(GTodoItem *item, gchar *comment)
{
	if(comment == NULL)
	{
		if(item->comment != NULL) g_free(item->comment);
		item->comment = NULL;
	}
	else{
		GString *string;
		int i;
		string = g_string_new(comment);
		for(i=0;i < string->len;i++)
		{
			if(string->str[i] == '&')
			{
				g_string_insert(string, i+1, "amp;");
			}
		}
		if(item->comment != NULL) g_free(item->comment);
		item->comment = string->str;
		g_string_free(string, FALSE);
	}
}

/*get/set done */
gboolean gtodo_todo_item_get_done(GTodoItem *item)
{
	return item->done;
}

void gtodo_todo_item_set_done(GTodoItem *item, gboolean done)
{
	/* if the item is set done, set done date aswell, useless to have the user set it twice */
	if(done == TRUE) gtodo_todo_item_set_stop_date_today(item);
	item->done = done;
}

/* > 0 allready due */
/* 0 due today */
/* <0 due in future */
/* GTODO_NO_DUE_DATE = no due date */
gint32 gtodo_todo_item_check_due(GTodoItem *item)
{
	GDate *today;
	int i;
	if(item->due == NULL) return GTODO_NO_DUE_DATE;
	today = g_date_new();
	g_date_set_time(today, time(NULL));
	i = g_date_days_between(item->due,today);
	g_date_free(today);
	return i;
}
/*returns the time in minutes that is still left.. it will return 0 when there is nothing left
  ofit isnt today. */

int gtodo_todo_item_check_due_time_minutes_left(GTodoItem *item)
{
	struct tm *lctime;
	time_t now;
	if(gtodo_todo_item_check_due(item) != 0) return 0;
	now = time(NULL);
	lctime =  localtime(&now);
	if(lctime == NULL) return 0;
	if(item->due_time[GTODO_DUE_TIME_HOURE] == -1 && item->due_time[GTODO_DUE_TIME_MINUTE] == 0) return 3000;
	return MAX(0, -((lctime->tm_hour*60+lctime->tm_min) - item->due_time[GTODO_DUE_TIME_HOURE]*60-item->due_time[GTODO_DUE_TIME_MINUTE]));
}

/* get the last_edited date in severall format's */
/* return an julian date ... -1 = no date set */
guint32 gtodo_todo_item_get_last_edited_date_as_julian(GTodoItem *item)
{
	if(item->last_edited == 0 ) return 1;
	else
	{
		GDate *date = g_date_new();
		guint32 julian=1;
		g_date_set_time(date, item->last_edited);
		julian = g_date_get_julian(date);
		g_date_free(date);
		return julian;
	}
}
/* return the houre of the due time */
gint gtodo_todo_item_get_due_time_houre(GTodoItem *item)
{
	return item->due_time[GTODO_DUE_TIME_HOURE];
}
/* return the houre of the due time */
gint gtodo_todo_item_get_due_time_minute(GTodoItem *item)
{
	return item->due_time[GTODO_DUE_TIME_MINUTE];
}
/* return the houre of the due time */
gint gtodo_todo_item_set_due_time_minute(GTodoItem *item, gint minute)
{
	if(minute < 0 && minute >=60 ) return FALSE;
	else item->due_time[GTODO_DUE_TIME_MINUTE] = minute;
	return TRUE;
}
/* return the houre of the due time */
gint gtodo_todo_item_set_due_time_houre(GTodoItem *item, gint houre)
{
	if(houre < -1 && houre >= 24 ) return FALSE;
	else item->due_time[GTODO_DUE_TIME_HOURE] = houre;
	return TRUE;
}
/* get the start date in severall format's */
/* return an julian date ... -1 = no date set */
guint32 gtodo_todo_item_get_start_date_as_julian(GTodoItem *item)
{
	if(item->start == NULL || !g_date_valid(item->start)) return 1;
	else
	{
		if(!g_date_valid_julian(g_date_get_julian(item->start))) return 1;
		return g_date_get_julian(item->start);
	}
}
/* set start date returns false when not able to set */
gboolean gtodo_todo_item_set_start_date_as_julian(GTodoItem *item, guint32 julian)
{
	if(!g_date_valid_julian(julian)) return FALSE;
	if(item->start == NULL) item->start = g_date_new_julian(julian);
	else g_date_set_julian(item->start, julian);
	return TRUE;
}


/* get localized string.. this needs to be freed! */
gchar *gtodo_todo_item_get_start_date_as_string(GTodoItem *item)  
{
	gchar *buffer = g_malloc(sizeof(gchar)*64);
	memset(buffer,'\0',  64*sizeof(gchar));
	if(item == NULL || item->start == NULL)
	{
		g_free(buffer);
		return NULL;
	}
	if(!g_date_valid(item->start))
	{
		g_free(buffer);
		return NULL;
	}
	if(g_date_strftime(buffer,  64*sizeof(gchar),  "%d %b %G", item->start) == 0)
	{
		g_free(buffer);
		return NULL;
	}
	return buffer;
}

/* get the stop date in severall format's */
/* return an julian date ... 1 = no date set */
guint32 gtodo_todo_item_get_stop_date_as_julian(GTodoItem *item)
{
	if(item->stop == NULL || !g_date_valid(item->stop)) return 1;
	else
	{
		if(!g_date_valid_julian(g_date_get_julian(item->stop))) return 1;
		return g_date_get_julian(item->stop);
	}
}

/* set stop date returns false when not able to set */
gboolean gtodo_todo_item_set_stop_date_as_julian(GTodoItem *item, guint32 julian)
{
	if(!g_date_valid_julian(julian)) return FALSE;
	if(item->stop == NULL) item->stop = g_date_new_julian(julian);
	else g_date_set_julian(item->stop, julian);
	return TRUE;
}
gboolean gtodo_todo_item_set_stop_date_today(GTodoItem *item)
{
	if(item == NULL) return FALSE;
	if(item->stop == NULL) item->stop = g_date_new();
	g_date_set_time(item->stop, time(NULL));
	return TRUE;
}
/* get localized string.. this needs to be freed! */
gchar *gtodo_todo_item_get_stop_date_as_string(GTodoItem *item)  
{
	gchar *buffer = g_malloc(sizeof(gchar)*64);
	memset(buffer, '\0', 64*sizeof(gchar));	
	if(item == NULL || item->stop == NULL)
	{
		g_free(buffer);
		return NULL;
	}
	if(!g_date_valid(item->stop))
	{
		g_free(buffer);
		return NULL;
	}
	if(g_date_strftime(buffer,  64*sizeof(gchar),  "%d %b %G", item->stop) == 0)
	{
		g_free(buffer);
		return NULL;
	}
	return buffer;
}
/* get the due date in severall format's */
/* return an julian date ... 1 = no date set */
guint32 gtodo_todo_item_get_due_date_as_julian(GTodoItem *item)
{
	if(item->due == NULL || !g_date_valid(item->due))
	{
		return GTODO_NO_DUE_DATE;
	}
	else
	{
		if(!g_date_valid_julian(g_date_get_julian(item->due))) return GTODO_NO_DUE_DATE;
		return g_date_get_julian(item->due);
	}
}

/* set due date returns false when not able to set */
gboolean gtodo_todo_item_set_due_date_as_julian(GTodoItem *item, guint32 julian)
{
	if(julian == GTODO_NO_DUE_DATE)
	{
		if(item->due != NULL)
		{
			g_date_free(item->due);
			item->due = NULL;
		}
	}
	if(!g_date_valid_julian((guint32)julian)) return FALSE;
	if(item->due == NULL) item->due = g_date_new_julian((guint32)julian);
	else g_date_set_julian(item->due, (guint32)julian);
	return TRUE;
}

GDate * gtodo_todo_item_get_due_date(GTodoItem *item)
{
	if(item == NULL || item->due == NULL) return NULL;
	if(!g_date_valid(item->due)) return NULL;
	return item->due;
}

/* get localized string.. this needs to be freed! */
gchar *gtodo_todo_item_get_due_date_as_string(GTodoItem *item)  
{
	gchar *buffer = g_malloc(sizeof(gchar)*64);
	memset(buffer, '\0', 64*sizeof(gchar));
	if(item == NULL || item->due == NULL)
	{
		g_free(buffer);
		return NULL;
	}
	if(!g_date_valid(item->due))
	{
		g_free(buffer);
		return NULL;
	}
	if(g_date_strftime(buffer, 64*sizeof(gchar), "%d %b %G", item->due) == 0)
	{
		g_free(buffer);
		return NULL;
	}
	return buffer;
}

/* should not be used by the user. internal function */	
GTodoItem * gtodo_client_get_todo_item_from_xml_ptr(GTodoClient *cl, xmlNodePtr node)
{
	GTodoItem *item =NULL;
	xmlChar *category;
	if(node == NULL) return NULL;
	category =  xmlGetProp(node->parent, (const xmlChar *)"title");
	node = node->xmlChildrenNode;
	item = gtodo_client_create_empty_todo_item();
	gtodo_todo_item_set_category(item, category);
	xmlFree(category);
	while(node != NULL)
	{
		if(xmlStrEqual(node->name, (const xmlChar *)"comment"))
		{
			xmlChar *temp;
			temp = xmlNodeGetContent(node);
			if(temp != NULL) 
			{
				item->comment = g_strdup(temp);
				xmlFree(temp);
			}
		}
		else if(xmlStrEqual(node->name, (const xmlChar *)"summary"))
		{
			xmlChar *temp;
			temp = xmlNodeGetContent(node);
			if(temp != NULL) 
			{
				item->summary = g_strdup(temp);
				xmlFree(temp);
			}
		}
		else if(xmlStrEqual(node->name, (const xmlChar *)"attribute"))
		{
			xmlChar *temp;
			temp = xmlGetProp(node, (const xmlChar *)"id");
			if(temp != NULL)
			{
				item->id = g_ascii_strtoull(temp, NULL,0);
				xmlFree(temp);
			}
			temp = xmlGetProp(node, (const xmlChar *)"priority");
			if(temp != NULL)
			{
				item->priority = atoi(temp);
				xmlFree(temp);
			}
			temp = xmlGetProp(node, (const xmlChar *)"done");
			if(temp != NULL)
			{
				item->done = atoi(temp);
				xmlFree(temp);
			}
			temp = xmlGetProp(node, (const xmlChar *)"start_date");
			if(temp != NULL)
			{
				guint64 i = g_ascii_strtoull(temp, NULL, 0);
				if(i > 0) item->start = g_date_new_julian(i);
				xmlFree(temp);
			}
			temp = xmlGetProp(node, (const xmlChar *)"completed_date");
			if(temp != NULL)
			{
				guint64 i = g_ascii_strtoull(temp, NULL, 0);
				if(i > 0) item->stop = g_date_new_julian(i);
				xmlFree(temp);
			}

			temp = xmlGetProp(node, (const xmlChar *)"notify");
			if(temp != NULL)
			{
				gint i = (int)g_ascii_strtod(temp,NULL);
				item->notify = (int)i;
				xmlFree(temp);
			}
			temp = xmlGetProp(node, (const xmlChar *)"enddate");
			if(temp != NULL)
			{
				guint64 i = g_ascii_strtoull(temp, NULL, 0);
				if(i > 1 && i != GTODO_NO_DUE_DATE) item->due = g_date_new_julian(i);
				xmlFree(temp);
			}
			temp = xmlGetProp(node, (const xmlChar *)"endtime");
			if(temp != NULL)
			{
				gint houre =0, minute = 0;
				gint i = (int)g_ascii_strtod(temp,NULL);
				if(i < 0)
				{
					houre = -1;minute = 0;
				}
				else if( i > 0 && i < 1500)
				{
					houre = (int)i/60;
					minute = (int)i - houre*60;
				}
				item->due_time[GTODO_DUE_TIME_HOURE] = houre;
				item->due_time[GTODO_DUE_TIME_MINUTE] = minute;
				xmlFree(temp);
			}
			temp = xmlGetProp(node, (const xmlChar *)"last_edited");
			if(temp != NULL)
			{
				guint64 i = g_ascii_strtoull(temp, NULL, 0);
				item->last_edited = (GTime) i;
				xmlFree(temp);
			}
		}
		node = node->next;
	}
	return item;
}

/* initialise the gtodo lib */
int gtodo_client_check_file(GTodoClient *cl, GError **error)
{
	GnomeVFSFileInfo info;
	GnomeVFSResult result;
	GnomeVFSResult info_result;
	GError *tmp_error = NULL;
	gchar *base_path = g_path_get_dirname(cl->xml_path);
	/* check if the error is good or wrong. */
	g_return_val_if_fail(error == NULL || *error == NULL,FALSE);

	/* this is dirty.. needs a fix hard *
	 * The client should do this.. so this code should be considert
	 * deprecated. I left it here thinking it wouldnt hurt anybody.
	 */
	
	if(base_path != NULL)
	{
		gnome_vfs_make_directory(base_path, 0755);
		g_free(base_path);
	}

	/* Get permission of the file */
	/* This also tell's us if it does exists */
	info_result = gnome_vfs_get_file_info(cl->xml_path, 
			&info, 
			GNOME_VFS_FILE_INFO_DEFAULT|GNOME_VFS_FILE_INFO_GET_ACCESS_RIGHTS );

	/* If I got the info to check it out */
	if(info_result == GNOME_VFS_OK )
	{
		GnomeVFSHandle *handle;
		gchar *read_buf = NULL;
		int perm = (info.permissions-(int)(info.permissions/65536)*65536);
		int read      = (int)(perm/256);
		int write     =(int)((perm - (int)(perm/256)*256)/128);

		/* If I am not allowed to read the file */
		if(read == 0)
		{
			/* save some more info here.. check for some logicol errors and print it. */
			g_set_error(&tmp_error,LIBGTODO_ERROR,LIBGTODO_ERROR_NO_PERMISSION,
					_("No permission to read the file."));		
			g_propagate_error(error, tmp_error);                                                         
			return TRUE;
		}
		cl->read_only = !write;


		if((result = gnome_vfs_open(&handle,cl->xml_path, GNOME_VFS_OPEN_READ)) != GNOME_VFS_OK)
		{
			g_set_error(&tmp_error,LIBGTODO_ERROR,LIBGTODO_ERROR_GNOME_VFS,gnome_vfs_result_to_string(result));
			g_propagate_error(error, tmp_error);
			return TRUE;
		}
		read_buf = g_malloc0((info.size+1)*sizeof(char));

		result = gnome_vfs_read(handle, read_buf, info.size,NULL);
		if(!(result == GNOME_VFS_OK || result == GNOME_VFS_ERROR_EOF))
		{
			g_free(read_buf);
			g_set_error(&tmp_error,LIBGTODO_ERROR,LIBGTODO_ERROR_GNOME_VFS,gnome_vfs_result_to_string(result));
			g_propagate_error(error, tmp_error);
			return TRUE;
		}
		gnome_vfs_close(handle);
		cl->gtodo_doc = xmlParseMemory(read_buf, info.size);
		if(cl->gtodo_doc == NULL)
		{
			g_set_error(&tmp_error,LIBGTODO_ERROR,LIBGTODO_ERROR_XML,_("Failed to parse xml structure"));
			g_propagate_error(error, tmp_error);
			if(debug) g_print("**DEBUG** failed to read the file \n");
			return TRUE;
		}
		g_free(read_buf);
		/* get root element.. this "root" is used in the while program */    
		cl->root = xmlDocGetRootElement(cl->gtodo_doc);
		if(cl->root == NULL)
		{
			g_set_error(&tmp_error,LIBGTODO_ERROR,LIBGTODO_ERROR_XML,_("Failed to parse xml structure"));
			g_propagate_error(error, tmp_error);
			if(debug)g_print("**DEBUG** failed to get root node.\n");
			return TRUE;
		}
		/* check if the name of the root file is ok.. just to make sure :) */
		if(!xmlStrEqual(cl->root->name, (const xmlChar *)"gtodo"))
		{
			g_set_error(&tmp_error,LIBGTODO_ERROR,LIBGTODO_ERROR_XML,_("File is no valid gtodo file"));
			g_propagate_error(error, tmp_error);
			return TRUE;
		}
	}
	else if(info_result == GNOME_VFS_ERROR_NOT_FOUND){
		xmlNodePtr newn;
		if(debug) g_print("Trying to create new file\n");
		cl->gtodo_doc = xmlNewDoc("1.0");
		cl->root = xmlNewDocNode(cl->gtodo_doc, NULL, "gtodo", NULL);	 
		xmlDocSetRootElement(cl->gtodo_doc, cl->root);
		newn = xmlNewTextChild(cl->root, NULL, "category", NULL);	
		xmlNewProp(newn, "title", _("Personal"));
		newn = xmlNewTextChild(cl->root, NULL, "category", NULL);	
		xmlNewProp(newn, "title", _("Business"));
		newn = xmlNewTextChild(cl->root, NULL, "category", NULL);	
		xmlNewProp(newn, "title", _("Unfiled"));
		if(gtodo_client_save_xml(cl, &tmp_error))
		{
			g_propagate_error(error, tmp_error);
			return TRUE;
		}
		cl->read_only = FALSE;
	}
	else{
		/* save some more info here.. check for some logicol errors and print it. */
		g_set_error(&tmp_error,LIBGTODO_ERROR,LIBGTODO_ERROR_GNOME_VFS,gnome_vfs_result_to_string(info_result));
		g_propagate_error(error, tmp_error);
		return TRUE;
	}
	return FALSE;
}

/* save the gtodo_Client */

int gtodo_client_save_xml(GTodoClient *cl, GError **error)
{
	GError *tmp_error = NULL;
	/* check if the error is good or wrong. */
	g_return_val_if_fail(error == NULL || *error == NULL,FALSE);

	if(debug)g_print("** DEBUG ** saving %s\n", cl->xml_path);
	if(gtodo_client_save_xml_to_file(cl, cl->xml_path, &tmp_error))
	{
		g_propagate_error(error, tmp_error);
		return TRUE;
	}
	return FALSE;
}


int gtodo_client_save_xml_to_file(GTodoClient *cl, gchar *file, GError **error)
{
	xmlChar *buffer;
	GnomeVFSHandle *handle;
	GnomeVFSResult result = 0;
	GError *tmp_error = NULL;
	int size;
	/* Test if there is actually a client to save */
	if(cl == NULL)
	{
		g_set_error(&tmp_error,LIBGTODO_ERROR,LIBGTODO_ERROR_GENERIC,_("No Gtodo Client to save.") );
		g_propagate_error(error, tmp_error);
		return TRUE;
	}
	/* dump the xml to memory */
	xmlDocDumpFormatMemory(cl->gtodo_doc, &buffer, &size, TRUE);
	/* dirty trick to get the whole crap to work on ftp */
	if(!strncmp(file, "ftp://", MIN(strlen(file),6)))
	{
		GnomeVFSURI *uri = gnome_vfs_uri_new(file);
		if(uri != NULL && gnome_vfs_uri_exists(uri))
		{
			/* stupid hack to make everything work.. darn ftp */
			if(debug)g_print("trying to unlink the file\n");
			if(gnome_vfs_unlink(file) != GNOME_VFS_OK)
			{      
				if(debug)g_print("Failed to delete\n");
				g_set_error(&tmp_error,LIBGTODO_ERROR,LIBGTODO_ERROR_GENERIC,_("Failed to delete %s."),file);	
				g_propagate_error(error, tmp_error);
				return TRUE;	
			}
			else
			{
				if(debug)g_print("file unlinked\n");
			}
		}
		gnome_vfs_uri_unref(uri);
	}

	/* open the file for writing */
	result = gnome_vfs_create(&handle,file,GNOME_VFS_OPEN_WRITE, 0, 0644);
	if(result != GNOME_VFS_OK)
	{
		g_set_error(&tmp_error,LIBGTODO_ERROR,LIBGTODO_ERROR_GENERIC,_("Failed to create/open file.") );
		g_propagate_error(error, tmp_error);
		return TRUE;
	}

	/* start writing */
	result = gnome_vfs_write(handle, buffer, size, NULL);
	if(result != GNOME_VFS_OK)
	{
		g_set_error(&tmp_error,LIBGTODO_ERROR,LIBGTODO_ERROR_GENERIC,_("Failed to write data to file.") );
		g_propagate_error(error, tmp_error);
		xmlFree(buffer);
		return TRUE;
	}
	/* close the file connection */
	gnome_vfs_close(handle);

	xmlFree(buffer);
	/* return that everything is ok */
	return FALSE;
}

int gtodo_client_reload(GTodoClient *cl)
{
	/* fixme */
	if (cl->gtodo_doc)
		xmlFreeDoc(cl->gtodo_doc);
	cl->root = NULL;
	if(gtodo_client_check_file(cl, NULL))
	{
		if(debug)g_print("Failed to reload the file\n");
		return FALSE;
	}
	return TRUE;
}

int gtodo_client_load(GTodoClient *cl, const gchar *xml_path)
{
	/* fixme */
	if (cl->gtodo_doc)
		xmlFreeDoc(cl->gtodo_doc);
	cl->root = NULL;
	if (cl->xml_path)
		g_free (cl->xml_path);
	cl->xml_path = g_strdup (xml_path);
	if(gtodo_client_check_file(cl, NULL))
	{
		if(debug)g_print("Failed to reload the file\n");
		return FALSE;
	}
	gtodo_client_set_changed_callback (cl, cl->function, cl->data);
	return TRUE;
}

GTodoClient * gtodo_client_new_default(GError **error)
{
	GError *tmp_error = NULL;
	GTodoClient *cl = NULL;
	/* check if the error is good or wrong. */
	g_return_val_if_fail(error == NULL || *error == NULL,FALSE);


	cl = g_malloc(sizeof(GTodoClient));
	cl->xml_path = g_strdup_printf("file:///%s/.gtodo/todos", g_getenv("HOME"));
	/* check, open or create the correct xml file */
	if(gtodo_client_check_file(cl, &tmp_error))
	{
		g_propagate_error(error, tmp_error);
		return NULL;	
	}
	cl->timeout = NULL;

	return cl;
}

GTodoClient * gtodo_client_new_from_file(char *filename, GError **error)
{
	GError *tmp_error = NULL;
	GTodoClient *cl = NULL;
	/* check if the error is good or wrong. */
	g_return_val_if_fail(error == NULL || *error == NULL,FALSE);
	if(debug)g_print("Trying to create a new client %s\n", filename);
	if(filename == NULL)
	{
		g_set_error(&tmp_error,LIBGTODO_ERROR,LIBGTODO_ERROR_NO_FILENAME,_("No filename supplied.") );
		g_propagate_error(error, tmp_error);
		return NULL;	
	}

	cl = g_malloc(sizeof(GTodoClient));
	cl->xml_path = g_strdup(filename);
	/* check, open or create the correct xml file */
	if(gtodo_client_check_file(cl,&tmp_error))
	{
		g_propagate_error(error, tmp_error);
		return NULL;
	}

	cl->timeout = NULL;	
	return cl;
}

void gtodo_client_quit(GTodoClient *cl)
{
	gtodo_client_save_xml(cl,NULL);
	g_free(cl->xml_path);
	g_free(cl);
}

gboolean sort_category_list(GTodoCategory *a, GTodoCategory *b)
{
	return a->id-b->id;
}


GTodoList * gtodo_client_get_category_list(GTodoClient *cl)
{
	xmlNodePtr  cur;
	int repos = 0;
	GTodoCategory *cat;
	GTodoList *list = g_malloc(sizeof(GTodoList));
	list->list = NULL;

	cl->number_of_categories = 0;
	cur = cl->root->xmlChildrenNode;


	while(cur != NULL){
		if(xmlStrEqual(cur->name, (const xmlChar *)"category")){
			xmlChar *temp, *place;
			int pos;
			temp = xmlGetProp(cur, (const xmlChar *)"title");
			place = xmlGetProp(cur, (const xmlChar *)"place");
			if(place == NULL)
			{
				gchar *buf = g_strdup_printf("%i", repos);
				xmlSetProp(cur, (const xmlChar *)"place", buf);
				g_free(buf);
				repos ++;
				pos = repos;
			}
			else pos = atoi(place);
			cl->number_of_categories++;
			cat = g_malloc(sizeof(GTodoCategory));
			cat->name = g_strdup(temp);
			cat->id = pos;
			list->list = g_list_append(list->list, cat);
			xmlFree(temp);
			xmlFree(place);
		}
		cur = cur->next;
	}
	/* sort the list */
	list->list = g_list_sort(list->list, (GCompareFunc) sort_category_list);
	/* if I passed numbers, save the file..  */
	/* if its OK, this should be allright.. it goes horrible wrong 
	   if there are items withouth a number in the same file that contain items with numbers */
	if(repos != 0) gtodo_client_save_xml(cl,NULL);
	if(list->list == NULL)
	{
		g_free(list);
		return NULL;
	}
	else
	{
		list->first = g_list_first(list->list);
		return list;
	}
}

void gtodo_client_free_category_item(GTodoCategory *cat)
{
	g_free(cat->name);
}

void gtodo_client_free_category_list(GTodoClient *cl, GTodoList *list)
{
	if(list == NULL)
	{
		return;
	}
	g_list_foreach(list->first, (GFunc) gtodo_client_free_category_item, NULL);    
	g_list_free(list->first);
	g_free(list);
}

/* get a glist with todo's */
GTodoList *gtodo_client_get_todo_item_list(GTodoClient *cl, gchar *category)
{
	xmlNodePtr cur = cl->root->xmlChildrenNode;
	GTodoList *list = g_malloc(sizeof(GTodoList));
	list->list = NULL;
	while (cur != NULL) 
	{
		xmlChar *temp;
		temp = xmlGetProp(cur, (const xmlChar *)"title");
		if(category == NULL || xmlStrEqual(temp, (const xmlChar *)category))
		{	
			xmlNodePtr cur1;
			cur1= cur->xmlChildrenNode;
			while(cur1 != NULL)
			{
				if(xmlStrEqual(cur1->name, (const xmlChar *)"item"))
				{
					GTodoItem *item = gtodo_client_get_todo_item_from_xml_ptr(cl,cur1);
					if(item != NULL)list->list = g_list_append(list->list, item);
				}
				cur1 = cur1->next;
			}
		}
		xmlFree(temp);
		cur = cur->next;
	}
	if(list->list == NULL)
	{
		g_free(list);
		return NULL;
	}
	list->first = g_list_first(list->list);
	return list;    
}

/* free the todo's items */
void gtodo_client_free_todo_item_list(GTodoClient *cl, GTodoList *list)
{
	if(list == NULL)
	{
		return;
	}
	g_list_foreach(list->first, (GFunc) gtodo_todo_item_free, NULL);        
	g_list_free(list->first);
	g_free(list);
}


GTodoItem *gtodo_client_get_todo_item_from_id(GTodoClient *cl, guint32 id)
{
	xmlNodePtr node = cl->root;
	xmlNodePtr cur = cl->root->xmlChildrenNode;
	while(cur != NULL){
		if(xmlStrEqual(cur->name, "category")){
			xmlChar *temp = xmlGetProp(cur, (const xmlChar *)"title");

			xmlNodePtr cur1;
			cur1 = cur->xmlChildrenNode;
			while(cur1 != NULL)
			{
				if(xmlStrEqual(cur1->name, (const xmlChar *)"item"))
				{	
					xmlNodePtr cur2;
					cur2 = cur1->xmlChildrenNode;
					while(cur2 != NULL)
					{
						if(xmlStrEqual(cur2->name, (const xmlChar *)"attribute"))
						{	
							xmlChar *temp1 = xmlGetProp(cur2,"id");
							if(temp1 != NULL)
							{
								if(atoi(temp1) == id)node = cur1;
								xmlFree(temp1);
							}     
						}	
						cur2 = cur2->next;
					}
				}	
				cur1 = cur1->next;
			}

			xmlFree(temp);
		}

		cur = cur->next;
	}
	if(node == cl->root)
	{
		return NULL;
	}
	return gtodo_client_get_todo_item_from_xml_ptr(cl,node);
}

gboolean gtodo_client_save_todo_item(GTodoClient *cl, GTodoItem *item)
{
	xmlNodePtr cur = cl->root->xmlChildrenNode;
	if(!gtodo_client_category_exists(cl, item->category))
	{
		gtodo_client_category_new(cl, item->category);
	}
	while (cur != NULL) 
	{
		xmlChar *temp2;
		temp2 = xmlGetProp(cur, (const xmlChar *)"title");
		if(xmlStrEqual(temp2, item->category))
		{	
			gchar *temp1;
			xmlNodePtr newn, newa;
			newn = xmlNewChild(cur, NULL, "item", NULL);
			/* id */
			newa = xmlNewChild(newn, NULL, "attribute", NULL);
			temp1 = g_strdup_printf("%i",  item->id);
			xmlSetProp(newa, "id", temp1);
			g_free(temp1);
			/* priority */
			temp1 = g_strdup_printf("%i",  item->priority);
			xmlSetProp(newa, "priority", temp1);
			g_free(temp1);
			/* done */
			temp1 = g_strdup_printf("%i", item->done);
			xmlSetProp(newa, "done", temp1);
			g_free(temp1);

			/* START_DATE */
			/* if new item .. nothing is done yet */
			if(item->start != NULL)
			{
				guint32 julian = g_date_get_julian(item->start);
				temp1 = g_strdup_printf("%u", julian);
				xmlSetProp(newa, "start_date", temp1);
				g_free(temp1);
			}
			/*( COMPLETED_DATE */
			if(item->stop != NULL && item->done)
			{
				guint32 julian = g_date_get_julian(item->stop);
				temp1 = g_strdup_printf("%u", julian);
				xmlSetProp(newa, "completed_date", temp1);
				g_free(temp1);
			}

			/* enddate (to the start date attribute) */
			if(item->due != NULL)
			{
				guint32 julian = g_date_get_julian(item->due);
				temp1 = g_strdup_printf("%u", julian);
				xmlSetProp(newa, "enddate", temp1);
				g_free(temp1);
			}
			/* enddate (to the start date attribute) */
			{
				temp1 = g_strdup_printf("%i", (gint)item->notify);
				xmlSetProp(newa, "notify", temp1);
				g_free(temp1);
			}
			/* endtime (to the start date attribute) */
			if(item->due != NULL)
			{
				temp1 = g_strdup_printf("%i", (item->due_time[GTODO_DUE_TIME_HOURE]*60)+item->due_time[GTODO_DUE_TIME_MINUTE]);
				xmlSetProp(newa, "endtime", temp1);
				g_free(temp1);
			}
			/* last edited (to the start date attribute) */
			{
				temp1 = g_strdup_printf("%u", (GTime)time(NULL));
				xmlSetProp(newa, "last_edited", temp1);
				g_free(temp1);
			}
			/* summary */
			newa = xmlNewChild(newn, NULL, "summary",  item->summary);
			/* comment */
			newa = xmlNewChild(newn, NULL, "comment", item->comment);
		}
		g_free(temp2);
		cur = cur->next;
	}
	gtodo_client_save_xml(cl,NULL);
	return TRUE;
}

gchar * gtodo_client_get_category_from_list(GTodoList *list)
{
	GTodoCategory * cat = list->list->data;
	return cat->name;
}

gint gtodo_client_get_category_id_from_list(GTodoList *list)
{
	GTodoCategory * cat = list->list->data;
	return cat->id;
}

GTodoItem *gtodo_client_get_todo_item_from_list(GTodoList *list)
{
	return list->list->data;
}
gboolean gtodo_client_get_list_next(GTodoList *list)
{
	if(list == NULL) return FALSE;
	if(list->list == NULL) return FALSE;
	list->list = g_list_next(list->list);
	if(list->list == NULL) return FALSE;
	else return TRUE;
}

/* You should get the todo item first..  then edit the item and pass it to this function to see it chagned*/
gboolean gtodo_client_edit_todo_item(GTodoClient *cl, GTodoItem *item)
{
	if(cl == NULL || item == NULL) return FALSE;
	if(!gtodo_client_category_exists(cl, item->category)) return FALSE;
	gtodo_client_delete_todo_by_id(cl, item->id);
	if(!gtodo_client_save_todo_item(cl, item)) return FALSE;
	return TRUE;
}

void gtodo_client_delete_todo_by_id(GTodoClient *cl, guint32 id)
{
	xmlNodePtr node = cl->root;
	xmlNodePtr cur = cl->root->xmlChildrenNode;
	while(cur != NULL){
		if(xmlStrEqual(cur->name, "category")){
			xmlChar *temp = xmlGetProp(cur, (const xmlChar *)"title");
			{	
				xmlNodePtr cur1;
				cur1 = cur->xmlChildrenNode;
				while(cur1 != NULL)
				{
					if(xmlStrEqual(cur1->name, (const xmlChar *)"item"))
					{	
						xmlNodePtr cur2;
						cur2 = cur1->xmlChildrenNode;
						while(cur2 != NULL)
						{
							if(xmlStrEqual(cur2->name, (const xmlChar *)"attribute"))
							{	
								xmlChar *temp1 = xmlGetProp(cur2,"id");
								if(temp1 != NULL)
								{
									if(g_ascii_strtoull(temp1,NULL,0) == id)node = cur1;
									xmlFree(temp1);
								}     
							}	
							cur2 = cur2->next;
						}
					}	
					cur1 = cur1->next;
				}
			}
			xmlFree(temp);
		}

		cur = cur->next;
	}
	if(node == cl->root)
	{
		return;
	}
	xmlUnlinkNode(node);
	xmlFreeNode(node);
	gtodo_client_save_xml(cl,NULL);
}

int check_item_changed(GnomeVFSMonitorHandle *handle, const gchar *uri, const gchar *info, GnomeVFSMonitorEventType event, GTodoClient *cl)
{
	gtodo_client_reload(cl);
	if(debug)g_print("**DEBUG** Item changed\n");
	(void *)cl->function(cl, cl->data);
	return TRUE;
}

/* set the fucntion it should call when the todo database has changed */
/* function should be of type  void functionname(GTodoType *cl, gpointer data); */
void gtodo_client_set_changed_callback(GTodoClient *cl, void *(*function)(gpointer cl, gpointer data), gpointer data)
{
	cl->function = function;    
	gnome_vfs_monitor_add(&cl->timeout,cl->xml_path,
			GNOME_VFS_MONITOR_FILE,
			(GnomeVFSMonitorCallback)check_item_changed, cl);
	cl->data = data; 
}

void gtodo_client_destroy_changed_callback(GTodoClient *cl, void *(*function)(gpointer cl, gpointer data), gpointer data)
{
	cl->function = NULL;   
	if(cl->timeout != NULL)
	{
		gnome_vfs_monitor_cancel(cl->timeout);
	}
	cl->data = NULL;    
}

/* returns TRUE is successfull */
gboolean gtodo_client_category_edit(GTodoClient *cl, gchar *old, gchar *newn)
{
	if(cl == NULL || old == NULL || newn == NULL) return FALSE;
	if(gtodo_client_category_exists(cl, newn) && !gtodo_client_category_exists(cl, old)) return FALSE;
	else 
	{
		xmlNodePtr cur = cl->root->xmlChildrenNode;
		while(cur != NULL){
			if(xmlStrEqual(cur->name, "category")){
				xmlChar *temp = xmlGetProp(cur, (const xmlChar *)"title");
				if(xmlStrEqual(temp, (const xmlChar *)old))
				{
					xmlSetProp(cur, "title", newn);
					cur = NULL;
				}
				else cur = cur->next;
				xmlFree(temp);	
			}
			else cur = cur->next;
		}
	}
	gtodo_client_save_xml(cl,NULL);
	return TRUE;
}


gboolean gtodo_client_category_set_id(GTodoClient *cl, gchar *name, gint id)
{
	if(cl == NULL ||name == NULL || id == -1) return FALSE;
	if(!gtodo_client_category_exists(cl, name)) return FALSE;
	else 
	{
		xmlNodePtr cur = cl->root->xmlChildrenNode;
		while(cur != NULL){
			if(xmlStrEqual(cur->name, "category")){
				xmlChar *temp = xmlGetProp(cur, (const xmlChar *)"title");
				if(xmlStrEqual(temp, (const xmlChar *)name))
				{
					gchar *buf = g_strdup_printf("%i", id);
					xmlSetProp(cur, "place", buf);
					g_free(buf);
					cur = NULL;
				}
				else cur = cur->next;
				xmlFree(temp);	
			}
			else cur = cur->next;
		}
	}
	gtodo_client_save_xml(cl,NULL);
	return TRUE;
}

gchar *gtodo_client_category_get_from_id(GTodoClient *cl,gint id)
{
	gchar *ret_val = NULL;
	GTodoList *list = gtodo_client_get_category_list(cl);
	if(list != NULL)
	{
		do{
			gint ref_id = gtodo_client_get_category_id_from_list(list);
			if(ref_id == id && ret_val == NULL) ret_val = g_strdup(gtodo_client_get_category_from_list(list));
		}while(gtodo_client_get_list_next(list));
		gtodo_client_free_category_list(cl,list);
	}
	return ret_val;
}

gboolean gtodo_client_category_move_up(GTodoClient *cl, gchar *name)
{
	gint orig_id=0;
	gchar *above_name = NULL;
	if(name != NULL)
	{
		GTodoList *list = gtodo_client_get_category_list(cl);
		if(list != NULL)
		{
			do{
				gchar *name1 = gtodo_client_get_category_from_list(list);
				gint id =  gtodo_client_get_category_id_from_list(list);
				if(strcmp(name1,name) == 0 && orig_id == 0)orig_id = id;
			}while(gtodo_client_get_list_next(list));
		}
		if(orig_id == 0)
		{
			gtodo_client_free_category_list(cl,list);
			return FALSE;
		}
		gtodo_client_get_list_first(list);
		if(list != NULL)
		{
			do{
				gchar *name1 = gtodo_client_get_category_from_list(list);
				gint id =  gtodo_client_get_category_id_from_list(list);
				if(id == (orig_id -1) && above_name == NULL) above_name = g_strdup(name1);
			}while(gtodo_client_get_list_next(list));
			gtodo_client_free_category_list(cl,list);
		}
		if(above_name == NULL) return FALSE;
		gtodo_client_category_set_id(cl, name, (orig_id -1));
		gtodo_client_category_set_id(cl, above_name, (orig_id));
		g_free(above_name);
		return TRUE;
	}
	return FALSE;
}

gboolean gtodo_client_category_move_down(GTodoClient *cl, gchar *name)
{
	gint orig_id=0;
	gchar *under_name = NULL;
	if(name != NULL)
	{
		GTodoList *list = gtodo_client_get_category_list(cl);
		if(list != NULL)
		{
			do{
				gchar *name1 = gtodo_client_get_category_from_list(list);
				gint id =  gtodo_client_get_category_id_from_list(list);
				if(strcmp(name1,name) == 0 && orig_id == 0)orig_id = id;
			}while(gtodo_client_get_list_next(list));
		}
		if(orig_id == (cl->number_of_categories -1))
		{
			gtodo_client_free_category_list(cl,list);
			return FALSE;
		}
		gtodo_client_get_list_first(list);
		if(list != NULL)
		{
			do{
				gchar *name1 = gtodo_client_get_category_from_list(list);
				gint id =  gtodo_client_get_category_id_from_list(list);
				if(id == (orig_id +1) && under_name == NULL) under_name = g_strdup(name1);
			}while(gtodo_client_get_list_next(list));
			gtodo_client_free_category_list(cl,list);
		}
		if(under_name == NULL) return FALSE;
		gtodo_client_category_set_id(cl, name, (orig_id +1));
		gtodo_client_category_set_id(cl, under_name, (orig_id));
		g_free(under_name);
		return TRUE;
	}
	return FALSE;
}

gboolean gtodo_client_category_exists(GTodoClient *cl, gchar *name)
{
	GTodoList *list = gtodo_client_get_category_list(cl);
	if(cl == NULL || name == NULL) return FALSE;
	if(list != NULL)
	{
		do{
			if(!strcmp(name, gtodo_client_get_category_from_list(list)))
			{
				gtodo_client_free_category_list(cl, list);
				return TRUE;
			}
		}while(gtodo_client_get_list_next(list));
	}
	return FALSE;
}

gboolean gtodo_client_category_new(GTodoClient *cl, gchar *name)
{
	xmlNodePtr newn;
	gchar *buf = NULL;
	if(cl == NULL || name == NULL) return FALSE;
	if(gtodo_client_category_exists(cl, name)) return FALSE;
	newn = xmlNewTextChild(cl->root, NULL, "category", NULL);	
	xmlNewProp(newn, "title", name);
	buf = g_strdup_printf("%i", cl->number_of_categories);
	cl->number_of_categories++;
	xmlNewProp(newn, "place", buf);
	g_free(buf);
	gtodo_client_save_xml(cl,NULL);
	return TRUE;
}

gboolean gtodo_client_category_remove(GTodoClient *cl, gchar *name)
{
	gint id=-1;
	if(cl == NULL || name == NULL) return FALSE;
	if(!gtodo_client_category_exists(cl, name)) return FALSE;
	else
	{
		xmlNodePtr cur = cl->root->xmlChildrenNode;
		/*		gtodo_client_block_changed_callback(cl);
		*/		while(cur != NULL){
			if(xmlStrEqual(cur->name, "category")){
				xmlChar *temp = xmlGetProp(cur, (const xmlChar *)"title");
				if(xmlStrEqual(temp, (const xmlChar *)name))
				{
					xmlChar *idchar =  xmlGetProp(cur, (const xmlChar *)"place");
					if(idchar != NULL) id = atoi(idchar);
					xmlFree(idchar);
					xmlUnlinkNode(cur);
					xmlFreeNode(cur);
					cur = NULL;
				}
				else cur = cur->next;
				xmlFree(temp);	
			}
			else cur = cur->next;
		}
	}
	gtodo_client_save_xml(cl,NULL);	    
	/* one number is removed.. now we need to renumber.. */
	if(id >= -1)
	{
		GTodoList *list = gtodo_client_get_category_list(cl);
		if(list != NULL)
		{
			do{
				int ref_id = gtodo_client_get_category_id_from_list(list);
				if(id < ref_id)
				{
					gchar *name = gtodo_client_get_category_from_list(list);
					gtodo_client_category_set_id(cl, name, (ref_id -1));
				}
			}while(gtodo_client_get_list_next(list));

		}
		gtodo_client_free_category_list(cl, list);
	}

	gtodo_client_save_xml(cl,NULL);
	/* this doesnt work.. have to adapt gtodo to do that 
	   gtodo_client_unblock_changed_callback(cl);
	   */
	return TRUE;
}


void gtodo_client_block_changed_callback(GTodoClient *cl)
{
	if(cl->timeout != NULL)
	{
		gnome_vfs_monitor_cancel(cl->timeout);
	}
	cl->timeout = NULL;
}

void gtodo_client_unblock_changed_callback(GTodoClient *cl)
{
	if(cl->timeout == NULL)
	{
		gnome_vfs_monitor_add(&cl->timeout,cl->xml_path,
				GNOME_VFS_MONITOR_FILE,
				(GnomeVFSMonitorCallback)check_item_changed, cl);

	}
}
/* This function is deprecated now */
void gtodo_client_reset_changed_callback(GTodoClient *cl)
{
	if(cl->timeout != NULL) return;
}

void gtodo_client_get_list_first(GTodoList *list)
{
	list->list = list->first;
}

/* als base niewer is dan test dan positief */
long int gtodo_item_compare_latest(GTodoItem *base, GTodoItem *test)
{
	if(base == NULL || test == NULL) return 0;
	return base->last_edited-test->last_edited;
}

/* make duplicate an exact copy of source */
void gtodo_client_save_client_to_client(GTodoClient *source, GTodoClient *duplicate)
{
	gtodo_client_save_xml_to_file(source, duplicate->xml_path,NULL);
	gtodo_client_reload(duplicate);
}

gboolean gtodo_client_get_read_only(GTodoClient *cl)
{
	if(cl == NULL) return FALSE;
	return cl->read_only;	
}
