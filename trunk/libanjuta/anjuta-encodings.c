/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * anjuta-encodings.c
 * Copyright (C) 2002 Paolo Maggi 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA 02110-1301, USA.
 */
 
/*
 * Modified by the gedit Team, 2002. See the gedit AUTHORS file for a 
 * list of people on the gedit Team.
 * See the gedit ChangeLog files for a list of changes. 
 */
 /* Stolen from gedit - Naba */

/**
 * SECTION:anjuta-encodings
 * @title: AnjutaEncoding
 * @short_description: Text encoding and decoding
 * @see_also: 
 * @stability: Unstable
 * @include: libanjuta/anjuta-encodings.h
 * 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtktreeview.h>
#include <gtk/gtkliststore.h>
#include <string.h>

#include <libanjuta/anjuta-encodings.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-debug.h>

struct _AnjutaEncoding
{
  gint   idx;
  gchar *charset;
  gchar *name;
};

/* 
 * The original versions of the following tables are taken from profterm 
 *
 * Copyright (C) 2002 Red Hat, Inc.
 */

typedef enum
{

  ANJUTA_ENCODING_ISO_8859_1,
  ANJUTA_ENCODING_ISO_8859_2,
  ANJUTA_ENCODING_ISO_8859_3,
  ANJUTA_ENCODING_ISO_8859_4,
  ANJUTA_ENCODING_ISO_8859_5,
  ANJUTA_ENCODING_ISO_8859_6,
  ANJUTA_ENCODING_ISO_8859_7,
  ANJUTA_ENCODING_ISO_8859_8,
  ANJUTA_ENCODING_ISO_8859_8_I,
  ANJUTA_ENCODING_ISO_8859_9,
  ANJUTA_ENCODING_ISO_8859_10,
  ANJUTA_ENCODING_ISO_8859_13,
  ANJUTA_ENCODING_ISO_8859_14,
  ANJUTA_ENCODING_ISO_8859_15,
  ANJUTA_ENCODING_ISO_8859_16,

  ANJUTA_ENCODING_UTF_7,
  ANJUTA_ENCODING_UTF_16,
  ANJUTA_ENCODING_UCS_2,
  ANJUTA_ENCODING_UCS_4,

  ANJUTA_ENCODING_ARMSCII_8,
  ANJUTA_ENCODING_BIG5,
  ANJUTA_ENCODING_BIG5_HKSCS,
  ANJUTA_ENCODING_CP_866,

  ANJUTA_ENCODING_EUC_JP,
  ANJUTA_ENCODING_EUC_KR,
  ANJUTA_ENCODING_EUC_TW,

  ANJUTA_ENCODING_GB18030,
  ANJUTA_ENCODING_GB2312,
  ANJUTA_ENCODING_GBK,
  ANJUTA_ENCODING_GEOSTD8,
  ANJUTA_ENCODING_HZ,

  ANJUTA_ENCODING_IBM_850,
  ANJUTA_ENCODING_IBM_852,
  ANJUTA_ENCODING_IBM_855,
  ANJUTA_ENCODING_IBM_857,
  ANJUTA_ENCODING_IBM_862,
  ANJUTA_ENCODING_IBM_864,

  ANJUTA_ENCODING_ISO_2022_JP,
  ANJUTA_ENCODING_ISO_2022_KR,
  ANJUTA_ENCODING_ISO_IR_111,
  ANJUTA_ENCODING_JOHAB,
  ANJUTA_ENCODING_KOI8_R,
  ANJUTA_ENCODING_KOI8_U,
  
  ANJUTA_ENCODING_SHIFT_JIS,
  ANJUTA_ENCODING_TCVN,
  ANJUTA_ENCODING_TIS_620,
  ANJUTA_ENCODING_UHC,
  ANJUTA_ENCODING_VISCII,

  ANJUTA_ENCODING_WINDOWS_1250,
  ANJUTA_ENCODING_WINDOWS_1251,
  ANJUTA_ENCODING_WINDOWS_1252,
  ANJUTA_ENCODING_WINDOWS_1253,
  ANJUTA_ENCODING_WINDOWS_1254,
  ANJUTA_ENCODING_WINDOWS_1255,
  ANJUTA_ENCODING_WINDOWS_1256,
  ANJUTA_ENCODING_WINDOWS_1257,
  ANJUTA_ENCODING_WINDOWS_1258,

  ANJUTA_ENCODING_LAST
  
} AnjutaEncodingIndex;

