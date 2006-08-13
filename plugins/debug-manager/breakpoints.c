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

/*
 * Handle breakpoints, mainly the graphical interface as the communication
 * with the debugger is handled by the corresponding debugger plugin. The
 * grahical interface includes:
 *       - A dialog including a tree view with all breakpoints
 *       - Several associated dialog (create a new breakpoints, confirmation...)
 *       - The markers in the editor plugin
 *
 * The list of breakpoints is kept in the tree view (GtkTreeModel).
 *
 * The AnjutaDebuggerBreakpoint object is used to communicate with the debugger plugin,
 * it includes all useful information for a breakpoint. Part of this structure
 * is filled by the debugger plugin, another part by the debug manager plugin.
 * This object is normally created and destroyed by the debug manager, but
 * the debugger plugin could create one too (but never destroy it).
 * 
 * The BreakpointItem object includes a AnjutaDebuggerBreakpoint and adds all useful
 * information for the graphical interface (pointer to the editor...)
 *---------------------------------------------------------------------------*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#define DEBUG

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
#include "plugin.h"
#include "utilities.h"

/* Markers definition */
#define BREAKPOINT_ENABLED   IANJUTA_MARKABLE_INTENSE
#define BREAKPOINT_DISABLED  IANJUTA_MARKABLE_ATTENTIVE
#define BREAKPOINT_NONE      -1

typedef struct _BreakpointItem BreakpointItem;

struct _BreakpointItem
{
	BreakpointsDBase *bd;      /* Database owning the breakpoint */
	
	IAnjutaDebuggerBreakpoint *bp;      /* Breakpoint data */
	
	gint handle;               /* handle to mark in editor */
	IAnjutaEditor *editor; 
	gchar* uri;
	
	time_t time;
	
	GtkTreeIter iter;
	gboolean link;
	
	gboolean keep;             /* Do not free memory */
};

struct _BreakpointsDBase
{
	DebugManagerPlugin *plugin;

	IAnjutaDebugger *debugger;
	GladeXML *gxml;
	gchar *cond_history, *loc_history, *pass_history;
	
	gboolean is_showing;
	gint win_pos_x, win_pos_y, win_width, win_height;

	/* Widgets */	
	GtkWidget *scrolledwindow;
	GtkMenu *popup;
	GtkTreeView *treeview;
	GtkWidget *add_button;
	GtkWidget *remove_button;
	GtkWidget *jumpto_button;
	GtkWidget *properties_button;
	GtkWidget *removeall_button;
	GtkWidget *enableall_button;
	GtkWidget *disableall_button;

	/* Menu action */
	GtkActionGroup *action_group;
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

/* Editor functions
 *---------------------------------------------------------------------------*/

static IAnjutaDocumentManager *
get_document_manager (DebugManagerPlugin *plugin)
{
	GObject *obj = anjuta_shell_get_object (ANJUTA_PLUGIN (plugin)->shell,
			"IAnjutaDocumentManager", NULL /* TODO */);
	return IANJUTA_DOCUMENT_MANAGER (obj);
}

/* BreakItem functions
 *---------------------------------------------------------------------------*/

static void
set_breakpoint_in_editor (BreakpointItem *item, gint mark,
						  gboolean bypass_handle)
{
	IAnjutaMarkable *ed;
	gint true_line = -1;

	g_return_if_fail (item != NULL);
	if (item->editor == NULL) return;  /* Do nothing, if editor is not defined */

	switch (mark)
	{
		case BREAKPOINT_NONE:
			if (item->editor)
			{
				ed = IANJUTA_MARKABLE (item->editor);
				
				if (!bypass_handle)
				{
					true_line =
						ianjuta_markable_location_from_handle (ed,
															   item->handle,
															   NULL);
				}
				if (true_line > 0)
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
					ianjuta_markable_unmark (ed, item->bp->line,
											 BREAKPOINT_ENABLED,
											 NULL);
					ianjuta_markable_unmark (ed, item->bp->line,
											 BREAKPOINT_DISABLED,
											 NULL);
				}
			}
			break;
		case BREAKPOINT_ENABLED:
			if (item->editor)
			{
				ed = IANJUTA_MARKABLE (item->editor);
				
				if (!bypass_handle)
				{
					true_line =
						ianjuta_markable_location_from_handle (ed,
															   item->handle,
															   NULL);
				}
				if (true_line > 0)
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
					ianjuta_markable_unmark (ed, item->bp->line,
											 BREAKPOINT_DISABLED,
											 NULL);
					item->handle = ianjuta_markable_mark (ed, item->bp->line,
														  BREAKPOINT_ENABLED,
														  NULL);
				}
			}
			break;
		case BREAKPOINT_DISABLED:
			if (item->editor)
			{
				ed = IANJUTA_MARKABLE (item->editor);
				
				if (!bypass_handle)
				{
					true_line =
						ianjuta_markable_location_from_handle (ed,
															   item->handle,
															   NULL);
				}
				if (true_line > 0)
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
					ianjuta_markable_unmark (ed, item->bp->line,
											 BREAKPOINT_ENABLED,
											 NULL);
					item->handle = ianjuta_markable_mark (ed, item->bp->line,
														  BREAKPOINT_DISABLED,
														  NULL);
				}
			}
			break;
		default:
			g_warning ("Should not be here");
	}
}

/* Anjuta Breakpoint constructor & destructor
 *---------------------------------------------------------------------------*/

static IAnjutaDebuggerBreakpoint*
anjuta_breakpoint_new (void)
{
	IAnjutaDebuggerBreakpoint *bp;
	
	bp = g_new0 (IAnjutaDebuggerBreakpoint, 1);
	
	return bp;
}

static void
anjuta_breakpoint_free (IAnjutaDebuggerBreakpoint* bp)
{
	g_free ((char *)bp->file);
	g_free ((char *)bp->function);
	g_free ((char *)bp->condition);
	
	g_free (bp);
}

