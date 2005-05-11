/*
    breakpoints.c
    Copyright (C) 2000  Naba Kumar <naba@gnome.org>

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

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <gnome.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include <libanjuta/resources.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-markable.h>
#include <libanjuta/interfaces/ianjuta-file.h>

#include "breakpoints.h"
#include "debugger.h"
#include "plugin.h"
#include "utilities.h"

#define BREAKPOINT_ITEM_TYPE breakpoint_item_get_type()

/* Markers definition */
#define BREAKPOINT_ENABLED   IANJUTA_MARKABLE_INTENSE
#define BREAKPOINT_DISABLED  IANJUTA_MARKABLE_ATTENTIVE
#define BREAKPOINT_NONE      -1

typedef enum _BreakpointType BreakpointType;
typedef struct _BreakpointItem BreakpointItem;
typedef struct _Properties Properties;

enum _BreakpointType
{
	breakpoint,
	watchpoint,
	catchpoint
};

struct _BreakpointItem
{
	BreakpointsDBase *bd;
	gboolean is_editing;
	gint id;
	gchar *disp;
	gchar *type;
	gboolean enable;
	gchar *address;
	gint pass;
	gint times;
	gchar *condition;
	gchar *file;
	guint line;
	gint handle;
	IAnjutaEditor *editor;
	gchar *function;
	gchar *info;
	time_t time;
};

struct _BreakpointsDBasePriv
{
	GdbPlugin *plugin;

	Debugger *debugger;
	GladeXML *gxml;
	gchar *cond_history, *loc_history, *pass_history;
	
	gboolean is_showing;
	gint win_pos_x, win_pos_y, win_width, win_height;

	/* Widgets */	
	GtkWidget *window;
	GtkWidget *treeview;
	GtkWidget *add_button;
	GtkWidget *remove_button;
	GtkWidget *jumpto_button;
	GtkWidget *properties_button;
	GtkWidget *removeall_button;
	GtkWidget *enableall_button;
	GtkWidget *disableall_button;
};

struct _Properties
{
	BreakpointsDBase *bd;
	GladeXML *gxml;
	BreakpointItem *bid;
	GtkWidget *dialog;
};

enum {
	ENABLED_COLUMN,
	NUMBER_COLUMN,
	FILENAME_COLUMN,
	LINENO_COLUMN,
	FUNCTION_COLUMN,
	TYPE_COLUMN,
	ADDRESS_COLUMN,
	PASS_COLUMN,
	TIMES_COLUMN,
	CONDITION_COLUMN,
	DISP_COLUMN,
	DATA_COLUMN,
	COLUMNS_NB
};

static char *column_names[COLUMNS_NB] = {
	N_("Enb"), N_("ID"), N_("File"), N_("Line"),
	N_("Function"), N_("Type"), N_("Address"), N_("Pass"),
	N_("Times"), N_("Condition"), N_("Disp")
};

static gboolean bk_item_add (BreakpointsDBase *bd, GladeXML *gxml);

static void breakpoints_dbase_update (Debugger *debugger,
									  const GDBMIValue *mi_values,
									  const GList * outputs, gpointer p);

/* TODO
static void breakpoint_item_save (BreakpointItem *bi, ProjectDBase *pdb,
								  const gint nBreak );
*/
static void breakpoints_dbase_delete_all_breakpoints (BreakpointsDBase *bd);
static void on_treeview_enabled_toggled (GtkCellRendererToggle *cell,
										 gchar *path_str, BreakpointsDBase *bd);
static void breakpoints_dbase_add_brkpnt (const GDBMIValue *brkpnt,
										  gpointer user_data);
// static gboolean breakpoints_dbase_set_from_item (BreakpointsDBase *bd,
//												 BreakpointItem *bi,
//												 gboolean add_to_treeview);

static IAnjutaDocumentManager *
get_document_manager (GdbPlugin *plugin)
{
	GObject *obj = anjuta_shell_get_object (ANJUTA_PLUGIN (plugin)->shell,
			"IAnjutaDocumentManager", NULL /* TODO */);
	return IANJUTA_DOCUMENT_MANAGER (obj);
}

static void
set_breakpoint_in_editor (BreakpointItem *item, gint mark,
						  gboolean bypass_handle)
{
	IAnjutaMarkable *ed;
	gint true_line = -1;
	
	g_return_if_fail (item != NULL);
	g_return_if_fail (item->editor != NULL);
	
	switch (mark)
	{
		case BREAKPOINT_NONE:
			if (item->editor && IANJUTA_IS_MARKABLE (item->editor))
			{
				ed = IANJUTA_MARKABLE (item->editor);
				
				if (!bypass_handle)
				{
					true_line =
						ianjuta_markable_location_from_handle (ed,
															   item->handle,
															   NULL);
				}
				if (true_line >= 0)
				{
					ianjuta_markable_unmark (ed, true_line,
											 BREAKPOINT_ENABLED,
											 NULL);
					ianjuta_markable_unmark (ed, true_line,
											 BREAKPOINT_DISABLED,
											 NULL);
				}
				else 
				{
					ianjuta_markable_unmark (ed, item->line,
											 BREAKPOINT_ENABLED,
											 NULL);
					ianjuta_markable_unmark (ed, item->line,
											 BREAKPOINT_DISABLED,
											 NULL);
				}
			}
			break;
		case BREAKPOINT_ENABLED:
			if (item->editor && IANJUTA_IS_MARKABLE (item->editor))
			{
				ed = IANJUTA_MARKABLE (item->editor);
				
				if (!bypass_handle)
				{
					true_line =
						ianjuta_markable_location_from_handle (ed,
															   item->handle,
															   NULL);
				}
				if (true_line >= 0)
				{
					ianjuta_markable_unmark (ed, true_line,
											 BREAKPOINT_DISABLED,
											 NULL);
					item->handle = ianjuta_markable_mark (ed, true_line,
														  BREAKPOINT_ENABLED,
														  NULL);
				}
				else 
				{
					ianjuta_markable_unmark (ed, item->line,
											 BREAKPOINT_DISABLED,
											 NULL);
					item->handle = ianjuta_markable_mark (ed, item->line,
														  BREAKPOINT_ENABLED,
														  NULL);
				}
			}
			break;
		case BREAKPOINT_DISABLED:
			if (item->editor && IANJUTA_IS_MARKABLE (item->editor))
			{
				ed = IANJUTA_MARKABLE (item->editor);
				
				if (!bypass_handle)
				{
					true_line =
						ianjuta_markable_location_from_handle (ed,
															   item->handle,
															   NULL);
				}
				if (true_line >= 0)
				{
					ianjuta_markable_unmark (ed, true_line,
											 BREAKPOINT_ENABLED,
											 NULL);
					item->handle = ianjuta_markable_mark (ed, true_line,
														  BREAKPOINT_DISABLED,
														  NULL);
				}
				else 
				{
					ianjuta_markable_unmark (ed, item->line,
											 BREAKPOINT_ENABLED,
											 NULL);
					item->handle = ianjuta_markable_mark (ed, item->line,
														  BREAKPOINT_DISABLED,
														  NULL);
				}
			}
			break;
		default:
			g_warning ("Should not be here");
	}
}