static AnjutaEncoding encodings [] = {

  { ANJUTA_ENCODING_ISO_8859_1,
    "ISO-8859-1", N_("Western") },
  { ANJUTA_ENCODING_ISO_8859_2,
   "ISO-8859-2", N_("Central European") },
  { ANJUTA_ENCODING_ISO_8859_3,
    "ISO-8859-3", N_("South European") },
  { ANJUTA_ENCODING_ISO_8859_4,
    "ISO-8859-4", N_("Baltic") },
  { ANJUTA_ENCODING_ISO_8859_5,
    "ISO-8859-5", N_("Cyrillic") },
  { ANJUTA_ENCODING_ISO_8859_6,
    "ISO-8859-6", N_("Arabic") },
  { ANJUTA_ENCODING_ISO_8859_7,
    "ISO-8859-7", N_("Greek") },
  { ANJUTA_ENCODING_ISO_8859_8,
    "ISO-8859-8", N_("Hebrew Visual") },
  { ANJUTA_ENCODING_ISO_8859_8_I,
    "ISO-8859-8-I", N_("Hebrew") },
  { ANJUTA_ENCODING_ISO_8859_9,
    "ISO-8859-9", N_("Turkish") },
  { ANJUTA_ENCODING_ISO_8859_10,
    "ISO-8859-10", N_("Nordic") },
  { ANJUTA_ENCODING_ISO_8859_13,
    "ISO-8859-13", N_("Baltic") },
  { ANJUTA_ENCODING_ISO_8859_14,
    "ISO-8859-14", N_("Celtic") },
  { ANJUTA_ENCODING_ISO_8859_15,
    "ISO-8859-15", N_("Western") },
  { ANJUTA_ENCODING_ISO_8859_16,
    "ISO-8859-16", N_("Romanian") },

  { ANJUTA_ENCODING_UTF_7,
    "UTF-7", N_("Unicode") },
  { ANJUTA_ENCODING_UTF_16,
    "UTF-16", N_("Unicode") },
  { ANJUTA_ENCODING_UCS_2,
    "UCS-2", N_("Unicode") },
  { ANJUTA_ENCODING_UCS_4,
    "UCS-4", N_("Unicode") },

  { ANJUTA_ENCODING_ARMSCII_8,
    "ARMSCII-8", N_("Armenian") },
  { ANJUTA_ENCODING_BIG5,
    "BIG5", N_("Chinese Traditional") },
  { ANJUTA_ENCODING_BIG5_HKSCS,
    "BIG5-HKSCS", N_("Chinese Traditional") },
  { ANJUTA_ENCODING_CP_866,
    "CP866", N_("Cyrillic/Russian") },

  { ANJUTA_ENCODING_EUC_JP,
    "EUC-JP", N_("Japanese") },
  { ANJUTA_ENCODING_EUC_KR,
    "EUC-KR", N_("Korean") },
  { ANJUTA_ENCODING_EUC_TW,
    "EUC-TW", N_("Chinese Traditional") },

  { ANJUTA_ENCODING_GB18030,
    "GB18030", N_("Chinese Simplified") },
  { ANJUTA_ENCODING_GB2312,
    "GB2312", N_("Chinese Simplified") },
  { ANJUTA_ENCODING_GBK,
    "GBK", N_("Chinese Simplified") },
  { ANJUTA_ENCODING_GEOSTD8,
    "GEORGIAN-ACADEMY", N_("Georgian") }, /* FIXME GEOSTD8 ? */
  { ANJUTA_ENCODING_HZ,
    "HZ", N_("Chinese Simplified") },

  { ANJUTA_ENCODING_IBM_850,
    "IBM850", N_("Western") },
  { ANJUTA_ENCODING_IBM_852,
    "IBM852", N_("Central European") },
  { ANJUTA_ENCODING_IBM_855,
    "IBM855", N_("Cyrillic") },
  { ANJUTA_ENCODING_IBM_857,
    "IBM857", N_("Turkish") },
  { ANJUTA_ENCODING_IBM_862,
    "IBM862", N_("Hebrew") },
  { ANJUTA_ENCODING_IBM_864,
    "IBM864", N_("Arabic") },

  { ANJUTA_ENCODING_ISO_2022_JP,
    "ISO-2022-JP", N_("Japanese") },
  { ANJUTA_ENCODING_ISO_2022_KR,
    "ISO-2022-KR", N_("Korean") },
  { ANJUTA_ENCODING_ISO_IR_111,
    "ISO-IR-111", N_("Cyrillic") },
  { ANJUTA_ENCODING_JOHAB,
    "JOHAB", N_("Korean") },
  { ANJUTA_ENCODING_KOI8_R,
    "KOI8R", N_("Cyrillic") },
  { ANJUTA_ENCODING_KOI8_U,
    "KOI8U", N_("Cyrillic/Ukrainian") },
  
  { ANJUTA_ENCODING_SHIFT_JIS,
    "SHIFT_JIS", N_("Japanese") },
  { ANJUTA_ENCODING_TCVN,
    "TCVN", N_("Vietnamese") },
  { ANJUTA_ENCODING_TIS_620,
    "TIS-620", N_("Thai") },
  { ANJUTA_ENCODING_UHC,
    "UHC", N_("Korean") },
  { ANJUTA_ENCODING_VISCII,
    "VISCII", N_("Vietnamese") },

  { ANJUTA_ENCODING_WINDOWS_1250,
    "WINDOWS-1250", N_("Central European") },
  { ANJUTA_ENCODING_WINDOWS_1251,
    "WINDOWS-1251", N_("Cyrillic") },
  { ANJUTA_ENCODING_WINDOWS_1252,
    "WINDOWS-1252", N_("Western") },
  { ANJUTA_ENCODING_WINDOWS_1253,
    "WINDOWS-1253", N_("Greek") },
  { ANJUTA_ENCODING_WINDOWS_1254,
    "WINDOWS-1254", N_("Turkish") },
  { ANJUTA_ENCODING_WINDOWS_1255,
    "WINDOWS-1255", N_("Hebrew") },
  { ANJUTA_ENCODING_WINDOWS_1256,
    "WINDOWS-1256", N_("Arabic") },
  { ANJUTA_ENCODING_WINDOWS_1257,
    "WINDOWS-1257", N_("Baltic") },
  { ANJUTA_ENCODING_WINDOWS_1258,
    "WINDOWS-1258", N_("Vietnamese") }
};

