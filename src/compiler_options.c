/*
    compiler_options.c
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
#include "anjuta.h"
#include "resources.h"
#include "compiler_options.h"

typedef struct
{
	gint ref_count;

	GtkWidget *window;

	GtkWidget *supp_clist;
	GtkWidget *inc_clist;
	GtkWidget *inc_entry;

	GtkWidget *lib_paths_clist;
	GtkWidget *lib_paths_entry;

	GtkWidget *lib_clist;
	GtkWidget *lib_stock_clist;
	GtkWidget *lib_entry;

	GtkWidget *def_clist;
	GtkWidget *def_stock_clist;
	GtkWidget *def_entry;

	GtkWidget *warnings_clist;
	GtkWidget *optimize_button[4];
	GtkWidget *other_button[2];

	GtkWidget *other_c_flags_entry;
	GtkWidget *other_l_flags_entry;
	GtkWidget *other_l_libs_entry;
} CompilerOptionsGui;

struct _CompilerOptionsPriv
{
	GladeXML *gxml;
	CompilerOptionsGui widgets;
	gboolean dirty;	

	gboolean is_showing;
	gint win_pos_x, win_pos_y;

	/* This property database is not the one
	 * from which compiler options will be loaded
	 * or saved, but the one in which option
	 * variables will be set for the commands
	 * to use
	 */
	PropsID props;
};


static void create_compiler_options_gui (CompilerOptions *co);


#define ANJUTA_SUPPORTS_END_STRING "SUPPORTS_END"
#define ANJUTA_SUPPORTS_END \
  { \
    ANJUTA_SUPPORTS_END_STRING, NULL, NULL, \
    NULL, NULL, NULL, NULL, NULL, NULL, NULL \
  }

gchar *anjuta_supports[][ANJUTA_SUPPORT_END_MARK] = {
	{
	 "GLIB",
	 "[gtk+ 1.2] Glib library for most C utilities",
	 "",
	 "AM_MISSING_PROG(GLIB_CONFIG, glib-config, $mising_dir)\n",
	 "`glib-config --cflags`",
	 "`glib-config --libs`",
	 "`glib-config --cflags`",
	 "`glib-config --libs`",
	 "",
	 "glib-config --version"}
	,
	{
	 "GTK",
	 "[gtk+ 1.2] Gimp Toolkit library for GUI development",
	 "",
	 "",
	 "gtk-config --cflags",
	 "$(GTK_LIBS)",
	 "`gtk-config --cflags`",
	 "`gtk-config --libs`",
	 "",
	 "gtk-config --version"}
	,
	{
	 "GNOME",
	 "[gnome 1.4] GNOME is Computing Made Easy",
	 "",
	 "",
	 "$(GNOME_INCLUDESDIR)",
	 "$(GNOME_LIBDIR) $(GNOME_LIBS)",
	 "`gnome-config --cflags glib gtk gnome gnomeui`",
	 "`gnome-config --libs glib gtk gnome gnomeui`",
	 "",
	 "gnome-config --version"}
	,
	{
	 "BONOBO",
	 "[gnome 1.4] GNOME Bonobo component for fast integration",
	 "",
	 "",
	 "$(BONOBO_CFLAGS)",
	 "$(BONOBO_LIBS)",
	 "`gnome-config --cflags glib gtk gnome gnomeui bonobo`",
	 "`gnome-config --libs glib gtk gnome gnomeui bonobo`",
	 "",
	 "gnome-config --version"}
	,
	{
	"gtkmm",
	 "[gtk+ 1.2] C++ Bindings for GTK",
	 "",
	 "",
	 "$(GTKMM_CFLAGS)",
	 "$(GTKMM_LIBS)",
	 "`gtkmm-config --cflags`",
	 "`gtkmm-config --libs`",
	 "",
	 "gtkmm-config --version"}
	 ,
	{
	 "gnomemm",
	 "[gnome 1.4] C++ bindings for GNOME",
	 "",
	 "",
	 "$(GNOMEMM_CFLAGS)",
	 "$(GNOMEMM_LIBS)",
	 "`gnome-config --cflags gnomemm`",
	 "`gnome-config --libs gnomemm`",
	 "",
	 "gnome-config --version"}
	 ,
	{
	 "LIBGLADE",
	 "[gnome 1.4] C program using libglade",
	 "",
	 "",
	 "$(LIBGLADE_CFLAGS)",
	 "$(LIBGLADE_LIBS)",
	 "`libglade-config --cflags gnome`",
	 "`libglade-config --libs gnome`",
	 "",
	 "libglade-config --version"}
	 ,
	{
	 "WXWINDOWS",
	 "[gtk+ 1.2] C++ program using wxWindows (wxGTK) toolkit",
	 "",
	 "",
	 "$(WX_CXXFLAGS)",
	 "$(WX_LIBS)",
	 "`wx-config --cxxflags`",
	 "`wx-config --libs`",
	 "",
	 "wx-config --version"}
	,
	ANJUTA_SUPPORTS_END
};

static gchar *anjuta_warnings[] = {
	" -Werror",                    "Consider warnings as errors",
	" -w",                         "No warnings",
	" -Wall",                      "Enable most warnings",
	" -Wimplicit",                 "Warning for implicit declarations",
	" -Wreturn-type",              "Warning for mismatched return types",
	" -Wunused",                   "Warning for unused variables",
	" -Wswitch",                "Warning for unhandled switch case with enums",
	" -Wcomment",                  "Warning for nested comments",
	" -Wuninitialized",            "Warning for uninitialized variable use",
	" -Wparentheses",              "Warning for missing parentheses",
	" -Wtraditional",         "Warning for differences to traditionanl syntax",
	" -Wshadow",                   "Warning for variable shadowing",
	" -Wpointer-arith",            "Warning for missing prototypes",
	" -Wmissing-prototypes",       "Warning for suspected pointer arithmetic",
	" -Winline",                   "Warning if declarations cannot be inlined",
	" -Woverloaded-virtual",        "Warning for overloaded virtuals",
	NULL, NULL
};

static gchar *anjuta_defines[] = {
	"HAVE_CONFIG_H",
	"DEBUG_LEVEL_2",
	"DEBUG_LEVEL_1",
	"DEBUG_LEVEL_0",
	"RELEASE",
	NULL,
};

static gchar *optimize_button_option[] = {
	"",
	" -O1",
	" -O2",
	" -O3"
};

static gchar *other_button_option[] = {
	" -g",
	" -pg",
};

enum {
	SUPP_TOGGLE_COLUMN,
	SUPP_NAME_COLUMN,
	SUPP_DESCRIPTION_COLUMN,
	SUPP_PKGCONFIG_COLUMN,
	N_SUPP_COLUMNS
};

enum {
	INC_PATHS_COLUMN,
	N_INC_COLUMNS
};

enum {
	LIB_PATHS_COLUMN,
	N_LIB_PATHS_COLUMN
};

enum {
	LIB_TOGGLE_COLUMN,
	LIB_COLUMN,
	N_LIB_COLUMNS
};

enum {
	LIB_STOCK_COLUMN,
	LIB_STOCK_DES_COLUMN,
	N_LIB_STOCK_COLUMNS
};

enum {
	DEF_TOGGLE_COLUMN,
	DEF_DEFINE_COLUMN,
	N_DEF_COLUMNS
};

enum {
	DEF_STOCK_COLUMN,
	N_DEF_STOCK_COLUMNS
};

enum {
	WARNINGS_TOGGLE_COLUMN,
	WARNINGS_COLUMN,
	WARNINGS_DES_COLUMN,
	N_WARNINGS_COLUMNS
};

/* private */
static void compiler_options_set_in_properties (CompilerOptions* co,
												PropsID props);

static void co_cid_set (GtkListStore* store, GtkTreeIter *iter, gboolean state)
{
	// cid->state = state;
}

static gboolean co_cid_get (GtkListStore* store, GtkTreeIter *iter)
{
	// return cid->state;
	return FALSE;
}

static gboolean co_cid_toggle (GtkListStore* store, GtkTreeIter *iter)
{
	return FALSE;
}

static void
populate_stock_libs (GtkListStore *tmodel)
{
	gint i, ch;
	gchar *stock_file;
	gchar lib_name[256];
	gchar desbuff[512];
	FILE *fp;
	
	/* Now read the stock libraries */
	stock_file =
		g_strconcat (app->dirs->data, "/stock_libs.anj",
				 NULL);
	fp = fopen (stock_file, "r");
	g_free (stock_file);

	if (fp == NULL)
		goto down;

	/* Skip the first line which is comment */
	do
	{
		ch = fgetc (fp);
		if (ch == EOF)
			goto down;
	}
	while (ch != '\n');

	while (feof (fp) == FALSE)
	{
		GtkTreeIter iter;
		
		/* Read a line at a time */
		fscanf (fp, "%s", lib_name);	/* The lib name */

		/* Followed by description */
		i = 0;
		do
		{
			ch = fgetc (fp);
			if (ch == EOF)
				goto down;	/* Done when eof */
			desbuff[i] = (gchar) ch;
			i++;
		}
		while (ch != '\n');

		desbuff[--i] = '\0';
		
		gtk_list_store_append (tmodel, &iter);
		gtk_list_store_set (tmodel, &iter,
							LIB_STOCK_COLUMN, lib_name,
							LIB_STOCK_DES_COLUMN, desbuff,
							-1);
	}

  down:
	if (fp)
		fclose (fp);
}

