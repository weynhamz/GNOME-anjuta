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
#include "text_editor_cbs.h"
#include "anjuta.h"
#include "pixmaps.h"
#include "mainmenu_callbacks.h"
#include "toolbar_callbacks.h"
#include "debugger.h"
#include "global.h"

static void
on_tem_dock_undock_activate (GtkWidget* menuitem, gpointer data);

static void
on_tem_toggle_linenum_activate (GtkWidget* menuitem, gpointer data)
{
	g_signal_emit_by_name (G_OBJECT
				 (app->widgets.menubar.view.editor_linenos),
				 "activate");
}

static void
on_tem_toggle_marker_margin_activate (GtkWidget* menuitem, gpointer data)
{
	g_signal_emit_by_name (G_OBJECT
				 (app->widgets.menubar.view.editor_markers),
				 "activate");
}

static void
on_tem_toggle_code_fold_activate (GtkWidget* menuitem, gpointer data)
{
	g_signal_emit_by_name (G_OBJECT
				 (app->widgets.menubar.view.editor_folds),
				 "activate");
}

static void
on_tem_toggle_guides_activate (GtkWidget* menuitem, gpointer data)
{
	g_signal_emit_by_name (G_OBJECT
				 (app->widgets.menubar.view.editor_indentguides),
				 "activate");
}

GnomeUIInfo text_editor_menu_goto_submenu_uiinfo[] = {
	{
	 GNOME_APP_UI_ITEM, N_("Back"),
	 NULL,
	 on_go_back_activate, NULL, NULL,
	 PIX_STOCK(GTK_STOCK_GO_BACK),
 	 0, 0, NULL}
	 ,
	{
	 GNOME_APP_UI_ITEM, N_("Forward"),
	 NULL,
	 on_go_forward_activate, NULL, NULL,
	 PIX_STOCK(GTK_STOCK_GO_FORWARD),
 	 0, 0, NULL}
	 ,
	 GNOMEUIINFO_SEPARATOR
	 ,
	{
	 GNOME_APP_UI_ITEM, N_("Tag Definition"),
	 NULL,
	 on_goto_tag_activate, (gpointer) TRUE, NULL,
	 PIX_FILE(TAG),
 	 0, 0, NULL}
	 ,
	{
	 GNOME_APP_UI_ITEM, N_("Tag Declaration"),
	 NULL,
	 on_goto_tag_activate, (gboolean) FALSE, NULL,
	 PIX_STOCK(GTK_STOCK_JUMP_TO),
 	 0, 0, NULL}
	 ,
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Prev mesg"),
	 NULL,
	 on_goto_prev_mesg1_activate, NULL, NULL,
	 PIX_STOCK(GTK_STOCK_GO_UP),
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Next mesg"),
	 NULL,
	 on_goto_next_mesg1_activate, NULL, NULL,
	 PIX_STOCK(GTK_STOCK_GO_DOWN),
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Prev bookmark"),
	 NULL,
	 on_editor_command_activate, (gpointer) (ANE_BOOKMARK_PREV), NULL,
	 PIX_FILE(BOOKMARK_PREV),
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Next bookmark"),
	 NULL,
	 on_editor_command_activate, (gpointer) (ANE_BOOKMARK_NEXT), NULL,
	 PIX_FILE(BOOKMARK_NEXT),
	 0, 0, NULL},
	GNOMEUIINFO_END
};

