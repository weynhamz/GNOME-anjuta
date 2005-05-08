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

#include <libanjuta/resources.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-markable.h>
#include <libanjuta/interfaces/ianjuta-file.h>

#include "breakpoints.h"
#include "debugger.h"
#include "plugin.h"
#include "utilities.h"

#define	BKPT_FIELDS	(13)

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
	gchar *file_path;
	guint line;
	gint handle;
	gboolean handle_invalid;
	gchar *function;
	gchar *info;
	time_t time;
};

struct _BreakpointsDBasePriv
{
	AnjutaPlugin *plugin;

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

void enable_all_breakpoints (BreakpointsDBase* bd);
void disable_all_breakpoints (BreakpointsDBase* bd);
void delete_breakpoint (gint id, BreakpointsDBase* bd, gboolean end);
void property_add_item (Debugger *debugger, const GDBMIValue *mi_results,
						const GList *utputs, gpointer data);
void property_destroy_breakpoint (BreakpointsDBase* bd, GladeXML *gxml,
								  BreakpointItem *bid, GtkWidget *dialog);
void enable_breakpoint (gint id, BreakpointsDBase* bd);
void disable_breakpoint (gint id, BreakpointsDBase* bd);


static gboolean bk_item_add (BreakpointsDBase *bd, GladeXML *gxml,
							 BreakpointItem *bid);

/* TODO
static void breakpoint_item_save (BreakpointItem *bi, ProjectDBase *pdb,
								  const gint nBreak );
*/
static void breakpoints_dbase_delete_all_breakpoints (BreakpointsDBase *bd);
static void breakpoints_dbase_delete_all_markers (BreakpointsDBase *bd,
		IAnjutaMarkableMarker marker);
static void on_treeview_enabled_toggled (GtkCellRendererToggle *cell,
										 gchar *path_str, gpointer data);
static void breakpoints_dbase_add_brkpnt (const GDBMIValue *brkpnt,
										  gpointer user_data);
// static gboolean breakpoints_dbase_set_from_item (BreakpointsDBase *bd,
//												 BreakpointItem *bi,
//												 gboolean add_to_treeview);

static IAnjutaDocumentManager *
get_document_manager (AnjutaPlugin *plugin)
{
	GObject *obj = anjuta_shell_get_object (plugin->shell,
			"IAnjutaDocumentManager", NULL /* TODO */);
	return IANJUTA_DOCUMENT_MANAGER (obj);
}
/*
void
breakpoints_info (Debugger *debugger, const GDBMIValue *mi_results,
				  const GList * outputs, gpointer data )
{
	BreakpointsDBase* bd = data;
	
	debugger_command (bd->priv->debugger, "info breakpoints", TRUE,
					  breakpoints_dbase_update, bd);
}
*/

void
enable_all_breakpoints (BreakpointsDBase* bd)
{
	if (debugger_is_ready (bd->priv->debugger) == FALSE)
		return;
	
	debugger_command (bd->priv->debugger, "enable breakpoints", FALSE,
					  NULL, NULL);
	gdb_util_append_message (bd->priv->plugin, _("All breakpoints enabled:"));
}

void
disable_all_breakpoints (BreakpointsDBase* bd)
{
	if (debugger_is_ready (bd->priv->debugger) == FALSE)
		return;
	
	debugger_command (bd->priv->debugger, "disable breakpoints", FALSE,
					  NULL, NULL);
	gdb_util_append_message (bd->priv->plugin, _("All breakpoints disabled:"));
}

void
delete_breakpoint (gint id, BreakpointsDBase* bd, gboolean end)
{
	gchar buff[20];
	gchar *cmd;
	static GList *list_bp= NULL;
	
	if (debugger_is_ready (bd->priv->debugger) == FALSE)
		return;
	
	if (!end)
	{
		list_bp = g_list_prepend (list_bp, GINT_TO_POINTER (id));
		return;
	}
	else
	{
		cmd = g_strdup("-break-delete");
		if (id != 0)
		{
			sprintf (buff, " %d", id);
			cmd = g_strconcat(cmd, buff, NULL);
		}
		else
		{
			while (list_bp)
			{
				sprintf (buff, " %d", GPOINTER_TO_INT (list_bp->data));
				cmd = g_strconcat(cmd, buff, NULL);
				list_bp = list_bp->next;
			}
		}
		debugger_command (bd->priv->debugger, cmd, FALSE, NULL, NULL);
		debugger_command (bd->priv->debugger, "-break-list", TRUE,
						  breakpoints_dbase_update, bd);
		g_free(cmd);
		g_list_free(list_bp);
	}
}

void
enable_breakpoint (gint id, BreakpointsDBase* bd)
{
	gchar buff[20];

	if (debugger_is_ready (bd->priv->debugger) == FALSE)
		return;
	
	sprintf (buff, "-break-enable %d", id);
	debugger_command (bd->priv->debugger, buff, FALSE, NULL, NULL);
	debugger_command (bd->priv->debugger, "-break-list", TRUE,
					  breakpoints_dbase_update, bd);
}

void
disable_breakpoint (gint id, BreakpointsDBase* bd)
{
	gchar buff[20];

	if (debugger_is_ready (bd->priv->debugger) == FALSE)
		return;
	
	sprintf (buff, "-break-disable %d", id);
	debugger_command (bd->priv->debugger, buff, FALSE, NULL, NULL);
	debugger_command (bd->priv->debugger, "-break-list", TRUE,
					  breakpoints_dbase_update, bd);
}

static BreakpointItem *
breakpoint_item_new ()
{
	BreakpointItem *bi;
	bi = g_new0 (BreakpointItem, 1);
	bi->handle_invalid = TRUE;
	return bi;
}

static void
breakpoint_item_destroy (BreakpointItem *bi)
{
	g_return_if_fail (bi != NULL);

	g_free (bi->function);
	g_free (bi->file);
	g_free (bi->file_path);
	g_free (bi->condition);
	g_free (bi->info);
	g_free (bi->disp);
	g_free (bi->address);

	g_free (bi);
}

static void
on_bk_remove_clicked (GtkWidget *button, gpointer data)
{
	BreakpointsDBase *bd;
	BreakpointItem *bi;
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
		gtk_tree_model_get (model, &iter, DATA_COLUMN, &bi, -1);		
		delete_breakpoint (bi->id, bd, TRUE);
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
	IAnjutaDocumentManager *docman;

	bd = (BreakpointsDBase *) data;

	selection =
		gtk_tree_view_get_selection (GTK_TREE_VIEW (bd->priv->treeview));
	valid = gtk_tree_selection_get_selected (selection, &model, &iter);
	if (valid)
	{
		BreakpointItem *bi;
		gtk_tree_model_get (model, &iter, DATA_COLUMN, &bi, -1);
		breakpoints_dbase_hide (bd);
		docman = get_document_manager (bd->priv->plugin);
		ianjuta_document_manager_goto_file_line (docman, bi->file, bi->line,
												 NULL);
	}
}

/*****************************************************************************/

static void
bk_item_add_mesg_arrived (Debugger *debugger, const GDBMIValue *mi_results,
						  const GList * lines, gpointer data)
{
	const GDBMIValue *bkpt;
	BreakpointItem *bid;

	bid = (BreakpointItem*) data;
	
	bkpt = gdbmi_value_hash_lookup (mi_results, "bkpt");
	g_return_if_fail (bkpt != NULL);
	
	breakpoints_dbase_add_brkpnt (bkpt, bid->bd);
}

static gboolean
bk_item_add (BreakpointsDBase *bd, GladeXML *gxml, BreakpointItem *bid)
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
		