static gboolean
editor_is_debugger_file (BreakpointItem *bi, IAnjutaEditor *te)
{
	const gchar *fname, *filename;
	gchar *local_fname, *debugger_fname;
	gboolean ret = FALSE;
	
	filename = ianjuta_editor_get_filename (te, NULL);
	fname = ianjuta_file_get_uri (IANJUTA_FILE (te), NULL);
	if (fname == NULL)
		return FALSE;
	
	local_fname = gnome_vfs_get_local_path_from_uri (fname);
	if (!local_fname)
		return FALSE;
	
	debugger_fname = debugger_get_source_path (bi->bd->priv->debugger,
											   bi->file);
	if (debugger_fname)
	{
		if (strcmp (local_fname, debugger_fname) == 0)
			ret = TRUE;
	}
	else
	{
		if (strcmp (bi->file, filename) == 0)
			ret = TRUE;
	}
	g_free (local_fname);
	g_free (debugger_fname);
	return ret;
}
						 
static BreakpointItem *
breakpoint_item_new (BreakpointsDBase *bd)
{
	BreakpointItem *bi;
	bi = g_new0 (BreakpointItem, 1);
	bi->bd = bd;
	return bi;
}

static BreakpointItem *
breakpoint_item_copy (const BreakpointItem *src)
{
	BreakpointItem *bi;
	bi = breakpoint_item_new (src->bd);
	bi->is_editing = src->is_editing;
	bi->id = src->id;
	if (src->disp)
		bi->disp = g_strdup (src->disp);
	if (src->type)
		bi->type = g_strdup (src->type);
	
	bi->enable = src->enable;
	
	if (src->address)
		bi->address = g_strdup (src->address);
	
	bi->pass = src->pass;
	bi->times = src->times;
	if (src->condition)
		bi->condition = g_strdup (src->condition);
	
	if (src->file)
		bi->file = g_strdup (src->file);
	
	bi->line = src->line;
	bi->handle = src->handle;
	bi->editor = src->editor;
	if (src->function)
		bi->function = g_strdup (src->function);
	if (src->info)
		bi->info = src->info;
	bi->time = src->time;
	return bi;
}

static void
breakpoint_item_free (BreakpointItem *bi)
{
	g_return_if_fail (bi != NULL);

	g_free (bi->function);
	g_free (bi->file);
	g_free (bi->condition);
	g_free (bi->info);
	g_free (bi->disp);
	g_free (bi->address);

	g_free (bi);
}

static GType
breakpoint_item_get_type (void)
{
	static GType type;
	if (!type)
	{
		type = g_boxed_type_register_static ("BreakpointItem",
										 (GBoxedCopyFunc)breakpoint_item_copy,
										 (GBoxedFreeFunc)breakpoint_item_free);
	}
	return type;
}

static void
on_bk_remove_clicked (GtkWidget *button, BreakpointsDBase *bd)
{
	BreakpointItem *bi;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	gboolean valid;
	gchar buff[256];

	selection =
		gtk_tree_view_get_selection (GTK_TREE_VIEW (bd->priv->treeview));
	valid = gtk_tree_selection_get_selected (selection, &model, &iter);
	if (valid)
	{
		gtk_tree_model_get (model, &iter, DATA_COLUMN, &bi, -1);
		if (bi->editor)
			set_breakpoint_in_editor (bi, BREAKPOINT_NONE, FALSE);
		snprintf (buff, 256, "-break-delete %d", bi->id);
		gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);
		debugger_command (bd->priv->debugger, buff, FALSE, NULL, NULL);
		breakpoint_item_free (bi);
	}
}

static void
on_bk_jumpto_clicked (GtkWidget *button, gpointer   data)
{
	BreakpointsDBase *bd;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	gboolean valid;

	bd = (BreakpointsDBase *) data;

	selection =
		gtk_tree_view_get_selection (GTK_TREE_VIEW (bd->priv->treeview));
	valid = gtk_tree_selection_get_selected (selection, &model, &iter);
	if (valid)
	{
		BreakpointItem *bi;
		gtk_tree_model_get (model, &iter, DATA_COLUMN, &bi, -1);
		breakpoints_dbase_hide (bd);
		debugger_change_location (bd->priv->debugger, bi->file,
								  bi->line, bi->address);
		breakpoint_item_free (bi);
	}
}

/*****************************************************************************/

static void
bk_item_add_mesg_arrived (Debugger *debugger, const GDBMIValue *mi_results,
						  const GList * lines, gpointer data)
{
	const GDBMIValue *bkpt;
	BreakpointsDBase *bd;

	bd = (BreakpointsDBase*) data;
	
	if (mi_results == NULL)
		return;
	
	bkpt = gdbmi_value_hash_lookup (mi_results, "bkpt");
	g_return_if_fail (bkpt != NULL);
	
	breakpoints_dbase_add_brkpnt (bkpt, bd);
}

static gboolean
bk_item_add (BreakpointsDBase *bd, GladeXML *gxml)
{
	GtkWidget *dialog;
	GtkWidget *location_entry;
	GtkWidget *condition_entry;
	GtkWidget *pass_entry;
	gchar *buff, *tmp;

	dialog = glade_xml_get_widget (gxml, "breakpoints_properties_dialog");
	location_entry = glade_xml_get_widget (gxml, "breakpoint_location_entry");
	condition_entry = glade_xml_get_widget (gxml, "breakpoint_condition_entry");
	pass_entry = glade_xml_get_widget (gxml, "breakpoint_pass_entry");

	if (strlen (gtk_entry_get_text (GTK_ENTRY (location_entry))) > 0)
	{
		const gchar *loc_text;
		const gchar *cond_text;
		const gchar *pass_text;
	
		loc_text = gtk_entry_get_text (GTK_ENTRY (location_entry));
		cond_text = gtk_entry_get_text (GTK_ENTRY (condition_entry));
		pass_text = gtk_entry_get_text (GTK_ENTRY (pass_entry));
		
		buff = g_strdup ("-break-insert");
		if (atoi(pass_text) > 0)
		{
			tmp = buff;
			buff = g_strconcat (tmp, " -i ", pass_text, NULL);
			g_free (tmp);
		}
		
		if (strlen (cond_text) > 0)
		{
			tmp = buff;
			buff = g_strconcat (tmp, " -c (", cond_text, ")", NULL);
			g_free (tmp);
		}
		tmp = buff;
		buff = g_strconcat (tmp, " ", loc_text, NULL);
		g_free (tmp);
		
		debugger_command (bd->priv->debugger, buff, FALSE,
						  bk_item_add_mesg_arrived, bd);
		g_free (buff);
		return TRUE;
	}
	else
	{
		anjuta_util_dialog_error (GTK_WINDOW (dialog),
			  _("You must give a valid location to set the breakpoint."));
		return FALSE;
	}
}