GnomeUIInfo text_editor_menu_debug_submenu_uiinfo[] = {
	{
	 GNOME_APP_UI_ITEM, N_("Toggle breakpoint"),
	 NULL,
	 on_toggle_breakpoint1_activate, NULL, NULL,
	 PIX_FILE(BREAKPOINT),
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Step in"),
	 NULL,
	 on_execution_step_in1_activate, NULL, NULL,
	 PIX_FILE(STEP_IN),
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Step over"),
	 NULL,
	 on_execution_step_over1_activate, NULL, NULL,
	 PIX_FILE(STEP_OVER),
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Step out"),
	 NULL,
	 on_execution_step_out1_activate, NULL, NULL,
	 PIX_FILE(STEP_OUT),
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Run to cursor"),
	 NULL,
	 on_execution_run_to_cursor1_activate, NULL, NULL,
	 PIX_FILE(RUN_TO_CURSOR),
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Run/Continue"),
	 NULL,
	 on_execution_continue1_activate, NULL, NULL,
	 PIX_FILE(CONTINUE),
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Inspect"),
	 NULL,
	 on_debugger_inspect_activate, NULL, NULL,
	 PIX_FILE(INSPECT),
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Interrupt"),
	 NULL,
	 on_debugger_interrupt_activate, NULL, NULL,
	 PIX_FILE(INTERRUPT),
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
	 on_zoom_text_activate, "++", NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL}
	,
	{
	 GNOME_APP_UI_ITEM, N_("--Zoom"),
	 NULL,
	 on_zoom_text_activate, "--", NULL,
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
	 on_editor_command_activate, (gpointer) ANE_CUT, NULL,
	 PIX_STOCK(GTK_STOCK_CUT),
	 GDK_x, GDK_CONTROL_MASK, NULL}
	,
	{
	 /* 1 */
	 GNOME_APP_UI_ITEM, N_("Copy"),
	 NULL,
	 on_editor_command_activate, (gpointer) ANE_COPY, NULL,
	 PIX_STOCK(GTK_STOCK_COPY),
	 GDK_c, GDK_CONTROL_MASK, NULL}
	,
	{
	 /* 2 */
	 GNOME_APP_UI_ITEM, N_("Paste"),
	 NULL,
	 on_editor_command_activate, (gpointer) ANE_PASTE, NULL,
	 PIX_STOCK(GTK_STOCK_PASTE),
	 GDK_v, GDK_CONTROL_MASK, NULL}
	,
	/* 3 */
	GNOMEUIINFO_SEPARATOR,
	{
	 /* 4 */
	 GNOME_APP_UI_ITEM, N_("Context Help"),
	 NULL,
	 on_context_help_activate, NULL, NULL,
	 PIX_STOCK(GTK_STOCK_HELP),
 	 GDK_h, GDK_CONTROL_MASK, NULL}
	 ,
	/* 5 */
	GNOMEUIINFO_SEPARATOR,
	{
	 /* 6 */
	 GNOME_APP_UI_ITEM, N_("Toggle Bookmark"),
	 NULL,
	 on_editor_command_activate, (gpointer) ANE_BOOKMARK_TOGGLE, NULL,
	 PIX_FILE(BOOKMARK_TOGGLE),
	 0, 0, NULL}
	,
	{
	 /* 7 */
	 GNOME_APP_UI_ITEM, N_("Auto format"),
	 NULL,
	 on_indent1_activate, NULL, NULL,
	 PIX_FILE(INDENT_AUTO),
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
	 PIX_FILE(GOTO),
	 0, 0, NULL}
	,
	{
	 /* 11 */
	 GNOME_APP_UI_ITEM, N_("Tags"),
	 NULL,
	 NULL, NULL, NULL,
	 PIX_FILE(TAG),
	 0, 0, NULL}
	,
	 /* 12 */
	GNOMEUIINFO_SEPARATOR,
	{
	 /* 13 */
	 GNOME_APP_UI_SUBTREE, N_("Debug"),
	 NULL,
	 text_editor_menu_debug_submenu_uiinfo, NULL, NULL,
	 PIX_FILE(DEBUG),
	 0, 0, NULL}
	,
	 /* 14 */
	GNOMEUIINFO_SEPARATOR,
	{
	 /* 15 */
	 GNOME_APP_UI_SUBTREE, N_("Options"),
	 NULL,
	 text_editor_menu_options_submenu_uiinfo, NULL, NULL,
	 PIX_STOCK(GTK_STOCK_PREFERENCES),
	 0, 0, NULL}
	,
	 /* 16 */
	GNOMEUIINFO_SEPARATOR,
	{
	 /* 17 */
	 GNOME_APP_UI_ITEM, N_("Find Usage"),
	 NULL,
	 on_lookup_symbol_activate, NULL, NULL,
	 PIX_STOCK(GTK_STOCK_FIND),
	 0, 0, NULL}
	,
	{
	 /* 18 */
	 GNOME_APP_UI_ITEM, N_("Close"),
	 NULL,
	 on_close_file1_activate, NULL, NULL,
	 PIX_STOCK(GTK_STOCK_CLOSE),
	 0, 0, NULL}
	,
	{
	 /* 19 */
	 GNOME_APP_UI_TOGGLEITEM, N_("Docked"),
	 NULL,
	 on_tem_dock_undock_activate, NULL, NULL,
	 PIX_FILE(DOCK),
	 0, 0, NULL}
	,
	 /* 20 */
	GNOMEUIINFO_END
};