static void
populate_supports (GtkListStore *tmodel)
{
	gchar *tmpfile;
	gchar *pkg_cmd;
	FILE  *pkg_fd;
	gchar *line;
	gint   length;
	GtkTreeIter iter;
	
	int i = 0;

	/* Now setup all the available supports */
	while (strcmp (anjuta_supports[i][ANJUTA_SUPPORT_ID],
				   ANJUTA_SUPPORTS_END_STRING) != 0)
	{
		gtk_list_store_append (tmodel, &iter);
		gtk_list_store_set (tmodel, &iter, 
							SUPP_TOGGLE_COLUMN, FALSE,
							SUPP_NAME_COLUMN, anjuta_supports[i][0],
							SUPP_DESCRIPTION_COLUMN, anjuta_supports[i][1],
							SUPP_PKGCONFIG_COLUMN, FALSE,
							-1);
		i++;
	}
	/* Now setup all the pkg-config supports */
	tmpfile = get_a_tmp_file ();
	pkg_cmd = g_strconcat ("pkg-config --list-all > ", tmpfile,
						   " 2>/dev/null", NULL);
	system (pkg_cmd);
	pkg_fd = fopen (tmpfile, "r");
	if (!pkg_fd)
	{
		g_warning ("Can not open %s for reading", tmpfile);
		g_free (tmpfile);
		return;
	}
	line = NULL;
	while (getline (&line, &length, pkg_fd) > 0)
	{
		gchar *name_end;
		gchar *desc_start;
		gchar *description;
		gchar *name;
		
		if (line == NULL)
			break;
		
		name_end = line;
		while (!isspace(*name_end))
			name_end++;
		desc_start = name_end;
		while (isspace(*desc_start))
			desc_start++;
		
		name = g_strndup (line, name_end-line);
		description = g_strndup (desc_start, strlen (desc_start)-1);
		
		gtk_list_store_append (tmodel, &iter);
		gtk_list_store_set (tmodel, &iter, 
							SUPP_TOGGLE_COLUMN, FALSE,
							SUPP_NAME_COLUMN, name,
							SUPP_DESCRIPTION_COLUMN, description,
							SUPP_PKGCONFIG_COLUMN, TRUE,
							-1);
		g_free (line);
		line = NULL;
	}
	fclose (pkg_fd);
	remove (tmpfile);
	g_free (tmpfile);
}

static void
populate_stock_defs (GtkListStore *tmodel)
{
	/* Now setup buildin defines */
	int i = 0;
	while (anjuta_defines[i])
	{
		GtkTreeIter iter;
		gtk_list_store_append (tmodel, &iter);
		gtk_list_store_set (tmodel, &iter,
							DEF_STOCK_COLUMN, anjuta_defines[i],
							-1);
		i++;
	}
}

static void
populate_warnings (GtkListStore *tmodel)
{
	/* Now setup buildin defines */
	int i = 0;
	while (anjuta_warnings[i])
	{
		GtkTreeIter iter;
		gtk_list_store_append (tmodel, &iter);
		gtk_list_store_set (tmodel, &iter,
							WARNINGS_TOGGLE_COLUMN, FALSE,
							WARNINGS_COLUMN, anjuta_warnings[i],
							WARNINGS_DES_COLUMN, anjuta_warnings[i+1],
							-1);
		i += 2;
	}
}

typedef struct {
	CompilerOptions *co;
	GtkTreeView *tree;
	GtkEntry *entry;
	gint col;
} EntrySignalInfo;

typedef struct {
	CompilerOptions *co;
	gint col;
} ToggleSignalInfo;

static EntrySignalInfo *
entry_signal_info_new (CompilerOptions *co, GtkWidget *tree, GtkWidget *entry, 
					   gint col)
{
	EntrySignalInfo *info;
	g_return_val_if_fail ((GTK_IS_TREE_VIEW (tree) && GTK_IS_ENTRY (entry)),
						  NULL);
	info = g_new0 (EntrySignalInfo, 1);
	info->co = co;
	info->tree = GTK_TREE_VIEW (tree);
	info->entry = GTK_ENTRY (entry);
	info->col = col;
	return info;
}

static ToggleSignalInfo *
toggle_signal_info_new (CompilerOptions *co, gint col)
{
	ToggleSignalInfo *info;

	info = g_new0 (ToggleSignalInfo, 1);
	info->co = co;
	info->col = col;
	return info;
}

static void
entry_signal_info_destroy (EntrySignalInfo *info)
{
	g_return_if_fail (info);
	g_free (info);
}


static void
on_comp_finish (CompilerOptions* co)
{
	gboolean should_rebuild;
	
	if (co->priv->dirty) 
	{
		gchar* msg = _("You have changed some of the compiler options of the project,\n"
					   "would you like the next build action to perform a complete\n"
					   "rebuild of the project?");				
		should_rebuild = anjuta_boolean_question (msg);
		if (should_rebuild)
			app->project_dbase->clean_before_build = TRUE;
	}
	compiler_options_set_in_properties(co, co->priv->props);
	project_dbase_save_project (app->project_dbase);
}

static gboolean
on_compt_delete_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	CompilerOptions *co = data;
	g_return_if_fail (co);	
	
	on_comp_finish (co);
	compiler_options_hide(co);
	return TRUE;
}

static void
on_compt_response (GtkWidget *widget, gint response, gpointer data)
{
	CompilerOptions *co = data;
	g_return_if_fail (co);	
	
	switch (response)
	{
	case GTK_RESPONSE_HELP:
		/* FIXME: Add help for compiler options here */
		return;
	case GTK_RESPONSE_CLOSE:
			on_comp_finish (co);
		compiler_options_hide (co);
		return;
	}
}

static void
on_toggle_clist_row_activated          (GtkTreeView     *treeview,
                                        GtkTreePath     *arg1,
                                        GtkTreeViewColumn *arg2,
                                        gpointer        data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean value;
	ToggleSignalInfo* info = (ToggleSignalInfo*) data;
	
	model = gtk_tree_view_get_model (treeview);
	if (gtk_tree_model_get_iter (model, &iter, arg1))
	{
		gtk_tree_model_get (model, &iter, info->col, &value, -1);
		value = value? FALSE : TRUE;
		gtk_list_store_set (GTK_LIST_STORE (model), &iter, info->col, value, -1);
		info->co->priv->dirty = TRUE;
	}
}

static void
on_update_selection_changed (GtkTreeSelection *sel, EntrySignalInfo *info)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	const gchar *text;
	GtkEntry *entry = info->entry;
	gint col = info->col;
	if (gtk_tree_selection_get_selected (sel, &model, &iter))
	{
		gtk_tree_model_get (model, &iter, col, &text, -1);
		gtk_entry_set_text (entry, text);
		info->co->priv->dirty = TRUE;
	}
}


static void
on_button_selection_changed (GtkToggleButton *togglebutton, gpointer user_data)
{
	CompilerOptions *co = (CompilerOptions*) user_data;
	co->priv->dirty = TRUE;
}

static void
on_entry_changed (GtkEntry *togglebutton, gpointer user_data)
{
	CompilerOptions *co = (CompilerOptions*) user_data;
	co->priv->dirty = TRUE;
}



static gboolean
verify_new_entry (GtkTreeView *tree, const gchar *str, gint col)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW(tree));
	g_assert (model);
	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid)
	{
		gchar *text;
		gtk_tree_model_get (model, &iter, col, &text, -1);
		if (strcmp(str, text) == 0)
			return FALSE;
		valid = gtk_tree_model_iter_next (model, &iter);
	}
	return TRUE;
}

static void
on_add_to_clist_clicked (GtkButton * button, gpointer data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;
	gchar *text;
	gchar *str;
	EntrySignalInfo *info = data;
	GtkTreeView *tree = info->tree;
	GtkEntry *entry = info->entry;
	gint col = info->col;
	
	str = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
	text = g_strstrip (str);
	if (strlen (text) == 0)
	{
		g_free (str);
		return;
	}
	if (verify_new_entry (tree, text, col) == FALSE)
	{
		gtk_entry_set_text (GTK_ENTRY (entry), "");
		g_free (str);
		return;
	}
	model = gtk_tree_view_get_model (GTK_TREE_VIEW(tree));
	g_assert (model);
	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	gtk_list_store_set (GTK_LIST_STORE(model), &iter, col, text, -1);
	if (col != 0) /* Also enable it */
		gtk_list_store_set (GTK_LIST_STORE(model), &iter, 0, TRUE, -1);
	gtk_entry_set_text (GTK_ENTRY (entry), "");
	g_free (str);
	info->co->priv->dirty = TRUE;	
}

static void
on_update_in_clist_clicked (GtkButton * button, gpointer data)
{
	GtkTreeModel *model;
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	gboolean valid;
	gchar *text, *str;
	EntrySignalInfo *info = data;
	GtkTreeView *tree = info->tree;
	GtkEntry *entry = info->entry;
	gint col = info->col;
	
	str = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
	text = g_strstrip(str);
	if (strlen (text) == 0)
	{
		g_free (str);
		return;
	}
	if (verify_new_entry (tree, text, col) == FALSE)
	{
		gtk_entry_set_text (GTK_ENTRY (entry), "");
		g_free (str);
		return;
	}
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW(tree));
	if (gtk_tree_selection_get_selected (sel, &model, &iter))
	{
		gtk_list_store_set (GTK_LIST_STORE(model), &iter, col, text, -1);
		gtk_entry_set_text (GTK_ENTRY (entry), "");
	}
	g_free (str);
	info->co->priv->dirty = TRUE;	
}

