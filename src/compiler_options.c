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
#include "../pixmaps/list_unselect.xpm"
#include "../pixmaps/list_select.xpm"

#define ANJUTA_SUPPORTS_END_STRING "SUPPORTS_END"
#define ANJUTA_SUPPORTS_END \
  { \
    ANJUTA_SUPPORTS_END_STRING, NULL, NULL, \
    NULL, NULL, NULL, NULL, NULL, NULL, NULL \
  }

gchar *anjuta_supports[][ANJUTA_SUPPORT_END_MARK] = {
	{
	 "GLIB",
	 "Glib library for most C utilities",
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
	 "Gimp Toolkit library for GUI development",
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
	 "GNOME is Computing Made Easy",
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
	 "GNOME Bonobo component for fast integration",
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
	 "C++ Bindings for GTK",
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
	 "C++ bindings for GNOME",
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
	 "C program using libglade",
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
	 "C++ program using wxWindows (wxGTK) toolkit",
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

static char *warning_button_option[] = {
	" -Werror",
	" -w",
	" -Wall",
	" -Wimplicit",
	" -Wreturn-type",
	" -Wunused",
	" -Wswitch",
	" -Wcomment",
	" -Wuninitialized",
	" -Wparentheses",
	" -Wtraditional",
	" -Wshadow",
	" -Wpointer-arith",
	" -Wmissing-prototypes",
	" -Winline",
	" -Woverloaded-virtual"
};

static char *optimize_button_option[] = {
	"",
	" -O1",
	" -O2",
	" -O3"
};

static char *other_button_option[] = {
	" -g",
	" -pg",
};

GdkPixmap *list_unselect_pixmap, *list_select_pixmap;
GdkBitmap *list_unselect_mask, *list_select_mask;

typedef struct _CoClistItemData CoClistItemData;
struct _CoClistItemData
{
	gboolean state;
};



static CoClistItemData* co_cid_new (gboolean state)
{
	CoClistItemData* cid;
	cid =g_malloc (sizeof (CoClistItemData));
	cid->state = state;
	return cid;
}

static void co_cid_destroy (CoClistItemData* cid)
{
	g_return_if_fail (cid != NULL);
	g_free (cid);
}

static void co_cid_set (CoClistItemData* cid, gboolean state)
{
	g_return_if_fail (cid != NULL);
	cid->state = state;
}

static gboolean co_cid_get (CoClistItemData* cid)
{
	g_return_val_if_fail (cid != NULL, FALSE);
	return cid->state;
}

#define co_cid_new_true() co_cid_new (TRUE)
#define co_cid_new_false() co_cid_new (FALSE)

void co_clist_row_data_set_true (GtkCList* clist, gint row)
{
	CoClistItemData* cid;
	cid = co_cid_new_true();
	gtk_clist_set_pixmap(clist, row, 0,
			       list_select_pixmap, list_select_mask);
	gtk_clist_set_row_data_full (clist, row, cid, (GtkDestroyNotify)  co_cid_destroy);
}

void co_clist_row_data_set_false (GtkCList* clist, gint row)
{
	CoClistItemData* cid;
	cid = co_cid_new_false();
	gtk_clist_set_pixmap (clist,  row, 0,
			list_unselect_pixmap, list_unselect_mask);
	gtk_clist_set_row_data_full (clist, row, cid, (GtkDestroyNotify)  co_cid_destroy);
}

void co_clist_row_data_toggle_state (GtkCList* clist, gint row)
{
	CoClistItemData* cid;
	cid = co_cid_new_false();
	if (co_clist_row_data_get_state (clist, row))
		co_clist_row_data_set_false (clist, row);
	else
		co_clist_row_data_set_true (clist, row);
}

gboolean co_clist_row_data_get_state (GtkCList* clist, gint row)
{
	CoClistItemData* cid;
	cid = gtk_clist_get_row_data (clist, row);
	return co_cid_get (cid);
}

void co_clist_row_data_set_state (GtkCList* clist, gint row, gboolean state)
{
	CoClistItemData* cid;
	cid = gtk_clist_get_row_data (clist, row);
	return co_cid_set (cid, state);
}

CompilerOptions *
compiler_options_new (PropsID props)
{
	gint i, ch;
	gchar *stock_file, *dummy[2];
	gchar lib_name[256];
	gchar desbuff[512];
	FILE *fp;

	CompilerOptions *co = g_malloc (sizeof (CompilerOptions));
	if (co)
	{
		co->other_c_flags = g_strdup ("");
		co->other_l_flags = g_strdup ("");
		co->other_l_libs = g_strdup ("");

		for (i = 0; i < 16; i++)
			co->warning_button_state[i] = FALSE;
		for (i = 0; i < 4; i++)
			co->optimize_button_state[i] = FALSE;
		for (i = 0; i < 2; i++)
			co->other_button_state[i] = FALSE;

		co->warning_button_state[2] = TRUE;
		co->optimize_button_state[0] = TRUE;
		co->other_button_state[0] = TRUE;

		co->supp_index = 0;
		co->inc_index = 0;
		co->lib_index = 0;
		co->lib_paths_index = 0;
		co->def_index = 0;

		co->is_showing = FALSE;
		co->win_pos_x = 100;
		co->win_pos_y = 100;
		
		co->props = props;

		create_compiler_options_gui (co);

		list_unselect_pixmap =
			gdk_pixmap_colormap_create_from_xpm_d(NULL,
				gtk_widget_get_colormap(co->widgets.window),
				&list_unselect_mask, NULL, list_unselect_xpm);

		list_select_pixmap =
			gdk_pixmap_colormap_create_from_xpm_d(NULL,
				gtk_widget_get_colormap(co->widgets.window),
				&list_select_mask, NULL, list_select_xpm);

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
			dummy[0] = lib_name;
			dummy[1] = desbuff;
			gtk_clist_append (GTK_CLIST
					  (co->widgets.lib_stock_clist),
					  dummy);
		}

	      down:
		if (fp)
			fclose (fp);

		/* Now setup all the available supports */
		i = 0;
		while (strcmp (anjuta_supports[i][ANJUTA_SUPPORT_ID], ANJUTA_SUPPORTS_END_STRING) != 0)
		{
			gchar *dummy[2];
			gchar *dum = "";

			dummy[0] = dum;
			dummy[1] = anjuta_supports[i][1];
			gtk_clist_append (GTK_CLIST (co->widgets.supp_clist),
					  dummy);
			co_clist_row_data_set_false (GTK_CLIST(co->widgets.supp_clist), 
					  GTK_CLIST(co->widgets.supp_clist)->rows-1);
			i++;
		}
	}
	return co;
}

void
compiler_options_destroy (CompilerOptions * co)
{
	gint i;
	if (co)
	{
		gtk_widget_unref (co->widgets.window);
		for (i = 0; i < 16; i++)
			gtk_widget_unref (co->widgets.warning_button[i]);
		for (i = 0; i < 4; i++)
			gtk_widget_unref (co->widgets.optimize_button[i]);
		for (i = 0; i < 2; i++)
			gtk_widget_unref (co->widgets.other_button[i]);

		gtk_widget_unref (co->widgets.inc_clist);
		gtk_widget_unref (co->widgets.inc_entry);
		gtk_widget_unref (co->widgets.inc_add_b);
		gtk_widget_unref (co->widgets.inc_update_b);
		gtk_widget_unref (co->widgets.inc_remove_b);
		gtk_widget_unref (co->widgets.inc_clear_b);

		gtk_widget_unref (co->widgets.lib_paths_clist);
		gtk_widget_unref (co->widgets.lib_paths_entry);
		gtk_widget_unref (co->widgets.lib_paths_add_b);
		gtk_widget_unref (co->widgets.lib_paths_update_b);
		gtk_widget_unref (co->widgets.lib_paths_remove_b);
		gtk_widget_unref (co->widgets.lib_paths_clear_b);

		gtk_widget_unref (co->widgets.lib_clist);
		gtk_widget_unref (co->widgets.lib_stock_clist);

		gtk_widget_unref (co->widgets.lib_entry);
		gtk_widget_unref (co->widgets.lib_add_b);
		gtk_widget_unref (co->widgets.lib_update_b);
		gtk_widget_unref (co->widgets.lib_remove_b);
		gtk_widget_unref (co->widgets.lib_clear_b);

		gtk_widget_unref (co->widgets.def_clist);
		gtk_widget_unref (co->widgets.def_entry);
		gtk_widget_unref (co->widgets.def_add_b);
		gtk_widget_unref (co->widgets.def_update_b);
		gtk_widget_unref (co->widgets.def_remove_b);
		gtk_widget_unref (co->widgets.def_clear_b);
		gtk_widget_unref (co->widgets.other_c_flags_entry);
		gtk_widget_unref (co->widgets.other_l_flags_entry);
		gtk_widget_unref (co->widgets.other_l_libs_entry);

		gtk_widget_unref (co->widgets.supp_clist);
		gtk_widget_unref (co->widgets.supp_info_b);

		if (co->widgets.window)
			gtk_widget_destroy (co->widgets.window);
		
		if (co->other_c_flags)
			g_free (co->other_c_flags);
		
		if (co->other_l_flags)
			g_free (co->other_l_flags);
		
		if (co->other_l_libs)
			g_free (co->other_l_libs);
		
		g_free (co);
		co = NULL;
	}
}

gboolean compiler_options_save_yourself (CompilerOptions * co, FILE * stream)
{
	if (stream == NULL || co == NULL)
		return FALSE;
	fprintf (stream, "compiler.options.win.pos.x=%d\n", co->win_pos_x);
	fprintf (stream, "compiler.options.win.pos.y=%d\n", co->win_pos_y);
	return TRUE;
}

gint compiler_options_save (CompilerOptions * co, FILE * s)
{
	gchar *text;
	gint length, i;

	g_return_val_if_fail (co != NULL, FALSE);
	g_return_val_if_fail (s != NULL, FALSE);

	length = g_list_length (GTK_CLIST (co->widgets.supp_clist)->row_list);
	fprintf (s, "compiler.options.supports=");
	for (i = 0; i < length; i++)
	{
		gboolean state;
		state = co_clist_row_data_get_state (GTK_CLIST (co->widgets.supp_clist), i);
		if(state)
		{
			if (fprintf (s, "%s ", anjuta_supports[i][ANJUTA_SUPPORT_ID]) <1)
			{
				return FALSE;
			}
		}
	}
	fprintf (s, "\n");
	
	length = g_list_length (GTK_CLIST (co->widgets.inc_clist)->row_list);
	fprintf (s, "compiler.options.include.paths=");
	for (i = 0; i < length; i++)
	{
		gtk_clist_get_text (GTK_CLIST (co->widgets.inc_clist), i, 0, &text);
		if (fprintf (s, "\\\n\t%s", text) <1)
		{
			return FALSE;
		}
	}
	fprintf (s, "\n");

	length =
		g_list_length (GTK_CLIST (co->widgets.lib_paths_clist)->
			       row_list);
	fprintf (s, "compiler.options.library.paths=");
	for (i = 0; i < length; i++)
	{
		gtk_clist_get_text (GTK_CLIST (co->widgets.lib_paths_clist),
				    i, 0, &text);
		if (fprintf (s, "\\\n\t%s", text) <1)
		{
			return FALSE;
		}
	}
	fprintf (s, "\n");

	length = g_list_length (GTK_CLIST (co->widgets.lib_clist)->row_list);
	fprintf (s, "compiler.options.libraries=");
	for (i = 0; i < length; i++)
	{
		gtk_clist_get_text (GTK_CLIST (co->widgets.lib_clist), i, 1, &text);
		if (fprintf (s, "\\\n\t%s", text) <1)
		{
			return FALSE;
		}
	}
	fprintf (s, "\n");

	length = g_list_length (GTK_CLIST (co->widgets.lib_clist)->row_list);
	fprintf (s, "compiler.options.libraries.selected=");
	for (i = 0; i < length; i++)
	{
		gboolean state;
		state = co_clist_row_data_get_state (GTK_CLIST (co->widgets.lib_clist), i);
		if (fprintf (s, "%d ", state) <1)
		{
			return FALSE;
		}
	}
	fprintf (s, "\n");

	length = g_list_length (GTK_CLIST (co->widgets.def_clist)->row_list);
	fprintf (s, "compiler.options.defines=");
	for (i = 0; i < length; i++)
	{
		gtk_clist_get_text (GTK_CLIST (co->widgets.def_clist), i, 0,
				    &text);
		if (fprintf (s, "\\\n\t%s", text) <1)
		{
			return FALSE;
		}
	}
	fprintf (s, "\n");

	fprintf (s, "compiler.options.warning.buttons=");
	for (i = 0; i < 16; i++)
		fprintf (s, "%d ", (int) co->warning_button_state[i]);
	fprintf (s, "\n");

	fprintf (s, "compiler.options.optimize.buttons=");
	for (i = 0; i < 4; i++)
		fprintf (s, "%d ", (int) co->optimize_button_state[i]);
	fprintf (s, "\n");

	fprintf (s, "compiler.options.other.buttons=");
	for (i = 0; i < 2; i++)
		fprintf (s, "%d ", (int) co->other_button_state[i]);
	fprintf (s, "\n");

	fprintf (s, "compiler.options.other.c.flags=%s\n",
		 co->other_c_flags);
	
	fprintf (s, "compiler.options.other.l.flags=%s\n",
		 co->other_l_flags);
	
	fprintf (s, "compiler.options.other.l.libs=%s\n",
		 co->other_l_libs);

	return TRUE;
}

gboolean compiler_options_load_yourself (CompilerOptions * co, PropsID props)
{
	if (co == NULL)
		return FALSE;
	co->win_pos_x =
		prop_get_int (props, "compiler.options.win.pos.x", 100);
	co->win_pos_y =
		prop_get_int (props, "compiler.options.win.pos.y", 100);
	return TRUE;
}
void
compiler_options_clear(CompilerOptions *co)
{
	gint i;

	g_return_if_fail (co != NULL);

	gtk_widget_set_sensitive (co->widgets.supp_clist, TRUE);

	/* Clear all supports */
	i = 0;
	while (strcmp (anjuta_supports[i][ANJUTA_SUPPORT_ID], ANJUTA_SUPPORTS_END_STRING) != 0)
	{
		co_clist_row_data_set_false (GTK_CLIST(co->widgets.supp_clist), i);
		i++;
	}

	gtk_clist_clear (GTK_CLIST (co->widgets.inc_clist));
	gtk_entry_set_text (GTK_ENTRY (co->widgets.inc_entry), "");
	gtk_clist_clear (GTK_CLIST (co->widgets.lib_paths_clist));
	gtk_entry_set_text (GTK_ENTRY (co->widgets.lib_paths_entry), "");
	gtk_clist_clear (GTK_CLIST (co->widgets.lib_clist));
	gtk_entry_set_text (GTK_ENTRY (co->widgets.lib_entry), "");
	gtk_clist_clear (GTK_CLIST (co->widgets.def_clist));
	gtk_entry_set_text (GTK_ENTRY (co->widgets.def_entry), "");
	for (i = 0; i < 16; i++)
		co->warning_button_state[i] = FALSE;
	for (i = 0; i < 4; i++)
		co->optimize_button_state[i] = FALSE;
	for (i = 0; i < 2; i++)
		co->other_button_state[i] = FALSE;

	co->supp_index = 0;
	co->inc_index = 0;
	co->lib_index = 0;
	co->lib_paths_index = 0;
	co->def_index = 0;

	co->warning_button_state[2] = TRUE;
	co->optimize_button_state[0] = TRUE;
	co->other_button_state[0] = TRUE;

	string_assign (&co->other_c_flags, "");
	string_assign (&co->other_l_flags, "");
	string_assign (&co->other_l_libs, "");
	compiler_options_sync (co);
}

void
compiler_options_load (CompilerOptions * co, PropsID props)
{
	gchar *str;
	gchar *dummy[2];
	gchar dumtext[] = "";
	GList *list, *node;
	gint i;

	g_return_if_fail (co != NULL);

	compiler_options_clear (co);

	/* Read all available supports for the project */
	list = glist_from_data (props, "compiler.options.supports");
	node = list;
	i=0;
	while (node)
	{
		if (node->data)
		{
			while (strcmp (anjuta_supports[i][ANJUTA_SUPPORT_ID],
					ANJUTA_SUPPORTS_END_STRING) != 0)
			{
				if (strcmp (anjuta_supports[i][ANJUTA_SUPPORT_ID], node->data)==0)
				{
					
					co_clist_row_data_set_true (GTK_CLIST (co->widgets.supp_clist), i);
					break;
				}
				i++;
			}
		}
		node = g_list_next (node);
	}
	glist_strings_free (list);
	
	/* For now, disable supports for projects. */
	if (app->project_dbase->project_is_open == TRUE)
	{
		gtk_widget_set_sensitive (co->widgets.supp_clist, FALSE);
	}

	list = glist_from_data (props, "compiler.options.include.paths");
	node = list;
	while (node)
	{
		dummy[0] = node->data;
		gtk_clist_append (GTK_CLIST (co->widgets.inc_clist), dummy);
		node = g_list_next (node);
	}
	glist_strings_free (list);

	list = glist_from_data (props, "compiler.options.library.paths");
	node = list;
	while (node)
	{
		dummy[0] = node->data;
		gtk_clist_append (GTK_CLIST (co->widgets.lib_paths_clist),
				  dummy);
		node = g_list_next (node);
	}
	glist_strings_free (list);

	list = glist_from_data (props, "compiler.options.libraries");
	node = list;
	while (node)
	{
		dummy[0] = dumtext;
		dummy[1] = node->data;
		if (dummy[1])
		{
			gtk_clist_append (GTK_CLIST (co->widgets.lib_clist), dummy);
			co_clist_row_data_set_true (GTK_CLIST(co->widgets.lib_clist), 
				  GTK_CLIST(co->widgets.lib_clist)->rows-1);
		}
		node = g_list_next (node);
	}
	glist_strings_free (list);

	list = glist_from_data (props, "compiler.options.libraries.selected");
	node = list;
	i=0;
	while (node)
	{
		if (node->data)
		{
			gint state;
			sscanf (node->data, "%d", &state);
			if (!state)
				co_clist_row_data_set_false (GTK_CLIST(co->widgets.lib_clist), i);
		}
		i++;
		node = g_list_next (node);
	}
	glist_strings_free (list);

	list = glist_from_data (props, "compiler.options.defines");
	node = list;
	while (node)
	{
		dummy[0] = node->data;
		if (dummy[0]) gtk_clist_append (GTK_CLIST (co->widgets.def_clist), dummy);
		node = g_list_next (node);
	}
	glist_strings_free (list);

	list = glist_from_data (props, "compiler.options.warning.buttons");
	node = list;
	i = 0;
	while (node)
	{
		gint state;
		if (node->data)
		{
			sscanf (node->data, "%d", &state);
			co->warning_button_state[i] = state;
		}
		i++;
		node = g_list_next (node);
	}
	glist_strings_free (list);

	list = glist_from_data (props, "compiler.options.optimize.buttons");
	node = list;
	i = 0;
	while (node)
	{
		gint state;
		if (node->data)
		{
			sscanf (node->data, "%d", &state);
			co->optimize_button_state[i] = state;
		}
		i++;
		node = g_list_next (node);
	}
	glist_strings_free (list);

	list = glist_from_data (props, "compiler.options.other.buttons");
	node = list;
	i = 0;
	while (node)
	{
		gint state;
		if (node->data)
		{
			sscanf (node->data, "%d", &state);
			co->other_button_state[i] = state;
		}
		i++;
		node = g_list_next (node);
	}
	glist_strings_free (list);

	str = prop_get (props, "compiler.options.other.c.flags");
	if (str)
	{
		string_assign (&co->other_c_flags, str);
		g_free (str);
	}
	
	str = prop_get (props, "compiler.options.other.l.flags");
	if (str)
	{
		string_assign (&co->other_l_flags, str);
		g_free (str);
	}

	str = prop_get (props, "compiler.options.other.l.libs");
	if (str)
	{
		string_assign (&co->other_l_libs, str);
		g_free (str);
	}
	compiler_options_sync (co);
}

void
compiler_options_sync (CompilerOptions * co)
{
	gint i;

	if (!co)
		return;
	for (i = 0; i < 16; i++)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (co->widgets.warning_button[i]),
					      co->warning_button_state[i]);
	}
	for (i = 0; i < 4; i++)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (co->widgets.
					       optimize_button[i]),
					      co->optimize_button_state[i]);
	}
	for (i = 0; i < 2; i++)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (co->widgets.other_button[i]),
					      co->other_button_state[i]);
	}

	if (co->other_c_flags)
		gtk_entry_set_text (GTK_ENTRY
				    (co->widgets.other_c_flags_entry),
				    co->other_c_flags);
	
	if (co->other_l_flags)
		gtk_entry_set_text (GTK_ENTRY
				    (co->widgets.other_l_flags_entry),
				    co->other_l_flags);
	
	if (co->other_l_libs)
		gtk_entry_set_text (GTK_ENTRY
				    (co->widgets.other_l_libs_entry),
				    co->other_l_libs);
	
	compiler_options_set_in_properties (co, co->props);
	compiler_options_update_controls (co);
	
}

