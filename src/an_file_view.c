#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "an_file_view.h"
#include "file_history.h"
#include "anjuta.h"

#include "cfolder.xpm"
#include "ofolder.xpm"
#include "fv_source.xpm"
#include "fv_exec.xpm"
#include "fv_image.xpm"
#include "fv_doc.xpm"
#include "fv_i18n.xpm"
#include "fv_autofile.xpm"
#include "fv_unknown.xpm"

static const char *image_extn[] = { "gif", "jpg", "jpeg", "png", "ico"
	, "bmp", "xpm", "mpeg", "mng", "svg", NULL };

typedef enum
{
	fv_unknown_t,
	fv_source_t,
	fv_exec_t,
	fv_image_t,
	fv_doc_t,
	fv_i18n_t,
	fv_autofile_t,
	fv_cfolder_t,
	fv_ofolder_t,
	fv_max_t
} FVFileType;

static GdkPixmap **fv_icons = NULL;
static GdkBitmap **fv_bitmaps = NULL;
static AnFileView *fv = NULL;

static gboolean fv_is_image_file(const char *file_name)
{
	const char **im_extn;
	const char *extn;
	g_return_val_if_fail(file_name, FALSE);

	extn = strrchr(file_name, '.');
	if (!extn)
		extn = file_name;
	else
		++ extn;
	for (im_extn = image_extn; *im_extn; ++ im_extn)
	{
		if (0 == strcmp(*im_extn, extn))
			return TRUE;
	}
	return FALSE;
}

static gboolean fv_is_auto_file(const char *name)
{
	g_return_val_if_fail(name, FALSE);

	return (gboolean) ((0 == strcmp("configure", name))
	  || (0 == strcmp("configure.in", name))
	  || (0 == strcmp("Makefile", name))
	  || (0 == strcmp("Makefile.in", name))
	  || (0 == strcmp("Makefile.am", name))
	  || (0 == strcmp("GNUMakefile", name))
	  || (0 == strcmp("makefile", name)));
}

static FVFileType fv_get_file_type(const char *name)
{
	g_return_val_if_fail(name, fv_unknown_t);
	if (fv_is_auto_file(name))
		return fv_autofile_t;
	else if (fv_is_image_file(name))
		return fv_image_t;
	else if (tm_project_is_source_file(fv->project, name))
		return fv_source_t;
	else
	{
		const char *extn = strrchr(name, '.');
		if (!extn)
			extn = name;
		else
			++ extn;
		if ((0 == strcmp(extn, "xml"))
			  || (0 == strcmp(extn, "html"))
			  || (0 == strcmp(extn, "doc"))
			  || (0 == strcmp(extn, "txt"))
			  || (0 == strcmp(extn, "sgml"))
			  || (0 == strcmp(extn, "README"))
			  || (0 == strcmp(extn, "AUTHORS"))
			  || (0 == strcmp(extn, "COPYING"))
			  || (0 == strcmp(extn, "NEWS"))
			  || (0 == strcmp(extn, "INSTALL"))
			  || (0 == strcmp(extn, "TODO"))
			  || (0 == strcmp(extn, "HACKING"))
			  || (0 == strcmp(extn, "THANKS"))
			  || (0 == strcmp(extn, "ChangeLog")))
			return fv_doc_t;
		else if (0 == strcmp(extn, "po"))
			return fv_i18n_t;
		else
			return fv_unknown_t;
	}
	g_assert_not_reached();
	return fv_unknown_t;
}

static void fv_load_pixmaps(void)
{
	if (fv_icons)
		return;
	g_return_if_fail(fv && fv->win);
	fv_icons = g_new(GdkPixmap *, fv_max_t);
	fv_bitmaps = g_new(GdkBitmap *, fv_max_t);
	fv_icons[fv_unknown_t] = gdk_pixmap_colormap_create_from_xpm_d(
	  NULL,	gtk_widget_get_colormap(fv->win), &fv_bitmaps[fv_unknown_t]
	  , NULL, fv_unknown_xpm);
	fv_icons[fv_autofile_t] = gdk_pixmap_colormap_create_from_xpm_d(
	  NULL,	gtk_widget_get_colormap(fv->win), &fv_bitmaps[fv_autofile_t]
	  , NULL, fv_autofile_xpm);
	fv_icons[fv_source_t] = gdk_pixmap_colormap_create_from_xpm_d(
	  NULL,	gtk_widget_get_colormap(fv->win), &fv_bitmaps[fv_source_t]
	  , NULL, fv_source_xpm);
	fv_icons[fv_exec_t] = gdk_pixmap_colormap_create_from_xpm_d(
	  NULL,	gtk_widget_get_colormap(fv->win), &fv_bitmaps[fv_exec_t]
	  , NULL, fv_exec_xpm);
	fv_icons[fv_image_t] = gdk_pixmap_colormap_create_from_xpm_d(
	  NULL,	gtk_widget_get_colormap(fv->win), &fv_bitmaps[fv_image_t]
	  , NULL, fv_image_xpm);
	fv_icons[fv_doc_t] = gdk_pixmap_colormap_create_from_xpm_d(
	  NULL,	gtk_widget_get_colormap(fv->win), &fv_bitmaps[fv_doc_t]
	  , NULL, fv_doc_xpm);
	fv_icons[fv_i18n_t] = gdk_pixmap_colormap_create_from_xpm_d(
	  NULL,	gtk_widget_get_colormap(fv->win), &fv_bitmaps[fv_i18n_t]
	  , NULL, fv_i18n_xpm);
	fv_icons[fv_cfolder_t] = gdk_pixmap_colormap_create_from_xpm_d(
	  NULL,	gtk_widget_get_colormap(fv->win), &fv_bitmaps[fv_cfolder_t]
	  , NULL, cfolder_xpm);
	fv_icons[fv_ofolder_t] = gdk_pixmap_colormap_create_from_xpm_d(
	  NULL,	gtk_widget_get_colormap(fv->win), &fv_bitmaps[fv_ofolder_t]
	  , NULL, ofolder_xpm);
	fv_icons[fv_max_t] = NULL;
	fv_bitmaps[fv_max_t] = NULL;
}

