/*
    breakpoints.c
    Copyright (C) 2000  Kh. Naba Kumar Singh

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
#include "anjuta.h"
#include "breakpoints.h"
#include "breakpoints_cbs.h"
#include "utilities.h"
#include "fileselection.h"
#include "resources.h"
#include "controls.h"
#include "utilities.h"
#include "debugger.h"

#define	BKPT_FIELDS	(13)

enum {
	ENABLED_COLUMN,
	FILENAME_COLUMN,
	LINENO_COLUMN,
	FUNCTION_COLUMN,
	PASS_COLUMN,
	CONDITION_COLUMN,
	COLUMNS_NB
};

static char *column_names[COLUMNS_NB] = {
	        N_("Enabled"), N_("File"), N_("Line"), N_("Function"), N_("Pass"), N_("Condition")
};

static void breakpoint_item_save ( BreakpointItem * bi, ProjectDBase * pdb, const gint nBreak );
static void breakpoints_dbase_delete_all_breakpoints (BreakpointsDBase * bd);
static gboolean breakpoint_item_load ( BreakpointItem * bi, gchar* szStr );
static void treeview_enabled_toggled (GtkCellRendererToggle *cell, gchar *path_str, gpointer data);

#define BREAKPOINTS_MARKER 1

BreakpointItem *
breakpoint_item_new ()
{
	BreakpointItem *bi;
	bi = g_malloc (sizeof (BreakpointItem));
	if (bi)
	{
		bi->id = 0;
		bi->enable = FALSE;
		bi->pass = 0;
		bi->condition = NULL;
		bi->file = NULL;
		bi->line = 0;
		bi->function = NULL;
		bi->handle = 0;
		bi->handle_invalid = TRUE;
		bi->info = NULL;
		bi->disp = NULL;
	}
	return bi;
}

void
breakpoint_item_destroy (BreakpointItem * bi)
{
	if (!bi)
		return;

	if (bi->function)
		g_free (bi->function);
	if (bi->file)
		g_free (bi->file);
	if (bi->condition)
		g_free (bi->condition);
	if (bi->info)
		g_free (bi->info);
	if (bi->disp)
		g_free (bi->disp);

	g_free (bi);
}


static gint
breakpoint_item_calc_size( BreakpointItem *bi )
{
	g_return_val_if_fail( bi != NULL, 0 );
	
	return 

	+calc_gnum_len( /*bi->id*/ )
	+calc_string_len( bi->disp )
	+calc_gnum_len( /*bi->enable*/ )
	+calc_gnum_len( /*bi->addr*/ )
	+calc_gnum_len( /*bi->pass*/ )
	+calc_string_len( bi->condition )
	+calc_string_len( bi->file )

	+calc_gnum_len( /*bi->line*/ )
	+calc_gnum_len( /*bi->handle*/ )	
	+calc_gnum_len( /*bi->handle_invalid*/ )	
	+calc_string_len( bi->function )	
	+calc_string_len( bi->info )
	+calc_gnum_len( /*bi->time*/ )	;
}

/* The saving format is a single string comma separated */
static void
breakpoint_item_save ( BreakpointItem * bi, ProjectDBase * pdb, const gint nBreak )
{
	gint	nSize ;
	gchar	*szStrSave, *szDst ;
	
	g_return_if_fail( bi != NULL );
	g_return_if_fail( pdb != NULL );

	nSize = breakpoint_item_calc_size( bi );
	szStrSave = g_malloc( nSize );
	if( NULL == szStrSave )
		return ;
	szDst = szStrSave ;
	/* Writes the fields to the string */
	szDst = WriteBufI( szDst, bi->id );
	szDst = WriteBufS( szDst, bi->disp );
	szDst = WriteBufB( szDst, bi->enable );
	szDst = WriteBufUL( szDst, bi->addr );
	szDst = WriteBufI( szDst, bi->pass );
	szDst = WriteBufS( szDst, bi->condition );
	szDst = WriteBufS( szDst, bi->file );	
	szDst = WriteBufI( szDst, bi->line );
	szDst = WriteBufI( szDst, bi->handle );
	szDst = WriteBufB( szDst, bi->handle_invalid );
	szDst = WriteBufS( szDst, bi->function );	
	szDst = WriteBufS( szDst, bi->info );	
	szDst = WriteBufUL( szDst, (gulong)bi->time );

	session_save_string_n( pdb, SECSTR(SECTION_BREAKPOINTS), nBreak, szStrSave );

	g_free( szStrSave );
};

