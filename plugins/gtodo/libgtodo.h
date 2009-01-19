#ifndef __LIBGTODO__
#define __LIBGTODO__

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <gio/gio.h>
#define GTODO_NO_DUE_DATE 99999999
/* the GError stuff */
#define LIBGTODO_ERROR g_quark_from_static_string("libgtodo-error-quark")

typedef enum {
	LIBGTODO_ERROR_OK, /* no error */
	LIBGTODO_ERROR_FAILED,
	LIBGTODO_ERROR_GENERIC,	
	LIBGTODO_ERROR_NO_FILE,
	LIBGTODO_ERROR_NO_FILENAME,
	LIBGTODO_ERROR_READ_ONLY,
	LIBGTODO_ERROR_NO_PERMISSION,
	LIBGTODO_ERROR_GNOME_VFS,
	LIBGTODO_ERROR_XML
} LibGTodoError;

enum {
	GTODO_DUE_TIME_HOURE,
	GTODO_DUE_TIME_MINUTE
};

/* The structure used by the gtodo client, The user shouldnt have to use ANY 
   of the internal variables, NEVER */

typedef struct _GTodoClient{
	/* the function gtodo should call when the backend data changes */
	void  *(* function)(gpointer cl, gpointer data);
	/* the pointer it should pass to that function */
	gpointer data;
	/* the last time the backend was edited */
	time_t last_edit;
	/* the timeout UID that is used to check again every x ms. */
	GFileMonitor *timeout;
	/* the path to the backend (in this case xml, should I make it more generic?) */
	GFile *xml_file;
	/* the pointer to the xml structure */
	xmlDocPtr gtodo_doc;
	/* pointer to the first node */
	xmlNodePtr root;
	/* number of categories */
	gint number_of_categories;
	/* permissions */
	gboolean read_only;
} GTodoClient;

/* The GTodoList structure..  */
/* NO public fields */
typedef struct _GTodoList{
	GList *list;
	GList *first;
} GTodoList;

/* The gtodo items, don't access the internal variables youreself, NEVER.
 * Use the wrapper functions 
 * I should move this out of this .h file
 */

typedef struct _GTotoItem{
	/* ID of todo item.  This one should be unique */
	/* for now I made it the time.. that should be unique enough for my purpose */
	GTime id;

	/* last edited, probly needed for syncing and stuff */
	GTime last_edited;

	/* Creation Date.  And possible end date */
	GDate *start;

	/* if NULL no due date*/
	GDate *stop;

	/* Done */
	gboolean done;

	/* if the item should notify when done */
	gboolean notify;

	/* Category name */
	gchar *category;

	/* due date if NULL no due date*/
	GDate *due;

	/* due_time[0] houre due_time[1] minute */
	/* see enumeration above */
	int due_time[2];

	/* Priority  see enumeration*/
	gint priority;

	/* summary */
	gchar *summary;

	/* comment */
	gchar *comment;

} GTodoItem;

/* the three different priorities..   try to use this enumeration */
enum 	{
	GTODO_PRIORITY_LOW,
	GTODO_PRIORITY_MEDIUM,
	GTODO_PRIORITY_HIGH
};

/*** The functions **/
gboolean gtodo_client_get_read_only(GTodoClient *cl);
/********************** GTodoItem *************************/
/* create an empty GTodoItem.. THIS HAS NO UID YET */
GTodoItem * gtodo_client_create_empty_todo_item(void);

/* create a new unique GTodoItem,use this to add an todo item */
GTodoItem * gtodo_client_create_new_todo_item(GTodoClient *cl);

/* free's an GTodoItem */
void gtodo_todo_item_free(GTodoItem *item);

/* Get the ID from the GTodoItem  */
guint32 gtodo_todo_item_get_id(GTodoItem *item);

/* There is no set ID because the ID should always be unique.. if you got good reasons to want this.. mail me */


/* enable or disable the flag that says if this item may produce a notification event */
void gtodo_todo_item_set_notify(GTodoItem *item, gboolean notify);

/* get the status of the notification flag */
gboolean gtodo_todo_item_get_notify(GTodoItem *item);


/* get the priority. see enumeration for possible return values */
int gtodo_todo_item_get_priority(GTodoItem *item);

/* set the priority, for possible value's look at the enumeration for possible values*/
void gtodo_todo_item_set_priority(GTodoItem *item, int priority);


/* get the summary, the returned value should NOT be FREED.*/
/* I return an empty string when there is no value. */
char *gtodo_todo_item_get_summary(GTodoItem *item);

/* set the summary, also setting to NULL, or changing is allowed */
void gtodo_todo_item_set_summary(GTodoItem *item, gchar *summary);


