/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Jeffrey Stedfast <fejj@ximian.com>
 *
 *  Copyright 2003 Ximian, Inc. (www.ximian.com)
 *  Copyright (C) Massimo Cora' 2005 <maxcvs@gmail.com> 
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <gconf/gconf-client.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>

#include "vgdefaultview.h"
#include "vgrulepattern.h"
#include "vgsearchbar.h"
#include "vgrule-list.h"
#include "menu-utils.h"
#include "vgio.h"
#include "vgactions.h"

#define d(x)
#define w(x) x


#define NUM_LINES_KEY      "/apps/anjuta/valgrind/num-lines"
#define CUSTOM_EDITOR_KEY  "/apps/anjuta/valgrind/editor"
#define SUPPRESSIONS_KEY   "/apps/anjuta/valgrind/general/suppressions"

enum {
	COL_STRING_DISPLAY,
	COL_POINTER_ERROR,
	COL_POINTER_SUMMARY,
	COL_POINTER_STACK,
	COL_LOAD_SRC_PREVIEW,
	COL_IS_SRC_PREVIEW,
	COL_LAST
};

static GType col_types[] = {
	G_TYPE_STRING,
	G_TYPE_POINTER,
	G_TYPE_POINTER,
	G_TYPE_POINTER,
	G_TYPE_BOOLEAN,
	G_TYPE_BOOLEAN
};

static void vg_default_view_class_init (VgDefaultViewClass *klass);
static void vg_default_view_init (VgDefaultView *view);
static void vg_default_view_destroy (GtkObject *obj);
static void vg_default_view_finalize (GObject *obj);

static void valgrind_view_clear (VgToolView *tool);
static void valgrind_view_reset (VgToolView *tool);
static void valgrind_view_connect (VgToolView *tool, int sockfd);
static int  valgrind_view_step (VgToolView *tool);
static void valgrind_view_disconnect (VgToolView *tool);
static int  valgrind_view_save_log (VgToolView *tool, gchar* uri);
static int  valgrind_view_load_log (VgToolView *tool, VgActions *actions, gchar* uri);
static void valgrind_view_cut (VgToolView *tool);
static void valgrind_view_copy (VgToolView *tool);
static void valgrind_view_paste (VgToolView *tool);
static void valgrind_view_show_rules (VgToolView *tool);

static void rule_added (VgRuleList *list, VgRule *rule, gpointer user_data);
static void tree_row_expanded (GtkTreeView *treeview, GtkTreeIter *root, GtkTreePath *path, gpointer user_data);
static gboolean tree_button_press (GtkWidget *treeview, GdkEventButton *event, gpointer user_data);

static void view_show_error (VgDefaultView *view, GtkTreeStore *model, VgError *err);
static void view_rebuild (VgDefaultView *view);


static VgToolViewClass *parent_class = NULL;


GType
vg_default_view_get_type (void)
{
	static GType type = 0;
	
	if (!type) {
		static const GTypeInfo info = {
			sizeof (VgDefaultViewClass),
			NULL, /* base_class_init */
			NULL, /* base_class_finalize */
			(GClassInitFunc) vg_default_view_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (VgDefaultView),
			0,    /* n_preallocs */
			(GInstanceInitFunc) vg_default_view_init,
		};
		
		type = g_type_register_static (VG_TYPE_TOOL_VIEW, "VgDefaultView", &info, 0);
	}
	
	return type;
}

static void
vg_default_view_class_init (VgDefaultViewClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkObjectClass *gtk_object_class = GTK_OBJECT_CLASS (klass);
	VgToolViewClass *tool_class = VG_TOOL_VIEW_CLASS (klass);
	
	parent_class = g_type_class_ref (VG_TYPE_TOOL_VIEW);
	
	object_class->finalize = vg_default_view_finalize;
	gtk_object_class->destroy = vg_default_view_destroy;
	
	/* virtual methods */
	tool_class->clear = valgrind_view_clear;
	tool_class->reset = valgrind_view_reset;
	tool_class->connect = valgrind_view_connect;
	tool_class->step = valgrind_view_step;
	tool_class->disconnect = valgrind_view_disconnect;
	tool_class->save_log = valgrind_view_save_log;
	tool_class->load_log = valgrind_view_load_log;	
	tool_class->cut = valgrind_view_cut;
	tool_class->copy = valgrind_view_copy;
	tool_class->paste = valgrind_view_paste;
	tool_class->show_rules = valgrind_view_show_rules;
}


