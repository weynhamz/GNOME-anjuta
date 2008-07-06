/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Jeffrey Stedfast <fejj@ximian.com>
 *
 *  Copyright 2003 Ximian, Inc. (www.ximian.com)
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <glib/gi18n.h>
#include <gconf/gconf-client.h>
#include <libanjuta/anjuta-utils.h>

#include "vgrule-list.h"
#include "vgmarshal.h"


typedef struct _RuleNode {
	struct _RuleNode *next;
	struct _RuleNode *prev;
	VgRule *rule;
} RuleNode;

enum {
	COL_STRING_NAME,
	COL_POINTER_RULE,
	COL_POINTER_RULE_NODE,
	COL_LAST
};

static GType col_types[] = {
	G_TYPE_STRING,
	G_TYPE_POINTER,
	G_TYPE_POINTER,
};

enum {
	RULE_ADDED,
	LAST_SIGNAL
};

static unsigned int signals[LAST_SIGNAL] = { 0 };

#define SUPPRESSIONS_KEY "/apps/anjuta/valgrind/general/suppressions"


static void vg_rule_list_class_init (VgRuleListClass *klass);
static void vg_rule_list_init (VgRuleList *list);
static void vg_rule_list_destroy (GtkObject *obj);
static void vg_rule_list_finalize (GObject *obj);


static GtkVBoxClass *parent_class = NULL;


GType
vg_rule_list_get_type (void)
{
	static GType type = 0;
	
	if (!type) {
		static const GTypeInfo info = {
			sizeof (VgRuleListClass),
			NULL, /* base_class_init */
			NULL, /* base_class_finalize */
			(GClassInitFunc) vg_rule_list_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (VgRuleList),
			0,    /* n_preallocs */
			(GInstanceInitFunc) vg_rule_list_init,
		};
		
		type = g_type_register_static (GTK_TYPE_VBOX, "VgRuleList", &info, 0);
	}
	
	return type;
}

static void
vg_rule_list_class_init (VgRuleListClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkObjectClass *gtk_object_class = GTK_OBJECT_CLASS (klass);
	
	parent_class = g_type_class_ref (GTK_TYPE_VBOX);
	
	object_class->finalize = vg_rule_list_finalize;
	gtk_object_class->destroy = vg_rule_list_destroy;
	
	signals[RULE_ADDED]
		= g_signal_new ("rule-added",
				G_OBJECT_CLASS_TYPE (object_class),
				G_SIGNAL_RUN_FIRST,
				G_STRUCT_OFFSET (VgRuleListClass, rule_added),
				NULL, NULL,
				vg_marshal_NONE__POINTER,
				G_TYPE_NONE, 1,
				G_TYPE_POINTER);
}


static GtkWidget *
rule_editor_dialog_new (GtkWindow *parent, VgRule *rule)
{
	GtkWidget *dialog, *editor;
	
	/* FIXME: we should really get this title from somewhere else? */
	dialog = gtk_dialog_new_with_buttons (_("Valgrind Suppression"), parent,
					      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	
	if (rule != NULL)
		editor = vg_rule_editor_new_from_rule (rule);
	else
		editor = vg_rule_editor_new ();
	
	gtk_container_set_border_width (GTK_CONTAINER (editor), 6);
	gtk_widget_show (editor);
	
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 3);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), editor, TRUE, TRUE, 0);
	
	g_object_set_data (G_OBJECT (dialog), "editor", editor);
	
	return dialog;
}