/* get the category name, the returned value should NOT be freed.*/
char *gtodo_todo_item_get_category(GTodoItem *item);

/* get the category id, this is only needed internal */
/* can be handy if you want to switch items. */
gint gtodo_client_get_category_id_from_list(GTodoList *list);

/* move the category up in the category list */
gboolean gtodo_client_category_move_up(GTodoClient *cl, gchar *name);
/* move the category down in the category list */
gboolean gtodo_client_category_move_down(GTodoClient *cl, gchar *name);

/* set the category, or changing is allowed. */
/* FIXME: if category exists, if not.. create it. */
void gtodo_todo_item_set_category(GTodoItem *item, gchar *category);


/* get the comment, this can be a multi line string */
char *gtodo_todo_item_get_comment(GTodoItem *item);


/* set the comment, setting to NULL is allowed, or changing */
void gtodo_todo_item_set_comment(GTodoItem *item, gchar *comment);


/*get the done flag */
gboolean gtodo_todo_item_get_done(GTodoItem *item);

/* set the done flag */
/* this also sets the stop date (or unsets it) */
void gtodo_todo_item_set_done(GTodoItem *item, gboolean done);


/* check if the GTodoItem is allready due on a day bases.  you only need to check for time if it returns 0*/
/* > 0 allready due */
/* 0 due today */
/* <0 due in future */
gint gtodo_todo_item_check_due(GTodoItem *item);


/*returns the time in minutes that is still left.. it will return 0 when there is nothing left or if it isnt today. */
int gtodo_todo_item_check_due_time_minutes_left(GTodoItem *item);

/* get the due date in severall format's */
/* returned value represents an julian date ... 1 = no date set */
guint32 gtodo_todo_item_get_due_date_as_julian(GTodoItem *item);


/* set due date returns false when not able to set */
gboolean gtodo_todo_item_set_due_date_as_julian(GTodoItem *item, guint32 julian);

/* get the due date as an GDate */
GDate * gtodo_todo_item_get_due_date(GTodoItem *item);


/* get localized string.. this needs to be FREED! */
gchar *gtodo_todo_item_get_due_date_as_string(GTodoItem *item);


/* return the houre of the due time */
gint gtodo_todo_item_get_due_time_houre(GTodoItem *item);


/* return the houre of the due time */
gint gtodo_todo_item_get_due_time_minute(GTodoItem *item);


/* return the houre of the due time */
gint gtodo_todo_item_set_due_time_minute(GTodoItem *item, gint minute);


/* return the houre of the due time */
gint gtodo_todo_item_set_due_time_houre(GTodoItem *item, gint houre);

/* Get the last edited date in severall dates. */
/* The last edited date entry is modified when saving the gtodo item. (or changing) */

/* The returned value represent an julian date ... -1 = no date set */
guint32 gtodo_todo_item_get_last_edited_date_as_julian(GTodoItem *item);

/* FIXME: more formats needed? */

/* get or set the start date in severall format's */
/* returned value represent an julian date ... -1 = no date set */
guint32 gtodo_todo_item_get_start_date_as_julian(GTodoItem *item);


/* set start date returns false when not able to set */
gboolean gtodo_todo_item_set_start_date_as_julian(GTodoItem *item, guint32 julian);


/* get an localized string representing the start date.. this needs to be FREED! */
gchar *gtodo_todo_item_get_start_date_as_string(GTodoItem *item);

/* get the stop date in severall format's */
/* returned value represent an julian date ... 1 = no date set */
guint32 gtodo_todo_item_get_stop_date_as_julian(GTodoItem *item);


/* set stop date as a julian date. returns false when not able to set */
gboolean gtodo_todo_item_set_stop_date_as_julian(GTodoItem *item, guint32 julian);


/* Set the stop date to today.. this is a function added to stop duplicating of code.*/
/* You probly don't need to use this, because it's allready set by the gtodo_item_set_done */
gboolean gtodo_todo_item_set_stop_date_today(GTodoItem *item);


/* get localized string.. this needs to be FREED! */
gchar *gtodo_todo_item_get_stop_date_as_string(GTodoItem *item);


/**************************** GTodoClient functions ******************************/

/* this causes the client to save its xml file... */
/* Only in very special cases you want todo this, */
/* its done automatically. USE WITH CARE */
int gtodo_client_save_xml(GTodoClient *cl, GError **error);

/* The above function should never be used, this can be useful f.e. backups or incase of syncronising */
int gtodo_client_save_xml_to_file(GTodoClient *cl, GFile *file, GError **error);

/* reloads the client backend data*/
gboolean gtodo_client_reload(GTodoClient *cl, GError **error);

/* Loads a file */
gboolean gtodo_client_load(GTodoClient *cl, GFile *xml_file, GError **error);