/* BreakItem constructor & destructor
 *---------------------------------------------------------------------------*/

static BreakpointItem *
breakpoint_item_new_from_uri (BreakpointsDBase *bd, const gchar* uri, guint line, gboolean enable)
{
	BreakpointItem *bi;
	bi = g_new0 (BreakpointItem, 1);
	bi->bd = bd;
	bi->bp = anjuta_breakpoint_new();
	if (uri != NULL)
	{
		bi->uri = g_strdup (uri);
		bi->bp->file = gnome_vfs_get_local_path_from_uri (uri);
	}
	bi->bp->line = line;
	bi->bp->enable = enable ? IANJUTA_DEBUGGER_YES : IANJUTA_DEBUGGER_NO;
	
	return bi;
}

static BreakpointItem *
breakpoint_item_new_from_string (BreakpointsDBase *bd, const gchar* string, const gchar* uri)
{
	BreakpointItem *bi;
	bi = g_new0 (BreakpointItem, 1);
	bi->bd = bd;
	bi->bp = anjuta_breakpoint_new();
	
	if (*string == '*')
	{
		/* break at address */
		if ((string[1] == '0') && ((string[2] == 'x') || (string[2] == 'X')))
		{
			bi->bp->address = strtoul (string + 3, NULL, 16);
		}
		else
		{
			bi->bp->address = strtoul (string + 3, NULL, 10);
		}
		bi->bp->type = IANJUTA_DEBUGGER_BREAK_ON_ADDRESS;
	}
	else if (isdigit (*string))
	{
		bi->uri = g_strdup (uri);
		bi->bp->file = gnome_vfs_get_local_path_from_uri (uri);
		bi->bp->line = strtoul (string, NULL, 10);
		bi->bp->type = IANJUTA_DEBUGGER_BREAK_ON_LINE;
	}
	else
	{
		const gchar *ptr;
		
		ptr = strchr (string, ':');
		
		if (ptr == NULL)
		{
			/* break on function */
			bi->bp->function = g_strdup (string);
			bi->bp->type = IANJUTA_DEBUGGER_BREAK_ON_FUNCTION;
		}
		else
		{
			if (isdigit (ptr[1]))
			{
				bi->bp->line = strtoul (ptr + 1, NULL, 10);
				bi->bp->type = IANJUTA_DEBUGGER_BREAK_ON_LINE;
			}
			else
			{
				bi->bp->function = g_strdup (ptr + 1);
				bi->bp->type = IANJUTA_DEBUGGER_BREAK_ON_FUNCTION;
			}
			bi->bp->file = g_strndup (string, ptr - string);
			bi->uri = g_strconcat ("file://", bi->bp->file, NULL);
		}
	}
	
	bi->uri = g_strdup (uri);

	return bi;
}

static void
breakpoint_item_free (BreakpointItem *bi)
{
	g_return_if_fail (bi != NULL);

	anjuta_breakpoint_free (bi->bp);
	g_free (bi);
}

/* Private callback functions
 *---------------------------------------------------------------------------*/

/* Callback functions, update breakpoint in tree view and in editor
 * Missing breakpoint are created */

static void
on_breakpoint_item_update_in_ui (const IAnjutaDebuggerBreakpoint *breakpoint, gpointer user_data, GError *err);

static void
breakpoint_item_update_in_ui (BreakpointItem *bi, const IAnjutaDebuggerBreakpoint* bp)
{
	gchar *adr;
	const gchar *filename;
	GtkListStore *store;

	/* Update data from debugger */
	if (bp != NULL)
	{
		bi->bp->id = bp->id;
		if ((bp->file != NULL) && (bi->bp->file == NULL))
		{
			bi->bp->file = g_strdup (bp->file);
		}
		if (bp->line != 0) bi->bp->line = bp->line;
		if ((bp->function != NULL) && (bi->bp->function == NULL))
		{
			bi->bp->function = g_strdup (bp->function);
		}
		if (bp->address !=0) bi->bp->address = bp->address;
		if (bp->enable != IANJUTA_DEBUGGER_UNDEFINED)
		{
			if (bi->bp->enable == IANJUTA_DEBUGGER_UNDEFINED)
			{
	       			bi->bp->enable = bp->enable;
			}
			else if (bi->bp->enable != bp->enable)
			{
				ianjuta_debugger_enable_breakpoint (bi->bd->debugger, bi->bp->id, bi->bp->enable, on_breakpoint_item_update_in_ui, bi, NULL);
			}
		}
		if (bp->ignore != G_MAXUINT32) bi->bp->ignore = bp->ignore;
		if (bp->times != G_MAXUINT32) bi->bp->times = bp->times;
		if (bp->condition != NULL)
		{
			bi->bp->condition = g_strdup (bp->condition);
		}
		if (bp->keep != IANJUTA_DEBUGGER_UNDEFINED)
		{
			bi->bp->keep = bp->keep;
		}
	}
	
	/* Add in tree view */
	store = GTK_LIST_STORE (gtk_tree_view_get_model	(bi->bd->treeview));
	if (bi->link == FALSE)
	{
		gtk_list_store_append (store, &bi->iter);
		bi->link = TRUE;
	}
	
	/* Update node information */
	adr = g_strdup_printf ("0x%x", bi->bp->address);
	filename = strrchr(bi->bp->file, G_DIR_SEPARATOR);
	filename = filename == NULL ? bi->bp->file : filename + 1; /* display name only */
	gtk_list_store_set (store, &bi->iter,
						ENABLED_COLUMN, bi->bp->enable == IANJUTA_DEBUGGER_YES ? TRUE : FALSE,
						NUMBER_COLUMN, bi->bp->id,
						DISP_COLUMN, bi->bp->keep == IANJUTA_DEBUGGER_YES ? "keep" : "nokeep",
						FILENAME_COLUMN, filename,
						LINENO_COLUMN, bi->bp->line,
						ADDRESS_COLUMN, adr,
						TYPE_COLUMN, "breakpoint",
						FUNCTION_COLUMN, bi->bp->function,
						PASS_COLUMN, bi->bp->ignore,
						TIMES_COLUMN, bi->bp->times,
						CONDITION_COLUMN, bi->bp->condition,
						DATA_COLUMN, bi,
						-1);
	g_free (adr);
	
	/* Add in editor if possible */
	if (bi->editor == NULL)
	{
		/* Try to find corresponding editor */
		IAnjutaDocumentManager *docman = IANJUTA_DOCUMENT_MANAGER (anjuta_shell_get_object (ANJUTA_PLUGIN (bi->bd->plugin)->shell,
			"IAnjutaDocumentManager", NULL));
		
		if (docman != NULL)
		{
			IAnjutaEditor* ed;

			ed = ianjuta_document_manager_find_editor_with_path (docman, bi->uri, NULL);
			if ((ed != NULL) && (IANJUTA_IS_MARKABLE (ed)))
			{
				bi->editor = ed;
			}
		}
		
		if (bi->editor != NULL)
		{
			/* Add breakpoint marker in screen */
			set_breakpoint_in_editor (bi, bi->bp->enable == IANJUTA_DEBUGGER_YES ? BREAKPOINT_ENABLED : BREAKPOINT_DISABLED, TRUE);
		}
	}
	else
	{
			set_breakpoint_in_editor (bi, bi->bp->enable == IANJUTA_DEBUGGER_YES ? BREAKPOINT_ENABLED : BREAKPOINT_DISABLED, FALSE);
	}
	
}