static void
add_response_cb (GtkDialog *dialog, int response, gpointer user_data)
{
	VgRuleList *list = user_data;
	GtkWidget *editor;
	GtkTreeIter iter;
	RuleNode *node;
	VgRule *rule;
	
	if (response == GTK_RESPONSE_OK) {
		const char *name;
		GtkWidget *msg;
		
		editor = g_object_get_data (G_OBJECT (dialog), "editor");
		
		name = vg_rule_editor_get_name (VG_RULE_EDITOR (editor));
		if (!name || *name == '\0') {
			msg = gtk_message_dialog_new (GTK_WINDOW (dialog), GTK_DIALOG_MODAL |
						      GTK_DIALOG_DESTROY_WITH_PARENT,
						      GTK_MESSAGE_WARNING,
						      GTK_BUTTONS_CLOSE,
						      _("You have forgotten to name your suppression rule."));
			
			gtk_dialog_run (GTK_DIALOG (msg));
			gtk_widget_destroy (msg);
			return;
		}
		
		list->changed = TRUE;
		
		rule = vg_rule_editor_get_rule (VG_RULE_EDITOR (editor));
		
		node = g_new (RuleNode, 1);
		node->rule = rule;
		
		list_append_node (&list->rules, (ListNode *) node);
		
		vg_rule_list_save (list);
		
		gtk_list_store_append (GTK_LIST_STORE (list->model), &iter);
		
		gtk_list_store_set (GTK_LIST_STORE (list->model), &iter,
				    COL_STRING_NAME, rule->name,
				    COL_POINTER_RULE, rule,
				    COL_POINTER_RULE_NODE, node, -1);
		
		g_signal_emit (list, signals[RULE_ADDED], 0, rule);
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
add_cb (GtkWidget *button, gpointer user_data)
{
	VgRuleList *list = user_data;
	GtkWidget *parent;
	GtkWidget *dialog;
	
	parent = gtk_widget_get_toplevel (GTK_WIDGET (list));
	parent = GTK_WIDGET_TOPLEVEL (parent) ? parent : NULL;
	
	dialog = rule_editor_dialog_new (GTK_WINDOW (parent), NULL);
	g_signal_connect (dialog, "response", G_CALLBACK (add_response_cb), list);
	
	gtk_widget_show (dialog);
}

static void
edit_response_cb (GtkDialog *dialog, int response, gpointer user_data)
{
	VgRuleList *list = user_data;
	GtkWidget *editor;
	GtkTreePath *path;
	GtkTreeIter iter;
	RuleNode *node;
	VgRule *rule;
	
	if (response == GTK_RESPONSE_OK) {
		const char *name;
		GtkWidget *msg;
		
		editor = g_object_get_data (G_OBJECT (dialog), "editor");
		
		name = vg_rule_editor_get_name (VG_RULE_EDITOR (editor));
		if (!name || *name == '\0') {
			msg = gtk_message_dialog_new (GTK_WINDOW (dialog), GTK_DIALOG_MODAL |
						      GTK_DIALOG_DESTROY_WITH_PARENT,
						      GTK_MESSAGE_WARNING,
						      GTK_BUTTONS_CLOSE,
						      _("You have forgotten to name your suppression rule."));
			
			gtk_dialog_run (GTK_DIALOG (msg));
			gtk_widget_destroy (msg);
			return;
		}
		
		list->changed = TRUE;
		
		rule = vg_rule_editor_get_rule (VG_RULE_EDITOR (editor));
		
		path = g_object_get_data (G_OBJECT (dialog), "path");
		if (gtk_tree_model_get_iter (list->model, &iter, path)) {
			/* replace the old rule node... */
			gtk_tree_model_get (list->model, &iter, COL_POINTER_RULE_NODE, &node, -1);
			vg_rule_free (node->rule);
			node->rule = rule;
		} else {
			/* create a new rule node... */
			node = g_new (RuleNode, 1);
			node->rule = rule;
			
			list_append_node (&list->rules, (ListNode *) node);
			
			gtk_list_store_append (GTK_LIST_STORE (list->model), &iter);
		}
		
		gtk_list_store_set (GTK_LIST_STORE (list->model), &iter,
				    COL_STRING_NAME, rule->name,
				    COL_POINTER_RULE, rule,
				    COL_POINTER_RULE_NODE, node, -1);
		
		/* FIXME: only emit if we've changed something that matters? */
		g_signal_emit (list, signals[RULE_ADDED], 0, rule);
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
edit_cb (GtkWidget *button, gpointer user_data)
{
	VgRuleList *list = user_data;
	GtkTreeSelection *selection;
	GtkTreeModel *model = NULL;
	VgRule *rule = NULL;
	GtkWidget *parent;
	GtkWidget *dialog;
	GtkTreePath *path;
	GtkTreeIter iter;
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list->list));
	if (gtk_tree_selection_get_selected (selection, &model, &iter))
		gtk_tree_model_get (model, &iter, COL_POINTER_RULE, &rule, -1);
	
	if (rule == NULL)
		return;
	
	path = gtk_tree_model_get_path (model, &iter);
	
	parent = gtk_widget_get_toplevel (GTK_WIDGET (list));
	parent = GTK_WIDGET_TOPLEVEL (parent) ? parent : NULL;
	
	dialog = rule_editor_dialog_new (GTK_WINDOW (parent), rule);
	g_signal_connect (dialog, "response", G_CALLBACK (edit_response_cb), list);
	g_object_set_data_full (G_OBJECT (dialog), "path", path, (GDestroyNotify) gtk_tree_path_free);
	
	gtk_widget_show (dialog);
}

static void
remove_cb (GtkWidget *button, gpointer user_data)
{
	VgRuleList *list = user_data;
	GtkTreeSelection *selection;
	GtkTreeModel *model = NULL;
	RuleNode *node = NULL;
	VgRule *rule = NULL;
	GtkTreeIter iter;
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list->list));
	if (gtk_tree_selection_get_selected (selection, &model, &iter))
		gtk_tree_model_get (model, &iter, COL_POINTER_RULE, &rule, COL_POINTER_RULE_NODE, &node, -1);
	
	if (rule == NULL)
		return;
	
	list->changed = TRUE;
	
	gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
	list_node_unlink ((ListNode *) node);
	vg_rule_free (rule);
	g_free (node);
}