static void
on_bk_properties_clicked (GtkWidget *button, gpointer data)
{
	GladeXML *gxml;
	BreakpointsDBase *bd;
	BreakpointItem *bid;
	GtkWidget *dialog;
	GtkWidget *location_label, *location_entry;
	GtkWidget *condition_entry;
	GtkWidget *pass_entry;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;
	gchar *buff;
	
	bd = (BreakpointsDBase *) data;
	selection =
		gtk_tree_view_get_selection (GTK_TREE_VIEW (bd->priv->treeview));
	valid = gtk_tree_selection_get_selected (selection, &model, &iter);
	if (!valid)
	{
		g_object_unref (gxml);
		return;
	}
	
	gtk_tree_model_get (model, &iter, DATA_COLUMN, &bid, -1);

	gxml = glade_xml_new (PREFS_GLADE,
						  "breakpoint_properties_dialog", NULL);
	dialog = glade_xml_get_widget (gxml, "breakpoint_properties_dialog");
	gtk_window_set_transient_for (GTK_WINDOW (dialog),
								  GTK_WINDOW (bd->priv->window));
	location_label = glade_xml_get_widget (gxml, "breakpoint_location_label");
	location_entry = glade_xml_get_widget (gxml, "breakpoint_location_entry");
	gtk_widget_hide (location_entry);
	gtk_widget_show (location_label);
	condition_entry = glade_xml_get_widget (gxml, "breakpoint_condition_entry");
	pass_entry = glade_xml_get_widget (gxml, "breakpoint_pass_entry");
	
	if (bid->file && strlen (bid->file) > 0)
		buff = g_strdup_printf ("%s:%d", bid->file, bid->line);
	else
		buff = g_strdup_printf ("%d", bid->line);
	
	gtk_label_set_text (GTK_LABEL (location_label), buff);
	g_free (buff);
	
	if (bid->condition && strlen (bid->condition) > 0)
		gtk_entry_set_text (GTK_ENTRY (condition_entry), bid->condition);
	
	buff = g_strdup_printf ("%d", bid->pass);
	gtk_entry_set_text (GTK_ENTRY (pass_entry), buff);
	g_free (buff);
	
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
	{
		const gchar *pass, *cond;
		gchar *command, passbuff[256];
		
		pass = gtk_entry_get_text (GTK_ENTRY (pass_entry));
		cond = gtk_entry_get_text (GTK_ENTRY (condition_entry));
		bid->pass = 0;
		if (pass && strlen (pass))
			bid->pass = atoi (pass);
		
		command = g_strdup_printf ("-break-after %d %d",
								   bid->id, bid->pass);
		debugger_command (bd->priv->debugger, command, FALSE, NULL, NULL);
		g_free (command);
		
		g_free (bid->condition);
		bid->condition = NULL;
		
		if (cond && strlen (cond))
		{
			bid->condition = g_strdup (cond);
			command = g_strdup_printf ("-break-condition %d %s",
									   bid->id, cond);
			debugger_command (bd->priv->debugger, command, FALSE, NULL, NULL);
			g_free (command);
		}
		else
		{
			command = g_strdup_printf ("-break-condition %d", bid->id);
			debugger_command (bd->priv->debugger, command, FALSE, NULL, NULL);
			g_free (command);
		}
		snprintf (passbuff, 256, "%d", bid->pass);
		gtk_tree_store_set (GTK_TREE_STORE (model), &iter, PASS_COLUMN,
							passbuff, CONDITION_COLUMN, cond,
							DATA_COLUMN, bid, -1);
		
		bid->is_editing = FALSE;
	}
	breakpoint_item_free (bid);
	gtk_widget_destroy (dialog);
	g_object_unref (gxml);
}

static void
on_treeview_enabled_toggled (GtkCellRendererToggle *cell,
							 gchar			       *path_str,
							 BreakpointsDBase      *bd)
{
	BreakpointItem *bi;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;
	gchar buff[256];
	
	path = gtk_tree_path_new_from_string (path_str);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (bd->priv->treeview));
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, DATA_COLUMN, &bi, -1);
	
	bi->enable ^= 1;

	if (bi->enable)
	{
		snprintf (buff, 256, "-break-enable %d", bi->id);
		if (bi->editor)
			set_breakpoint_in_editor (bi, BREAKPOINT_ENABLED, FALSE);
	}
	else
	{
		snprintf (buff, 256, "-break-disable %d", bi->id);
		if (bi->editor)
			set_breakpoint_in_editor (bi, BREAKPOINT_DISABLED, FALSE);
	}
	debugger_command (bd->priv->debugger, buff, FALSE, NULL, NULL);
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
						ENABLED_COLUMN,	bi->enable,
						DATA_COLUMN, bi, -1);
	breakpoint_item_free (bi);
}

static void
on_bk_treeview_selection_changed (GtkTreeSelection *selectoin,
								  BreakpointsDBase *bd)
{
	breakpoints_dbase_update_controls (bd);
}


static gboolean
on_bk_window_delete_event (GtkWindow *win, GdkEvent *event, BreakpointsDBase *bd)
{
	breakpoints_dbase_hide (bd);
	return TRUE;
}

static gboolean
on_breakpoints_key_press_event(GtkWidget *widget, GdkEventKey *event,
                               BreakpointsDBase *bd)
{
	if (event->keyval == GDK_Escape)
	{
		breakpoints_dbase_hide (bd);
		return TRUE;
	}
	return FALSE;
}

static void
on_results_arrived (Debugger *debugger, const gchar *command,
					const GDBMIValue *mi_results,	BreakpointsDBase *bd)
{
	if (strncmp (command, "break", strlen ("break")) == 0 ||
		strncmp (command, "delete", strlen ("delete")) == 0 ||
		strncmp (command, "enable", strlen ("enable")) == 0 ||
		strncmp (command, "disable", strlen ("disable")) == 0 ||
		strncmp (command, "tbreak", strlen ("tbreak")) == 0 ||
		strncmp (command, "hbreak", strlen ("hbreak")) == 0 ||
		strncmp (command, "thbreak", strlen ("thbreak")) == 0 ||
		strncmp (command, "rbreak", strlen ("rbreak")) == 0)
	{
		debugger_command (bd->priv->debugger, "-break-list", FALSE,
						  breakpoints_dbase_update, bd);
	}
}