static void
on_breakpoint_item_update_in_ui (const IAnjutaDebuggerBreakpoint *breakpoint, gpointer user_data, GError *err)
{
	breakpoint_item_update_in_ui ((BreakpointItem *)user_data, breakpoint);
}

/* Callback function, remove breakpoint in tree view and in editor
 * Missing breakpoint are created */

static void
breakpoint_item_remove_in_ui (BreakpointItem *bi, const IAnjutaDebuggerBreakpoint* bp)
{
	if (bi->keep == TRUE)
	{
		/* Keep breakpoint */
		bi->keep = FALSE;
		return;
	}
		
	/* Breakpoint exist in data base */
	GtkListStore *store;

	/* Remove from tree view */
	store = GTK_LIST_STORE (gtk_tree_view_get_model (bi->bd->treeview));
	gtk_list_store_remove (store, &bi->iter);

	/* Delete breakpoint marker from screen */
	set_breakpoint_in_editor (bi, BREAKPOINT_NONE, FALSE);
		
	breakpoint_item_free (bi);
}

static void
on_breakpoint_item_remove_in_ui (const IAnjutaDebuggerBreakpoint *breakpoint, gpointer user_data, GError *err)
{
	breakpoint_item_remove_in_ui ((BreakpointItem *)user_data, breakpoint);
}

/* Private functions
 *---------------------------------------------------------------------------*/

/* Add breakpoint in tree view, in editor and in debugger
 * Already existing breakpoint are updated */

static void
breakpoints_dbase_add_breakpoint (BreakpointsDBase *bd,  BreakpointItem *bi)
{
	if ((bd->debugger != NULL) && (bi->bp->enable != IANJUTA_DEBUGGER_UNDEFINED))
	{
		if (bi->bp->id != 0)
		{
			/* Breakpoint already exist, remove it first */
			bi->bp->keep = IANJUTA_DEBUGGER_YES;
			ianjuta_debugger_clear_breakpoint (bd->debugger, bi->bp->id, on_breakpoint_item_remove_in_ui, bi, NULL);
		}
		/* Add breakpoint in debugger */
		switch (bi->bp->type)
		{
		case IANJUTA_DEBUGGER_BREAK_ON_LINE:
			ianjuta_debugger_set_breakpoint_at_line (bd->debugger, bi->bp->file, bi->bp->line, on_breakpoint_item_update_in_ui, bi, NULL);
		    break;
		case IANJUTA_DEBUGGER_BREAK_ON_FUNCTION:
			ianjuta_debugger_set_breakpoint_at_function (bd->debugger, bi->bp->file, bi->bp->function, on_breakpoint_item_update_in_ui, bi, NULL);
		    break;
		case IANJUTA_DEBUGGER_BREAK_ON_ADDRESS:
			ianjuta_debugger_set_breakpoint_at_address (bd->debugger, bi->bp->address, on_breakpoint_item_update_in_ui, bi, NULL);
		    break;
		}
	}
	else
	{
		/* Add it directly */
		breakpoint_item_update_in_ui (bi, bi->bp);
	}
}

/* Remove breakpoint in tree view, in editor and in debugger */

static void
breakpoints_dbase_remove_breakpoint (BreakpointsDBase *bd, BreakpointItem *bi)
{
	if (bd->debugger != NULL)
	{
		/* Remove breakpoint in debugger first */
		ianjuta_debugger_clear_breakpoint (bd->debugger, bi->bp->id, on_breakpoint_item_remove_in_ui, bi, NULL);
	}
	else
	{
		/* Remove breakpoint directly */
		breakpoint_item_remove_in_ui (bi, NULL);
	}
}

/* Enable or disable breakpoint in tree view, in editor and in debugger */
 
static void
breakpoints_dbase_enable_breakpoint (BreakpointsDBase *bd, BreakpointItem *bi, gboolean enable)
{
	bi->bp->enable = enable;
	if (bd->debugger != NULL)
	{
		/* Update breakpoint in debugger firt */
		ianjuta_debugger_enable_breakpoint (bd->debugger, bi->bp->id, enable, on_breakpoint_item_update_in_ui, bi, NULL);
	}
	else
	{
		/* Disable breakpoint directly */
		breakpoint_item_update_in_ui (bi, bi->bp);
	}
}

/* Send all breakpoints in debugger */