static GtkWidget *
valgrind_table_new (void)
{
	GtkTreeSelection *selection;
	GtkCellRenderer *renderer;
	GtkTreeStore *model;
	GtkWidget *table;
	
	model = gtk_tree_store_newv (COL_LAST, col_types);
	table = gtk_tree_view_new_with_model (GTK_TREE_MODEL (model));
	
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (table), -1, "",
						     renderer, "text", 0, NULL);
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (table));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (table), FALSE);
	
	return table;
}

enum {
	SEARCH_ID_NONE,
	SEARCH_ID_ERROR,
	SEARCH_ID_FUNCTION,
	SEARCH_ID_OBJECT,
	SEARCH_ID_SOURCE
};	

static VgSearchBarItem search_items[] = {
	{ N_("Error contains"),           SEARCH_ID_ERROR    },
	{ N_("Function contains"),        SEARCH_ID_FUNCTION },
	{ N_("Object contains"),          SEARCH_ID_OBJECT   },
	{ N_("Source filename contains"), SEARCH_ID_SOURCE   },
	{ NULL, 0 }
};

static void
set_search (VgDefaultView *view, int item_id, const char *expr)
{
	GtkWidget *parent, *dialog;
	size_t size;
	char *err;
	int ret;
	
	if (view->search_id != SEARCH_ID_NONE)
		regfree (&view->search_regex);
	
	view->search_id = item_id;
	if (item_id == SEARCH_ID_NONE)
		return;
	
	if ((ret = regcomp (&view->search_regex, expr, REG_EXTENDED | REG_NOSUB)) == 0)
		return;
	
	/* regex compilation failed */
	view->search_id = SEARCH_ID_NONE;
	
	size = regerror (ret, &view->search_regex, NULL, 0);
	err = g_malloc (size);
	regerror (ret, &view->search_regex, err, size);
	
	regfree (&view->search_regex);
	
	parent = gtk_widget_get_toplevel (GTK_WIDGET (view));
	parent = GTK_WIDGET_TOPLEVEL (parent) ? parent : NULL;
	
	dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
					 _("Invalid regular expression: '%s': %s"),
					 expr, err);
	g_free (err);
	
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

static void
search_bar_search (VgSearchBar *bar, int item_id, VgDefaultView *view)
{
	const char *expr;
	
	expr = vg_search_bar_get_text (bar);
	
	set_search (view, item_id, expr);
	view_rebuild (view);
}

static void
search_bar_clear (VgSearchBar *bar, VgDefaultView *view)
{
	set_search (view, SEARCH_ID_NONE, NULL);
	view_rebuild (view);
}

static void
vg_default_view_init (VgDefaultView *view)
{
	GtkWidget *scrolled;
	GtkWidget *search;
	char *filename;
	
	view->suppressions = g_ptr_array_new ();
	view->errors = g_ptr_array_new ();
	view->search_id = SEARCH_ID_NONE;
	view->parser = NULL;
	view->rules_id = 0;
	view->srclines = 0;
	view->lines_id = 0;
	
	view->gconf = gconf_client_get_default ();
	filename = gconf_client_get_string (view->gconf, SUPPRESSIONS_KEY, NULL);
	view->rule_list = vg_rule_list_new (filename);
	g_signal_connect (view->rule_list, "rule-added", G_CALLBACK (rule_added), view);
	g_object_ref (view->rule_list);
	gtk_object_sink (GTK_OBJECT (view->rule_list));
	gtk_widget_show (view->rule_list);
	g_free (filename);
	
	search = vg_search_bar_new ();
	vg_search_bar_set_menu_items (VG_SEARCH_BAR (search), search_items);
	g_signal_connect (search, "search", G_CALLBACK (search_bar_search), view);
	g_signal_connect (search, "clear", G_CALLBACK (search_bar_clear), view);
	gtk_widget_show (search);
	gtk_box_pack_start (GTK_BOX (view), search, FALSE, FALSE, 3);
	
	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled), GTK_SHADOW_IN);
	
	view->table = valgrind_table_new ();
	gtk_widget_show (view->table);
	
	g_signal_connect (view->table, "row-expanded", G_CALLBACK (tree_row_expanded), view);
	g_signal_connect (view->table, "button-press-event", G_CALLBACK (tree_button_press), view);
	
	gtk_container_add (GTK_CONTAINER (scrolled), view->table);
	gtk_widget_show (scrolled);
	
	/*gtk_container_add (GTK_CONTAINER (view), scrolled);*/
	gtk_box_pack_start (GTK_BOX (view), scrolled, TRUE, TRUE, 0);
}