static void
row_activate_cb (GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
	/* double-clicked on a rule, pop the rule up in an editor */
	edit_cb (NULL, user_data);
}

static void
selection_change_cb (GtkTreeSelection *selection, gpointer user_data)
{
	VgRuleList *list = user_data;
	GtkTreeModel *model;
	GtkTreeIter iter;
	int state;
	
	state = gtk_tree_selection_get_selected (selection, &model, &iter);
	if (state == 0)
		gtk_widget_grab_focus (list->add);
	
	gtk_widget_set_sensitive (list->edit, state);
	gtk_widget_set_sensitive (list->remove, state);
}

static void
vg_rule_list_init (VgRuleList *list)
{
	GtkTreeSelection *selection;
	GtkCellRenderer *renderer;
	GtkWidget *hbox, *vbox;
	GtkWidget *scrolled;
	
	hbox = gtk_hbox_new (FALSE, 6);
	
	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled), GTK_SHADOW_IN);
	
	list->model = GTK_TREE_MODEL (gtk_list_store_newv (COL_LAST, col_types));
	list->list = gtk_tree_view_new_with_model (list->model);
	
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (list->list), -1, "",
						     renderer, "text", 0, NULL);
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list->list));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (list->list), FALSE);
	
	g_signal_connect (selection, "changed", G_CALLBACK (selection_change_cb), list);
	g_signal_connect (list->list, "row-activated", G_CALLBACK (row_activate_cb), list);
	
	gtk_widget_show (list->list);
	gtk_container_add (GTK_CONTAINER (scrolled), list->list);
	
	gtk_widget_show (scrolled);
	gtk_box_pack_start (GTK_BOX (hbox), scrolled, TRUE, TRUE, 0);
	
	vbox = gtk_vbox_new (FALSE, 3);
	
	list->add = gtk_button_new_from_stock (GTK_STOCK_ADD);
	g_signal_connect (list->add, "clicked", G_CALLBACK (add_cb), list);
	gtk_widget_show (list->add);
	gtk_box_pack_start (GTK_BOX (vbox), list->add, FALSE, FALSE, 0);
	
	list->edit = gtk_button_new_with_mnemonic (_("_Edit"));
	gtk_widget_set_sensitive (list->edit, FALSE);
	g_signal_connect (list->edit, "clicked", G_CALLBACK (edit_cb), list);
	gtk_widget_show (list->edit);
	gtk_box_pack_start (GTK_BOX (vbox), list->edit, FALSE, FALSE, 0);
	
	list->remove = gtk_button_new_from_stock (GTK_STOCK_REMOVE);
	gtk_widget_set_sensitive (list->remove, FALSE);
	g_signal_connect (list->remove, "clicked", G_CALLBACK (remove_cb), list);
	gtk_widget_show (list->remove);
	gtk_box_pack_start (GTK_BOX (vbox), list->remove, FALSE, FALSE, 0);
	
	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
	
	gtk_widget_show (hbox);
	gtk_container_add (GTK_CONTAINER (list), hbox);
	
	/* init our data */
	list_init (&list->rules);
	list->changed = FALSE;
	list->filename = NULL;
	list->parser = NULL;
	list->gio = NULL;
	list->show_id = 0;
	list->load_id = 0;
}