void
on_remove_from_clist_clicked (GtkButton * button, gpointer data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *sel;
	gboolean valid;
	EntrySignalInfo *info = data;
	GtkTreeView *tree = info->tree;
	GtkEntry *entry = info->entry;
	gint col = info->col;
	
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW(tree));
	if (gtk_tree_selection_get_selected (sel, &model, &iter))
	{
		gtk_list_store_remove (GTK_LIST_STORE(model), &iter);
		gtk_entry_set_text (GTK_ENTRY (entry), "");
		info->co->priv->dirty = TRUE;	
	}
}

static void
on_clear_clist_clicked (GtkButton * button, gpointer data)
{
	GtkWidget *win;
	GtkWidget *top_level;
	EntrySignalInfo *info = data;
	GtkTreeView *tree = info->tree;
	GtkListStore *model = GTK_LIST_STORE (gtk_tree_view_get_model (tree));
	top_level = gtk_widget_get_toplevel (GTK_WIDGET (tree));
	win = gtk_message_dialog_new (GTK_WINDOW (top_level),
								  GTK_DIALOG_DESTROY_WITH_PARENT,
								  GTK_MESSAGE_QUESTION,
								  GTK_BUTTONS_NONE,
								  _("Do you want to clear the list?"),
								  NULL);
	gtk_dialog_add_buttons (GTK_DIALOG (win),
							GTK_STOCK_CANCEL,	GTK_RESPONSE_CANCEL,
							GTK_STOCK_CLEAR,	GTK_RESPONSE_YES,
							NULL);
	if (gtk_dialog_run (GTK_DIALOG (win)) == GTK_RESPONSE_YES)
	{
		gtk_list_store_clear (GTK_LIST_STORE (model));
		info->co->priv->dirty = TRUE;
	}
	gtk_widget_destroy (win);
}

#define BUTTON_SIGNAL_CONNECT(w, f, d) \
	g_signal_connect (G_OBJECT (w), "clicked", G_CALLBACK (f), d);