static void
vg_default_view_finalize (GObject *obj)
{
	VgDefaultView *view = VG_DEFAULT_VIEW (obj);
	int i;
	
	for (i = 0; i < view->suppressions->len; i++)
		vg_rule_pattern_free (view->suppressions->pdata[i]);
	g_ptr_array_free (view->suppressions, TRUE);
	
	for (i = 0; i < view->errors->len; i++)
		vg_error_free (view->errors->pdata[i]);
	g_ptr_array_free (view->errors, TRUE);
	
	if (view->parser) {
		vg_error_parser_free (view->parser);
		view->parser = NULL;
	}
	
	if (view->search_id != SEARCH_ID_NONE)
		regfree (&view->search_regex);
	
	/* set to null AnjutaValgrindPlugin* object */
	view->valgrind_plugin = NULL;
	
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
vg_default_view_destroy (GtkObject *obj)
{
	VgDefaultView *view = VG_DEFAULT_VIEW (obj);
	
	if (view->rule_list) {
		g_object_unref (view->rule_list);
		view->rule_list = NULL;
	}
	
	if (view->gconf) {
		if (view->rules_id != 0) {
			gconf_client_notify_remove (view->gconf, view->rules_id);
			view->rules_id = 0;
		}
		
		if (view->lines_id != 0) {
			gconf_client_notify_remove (view->gconf, view->lines_id);
			view->lines_id = 0;
		}
		
		g_object_unref (view->gconf);
		view->gconf = NULL;
	}
	
	GTK_OBJECT_CLASS (parent_class)->destroy (obj);
}


static void
valgrind_view_clear (VgToolView *tool)
{
	VgDefaultView *view = VG_DEFAULT_VIEW (tool);
	GtkTreeStore *model;
	int i;
	
	model = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (view->table)));
	
	gtk_tree_store_clear (model);
	
	for (i = 0; i < view->errors->len; i++)
		vg_error_free (view->errors->pdata[i]);
	g_ptr_array_set_size (view->errors, 0);
}

static void
valgrind_view_reset (VgToolView *tool)
{
	;
}


static void
valgrind_init_src_preview (VgDefaultView *view, GtkTreeStore *model, GtkTreeIter *root, VgErrorStack *stack)
{
	VgToolView *tool = VG_TOOL_VIEW (view);
	GtkTreeIter iter;
	
	if (!tool->symtab && !tool->srcdir)
		return;
	
	gtk_tree_store_append (model, &iter, root);
	gtk_tree_store_set (model, &iter,
			    COL_STRING_DISPLAY, NULL,
			    COL_POINTER_ERROR, stack->summary->parent,
			    COL_POINTER_SUMMARY, stack->summary,
			    COL_POINTER_STACK, stack,
			    COL_LOAD_SRC_PREVIEW, TRUE,
			    COL_IS_SRC_PREVIEW, TRUE,
			    -1);
}

static void
view_show_error (VgDefaultView *view, GtkTreeStore *model, VgError *err)
{
	VgErrorSummary *summary;
	GtkTreeIter iter, root;
	
	summary = err->summary;
	
	gtk_tree_store_append (model, &iter, NULL);
	gtk_tree_store_set (model, &iter,
			    COL_STRING_DISPLAY, summary->report,
			    COL_POINTER_ERROR, err,
			    COL_POINTER_SUMMARY, summary,
			    COL_POINTER_STACK, NULL,
			    COL_LOAD_SRC_PREVIEW, FALSE,
			    COL_IS_SRC_PREVIEW, FALSE,
			    -1);
	root = iter;
	
	do {
		VgErrorStack *stack;
		
		stack = summary->frames;
		while (stack != NULL) {
			gboolean load = FALSE;
			GString *str;
			
			gtk_tree_store_append (model, &iter, &root);
			
			str = g_string_new (stack->where == VG_WHERE_AT ? "at " : "by ");
			g_string_append (str, stack->symbol ? stack->symbol : "??");
			g_string_append (str, " [");
			
			if (stack->type == VG_STACK_SOURCE) {
				if (stack->info.src.filename) {
					g_string_append (str, stack->info.src.filename);
					if (stack->info.src.lineno) {
						g_string_append_printf (str, ":%u", stack->info.src.lineno);
						load = TRUE;
					}
				}
			} else {
				g_string_append (str, "in ");
				g_string_append (str, stack->info.object);
			}
			
			g_string_append_c (str, ']');
			
			gtk_tree_store_set (model, &iter,
					    COL_STRING_DISPLAY, str->str,
					    COL_POINTER_ERROR, err,
					    COL_POINTER_SUMMARY, summary,
					    COL_POINTER_STACK, stack,
					    COL_LOAD_SRC_PREVIEW, load,
					    COL_IS_SRC_PREVIEW, FALSE,
					    -1);
			
			g_string_free (str, TRUE);
			
			if (load)
				valgrind_init_src_preview (view, model, &iter, stack);
			
			stack = stack->next;
		}
		
		if ((summary = summary->next) != NULL) {
			gtk_tree_store_append (model, &iter, &root);
			gtk_tree_store_set (model, &iter,
					    COL_STRING_DISPLAY, summary->report,
					    COL_POINTER_ERROR, err,
					    COL_POINTER_SUMMARY, summary,
					    COL_POINTER_STACK, NULL,
					    COL_LOAD_SRC_PREVIEW, FALSE,
					    COL_IS_SRC_PREVIEW, FALSE,
					    -1);
		}
	} while (summary != NULL);
}