static void
vg_rule_list_finalize (GObject *obj)
{
	VgRuleList *list = (VgRuleList *) obj;
	RuleNode *n, *nn;
	
	g_free (list->filename);
	
	vg_rule_parser_free (list->parser);
	
	n = (RuleNode *) list->rules.head;
	while (n->next != NULL) {
		nn = n->next;
		vg_rule_free (n->rule);
		g_free (n);
		n = nn;
	}
	
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
vg_rule_list_destroy (GtkObject *obj)
{
	VgRuleList *list = (VgRuleList *) obj;
	
	if (list->gio != NULL) {
		g_io_channel_close (list->gio);
		g_io_channel_unref (list->gio);
		list->load_id = 0;
		list->gio = NULL;
	}
	
	GTK_OBJECT_CLASS (parent_class)->destroy (obj);
}


static void
load_rule_cb (VgRuleParser *parser, VgRule *rule, gpointer user_data)
{
	VgRuleList *list = user_data;
	GtkTreeIter iter;
	RuleNode *node;
	
	node = g_new (RuleNode, 1);
	node->rule = rule;
	
	list_append_node (&list->rules, (ListNode *) node);
	
	gtk_list_store_append (GTK_LIST_STORE (list->model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (list->model), &iter,
			    COL_STRING_NAME, rule->name,
			    COL_POINTER_RULE, rule,
			    COL_POINTER_RULE_NODE, node, -1);
}

static gboolean
load_rules_step_cb (GIOChannel *gio, GIOCondition condition, gpointer user_data)
{
	VgRuleList *list = user_data;
	
	if ((condition & G_IO_IN) && vg_rule_parser_step (list->parser) <= 0)
		goto disconnect;
	
	if (condition & G_IO_HUP)
		goto disconnect;
	
	return TRUE;
	
 disconnect:
	
	vg_rule_parser_free (list->parser);
	list->parser = NULL;
	
	g_io_channel_close (list->gio);
	g_io_channel_unref (list->gio);
	list->load_id = 0;
	list->gio = NULL;
	
	return FALSE;
}

static void
load_rules (GtkWidget *widget, gpointer user_data)
{
	VgRuleList *list = user_data;
	int fd;
	
	list->changed = FALSE;
	
	if (list->show_id != 0) {
		g_signal_handler_disconnect (list, list->show_id);
		list->show_id = 0;
	}
	
	if (list->filename == NULL)
		return;
	
	if ((fd = open (list->filename, O_RDONLY)) == -1)
		return;
	
	list->parser = vg_rule_parser_new (fd, load_rule_cb, list);
	list->gio = g_io_channel_unix_new (fd);
	list->load_id = g_io_add_watch (list->gio, G_IO_IN | G_IO_HUP, load_rules_step_cb, list);
}


GtkWidget *
vg_rule_list_new (const char *filename)
{
	VgRuleList *list;
	
	list = g_object_new (VG_TYPE_RULE_LIST, NULL);
	list->filename = g_strdup (filename);
	
	/* suppressions file may be large, so don't actually load it
           until the user shows it for the first time */
	list->show_id = g_signal_connect (list, "map", G_CALLBACK (load_rules), list);
	
	return GTK_WIDGET (list);
}


void
vg_rule_list_set_filename (VgRuleList *list, const char *filename)
{
	RuleNode *n, *nn;
	
	g_free (list->filename);
	list->filename = g_strdup (filename);
	
	if (list->show_id != 0) {
		/* good, the user hasn't shown the dialog yet.
		 * this means that we don't have to do anything special!
		 */
	} else {
		if (list->load_id != 0) {
			/* this means the file is currently being loaded. ugh */
			vg_rule_parser_free (list->parser);
			g_io_channel_close (list->gio);
			g_io_channel_unref (list->gio);
			list->load_id = 0;
			list->gio = NULL;
		}
		
		n = (RuleNode *) list->rules.head;
		while (n->next != NULL) {
			nn = n->next;
			vg_rule_free (n->rule);
			g_free (n);
			n = nn;
		}
		
		gtk_list_store_clear (GTK_LIST_STORE (list->model));
		
		if (!GTK_WIDGET_MAPPED (list))
			list->show_id = g_signal_connect (list, "map", G_CALLBACK (load_rules), list);
		else
			load_rules (GTK_WIDGET (list), list);
	}
}


int
vg_rule_list_save (VgRuleList *list)
{
	GtkWidget *parent, *msg;
	char *filename = NULL;
	const char *basename;
	RuleNode *node;
	int fd = -1;
	
	/* only save if we need to... */
	if (!list->changed)
		return 0;
	
	if (list->filename == NULL)
		goto exception;
	
	if (!(basename = strrchr (list->filename, '/')))
		basename = list->filename;
	else
		basename++;
	
	filename = g_strdup_printf ("%.*s.#%s", (basename - list->filename),
				    list->filename, basename);
	
	if ((fd = open (filename, O_WRONLY | O_CREAT | O_TRUNC | O_EXCL, 0666)) == -1)
		goto exception;
	
	if (vg_suppressions_file_write_header (fd, "This Valgrind suppresion file was generated by Alleyoop") == -1)
		goto exception;
	
	node = (RuleNode *) list->rules.head;
	while (node->next != NULL) {
		if (vg_suppressions_file_append_rule (fd, node->rule) == -1)
			goto exception;
		node = node->next;
	}
	
	close (fd);
	fd = -1;
	
	if (rename (filename, list->filename) == -1)
		goto exception;
	
	g_free (filename);
	
	return 0;
	
 exception:
	
	parent = gtk_widget_get_toplevel (GTK_WIDGET (list));
	parent = GTK_WIDGET_TOPLEVEL (parent) ? parent : NULL;
	
	msg = gtk_message_dialog_new (GTK_WINDOW (parent), GTK_DIALOG_MODAL,
				      GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
				      _("Cannot save suppression rules: %s"),
				      list->filename ? g_strerror (errno) :
				      _("You have not set a suppressions file in your settings."));
	
	g_signal_connect_swapped (msg, "response", G_CALLBACK (gtk_widget_destroy), msg);
	
	gtk_widget_show (msg);
	
	if (fd != -1)
		close (fd);
	
	if (filename) {
		unlink (filename);
		g_free (filename);
	}
	
	return -1;
}


void
vg_rule_list_add_rule (VgRuleList *list, const char *title, GtkWindow *parent, VgErrorSummary *summary)
{
	GtkWidget *dialog, *editor;
	GConfClient *gconf;
	
	dialog = gtk_dialog_new_with_buttons (title, parent, GTK_DIALOG_MODAL |
					      GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	
	if (summary != NULL)
		editor = vg_rule_editor_new_from_summary (summary);
	else
		editor = vg_rule_editor_new ();
	
	gtk_widget_show (editor);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), editor, TRUE, TRUE, 0);
	g_signal_connect (dialog, "response", G_CALLBACK (add_response_cb), list);
	g_object_set_data (G_OBJECT (dialog), "editor", editor);
	
	if (list->filename == NULL) {
		gconf = gconf_client_get_default ();
		
		// FIXME: hardcoded path
		list->filename = anjuta_util_get_user_config_file_path ("valgrind.supp", NULL);
		gconf_client_set_string (gconf, SUPPRESSIONS_KEY, list->filename, NULL);
		g_object_unref (gconf);
	}
	
	/* FIXME: what if the rules haven't finished loading before the user adds this rule? */
	
	gtk_widget_show (dialog);
}