static
void create_compiler_options_gui (CompilerOptions *co)
{
	GtkTreeView *clist;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;
	GtkListStore *store, *list;
	GtkCellRenderer *renderer;
	GtkWidget *button;
	int i;
	
	/* Compiler Options Dialog */
	co->priv->gxml = glade_xml_new (GLADE_FILE_ANJUTA, "compiler_options_dialog", NULL);
	co->priv->widgets.window = glade_xml_get_widget (co->priv->gxml, "compiler_options_dialog");
	gtk_widget_hide (co->priv->widgets.window);
	gtk_window_set_transient_for (GTK_WINDOW (co->priv->widgets.window),
								  GTK_WINDOW (app->widgets.window));
	co->priv->widgets.supp_clist = glade_xml_get_widget (co->priv->gxml, "supp_clist");
	co->priv->widgets.inc_clist = glade_xml_get_widget (co->priv->gxml, "inc_clist");
	co->priv->widgets.inc_entry = glade_xml_get_widget (co->priv->gxml, "inc_entry");
	co->priv->widgets.lib_paths_clist = glade_xml_get_widget (co->priv->gxml, "lib_paths_clist");
	co->priv->widgets.lib_paths_entry = glade_xml_get_widget (co->priv->gxml, "lib_paths_entry");
	co->priv->widgets.lib_clist = glade_xml_get_widget (co->priv->gxml, "lib_clist");
	co->priv->widgets.lib_stock_clist = glade_xml_get_widget (co->priv->gxml, "lib_stock_clist");
	co->priv->widgets.lib_entry = glade_xml_get_widget (co->priv->gxml, "lib_entry");
	co->priv->widgets.def_clist = glade_xml_get_widget (co->priv->gxml, "def_clist");
	co->priv->widgets.def_stock_clist = glade_xml_get_widget (co->priv->gxml, "def_stock_clist");
	co->priv->widgets.def_entry = glade_xml_get_widget (co->priv->gxml, "def_entry");
	co->priv->widgets.warnings_clist = glade_xml_get_widget (co->priv->gxml, "warnings_clist");
	co->priv->widgets.optimize_button[0] = glade_xml_get_widget (co->priv->gxml, "optimization_b_1");
	co->priv->widgets.optimize_button[1] = glade_xml_get_widget (co->priv->gxml, "optimization_b_2");
	co->priv->widgets.optimize_button[2] = glade_xml_get_widget (co->priv->gxml, "optimization_b_3");
	co->priv->widgets.optimize_button[3] = glade_xml_get_widget (co->priv->gxml, "optimization_b_4");
	co->priv->widgets.other_button[0] = glade_xml_get_widget (co->priv->gxml, "debugging_b");
	co->priv->widgets.other_button[1] = glade_xml_get_widget (co->priv->gxml, "profiling_b");
	co->priv->widgets.other_c_flags_entry = glade_xml_get_widget (co->priv->gxml, "other_c_flags_entry");
	co->priv->widgets.other_l_flags_entry = glade_xml_get_widget (co->priv->gxml, "other_l_flags_entry");
	co->priv->widgets.other_l_libs_entry = glade_xml_get_widget (co->priv->gxml, "other_l_libs_entry");

	/* Add supports tree model */
	clist = GTK_TREE_VIEW (co->priv->widgets.supp_clist);
	store = gtk_list_store_new (N_SUPP_COLUMNS,
								G_TYPE_BOOLEAN,
								G_TYPE_STRING,
								G_TYPE_STRING,
								G_TYPE_BOOLEAN);
	gtk_tree_view_set_model (clist,	GTK_TREE_MODEL(store));
	
	/* Add supports columns */	
	renderer = gtk_cell_renderer_toggle_new ();
	column = gtk_tree_view_column_new_with_attributes ("", renderer,
													   "active",
													   SUPP_TOGGLE_COLUMN,
													   NULL);
	gtk_tree_view_append_column (clist, column);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Name"),
													   renderer,
													   "text",
													   SUPP_NAME_COLUMN,
													   NULL);
	gtk_tree_view_append_column (clist, column);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Description"),
													   renderer,
													   "text",
													   SUPP_DESCRIPTION_COLUMN,
													   NULL);
	gtk_tree_view_append_column (clist, column);
	populate_supports (store);
	g_object_unref (G_OBJECT(store));
	
	/* Add "includes" tree model */
	clist = GTK_TREE_VIEW (co->priv->widgets.inc_clist);
	store = gtk_list_store_new (N_INC_COLUMNS, G_TYPE_STRING);
	gtk_tree_view_set_model (clist, GTK_TREE_MODEL(store));
	
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Include paths"),
													   renderer,
													   "text",
													   INC_PATHS_COLUMN,
													   NULL);
	gtk_tree_view_append_column (clist, column);
	g_object_unref (G_OBJECT(store));
	
	/* Add "libraries paths" model */
	clist = GTK_TREE_VIEW (co->priv->widgets.lib_paths_clist);
	store = gtk_list_store_new (N_LIB_PATHS_COLUMN, G_TYPE_STRING);
	gtk_tree_view_set_model (clist,	GTK_TREE_MODEL(store));
	
	/* Add "libraries paths" columns */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Libraries paths"),
												   renderer,
												   "text",
												   LIB_PATHS_COLUMN,
												   NULL);
	gtk_tree_view_append_column (clist, column);
	g_object_unref (G_OBJECT(store));
	
	/* Add "includes" tree model */
	clist = GTK_TREE_VIEW (co->priv->widgets.lib_clist);
	store = gtk_list_store_new (N_LIB_COLUMNS, G_TYPE_BOOLEAN,
								G_TYPE_STRING);
	gtk_tree_view_set_model (clist,	GTK_TREE_MODEL(store));
	
	/* Add "libraries paths" columns */
	renderer = gtk_cell_renderer_toggle_new ();
	column = gtk_tree_view_column_new_with_attributes ("", renderer,
													   "active",
													   LIB_TOGGLE_COLUMN,
													   NULL);
	gtk_tree_view_append_column (clist, column);
	renderer = gtk_cell_renderer_text_new ();
	column =
	gtk_tree_view_column_new_with_attributes (_("Libraries and modules"),
												   renderer,
												   "text",
												   LIB_COLUMN,
												   NULL);
	gtk_tree_view_append_column (clist, column);
	g_object_unref (G_OBJECT(store));
		
	/* Add "Libraries stock" model */
	clist = GTK_TREE_VIEW (co->priv->widgets.lib_stock_clist);
	store = gtk_list_store_new (N_LIB_STOCK_COLUMNS, G_TYPE_STRING,
								G_TYPE_STRING);
	gtk_tree_view_set_model (clist,	GTK_TREE_MODEL(store));
	
	/* Add "Libraries stock" column */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Stock"),
												   renderer,
												   "text",
												   LIB_STOCK_COLUMN,
												   NULL);
	gtk_tree_view_append_column (clist, column);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Description"),
												   renderer,
												   "text",
												   LIB_STOCK_DES_COLUMN,
												   NULL);
	gtk_tree_view_append_column (clist, column);
	populate_stock_libs (store);
	g_object_unref (G_OBJECT(store));
	
	/* Add "Defines" model */
	clist = GTK_TREE_VIEW (co->priv->widgets.def_clist);
	store = gtk_list_store_new (N_DEF_COLUMNS, G_TYPE_BOOLEAN,
								G_TYPE_STRING);
	gtk_tree_view_set_model (clist,	GTK_TREE_MODEL(store));
	
	/* Add defines columns */	
	renderer = gtk_cell_renderer_toggle_new ();
	column = gtk_tree_view_column_new_with_attributes ("", renderer,
													   "active",
													   DEF_TOGGLE_COLUMN,
													   NULL);
	gtk_tree_view_append_column (clist, column);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Description"),
													   renderer,
													   "text",
													   DEF_DEFINE_COLUMN,
													   NULL);
	gtk_tree_view_append_column (clist, column);
	g_object_unref (G_OBJECT(store));

	/* Add "Defines stock" model */
	clist = GTK_TREE_VIEW (co->priv->widgets.def_stock_clist);
	store = gtk_list_store_new (N_DEF_STOCK_COLUMNS, G_TYPE_STRING);
	gtk_tree_view_set_model (clist,	GTK_TREE_MODEL(store));
	
	/* Add defines columns */	
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Stock Defines"),
													   renderer,
													   "text",
													   DEF_STOCK_COLUMN,
													   NULL);
	gtk_tree_view_append_column (clist, column);
	populate_stock_defs (store);
	g_object_unref (G_OBJECT(store));
	
	/* Add "warnings" model */
	clist = GTK_TREE_VIEW (co->priv->widgets.warnings_clist);
	store = gtk_list_store_new (N_WARNINGS_COLUMNS, G_TYPE_BOOLEAN,
								G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model (clist, GTK_TREE_MODEL(store));
	
	/* Add "warnings" columns */	
	renderer = gtk_cell_renderer_toggle_new ();
	column = gtk_tree_view_column_new_with_attributes ("", renderer,
													"active",
													WARNINGS_TOGGLE_COLUMN,
													NULL);
	gtk_tree_view_append_column (clist, column);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Warning"),
													   renderer,
													   "text",
													   WARNINGS_COLUMN,
													   NULL);
	gtk_tree_view_append_column (clist, column);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Description"),
													   renderer,
													   "text",
													   WARNINGS_DES_COLUMN,
													   NULL);
	gtk_tree_view_append_column (clist, column);
	populate_warnings (store);
	g_object_unref (G_OBJECT(store));
	
	/* Connect window signals */
	g_signal_connect (G_OBJECT (co->priv->widgets.window), "delete_event",
					  G_CALLBACK (on_compt_delete_event), co);
	g_signal_connect (G_OBJECT (co->priv->widgets.window), "response",
					  G_CALLBACK (on_compt_response), co);
	
	/* Connect toggle signals */
	g_signal_connect (G_OBJECT (co->priv->widgets.supp_clist), "row_activated",
					  G_CALLBACK (on_toggle_clist_row_activated),
					  toggle_signal_info_new(co,SUPP_TOGGLE_COLUMN));
	g_signal_connect (G_OBJECT (co->priv->widgets.lib_clist), "row_activated",
					  G_CALLBACK (on_toggle_clist_row_activated),
					  toggle_signal_info_new(co,LIB_TOGGLE_COLUMN));
	g_signal_connect (G_OBJECT (co->priv->widgets.def_clist), "row_activated",
					  G_CALLBACK (on_toggle_clist_row_activated),
					  toggle_signal_info_new(co,DEF_TOGGLE_COLUMN));
	g_signal_connect (G_OBJECT (co->priv->widgets.warnings_clist), "row_activated",
					  G_CALLBACK (on_toggle_clist_row_activated),
					  toggle_signal_info_new(co,WARNINGS_TOGGLE_COLUMN));

	/* Connect Entry update signals */
	clist = GTK_TREE_VIEW (co->priv->widgets.inc_clist);
	selection = gtk_tree_view_get_selection (clist);
	g_signal_connect (G_OBJECT (selection), "changed",
					  G_CALLBACK (on_update_selection_changed),
					  entry_signal_info_new (co,
					  						 co->priv->widgets.inc_clist,
					   						 co->priv->widgets.inc_entry,
											 INC_PATHS_COLUMN));
	clist = GTK_TREE_VIEW (co->priv->widgets.lib_clist);
	selection = gtk_tree_view_get_selection (clist);
	g_signal_connect (G_OBJECT (selection), "changed",
					  G_CALLBACK (on_update_selection_changed),
					  entry_signal_info_new (co, co->priv->widgets.lib_clist,
					   						 co->priv->widgets.lib_entry,
											 LIB_COLUMN));
	clist = GTK_TREE_VIEW (co->priv->widgets.lib_stock_clist);
	selection = gtk_tree_view_get_selection (clist);
	g_signal_connect (G_OBJECT (selection), "changed",
					  G_CALLBACK (on_update_selection_changed),
					  entry_signal_info_new (co, co->priv->widgets.lib_stock_clist,
					   						 co->priv->widgets.lib_entry,
											 LIB_STOCK_COLUMN));
	clist = GTK_TREE_VIEW (co->priv->widgets.lib_paths_clist);
	selection = gtk_tree_view_get_selection (clist);
	g_signal_connect (G_OBJECT (selection), "changed",
					  G_CALLBACK (on_update_selection_changed),
					  entry_signal_info_new (co, co->priv->widgets.lib_paths_clist,
					   						 co->priv->widgets.lib_paths_entry,
											 LIB_PATHS_COLUMN));
	clist = GTK_TREE_VIEW (co->priv->widgets.def_clist);
	selection = gtk_tree_view_get_selection (clist);
	g_signal_connect (G_OBJECT (selection), "changed",
					  G_CALLBACK (on_update_selection_changed),
					  entry_signal_info_new (co, co->priv->widgets.def_clist,
					   						 co->priv->widgets.def_entry,
											 DEF_DEFINE_COLUMN));
	clist = GTK_TREE_VIEW (co->priv->widgets.def_stock_clist);
	selection = gtk_tree_view_get_selection (clist);
	g_signal_connect (G_OBJECT (selection), "changed",
					  G_CALLBACK (on_update_selection_changed),
					  entry_signal_info_new (co, co->priv->widgets.def_stock_clist,
					   						 co->priv->widgets.def_entry,
											 DEF_STOCK_COLUMN));
	
	g_signal_connect (G_OBJECT (co->priv->widgets.other_c_flags_entry), "changed",
					  G_CALLBACK (on_entry_changed), co);

	g_signal_connect (G_OBJECT (co->priv->widgets.other_l_flags_entry), "changed",
					  G_CALLBACK (on_entry_changed), co);

	g_signal_connect (G_OBJECT (co->priv->widgets.other_l_libs_entry), "changed",
					  G_CALLBACK (on_entry_changed), co);

	/* optimization buttons */
	for (i = 0 ; i < 4 ; i++)											 
		g_signal_connect (GTK_TOGGLE_BUTTON (co->priv->widgets.optimize_button[i]), 
			"toggled", G_CALLBACK (on_button_selection_changed),co);

	/* debug and profile */
	for (i = 0 ; i < 2 ; i++)											 
		g_signal_connect (GTK_TOGGLE_BUTTON (co->priv->widgets.other_button[i]), 
			"toggled", G_CALLBACK (on_button_selection_changed),co);
	
	g_signal_connect (G_OBJECT (co->priv->widgets.other_c_flags_entry), "changed",
					  G_CALLBACK (on_entry_changed), co);

	g_signal_connect (G_OBJECT (co->priv->widgets.other_l_flags_entry), "changed",
					  G_CALLBACK (on_entry_changed), co);

	g_signal_connect (G_OBJECT (co->priv->widgets.other_l_libs_entry), "changed",
					  G_CALLBACK (on_entry_changed), co);

	/* optimization buttons */
	for (i = 0 ; i < 4 ; i++)											 
		g_signal_connect (GTK_TOGGLE_BUTTON (co->priv->widgets.optimize_button[i]), 
			"toggled", G_CALLBACK (on_button_selection_changed),co);

	/* debug and profile */
	for (i = 0 ; i < 2 ; i++)											 
		g_signal_connect (GTK_TOGGLE_BUTTON (co->priv->widgets.other_button[i]), 
			"toggled", G_CALLBACK (on_button_selection_changed),co);
	
	g_signal_connect (G_OBJECT (co->priv->widgets.other_c_flags_entry), "changed",
					  G_CALLBACK (on_entry_changed), co);

	g_signal_connect (G_OBJECT (co->priv->widgets.other_l_flags_entry), "changed",
					  G_CALLBACK (on_entry_changed), co);

	g_signal_connect (G_OBJECT (co->priv->widgets.other_l_libs_entry), "changed",
					  G_CALLBACK (on_entry_changed), co);

	/* optimization buttons */
	for (i = 0 ; i < 4 ; i++)											 
		g_signal_connect (GTK_TOGGLE_BUTTON (co->priv->widgets.optimize_button[i]), 
			"toggled", G_CALLBACK (on_button_selection_changed),co);

	/* debug and profile */
	for (i = 0 ; i < 2 ; i++)											 
		g_signal_connect (GTK_TOGGLE_BUTTON (co->priv->widgets.other_button[i]), 
			"toggled", G_CALLBACK (on_button_selection_changed),co);
	
	/* Connect editiong button signals */
	button = glade_xml_get_widget (co->priv->gxml, "inc_add_b");
	BUTTON_SIGNAL_CONNECT (button, on_add_to_clist_clicked,
						   entry_signal_info_new (co, co->priv->widgets.inc_clist,
						   						  co->priv->widgets.inc_entry,
												  INC_PATHS_COLUMN));
	button = glade_xml_get_widget (co->priv->gxml, "inc_update_b");
	BUTTON_SIGNAL_CONNECT (button, on_update_in_clist_clicked,
						   entry_signal_info_new (co, co->priv->widgets.inc_clist,
						   						  co->priv->widgets.inc_entry,
												  INC_PATHS_COLUMN));
	button = glade_xml_get_widget (co->priv->gxml, "inc_remove_b");
	BUTTON_SIGNAL_CONNECT (button, on_remove_from_clist_clicked,
						   entry_signal_info_new (co, co->priv->widgets.inc_clist,
						   						  co->priv->widgets.inc_entry,
												  INC_PATHS_COLUMN));
	button = glade_xml_get_widget (co->priv->gxml, "inc_clear_b");
	BUTTON_SIGNAL_CONNECT (button, on_clear_clist_clicked,
						   entry_signal_info_new (co, co->priv->widgets.inc_clist,
						   						  co->priv->widgets.inc_entry,
												  INC_PATHS_COLUMN));
	
	button = glade_xml_get_widget (co->priv->gxml, "lib_add_b");
	BUTTON_SIGNAL_CONNECT (button, on_add_to_clist_clicked,
						   entry_signal_info_new (co, co->priv->widgets.lib_clist,
						   						  co->priv->widgets.lib_entry,
												  LIB_COLUMN));
	button = glade_xml_get_widget (co->priv->gxml, "lib_update_b");
	BUTTON_SIGNAL_CONNECT (button, on_update_in_clist_clicked,
						   entry_signal_info_new (co, co->priv->widgets.lib_clist,
						   						  co->priv->widgets.lib_entry,
												  LIB_COLUMN));
	button = glade_xml_get_widget (co->priv->gxml, "lib_remove_b");
	BUTTON_SIGNAL_CONNECT (button, on_remove_from_clist_clicked,
						   entry_signal_info_new (co, co->priv->widgets.lib_clist,
						   						  co->priv->widgets.lib_entry,
												  LIB_COLUMN));
	button = glade_xml_get_widget (co->priv->gxml, "lib_clear_b");
	BUTTON_SIGNAL_CONNECT (button, on_clear_clist_clicked,
						   entry_signal_info_new (co, co->priv->widgets.lib_clist,
						   						  co->priv->widgets.lib_entry,
												  LIB_COLUMN));
	
	button = glade_xml_get_widget (co->priv->gxml, "lib_paths_add_b");
	BUTTON_SIGNAL_CONNECT (button, on_add_to_clist_clicked,
						   entry_signal_info_new (co, co->priv->widgets.lib_paths_clist,
						   						  co->priv->widgets.lib_paths_entry,
												  LIB_PATHS_COLUMN));
	button = glade_xml_get_widget (co->priv->gxml, "lib_paths_update_b");
	BUTTON_SIGNAL_CONNECT (button, on_update_in_clist_clicked,
						   entry_signal_info_new (co, co->priv->widgets.lib_paths_clist,
						   						  co->priv->widgets.lib_paths_entry,
												  LIB_PATHS_COLUMN));
	button = glade_xml_get_widget (co->priv->gxml, "lib_paths_remove_b");
	BUTTON_SIGNAL_CONNECT (button, on_remove_from_clist_clicked,
						   entry_signal_info_new (co, co->priv->widgets.lib_paths_clist,
						   						  co->priv->widgets.lib_paths_entry,
												  LIB_PATHS_COLUMN));
	button = glade_xml_get_widget (co->priv->gxml, "lib_paths_clear_b");
	BUTTON_SIGNAL_CONNECT (button, on_clear_clist_clicked,
						   entry_signal_info_new (co, co->priv->widgets.lib_paths_clist,
						   						  co->priv->widgets.lib_paths_entry,
												  LIB_PATHS_COLUMN));
	
	button = glade_xml_get_widget (co->priv->gxml, "def_add_b");
	BUTTON_SIGNAL_CONNECT (button, on_add_to_clist_clicked,
						   entry_signal_info_new (co, co->priv->widgets.def_clist,
						   						  co->priv->widgets.def_entry,
												  DEF_DEFINE_COLUMN));
	button = glade_xml_get_widget (co->priv->gxml, "def_update_b");
	BUTTON_SIGNAL_CONNECT (button, on_update_in_clist_clicked,
						   entry_signal_info_new (co, co->priv->widgets.def_clist,
						   						  co->priv->widgets.def_entry,
												  DEF_DEFINE_COLUMN));
	button = glade_xml_get_widget (co->priv->gxml, "lib_paths_remove_b");
	BUTTON_SIGNAL_CONNECT (button, on_remove_from_clist_clicked,
						   entry_signal_info_new (co, co->priv->widgets.def_clist,
						   						  co->priv->widgets.def_entry,
												  DEF_DEFINE_COLUMN));
	button = glade_xml_get_widget (co->priv->gxml, "lib_paths_clear_b");
	BUTTON_SIGNAL_CONNECT (button, on_clear_clist_clicked,
						   entry_signal_info_new (co, co->priv->widgets.def_clist,
						   						  co->priv->widgets.def_entry,
												  DEF_DEFINE_COLUMN));
	gtk_widget_ref (co->priv->widgets.window);
	gtk_widget_ref (co->priv->widgets.warnings_clist);
	for (i = 0; i < 4; i++)
		gtk_widget_ref (co->priv->widgets.optimize_button[i]);
	for (i = 0; i < 2; i++)
		gtk_widget_ref (co->priv->widgets.other_button[i]);
	
	gtk_widget_ref (co->priv->widgets.inc_clist);
	gtk_widget_ref (co->priv->widgets.inc_entry);
	gtk_widget_ref (co->priv->widgets.lib_paths_clist);
	gtk_widget_ref (co->priv->widgets.lib_paths_entry);
	gtk_widget_ref (co->priv->widgets.lib_clist);
	gtk_widget_ref (co->priv->widgets.lib_stock_clist);
	gtk_widget_ref (co->priv->widgets.lib_entry);
	gtk_widget_ref (co->priv->widgets.def_clist);
	gtk_widget_ref (co->priv->widgets.def_stock_clist);
	gtk_widget_ref (co->priv->widgets.def_entry);
	gtk_widget_ref (co->priv->widgets.other_c_flags_entry);
	gtk_widget_ref (co->priv->widgets.other_l_flags_entry);
	gtk_widget_ref (co->priv->widgets.other_l_libs_entry);
	gtk_widget_ref (co->priv->widgets.supp_clist);
}