#define	ASS_STR(x,nItem)	do{ if (NULL == ( bi->x = GetStrCod( p[nItem] ) ) ){ goto fine;} }while(0)

static gboolean
breakpoint_item_load ( BreakpointItem * bi, gchar* szStr )
{
	gchar		**p;
	gboolean	bOK = FALSE ;
	
	g_return_val_if_fail( bi != NULL, FALSE );
	
	p = PARSE_STR(BKPT_FIELDS,szStr);
	if( NULL == p )
		return FALSE ;
	bi->id	= atoi( p[0] );
	ASS_STR(disp ,1);
	bi->enable	= atoi( p[2] ) ? TRUE : FALSE ;
	bi->addr	= atol( p[3] );
	bi->pass	= atol( p[4] );
	ASS_STR(condition ,5);
	ASS_STR(file ,6);
	bi->line	= atoi( p[7] );
	bi->handle	= atoi( p[8] );
	bi->handle_invalid	= atoi( p[9] ) ? TRUE : FALSE ;
	ASS_STR(function ,10);
	ASS_STR(info ,11);
	bi->time	=  atol( p[12] );
	bOK = TRUE ;
fine:
	g_free(p);
	return bOK ;
};

static void
treeview_enabled_toggled (GtkCellRendererToggle *cell,
			  gchar			*path_str,
			  gpointer		 data)
{
	BreakpointsDBase *bd;
	BreakpointItem *bi;
	GtkTreeModel *model = GTK_TREE_MODEL (data);
	GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
	GtkTreeIter iter;
	gboolean state;

	bd = (BreakpointsDBase *) data;
	bi = g_list_nth_data (bd->breakpoints, bd->current_index);
	bi->enable ^= 1;

	if (!bi->enable)
		debugger_disable_breakpoint (bi->id);
	else
		debugger_enable_breakpoint (bi->id);

	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, ENABLED_COLUMN, &state, -1);

	state ^= 1;

	gtk_tree_store_set (GTK_TREE_STORE (model), &iter, ENABLED_COLUMN, state, -1);
}			  