BreakpointsDBase *
breakpoints_dbase_new (AnjutaPlugin *plugin, Debugger *debugger)
{
	BreakpointsDBase *bd;
	
	bd = g_new0 (BreakpointsDBase, 1);
	bd->priv = g_new0 (BreakpointsDBasePriv, 1);
	if (bd) {
		GtkTreeView *view;
		GtkTreeStore *store;
		GtkCellRenderer *renderer;
		GtkTreeViewColumn *column;
		int i;

		bd->priv->plugin = (GdbPlugin*)plugin; /* FIXME */
		bd->priv->debugger = debugger;
		g_signal_connect (debugger, "results-arrived",
						  G_CALLBACK (on_results_arrived), bd);
		g_object_ref (debugger);
		
		/* breakpoints dialog */
		bd->priv->gxml = glade_xml_new (PREFS_GLADE,
										"breakpoints_dialog", NULL);
		glade_xml_signal_autoconnect (bd->priv->gxml);
		
		bd->priv->window = glade_xml_get_widget (bd->priv->gxml,
												 "breakpoints_dialog");
		gtk_widget_hide (bd->priv->window);
		
		bd->priv->treeview = glade_xml_get_widget (bd->priv->gxml,
												"breakpoints_tv");
		bd->priv->remove_button = glade_xml_get_widget (bd->priv->gxml,
												"breakpoints_remove_button");
		bd->priv->jumpto_button = glade_xml_get_widget (bd->priv->gxml,
												"breakpoints_jumpto_button");
		bd->priv->properties_button = glade_xml_get_widget (bd->priv->gxml,
												"breakpoints_properties_button");
		bd->priv->add_button = glade_xml_get_widget (bd->priv->gxml,
												"breakpoints_add_button");
		bd->priv->removeall_button = glade_xml_get_widget (bd->priv->gxml,
												"breakpoints_removeall_button");
		bd->priv->enableall_button = glade_xml_get_widget (bd->priv->gxml,
												"breakpoints_enableall_button");
		bd->priv->disableall_button = glade_xml_get_widget (bd->priv->gxml,
												"breakpoints_disableall_button");

		view = GTK_TREE_VIEW (bd->priv->treeview);
		store = gtk_tree_store_new (COLUMNS_NB,
									G_TYPE_BOOLEAN,
									G_TYPE_STRING,
									G_TYPE_STRING,
									G_TYPE_STRING,
									G_TYPE_STRING,
									G_TYPE_STRING,
									G_TYPE_STRING,
									G_TYPE_STRING,
									G_TYPE_STRING,
									G_TYPE_STRING,
									G_TYPE_STRING,
									BREAKPOINT_ITEM_TYPE);
		
		gtk_tree_view_set_model (view, GTK_TREE_MODEL (store));
		gtk_tree_selection_set_mode (gtk_tree_view_get_selection (view),
					     GTK_SELECTION_SINGLE);
		g_object_unref (G_OBJECT (store));

		renderer = gtk_cell_renderer_toggle_new ();
		column = gtk_tree_view_column_new_with_attributes (column_names[0],
														   renderer,
								   						   "active",
														   ENABLED_COLUMN,
														   NULL);
		g_signal_connect (renderer, "toggled",
						  G_CALLBACK (on_treeview_enabled_toggled), bd);
		gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
		gtk_tree_view_append_column (view, column);
		renderer = gtk_cell_renderer_text_new ();

		for (i = NUMBER_COLUMN; i < (COLUMNS_NB - 1); i++)
		{
			column =
				gtk_tree_view_column_new_with_attributes (column_names[i],
													renderer, "text", i, NULL);
			gtk_tree_view_column_set_sizing (column,
											 GTK_TREE_VIEW_COLUMN_AUTOSIZE);
			gtk_tree_view_append_column (view, column);
		}

		g_signal_connect (G_OBJECT (bd->priv->remove_button), "clicked",
						  G_CALLBACK (on_bk_remove_clicked), bd);
		g_signal_connect (G_OBJECT (bd->priv->jumpto_button), "clicked",
						  G_CALLBACK (on_bk_jumpto_clicked), bd);
		g_signal_connect (G_OBJECT (bd->priv->properties_button), "clicked",
						  G_CALLBACK (on_bk_properties_clicked), bd);
		
		g_signal_connect_swapped (G_OBJECT (bd->priv->add_button),
								  "clicked",
								  G_CALLBACK (breakpoints_dbase_add),
								  bd);
		g_signal_connect_swapped (G_OBJECT (bd->priv->removeall_button),
								  "clicked",
								  G_CALLBACK (breakpoints_dbase_remove_all),
								  bd);
		g_signal_connect_swapped (G_OBJECT (bd->priv->enableall_button),
								  "clicked",
								  G_CALLBACK (breakpoints_dbase_enable_all),
								  bd);
		g_signal_connect_swapped (G_OBJECT (bd->priv->disableall_button),
								  "clicked",
								  G_CALLBACK (breakpoints_dbase_disable_all),
								  bd);

		g_signal_connect (G_OBJECT (gtk_tree_view_get_selection
					(GTK_TREE_VIEW (bd->priv->treeview))), "changed",
						  G_CALLBACK (on_bk_treeview_selection_changed), bd);
		g_signal_connect (G_OBJECT (bd->priv->window), "delete_event",
				  G_CALLBACK (on_bk_window_delete_event), bd);
		g_signal_connect (G_OBJECT (bd->priv->window), "key-press-event",
					  GTK_SIGNAL_FUNC (on_breakpoints_key_press_event), bd);

		bd->priv->cond_history = NULL;
		bd->priv->pass_history = NULL;
		bd->priv->loc_history = NULL;
		bd->priv->is_showing = FALSE;
		bd->priv->win_pos_x = 50;
		bd->priv->win_pos_y = 50;
		bd->priv->win_width = 500;
		bd->priv->win_height = 300;
	}

	return bd;
}

void
breakpoints_dbase_destroy (BreakpointsDBase * bd)
{
	g_return_if_fail (bd != NULL);
	
	/* This is necessary to disconnect the update signals */
	breakpoints_dbase_hide (bd);
	/* This is necessary to clear the editor of breakpoint markers */
	breakpoints_dbase_clear (bd);
	
	g_signal_handlers_disconnect_by_func (bd->priv->debugger,
										  G_CALLBACK (on_results_arrived),
										  bd);

	g_object_unref (bd->priv->debugger);
	
	if (bd->priv->cond_history)
		g_free (bd->priv->cond_history);
	if (bd->priv->pass_history)
		g_free (bd->priv->pass_history);
	if (bd->priv->loc_history)
		g_free (bd->priv->loc_history);

	gtk_widget_destroy (bd->priv->window);
	g_object_unref (bd->priv->gxml);
	g_free (bd->priv);
	g_free (bd);
}