void
compiler_options_show (CompilerOptions * co)
{
	compiler_options_sync (co);

	if (co->is_showing)
	{
		gdk_window_raise (co->widgets.window->window);
		return;
	}
	gtk_widget_set_uposition (co->widgets.window, co->win_pos_x,
				  co->win_pos_y);
	gtk_widget_show (co->widgets.window);
	co->is_showing = TRUE;
}

void
compiler_options_hide (CompilerOptions * co)
{
	if (!co)
		return;
	if (co->is_showing == FALSE)
		return;
	gdk_window_get_root_origin (co->widgets.window->window,
				    &co->win_pos_x, &co->win_pos_y);
	co->is_showing = FALSE;
}

void
compiler_options_update_controls (CompilerOptions * co)
{
	gint length;

	length = g_list_length (GTK_CLIST (co->widgets.inc_clist)->row_list);
	if (length < 2)
		gtk_widget_set_sensitive (co->widgets.inc_clear_b, FALSE);
	else
		gtk_widget_set_sensitive (co->widgets.inc_clear_b, TRUE);
	if (length < 1)
	{
		gtk_widget_set_sensitive (co->widgets.inc_remove_b, FALSE);
		gtk_widget_set_sensitive (co->widgets.inc_update_b, FALSE);
	}
	else
	{
		gtk_widget_set_sensitive (co->widgets.inc_remove_b, TRUE);
		gtk_widget_set_sensitive (co->widgets.inc_update_b, TRUE);
	}

	length =
		g_list_length (GTK_CLIST (co->widgets.lib_paths_clist)->
			       row_list);
	if (length < 2)
		gtk_widget_set_sensitive (co->widgets.lib_paths_clear_b,
					  FALSE);
	else
		gtk_widget_set_sensitive (co->widgets.lib_paths_clear_b,
					  TRUE);
	if (length < 1)
	{
		gtk_widget_set_sensitive (co->widgets.lib_paths_remove_b,
					  FALSE);
		gtk_widget_set_sensitive (co->widgets.lib_paths_update_b,
					  FALSE);
	}
	else
	{
		gtk_widget_set_sensitive (co->widgets.lib_paths_remove_b,
					  TRUE);
		gtk_widget_set_sensitive (co->widgets.lib_paths_update_b,
					  TRUE);
	}

	length = g_list_length (GTK_CLIST (co->widgets.lib_clist)->row_list);
	if (length < 2)
		gtk_widget_set_sensitive (co->widgets.lib_clear_b, FALSE);
	else
		gtk_widget_set_sensitive (co->widgets.lib_clear_b, TRUE);
	if (length < 1)
	{
		gtk_widget_set_sensitive (co->widgets.lib_remove_b, FALSE);
		gtk_widget_set_sensitive (co->widgets.lib_update_b, FALSE);
	}
	else
	{
		gtk_widget_set_sensitive (co->widgets.lib_remove_b, TRUE);
		gtk_widget_set_sensitive (co->widgets.lib_update_b, TRUE);
	}

	length = g_list_length (GTK_CLIST (co->widgets.def_clist)->row_list);
	if (length < 2)
		gtk_widget_set_sensitive (co->widgets.def_clear_b, FALSE);
	else
		gtk_widget_set_sensitive (co->widgets.def_clear_b, TRUE);
	if (length < 1)
	{
		gtk_widget_set_sensitive (co->widgets.def_remove_b, FALSE);
		gtk_widget_set_sensitive (co->widgets.def_update_b, FALSE);
	}
	else
	{
		gtk_widget_set_sensitive (co->widgets.def_remove_b, TRUE);
		gtk_widget_set_sensitive (co->widgets.def_update_b, TRUE);
	}
}