static void
on_tem_dock_undock_activate (GtkWidget* menuitem, gpointer data)
{
	TextEditor *te = anjuta_get_current_text_editor();

	if (NULL == te)
		return;
	if (TEXT_EDITOR_WINDOWED == te->mode)
		on_text_editor_dock_activate(NULL, NULL);
	else
		on_detach1_activate(NULL, NULL);
}


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
		gtk_widget_unref (tm->docked);
		if (tm->GUI)
			gtk_widget_destroy (tm->GUI);
		free (tm);
		tm = NULL;
	}
}

void
text_editor_menu_popup (TextEditorMenu * menu, GdkEventButton * bevent)
{
	const GList *local_tags;
	GtkWidget *submenu;
	TextEditor *te;
	gboolean A, B;

	g_return_if_fail (menu != NULL);

	te = anjuta_get_current_text_editor ();
	if (te == NULL)
		return;

	A = debugger_is_active ();
	B = (te->mode == TEXT_EDITOR_WINDOWED);

	local_tags = anjuta_get_tag_list(te, tm_tag_max_t);
	if (local_tags)
	{
		submenu =
			create_submenu (N_("Tags"), (GList *) local_tags,
					G_CALLBACK
					(on_text_editor_menu_tags_activate));
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
	gtk_widget_set_sensitive(menu->context_help, app->has_devhelp);
	GTK_CHECK_MENU_ITEM(menu->docked)->active = !B;
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
		menu->cut = text_editor_menu_uiinfo[0].widget;
		menu->copy = text_editor_menu_uiinfo[1].widget;
		menu->paste = text_editor_menu_uiinfo[2].widget;
		menu->context_help = text_editor_menu_uiinfo[4].widget;
		menu->autoformat = text_editor_menu_uiinfo[7].widget;
		menu->swap = text_editor_menu_uiinfo[8].widget;
		menu->functions = text_editor_menu_uiinfo[11].widget;
		menu->debug = text_editor_menu_uiinfo[13].widget;
		menu->docked = text_editor_menu_uiinfo[19].widget;

		gtk_widget_ref (menu->GUI);
		gtk_widget_ref (menu->cut);
		gtk_widget_ref (menu->copy);
		gtk_widget_ref (menu->paste);
		gtk_widget_ref (menu->context_help);
		gtk_widget_ref (menu->autoformat);
		gtk_widget_ref (menu->swap);
		gtk_widget_ref (menu->functions);
		gtk_widget_ref (menu->debug);
		gtk_widget_ref (menu->docked);
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
on_text_editor_menu_tags_activate (GtkMenuItem * menuitem,
				       gpointer user_data)
{
	TextEditor *te = anjuta_get_current_text_editor();
	if (!te)
		return;
	anjuta_goto_local_tag(te, (const char *) user_data);
}