#if 0
/* FIXME: Dont' make it static */
static int count;

static gboolean
on_treeview_foreach (GtkTreeModel *model, GtkTreePath *path,
					 GtkTreeIter *iter, gpointer data)
{
	BreakpointItem *bi;
/* TODO	ProjectDBase *pdb = data; */
	
	gtk_tree_model_get (GTK_TREE_MODEL (model), iter, DATA_COLUMN, &bi, -1);
/* TODO	breakpoint_item_save (bi, pdb, count); */
	breakpoint_item_free (bi);
	count++;
	return FALSE;
}

void
breakpoints_dbase_save (BreakpointsDBase * bd, ProjectDBase * pdb )
{
	GtkTreeModel *model;

	g_return_if_fail (bd != NULL);
	g_return_if_fail (pdb != NULL);
	
	session_clear_section( pdb, SECSTR(SECTION_BREAKPOINTS) );
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (bd->priv->treeview));
	count = 0;
	gtk_tree_model_foreach (model, on_treeview_foreach, pdb);
}

void
breakpoints_dbase_load (BreakpointsDBase *bd, ProjectDBase *p)
{
	gpointer config_iterator;
	guint loaded = 0;

	g_return_if_fail (p != NULL);
	breakpoints_dbase_clear(bd);
	config_iterator = session_get_iterator (p, SECSTR (SECTION_BREAKPOINTS));
	if (config_iterator !=  NULL)
	{
		gchar *szItem, *szData;
		loaded = 0;
		while ((config_iterator = gnome_config_iterator_next (config_iterator,
															  &szItem,
															  &szData )))
		{
			// shouldn't happen, but I'm paranoid
			if( NULL != szData )
			{
				BreakpointItem *bi;
				gboolean bToDel = TRUE ;
				
				bi = breakpoint_item_new ();
				if (NULL != bi)
				{
					if (breakpoint_item_load (bi, szData))
					{
						breakpoints_dbase_set_from_item (bd, bi, TRUE);
						bToDel = FALSE ;
					}
				}
				if (bToDel && bi)
					breakpoint_item_destroy (bi);
			}
			loaded++;
			g_free (szItem);
			g_free (szData);
		}
	}
}

#endif

void
breakpoints_dbase_clear (BreakpointsDBase *bd)
{
	g_return_if_fail (bd != NULL);

	breakpoints_dbase_delete_all_breakpoints (bd);
	if (bd->priv->treeview) {
		GtkTreeModel *model =
			gtk_tree_view_get_model (GTK_TREE_VIEW (bd->priv->treeview));
		gtk_tree_store_clear (GTK_TREE_STORE (model));
	}
	if (bd->priv->plugin->current_editor &&
		IANJUTA_IS_MARKABLE (bd->priv->plugin->current_editor))
	{
		IAnjutaMarkable *ed = IANJUTA_MARKABLE (bd->priv->plugin->current_editor);
		ianjuta_markable_delete_all_markers (ed, BREAKPOINT_ENABLED, NULL);
		ianjuta_markable_delete_all_markers (ed, BREAKPOINT_DISABLED, NULL);
	}
}

static void
breakpoints_dbase_delete_all_breakpoints (BreakpointsDBase *bd)
{
	g_return_if_fail (bd != NULL);

	/* TODO */
}

void
breakpoints_dbase_show (BreakpointsDBase *bd)
{
	g_return_if_fail (bd != NULL);
	if (bd->priv->is_showing)
	{
		gdk_window_raise (bd->priv->window->window);
		return;
	}
	gtk_widget_set_uposition (bd->priv->window,
							  bd->priv->win_pos_x,
							  bd->priv->win_pos_y);
	gtk_window_set_default_size (GTK_WINDOW
								 (bd->priv->window),
								 bd->priv->win_width,
								 bd->priv->win_height);
	gtk_widget_show (bd->priv->window);
	bd->priv->is_showing = TRUE;
	
	/* Control updates enable */
	breakpoints_dbase_update_controls (bd);
	g_signal_connect_swapped (bd->priv->debugger, "program-running",
							  G_CALLBACK (breakpoints_dbase_update_controls),
							  bd);						  
	g_signal_connect_swapped (bd->priv->debugger, "program-stopped",
							  G_CALLBACK (breakpoints_dbase_update_controls),
							  bd);						  
	g_signal_connect_swapped (bd->priv->debugger, "program-exited",
							  G_CALLBACK (breakpoints_dbase_update_controls),
							  bd);						  
}

void
breakpoints_dbase_hide (BreakpointsDBase *bd)
{
	g_return_if_fail (bd != NULL);
	if (bd->priv->is_showing == FALSE)
		return;
	gdk_window_get_root_origin (bd->priv->window->
								window, &bd->priv->win_pos_x,
								&bd->priv->win_pos_y);
	gdk_window_get_size (bd->priv->window->window,
						 &bd->priv->win_width, &bd->priv->win_height);
	gtk_widget_hide (bd->priv->window);
	bd->priv->is_showing = FALSE;
	
	g_signal_handlers_disconnect_by_func (bd->priv->debugger,
										  breakpoints_dbase_update_controls,
										  bd);
}

/* call back for the gdb '-break-list' command'
   parse the gdb output and updates the db accordingly */
void
breakpoints_dbase_update (Debugger *debugger, const GDBMIValue *mi_results,
						  const GList *outputs, gpointer data)
{
	const GDBMIValue *table, *body;
	BreakpointsDBase *bd = (BreakpointsDBase*) data;
	
	breakpoints_dbase_clear (bd);
	
	if (mi_results == NULL)
		return;

	table = gdbmi_value_hash_lookup (mi_results, "BreakpointTable");
	g_return_if_fail (table != NULL);
	
	body = gdbmi_value_hash_lookup (table, "body");
	if (body == NULL)
		return;
	
	gdbmi_value_foreach (body, (GFunc)breakpoints_dbase_add_brkpnt, bd);
}