static gchar *
get_include_paths (CompilerOptions * co, gboolean with_support)
{
	gchar *str, *tmp;
	gchar *text;
	gint i;

	str = g_strdup ("");
	for (i = 0;
	     i < g_list_length (GTK_CLIST (co->widgets.inc_clist)->row_list);
	     i++)
	{
		gtk_clist_get_text (GTK_CLIST (co->widgets.inc_clist), i, 0,
				    &text);
		tmp = g_strconcat (str, " -I", text, NULL);
		g_free (str);
		str = tmp;
	}
	if (with_support)
	{
		for (i = 0; i < g_list_length (GTK_CLIST (co->widgets.supp_clist)->row_list); i++)
		{
			if (co_clist_row_data_get_state (GTK_CLIST (co->widgets.supp_clist), i) == FALSE)
				continue;
			text = anjuta_supports [i][ANJUTA_SUPPORT_FILE_CFLAGS];
			tmp = g_strconcat (str, " ", text, NULL);
			g_free (str);
			str = tmp;
		}
	}
	return str;
}

static gchar *
get_library_paths (CompilerOptions * co)
{
	gchar *str, *tmp;
	gchar *text;
	gint i;

	str = g_strdup ("");
	for (i = 0;
	     i <
	     g_list_length (GTK_CLIST (co->widgets.lib_paths_clist)->
			    row_list); i++)
	{
		gtk_clist_get_text (GTK_CLIST (co->widgets.lib_paths_clist),
				    i, 0, &text);
		tmp = g_strconcat (str, " -L", text, NULL);
		g_free (str);
		str = tmp;
	}
	return str;
}