static void
breakpoints_dbase_add_all_in_debugger (BreakpointsDBase *bd)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	
	g_return_if_fail (bd->treeview != NULL);
	
	model = gtk_tree_view_get_model (bd->treeview);
	
	if (gtk_tree_model_get_iter_first (model, &iter))
	{
		do
		{
			BreakpointItem *bi;
		
			gtk_tree_model_get (model, &iter, DATA_COLUMN, &bi, -1);

			breakpoints_dbase_add_breakpoint (bd, bi);
		} while (gtk_tree_model_iter_next (model, &iter));
	}
	
//		gdb_util_append_message (ANJUTA_PLUGIN (bd->plugin),
//								 _("All breakpoints set.\n"));
}

/* Send all breakpoints in debugger */

static void
breakpoints_dbase_add_pending_in_debugger (BreakpointsDBase *bd)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	
	g_return_if_fail (bd->treeview != NULL);
	
	model = gtk_tree_view_get_model (bd->treeview);

	if (gtk_tree_model_get_iter_first (model, &iter))
	{
		guint i = 0;	
		do
		{
			BreakpointItem *bi;
		
			gtk_tree_model_get (model, &iter, DATA_COLUMN, &bi, -1);

			if (bi->bp->id == 0)
			{
				breakpoints_dbase_add_breakpoint (bd, bi);
			}
		} while (gtk_tree_model_iter_next (model, &iter));
	}
}

static void
on_breakpoint_sharedlib_event (BreakpointsDBase *bd)
{
	breakpoints_dbase_add_pending_in_debugger (bd);
}

/* Remove link with debugger in all breakpoints */

static void
breakpoints_dbase_remove_all_in_debugger (BreakpointsDBase *bd)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	
	g_return_if_fail (bd->treeview != NULL);
	
	model = gtk_tree_view_get_model (bd->treeview);
	
	if (gtk_tree_model_get_iter_first (model, &iter))
	{
		do
		{
			BreakpointItem *bi;
		
			gtk_tree_model_get (model, &iter, DATA_COLUMN, &bi, -1);

			bi->bp->id = 0;
			if ((bi->editor != NULL) && (bi->bp->enable != IANJUTA_DEBUGGER_UNDEFINED))
			{
				/* Add breakpoint marker in screen */
				set_breakpoint_in_editor (bi, bi->bp->enable == IANJUTA_DEBUGGER_YES ? BREAKPOINT_ENABLED : BREAKPOINT_DISABLED, TRUE);
			}
		} while (gtk_tree_model_iter_next (model, &iter));
	}
}

/* Remove all breakpoints */

static void
breakpoints_dbase_remove_all (BreakpointsDBase *bd)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	
	g_return_if_fail (bd->treeview != NULL);
	
	model = gtk_tree_view_get_model (bd->treeview);

	while (gtk_tree_model_get_iter_first (model, &iter))
	{
		BreakpointItem *bi;
		
		gtk_tree_model_get (model, &iter, DATA_COLUMN, &bi, -1);

		breakpoints_dbase_remove_breakpoint (bd, bi);
	}
}

/* Enable or disable all breakpoints in tree view, in editor and in debugger */

static void
breakpoints_dbase_enable_all (BreakpointsDBase *bd, gboolean enable)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	
	g_return_if_fail (bd->treeview != NULL);
	
	model = gtk_tree_view_get_model (bd->treeview);
	
	if (gtk_tree_model_get_iter_first (model, &iter))
	{
		do
		{
			BreakpointItem *bi;
		
			gtk_tree_model_get (model, &iter, DATA_COLUMN, &bi, -1);
			
			breakpoints_dbase_enable_breakpoint (bd, bi, enable);
		} while (gtk_tree_model_iter_next (model, &iter));
	}
	
//	gdb_util_append_message (ANJUTA_PLUGIN (bd->plugin), enable ?
//							 _("All breakpoints enabled.\n") :
//							 _("All breakpoints disabled.\n"));
}

static BreakpointItem* 
breakpoints_dbase_find_breakpoint (BreakpointsDBase *bd, const gchar* uri, guint line)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	
	g_return_val_if_fail (bd->treeview != NULL, NULL);
	
	model = gtk_tree_view_get_model (bd->treeview);
	
	if (gtk_tree_model_get_iter_first (model, &iter))
	{
		do
		{
			BreakpointItem *bi;
		
			gtk_tree_model_get (model, &iter, DATA_COLUMN, &bi, -1);
			
			if ((line == bi->bp->line) && (strcmp (uri, bi->uri) == 0)) return bi;
		} while (gtk_tree_model_iter_next (model, &iter));
	}
	
	return NULL;
}

static void
breakpoints_dbase_update_controls (BreakpointsDBase * bd)
{
	gboolean A, R, C, S;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	
	A = TRUE;
	R = TRUE; /* debugger_is_ready (bd->debugger); */

/*	selection =
		gtk_tree_view_get_selection (bd->treeview);
	S = gtk_tree_selection_get_selected (selection, &model, &iter);
	C = gtk_tree_model_get_iter_first(model, &iter);
	
	gtk_widget_set_sensitive (bd->remove_button, A && R && C && S);
	gtk_widget_set_sensitive (bd->jumpto_button, C && S);
	gtk_widget_set_sensitive (bd->properties_button, A && R && C && S);

	gtk_widget_set_sensitive (bd->add_button, A && R);
	gtk_widget_set_sensitive (bd->removeall_button, A && R);
	gtk_widget_set_sensitive (bd->enableall_button, A && R);
	gtk_widget_set_sensitive (bd->disableall_button, A && R);
	gtk_widget_set_sensitive (GTK_WIDGET (bd->treeview), A && R);*/
}

