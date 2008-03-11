/***************************************************************************
 *            build-options.c
 *
 *  Sat Mar  1 20:47:23 2008
 *  Copyright  2008  Johannes Schmid
 *  <jhs@gnome.org>
 ****************************************************************************/

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */
 

#include "build-options.h"
#include <glib/gi18n.h>
#include <glade/glade-xml.h>
#include <libanjuta/anjuta-debug.h>
#include <string.h>

#define GLADE_FILE PACKAGE_DATA_DIR"/glade/anjuta-build-basic-autotools-plugin.glade"

typedef struct
{
	gchar* label;
	gchar* options;
} Options;

const Options gcc[] = {
	{N_("Default"), ""},
	{N_("Debug"), "-g -O0"},
	{N_("Profiling"), "-g -pg"},
	{N_("Optimized"), "-O2"}
};

enum
{
	COLUMN_LABEL,
	COLUMN_OPTIONS,
	N_COLUMNS
};

static void 
fill_options_combo (GtkComboBoxEntry* combo)
{
	GtkCellRenderer* renderer_label = gtk_cell_renderer_text_new ();
	GtkCellRenderer* renderer_options = gtk_cell_renderer_text_new ();
	
	GtkListStore* store = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);
	gint i;
		
	for (i = 0; i < G_N_ELEMENTS(gcc); i++)
	{
		GtkTreeIter iter;
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 
												COLUMN_LABEL, gcc[i].label,
												COLUMN_OPTIONS, gcc[i].options, -1);
	}
	gtk_combo_box_set_model (GTK_COMBO_BOX(combo), GTK_TREE_MODEL(store));
	gtk_combo_box_entry_set_text_column (combo, 1);

	gtk_cell_layout_clear (GTK_CELL_LAYOUT(combo));
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT(combo), renderer_label, TRUE);
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT(combo), renderer_options, FALSE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT(combo), renderer_label,
																 "text", COLUMN_LABEL);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT(combo), renderer_options,
																 "text", COLUMN_OPTIONS);
	g_object_set (renderer_label,
								"style", PANGO_STYLE_ITALIC, NULL);
}

gboolean
build_dialog_configure (GtkWindow* parent, const gchar* dialog_title,
												GHashTable** build_options,
												const gchar* default_args, gchar** args)
{
	GladeXML* gxml = glade_xml_new (GLADE_FILE, "configure_dialog", NULL);
	GtkDialog* dialog = GTK_DIALOG (glade_xml_get_widget (gxml, "configure_dialog"));
	GtkComboBoxEntry* combo
		= GTK_COMBO_BOX_ENTRY (glade_xml_get_widget(gxml, "build_options_combo"));
	GtkEntry* entry_args = GTK_ENTRY (glade_xml_get_widget (gxml, "configure_args_entry"));
	
	gtk_window_set_title (GTK_WINDOW(dialog), dialog_title);
	
	if (default_args)
		gtk_entry_set_text (entry_args, default_args);
	fill_options_combo(combo);
	
	int response = gtk_dialog_run (dialog);
	if (response != GTK_RESPONSE_OK)
	{
		*build_options = NULL;
		*args = NULL;
		gtk_widget_destroy (GTK_WIDGET(dialog));
		return FALSE;
	}
	else
	{
		GtkEntry* build_options_entry = GTK_ENTRY(gtk_bin_get_child (GTK_BIN(combo)));
		const gchar* options = gtk_entry_get_text (build_options_entry);
		*args = g_strdup (gtk_entry_get_text (entry_args));
		*build_options = g_hash_table_new_full (g_str_hash, g_str_equal,
																						NULL, g_free);
		if (strlen(options))
		{
			/* set options for all languages */
			g_hash_table_insert (*build_options,
								 "CFLAGS", g_strdup(options));
			g_hash_table_insert (*build_options,
								 "CXXFLAGS", g_strdup(options));
			g_hash_table_insert (*build_options,
								 "JFLAGS", g_strdup(options));
			g_hash_table_insert (*build_options,
								 "FFLAGS", g_strdup(options));
		}
		gtk_widget_destroy (GTK_WIDGET(dialog));
		return TRUE;
	}
}