CompilerOptions *
compiler_options_new (PropsID props)
{
	int i;
	CompilerOptions *co = g_new0 (CompilerOptions, 1);
	
	co->priv = g_new0 (CompilerOptionsPriv, 1);
	co->priv->is_showing = FALSE;
	co->priv->win_pos_x = 100;
	co->priv->win_pos_y = 100;
	co->priv->props = props;
	create_compiler_options_gui (co);
	co->priv->dirty = FALSE;
	
	return co;
}

void
compiler_options_destroy (CompilerOptions * co)
{
	gint i;
	g_return_if_fail (co);

	gtk_widget_unref (co->priv->widgets.window);
	gtk_widget_unref (co->priv->widgets.warnings_clist);
	for (i = 0; i < 4; i++)
		gtk_widget_unref (co->priv->widgets.optimize_button[i]);
	for (i = 0; i < 2; i++)
		gtk_widget_unref (co->priv->widgets.other_button[i]);

	gtk_widget_unref (co->priv->widgets.inc_clist);
	gtk_widget_unref (co->priv->widgets.inc_entry);
	gtk_widget_unref (co->priv->widgets.lib_paths_clist);
	gtk_widget_unref (co->priv->widgets.lib_paths_entry);
	gtk_widget_unref (co->priv->widgets.lib_clist);
	gtk_widget_unref (co->priv->widgets.lib_stock_clist);
	gtk_widget_unref (co->priv->widgets.lib_entry);
	gtk_widget_unref (co->priv->widgets.def_clist);
	gtk_widget_unref (co->priv->widgets.def_stock_clist);
	gtk_widget_unref (co->priv->widgets.def_entry);
	gtk_widget_unref (co->priv->widgets.other_c_flags_entry);
	gtk_widget_unref (co->priv->widgets.other_l_flags_entry);
	gtk_widget_unref (co->priv->widgets.other_l_libs_entry);
	gtk_widget_unref (co->priv->widgets.supp_clist);

	if (co->priv->widgets.window)
		gtk_widget_destroy (co->priv->widgets.window);
	g_object_unref (co->priv->gxml);
	
	g_free (co->priv);
	g_free (co);
}

gboolean compiler_options_save_yourself (CompilerOptions * co, FILE * stream)
{
	if (stream == NULL || co == NULL)
		return FALSE;
	fprintf (stream, "compiler.options.win.pos.x=%d\n", co->priv->win_pos_x);
	fprintf (stream, "compiler.options.win.pos.y=%d\n", co->priv->win_pos_y);
	return TRUE;
}