BreakpointsDBase *
breakpoints_dbase_new ()
{
	BreakpointsDBase *bd;

	bd = g_new0 (BreakpointsDBase, 1);

	if (bd) {
		GtkTreeView *view;
		GtkTreeStore *store;
		GtkCellRenderer *renderer;
		GtkTreeViewColumn *column;
		struct BkItemData *data;
		int i;

		/* breakpoints dialog */
		bd->gxml_prop = glade_xml_new (GLADE_FILE_ANJUTA, "beakpoints_properties_dialog", NULL);
		glade_xml_signal_autoconnect (bd->gxml_prop);
		gtk_widget_hide (glade_xml_get_widget (bd->gxml_prop, "breakpoints_properties_dialog"));
		
		bd->gxml = glade_xml_new (GLADE_FILE_ANJUTA, "beakpoints_dialog", NULL);
		glade_xml_signal_autoconnect (bd->gxml);
		
		bd->widgets.window = glade_xml_get_widget (bd->gxml, "breakpoints_dialog");
		gtk_widget_hide (bd->widgets.window);
		bd->widgets.treeview = glade_xml_get_widget (bd->gxml, "breakpoints_tv");
		bd->widgets.remove_button = glade_xml_get_widget (bd->gxml, "breakpoints_remove_button");
		bd->widgets.jumpto_button = glade_xml_get_widget (bd->gxml, "breakpoints_jumpto_button");
		bd->widgets.properties_button = glade_xml_get_widget (bd->gxml, "breakpoints_properties_button");
		bd->widgets.add_button = glade_xml_get_widget (bd->gxml, "breakpoints_add_button");
		bd->widgets.removeall_button = glade_xml_get_widget (bd->gxml, "breakpoints_removeall_button");
		bd->widgets.enableall_button = glade_xml_get_widget (bd->gxml, "breakpoints_enableall_button");
		bd->widgets.disableall_button = glade_xml_get_widget (bd->gxml, "breakpoints_disableall_button");

 		gtk_widget_ref (bd->widgets.window);
		gtk_widget_ref (bd->widgets.treeview);
		gtk_widget_ref (bd->widgets.remove_button);
		gtk_widget_ref (bd->widgets.jumpto_button);
		gtk_widget_ref (bd->widgets.properties_button);
		gtk_widget_ref (bd->widgets.add_button);
		gtk_widget_ref (bd->widgets.removeall_button);
		gtk_widget_ref (bd->widgets.enableall_button);
		gtk_widget_ref (bd->widgets.disableall_button);

		view = GTK_TREE_VIEW (bd->widgets.treeview);
		store = gtk_tree_store_new (COLUMNS_NB,
					    G_TYPE_BOOLEAN,
					    G_TYPE_STRING,
					    G_TYPE_STRING,
					    G_TYPE_STRING,
					    G_TYPE_STRING,
					    G_TYPE_STRING);
		gtk_tree_view_set_model (view, GTK_TREE_MODEL (store));
		gtk_tree_selection_set_mode (gtk_tree_view_get_selection (view),
					     GTK_SELECTION_SINGLE);
		g_object_unref (G_OBJECT (store));

		renderer = gtk_cell_renderer_toggle_new ();
		column = gtk_tree_view_column_new_with_attributes (column_names[0], renderer,
								   "active", ENABLED_COLUMN,
								   NULL);
		g_signal_connect (renderer, "toggled", G_CALLBACK (treeview_enabled_toggled), bd);
		gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
		gtk_tree_view_append_column (view, column);

		renderer = gtk_cell_renderer_text_new ();

		for (i = FILENAME_COLUMN; i < COLUMNS_NB; i++) {
			column = gtk_tree_view_column_new_with_attributes (column_names[i], renderer, "text", i, NULL);
			gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
			gtk_tree_view_append_column (view, column);
		}

		g_signal_connect (G_OBJECT (bd->widgets.remove_button), "clicked",
				  G_CALLBACK (on_bk_remove_clicked), bd);
		g_signal_connect (G_OBJECT (bd->widgets.jumpto_button), "clicked",
				  G_CALLBACK (on_bk_jumpto_clicked), bd);
		g_signal_connect (G_OBJECT (bd->widgets.properties_button), "clicked",
				  G_CALLBACK (on_bk_properties_clicked), bd);
		g_signal_connect (G_OBJECT (bd->widgets.add_button), "clicked",
				  G_CALLBACK (on_bk_add_clicked), bd);
		g_signal_connect (G_OBJECT (bd->widgets.removeall_button), "clicked",
				  G_CALLBACK (on_bk_removeall_clicked), bd);
		g_signal_connect (G_OBJECT (bd->widgets.enableall_button), "clicked",
				  G_CALLBACK (on_bk_enableall_clicked), bd);
		g_signal_connect (G_OBJECT (bd->widgets.disableall_button), "clicked",
				  G_CALLBACK (on_bk_disableall_clicked), bd);

		g_signal_connect (G_OBJECT (bd->widgets.treeview), "event",
				  G_CALLBACK (on_bk_treeview_event), NULL);

		g_signal_connect (G_OBJECT (bd->widgets.window), "close",
				  G_CALLBACK (breakpoints_dbase_hide), bd);

		bd->breakpoints = NULL;
		bd->cond_history = NULL;
		bd->pass_history = NULL;
		bd->loc_history = NULL;
		bd->current_index = -1;
		bd->edit_index = -1;
		bd->is_showing = FALSE;
		bd->is_docked = FALSE;
		bd->win_pos_x = 50;
		bd->win_pos_y = 50;
		bd->win_width = 500;
		bd->win_height = 300;
	}

	return bd;
}

void
breakpoints_dbase_destroy (BreakpointsDBase * bd)
{
	BreakpointItem *bi;
	gint i;

	if (bd) {
		for (i = 0; i < g_list_length (bd->breakpoints); i++) {
			bi = g_list_nth_data (bd->breakpoints, i);
			breakpoint_item_destroy (bi);
		}

		g_list_free (bd->breakpoints);

		if (bd->cond_history)
			g_free (bd->cond_history);
		if (bd->pass_history)
			g_free (bd->pass_history);
		if (bd->loc_history)
			g_free (bd->loc_history);

		gtk_widget_unref (bd->widgets.window);
		gtk_widget_unref (bd->widgets.treeview);
		gtk_widget_unref (bd->widgets.remove_button);
		gtk_widget_unref (bd->widgets.jumpto_button);
		gtk_widget_unref (bd->widgets.properties_button);
		gtk_widget_unref (bd->widgets.add_button);
		gtk_widget_unref (bd->widgets.removeall_button);
		gtk_widget_unref (bd->widgets.enableall_button);
		gtk_widget_unref (bd->widgets.disableall_button);

		gtk_widget_destroy (bd->widgets.window);
		g_object_unref (bd->gxml);
		g_object_unref (bd->gxml_prop);
		g_free (bd);
	}
}