static gboolean
error_matches_search (VgError *err, int search_id, regex_t *regex)
{
	VgErrorSummary *summary;
	VgErrorStack *stack;
	
	if (search_id == SEARCH_ID_NONE)
		return TRUE;
	
	if (regex == NULL)
		return FALSE;
	
	summary = err->summary;
	if (search_id == SEARCH_ID_ERROR) {
		return regexec (regex, summary->report, 0, NULL, 0) == 0;
	} else {
		do {
			stack = summary->frames;
			while (stack != NULL) {
				const char *str;
				
				switch (search_id) {
				case SEARCH_ID_FUNCTION:
					str = stack->symbol;
					break;
				case SEARCH_ID_OBJECT:
					str = stack->type == VG_STACK_OBJECT ? stack->info.object : NULL;
					break;
				case SEARCH_ID_SOURCE:
					str = stack->type == VG_STACK_SOURCE ? stack->info.src.filename : NULL;
					break;
				default:
					g_assert_not_reached ();
				}
				
				if (str && regexec (regex, str, 0, NULL, 0) == 0)
					return TRUE;
				
				stack = stack->next;
			}
			
			summary = summary->next;
		} while (summary != NULL);
	}
	
	return FALSE;
}

static void
view_rebuild (VgDefaultView *view)
{
	GtkTreeStore *model;
	int i;
	
	model = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (view->table)));
	
	gtk_tree_store_clear (model);
	
	for (i = 0; i < view->errors->len; i++) {
		VgError *err = view->errors->pdata[i];
		
		if (error_matches_search (err, view->search_id, &view->search_regex))
			view_show_error (view, model, err);
	}
}

static void
recv_error_cb (VgErrorParser *parser, VgError *err, gpointer user_data)
{
	VgDefaultView *view = user_data;
	GtkTreeStore *model;
	int i;
	
	for (i = 0; i < view->suppressions->len; i++) {
		VgRulePattern *pat = view->suppressions->pdata[i];
		if (vg_rule_pattern_matches (pat, err)) {
			/* suppress this error */
			vg_error_free (err);
			return;
		}
	}
	
	g_ptr_array_add (view->errors, err);
	
	model = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (view->table)));
	
	if (error_matches_search (err, view->search_id, &view->search_regex))
		view_show_error (view, model, err);
}

static void
valgrind_view_connect (VgToolView *tool, int sockfd)
{
	VgDefaultView *view = VG_DEFAULT_VIEW (tool);
	
	if (view->parser != NULL)
		valgrind_view_disconnect (tool);
	
	view->parser = vg_error_parser_new (sockfd, recv_error_cb, view);
}

static int
valgrind_view_step (VgToolView *tool)
{
	VgDefaultView *view = VG_DEFAULT_VIEW (tool);
	g_return_val_if_fail (view->parser != NULL, -1);

	return vg_error_parser_step (view->parser);
}

static void
valgrind_view_disconnect (VgToolView *tool)
{
	VgDefaultView *view = VG_DEFAULT_VIEW (tool);
	int i;
	
	if (view->parser) {
		vg_error_parser_flush (view->parser);
		vg_error_parser_free (view->parser);
		view->parser = NULL;
	}
	
	/* clear out suppressions added last session - a new `run'
	 * will force valgrind to re-read it's own suppressions file
	 * and so these will be unneeded */
	for (i = 0; i < view->suppressions->len; i++)
		vg_rule_pattern_free (view->suppressions->pdata[i]);
	g_ptr_array_set_size (view->suppressions, 0);
}