static void save_property (void);

static void
anjuta_encoding_lazy_init (void)
{

	static gboolean initialized = FALSE;
	gint i;
	
	if (initialized)
		return;

	g_return_if_fail (G_N_ELEMENTS (encodings) == ANJUTA_ENCODING_LAST);
  
	i = 0;
	while (i < ANJUTA_ENCODING_LAST)
	{
		g_return_if_fail (encodings[i].idx == i);

		/* Translate the names */
		encodings[i].name = _(encodings[i].name);
      
		++i;
    	}

	initialized = TRUE;
}

/**
* anjuta_encoding_get_from_charset:
* @charset: Character set for the encoding.
* 
* Gets #AnjutaEncoding object corresponding to the given character set
* 
* Returns: #AnjutaEncoding object for the given charset
*/
const AnjutaEncoding *
anjuta_encoding_get_from_charset (const gchar *charset)
{
	gint i;

	anjuta_encoding_lazy_init ();

	i = 0; 
	while (i < ANJUTA_ENCODING_LAST)
	{
		if (strcmp (charset, encodings[i].charset) == 0)
			return &encodings[i];
      
		++i;
	}
 
	return NULL;
}

/**
* anjuta_encoding_get_from_index:
* @idx: Index of the encoding object
* 
* Retrieves #AnjutaEncoding object at the given index.
* 
* Returns: #AnjutaEncoding object at the index @idx.
*/
const AnjutaEncoding *
anjuta_encoding_get_from_index (gint idx)
{
	g_return_val_if_fail (idx >= 0, NULL);

	if (idx >= ANJUTA_ENCODING_LAST)
		return NULL;

	anjuta_encoding_lazy_init ();

	return &encodings [idx];
}