		bid->pass = atoi (pass_text);
		bid->condition = g_strdup (cond_text);
		bid->bd = bd;
		bid->is_editing = FALSE;
		
		buff = g_strdup ("-break-insert");
		if (bid->pass > 0)
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
						  bk_item_add_mesg_arrived,
						  bid);
		g_free (buff);
		return TRUE;
	}
	else
	{
		anjuta_util_dialog_error (GTK_WINDOW (dialog),
			  _("You must give a valid location to set the breakpoint."));
		return TRUE;
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
	bid->bd = bd;
	bid->is_editing = TRUE;

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
							passbuff, CONDITION_COLUMN, cond, -1);
		
		bid->is_editing = FALSE;
	}
	gtk_widget_destroy (dialog);
	g_object_unref (gxml);
}

static void
on_bk_add_clicked (GtkWidget *button, gpointer data)
{
	GladeXML *gxml;
	BreakpointsDBase *bd;
	GtkWidget *dialog;
	BreakpointItem *bid;
	gchar *buff;
	GtkWidget *location_entry;
	IAnjutaDocumentManager *docman = NULL;
	IAnjutaEditor *editor = NULL;
	
	bd = (BreakpointsDBase *) data;
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
	bid = breakpoint_item_new ();
	bid->bd = bd;
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
	{
		if (bk_item_add (bd, gxml, bid) == TRUE)
			gtk_widget_destroy (dialog);
	}
	else
	{
		gtk_widget_destroy (dialog);
	}
	g_object_unref (gxml);
}