void
breakpoints_dbase_save (BreakpointsDBase * bd, ProjectDBase * pdb )
{
	gint	i;
	gint	nLen ;

	g_return_if_fail (bd != NULL);
	g_return_if_fail (pdb != NULL);
	
	session_clear_section( pdb, SECSTR(SECTION_BREAKPOINTS) );

	nLen = g_list_length (bd->breakpoints) ;
	for (i = 0; i < nLen ; i++)
	{
		breakpoint_item_save ((BreakpointItem *)
						g_list_nth_data (bd->breakpoints, i),
						pdb, i );
	}
}


void
breakpoints_dbase_load (BreakpointsDBase * bd, ProjectDBase *p )
{
	gpointer	config_iterator;
	guint		loaded = 0;

	g_return_if_fail( p != NULL );
	breakpoints_dbase_clear(bd);
	config_iterator = session_get_iterator( p, SECSTR(SECTION_BREAKPOINTS) );
	if ( config_iterator !=  NULL )
	{
		gchar * szItem, *szData;
		loaded = 0;
		while ((config_iterator = gnome_config_iterator_next( config_iterator,
									&szItem, &szData )))
		{
			// shouldn't happen, but I'm paranoid
			if( NULL != szData )
			{
				gboolean	bToDel = TRUE ;
				BreakpointItem *bi = breakpoint_item_new();
				if( NULL != bi )
				{
					if( breakpoint_item_load ( bi, szData ) )
					{
						g_list_append (bd->breakpoints, (gpointer) bi);
						bToDel = FALSE ;
					}
				}
				if( bToDel && bi )
					breakpoint_item_destroy(bi);
			}
			loaded ++ ;
			g_free( szItem );
			g_free( szData );
		}
	}
}

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
breakpoints_dbase_clear (BreakpointsDBase * bd)
{
	g_return_if_fail (bd != NULL);

	breakpoints_dbase_delete_all_breakpoints (bd);

	if (bd->breakpoints)
		g_list_free (bd->breakpoints);

	bd->breakpoints = NULL;

	if (bd->widgets.treeview) {
		GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (bd->widgets.treeview));
	        gtk_tree_store_clear (GTK_TREE_STORE (model));
	}

	bd->current_index = -1;

	anjuta_delete_all_marker (BREAKPOINTS_MARKER);
}

static void
breakpoints_dbase_delete_all_breakpoints (BreakpointsDBase * bd)
{
	gint i;
	g_return_if_fail (bd != NULL);
	for (i = 0; i < g_list_length (bd->breakpoints); i++)
		breakpoint_item_destroy ((BreakpointItem *)
					 g_list_nth_data (bd->breakpoints,
							  i));
}

void
breakpoints_dbase_show (BreakpointsDBase * bd)
{
	g_return_if_fail (bd != NULL);
	if (bd->is_showing)
	{
		if (bd->is_docked == FALSE)
			gdk_window_raise (bd->widgets.window->window);
		return;
	}
	if (bd->is_docked)
	{
		breakpoints_dbase_attach (bd);
	}
	else		/* Is not docked */
	{
		gtk_widget_set_uposition (bd->widgets.window,
					  bd->win_pos_x,
					  bd->win_pos_y);
		gtk_window_set_default_size (GTK_WINDOW
					     (bd->widgets.window),
					     bd->win_width,
					     bd->win_height);
		gtk_widget_show (bd->widgets.window);
	}
	bd->is_showing = TRUE;
}