static void
breakpoints_dbase_edit_breakpoint (BreakpointsDBase *bd, BreakpointItem *bi)
{
	GladeXML *gxml;
	GtkWidget *dialog;
	GtkWidget *location_label, *location_entry;
	GtkWidget *condition_entry;
	GtkWidget *pass_entry;
	gchar *buff;
	gchar *location = NULL;
	const gchar *uri = NULL;

	gxml = glade_xml_new (GLADE_FILE,
						  "breakpoint_properties_dialog", NULL);
	dialog = glade_xml_get_widget (gxml, "breakpoint_properties_dialog");
	gtk_window_set_transient_for (GTK_WINDOW (dialog),
								  GTK_WINDOW (bd->scrolledwindow));
	location_label = glade_xml_get_widget (gxml, "breakpoint_location_label");
	location_entry = glade_xml_get_widget (gxml, "breakpoint_location_entry");
	condition_entry = glade_xml_get_widget (gxml, "breakpoint_condition_entry");
	pass_entry = glade_xml_get_widget (gxml, "breakpoint_pass_entry");

	
	if (bi == NULL)
	{
		IAnjutaDocumentManager *docman;
		IAnjutaEditor *te;
		guint line = 0;
		
		/* New breakpoint */
		gtk_widget_show (location_entry);
		gtk_widget_hide (location_label);

		/* Get current editor and line */
		docman = get_document_manager (bd->plugin);
		if (docman != NULL)
		{
			te = ianjuta_document_manager_get_current_editor (docman, NULL);
			if (te != NULL)
			{
				uri = ianjuta_file_get_uri (IANJUTA_FILE (te), NULL);
				line = ianjuta_editor_get_lineno (te, NULL);
			}
		}
	
		bi = breakpoint_item_new_from_uri (bd, uri, line, TRUE);
	}
	else
	{
		/* Update breakpoint */
		gtk_widget_hide (location_entry);
		gtk_widget_show (location_label);
	}
		
	if (bi->uri != NULL)
	{
		if (bi->bp->line != 0)
		{
			// file and line
			location = g_strdup_printf ("%s:%d", bi->bp->file, bi->bp->line);
		}
		else
		{
			// file and function
			location = g_strdup_printf ("%s:%s", bi->bp->file, bi->bp->function);
		}
	}	
	else if (bi->bp->address != 0)
	{
		// address
		location = g_strdup_printf ("*%x", bi->bp->address);
	}

	if (GTK_WIDGET_VISIBLE(location_entry))
	{
		gtk_entry_set_text (GTK_ENTRY (location_entry), location == NULL ? "" : location);
	}
	else
	{
		gtk_label_set_text (GTK_LABEL (location_label), location == NULL ? "" : location);
	}
	
	if (bi->bp->condition && strlen (bi->bp->condition) > 0)
		gtk_entry_set_text (GTK_ENTRY (condition_entry), bi->bp->condition);
	
	buff = g_strdup_printf ("%d", bi->bp->ignore);
	gtk_entry_set_text (GTK_ENTRY (pass_entry), buff);
	g_free (buff);
	
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
	{
		const gchar *cond;
		guint ignore;
		const gchar *new_location;
		
		ignore = atoi (gtk_entry_get_text (GTK_ENTRY (pass_entry)));
		cond = gtk_entry_get_text (GTK_ENTRY (condition_entry));
		while (isspace(*cond)) cond++;
		if (*cond == '\0') cond = NULL;

		if (GTK_WIDGET_VISIBLE(location_entry))
		{
			new_location = gtk_entry_get_text (GTK_ENTRY (location_entry));
			while (isspace(*new_location)) new_location++;
				
			if ((location == NULL) || (strcmp (new_location, location) != 0))
			{
				/* location has been changed, create a new breakpoint */
				breakpoint_item_free (bi);
				bi = NULL;
				
				if (*new_location != '\0')
				{
					bi = breakpoint_item_new_from_string (bd, new_location, uri);
				}
			}
		}
		else
		{
			new_location = NULL;
		}

		if (bi != NULL)
		{
			if ((new_location != NULL) || (ignore != bi->bp->ignore) || (cond != bi->bp->condition))
			{
				bi->bp->ignore = ignore;
				if (bi->bp->condition) g_free ((char *)bi->bp->condition);
					bi->bp->condition = cond != NULL ? g_strdup (cond) : NULL;
				breakpoints_dbase_add_breakpoint (bd, bi);
			}
		}
	}
	g_free (location);
	gtk_widget_destroy (dialog);
	g_object_unref (gxml);
}

static GList*
breakpoints_dbase_get_breakpoint_list (BreakpointsDBase *bd)
{
	GList* list = NULL;
	GtkTreeModel *model;
	GtkTreeIter iter;
	
	model = gtk_tree_view_get_model (bd->treeview);
	
	if (gtk_tree_model_get_iter_first (model, &iter))
	{
		do
		{
			BreakpointItem *bi;
		
			gtk_tree_model_get (model, &iter, DATA_COLUMN, &bi, -1);
			
			list = g_list_prepend (list, g_strdup_printf("%d:%s:%u", bi->bp->enable == IANJUTA_DEBUGGER_YES ? 1 : 0, bi->uri, bi->bp->line));
		} while (gtk_tree_model_iter_next (model, &iter));
	}
	
	list = g_list_reverse (list);
	
	return list;
}

static void
on_add_breakpoint_list (gpointer data, gpointer user_data)
{
	BreakpointsDBase* bd = (BreakpointsDBase *)user_data;
	BreakpointItem *bi;
	gchar *uri = (gchar *)data;
	gchar *pos;
	guint line;
	gboolean enable;

	pos = strrchr (uri, ':');
	*pos = '\0';
	line = strtoul (pos + 1, NULL, 10);
	enable = uri[0] == '0' ? FALSE : TRUE;
	bi = breakpoint_item_new_from_uri (bd, uri + 2, line, enable);
		
	breakpoints_dbase_add_breakpoint (bd, bi);
}

static void
breakpoints_dbase_add_breakpoint_list (BreakpointsDBase *bd, GList *list)
{
	g_list_foreach (list, on_add_breakpoint_list, bd);
}

/* Public functions
 *---------------------------------------------------------------------------*/