static void
on_bk_removeall_clicked (GtkWidget *button, BreakpointsDBase *bd)
{
	GtkWidget *dialog;
	
	dialog = gtk_message_dialog_new (GTK_WINDOW (bd->priv->window),
									 GTK_DIALOG_MODAL |
										GTK_DIALOG_DESTROY_WITH_PARENT,
									 GTK_MESSAGE_QUESTION,
									 GTK_BUTTONS_NONE,
					_("Are you sure you want to delete all the breakpoints?"));
	gtk_dialog_add_buttons (GTK_DIALOG (dialog),
							GTK_STOCK_CANCEL, GTK_RESPONSE_NO,
							GTK_STOCK_DELETE, GTK_RESPONSE_YES,
							NULL);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
	{
		breakpoints_dbase_remove_all (bd);
		breakpoints_dbase_clear (bd);
	}
	else
	{
		breakpoints_dbase_hide (bd);
	}
	gtk_widget_destroy (dialog);
}

static void
on_bk_enableall_clicked (GtkWidget *button, gpointer data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;
	BreakpointsDBase *bd = data;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (bd->priv->treeview));
	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid)
	{
		gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
							ENABLED_COLUMN, TRUE, -1);
		valid = gtk_tree_model_iter_next (model, &iter);
	}
	enable_all_breakpoints (bd);
}

static void
on_bk_disableall_clicked (GtkWidget *button, gpointer data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;
	BreakpointsDBase *bd = data;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (bd->priv->treeview));
	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid)
	{
		gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
							ENABLED_COLUMN, FALSE, -1);
		valid = gtk_tree_model_iter_next (model, &iter);
	}
	disable_all_breakpoints (bd);
}

static void
on_treeview_enabled_toggled (GtkCellRendererToggle *cell,
							 gchar			*path_str,
							 gpointer		 data)
{
	BreakpointsDBase *bd;
	BreakpointItem *bi;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean state;
	GtkTreePath *path;

	path = gtk_tree_path_new_from_string (path_str);

	bd = (BreakpointsDBase *) data;
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (bd->priv->treeview));
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, ENABLED_COLUMN, &state,
						DATA_COLUMN, &bi, -1);
	bi->enable ^= 1;

	if (!bi->enable)
		disable_breakpoint (bi->id, bd);
	else
		enable_breakpoint (bi->id, bd);

	state ^= 1;

	gtk_tree_store_set (GTK_TREE_STORE (model), &iter, ENABLED_COLUMN,
						state, -1);
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

		bd->priv->plugin = plugin;
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
									G_TYPE_POINTER);
		
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
		g_signal_connect (G_OBJECT (bd->priv->add_button), "clicked",
				  G_CALLBACK (on_bk_add_clicked), bd);
		g_signal_connect (G_OBJECT (bd->priv->removeall_button), "clicked",
				  G_CALLBACK (on_bk_removeall_clicked), bd);
		g_signal_connect (G_OBJECT (bd->priv->enableall_button), "clicked",
				  G_CALLBACK (on_bk_enableall_clicked), bd);
		g_signal_connect (G_OBJECT (bd->priv->disableall_button), "clicked",
				  G_CALLBACK (on_bk_disableall_clicked), bd);

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
	count++;
	return FALSE;
}

/* TODO
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
*/

#if 0
void
experimental_not_use_breakpoints_dbase_toggle_breakpoint (BreakpointsDBase* b)
{
	guint line;
	struct BkItemData *bid;
	gchar *buff;
	TextEditor* te;

	g_return_if_fail (b != NULL);
	te = anjuta_get_current_text_editor();
	g_return_if_fail (te != NULL);
	
	if (debugger_is_active()==FALSE) return;
	if (debugger_is_ready()==FALSE) return;

	line = text_editor_get_current_lineno (te);
	/* Is breakpoint set? */
	
	/* Brakpoint is not set. So, set it. */
	bid = g_malloc (sizeof(struct BkItemData));
	if (bid == NULL) return;
	bid->loc_text = g_strdup_printf ("%s:%d", te->filename, line);
	bid->cond_text = g_strdup("");
	bid->pass_text = g_strdup("");
	bid->bd = b;

	buff = g_strdup_printf ("break %s", bid->loc_text);
	debugger_put_cmd_in_queqe (buff, DB_CMD_ALL,
				   bk_item_add_mesg_arrived,
				   bid);
	g_free (buff);
	debugger_execute_cmd_in_queqe ();
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

	breakpoints_dbase_delete_all_markers (bd, IANJUTA_MARKABLE_INTENSE);
	breakpoints_dbase_delete_all_markers (bd, IANJUTA_MARKABLE_ATTENTIVE);
}