void
breakpoints_dbase_hide (BreakpointsDBase * bd)
{
	g_return_if_fail (bd != NULL);
	if (bd->is_showing == FALSE)
		return;
	if (bd->is_docked == TRUE)
	{
		breakpoints_dbase_detach (bd);
	}
	else		/* Is not docked */
	{
		gdk_window_get_root_origin (bd->widgets.window->
					    window, &bd->win_pos_x,
					    &bd->win_pos_y);
		gdk_window_get_size (bd->widgets.window->window,
				     &bd->win_width, &bd->win_height);
	}
	bd->is_showing = FALSE;
}

void
breakpoints_dbase_update (GList * outputs, gpointer data)
{
	BreakpointsDBase *bd;
	gchar *ptr;
	GList *list, *node;

	bd = (BreakpointsDBase *) data;

	list = remove_blank_lines (outputs);
	breakpoints_dbase_clear (debugger.breakpoints_dbase);
	if (g_list_length (list) < 2)
	{
		g_list_free (list);
		return;
	}
	if (!strcmp ((gchar *) list->data, "No breakpoints or watchpoints"))
	{
		g_list_free (list);
		return;
	}

	ptr = g_strconcat ((gchar *) list->next->data, "\n", NULL);
	node = list->next->next;
	while (node)
	{
		gchar *line = (gchar *) node->data;
		node = node->next;
		if (isspace (line[0]))	/* line break */
		{
			gchar *tmp;
			tmp = ptr;
			ptr = g_strconcat (tmp, line, "\n", NULL);
			g_free (tmp);
		}
		else
		{
			breakpoints_dbase_add_brkpnt (bd, ptr);
			g_free (ptr);
			ptr = g_strconcat (line, "\n", NULL);
		}
	}
	if (ptr)
	{
		breakpoints_dbase_add_brkpnt (bd, ptr);
		g_free (ptr);
		ptr = NULL;
	}
	breakpoints_dbase_update_controls (bd);
	g_list_free (list);
}

void
breakpoints_dbase_set_all (BreakpointsDBase * bd)
{
	gint i, ret;
	struct stat st;
	BreakpointItem *bi;
	gchar *fn, *buff;
	gboolean old = FALSE;
	if (g_list_length (bd->breakpoints) < 1)
		return;
	for (i = 0; i < g_list_length (bd->breakpoints); i++)
	{
		gboolean disable;
		disable = FALSE;
		bi = g_list_nth_data (bd->breakpoints, i);
		fn = anjuta_get_full_filename (bi->file);
		ret = stat (fn, &st);
		g_free (fn);
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
			buff =
				g_strdup_printf ("break %s:%u if %s",
						 bi->file, bi->line,
						 bi->condition);
			debugger_put_cmd_in_queqe (buff, DB_CMD_NONE, NULL,
						   NULL);
			g_free (buff);
		}
		else
		{
			buff =
				g_strdup_printf ("break %s:%u", bi->file,
						 bi->line);
			debugger_put_cmd_in_queqe (buff, DB_CMD_NONE, NULL,
						   NULL);
			g_free (buff);
		}
		if (bi->pass > 0)
		{
			buff = g_strdup_printf ("ignore $bpnum %d", bi->pass);
			debugger_put_cmd_in_queqe (buff, DB_CMD_NONE, NULL,
						   NULL);
			g_free (buff);
		}
		if (disable)
		{
			buff = g_strdup_printf ("disable $bpnum");
			debugger_put_cmd_in_queqe (buff, DB_CMD_NONE, NULL,
						   NULL);
			g_free (buff);
			disable = FALSE;
		}
	}
	if (old)
		anjuta_warning (_("Old breakpoints disabled."));
	debugger_put_cmd_in_queqe ("info breakpoints", DB_CMD_NONE,
				   breakpoints_dbase_update,
				   debugger.breakpoints_dbase);
}

void
breakpoints_dbase_attach (BreakpointsDBase * bd)
{
/* TODO ?? */
}

void
breakpoints_dbase_detach (BreakpointsDBase * bd)
{
/* TODO ?? */
}

void
breakpoints_dbase_dock (BreakpointsDBase * bd)
{
/* TODO ?? */
}

void
breakpoints_dbase_undock (BreakpointsDBase * bd)
{
	/* TODO ?? */
}