void
breakpoints_dbase_set_all_in_editor (BreakpointsDBase* bd, IAnjutaEditor* te)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *uri;
	
	DEBUG_PRINT("DMA: set_all_in_editor %p", te);
	
	g_return_if_fail (te != NULL);
	g_return_if_fail (bd != NULL);
	g_return_if_fail (bd->treeview != NULL);
	
	uri = ianjuta_file_get_uri (IANJUTA_FILE (te), NULL);
	if (uri == NULL)
		return;
	
	if (IANJUTA_IS_MARKABLE (te))
	{
		IAnjutaMarkable *ed = IANJUTA_MARKABLE (te);
		ianjuta_markable_delete_all_markers (ed, BREAKPOINT_ENABLED, NULL);
		ianjuta_markable_delete_all_markers (ed, BREAKPOINT_DISABLED, NULL);
	}
	
	model = gtk_tree_view_get_model (bd->treeview);
	
	if (gtk_tree_model_get_iter_first (model, &iter))
	{
		do
		{
			BreakpointItem *bi;
		
			gtk_tree_model_get (model, &iter, DATA_COLUMN, &bi, -1);

			if (strcmp (uri, bi->uri) == 0)
			{
				bi->editor = te;
				set_breakpoint_in_editor (bi, bi->bp->enable == IANJUTA_DEBUGGER_YES ? BREAKPOINT_ENABLED : BREAKPOINT_DISABLED, FALSE);
			}
		} while (gtk_tree_model_iter_next (model, &iter));
	}
	g_free (uri);
}

void
breakpoints_dbase_clear_all_in_editor (BreakpointsDBase* bd, IAnjutaEditor* te)
{
	GtkTreeModel *model;
	GtkTreeIter iter;

	DEBUG_PRINT("DMA: clear_all_in_editor %p", te);
	
	g_return_if_fail (te != NULL);
	g_return_if_fail (bd != NULL);
	g_return_if_fail (bd->treeview != NULL);

	model = gtk_tree_view_get_model (bd->treeview);
	
	if (gtk_tree_model_get_iter_first (model, &iter))
	{
		do
		{
			BreakpointItem *bi;
		
			gtk_tree_model_get (model, &iter, DATA_COLUMN, &bi, -1);

			if (bi->editor == te) bi->editor = NULL;
		} while (gtk_tree_model_iter_next (model, &iter));
	}
}

void
breakpoints_dbase_connect (BreakpointsDBase *bd, IAnjutaDebugger *debugger)
{
	if (bd->debugger != NULL)
	{
		if (bd->debugger == debugger)
		{
			/* Already connected */
			return;
		}
		/* Disconnect from first debugger */
		breakpoints_dbase_disconnect (bd);
	}
		
	g_object_ref (debugger);
	bd->debugger = debugger;
	breakpoints_dbase_add_all_in_debugger (bd);
	g_signal_connect_swapped (bd->debugger, "sharedlib-event",
				  G_CALLBACK (on_breakpoint_sharedlib_event), bd);
}

void
breakpoints_dbase_disconnect (BreakpointsDBase *bd)
{
	if (bd->debugger != NULL)
	{
		breakpoints_dbase_remove_all_in_debugger (bd);
		g_signal_handlers_disconnect_by_func (bd->debugger, G_CALLBACK (on_breakpoint_sharedlib_event), bd);
		g_object_unref (bd->debugger);
		bd->debugger = NULL;
	}
}

/* Callbacks
 *---------------------------------------------------------------------------*/

static gboolean
on_breakpoints_button_press (GtkWidget * widget, GdkEventButton * bevent, BreakpointsDBase *bd)
{
	if (bevent->button == 3)
	{
		gtk_menu_popup (bd->popup, NULL, NULL, NULL, NULL,
						bevent->button, bevent->time);
	}
	
	return FALSE;
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
	
	path = gtk_tree_path_new_from_string (path_str);

	model = gtk_tree_view_get_model (bd->treeview);
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, DATA_COLUMN, &bi, -1);

	if (bi->bp->enable == IANJUTA_DEBUGGER_YES)
	{
		bi->bp->enable = IANJUTA_DEBUGGER_NO;
	}
	else if (bi->bp->enable == IANJUTA_DEBUGGER_NO)
	{
		bi->bp->enable = IANJUTA_DEBUGGER_YES;
	}

	if (bd->debugger != NULL)
	{
		if (bi->bp->enable != IANJUTA_DEBUGGER_UNDEFINED)
		{
			ianjuta_debugger_enable_breakpoint (bd->debugger, bi->bp->id, bi->bp->enable == IANJUTA_DEBUGGER_YES ? TRUE : FALSE, on_breakpoint_item_update_in_ui, bi, NULL);
		}
	}
	else
	{
		/* update ui */
		gtk_list_store_set (GTK_LIST_STORE (model), &bi->iter,
						ENABLED_COLUMN, bi->bp->enable == IANJUTA_DEBUGGER_YES ? TRUE : FALSE,
						-1);
	}
}

static void
on_clear_all_breakpoints (GtkWidget *button, BreakpointsDBase *bd)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new (GTK_WINDOW (ANJUTA_PLUGIN (bd->plugin)->shell), 
									 GTK_DIALOG_DESTROY_WITH_PARENT,
									 GTK_MESSAGE_QUESTION,
									 GTK_BUTTONS_NONE,
					_("Are you sure you want to delete all the breakpoints?"));
	gtk_dialog_add_buttons (GTK_DIALOG (dialog),
							GTK_STOCK_CANCEL, GTK_RESPONSE_NO,
							GTK_STOCK_DELETE, GTK_RESPONSE_YES,
							NULL);
	
	gtk_window_set_transient_for (GTK_WINDOW (dialog),
			GTK_WINDOW (ANJUTA_PLUGIN (bd->plugin)->shell) );

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
	{
		breakpoints_dbase_remove_all (bd);
	}
	gtk_widget_destroy (dialog);
}