static void fv_on_button_press(GtkWidget *widget
  , GdkEventButton *event, gpointer user_data)
{
}

static void fv_on_select_row(GtkCList *clist, gint row, gint column
  , GdkEventButton *event, gpointer user_data)
{
	GtkCTreeNode *node;
	char *full_filename;

	node = gtk_ctree_node_nth(GTK_CTREE(fv->tree), row);
	if (!node || !event || row < 0 || column < 0)
		return;
	if (event->type != GDK_2BUTTON_PRESS)
		return;
	if (event->button != 1)
		return;
	full_filename = (char *) gtk_ctree_node_get_row_data(
	  GTK_CTREE(fv->tree), GTK_CTREE_NODE(node));
	if (full_filename)
		anjuta_goto_file_line_mark(full_filename, -1, FALSE);
}

static void fv_disconnect(void)
{
	g_return_if_fail(fv);
	gtk_signal_disconnect_by_func(GTK_OBJECT(fv->tree)
	  , GTK_SIGNAL_FUNC(fv_on_select_row), NULL);
	gtk_signal_disconnect_by_func(GTK_OBJECT(fv->tree)
	  , GTK_SIGNAL_FUNC(fv_on_button_press), NULL);
}

static void fv_connect(void)
{
	g_return_if_fail(fv && fv->tree);
	gtk_signal_connect(GTK_OBJECT(fv->tree), "select_row"
	  , GTK_SIGNAL_FUNC(fv_on_select_row), NULL);
	gtk_signal_connect(GTK_OBJECT(fv->tree), "button_press_event"
	  , GTK_SIGNAL_FUNC(fv_on_button_press), NULL);
}

static void fv_create(void)
{
	fv = g_new(AnFileView, 1);
	fv->project = NULL;
	fv->win=gtk_scrolled_window_new(NULL,NULL);
	gtk_widget_ref(fv->win);
	gtk_widget_show(fv->win);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(fv->win),
	  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	fv->tree=gtk_ctree_new(1,0);
	gtk_widget_ref(fv->tree);
	gtk_widget_show(fv->tree);
	gtk_ctree_set_line_style(GTK_CTREE(fv->tree), GTK_CTREE_LINES_DOTTED);
	gtk_clist_set_column_auto_resize(GTK_CLIST(fv->tree), 0, TRUE);
	gtk_clist_set_selection_mode(GTK_CLIST(fv->tree)
	  ,GTK_SELECTION_BROWSE);
	gtk_container_add (GTK_CONTAINER(fv->win), fv->tree);
	if (!fv_icons)
		fv_load_pixmaps();
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
	fv->project = NULL;
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

	g_return_if_fail(entry && entry->path && entry->name &&
	  fv && fv->project && fv->tree);
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
	gtk_ctree_sort_node(GTK_CTREE(fv->tree), parent);
	for (tmp = entry->children; tmp; tmp = g_slist_next(tmp))
	{
		child = (TMFileEntry *) tmp->data;
		if (tm_file_dir_t == entry->type)
			fv_add_tree_entry(child, parent);
	}
}

AnFileView *fv_populate(TMProject *tm_proj)
{
#ifdef DEBUG
	g_message("Populating file view..");
#endif
	if (!fv)
		fv_create();
	fv_disconnect();
	fv_freeze();
	fv_clear();
	if (!tm_proj) goto clean_leave;
	if (!IS_TM_PROJECT((TMWorkObject *) tm_proj)) goto clean_leave;
	if (!(tm_proj->file_tree)) goto clean_leave;
	if (!(tm_proj->dir)) goto clean_leave;
	
	fv->project = tm_proj;
	fv_add_tree_entry(tm_proj->file_tree, NULL);

clean_leave:
	fv_connect();
	fv_thaw();
	return fv;
}