gboolean
breakpoints_dbase_save_yourself (BreakpointsDBase * bd, FILE * stream)
{
	g_return_val_if_fail (bd != NULL, FALSE);

	fprintf (stream, "breakpoints.is.docked=%d\n", bd->is_docked);
	if (bd->is_showing && !bd->is_docked)
	{
		gdk_window_get_root_origin (bd->widgets.window->window,
					    &bd->win_pos_x, &bd->win_pos_y);
		gdk_window_get_size (bd->widgets.window->window,
				     &bd->win_width, &bd->win_height);
	}
	fprintf (stream, "breakpoints.win.pos.x=%d\n", bd->win_pos_x);
	fprintf (stream, "breakpoints.win.pos.y=%d\n", bd->win_pos_y);
	fprintf (stream, "breakpoints.win.width=%d\n", bd->win_width);
	fprintf (stream, "breakpoints.win.height=%d\n", bd->win_height);
	return TRUE;
}

gboolean
breakpoints_dbase_load_yourself (BreakpointsDBase * bd, PropsID props)
{
	gboolean dock_flag;

	g_return_val_if_fail (bd != NULL, FALSE);
	dock_flag = prop_get_int (props, "breakpoints.is.docked", 0);
	bd->win_pos_x = prop_get_int (props, "breakpoints.win.pos.x", 50);
	bd->win_pos_y = prop_get_int (props, "breakpoints.win.pos.y", 50);
	bd->win_width = prop_get_int (props, "breakpoints.win.width", 500);
	bd->win_height = prop_get_int (props, "breakpoints.win.height", 300);
	if (dock_flag)
		breakpoints_dbase_dock (bd);
	else
		breakpoints_dbase_undock (bd);
	return TRUE;
}


/**********************************
 * Private functions: Do not use  *
 **********************************/
void
breakpoints_dbase_update_controls (BreakpointsDBase * bd)
{
	gboolean A, R, C, S;

	A = debugger_is_active ();
	R = debugger_is_ready ();
	C = (g_list_length (bd->breakpoints) > 0);
	S = (bd->current_index >= 0);

	gtk_widget_set_sensitive (bd->widgets.remove_button, A && R && C && S);
	gtk_widget_set_sensitive (bd->widgets.jumpto_button, C && S);
	gtk_widget_set_sensitive (bd->widgets.properties_button, A && R && C && S);

	gtk_widget_set_sensitive (bd->widgets.add_button, A && R);
	gtk_widget_set_sensitive (bd->widgets.removeall_button, A && R);
	gtk_widget_set_sensitive (bd->widgets.enableall_button, A && R);
	gtk_widget_set_sensitive (bd->widgets.disableall_button, A && R);
}

void
breakpoints_dbase_add_brkpnt (BreakpointsDBase * bd, gchar * brkpnt)
{
	BreakpointItem *bi;
	GList *node;
	gchar* full_fname = NULL;
	gchar brkno[10];
	gchar function[256];
	gchar fileln[512];
	gchar file[512];
	gchar line[10];
	gchar ignore[10];
	gchar enb[5];
	gchar cond[512];
	gchar *ptr;
	glong count;

	g_return_if_fail (bd != NULL);

	/* this should not happen, abort */
	if (isspace (brkpnt[0]))
		return;

	/* only breakpoints, no watchpoints */
	if (strstr (brkpnt, "watchpoint"))
		return;

	count = sscanf (brkpnt, "%s %*s %*s %s %*s in %s at %s", brkno, enb, function, fileln);

	if (count == 4 || count == 2) {
		GtkTreeStore *store;
		GtkTreeIter iter;

		if (count == 4)
		{
			/* get file and line no */
			ptr = strchr (fileln, ':');
			ptr++;
			strcpy (line, ptr);
			ptr--;
			*ptr = '\0';
			strcpy (file, fileln);
		}
		else
		{
			strcpy (file, "??");
			strcpy (line, "??");
			strcpy (function, "??");
		}

		/* add breakpoint to list */

		bi = breakpoint_item_new ();
		bi->file = g_strdup (file);

		if (count == 4)
			bi->line = atoi (line);
		else
			bi->line = -1;

		bi->function = g_strdup (function);
		bi->id = atoi (brkno);

		if (strcmp (enb, "y") == 0)
			bi->enable = TRUE;
		else
			bi->enable = FALSE;

		if ((ptr = strstr (brkpnt, "ignore")))
		{
			sscanf (ptr, "ignore next %s", ignore);
			bi->pass = atoi (ignore);
		}
		else
		{
			strcpy (ignore, _("0"));
			bi->pass = 0;
		}
		if ((ptr = strstr (brkpnt, "stop only if ")))
		{
			gint i = 0;
			ptr += strlen ("stop only if ");
			while (*ptr != '\n' && *ptr != '\0')
			{
				cond[i] = *ptr++;
				i++;
			}
			cond[i] = '\0';
			bi->condition = g_strdup (cond);
		}
		else
		{
			strcpy (cond, "");
			bi->condition = NULL;
		}

/*		full_fname = anjuta_get_full_filename (bi->file); */

		for (node = app->text_editor_list; node; node = g_list_next (node)) {
			TextEditor* te;

			te = node->data;

			if (te->full_filename == NULL) {
				node = g_list_next (node);
				continue;
			}

			if (strcmp (te->filename, bi->file) == 0) {
				bi->handle = text_editor_set_marker (te, bi->line, BREAKPOINTS_MARKER);
				bi->handle_invalid = FALSE;
				break;
			}
		}

		if (full_fname)
			g_free (full_fname);

		bi->time = time (NULL);

		bd->breakpoints = g_list_append (bd->breakpoints, (gpointer) bi);

		store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (bd->widgets.treeview)));
		gtk_tree_store_append (store, &iter, NULL);
		gtk_tree_store_set (store, &iter,
				    ENABLED_COLUMN, bi->enable,
				    FILENAME_COLUMN, file,
				    LINENO_COLUMN, line,
				    FUNCTION_COLUMN, function,
				    PASS_COLUMN, ignore,
				    CONDITION_COLUMN, cond,
				    -1);
	}
}