/**
* anjuta_encoding_to_string:
* @enc: an #AnjutaEncoding object
* 
* Returns the string form of the given encoding.
* 
* Returns: string name of the encoding.
*/
gchar *
anjuta_encoding_to_string (const AnjutaEncoding* enc)
{
	g_return_val_if_fail (enc != NULL, NULL);
	g_return_val_if_fail (enc->name != NULL, NULL);
	g_return_val_if_fail (enc->charset != NULL, NULL);

	anjuta_encoding_lazy_init ();

    	return g_strdup_printf ("%s (%s)", enc->name, enc->charset);
}

/**
* anjuta_encoding_get_charset:
* @enc: an #AnjutaEncoding object
* 
* Gets the character set for the given encoding.
*
* Returns: Character set
*/
const gchar *
anjuta_encoding_get_charset (const AnjutaEncoding* enc)
{
	g_return_val_if_fail (enc != NULL, NULL);
	g_return_val_if_fail (enc->charset != NULL, NULL);

	anjuta_encoding_lazy_init ();

	return enc->charset;
}

/**
* anjuta_encoding_get_encodings:
* @encodings_strings: List of encoding names.
* 
* Returns list of encoding objects for the given names. 
* 
* Returns: list of #AnjutaEncoding objects
*/
GList *
anjuta_encoding_get_encodings (GList *encoding_strings)
{
	GList *res = NULL;

	if (encoding_strings != NULL)
	{	
		GList *tmp;
		const AnjutaEncoding *enc;

		tmp = encoding_strings;
		
		while (tmp)
		{
		      const char *charset = tmp->data;

		      if (strcmp (charset, "current") == 0)
			      g_get_charset (&charset);
      
		      g_return_val_if_fail (charset != NULL, NULL);
		      enc = anjuta_encoding_get_from_charset (charset);
		      
		      if (enc != NULL)
				res = g_list_append (res, (gpointer)enc);

		      tmp = g_list_next (tmp);
		}
	}
	return res;
}

typedef struct
{
	AnjutaPreferences *pref;
	GtkWidget *add_button;
	GtkWidget *remove_button;
	GtkWidget *up_button;
	GtkWidget *down_button;
	GtkWidget *supported_treeview;
	GtkWidget *stock_treeview;
} AnjutaEncodingsDialog;

static AnjutaEncodingsDialog *anjuta_encodings_dialog = NULL;

enum
{
	COLUMN_ENCODING_NAME = 0,
	COLUMN_ENCODING_INDEX,
	ENCODING_NUM_COLS
};

enum
{
	COLUMN_SUPPORTED_ENCODING_NAME = 0,
	COLUMN_SUPPORTED_ENCODING,
	SUPPORTED_ENCODING_NUM_COLS
};

static GtkTreeModel*
create_encodings_treeview_model (void)
{
	GtkListStore *store;
	GtkTreeIter iter;
	gint i;
	const AnjutaEncoding* enc;

	/* create list store */
	store = gtk_list_store_new (ENCODING_NUM_COLS, G_TYPE_STRING, G_TYPE_INT);

	i = 0;
	while ((enc = anjuta_encoding_get_from_index (i)) != NULL)
	{
		gchar *name;
		enc = anjuta_encoding_get_from_index (i);
		name = anjuta_encoding_to_string (enc);
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, COLUMN_ENCODING_NAME, name,
				    		COLUMN_ENCODING_INDEX, i, -1);
		g_free (name);
		++i;
	}
	return GTK_TREE_MODEL (store);
}

