/*
    text_editor_menu.c
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

#include <gnome.h>


#include "resources.h"
#include "text_editor_menu.h"
#include "anjuta.h"
#include "mainmenu_callbacks.h"
#include "toolbar_callbacks.h"
#include "debugger.h"
#include "global.h"

static void
on_tem_toggle_linenum_activate (GtkWidget* menuitem, gpointer data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.view.editor_linenos),
				 "activate");
}

static void
on_tem_toggle_marker_margin_activate (GtkWidget* menuitem, gpointer data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.view.editor_markers),
				 "activate");
}

static void
on_tem_toggle_code_fold_activate (GtkWidget* menuitem, gpointer data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.view.editor_folds),
				 "activate");
}

static void
on_tem_toggle_guides_activate (GtkWidget* menuitem, gpointer data)
{
	gtk_signal_emit_by_name (GTK_OBJECT
				 (app->widgets.menubar.view.editor_indentguides),
				 "activate");
}

GnomeUIInfo text_editor_menu_goto_submenu_uiinfo[] = {
	{
	 GNOME_APP_UI_ITEM, N_("Back"),
	 NULL,
	 on_go_back_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
 	 0, 0, NULL}
	 ,
	{
	 GNOME_APP_UI_ITEM, N_("Forward"),
	 NULL,
	 on_go_forward_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
 	 0, 0, NULL}
	 ,
	{
	 GNOME_APP_UI_ITEM, N_("History"),
	 NULL,
	 on_history_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
 	 0, 0, NULL}
	 ,
	 GNOMEUIINFO_SEPARATOR
	 ,
	{
	 GNOME_APP_UI_ITEM, N_("Symbol Definition"),
	 NULL,
	 on_goto_tag_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
 	 0, 0, NULL}
	 ,
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Prev mesg"),
	 NULL,
	 on_goto_prev_mesg1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Next mesg"),
	 NULL,
	 on_goto_next_mesg1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Prev bookmark"),
	 NULL,
	 on_book_prev1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Next bookmark"),
	 NULL,
	 on_book_next1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_END
};

GnomeUIInfo text_editor_menu_debug_submenu_uiinfo[] = {
	{
	 GNOME_APP_UI_ITEM, N_("Toggle breakpoint"),
	 NULL,
	 on_toggle_breakpoint1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Step in"),
	 NULL,
	 on_execution_step_in1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Step over"),
	 NULL,
	 on_execution_step_over1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Step out"),
	 NULL,
	 on_execution_step_out1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Run to cursor"),
	 NULL,
	 on_execution_run_to_cursor1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Run/Continue"),
	 NULL,
	 on_execution_continue1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Inspect"),
	 NULL,
	 on_debugger_inspect_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Interrupt"),
	 NULL,
	 on_debugger_interrupt_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_END
};

GnomeUIInfo text_editor_menu_options_submenu_uiinfo[] = {
	{
	 GNOME_APP_UI_ITEM, N_("Toggle Line numbers"),
	 NULL,
	 on_tem_toggle_linenum_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL}
	,
	{
	 GNOME_APP_UI_ITEM, N_("Toggle Marker Margin"),
	 NULL,
	 on_tem_toggle_marker_margin_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL}
	,
	{
	 GNOME_APP_UI_ITEM, N_("Toggle Fold Margin"),
	 NULL,
	 on_tem_toggle_code_fold_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL}
	,
	{
	 GNOME_APP_UI_ITEM, N_("Toggle Guides"),
	 NULL,
	 on_tem_toggle_guides_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL}
	,
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("++Zoom"),
	 NULL,
	 on_zoom_text_plus_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL}
	,
	{
	 GNOME_APP_UI_ITEM, N_("--Zoom"),
	 NULL,
	 on_zoom_text_minus_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL}
	,
	GNOMEUIINFO_END
};

GnomeUIInfo text_editor_menu_uiinfo[] = {
	{
	 /* 0 */
	 GNOME_APP_UI_ITEM, N_("Cut"),
	 NULL,
	 on_cut1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL}
	,
	{
	 /* 1 */
	 GNOME_APP_UI_ITEM, N_("Copy"),
	 NULL,
	 on_copy1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL}
	,
	{
	 /* 2 */
	 GNOME_APP_UI_ITEM, N_("Paste"),
	 NULL,
	 on_paste1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 GDK_v, GDK_CONTROL_MASK, NULL}
	,
	/* 3 */
	GNOMEUIINFO_SEPARATOR,
	{
	 /* 4 */
	 GNOME_APP_UI_ITEM, N_("Context Help"),
	 NULL,
	 on_context_help_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
 	 0, 0, NULL}
	 ,
	/* 5 */
	GNOMEUIINFO_SEPARATOR,
	{
	 /* 6 */
	 GNOME_APP_UI_ITEM, N_("Toggle Bookmark"),
	 NULL,
	 on_book_toggle1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL}
	,
	{
	 /* 7 */
	 GNOME_APP_UI_ITEM, N_("Auto format"),
	 NULL,
	 on_indent1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL}
	,
	{
	 /* 8 */
	 GNOME_APP_UI_ITEM, N_("Swap .h/.c"),
	 NULL,
	 on_text_editor_menu_swap_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL}
	,
	 /* 9 */
	GNOMEUIINFO_SEPARATOR,
	{
	 /* 10 */
	 GNOME_APP_UI_SUBTREE, N_("Go"),
	 NULL,
	 text_editor_menu_goto_submenu_uiinfo, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL}
	,
	{
	 /* 11 */
	 GNOME_APP_UI_ITEM, N_("Function"),
	 NULL,
	 NULL, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL}
	,
	 /* 12 */
	GNOMEUIINFO_SEPARATOR,
	{
	 /* 13 */
	 GNOME_APP_UI_SUBTREE, N_("Debug"),
	 NULL,
	 text_editor_menu_debug_submenu_uiinfo, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL}
	,
	 /* 14 */
	GNOMEUIINFO_SEPARATOR,
	{
	 /* 15 */
	 GNOME_APP_UI_SUBTREE, N_("Options"),
	 NULL,
	 text_editor_menu_options_submenu_uiinfo, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL}
	,
	 /* 16 */
	GNOMEUIINFO_SEPARATOR,
	{
	 /* 17 */
	 GNOME_APP_UI_ITEM, N_("Close"),
	 NULL,
	 on_close_file1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL}
	,
	 /* 18 */
	GNOMEUIINFO_END
};