/*------------------------------------------------------------------------------
 * perform a GnomeVFS save of the file. 
 * NOTE: the chosen file will be overwritten if it already exists.
 */
static int
valgrind_view_save_log (VgToolView *tool, gchar* uri)
{
	VgDefaultView *view = VG_DEFAULT_VIEW (tool);
	VgError *err;
	GString *str;
	
	int i;
	GnomeVFSHandle* handle;
	
	if (uri == NULL) 
		return -1;
		
	/* Create file */
	if (gnome_vfs_create (&handle, uri, GNOME_VFS_OPEN_WRITE, FALSE, 0664) != GNOME_VFS_OK)	{
		return -1;
	}
		
	str = g_string_new ("");
	
	for (i = 0; i < view->errors->len; i++) {
		GnomeVFSFileSize written;
		err = view->errors->pdata[i];
		vg_error_to_string (err, str);

		if (gnome_vfs_write (handle, str->str, str->len, &written) != GNOME_VFS_OK) {
			g_string_free (str, TRUE);
			return -1;
		}
		g_string_truncate (str, 0);
	}
	
	g_string_free (str, TRUE);

	gnome_vfs_close (handle);	

	return 0;
}

/*-----------------------------------------------------------------------------
 * FIXME:
 * We can only load local files. Support to VFS would require to change all the
 * I/O [i.e. fopen/fwrite/etc...] calls for this plugin. I hope day somebody
 * will write a gnomelib wrapper that supports old I/O methos.
 * Perhaps a solution would be to save the file grabbed by the gnome_vfs to /tmp
 * and then open it with the I/O fopen/fwrite.
 */
 
static int
valgrind_view_load_log (VgToolView *tool, VgActions *actions, gchar* uri)
{
	int fd;
	gchar *filename;

	filename = gnome_vfs_get_local_path_from_uri (uri);
	
	if ((fd = open (filename, O_RDONLY)) != -1) {
		vg_tool_view_connect (tool, fd);
	}
	
	vg_actions_set_pid (actions, (pid_t)-1);
	
	/* with this call we'll set automatically the watch_id too. */
	vg_actions_set_giochan (actions, g_io_channel_unix_new (fd));

	g_free (filename);
	return 0;	
}


static void
valgrind_view_cut (VgToolView *tool)
{
	valgrind_view_copy (tool);
}

static void
valgrind_view_copy (VgToolView *tool)
{
	VgDefaultView *view = VG_DEFAULT_VIEW (tool);
	GtkTreeSelection *selection;
	GtkClipboard *clipboard;
	VgErrorSummary *summary;
	GtkTreeModel *model;
	GtkTreeIter iter;
	VgError *err;
	GString *str;
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view->table));
	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return;
	
	gtk_tree_model_get (model, &iter, COL_POINTER_ERROR, &err, COL_POINTER_SUMMARY, &summary, -1);
	
	str = g_string_new ("");
	vg_error_to_string (err, str);
	
	clipboard = gtk_widget_get_clipboard (GTK_WIDGET (view), GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_set_text (clipboard, str->str, str->len);
	g_string_free (str, TRUE);
}

static void
valgrind_view_paste (VgToolView *tool)
{
	;
}

static void
rules_response_cb (GtkDialog *dialog, int response, gpointer user_data)
{
	VgDefaultView *view = user_data;
	
	if (response == GTK_RESPONSE_OK)
		vg_rule_list_save (VG_RULE_LIST (view->rule_list));
	
	gtk_widget_hide (GTK_WIDGET (dialog));
}

static gboolean
rules_delete_event_cb (GtkWidget *widget, gpointer user_data)
{
	gtk_widget_hide (widget);
	
	return TRUE;
}

static void
valgrind_view_show_rules (VgToolView *tool)
{
	VgDefaultView *view = VG_DEFAULT_VIEW (tool);
	GtkWidget *dialog;
	GtkWidget *parent;
	
	if (tool->rules == NULL) {
		parent = gtk_widget_get_toplevel (GTK_WIDGET (tool));
		parent = GTK_WIDGET_TOPLEVEL (parent) ? parent : NULL;
		
		/* FIXME: we should really get this title from somewhere else? */
		dialog = gtk_dialog_new_with_buttons (_("Valgrind Suppression Rules"),
						      GTK_WINDOW (parent),
						      GTK_DIALOG_DESTROY_WITH_PARENT,
						      GTK_STOCK_CANCEL,
						      GTK_RESPONSE_CANCEL,
						      GTK_STOCK_OK,
						      GTK_RESPONSE_OK,
						      NULL);
		
		gtk_window_set_type_hint (GTK_WINDOW (dialog), GDK_WINDOW_TYPE_HINT_NORMAL);
		gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 3);
		gtk_window_set_default_size (GTK_WINDOW (dialog), 450, 400);
		
		gtk_container_set_border_width (GTK_CONTAINER (view->rule_list), 6);
		gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), view->rule_list, TRUE, TRUE, 0);
		
		g_signal_connect (dialog, "response", G_CALLBACK (rules_response_cb), view);
		g_signal_connect (dialog, "delete-event", G_CALLBACK (rules_delete_event_cb), view);
		
		tool->rules = dialog;
	}
	
	VG_TOOL_VIEW_CLASS (parent_class)->show_rules (tool);
}