static void 
on_add_encodings (GtkButton *button)
{
	GValue value = {0, };
	const AnjutaEncoding* enc;
	GSList *encs = NULL;
	
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	

	selection =
		gtk_tree_view_get_selection (GTK_TREE_VIEW
									 (anjuta_encodings_dialog->stock_treeview));
	g_return_if_fail (selection != NULL);
	
	model =	gtk_tree_view_get_model (GTK_TREE_VIEW 
							 (anjuta_encodings_dialog->stock_treeview));
	if (!gtk_tree_model_get_iter_first (model, &iter))
		return;

	if (gtk_tree_selection_iter_is_selected (selection, &iter))
	{
		gtk_tree_model_get_value (model, &iter,
								  COLUMN_ENCODING_INDEX, &value);
		enc = anjuta_encoding_get_from_index (g_value_get_int (&value));
		g_return_if_fail (enc != NULL);
		encs = g_slist_prepend (encs, (gpointer)enc);
		
		g_value_unset (&value);
	}

	while (gtk_tree_model_iter_next (model, &iter))
	{
		if (gtk_tree_selection_iter_is_selected (selection, &iter))
		{
			gtk_tree_model_get_value (model, &iter,
				    COLUMN_ENCODING_INDEX, &value);

			enc = anjuta_encoding_get_from_index (g_value_get_int (&value));
			g_return_if_fail (enc != NULL);
	
			encs = g_slist_prepend (encs, (gpointer)enc);
	
			g_value_unset (&value);
		}
	}

	if (encs != NULL)
	{
		GSList *node;
		model =	gtk_tree_view_get_model (GTK_TREE_VIEW 
							 (anjuta_encodings_dialog->supported_treeview));
		encs = g_slist_reverse (encs);
		node = encs;
		while (node)
		{
			const AnjutaEncoding *enc;
			gchar *name;
			GtkTreeIter iter;
			enc = (const AnjutaEncoding *) node->data;
			
			name = anjuta_encoding_to_string (enc);
			
			gtk_list_store_append (GTK_LIST_STORE (model), &iter);
			gtk_list_store_set (GTK_LIST_STORE (model), &iter,
						COLUMN_SUPPORTED_ENCODING_NAME, name,
						COLUMN_SUPPORTED_ENCODING, enc,
						-1);
			g_free (name);
	
			node = g_slist_next (node);
		}
		g_slist_free (encs);
		save_property ();
	}
}

static void
on_remove_encodings (GtkButton *button)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeView *treeview;
	
	treeview = GTK_TREE_VIEW (anjuta_encodings_dialog->supported_treeview);
	selection = gtk_tree_view_get_selection (treeview);
	if (selection &&
		gtk_tree_selection_get_selected (selection, &model, &iter))
	{
		gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
		save_property ();
	}
}

static void
on_up_encoding (GtkButton *button)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeView *treeview;
	
	treeview = GTK_TREE_VIEW (anjuta_encodings_dialog->supported_treeview);
	selection = gtk_tree_view_get_selection (treeview);
	if (selection &&
		gtk_tree_selection_get_selected (selection, &model, &iter))
	{
		GtkTreePath *path;
		
		path = gtk_tree_model_get_path (model, &iter);
		if (gtk_tree_path_prev (path))
		{
			GtkTreeIter prev_iter;
			gtk_tree_model_get_iter (model, &prev_iter, path);
			gtk_list_store_swap (GTK_LIST_STORE (model), &prev_iter, &iter);
		}
		gtk_tree_path_free (path);
		save_property ();
	}
}

static void
on_down_encoding (GtkButton *button)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeView *treeview;
	
	treeview = GTK_TREE_VIEW (anjuta_encodings_dialog->supported_treeview);
	selection = gtk_tree_view_get_selection (treeview);
	if (selection &&
		gtk_tree_selection_get_selected (selection, &model, &iter))
	{
		GtkTreeIter next_iter = iter;
		if (gtk_tree_model_iter_next (model, &next_iter))
		{
			gtk_list_store_swap (GTK_LIST_STORE (model), &iter, &next_iter);
			save_property ();
		}
	}
}