static gchar *
get_libraries (CompilerOptions * co, gboolean with_support)
{
	gchar *str, *tmp;
	gchar *text;
	gint i;

	str = g_strdup ("");
	for (i = 0; i < g_list_length (GTK_CLIST (co->widgets.lib_clist)->row_list); i++)
	{
		if (co_clist_row_data_get_state (GTK_CLIST (co->widgets.lib_clist), i) == FALSE)
			continue;
		gtk_clist_get_text (GTK_CLIST (co->widgets.lib_clist), i, 1,
				    &text);
		
		if (*text == '*') /* If it starts with '*' then consider it as an object file and not a lib file */
		{
			/* Remove the '*' and put the rest */
			tmp = g_strconcat (str, " ", &text[1], NULL);
		}
		else /* Lib file */
		{
			tmp = g_strconcat (str, " -l", text, NULL);
		}
		g_free (str);
		str = tmp;
	}
	if (with_support)
	{
		for (i = 0; i < g_list_length (GTK_CLIST (co->widgets.supp_clist)->row_list); i++)
		{
			if (co_clist_row_data_get_state (GTK_CLIST (co->widgets.supp_clist), i) == FALSE)
				continue;
			text = anjuta_supports [i][ANJUTA_SUPPORT_FILE_LIBS];
			tmp = g_strconcat (str, " ", text, NULL);
			g_free (str);
			str = tmp;
		}
	}
	return str;
}