static void
breakpoints_dbase_delete_all_breakpoints (BreakpointsDBase *bd)
{
	g_return_if_fail (bd != NULL);

	/* TODO */
}

static void
breakpoints_dbase_delete_all_markers (BreakpointsDBase *bd,
									  IAnjutaMarkableMarker marker)
{
	IAnjutaDocumentManager *docman = get_document_manager (bd->priv->plugin);
	GList *editors, *node;

	g_return_if_fail (docman != NULL);

	editors = ianjuta_document_manager_get_editors (docman, NULL /* TODO */);
	if (editors)
	{
		node = editors;
		do
		{
			if (IANJUTA_IS_MARKABLE (node))
			{
				ianjuta_markable_delete_all_markers (IANJUTA_MARKABLE (node),
													 marker, NULL);
			}
			node = g_list_next (node);
		}
		while (node);
		g_list_free (editors);
	}
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
	R = debugger_is_ready (bd->priv->debugger);

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
	const gchar* full_fname = NULL;
	IAnjutaDocumentManager *docman = NULL;
	IAnjutaEditor *ed = NULL;
	IAnjutaMarkable *markable = NULL;
	
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

	bi = breakpoint_item_new ();
	bi->bd = bd;
	
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

	docman = get_document_manager (bd->priv->plugin);
	full_fname = ianjuta_document_manager_get_full_filename (docman,
															 bi->file,
															 NULL);
	if (full_fname)
		bi->file_path = g_strdup (full_fname);
	
	ed = ianjuta_document_manager_find_editor_with_path (docman, full_fname,
														 NULL);
	if (ed != NULL && IANJUTA_IS_MARKABLE (ed))
	{
		/* IANJUTA_MARKABLE_INTENSE .......... normal breakpoint */
		/* IANJUTA_MARKABLE_ATTENTIVE ........ disabled breakpoint */
		
		markable = IANJUTA_MARKABLE (ed);
		if (bi->enable)
		{
			ianjuta_markable_unmark (markable, bi->line,
									 IANJUTA_MARKABLE_ATTENTIVE,
									 NULL);
			bi->handle = ianjuta_markable_mark (markable, bi->line,
												IANJUTA_MARKABLE_INTENSE,
												NULL);
		}
		else
		{
			ianjuta_markable_unmark (markable, bi->line,
									 IANJUTA_MARKABLE_INTENSE,
									 NULL);
			bi->handle = ianjuta_markable_mark (markable, bi->line,
												IANJUTA_MARKABLE_ATTENTIVE,
												NULL);
		}
		bi->handle_invalid = FALSE;
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
	const gchar *fname;
	BreakpointItem *bi;
	IAnjutaEditor *te = data;
	
	gtk_tree_model_get (GTK_TREE_MODEL (model), iter, DATA_COLUMN, &bi, -1);
	if (bi->line < 0 || bi->handle_invalid == FALSE)
		return FALSE;
	fname = ianjuta_editor_get_filename (te, NULL);
	if (fname && strcmp (fname, bi->file) == 0)
	{
		bi->handle_invalid = FALSE;
	}
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
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (bd->priv->treeview));
	gtk_tree_model_foreach (model, on_set_breakpoint_te_foreach, te);
}

static gboolean
on_clear_breakpoint_te_foreach (GtkTreeModel *model, GtkTreePath *path,
								GtkTreeIter *iter, gpointer data)
{
	const gchar* fname;
	BreakpointItem *bi;
	IAnjutaEditor *te = data;
	
	gtk_tree_model_get (GTK_TREE_MODEL (model), iter, DATA_COLUMN, &bi, -1);
	if (bi->handle_invalid == TRUE)
		return FALSE;
	
	fname = ianjuta_editor_get_filename (te, NULL);
	
	/* FIXME: We should actually be comparing full file paths and not just
	 * the filename. But debugger bi struct does not store full path.
	 */
	if (fname && strcmp (bi->file, fname) == 0)
	{
		bi->handle_invalid = TRUE;
	}
	return FALSE;
}

void
breakpoints_dbase_clear_all_in_editor (BreakpointsDBase *bd, IAnjutaEditor *te)
{
	gchar *uri;
	GtkTreeModel *model;
	g_return_if_fail (te != NULL);
	g_return_if_fail (bd != NULL);
	
	uri = ianjuta_file_get_uri (IANJUTA_FILE (te), NULL);
	if (uri == NULL)
		return;
	
	g_free (uri);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (bd->priv->treeview));
	gtk_tree_model_foreach (model, on_clear_breakpoint_te_foreach, te);
}