TextEditorMenu *
text_editor_menu_new ()
{
	TextEditorMenu *tm;
	tm = (TextEditorMenu *) malloc (sizeof (TextEditorMenu));
	if (tm == NULL)
		return NULL;
	create_text_editor_menu_gui (tm);
	return tm;
}

void
text_editor_menu_destroy (TextEditorMenu * tm)
{
	if (tm)
	{
		gtk_widget_unref (tm->GUI);
		gtk_widget_unref (tm->copy);
		gtk_widget_unref (tm->cut);
		gtk_widget_unref (tm->autoformat);
		gtk_widget_unref (tm->swap);
		gtk_widget_unref (tm->functions);
		gtk_widget_unref (tm->debug);
		if (tm->GUI)
			gtk_widget_destroy (tm->GUI);
		free (tm);
		tm = NULL;
	}
}

void
text_editor_menu_popup (TextEditorMenu * menu, GdkEventButton * bevent)
{
	GList *funcs;
	GtkWidget *submenu;
	TextEditor *te;
	gboolean A;
	
	g_return_if_fail (menu != NULL);

	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;

	A = debugger_is_active ();

	funcs = anjuta_get_function_list(te);
	if (funcs)
	{
		submenu =
			create_submenu (_("Functions "), funcs,
					GTK_SIGNAL_FUNC
					(on_text_editor_menu_function_activate));
		gtk_menu_item_set_submenu (GTK_MENU_ITEM
					   (menu->functions), submenu);
	}
	else
		gtk_widget_set_sensitive (menu->functions, FALSE);
	if (text_editor_has_selection (te))
	{
		gtk_widget_set_sensitive (menu->copy, TRUE);
		gtk_widget_set_sensitive (menu->cut, TRUE);
	}
	else
	{
		gtk_widget_set_sensitive (menu->copy, FALSE);
		gtk_widget_set_sensitive (menu->cut, FALSE);
	}
	gtk_widget_set_sensitive (menu->debug, A);
	
	gtk_menu_popup (GTK_MENU (menu->GUI), NULL, NULL, NULL, NULL,
			bevent->button, bevent->time);
}

void
create_text_editor_menu_gui (TextEditorMenu * menu)
{
	if (menu)
	{
		GtkWidget *text_editor_menu;

		text_editor_menu = gtk_menu_new ();
		gnome_app_fill_menu (GTK_MENU_SHELL (text_editor_menu),
				     text_editor_menu_uiinfo, NULL, FALSE, 0);

		menu->GUI = text_editor_menu;
		menu->copy = text_editor_menu_uiinfo[0].widget;
		menu->cut = text_editor_menu_uiinfo[1].widget;
		menu->autoformat = text_editor_menu_uiinfo[7].widget;
		menu->swap = text_editor_menu_uiinfo[9].widget;
		menu->functions = text_editor_menu_uiinfo[11].widget;
		menu->debug = text_editor_menu_uiinfo[13].widget;

		gtk_widget_ref (menu->GUI);
		gtk_widget_ref (menu->copy);
		gtk_widget_ref (menu->cut);
		gtk_widget_ref (menu->autoformat);
		gtk_widget_ref (menu->swap);
		gtk_widget_ref (menu->functions);
		gtk_widget_ref (menu->debug);
	}
}

void
on_text_editor_menu_swap_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gchar *newfname;
	TextEditor *te;

	te = anjuta_get_current_text_editor ();
	if (!te)
		return;
	if (!te->full_filename)
		return;

	newfname = get_swapped_filename (te->full_filename);
	if (newfname)
	{
		anjuta_goto_file_line (newfname, -1);
		g_free (newfname);
	}
	return;
}

void
on_text_editor_menu_function_activate (GtkMenuItem * menuitem,
				       gpointer user_data)
{
	anjuta_goto_symbol_definition((const char *) user_data, NULL);
}

