#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gnome.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>

#include "anjuta.h"
#include "resources.h"
#include "pixmaps.h"

#include "an_file_view.h"

typedef enum
{
	fv_unknown_t,
	fv_text_t,
	fv_image_t,
	fv_exec_t,
	fv_core_t,
	fv_cfolder_t,
	fv_ofolder_t,
	fv_max_t
} FVFileType;

static GdkPixmap **fv_icons = NULL;
static GdkBitmap **fv_bitmaps = NULL;
static AnFileView *fv = NULL;

#define CREATE_FV_ICON(N, F) fv_icons[(N)] = gdk_pixmap_colormap_create_from_xpm(\
  NULL,	gtk_widget_get_colormap(fv->win), &fv_bitmaps[(N)],\
  NULL, anjuta_res_get_pixmap_file(F));

static void fv_load_pixmaps(void)
{
	if (fv_icons)
		return;
	g_return_if_fail(fv && fv->win);
	fv_icons = g_new(GdkPixmap *, (fv_max_t+1));
	fv_bitmaps = g_new(GdkBitmap *, (fv_max_t+1));
	CREATE_FV_ICON(fv_unknown_t, ANJUTA_PIXMAP_FV_UNKNOWN);
	CREATE_FV_ICON(fv_text_t, ANJUTA_PIXMAP_FV_TEXT);
	CREATE_FV_ICON(fv_image_t, ANJUTA_PIXMAP_FV_IMAGE);
	CREATE_FV_ICON(fv_exec_t, ANJUTA_PIXMAP_FV_EXECUTABLE);
	CREATE_FV_ICON(fv_core_t, ANJUTA_PIXMAP_FV_CORE);
	CREATE_FV_ICON(fv_cfolder_t, ANJUTA_PIXMAP_CLOSED_FOLDER);
	CREATE_FV_ICON(fv_ofolder_t, ANJUTA_PIXMAP_OPEN_FOLDER);
	fv_icons[fv_max_t] = NULL;
	fv_bitmaps[fv_max_t] = NULL;
}

static FVFileType fv_get_file_type(const char *name)
{
	const char *mime_type = gnome_vfs_mime_type_from_name(name);
	if (0 == strncmp(mime_type, "text/", 5))
		return fv_text_t;
	else if (0 == strncmp(mime_type, "image/", 6))
		return fv_image_t;
	else if (0 == strcmp(mime_type, "application/x-executable-binary"))
		return fv_exec_t;
	else if (0 == strcmp(mime_type, "application/x-core-file"))
		return fv_core_t;
	else
		return fv_unknown_t;
}

gboolean anjuta_fv_open_file(const char *path, gboolean use_anjuta)
{
	gboolean status = FALSE;
	const char *mime_type = gnome_vfs_get_file_mime_type(path, NULL, FALSE);
	if (use_anjuta)
	{
		anjuta_goto_file_line_mark((char *) path, -1, FALSE);
		status = TRUE;
	}
	else
	{
		GnomeVFSMimeApplication *app = gnome_vfs_mime_get_default_application(mime_type);
		if ((app) && (app->command))
		{
			char **argv = g_new(char *, 3);
			argv[0] = app->command;
			argv[1] = (char *) path;
			argv[2] = NULL;
			if (-1 == gnome_execute_async(NULL, 2, argv))
				anjuta_warning(_("Unable to open %s in %s"), path, app->command);
			gnome_vfs_mime_application_free(app);
			status = TRUE;
		}
		else
		{
			anjuta_warning(_("No default viewer specified for the mime type %s.\n"
					"Please set it in GNOME control center"), mime_type);
		}
	}
	return status;
}

typedef enum
{
	OPEN,
	VIEW,
	REFRESH,
	MENU_MAX
} FVSignal;

static void fv_context_handler(GtkMenuItem *item, gpointer user_data)
{
	FVSignal signal = (FVSignal) user_data;
	switch (signal)
	{
		case OPEN:
			if (fv->file)
				anjuta_fv_open_file(fv->file, TRUE);
			break;
		case VIEW:
			if (fv->file)
				anjuta_fv_open_file(fv->file, FALSE);
			break;
		case REFRESH:
			fv_populate();
			break;
		default:
			break;
	}
}