#if 0
static gboolean
breakpoints_dbase_set_from_item (BreakpointsDBase *bd, BreakpointItem *bi,
								 gboolean add_to_treeview)
{
	struct stat st;
	const gchar *fn;
	gchar *buff;
	gboolean disable, old;
	gint ret;
	IAnjutaDocumentManager *docman;
	
	old = disable = FALSE;
	docman = get_document_manager (bd->priv->plugin);
	fn = ianjuta_document_manager_get_full_filename (docman, bi->file, NULL);
	ret = stat (fn, &st);
	if (ret != 0)
	{
		old = TRUE;
		disable = TRUE;
	}
	if (bi->time < st.st_mtime)
	{
		old = TRUE;
		disable = TRUE;
	}
	if (bi->condition)
	{
		buff = g_strdup_printf ("break %s:%u if %s", bi->file, bi->line,
								bi->condition);
		debugger_command (bd->priv->debugger, buff, FALSE, NULL, NULL);
		g_free (buff);
	}
	else
	{
		buff = g_strdup_printf ("break %s:%u", bi->file, bi->line);
		debugger_command (bd->priv->debugger, buff, FALSE, NULL, NULL);
		g_free (buff);
	}
	if (bi->pass > 0)
	{
		buff = g_strdup_printf ("ignore $bpnum %d", bi->pass);
		debugger_command (bd->priv->debugger, buff, FALSE, NULL, NULL);
		g_free (buff);
	}
	if (disable)
	{
		buff = g_strdup_printf ("disable $bpnum");
		debugger_command (bd->priv->debugger, buff, FALSE, NULL, NULL);
		g_free (buff);
		disable = FALSE;
	}
	return old;
}

static gboolean
on_set_breakpoint_foreach (GtkTreeModel *model, GtkTreePath *path,
						   GtkTreeIter *iter, gpointer data)
{
	BreakpointItem *bi;
	BreakpointsDBase *bd = data;
	
	gtk_tree_model_get (GTK_TREE_MODEL (model), iter, DATA_COLUMN, &bi, -1);
	breakpoints_dbase_set_from_item (bd, bi, FALSE);
	breakpoint_item_free (bi);
	return FALSE;
}

void
breakpoints_dbase_set_all (BreakpointsDBase * bd)
{
	gboolean old = FALSE;
	GtkTreeModel *model;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (bd->priv->treeview));
	gtk_tree_model_foreach (model, on_set_breakpoint_foreach, bd);

	if (old)
	{
/* TODO		anjuta_warning (_("Old breakpoints disabled.")); */
	}
	debugger_command (bd->priv->debugger, "-break-list", FALSE,
					  breakpoints_dbase_update, bd);
}
#endif

/**********************************
 * Private functions: Do not use  *
 **********************************/
void
breakpoints_dbase_update_controls (BreakpointsDBase * bd)
{
	gboolean A, R, C, S;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	
	A = TRUE;
	R = TRUE; /* debugger_is_ready (bd->priv->debugger); */

	selection =
		gtk_tree_view_get_selection (GTK_TREE_VIEW (bd->priv->treeview));
	S = gtk_tree_selection_get_selected (selection, &model, &iter);
	C = gtk_tree_model_get_iter_first(model, &iter);
	
	gtk_widget_set_sensitive (bd->priv->remove_button, A && R && C && S);
	gtk_widget_set_sensitive (bd->priv->jumpto_button, C && S);
	gtk_widget_set_sensitive (bd->priv->properties_button, A && R && C && S);

	gtk_widget_set_sensitive (bd->priv->add_button, A && R);
	gtk_widget_set_sensitive (bd->priv->removeall_button, A && R);
	gtk_widget_set_sensitive (bd->priv->enableall_button, A && R);
	gtk_widget_set_sensitive (bd->priv->disableall_button, A && R);
	gtk_widget_set_sensitive (bd->priv->treeview, A && R);
}

static void
breakpoints_dbase_add_brkpnt (const GDBMIValue *brkpnt, gpointer user_data)
{
	BreakpointsDBase *bd;
	BreakpointItem *bi;
	GtkTreeStore *store;
	GtkTreeIter iter;
	const GDBMIValue *literal;
	const gchar *number, *type, *func, *disp, *times, *addr;
	const gchar *file, *line, *ignore, *enabled, *cond;
	
	bd = (BreakpointsDBase*) user_data;

	g_return_if_fail (bd != NULL);
	
	number = type = func = disp = times = addr = NULL;
	file = line = ignore = enabled = cond = NULL;
	
	literal = gdbmi_value_hash_lookup (brkpnt, "number");
	number = gdbmi_value_literal_get (literal);

	literal = gdbmi_value_hash_lookup (brkpnt, "file");
	file = gdbmi_value_literal_get (literal);
	
	literal = gdbmi_value_hash_lookup (brkpnt, "line");
	line = gdbmi_value_literal_get (literal);

	literal = gdbmi_value_hash_lookup (brkpnt, "type");
	if (literal)
		type = gdbmi_value_literal_get (literal);
	
	literal = gdbmi_value_hash_lookup (brkpnt, "disp");
	if (literal)
		disp = gdbmi_value_literal_get (literal);
	
	literal = gdbmi_value_hash_lookup (brkpnt, "enabled");
	if (literal)
		enabled = gdbmi_value_literal_get (literal);
	
	literal = gdbmi_value_hash_lookup (brkpnt, "addr");
	if (literal)
		addr = gdbmi_value_literal_get (literal);
	
	literal = gdbmi_value_hash_lookup (brkpnt, "func");
	if (literal)
		func = gdbmi_value_literal_get (literal);
	
	literal = gdbmi_value_hash_lookup (brkpnt, "times");
	if (literal)
		times = gdbmi_value_literal_get (literal);
	
	literal = gdbmi_value_hash_lookup (brkpnt, "ignore");
	if (literal)
		ignore = gdbmi_value_literal_get (literal);
	
	literal = gdbmi_value_hash_lookup (brkpnt, "cond");
	if (literal)
		cond = gdbmi_value_literal_get (literal);
	
	g_return_if_fail (number && file && line);
	
	/* add breakpoint to list */

	bi = breakpoint_item_new (bd);
	
	if (number)
		bi->id = atoi (number);
	if (type)
		bi->type = g_strdup (type);
	if (file)
		bi->file = g_path_get_basename (file);
	if (line)
		bi->line = atoi (line);
	if (func)
		bi->function = g_strdup (func);
	if (ignore)
		bi->pass = atoi (ignore);
	if (cond)
		bi->condition = g_strdup (cond);
	if (times)
		bi->times = atoi (times);
	if (addr)
		bi->address = g_strdup (addr);
	if (disp)
		bi->disp = g_strdup (disp);

	if (strcmp (enabled, "y") == 0)
		bi->enable = TRUE;
	else
		bi->enable = FALSE;

	if (bd->priv->plugin->current_editor &&
		editor_is_debugger_file (bi,
					 IANJUTA_EDITOR (bd->priv->plugin->current_editor)))
	{
		bi->editor = IANJUTA_EDITOR (bd->priv->plugin->current_editor);
		if (bi->enable)
			set_breakpoint_in_editor (bi, BREAKPOINT_ENABLED, TRUE);
		else
			set_breakpoint_in_editor (bi, BREAKPOINT_DISABLED, TRUE);
	}
	
	bi->time = time (NULL);
	
	store = GTK_TREE_STORE (gtk_tree_view_get_model
							(GTK_TREE_VIEW (bd->priv->treeview)));
	gtk_tree_store_append (store, &iter, NULL);
	
	gtk_tree_store_set (store, &iter,
						ENABLED_COLUMN, bi->enable,
						NUMBER_COLUMN, number,
						TYPE_COLUMN, type,
						DISP_COLUMN, disp,
						FILENAME_COLUMN, bi->file,
						LINENO_COLUMN, line,
						ADDRESS_COLUMN, addr,
						FUNCTION_COLUMN, func,
						PASS_COLUMN, ignore,
						TIMES_COLUMN, times,
						CONDITION_COLUMN, cond,
						DATA_COLUMN, bi,
						-1);
}