static void
rules_filename_changed (GConfClient *client, guint cnxn_id,
			GConfEntry *entry, gpointer user_data)
{
	VgDefaultView *view = user_data;
	char *filename;
	
	filename = gconf_client_get_string (client, SUPPRESSIONS_KEY, NULL);
	vg_rule_list_set_filename (VG_RULE_LIST (view->rule_list), filename);
	g_free (filename);
}

static void
num_lines_changed (GConfClient *client, guint cnxn_id,
		   GConfEntry *entry, gpointer user_data)
{
	VgDefaultView *view = user_data;
	
	view->srclines = gconf_client_get_int (client, NUM_LINES_KEY, NULL);
}


GtkWidget *
vg_default_view_new (AnjutaValgrindPlugin *valgrind_plugin)
{
	VgDefaultView *view;
	
	view = g_object_new (VG_TYPE_DEFAULT_VIEW, NULL);
	
	view->srclines = gconf_client_get_int (view->gconf, NUM_LINES_KEY, NULL);
	
	/* listen for changes in the number of lines to show above/below target src line */
	gconf_client_add_dir (view->gconf, NUM_LINES_KEY, GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);
	
	view->lines_id = gconf_client_notify_add (view->gconf, NUM_LINES_KEY,
						  num_lines_changed, view, NULL, NULL);
	
	/* listen for suppression-file changes */
	gconf_client_add_dir (view->gconf, SUPPRESSIONS_KEY, GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);
	
	view->rules_id = gconf_client_notify_add (view->gconf, SUPPRESSIONS_KEY,
						  rules_filename_changed, view, NULL, NULL);
	
	/* set up the ValgrindPlugin reference. It'll be setted to NULL on this->object
	 * destroying */
	view->valgrind_plugin = valgrind_plugin;
	
	return GTK_WIDGET (view);
}



/* signal callbacks */

static void
rule_added (VgRuleList *list, VgRule *rule, gpointer user_data)
{
	VgDefaultView *view = user_data;
	VgRulePattern *pat;
	int i;
	
	if (!(pat = vg_rule_pattern_new (rule)))
		return;
	
	g_ptr_array_add (view->suppressions, pat);
	
	for (i = view->errors->len - 1; i >= 0; ) {
		VgError *err = view->errors->pdata[i];
		
		if (vg_rule_pattern_matches (pat, err)) {
			g_ptr_array_remove_index (view->errors, i);
			vg_error_free (err);
			
			if (i == view->errors->len)
				i--;
		} else {
			i--;
		}
	}
	
	view_rebuild (view);
}

static char *
read_src_lines (const char *filename, size_t first, size_t last, size_t target)
{
	unsigned char linebuf[4097];
	size_t len, buflen = 0;
	size_t lineno = 1;
	gboolean midline;
	char *buf = NULL;
	FILE *fp;
	
	d(printf ("read_src_lines (\"%s\", %u, %u)\n", filename, first, last));
	
	if ((fp = fopen (filename, "rt")) == NULL)
		return NULL;
	
	midline = FALSE;
	while (lineno <= last && fgets ((char *)linebuf, sizeof (linebuf), fp) != NULL) {
		len = strlen ((char *)linebuf);
		
		if (lineno >= first) {
			if (buf != NULL) {
				buf = g_realloc (buf, buflen + len + 2);
			} else {
				buf = g_malloc (len + 2);
			}
			
			d(printf ("read_src_lines(): added '%.*s'\n", len - 1, linebuf));
			
			if (!midline) {
				if (lineno == target) {
					buf[buflen++] = '=';
					buf[buflen++] = '>';
				} else {
					buf[buflen++] = ' ';
					buf[buflen++] = ' ';
				}
			} 
			
			memcpy (buf + buflen, linebuf, len);
			buflen += len;
		}
		
		if (linebuf[len - 1] == '\n') {
			midline = FALSE;
			lineno++;
		} else {
			midline = TRUE;
		}
	}
	
	fclose (fp);
	
	if (buf != NULL) {
		buf[buflen - 1] = '\0';
		d(printf ("read_src_lines(): returning:\n%s\n", buf));
	} else {
		d(printf ("read_src_lines(): returning: (null)\n", buf));
	}
	
	return buf;
}