static gboolean
on_delete_matching_foreach (GtkTreeModel *model, GtkTreePath *path,
							GtkTreeIter *iter, gpointer data)
{
	BreakpointItem *item;
	gint moved_line, line;
	BreakpointsDBase* bd;
	IAnjutaDocumentManager *docman;
	IAnjutaEditor *te;
	const gchar *filename;

	bd = (BreakpointsDBase *) data;

	docman = get_document_manager (bd->priv->plugin);
	te = ianjuta_document_manager_get_current_editor (docman, NULL);

	gtk_tree_model_get (GTK_TREE_MODEL (model), iter, DATA_COLUMN, &item, -1);
	if (item->handle_invalid)
		return FALSE;

	filename = ianjuta_editor_get_filename (te, NULL);
	if (strcmp (filename, item->file) != 0)
		return FALSE;
	
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
									 IANJUTA_MARKABLE_INTENSE, NULL);
			ianjuta_markable_unmark (IANJUTA_MARKABLE (te), line,
									 IANJUTA_MARKABLE_ATTENTIVE, NULL);
		}
		/* Delete breakpoint in debugger */
		delete_breakpoint (item->id, bd, FALSE);
	}
	return FALSE;
}

/*  if l == 0  :  line = current line */
/*  return FALSE if debugger not active */
gboolean
breakpoints_dbase_toggle_breakpoint (BreakpointsDBase *bd,
									 const gchar *file, guint l)
{
	guint line;
	BreakpointItem *bid;
	gchar *buff;
	IAnjutaDocumentManager *docman = NULL;
	IAnjutaEditor *te = NULL;
	IAnjutaMarkable *markable = NULL;
	const gchar *filename = NULL;

	if (debugger_is_ready (bd->priv->debugger) == FALSE) return FALSE;

	g_return_val_if_fail (bd != NULL, FALSE);

	docman = get_document_manager (bd->priv->plugin);
	g_return_val_if_fail (docman != NULL, FALSE);
	
	if (file)
		ianjuta_document_manager_goto_file_line (docman, file, l, NULL);
	
	te = ianjuta_document_manager_get_current_editor (docman, NULL);
	g_return_val_if_fail (te != NULL, FALSE);
		
	if (l <= 0)
		line = ianjuta_editor_get_lineno (te, NULL);
	else
	{
		line = l;
		ianjuta_editor_goto_line (te, line, NULL);
	}

	/* Is breakpoint set? */
	if (IANJUTA_IS_MARKABLE (te))
	{
		markable = IANJUTA_MARKABLE (te);
		if (ianjuta_markable_is_marker_set (markable, line,
											IANJUTA_MARKABLE_INTENSE, NULL) ||
			ianjuta_markable_is_marker_set (markable, line,
											IANJUTA_MARKABLE_ATTENTIVE, NULL))
		{
			/* Breakpoint is set. So, delete it. */
			GtkTreeModel *model;
			
			model = gtk_tree_view_get_model (GTK_TREE_VIEW (bd->priv->treeview));
			
			gtk_tree_model_foreach (model, on_delete_matching_foreach, bd);
			delete_breakpoint (0, bd, TRUE);
			return TRUE;
		}
	}

	/* Breakpoint is not set. So, set it. */
	bid = breakpoint_item_new ();
	bid->pass = 0;
	bid->bd = bd;

	filename = ianjuta_editor_get_filename (te, NULL);
	buff = g_strdup_printf ("-break-insert %s:%d", filename, line);
	debugger_command (bd->priv->debugger, buff, FALSE,
					  bk_item_add_mesg_arrived,
					  bid);
	g_free (buff);
	return TRUE;
}

/*  return FALSE if debugger not active */
gboolean
breakpoints_dbase_toggle_doubleclick (BreakpointsDBase *bd, guint line)
{
	return breakpoints_dbase_toggle_breakpoint (bd,	NULL, line);
}

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
		valid = gtk_tree_model_iter_next (model, &iter);
	}
}

void
breakpoints_dbase_toggle_singleclick (BreakpointsDBase *bd, guint line)
{
	breakpoints_toggle_enable (bd, line);
}

void breakpoints_dbase_add (BreakpointsDBase *bd)
{
	on_bk_add_clicked (NULL, bd);
}

void breakpoints_dbase_disable_all (BreakpointsDBase *bd)
{
	on_bk_disableall_clicked (NULL, bd);
}

void breakpoints_dbase_remove_all (BreakpointsDBase *bd)
{
	on_bk_removeall_clicked (NULL, bd);
}