static gboolean
on_set_breakpoint_te_foreach (GtkTreeModel *model, GtkTreePath *path,
							  GtkTreeIter *iter, gpointer data)
{
	BreakpointItem *bi;
	IAnjutaEditor *te = data;
	
	gtk_tree_model_get (GTK_TREE_MODEL (model), iter, DATA_COLUMN, &bi, -1);
	
	if (bi->line > 0 && bi->editor == NULL)
	{
		if (editor_is_debugger_file (bi, te))
		{
			bi->editor = te;
			if (bi->enable)
				set_breakpoint_in_editor (bi, BREAKPOINT_ENABLED, TRUE);
			else
				set_breakpoint_in_editor (bi, BREAKPOINT_DISABLED, TRUE);
			gtk_tree_store_set (GTK_TREE_STORE (model), iter,
								DATA_COLUMN, bi, -1);
		}
	}
	breakpoint_item_free (bi);
	return FALSE;
}

void
breakpoints_dbase_set_all_in_editor (BreakpointsDBase* bd, IAnjutaEditor* te)
{
	GtkTreeModel *model;
	gchar *uri;
	
	g_return_if_fail (te != NULL);
	g_return_if_fail (bd != NULL);
	
	uri = ianjuta_file_get_uri (IANJUTA_FILE (te), NULL);
	if (uri == NULL)
		return;
	
	g_free (uri);
	
	if (IANJUTA_IS_MARKABLE (te))
	{
		IAnjutaMarkable *ed = IANJUTA_MARKABLE (te);
		ianjuta_markable_delete_all_markers (ed, BREAKPOINT_ENABLED, NULL);
		ianjuta_markable_delete_all_markers (ed, BREAKPOINT_DISABLED, NULL);
	}
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (bd->priv->treeview));
	gtk_tree_model_foreach (model, on_set_breakpoint_te_foreach, te);
}

static gboolean
on_clear_breakpoint_te_foreach (GtkTreeModel *model, GtkTreePath *path,
								GtkTreeIter *iter, gpointer data)
{
	BreakpointItem *bi;
	GObject *dead_te = data;
	
	gtk_tree_model_get (GTK_TREE_MODEL (model), iter, DATA_COLUMN, &bi, -1);
	
	if ((GObject*)bi->editor == dead_te)
	{	
		bi->editor = NULL;
		gtk_tree_store_set (GTK_TREE_STORE (model), iter,
							DATA_COLUMN, bi, -1);
	}
	breakpoint_item_free (bi);
	return FALSE;
}

void
breakpoints_dbase_clear_all_in_editor (BreakpointsDBase *bd, GObject *dead_te)
{
	GtkTreeModel *model;
	g_return_if_fail (dead_te != NULL);
	g_return_if_fail (bd != NULL);
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (bd->priv->treeview));
	gtk_tree_model_foreach (model, on_clear_breakpoint_te_foreach, dead_te);
}

static gboolean
on_delete_matching_foreach (GtkTreeModel *model, GtkTreePath *path,
							GtkTreeIter *iter, gpointer data)
{
	BreakpointItem *item;
	gint moved_line, line;
	BreakpointsDBase* bd;
	IAnjutaEditor *te;
	const gchar *filename;
	gchar buff[256];

	bd = (BreakpointsDBase *) data;

	if (bd->priv->plugin->current_editor == NULL)
		return TRUE;
	
	te = IANJUTA_EDITOR (bd->priv->plugin->current_editor);

	gtk_tree_model_get (GTK_TREE_MODEL (model), iter, DATA_COLUMN, &item, -1);
	
	if (item->editor != te)
	{
		breakpoint_item_free (item);
		return FALSE;
	}
	filename = ianjuta_editor_get_filename (te, NULL);
	if (strcmp (filename, item->file) != 0)
	{
		breakpoint_item_free (item);
		return FALSE;
	}
	line = ianjuta_editor_get_lineno (te, NULL);
	moved_line = 0;
	if (IANJUTA_IS_MARKABLE (te))
	{
		moved_line = ianjuta_markable_location_from_handle (IANJUTA_MARKABLE (te),
															item->handle, NULL);
	}
	if (moved_line <= 0)
		moved_line = item->line;
	
	if (moved_line == line && moved_line >= 0)
	{
		/* Delete breakpoint marker from screen */
		if (IANJUTA_IS_MARKABLE (te))
		{
			ianjuta_markable_unmark (IANJUTA_MARKABLE (te), line,
									 BREAKPOINT_ENABLED, NULL);
			ianjuta_markable_unmark (IANJUTA_MARKABLE (te), line,
									 BREAKPOINT_DISABLED, NULL);
		}
		/* Delete breakpoint in debugger */
		snprintf (buff, 256, "-break-delete %d", item->id);
		debugger_command (bd->priv->debugger, buff, FALSE, NULL, NULL);
		gtk_tree_store_remove (GTK_TREE_STORE (model), iter);
		breakpoint_item_free (item);
		return TRUE;
	}
	breakpoint_item_free (item);
	return FALSE;
}

/*  if l == 0  :  line = current line */
/*  return FALSE if debugger not active */
gboolean
breakpoints_dbase_toggle_breakpoint (BreakpointsDBase *bd, guint l)
{
	guint line;
	gchar *buff;
	IAnjutaEditor *te = NULL;
	IAnjutaMarkable *markable = NULL;
	const gchar *filename = NULL;

	g_return_val_if_fail (bd != NULL, FALSE);
	g_return_val_if_fail (bd->priv->plugin->current_editor != NULL, FALSE);
	
	if (debugger_is_ready (bd->priv->debugger) == FALSE)
		return FALSE;
	
	te = IANJUTA_EDITOR (bd->priv->plugin->current_editor);
	filename = ianjuta_editor_get_filename (te, NULL);
	
	if (l <= 0)
		line = ianjuta_editor_get_lineno (te, NULL);
	else
	{
		line = l;
		/* ianjuta_editor_goto_line (te, l, NULL); */
	}
	
	/* Is breakpoint set? */
	if (IANJUTA_IS_MARKABLE (te))
	{
		markable = IANJUTA_MARKABLE (te);
		if (ianjuta_markable_is_marker_set (markable, line,
											BREAKPOINT_ENABLED, NULL) ||
			ianjuta_markable_is_marker_set (markable, line,
											BREAKPOINT_DISABLED, NULL))
		{
			/* Breakpoint is set. So, delete it. */
			GtkTreeModel *model;
			
			model = gtk_tree_view_get_model (GTK_TREE_VIEW (bd->priv->treeview));
			
			gtk_tree_model_foreach (model, on_delete_matching_foreach, bd);
			return TRUE;
		}
	}

	/* Breakpoint is not set. So, set it. */
	filename = ianjuta_editor_get_filename (te, NULL);
	buff = g_strdup_printf ("-break-insert %s:%d", filename, line);
	debugger_command (bd->priv->debugger, buff, FALSE,
					  bk_item_add_mesg_arrived,  bd);
	g_free (buff);
	return TRUE;
}