static int
str_ends_with (const char *str, const char *match)
{
	return !(strcmp (str + strlen (str) - strlen (match), match));
}

static char *
resolve_full_path (VgToolView *tool, VgErrorStack *stack)
{
	const char *filename;
	SymTabSymbol *sym;
	char *path;
	
	if (!(filename = stack->info.src.filename))
		return NULL;
	
	if (*filename == '/')
		return g_strdup (filename);
	
	if (tool->symtab && (sym = symtab_resolve_addr (tool->symtab, (void *) stack->addr, FALSE))) {
		if (sym->filename != NULL) {
			if (*sym->filename == '/' && str_ends_with (sym->filename, filename)) {
				path = g_strdup (sym->filename);
				symtab_symbol_free (sym);
				return path;
			} else {
				w(g_warning ("symtab_resolve_addr() found the wrong symbol for 0x%.8x", stack->addr));
			}
		}
		
		symtab_symbol_free (sym);
	}
	
	if (tool->srcdir) {
		unsigned int buflen;
		const char *dir;
		struct stat st;
		int flen, dlen;
		char *p;
		int i;
		
		buflen = 1024;
		path = g_malloc (1024);
		
		flen = strlen (filename);
		
		for (i = 0; tool->srcdir[i] != NULL; i++) {
			dir = tool->srcdir[i];
			dlen = strlen (dir);
			
			if (dlen + flen + 2 > buflen) {
				buflen = dlen + flen + 2;
				buflen = ((buflen >> 5) << 5) + 64;
				path = g_realloc (path, buflen);
			}
			
			p = g_stpcpy (path, dir);
			*p++ = '/';
			strcpy (p, filename);
			
			if (stat (path, &st) != -1)
				break;
		}
		
		if (tool->srcdir[i] != NULL && S_ISREG (st.st_mode))
			return path;
		
		g_free (path);
	}
	
	return NULL;
}

static char *
load_src_buf (VgToolView *tool, VgErrorStack *stack, int srclines)
{
	char *path, *srcbuf;
	size_t lineno;
	size_t first;
	
	lineno = stack->info.src.lineno;
	first = lineno > (srclines + 1) ? lineno - srclines : 1;
	
	if (!(path = resolve_full_path (tool, stack)))
		return NULL;
	
	srcbuf = read_src_lines (path, first, lineno + srclines, lineno);
	g_free (path);
	
	return srcbuf;
}

static void
tree_row_expanded (GtkTreeView *treeview, GtkTreeIter *parent, GtkTreePath *path, gpointer user_data)
{
	VgDefaultView *view = user_data;
	VgToolView *tool = user_data;
	VgErrorStack *stack = NULL;
	GtkTreeStore *model;
	GtkTreeIter iter;
	gboolean load;
	char *srcbuf;
	
	model = GTK_TREE_STORE (gtk_tree_view_get_model (treeview));
	
	gtk_tree_model_get (GTK_TREE_MODEL (model), parent, COL_LOAD_SRC_PREVIEW, &load, -1);
	if (!load)
		return;
	
	/* unset the LOAD_SRC_PREVIEW flag - pass or fail, we won't be trying this again */
	gtk_tree_store_set (model, parent, COL_LOAD_SRC_PREVIEW, FALSE, -1);
	
	/* get the first child (which will be a dummy if we haven't loaded the src-preview yet) */
	gtk_tree_model_iter_children (GTK_TREE_MODEL (model), &iter, parent);
	
	gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, COL_POINTER_STACK, &stack, -1);
	
	if (!(srcbuf = load_src_buf (tool, stack, view->srclines))) {
		w(g_warning ("couldn't load src preview"));
		gtk_tree_store_remove (model, &iter);
		return;
	}
	
	gtk_tree_store_set (model, &iter,
			    COL_STRING_DISPLAY, srcbuf,
			    COL_POINTER_ERROR, stack->summary->parent,
			    COL_POINTER_SUMMARY, stack->summary,
			    COL_POINTER_STACK, stack,
			    COL_LOAD_SRC_PREVIEW, FALSE,
			    COL_IS_SRC_PREVIEW, TRUE,
			    -1);
	
	g_free (srcbuf);
}