static void
on_toggle_breakpoint_activate (GtkAction * action, BreakpointsDBase *bd)
{
	IAnjutaDocumentManager *docman = NULL;
	IAnjutaEditor *te = NULL;
	BreakpointItem *bi;
	const gchar *uri;
	guint line;

	/* Get current editor and line */
	docman = get_document_manager (bd->plugin);
	if (docman == NULL) return;   /* No document manager */
	te = ianjuta_document_manager_get_current_editor (docman, NULL);
	if (te == NULL) return;       /* Missing editor */
	uri = ianjuta_file_get_uri (IANJUTA_FILE (te), NULL);
	line = ianjuta_editor_get_lineno (te, NULL);
	
	/* Find corresponding breakpoint */
	bi = breakpoints_dbase_find_breakpoint (bd, uri, line);

	if (bi == NULL)
	{
		bi = breakpoint_item_new_from_uri (bd, uri, line, TRUE);
		
		breakpoints_dbase_add_breakpoint (bd, bi);
	}
	else
	{
		breakpoints_dbase_remove_breakpoint (bd, bi);
	}
}

static void
on_disable_all_breakpoints_activate (GtkAction * action, BreakpointsDBase *bd)
{
	breakpoints_dbase_enable_all (bd, FALSE);
}

static void
on_clear_all_breakpoints_activate (GtkAction * action, BreakpointsDBase *bd)
{
	on_clear_all_breakpoints (NULL, bd);
}

static void
on_add_breakpoint_activate (GtkAction * action, BreakpointsDBase *bd)
{
	breakpoints_dbase_edit_breakpoint (bd, NULL);
}

static void
on_remove_breakpoint_activate (GtkAction * action, BreakpointsDBase *bd)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	gboolean valid;

	selection =	gtk_tree_view_get_selection (bd->treeview);
	valid = gtk_tree_selection_get_selected (selection, &model, &iter);
	if (valid)
	{
		BreakpointItem *bi;
		
		gtk_tree_model_get (model, &iter, DATA_COLUMN, &bi, -1);
		breakpoints_dbase_remove_breakpoint (bd, bi);
	}
}

static void
on_edit_breakpoint_activate (GtkAction * action, BreakpointsDBase *bd)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	gboolean valid;

	selection =	gtk_tree_view_get_selection (bd->treeview);
	valid = gtk_tree_selection_get_selected (selection, &model, &iter);
	if (valid)
	{
		BreakpointItem *bi;
		
		gtk_tree_model_get (model, &iter, DATA_COLUMN, &bi, -1);
		breakpoints_dbase_edit_breakpoint (bd, bi);
	}
}

static void
on_jump_to_breakpoint_activate (GtkAction * action, BreakpointsDBase *bd)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	gboolean valid;

	selection =	gtk_tree_view_get_selection (bd->treeview);
	valid = gtk_tree_selection_get_selected (selection, &model, &iter);
	if (valid)
	{
		BreakpointItem *bi;
		
		gtk_tree_model_get (model, &iter, DATA_COLUMN, &bi, -1);
		
		goto_location_in_editor (bd->plugin, bi->uri, bi->bp->line);
	}
}

/* Actions table
 *---------------------------------------------------------------------------*/

static GtkActionEntry actions_breakpoints[] = {
	{
		"ActionMenuGdbBreakpoints",               /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("_Breakpoints"),                       /* Display label */
		NULL,                                     /* short-cut */
		NULL,                                     /* Tooltip */
		NULL                                      /* action callback */
	},
	{
		"ActionGdbToggleBreakpoint",              /* Action name */
		"gdb-breakpoint-toggle",                  /* Stock icon, if any */
		N_("Toggle breakpoint"),                  /* Display label */
		NULL,                                     /* short-cut */
		N_("Toggle breakpoint at the current location"), /* Tooltip */
		G_CALLBACK (on_toggle_breakpoint_activate) /* action callback */
	},
	{
		"ActionGdbSetBreakpoint",                 /* Action name */
		"gdb-breakpoint-toggle",                  /* Stock icon, if any */
		N_("Add Breakpoint"),                     /* Display label */
		NULL,                                     /* short-cut */
		N_("Add a breakpoint"),                   /* Tooltip */
		G_CALLBACK (on_add_breakpoint_activate)   /* action callback */
	},
	{
		"ActionGdbClearBreakpoint",               /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("Remove Breakpoint"),                  /* Display label */
		NULL,                                     /* short-cut */
		N_("Remove a breakpoint"),                /* Tooltip */
		G_CALLBACK (on_remove_breakpoint_activate)   /* action callback */
	},
	{
		"ActionGdbJumpToBreakpoint",              /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("Jump to Breakpoint"),                 /* Display label */
		NULL,                                     /* short-cut */
		N_("Jump to breakpoint location"),        /* Tooltip */
		G_CALLBACK (on_jump_to_breakpoint_activate)   /* action callback */
	},
	{
		"ActionGdbEditBreakpoint",                /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("Edit Breakpoint"),                    /* Display label */
		NULL,                                     /* short-cut */
		N_("Edit breakpoint properties"),         /* Tooltip */
		G_CALLBACK (on_edit_breakpoint_activate)   /* action callback */
	},
	{
		"ActionGdbEnableDisableBreakpoint",       /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("Enable Breakpoint"),                  /* Display label */
		NULL,                                     /* short-cut */
		N_("Enable a breakpoint"),                /* Tooltip */
		G_CALLBACK (on_add_breakpoint_activate)   /* action callback */
	},
	{
		"ActionGdbDisableAllBreakpoints",         /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("Disable all Breakpoints"),            /* Display label */
		NULL,                                     /* short-cut */
		N_("Deactivate all breakpoints"),         /* Tooltip */
		G_CALLBACK (on_disable_all_breakpoints_activate)/* action callback */
	},
	{
		"ActionGdbClearAllBreakpoints",           /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("C_lear all Breakpoints"),             /* Display label */
		NULL,                                     /* short-cut */
		N_("Delete all breakpoints"),             /* Tooltip */
		G_CALLBACK (on_clear_all_breakpoints_activate)/* action callback */
	},
};		