gint compiler_options_save (CompilerOptions * co, FILE * s)
{
	const gchar *text;
	gint i;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gboolean valid;
	
	g_return_val_if_fail (co != NULL, FALSE);
	g_return_val_if_fail (s != NULL, FALSE);

	/* Save support list */
	model =
		gtk_tree_view_get_model (GTK_TREE_VIEW(co->priv->widgets.supp_clist));
	valid = gtk_tree_model_get_iter_first (model, &iter);
	fprintf (s, "compiler.options.supports=");
	while (valid)
	{
		gboolean state;
		gchar *name;
		gtk_tree_model_get (model, &iter, SUPP_TOGGLE_COLUMN, &state,
							SUPP_NAME_COLUMN, &name, -1);
		if(state)
		{
			if (fprintf (s, "%s ", name) <1)
				return FALSE;
		}
		valid = gtk_tree_model_iter_next (model, &iter);
	}
	fprintf (s, "\n");
	
	/* Save include paths list */
	model =
		gtk_tree_view_get_model (GTK_TREE_VIEW(co->priv->widgets.inc_clist));
	valid = gtk_tree_model_get_iter_first (model, &iter);
	fprintf (s, "compiler.options.include.paths=");
	while (valid)
	{
		gchar *path;
		gtk_tree_model_get (model, &iter, INC_PATHS_COLUMN, &path, -1);
		if (fprintf (s, "\\\n\t%s", path) <1)
			return FALSE;
		valid = gtk_tree_model_iter_next (model, &iter);
	}
	fprintf (s, "\n");

	/* Save library paths list */
	model =
		gtk_tree_view_get_model (GTK_TREE_VIEW(co->priv->widgets.lib_paths_clist));
	valid = gtk_tree_model_get_iter_first (model, &iter);
	fprintf (s, "compiler.options.library.paths=");
	while (valid)
	{
		gchar *path;
		gtk_tree_model_get (model, &iter, LIB_PATHS_COLUMN, &path, -1);
		if (fprintf (s, "\\\n\t%s", path) <1)
			return FALSE;
		valid = gtk_tree_model_iter_next (model, &iter);
	}
	fprintf (s, "\n");

	/* Save libraries list */
	model =
		gtk_tree_view_get_model (GTK_TREE_VIEW(co->priv->widgets.lib_clist));
	valid = gtk_tree_model_get_iter_first (model, &iter);
	fprintf (s, "compiler.options.libraries=");
	while (valid)
	{
		gchar *name;
		gtk_tree_model_get (model, &iter, LIB_COLUMN, &name, -1);
		if (fprintf (s, "\\\n\t%s", name) <1)
			return FALSE;
		valid = gtk_tree_model_iter_next (model, &iter);
	}
	fprintf (s, "\n");
	

	/* Save selected libraries list */
	model =
		gtk_tree_view_get_model (GTK_TREE_VIEW(co->priv->widgets.lib_clist));
	valid = gtk_tree_model_get_iter_first (model, &iter);
	fprintf (s, "compiler.options.libraries.selected=");
	while (valid)
	{
		gboolean state;
		gtk_tree_model_get (model, &iter, LIB_TOGGLE_COLUMN, &state, -1);
		if (fprintf (s, "%d ", state) <1)
			return FALSE;
		valid = gtk_tree_model_iter_next (model, &iter);
	}
	fprintf (s, "\n");
	
	/* Save defines list */
	model =
		gtk_tree_view_get_model (GTK_TREE_VIEW(co->priv->widgets.def_clist));
	valid = gtk_tree_model_get_iter_first (model, &iter);
	fprintf (s, "compiler.options.defines=");
	while (valid)
	{
		gchar *name;
		gtk_tree_model_get (model, &iter, DEF_DEFINE_COLUMN, &name, -1);
		if (fprintf (s, "\\\n\t%s", name) <1)
			return FALSE;
		valid = gtk_tree_model_iter_next (model, &iter);
	}
	fprintf (s, "\n");
	

	/* Save selected defines list */
	model =
		gtk_tree_view_get_model (GTK_TREE_VIEW(co->priv->widgets.lib_clist));
	valid = gtk_tree_model_get_iter_first (model, &iter);
	fprintf (s, "compiler.options.defines.selected=");
	while (valid)
	{
		gboolean state;
		gtk_tree_model_get (model, &iter, DEF_TOGGLE_COLUMN, &state, -1);
		if (fprintf (s, "%d ", state) <1)
			return FALSE;
		valid = gtk_tree_model_iter_next (model, &iter);
	}
	fprintf (s, "\n");
	

	/* Save warnings list */
	model =
		gtk_tree_view_get_model (GTK_TREE_VIEW(co->priv->widgets.warnings_clist));
	valid = gtk_tree_model_get_iter_first (model, &iter);
	fprintf (s, "compiler.options.warning.buttons=");
	while (valid)
	{
		gboolean state;
		gtk_tree_model_get (model, &iter, WARNINGS_TOGGLE_COLUMN, &state, -1);
		if (fprintf (s, "%d ", state) <1)
			return FALSE;
		valid = gtk_tree_model_iter_next (model, &iter);
	}
	fprintf (s, "\n");

	fprintf (s, "compiler.options.optimize.buttons=");
	for (i = 0; i < 4; i++)
	{
		gboolean state;
		state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(co->priv->
											 widgets.optimize_button[i]));
		fprintf (s, "%d ", (int) state);
	}
	fprintf (s, "\n");

	fprintf (s, "compiler.options.other.buttons=");
	for (i = 0; i < 2; i++)
	{
		gboolean state;
		state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(co->priv->
											 widgets.other_button[i]));
		fprintf (s, "%d ", (int) state);
	}
	fprintf (s, "\n");

	text = gtk_entry_get_text (GTK_ENTRY (co->priv->widgets.other_c_flags_entry));
	fprintf (s, "compiler.options.other.c.flags=%s\n", text);
	
	text = gtk_entry_get_text (GTK_ENTRY (co->priv->widgets.other_l_flags_entry));
	fprintf (s, "compiler.options.other.l.flags=%s\n", text);
	
	text = gtk_entry_get_text (GTK_ENTRY (co->priv->widgets.other_l_libs_entry));
	fprintf (s, "compiler.options.other.l.libs=%s\n", text);

	fprintf (s, "\n");
	return TRUE;
}

gboolean compiler_options_load_yourself (CompilerOptions * co, PropsID props)
{
	if (co == NULL)
		return FALSE;
	co->priv->win_pos_x =
		prop_get_int (props, "compiler.options.win.pos.x", 100);
	co->priv->win_pos_y =
		prop_get_int (props, "compiler.options.win.pos.y", 100);
	return TRUE;
}

#define CLIST_CLEAR_ALL(widget) \
	model = gtk_tree_view_get_model (GTK_TREE_VIEW(widget)); \
	g_assert (model); \
	if (model) gtk_list_store_clear (GTK_LIST_STORE (model));

#define CLIST_APPEND_STRING_ALL(property, widget, col) \
	list = glist_from_data (props, property); \
	model = gtk_tree_view_get_model (GTK_TREE_VIEW(widget)); \
	g_assert (model); \
	node = list; \
	while (node) \
	{ \
		gtk_list_store_append (GTK_LIST_STORE(model), &iter); \
		gtk_list_store_set (GTK_LIST_STORE(model), \
							&iter, col, node->data, -1); \
		node = g_list_next (node); \
	} \
	glist_strings_free (list);

#define CLIST_UPDATE_BOOLEAN_ALL(property, widget, col) \
	list = glist_from_data (props, property); \
	model = gtk_tree_view_get_model (GTK_TREE_VIEW(widget)); \
	g_assert (model); \
	valid = gtk_tree_model_get_iter_first (model, &iter); \
	node = list; \
	while (node && valid) \
	{ \
		int value = atoi(node->data); \
		gtk_list_store_set (GTK_LIST_STORE(model), &iter, col, value, -1); \
		node = g_list_next (node); \
		valid = gtk_tree_model_iter_next (model, &iter); \
	} \
	glist_strings_free (list);


void
compiler_options_clear(CompilerOptions *co)
{
	gint i;
	GtkTreeModel* model;
	GtkTreeIter iter;
	gboolean valid;

	g_return_if_fail (co != NULL);

	gtk_widget_set_sensitive (co->priv->widgets.supp_clist, TRUE);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW(co->priv->widgets.supp_clist));
	g_assert (model);
	
	/* Clear all supports */
	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid)
	{
		gtk_list_store_set (GTK_LIST_STORE(model), &iter,
							SUPP_TOGGLE_COLUMN, FALSE, -1);
		valid = gtk_tree_model_iter_next (model, &iter);
	}

	CLIST_CLEAR_ALL(co->priv->widgets.inc_clist);
	CLIST_CLEAR_ALL(co->priv->widgets.lib_paths_clist);
	CLIST_CLEAR_ALL(co->priv->widgets.lib_clist);
	CLIST_CLEAR_ALL(co->priv->widgets.def_clist);
	
	gtk_entry_set_text (GTK_ENTRY (co->priv->widgets.inc_entry), "");
	gtk_entry_set_text (GTK_ENTRY (co->priv->widgets.lib_paths_entry), "");
	gtk_entry_set_text (GTK_ENTRY (co->priv->widgets.lib_entry), "");
	gtk_entry_set_text (GTK_ENTRY (co->priv->widgets.def_entry), "");
	
	for (i = 0; i < 4; i++)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(co->priv->widgets.optimize_button[i]), FALSE);
	for (i = 0; i < 2; i++)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(co->priv->widgets.other_button[i]), FALSE);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW(co->priv->widgets.warnings_clist));
	g_assert (model);
	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid)
	{
		gtk_list_store_set (GTK_LIST_STORE(model), &iter, 0, FALSE, -1);
		valid = gtk_tree_model_iter_next (model, &iter);
	}

	gtk_entry_set_text (GTK_ENTRY(co->priv->widgets.other_c_flags_entry), "");
	gtk_entry_set_text (GTK_ENTRY(co->priv->widgets.other_l_flags_entry), "");
	gtk_entry_set_text (GTK_ENTRY(co->priv->widgets.other_l_libs_entry), "");
	compiler_options_set_in_properties(co, co->priv->props);
}