static void fv_create_context_menu(void)
{
	GtkWidget *item;
	fv->menu = gtk_menu_new();
	gtk_widget_ref(fv->menu);
	gtk_widget_show(fv->menu);
	item = gtk_menu_item_new_with_label(_("Open"));
	gtk_signal_connect(GTK_OBJECT(item), "activate"
	  , GTK_SIGNAL_FUNC(fv_context_handler)
	  , (gpointer) OPEN);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(fv->menu), item);
	item = gtk_menu_item_new_with_label(_("View in Default Viewer"));
	gtk_signal_connect(GTK_OBJECT(item), "activate"
	  , GTK_SIGNAL_FUNC(fv_context_handler)
	  , (gpointer) VIEW);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(fv->menu), item);
	item = gtk_menu_item_new_with_label(_("Refresh Tree"));
	gtk_signal_connect(GTK_OBJECT(item), "activate"
	  , GTK_SIGNAL_FUNC(fv_context_handler)
	  , (gpointer) REFRESH);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(fv->menu), item);
}

static gboolean
fv_on_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	if (!event)
		return FALSE;

	if (event->type == GDK_BUTTON_PRESS) {
		if (((GdkEventButton *) event)->button != 3)
			return FALSE;
		
		/* Popup project menu */
		gtk_menu_popup(GTK_MENU(fv->menu), NULL, NULL, NULL, NULL
		  , ((GdkEventButton *) event)->button, ((GdkEventButton *) event)->time);
		
		return TRUE;
	} else if (event->type == GDK_KEY_PRESS){
		gint row;
		GtkCTree *tree;
		GtkCTreeNode *node;
		tree = GTK_CTREE(widget);
		row = tree->clist.focus_row;
		node = gtk_ctree_node_nth(tree,row);
		
		if (!node)
			return FALSE;
			
		switch(((GdkEventKey *)event)->keyval) {
			case GDK_space:
			case GDK_Return:
				if(GTK_CTREE_ROW(node)->is_leaf) {
					fv->file = (char *) gtk_ctree_node_get_row_data(
					  GTK_CTREE(fv->tree), GTK_CTREE_NODE(node));
					if (fv->file)
						anjuta_fv_open_file(fv->file, TRUE);
				}
				break;
			case GDK_Left:
				gtk_ctree_collapse(tree, node);
				break;
			case GDK_Right:
				gtk_ctree_expand(tree, node);
				break;
			default:
				return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}

static void fv_on_select_row(GtkCList *clist, gint row, gint column
  , GdkEventButton *event, gpointer user_data)
{
	GtkCTreeNode *node = gtk_ctree_node_nth(GTK_CTREE(fv->tree), row);
	if (!node || !event || row < 0 || column < 0)
		return;
	fv->file = (char *) gtk_ctree_node_get_row_data(
	  GTK_CTREE(fv->tree), GTK_CTREE_NODE(node));
	if ((event->type == GDK_2BUTTON_PRESS) && (event->button == 1))
	{
		if (fv->file)
			anjuta_fv_open_file(fv->file, TRUE);
	}
}

static void fv_disconnect(void)
{
	g_return_if_fail(fv);
	gtk_signal_disconnect_by_func(GTK_OBJECT(fv->tree)
	  , GTK_SIGNAL_FUNC(fv_on_select_row), NULL);
	gtk_signal_disconnect_by_func(GTK_OBJECT(fv->tree)
	  , GTK_SIGNAL_FUNC(fv_on_event), NULL);
}

static void fv_connect(void)
{
	g_return_if_fail(fv && fv->tree);
	gtk_signal_connect(GTK_OBJECT(fv->tree), "select_row"
	  , GTK_SIGNAL_FUNC(fv_on_select_row), NULL);
	gtk_signal_connect(GTK_OBJECT(fv->tree), "event"
	  , GTK_SIGNAL_FUNC(fv_on_event), NULL);
}

static void fv_create(void)
{
	fv = g_new0(AnFileView, 1);
	fv->win=gtk_scrolled_window_new(NULL,NULL);
	gtk_widget_ref(fv->win);
	gtk_widget_show(fv->win);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(fv->win),
	  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	fv->tree=gtk_ctree_new(1,0);
	gtk_ctree_set_line_style (GTK_CTREE(fv->tree), GTK_CTREE_LINES_DOTTED);
	gtk_ctree_set_expander_style (GTK_CTREE(fv->tree), GTK_CTREE_EXPANDER_SQUARE);
	gtk_widget_ref(fv->tree);
	gtk_widget_show(fv->tree);
	gtk_clist_set_column_auto_resize(GTK_CLIST(fv->tree), 0, TRUE);
	gtk_clist_set_selection_mode(GTK_CLIST(fv->tree)
	  ,GTK_SELECTION_BROWSE);
	gtk_container_add (GTK_CONTAINER(fv->win), fv->tree);
	if (!fv_icons)
		fv_load_pixmaps();
	fv_create_context_menu();
	fv_connect();
}

static void fv_freeze(void)
{
	g_return_if_fail(fv && fv->tree);
	gtk_clist_freeze(GTK_CLIST(fv->tree));
}

static void fv_thaw(void)
{
	g_return_if_fail(fv && fv->tree);
	gtk_clist_thaw(GTK_CLIST(fv->tree));
}

void fv_clear(void)
{
	g_return_if_fail(fv && fv->tree);
	gtk_clist_clear(GTK_CLIST(fv->tree));
}

static void fv_add_tree_entry(TMFileEntry *entry, GtkCTreeNode *root)
{
	char *arr[1];
	FVFileType type;
	GtkCTreeNode *parent, *item;
	TMFileEntry *child;
	GSList *tmp;
	GdkPixmap *closed_icon, *open_icon;
	GdkBitmap *closed_bitmap, *open_bitmap;

	if (!entry || !entry->path || !entry->name || !fv || !fv->tree)
		return;
	if ((tm_file_dir_t != entry->type) || (!entry->children))
		return;
	closed_icon = fv_icons[fv_cfolder_t];
	closed_bitmap = fv_bitmaps[fv_cfolder_t];
	open_icon = fv_icons[fv_ofolder_t];
	open_bitmap = fv_bitmaps[fv_ofolder_t];
	arr[0] = entry->name;
	parent = gtk_ctree_insert_node(GTK_CTREE(fv->tree), root, NULL
	  , arr, 5, closed_icon, closed_bitmap, open_icon, open_bitmap
	  , FALSE, !root);
	for (tmp = entry->children; tmp; tmp = g_slist_next(tmp))
	{
		child = (TMFileEntry *) tmp->data;
		if (tm_file_dir_t == entry->type)
			fv_add_tree_entry(child, parent);
	}
	for (tmp = entry->children; tmp; tmp = g_slist_next(tmp))
	{
		child = (TMFileEntry *) tmp->data;
		if (tm_file_regular_t == child->type)
		{
			type = fv_get_file_type(child->name);
			closed_icon = fv_icons[type];
			closed_bitmap = fv_bitmaps[type];
			open_icon = fv_icons[type];
			open_bitmap = fv_bitmaps[type];
			arr[0] = child->name;
			item = gtk_ctree_insert_node(GTK_CTREE(fv->tree), parent, NULL
			  , arr, 5, closed_icon, closed_bitmap, open_icon, open_bitmap
			  , TRUE, FALSE);
			gtk_ctree_node_set_row_data_full(GTK_CTREE(fv->tree), item
			  , g_strdup(child->path), (GtkDestroyNotify) g_free);
		}
	}
}

AnFileView *fv_populate(void)
{
	static const char *ignore[] = {"CVS", NULL};
	TMFileEntry *file_tree;
#ifdef DEBUG
	g_message("Populating file view..");
#endif
	if (!fv)
		fv_create();
	fv_disconnect();
	fv_freeze();
	fv_clear();
	if (!app || !app->project_dbase || !app->project_dbase->top_proj_dir)
		goto clean_leave;
	file_tree = tm_file_entry_new(app->project_dbase->top_proj_dir,
	  NULL, TRUE, NULL, ignore, TRUE);
	if (!file_tree)
		goto clean_leave;
	fv_add_tree_entry(file_tree, NULL);
	tm_file_entry_free(file_tree);

clean_leave:
	fv_connect();
	fv_thaw();
	return fv;
}