static void
on_stock_selection_changed (GtkTreeSelection *selection)
{
	g_return_if_fail(anjuta_encodings_dialog != NULL);
	if (gtk_tree_selection_count_selected_rows (selection) > 0)
		gtk_widget_set_sensitive (anjuta_encodings_dialog->add_button, TRUE);
	else
		gtk_widget_set_sensitive (anjuta_encodings_dialog->add_button, FALSE);
}

static void
on_supported_selection_changed (GtkTreeSelection *selection)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	
	g_return_if_fail(anjuta_encodings_dialog != NULL);	

	if (gtk_tree_selection_get_selected (selection, &model, &iter) > 0)
	{
		GtkTreePath *path;
		gtk_widget_set_sensitive (anjuta_encodings_dialog->remove_button, TRUE);
		path = gtk_tree_model_get_path (model, &iter);
		if (gtk_tree_path_prev (path))
			gtk_widget_set_sensitive (anjuta_encodings_dialog->up_button, TRUE);
		else
			gtk_widget_set_sensitive (anjuta_encodings_dialog->up_button, FALSE);
		gtk_tree_path_free (path);
		
		if (gtk_tree_model_iter_next (model, &iter))
			gtk_widget_set_sensitive (anjuta_encodings_dialog->down_button, TRUE);
		else
			gtk_widget_set_sensitive (anjuta_encodings_dialog->down_button, FALSE);
	}
	else
	{
		gtk_widget_set_sensitive (anjuta_encodings_dialog->remove_button, FALSE);
		gtk_widget_set_sensitive (anjuta_encodings_dialog->up_button, FALSE);
		gtk_widget_set_sensitive (anjuta_encodings_dialog->down_button, FALSE);
	}
}

static gchar *
get_current_value (GtkWidget *tree_view)
{
	GtkTreeView *treeview;
	GString *str;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gboolean valid;
	gchar *value;
	
	treeview = GTK_TREE_VIEW (tree_view);
	
	str = g_string_new ("");
	
	model =	gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid)
	{
		AnjutaEncoding *enc;
		gtk_tree_model_get (model, &iter, COLUMN_SUPPORTED_ENCODING, &enc, -1);
		g_assert (enc != NULL);
		g_assert (enc->charset != NULL);
		str = g_string_append (str, enc->charset);
		str = g_string_append (str, " ");
		valid = gtk_tree_model_iter_next (model, &iter);
	}
	value = g_string_free (str, FALSE);
	return value;
}

static gchar *
get_property (AnjutaProperty *prop)
{
	return get_current_value (anjuta_property_get_widget (prop));
}

static void
set_property (AnjutaProperty *prop, const gchar *value)
{
	GtkTreeView *treeview;
	GtkTreeModel *model;
	GList *list, *node;
	
	treeview = GTK_TREE_VIEW (anjuta_property_get_widget (prop));
	model = gtk_tree_view_get_model (treeview);
	gtk_list_store_clear (GTK_LIST_STORE (model));
	
	if (!value || strlen (value) <= 0)
		return;
	
	/* Fill the model */
	list = anjuta_util_glist_from_string (value);
	node = list;
	while (node)
	{
		const AnjutaEncoding *enc;
		gchar *name;
		GtkTreeIter iter;
		
		enc = anjuta_encoding_get_from_charset ((gchar *) node->data);
		name = anjuta_encoding_to_string (enc);
		
		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				    COLUMN_SUPPORTED_ENCODING_NAME, name,
					COLUMN_SUPPORTED_ENCODING, enc,
				    -1);
		g_free (name);

		node = g_list_next (node);
	}
	anjuta_util_glist_strings_free (list);
}

static void
save_property (void)
{
	gchar *value = get_current_value (anjuta_encodings_dialog->supported_treeview);
	anjuta_preferences_set (anjuta_encodings_dialog->pref, 
							SUPPORTED_ENCODINGS,
							value);
	g_free (value);
}