static gchar *
get_defines (CompilerOptions * co)
{
	gchar *str, *tmp;
	gchar *text;
	gint i;

	str = g_strdup ("");
	for (i = 0;
	     i < g_list_length (GTK_CLIST (co->widgets.def_clist)->row_list);
	     i++)
	{
		gtk_clist_get_text (GTK_CLIST (co->widgets.def_clist), i, 0,
				    &text);
		tmp = g_strconcat (str, " -D", text, NULL);
		g_free (str);
		str = tmp;
	}
	return str;
}

void
compiler_options_set_in_properties (CompilerOptions * co, PropsID props)
{
	gchar *str, *tmp, *buff;
	gint i;

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

	str = g_strdup ("");
	for (i = 0; i < 15; i++)
		if (co->warning_button_state[i])
		{
			tmp = str;
			str = g_strconcat (tmp, warning_button_option[i],
					     NULL);
			g_free (tmp);
			if (i==1)
				break;
		}
	prop_set_with_key (props, "anjuta.compiler.warning.flags", str);
	g_free (str);

	str = g_strdup ("");
	for (i = 0; i < 4; i++)
		if (co->optimize_button_state[i])
		{
			tmp = str;
			str =
				g_strconcat (tmp, optimize_button_option[i],
					     NULL);
			g_free (tmp);
		}
	prop_set_with_key (props, "anjuta.compiler.optimization.flags", str);
	g_free (str);

	str = g_strdup ("");
	for (i = 0; i < 2; i++)
		if (co->other_button_state[i])
		{
			tmp = str;
			str = g_strconcat (tmp, other_button_option[i], NULL);
			g_free (tmp);
		}
	prop_set_with_key (props, "anjuta.compiler.other.flags", str);
	g_free (str);
	
	prop_set_with_key (props, "anjuta.compiler.additional.flags", co->other_c_flags);
	prop_set_with_key (props, "anjuta.linker.additional.flags", co->other_l_flags);
	prop_set_with_key (props, "anjuta.linker.additional.libs", co->other_l_libs);

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

void
compiler_options_set_prjincl_in_file (CompilerOptions* co, FILE* fp)
{
	gchar *buff;
	buff = get_include_paths (co, FALSE);
	if (buff)
	{
		if (strlen (buff))
		{
			fprintf (fp, "\\\n\t%s", buff);
		}
		g_free (buff);
	}	
}

void
compiler_options_set_prjcflags_in_file (CompilerOptions * co, FILE* fp)
{
	gchar *buff, *tmp;
	gint i;

/*
	for (i = 0; i < g_list_length (GTK_CLIST (co->widgets.supp_clist)->row_list); i++)
	{
		if (co_clist_row_data_get_state (GTK_CLIST (co->widgets.supp_clist), i) == FALSE)
			continue;
		buff = anjuta_supports [i][ANJUTA_SUPPORT_PRJ_CFLAGS];
		fprintf (fp, "\\\n\t%s", buff);
	}

*/
	buff = get_defines (co);
	if (buff)
	{
		if (strlen (buff))
		{
			fprintf (fp, "\\\n\t%s", buff);
		}
		g_free (buff);
	}

	if (co->other_c_flags)
	{
		if (strlen (co->other_c_flags))
		{
			fprintf (fp, "\\\n\t%s", co->other_c_flags);
		}
	}

	buff = g_strdup ("");
	for (i = 0; i < 15; i++)
		if (co->warning_button_state[i])
		{
			tmp = buff;
			buff =
				g_strconcat (tmp, warning_button_option[i],
					     NULL);
			g_free (tmp);
		}
	if (buff)
	{
		if (strlen (buff))
		{
			fprintf (fp, "\\\n\t%s", buff);
		}
		g_free (buff);
	}

	buff = g_strdup ("");
	for (i = 0; i < 4; i++)
		if (co->optimize_button_state[i])
		{
			tmp = buff;
			buff =
				g_strconcat (tmp, optimize_button_option[i],
					     NULL);
			g_free (tmp);
		}
	if (buff)
	{
		if (strlen (buff))
		{
			fprintf (fp, "\\\n\t%s", buff);
		}
		g_free (buff);
	}

	buff = g_strdup ("");
	for (i = 0; i < 2; i++)
		if (co->other_button_state[i])
		{
			tmp = buff;
			buff = g_strconcat (tmp, other_button_option[i], NULL);
			g_free (tmp);
		}
	if (buff)
	{
		if (strlen (buff))
		{
			fprintf (fp, "\\\n\t%s", buff);
		}
		g_free (buff);
	}
}

void
compiler_options_set_prjlibs_in_file (CompilerOptions * co, FILE* fp)
{
	gchar *buff;
	
	buff = get_libraries (co, FALSE);
	
	if (buff)
	{
		if (strlen (buff))
		{
			fprintf (fp, "\\\n\t%s", buff);
		}
		g_free (buff);
	}
	
	if (co->other_l_libs)
	{
		if (strlen (co->other_l_libs))
		{
			fprintf (fp, "\\\n\t%s", co->other_l_libs);
		}
	}
}

void
compiler_options_set_prjlflags_in_file (CompilerOptions * co, FILE* fp)
{
	gchar *buff;
/*	gint i; */

	if (co->other_l_flags)
	{
		if (strlen (co->other_l_flags))
		{
			fprintf (fp, "\\\n\t%s", co->other_l_flags);
		}
	}
	
	buff = get_library_paths (co);
	if (buff)
	{
		if (strlen (buff))
		{
			fprintf (fp, "\\\n\t%s", buff);
		}
		g_free (buff);
	}

/*
	for (i = 0; i < g_list_length (GTK_CLIST (co->widgets.supp_clist)->row_list); i++)
	{
		if (co_clist_row_data_get_state (GTK_CLIST (co->widgets.supp_clist), i) == FALSE)
			continue;
		buff = anjuta_supports [i][ANJUTA_SUPPORT_PRJ_LIBS];
		fprintf (fp, "\\\n\t%s", buff);
	}
*/
}

void
compiler_options_set_prjmacros_in_file (CompilerOptions * co, FILE* fp)
{
	gint i;
	gchar* buff;
	
	for (i = 0; i < g_list_length (GTK_CLIST (co->widgets.supp_clist)->row_list); i++)
	{
		if (co_clist_row_data_get_state (GTK_CLIST (co->widgets.supp_clist), i) == FALSE)
			continue;
		buff = anjuta_supports [i][ANJUTA_SUPPORT_MACROS];
		fprintf (fp, "%s\n", buff);
	}
}