/* Callback for saving session
 *---------------------------------------------------------------------------*/

static void
on_session_save (AnjutaShell *shell, AnjutaSessionPhase phase, AnjutaSession *session, BreakpointsDBase *bd)
{
	GList *list;

	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;

	list = breakpoints_dbase_get_breakpoint_list (bd);
	if (list != NULL)
		anjuta_session_set_string_list (session, "Debugger", "Breakpoint", list);
}

static void
on_session_load (AnjutaShell *shell, AnjutaSessionPhase phase, AnjutaSession *session, BreakpointsDBase *bd)
{
	GList *list;

	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;

	/* breakpoints_dbase_remove_all (bd); */
	list = anjuta_session_get_string_list (session, "Debugger", "Breakpoint");
	if (list != NULL)
		breakpoints_dbase_add_breakpoint_list (bd, list);
}


/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

BreakpointsDBase *
breakpoints_dbase_new (AnjutaPlugin *plugin)
{
	BreakpointsDBase *bd;
	AnjutaUI *ui;
	
	bd = g_new0 (BreakpointsDBase, 1);
	
	if (bd) {
		GtkListStore *store;
		GtkCellRenderer *renderer;
		GtkTreeViewColumn *column;
		int i;

		bd->plugin = (DebugManagerPlugin*)plugin; /* FIXME */


		/* Connect to Load and Save event */
		g_signal_connect (plugin->shell, "save-session",
					  G_CALLBACK (on_session_save), bd);
		g_signal_connect (plugin->shell, "load-session",
					  G_CALLBACK (on_session_load), bd);
		
		/* breakpoints dialog */
		store = gtk_list_store_new (COLUMNS_NB,
									G_TYPE_BOOLEAN,
									G_TYPE_UINT,
									G_TYPE_STRING,
									G_TYPE_UINT,
									G_TYPE_STRING,
									G_TYPE_STRING,
									G_TYPE_STRING,
									G_TYPE_UINT,
									G_TYPE_UINT,
									G_TYPE_STRING,
									G_TYPE_STRING,
									G_TYPE_POINTER);
		
		bd->treeview = GTK_TREE_VIEW (gtk_tree_view_new_with_model (GTK_TREE_MODEL (store)));
		gtk_tree_selection_set_mode (gtk_tree_view_get_selection (bd->treeview),
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
		gtk_tree_view_append_column (bd->treeview, column);
		renderer = gtk_cell_renderer_text_new ();

		for (i = NUMBER_COLUMN; i < (COLUMNS_NB - 1); i++)
		{
			column =
				gtk_tree_view_column_new_with_attributes (column_names[i],
													renderer, "text", i, NULL);
			gtk_tree_view_column_set_sizing (column,
											 GTK_TREE_VIEW_COLUMN_AUTOSIZE);
			gtk_tree_view_append_column (bd->treeview, column);
		}

		/* Register menu actions */
		ui = anjuta_shell_get_ui (plugin->shell, NULL);
		bd->action_group =
		anjuta_ui_add_action_group_entries (ui, "ActionGroupBreakpoint",
											_("Breakpoint operations"),
											actions_breakpoints,
											G_N_ELEMENTS (actions_breakpoints),
											GETTEXT_PACKAGE, bd);

		/* Add breakpoint window */
		bd->scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
		gtk_widget_show (bd->scrolledwindow);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (bd->scrolledwindow),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (bd->scrolledwindow),
									 GTK_SHADOW_IN);
		gtk_container_add (GTK_CONTAINER (bd->scrolledwindow), GTK_WIDGET (bd->treeview));
		gtk_widget_show_all (bd->scrolledwindow);
		
		anjuta_shell_add_widget (plugin->shell,
							 bd->scrolledwindow,
                             "AnjutaDebuggerBreakpoints", _("Breakpoints"),
                             "gdb-breakpoint-toggle", ANJUTA_SHELL_PLACEMENT_BOTTOM,
                              NULL);
											
		bd->is_showing = TRUE;
		bd->cond_history = NULL;
		bd->pass_history = NULL;
		bd->loc_history = NULL;

		/* Add popup menu */
		bd->popup =  GTK_MENU (gtk_ui_manager_get_widget (GTK_UI_MANAGER (ui), "/PopupBreakpoint"));
		if (bd->popup)
		{	
			gtk_widget_ref (GTK_WIDGET(bd->popup));
			g_signal_connect (bd->treeview, "button-press-event", G_CALLBACK (on_breakpoints_button_press), bd);  
		}
			
	}

	return bd;
}

void
breakpoints_dbase_destroy (BreakpointsDBase * bd)
{
	AnjutaUI *ui;
	
	g_return_if_fail (bd != NULL);

	/* This is necessary to clear the editor of breakpoint markers */
	breakpoints_dbase_remove_all (bd);

	breakpoints_dbase_disconnect (bd);

	/* Disconnect from Load and Save event */
	g_signal_handlers_disconnect_by_func (ANJUTA_PLUGIN (bd->plugin)->shell, G_CALLBACK (on_session_save), bd);
	g_signal_handlers_disconnect_by_func (ANJUTA_PLUGIN (bd->plugin)->shell, G_CALLBACK (on_session_load), bd);

	/* Remove menu actions */
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (bd->plugin)->shell, NULL);
	anjuta_ui_remove_action_group (ui, bd->action_group);
	
	if (bd->cond_history)
		g_free (bd->cond_history);
	if (bd->pass_history)
		g_free (bd->pass_history);
	if (bd->loc_history)
		g_free (bd->loc_history);

	gtk_widget_destroy (bd->scrolledwindow);
	if (bd->popup) gtk_widget_unref (GTK_WIDGET (bd->popup));
	g_free (bd);
}