static void
cut_cb (GtkWidget *widget, gpointer user_data)
{
	VgToolView *tool = user_data;
	
	valgrind_view_cut (tool);
}

static void
copy_cb (GtkWidget *widget, gpointer user_data)
{
	VgToolView *tool = user_data;
	
	valgrind_view_copy (tool);
}

static void
suppress_cb (GtkWidget *widget, gpointer user_data)
{
	VgDefaultView *view = user_data;
	GtkTreeSelection *selection;
	VgErrorSummary *summary;
	GtkTreeModel *model;
	GtkWidget *parent;
	GtkTreeIter iter;
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view->table));
	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return;
	
	gtk_tree_model_get (model, &iter, COL_POINTER_SUMMARY, &summary, -1);
	if (summary == NULL)
		return;
	
	parent = gtk_widget_get_toplevel (GTK_WIDGET (view));
	parent = GTK_WIDGET_TOPLEVEL (parent) ? parent : NULL;
	
	/* FIXME: we should really get this title from somewhere else? */
	vg_rule_list_add_rule (VG_RULE_LIST (view->rule_list), _("Valgrind Suppression"),
			       GTK_WINDOW (parent), summary);
}

static void
custom_editor_cb (GtkWidget *widget, gpointer user_data)
{
	VgDefaultView *view = user_data;
	IAnjutaFileLoader *loader;
	GtkTreeSelection *selection;
	VgErrorStack *stack = NULL;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *path;
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view->table));
	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return;
	
	gtk_tree_model_get (model, &iter, COL_POINTER_STACK, &stack, -1);
	if (stack == NULL)
		return;
	
	if (!(path = resolve_full_path (VG_TOOL_VIEW (view), stack)))
		return;
	
	DEBUG_PRINT ("got this path for file opening: %s and line %d", path, stack->info.src.lineno );
	
	/* Goto file line */
	loader = anjuta_shell_get_interface (ANJUTA_PLUGIN (view->valgrind_plugin)->shell,
									 IAnjutaFileLoader, NULL);
	if (loader)
	{
		gchar *uri;
		uri = g_strdup_printf ("file:///%s#%d", path, stack->info.src.lineno);
		
		ianjuta_file_loader_load (loader, uri, FALSE, NULL);
		g_free (uri);
	}	
	
	g_free (path);
}


enum {
	SELECTED_MASK    = (1 << 0),
	STACK_MASK       = (1 << 1),
};

static struct _MenuItem popup_menu_items[] = {
	{ N_("Cu_t"),     GTK_STOCK_CUT,     TRUE,  FALSE, FALSE, FALSE, G_CALLBACK (cut_cb),      SELECTED_MASK },
	{ N_("_Copy"),    GTK_STOCK_COPY,    TRUE,  FALSE, FALSE, FALSE, G_CALLBACK (copy_cb),     SELECTED_MASK },
	{ N_("_Paste"),   GTK_STOCK_PASTE,   TRUE,  FALSE, FALSE, FALSE, NULL,                     0             },
	MENU_ITEM_SEPARATOR,
	{ N_("Suppress"), NULL,              FALSE, FALSE, FALSE, FALSE, G_CALLBACK (suppress_cb), SELECTED_MASK },
	MENU_ITEM_SEPARATOR,
	{ N_("Edit in Custom Editor"), NULL,     FALSE, FALSE, FALSE, FALSE, G_CALLBACK (custom_editor_cb), STACK_MASK },
	MENU_ITEM_TERMINATOR
};

static gboolean
tree_button_press (GtkWidget *treeview, GdkEventButton *event, gpointer user_data)
{
	VgDefaultView *view = user_data;
	GtkTreeSelection *selection;
	VgErrorStack *stack = NULL;
	GtkTreeModel *model;
	GtkTreeIter iter;
	guint32 mask = 0;
	GtkWidget *menu;
	
	if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
		/* right-click */
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
		if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
			gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, COL_POINTER_STACK, &stack, -1);
			if (stack == NULL)
				mask |= STACK_MASK;
		} else {
			mask |= (SELECTED_MASK | STACK_MASK);
		}

		menu = gtk_menu_new ();
		menu_utils_construct_menu (menu, popup_menu_items, mask, view);
		gtk_widget_show (menu);
		
		gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, event->button, event->time);
	} else if (event->type == GDK_2BUTTON_PRESS && event->button == 1) {
		/* double-click */
		custom_editor_cb (treeview, user_data);
	}
	
	return FALSE;
}