/**
* anjuta_encodings_init:
* @pref: an #AnjutaPreferences object.
* @gxml: an #GladeXML object holding encodings dialog.
* 
* Initializes the encodings system.
*/
void
anjuta_encodings_init (AnjutaPreferences *pref, GladeXML *gxml)
{
	GtkWidget *add_button;
	GtkWidget *remove_button;
	GtkWidget *up_button;
	GtkWidget *down_button;
	GtkWidget *supported_treeview;
	GtkWidget *stock_treeview;
	GtkTreeModel *model;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;
	
	supported_treeview = glade_xml_get_widget (gxml, "supported_treeview");
	stock_treeview =  glade_xml_get_widget (gxml, "stock_treeview");
	add_button = glade_xml_get_widget (gxml, "add_button");
	remove_button = glade_xml_get_widget (gxml, "remove_button");
	up_button = glade_xml_get_widget (gxml, "up_button");
	down_button = glade_xml_get_widget (gxml, "down_button");
	
	/* Add the encoding column for stock treeview*/
	cell = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Stock Encodings"),
													  cell, "text",
													  COLUMN_ENCODING_NAME,
													  NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (stock_treeview), column);
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (stock_treeview),
									 COLUMN_ENCODING_NAME);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (stock_treeview));
	g_return_if_fail (selection != NULL);
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

	model = create_encodings_treeview_model ();
	gtk_tree_view_set_model (GTK_TREE_VIEW (stock_treeview), model);
	g_signal_connect (G_OBJECT (selection), "changed",
					  G_CALLBACK (on_stock_selection_changed), NULL);
	g_object_unref (model);

	/* Add the encoding column for supported treeview*/
	cell = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Supported Encodings"),
													   cell, "text",
													   COLUMN_ENCODING_NAME,
													   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (supported_treeview), column);
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (supported_treeview),
									 COLUMN_ENCODING_NAME);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (supported_treeview));
	g_return_if_fail (selection != NULL);
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

	/* create list store */
	model = GTK_TREE_MODEL (gtk_list_store_new (SUPPORTED_ENCODING_NUM_COLS,
												G_TYPE_STRING, G_TYPE_POINTER));
	gtk_tree_view_set_model (GTK_TREE_VIEW (supported_treeview), model);
	g_signal_connect (G_OBJECT (selection), "changed",
					  G_CALLBACK (on_supported_selection_changed), NULL);
	g_object_unref (model);
	
	anjuta_preferences_register_property_custom (pref, supported_treeview,
												SUPPORTED_ENCODINGS,
												"ISO-8859-15",
												ANJUTA_PROPERTY_DATA_TYPE_TEXT,
												0,
												set_property,
												get_property);

	g_signal_connect (G_OBJECT (add_button), "clicked",
					  G_CALLBACK (on_add_encodings), NULL);
	g_signal_connect (G_OBJECT (remove_button), "clicked",
					  G_CALLBACK (on_remove_encodings), NULL);
	g_signal_connect (G_OBJECT (up_button), "clicked",
					  G_CALLBACK (on_up_encoding), NULL);
	g_signal_connect (G_OBJECT (down_button), "clicked",
					  G_CALLBACK (on_down_encoding), NULL);
	
	gtk_widget_set_sensitive (add_button, FALSE);
	gtk_widget_set_sensitive (remove_button, FALSE);
	gtk_widget_set_sensitive (up_button, FALSE);
	gtk_widget_set_sensitive (down_button, FALSE);
	
	anjuta_encodings_dialog = g_new0 (AnjutaEncodingsDialog, 1);
	anjuta_encodings_dialog->pref = pref;
	anjuta_encodings_dialog->add_button = add_button;
	anjuta_encodings_dialog->remove_button = remove_button;
	anjuta_encodings_dialog->up_button = up_button;
	anjuta_encodings_dialog->down_button = down_button;
	anjuta_encodings_dialog->supported_treeview = supported_treeview;
	anjuta_encodings_dialog->stock_treeview = stock_treeview;
}