void
breakpoints_dbase_set_all_in_editor (BreakpointsDBase* bd, TextEditor* te)
{
	GList *node;
	
	g_return_if_fail ( te != NULL);
	g_return_if_fail ( bd != NULL);
	if (te->full_filename == NULL)
		return;
	
	node = bd->breakpoints;
	while (node)
	{
		BreakpointItem* bi;
		gchar* full_fname = NULL;
		
		bi = node->data;
		if (bi->line < 0 || bi->handle_invalid == FALSE)
			break;
/*
		full_fname = anjuta_get_full_filename (bi->file);
		if (strcmp (te->full_filename, full_fname) == 0)
*/
		if (strcmp (te->filename, bi->file) == 0)
		{
			bi->handle = text_editor_set_marker (te, bi->line, BREAKPOINTS_MARKER);
			bi->handle_invalid = FALSE;
		}
		if (full_fname) g_free (full_fname);
		node = g_list_next (node);
	}
}

void
breakpoints_dbase_clear_all_in_editor (BreakpointsDBase* bd, TextEditor* te)
{
	GList *node;
	
	g_return_if_fail ( te != NULL);
	g_return_if_fail ( bd != NULL);
	if (te->full_filename == NULL)
		return;
	node = bd->breakpoints;
	while (node)
	{
		BreakpointItem* bi;
		gchar* full_fname;
		
		bi = node->data;
		if (bi->handle_invalid == TRUE)
			break;
		full_fname = anjuta_get_full_filename (bi->file);
		if (strcmp (te->full_filename, full_fname) == 0)
		{
			bi->handle_invalid = TRUE;
		}
		if (full_fname) g_free (full_fname);
		node = g_list_next (node);
	}
}

void
breakpoints_dbase_toggle_breakpoint (BreakpointsDBase* b)
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
	if (text_editor_is_marker_set (te, line, BREAKPOINTS_MARKER))
	{
		/* Breakpoint is set. So, delete it. */
		GList *node;
		
		node = b->breakpoints;
		while (node)
		{
			
			BreakpointItem* item;
			gint moved_line;
			
			item = node->data;
			if (item->handle_invalid)
			{
				node = g_list_next (node);
				continue;
			}
			if (strcmp (te->filename, item->file) != 0)
			{
				node = g_list_next (node);
				continue;
			}
			moved_line = text_editor_line_from_handle(te, item->handle);
			if (moved_line == line && moved_line >= 0)
			{
				text_editor_delete_marker (te, line, BREAKPOINTS_MARKER);
				debugger_delete_breakpoint (item->id);
				return;
			}
			node = g_list_next (node);
		}
		g_warning (_("Breakpoint is set, but the ID could not be found."));
		return;
	}
	
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