/* creates a new GTodoClient that opens the default backend */
GTodoClient * gtodo_client_new_default(GError **error);

/* creates a new GTodoClient that opens and alternative backend location*/
GTodoClient * gtodo_client_new_from_file(char *filename, GError **error);

/* quit the client, it makes sure everything is saved and free's the needed data */
void gtodo_client_quit(GTodoClient *cl);

/* set the fucntion it should call when the todo database has changed */
/* function should be of type  void functionname(GTodoType *cl, gpointer data); */
void gtodo_client_set_changed_callback(GTodoClient *cl, void *(*function)(gpointer cl, gpointer data), gpointer data);

/* destroys the changed callback */
void gtodo_client_destroy_changed_callback(GTodoClient *cl, void *(*function)(gpointer cl, gpointer data), gpointer data);

/* block the signal from being emitted.. */
void gtodo_client_block_changed_callback(GTodoClient *cl);

/* unblock the callback.. if there was a change between the block and the unblock it will be called after this function */
void gtodo_client_unblock_changed_callback(GTodoClient *cl);

/* Deprecated now */
/* if you don't want to have the callback function to be called between a block and unblock  call this function directly before unblocking */
void gtodo_client_reset_changed_callback(GTodoClient *cl);


/************** GTodoLists ******************/

/* function to step through the list */
/* it will return FALSE when no more itmes in the list.*/
/* otherwise it will let list point to the next item */
gboolean gtodo_client_get_list_next(GTodoList *list);

/* set the GTodoList to the first item in the list */
void gtodo_client_get_list_first(GTodoList *list);

/* get the category list..  Makesure you don't mix a Category list with a todo item list.. */
/* they use the same GTodoList, but the internall data is different */
/* the returned list has to be freed when no longer needed.*/
/* the returned list isnt kept up to date.. */

GTodoList * gtodo_client_get_category_list(GTodoClient *cl);

/* free the Category list and all the items it contains*/
void gtodo_client_free_category_list(GTodoClient *cl, GTodoList *list);

/* get the category from the list, use gtodo_client_get_list_next to cycle through them, Don't free the result */
gchar * gtodo_client_get_category_from_list(GTodoList *list);

/* get a GTodoList with GtodoItem. */
/* make sure you don't mix this one with an category list */
/* if you Pass  NULL for the category you get all items in all categories */
/* the returned list has to be freed when no longer needed.*/
/* the returned list isnt kept up to date.. */
GTodoList *gtodo_client_get_todo_item_list(GTodoClient *cl, gchar *category);

/* get an GTodoItem from the list.. you dont need to free this */
/* use gtodo_client_get_list_next to cycle through them*/
GTodoItem *gtodo_client_get_todo_item_from_list(GTodoList *list);

/* free the todo's items */
void gtodo_client_free_todo_item_list(GTodoClient *cl, GTodoList *list);

/* get an todo item from is UID. */
/* returns NULL when it doesnt exists.. */
/* make sure you free the result */
GTodoItem *gtodo_client_get_todo_item_from_id(GTodoClient *cl, guint32 id);


/* saves an GtodoItem.. Dont use this to modify an GtodoItem.. */
/* if the todo item doesnt have a existing category, it will be created */
gboolean gtodo_client_save_todo_item(GTodoClient *cl, GTodoItem *item);;

/* You should get the todo item first..  then edit the item and pass it to this function apply the changes*/
gboolean gtodo_client_edit_todo_item(GTodoClient *cl, GTodoItem *item);

/* Delete an todo item by its ID*/ 
void gtodo_client_delete_todo_by_id(GTodoClient *cl, guint32 id);


/* change the name of a Category*/
/* returns TRUE is successfull */
gboolean gtodo_client_category_edit(GTodoClient *cl, gchar *old, gchar *newn);

/* Check if an category allready exists */
gboolean gtodo_client_category_exists(GTodoClient *cl, gchar *name);

/* create a new category */
gboolean gtodo_client_category_new(GTodoClient *cl, gchar *name);

/* remove a category and ALL the todo items in it */
gboolean gtodo_client_category_remove(GTodoClient *cl, gchar *name);




/* it checks two GToDo items to test witch one is the last edited. */
/* it returns positive when base is newer and negative when test.*/
long int gtodo_item_compare_latest(GTodoItem *base, GTodoItem *test);


/* make duplicate an exact copy of source */
/* could be usefull for syncronising */
void gtodo_client_save_client_to_client(GTodoClient *source, GTodoClient *duplicate);

gboolean gtodo_client_export(GTodoClient *source, GFile *dest, const gchar *path_to_xsl, gchar **params, GError **error);

#endif