/*  return FALSE if debugger not active */
gboolean
breakpoints_dbase_toggle_doubleclick (BreakpointsDBase *bd, guint line)
{
	return breakpoints_dbase_toggle_breakpoint (bd, line);
}

#if 0
static void
breakpoints_toggle_enable (BreakpointsDBase *bd, guint line)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;
	BreakpointItem *bi;
	char *treeline;
	gboolean state;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (bd->priv->treeview));
	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid)
	{
		gtk_tree_model_get (model, &iter, ENABLED_COLUMN, &state,
							LINENO_COLUMN, &treeline, DATA_COLUMN, &bi, -1);
		if (atoi(treeline) == line)
		{
			bi->enable ^= 1;

			if (!bi->enable)
				disable_breakpoint (bi->id, bd);
			else
				enable_breakpoint (bi->id, bd);
		}
		g_free (treeline);
		breakpoint_item_free (item);
		valid = gtk_tree_model_iter_next (model, &iter);
	}
}

void
breakpoints_dbase_toggle_singleclick (BreakpointsDBase *bd, guint line)
{
	breakpoints_toggle_enable (bd, line);
}
#endif

void breakpoints_dbase_add (BreakpointsDBase *bd)
{
	GladeXML *gxml;
	GtkWidget *dialog;
	gchar *buff;
	GtkWidget *location_entry;
	IAnjutaDocumentManager *docman = NULL;
	IAnjutaEditor *editor = NULL;
	
	gxml = glade_xml_new (PREFS_GLADE,
						  "breakpoint_properties_dialog", NULL);
	dialog = glade_xml_get_widget (gxml, "breakpoint_properties_dialog");
	gtk_window_set_transient_for (GTK_WINDOW (dialog),
								  GTK_WINDOW (bd->priv->window));
	
	docman = get_document_manager (bd->priv->plugin);
	editor = ianjuta_document_manager_get_current_editor (docman, NULL);
	
	if(editor)
	{
		const gchar *filename;
		filename = ianjuta_editor_get_filename (editor, NULL);
		if (filename)
		{
			gint line;
			line = ianjuta_editor_get_lineno (editor, NULL);
			
			if (line)
				buff = g_strdup_printf("%s:%d", filename, line);
			else
				buff = g_strdup_printf("%s", filename);
			
			location_entry = glade_xml_get_widget (gxml, 
												   "breakpoint_location_entry");
			
			gtk_entry_set_text (GTK_ENTRY (location_entry), buff);
			g_free (buff);
		}
	}
	
	while (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
	{
		if (bk_item_add (bd, gxml) == TRUE)
		{
			break;
		}
	}
	gtk_widget_destroy (dialog);
	g_object_unref (gxml);
}

void breakpoints_dbase_disable_all (BreakpointsDBase *bd)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (bd->priv->treeview));
	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid)
	{
		BreakpointItem *item;
		
		gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
							ENABLED_COLUMN, FALSE, -1);
		
		/* Disable the breakpoint in their editors */
		gtk_tree_model_get (model, &iter, DATA_COLUMN, &item, -1);
		item->enable = FALSE;
		if (item->editor)
			set_breakpoint_in_editor (item, BREAKPOINT_DISABLED, FALSE);
		gtk_tree_store_set (GTK_TREE_STORE (model), &iter, DATA_COLUMN,
							item, ENABLED_COLUMN, FALSE, -1);
		breakpoint_item_free (item);
		valid = gtk_tree_model_iter_next (model, &iter);
	}
	debugger_command (bd->priv->debugger, "disable breakpoints", FALSE,
					  NULL, NULL);
	gdb_util_append_message (ANJUTA_PLUGIN (bd->priv->plugin),
							 _("All breakpoints disabled.\n"));
}

void breakpoints_dbase_enable_all (BreakpointsDBase *bd)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (bd->priv->treeview));
	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid)
	{
		BreakpointItem *item;
		gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
							ENABLED_COLUMN, TRUE, -1);
		
		/* Enable the breakpoint in their editors */
		gtk_tree_model_get (model, &iter, DATA_COLUMN, &item, -1);
		item->enable = TRUE;
		if (item->editor)
			set_breakpoint_in_editor (item, BREAKPOINT_ENABLED, FALSE);
		gtk_tree_store_set (GTK_TREE_STORE (model), &iter, DATA_COLUMN,
							item, ENABLED_COLUMN, TRUE, -1);
		breakpoint_item_free (item);
		valid = gtk_tree_model_iter_next (model, &iter);
	}
	
	debugger_command (bd->priv->debugger, "enable breakpoints", FALSE,
					  NULL, NULL);
	gdb_util_append_message (ANJUTA_PLUGIN (bd->priv->plugin),
							 _("All breakpoints enabled.\n"));
}

void breakpoints_dbase_remove_all (BreakpointsDBase *bd)
{
	GtkWidget *dialog;
	
	dialog = gtk_message_dialog_new (GTK_WINDOW (bd->priv->window),
									 GTK_DIALOG_DESTROY_WITH_PARENT,
									 GTK_MESSAGE_QUESTION,
									 GTK_BUTTONS_NONE,
					_("Are you sure you want to delete all the breakpoints?"));
	gtk_dialog_add_buttons (GTK_DIALOG (dialog),
							GTK_STOCK_CANCEL, GTK_RESPONSE_NO,
							GTK_STOCK_DELETE, GTK_RESPONSE_YES,
							NULL);
	
	gtk_window_set_transient_for (GTK_WINDOW (dialog),
								  GTK_WINDOW (bd->priv->window));

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
	{
		breakpoints_dbase_clear (bd);
		debugger_command (bd->priv->debugger, "delete breakpoints", FALSE,
						  NULL, NULL);
		gdb_util_append_message (ANJUTA_PLUGIN (bd->priv->plugin),
								 _("All breakpoints removed.\n"));
	}
	gtk_widget_destroy (dialog);
}