#define ASSIGN_PROPERTY_VALUE_TO_ENTRY(props, key, entry) \
	{ \
		gchar *str = prop_get ((props), (key)); \
		if (str) { \
			gtk_entry_set_text (GTK_ENTRY((entry)), str); \
			g_free (str); \
		} else { \
			gtk_entry_set_text (GTK_ENTRY((entry)), ""); \
		} \
	}

void
compiler_options_load (CompilerOptions * co, PropsID props)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GList *list, *node;
	gboolean valid;
	gint i;
	
	g_return_if_fail (co != NULL);

	compiler_options_clear (co);

	/* Read all available supports for the project */
	list = glist_from_data (props, "compiler.options.supports");
	node = list;
	while (node)
	{
		if (node->data)
		{
			GtkTreeView *clist;
			
			clist = GTK_TREE_VIEW(co->priv->widgets.supp_clist);
			model = gtk_tree_view_get_model (clist);
			g_assert (model);
			valid = gtk_tree_model_get_iter_first (model, &iter);
			while (valid)
			{
				gchar *name;
				gtk_tree_model_get (model, &iter, SUPP_NAME_COLUMN, &name, -1);
				if (strcasecmp (name, node->data)==0)
				{
					gtk_list_store_set (GTK_LIST_STORE(model), &iter,
										SUPP_TOGGLE_COLUMN, TRUE, -1);
					break;
				}
				valid = gtk_tree_model_iter_next (model, &iter);
			}
		}
		node = g_list_next (node);
	}
	glist_strings_free (list);
	
	/* For now, disable supports for projects. */
	if (app->project_dbase->project_is_open == TRUE)
	{
		gtk_widget_set_sensitive (co->priv->widgets.supp_clist, FALSE);
	}

	CLIST_APPEND_STRING_ALL ("compiler.options.include.paths",
							 co->priv->widgets.inc_clist, INC_PATHS_COLUMN);
	CLIST_APPEND_STRING_ALL ("compiler.options.library.paths",
							 co->priv->widgets.lib_paths_clist,
							 LIB_PATHS_COLUMN);
	CLIST_APPEND_STRING_ALL ("compiler.options.libraries",
							 co->priv->widgets.lib_clist, LIB_COLUMN);
	CLIST_UPDATE_BOOLEAN_ALL ("compiler.options.libraries.selected",
							 co->priv->widgets.lib_clist, LIB_TOGGLE_COLUMN);
	CLIST_APPEND_STRING_ALL ("compiler.options.defines",
							 co->priv->widgets.def_clist, DEF_DEFINE_COLUMN);
	CLIST_UPDATE_BOOLEAN_ALL ("compiler.options.defines.selected",
							 co->priv->widgets.def_clist, DEF_TOGGLE_COLUMN);
	CLIST_UPDATE_BOOLEAN_ALL ("compiler.options.warning.buttons",
							  co->priv->widgets.warnings_clist,
							  WARNINGS_TOGGLE_COLUMN);
	
	list = glist_from_data (props, "compiler.options.optimize.buttons");
	node = list;
	i = 0;
	while (node)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(co->priv->
									 widgets.optimize_button[i]),
			atoi(node->data));
		node = g_list_next (node);
		i++;
	}
	glist_strings_free (list);
	
	list = glist_from_data (props, "compiler.options.other.buttons");
	node = list;
	i = 0;
	while (node) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(co->priv->
									 widgets.other_button[i]),
			atoi(node->data));
		node = g_list_next (node);
		i++;
	}
	glist_strings_free (list);

	ASSIGN_PROPERTY_VALUE_TO_ENTRY(props, "compiler.options.other.c.flags",
								   co->priv->widgets.other_c_flags_entry);
	ASSIGN_PROPERTY_VALUE_TO_ENTRY(props, "compiler.options.other.l.flags",
								   co->priv->widgets.other_l_flags_entry);
	ASSIGN_PROPERTY_VALUE_TO_ENTRY(props, "compiler.options.other.l.libs",
								   co->priv->widgets.other_l_libs_entry);
	
	compiler_options_set_in_properties (co, co->priv->props);
	
	co->priv->dirty = FALSE;
}

void
compiler_options_show (CompilerOptions * co)
{
	if (co->priv->is_showing)
	{
		gdk_window_raise (co->priv->widgets.window->window);
		return;
	}
	gtk_widget_set_uposition (co->priv->widgets.window, co->priv->win_pos_x,
							  co->priv->win_pos_y);
	gtk_widget_show (co->priv->widgets.window);
	co->priv->is_showing = TRUE;
}

void
compiler_options_hide (CompilerOptions * co)
{
	if (!co)
		return;
	if (co->priv->is_showing == FALSE)
		return;
	gdk_window_get_root_origin (co->priv->widgets.window->window,
				    &co->priv->win_pos_x, &co->priv->win_pos_y);
	gtk_widget_hide (co->priv->widgets.window);
	co->priv->is_showing = FALSE;
}

#define GET_ALL_STRING_DATA(widget, str, separator, col) \
{ \
	GtkTreeModel *model; \
	GtkTreeIter iter; \
	gboolean valid; \
	gchar* tmp; \
	model = gtk_tree_view_get_model (GTK_TREE_VIEW(widget)); \
	g_assert(model); \
	valid = gtk_tree_model_get_iter_first(model, &iter); \
	while (valid) \
	{ \
		gchar *text; \
		gtk_tree_model_get (model, &iter, col, &text, -1); \
		tmp = g_strconcat (str, separator, text, NULL); \
		g_free (str); \
		str = tmp; \
		valid = gtk_tree_model_iter_next (model, &iter); \
	} \
}

static gchar *
get_supports (CompilerOptions *co, gint item, gchar *separator)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;
	gchar *str;
	gchar *pkg_modules;
	gint i;
	gboolean has_pkg_modules = FALSE;
	
	str = g_strdup ("");
	pkg_modules = g_strdup ("");
	
	model =
		gtk_tree_view_get_model (GTK_TREE_VIEW(co->priv->widgets.supp_clist));
	g_assert(model);
	valid = gtk_tree_model_get_iter_first(model, &iter);
	i = 0;
	while (valid)
	{
		gboolean value, pkgconfig;
		gchar *text, *tmp, *name;
		
		gtk_tree_model_get (model, &iter,
							SUPP_TOGGLE_COLUMN, &value,
							SUPP_PKGCONFIG_COLUMN, &pkgconfig,
							SUPP_NAME_COLUMN, &name, -1);
		valid = gtk_tree_model_iter_next (model, &iter);
		if (!value)
		{
			i++;
			continue;
		}
		if (!pkgconfig)
		{
			text = anjuta_supports [i][item];
			tmp = g_strconcat (str, text, separator, NULL);
			g_free (str);
			str = tmp;
		}
		else
		{
			has_pkg_modules = TRUE;
			tmp = pkg_modules;
			pkg_modules = g_strconcat (tmp, " ", name, NULL);
			g_free (tmp);
		}
		if (name)
			g_free(name);
		i++;
	}
	if (has_pkg_modules)
	{
		gchar *text, *tmp;
		
		switch (item)
		{
		case ANJUTA_SUPPORT_FILE_CFLAGS:
			text = g_strconcat ("`pkg-config --cflags", pkg_modules, "`", NULL);
			break;
		case ANJUTA_SUPPORT_FILE_LIBS:
			text = g_strconcat ("`pkg-config --libs", pkg_modules, "`", NULL);
			break;
		case ANJUTA_SUPPORT_PRJ_CFLAGS:
			text = g_strconcat ("`pkg-config --cflags", pkg_modules, "`", NULL);
			break;
		case ANJUTA_SUPPORT_PRJ_LIBS:
			text = g_strconcat ("`pkg-config --libs", pkg_modules, "`", NULL);
			break;
		case ANJUTA_SUPPORT_MACROS:
			text = g_strdup (""); //FIXME.
			break;
		}
		tmp = str;
		str = g_strconcat (tmp, " ", text, NULL);
		g_free (tmp);
		g_free (text);
	}
	g_free (pkg_modules);
	return str;
}

static gchar *
get_include_paths (CompilerOptions * co, gboolean with_support)
{
	gchar *str;

	str = g_strdup ("");
	GET_ALL_STRING_DATA (co->priv->widgets.inc_clist,
						str, " -I", INC_PATHS_COLUMN);
	
	if (with_support)
	{
		gchar *supports = get_supports (co, ANJUTA_SUPPORT_FILE_CFLAGS, " ");
		gchar *tmp = g_strconcat (str, supports, NULL);
		g_free (str);
		g_free (supports);
		str = tmp;
	}
	return str;
}

static gchar *
get_library_paths (CompilerOptions * co)
{
	gchar *str;

	str = g_strdup ("");
	GET_ALL_STRING_DATA (co->priv->widgets.lib_paths_clist, str,
						 " -L", LIB_PATHS_COLUMN);
	return str;
}

static gchar *
get_libraries (CompilerOptions * co, gboolean with_support)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;
	gchar *str, *tmp;
	gchar *text;

	str = g_strdup ("");
	model = gtk_tree_view_get_model (GTK_TREE_VIEW(co->priv->widgets.lib_clist));
	g_assert(model);
	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid)
	{
		gboolean value;
		
		gtk_tree_model_get (model, &iter, LIB_TOGGLE_COLUMN,
							&value, LIB_COLUMN, &text, -1);
		valid = gtk_tree_model_iter_next (model, &iter);
		if (!value)
			continue;
		
		/* If it starts with '*' then consider it as an object file
		 * and not a lib file
		 */
		if (*text == '*')
			/* Remove the '*' and put the rest */
			tmp = g_strconcat (str, " ", &text[1], NULL);
		else /* Lib file */
			tmp = g_strconcat (str, " -l", text, NULL);
		g_free (str);
		str = tmp;
	}
	if (with_support)
	{
		gchar *supports = get_supports (co, ANJUTA_SUPPORT_FILE_LIBS, " ");
		gchar *tmp = g_strconcat (str, supports, NULL);
		g_free (str);
		g_free (supports);
		str = tmp;
	}
	return str;
}

static gchar *
get_defines (CompilerOptions * co)
{
	gchar *str;
	str = g_strdup ("");
	GET_ALL_STRING_DATA (co->priv->widgets.def_clist, str,
						 " -D", DEF_DEFINE_COLUMN);
	return str;
}

static gchar*
get_warnings(CompilerOptions *co)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;
	gchar *str, *tmp;
	gint i;
	
	str = g_strdup ("");
	model = gtk_tree_view_get_model (GTK_TREE_VIEW(co->priv->
									 widgets.warnings_clist));
	g_assert(model);
	valid = gtk_tree_model_get_iter_first (model, &iter);
	i = 0;
	while (valid)
	{
		gboolean value;
		gtk_tree_model_get (model, &iter, WARNINGS_TOGGLE_COLUMN,
							&value, -1);
		valid = gtk_tree_model_iter_next (model, &iter);
		if (!value)
		{
			i += 2;
			continue;
		}
		tmp = str;
		str = g_strconcat (tmp, anjuta_warnings[i], NULL);
		g_free (tmp);
		i += 2;
	}
	return str;
}

static gchar*
get_optimization (CompilerOptions *co)
{
	gchar *str;
	gint i;
	
	str = g_strdup ("");
	for (i = 0; i < 4; i++)
	{
		gboolean state;
		gchar *tmp;
		GtkToggleButton *b;

		b= GTK_TOGGLE_BUTTON (co->priv->widgets.optimize_button[i]);
		state = gtk_toggle_button_get_active (b);
		if (state)
		{
			tmp = str;
			str = g_strconcat (tmp, optimize_button_option[i], NULL);
			g_free (tmp);
		}
	}
	return str;
}

static gchar*
get_others (CompilerOptions *co)
{
	gchar *str;
	gint i;
	
	str = g_strdup ("");
	for (i = 0; i < 2; i++)
	{
		gboolean state;
		gchar *tmp;
		GtkToggleButton *b;
		
		b = GTK_TOGGLE_BUTTON (co->priv->widgets.other_button[i]);
		state = gtk_toggle_button_get_active (b);
		if (state)
		{
			tmp = str;
			str = g_strconcat (tmp, other_button_option[i], NULL);
			g_free (tmp);
		}
	}
	return str;
}

void
compiler_options_set_in_properties (CompilerOptions * co, PropsID props)
{
	gchar *buff;

	buff = get_include_paths (co, TRUE);
	prop_set_with_key (props, "anjuta.compiler.includes", buff);
	g_free (buff);

	buff = get_library_paths (co);
	prop_set_with_key (props, "anjuta.linker.library.paths", buff);
	g_free (buff);

	buff = get_libraries (co, TRUE);
	prop_set_with_key (props, "anjuta.linker.libraries", buff);
	g_free (buff);

	buff = get_defines (co);
	prop_set_with_key (props, "anjuta.compiler.defines", buff);
	g_free (buff);

	buff = get_warnings(co);
	prop_set_with_key (props, "anjuta.compiler.warning.flags", buff);
	g_free (buff);
	
	
	buff = get_optimization(co);
	prop_set_with_key (props, "anjuta.compiler.optimization.flags", buff);
	g_free (buff);

	buff = get_others(co);
	prop_set_with_key (props, "anjuta.compiler.other.flags", buff);
	g_free (buff);
	
	prop_set_with_key (props, "anjuta.compiler.additional.flags",
		gtk_entry_get_text (GTK_ENTRY (co->priv->widgets.other_c_flags_entry)));
	prop_set_with_key (props, "anjuta.linker.additional.flags",
		gtk_entry_get_text (GTK_ENTRY (co->priv->widgets.other_l_flags_entry)));
	prop_set_with_key (props, "anjuta.linker.additional.libs",
		gtk_entry_get_text (GTK_ENTRY (co->priv->widgets.other_l_libs_entry)));

	prop_set_with_key (props, "anjuta.compiler.flags",
			   "$(anjuta.compiler.includes) $(anjuta.compiler.defines) "
			   "$(anjuta.compiler.warning.flags) "
			   "$(anjuta.compiler.optimization.flags) "
			   "$(anjuta.compiler.other.flags) "
			   "$(anjuta.compiler.additional.flags)");

	prop_set_with_key (props, "anjuta.linker.flags",
	   "$(anjuta.linker.library.paths) $(anjuta.linker.libraries) "
	   "$(anjuta.linker.additional.flags) $(anjuta.linker.additional.libs)");
}

#define PRINT_TO_STREAM(stream, format, buff) \
	if (buff) \
	{ \
		if (strlen (buff)) \
		{ \
			fprintf (fp, "\\\n\t%s", buff); \
		} \
	}

#define PRINT_TO_STREAM_AND_FREE(stream, format, buff) \
	if (buff) \
	{ \
		if (strlen (buff)) \
		{ \
			fprintf (fp, "\\\n\t%s", buff); \
		} \
		g_free (buff); \
	}

void
compiler_options_set_prjincl_in_file (CompilerOptions* co, FILE* fp)
{
	gchar *buff;
	buff = get_include_paths (co, FALSE);
	PRINT_TO_STREAM_AND_FREE(fp, "\\\n\t%s", buff);
}

void
compiler_options_set_prjcflags_in_file (CompilerOptions * co, FILE* fp)
{
	gchar *buff;
/*
	buff = get_supports (co, ANJUTA_SUPPORT_PRJ_CFLAGS, " ");
	PRINT_TO_STREAM_AND_FREE (fp, "\\\n\t%s", buff);
*/
	buff = get_defines (co);
	PRINT_TO_STREAM_AND_FREE (fp, "\\\n\t%s", buff);
	
	buff = (gchar*)gtk_entry_get_text (GTK_ENTRY(co->priv->widgets.other_c_flags_entry));
	PRINT_TO_STREAM (fp, "\\\n\t%s", buff);

	buff = get_warnings(co);
	PRINT_TO_STREAM_AND_FREE(fp, "\\\n\t%s", buff);

	buff = get_optimization(co);
	PRINT_TO_STREAM_AND_FREE(fp, "\\\n\t%s", buff);

	buff = get_others(co);
	PRINT_TO_STREAM_AND_FREE(fp, "\\\n\t%s", buff);
}

void
compiler_options_set_prjlibs_in_file (CompilerOptions * co, FILE* fp)
{
	gchar *buff;
	
	buff = get_libraries (co, FALSE);
	PRINT_TO_STREAM_AND_FREE(fp, "\\\n\t%s", buff);
	
	buff = (gchar*)gtk_entry_get_text (GTK_ENTRY(co->priv->widgets.other_l_libs_entry));
	PRINT_TO_STREAM (fp, "\\\n\t%s", buff);
}

void
compiler_options_set_prjlflags_in_file (CompilerOptions * co, FILE* fp)
{
	gchar *buff;

	buff = (gchar*)gtk_entry_get_text (GTK_ENTRY(co->priv->widgets.other_l_flags_entry));
	PRINT_TO_STREAM (fp, "\\\n\t%s", buff);
	
	buff = get_library_paths (co);
	PRINT_TO_STREAM_AND_FREE (fp, "\\\n\t%s", buff);

/*
	buff = get_supports (co, ANJUTA_SUPPORT_PRJ_LIBS, " ");
	PRINT_TO_STREAM_AND_FREE (fp, "\\\n\t%s", buff);
*/
}

void
compiler_options_set_prjmacros_in_file (CompilerOptions * co, FILE* fp)
{
	gchar* buff;
	
	buff = get_supports (co, ANJUTA_SUPPORT_MACROS, "\n");
	PRINT_TO_STREAM_AND_FREE (fp, "\\\n\t%s", buff);
}


void 
compiler_options_set_dirty_flag (CompilerOptions* co, gboolean is_dirty)
{
	co->priv->dirty = is_dirty;
}
