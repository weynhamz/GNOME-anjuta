/*
    style-editor.c
    Copyright (C) 2000  Naba Kumar <gnome.org>

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

#include <ctype.h>
#include <gnome.h>
#include <glade/glade.h>

#include "anjuta.h"
#include "utilities.h"
#include "style-editor.h"

gchar *hilite_style[] = {
	"Normal <Default>", "style.*.32",
	"Comments", "style.default.comment",
	"Numbers", "style.default.number",
	"Keywords", "style.default.keyword",
	"System Keywords", "style.default.syskeyword",
	"Local Keywords", "style.default.localkeyword",
	"Double Quoted Strings", "style.default.doublequote",
	"Single Quoted Strings", "style.default.singlequote",
	"Unclosed Strings", "style.default.unclosedstring",
	"Operators","style.default.operator",
	"Preprocessor Directives", "style.default.preprocessor",
	"Identifiers (Not C Style)", "style.default.identifier",
	"Definitions (Not C Style)", "style.default.definition",
	"Functions (Not C Style)", "style.default.function",
	"Matched Braces", "style.*.34",
	"Incomplete Brace", "style.*.35",
	"Control Characters", "style.*.36",
	"Line Numbers", "style.*.33",
	"Indentation Guides", "style.*.37",
	NULL, NULL
};

typedef struct _StyleData StyleData;

struct _StyleData
{
	gchar *item;
	gchar *font;
	gint size;
	gboolean bold, italics, underlined;
	gchar *fore, *back;
	gboolean eolfilled;

	gboolean font_use_default;
	gboolean attrib_use_default;
	gboolean fore_use_default;
	gboolean back_use_default;
};

struct _StyleEditorPriv
{
	/* Widgets */
	GtkWidget *dialog;
	GtkWidget *hilite_item_combo;
	GtkWidget *font_picker;
	GtkWidget *font_bold_check;
	GtkWidget *font_italics_check;
	GtkWidget *font_underlined_check;
	GtkWidget *fore_colorpicker;
	GtkWidget *back_colorpicker;
	GtkWidget *font_use_default_check;
	GtkWidget *font_attrib_use_default_check;
	GtkWidget *fore_color_use_default_check;
	GtkWidget *back_color_use_default_check;
	GtkWidget *caret_fore_color;
	GtkWidget *selection_fore_color;
	GtkWidget *selection_back_color;
	GtkWidget *calltip_back_color;
	
	/* Data */
	StyleData *default_style;
	StyleData *current_style;
};

static gchar *
style_data_get_string (StyleData * sdata)
{
	gchar *tmp, *str;

	g_return_if_fail (sdata);
	
	str = NULL;
	if ((sdata->font) && strlen (sdata->font) && sdata->font_use_default == FALSE)
	{
		str = g_strconcat ("font:", sdata->font, NULL);
	}
	if (sdata->size > 0 && sdata->font_use_default == FALSE)
	{
		tmp = str;
		if (tmp)
		{
			str =
				g_strdup_printf ("%s,size:%d", tmp,
						 sdata->size);
			g_free (tmp);
		}
		else
			str = g_strdup_printf ("size:%d", sdata->size);

	}
	if (sdata->attrib_use_default == FALSE)
	{
		if (sdata->bold)
		{
			tmp = str;
			if (tmp)
			{
				str = g_strconcat (tmp, ",bold", NULL);
				g_free (tmp);
			}
			else
				str = g_strdup ("bold");
		}
		else
		{
			tmp = str;
			if (tmp)
			{
				str = g_strconcat (tmp, ",notbold", NULL);
				g_free (tmp);
			}
			else
				str = g_strdup ("notbold");
		}
		if (sdata->italics)
		{
			tmp = str;
			if (tmp)
			{
				str = g_strconcat (tmp, ",italics", NULL);
				g_free (tmp);
			}
			else
				str = g_strdup ("italics");
		}
		else
		{
			tmp = str;
			if (tmp)
			{
				str = g_strconcat (tmp, ",notitalics", NULL);
				g_free (tmp);
			}
			else
				str = g_strdup ("notitalics");
		}
		if (sdata->underlined)
		{
			tmp = str;
			if (tmp)
			{
				str = g_strconcat (tmp, ",underlined", NULL);
				g_free (tmp);
			}
			else
				str = g_strdup ("underlined");
		}
		else
		{
			tmp = str;
			if (tmp)
			{
				str =
					g_strconcat (tmp, ",notunderlined",
						     NULL);
				g_free (tmp);
			}
			else
				str = g_strdup ("notunderlined");
		}
		if (sdata->eolfilled)
		{
			tmp = str;
			if (tmp)
			{
				str = g_strconcat (tmp, ",eolfilled", NULL);
				g_free (tmp);
			}
			else
				str = g_strdup ("eolfilled");
		}
		else
		{
			tmp = str;
			if (tmp)
			{
				str =
					g_strconcat (tmp, ",noteolfilled",
						     NULL);
				g_free (tmp);
			}
			else
				str = g_strdup ("noteolfilled");
		}
	}
	if (sdata->fore_use_default == FALSE)
	{
		tmp = str;
		if (tmp)
		{
			str = g_strconcat (tmp, ",fore:", sdata->fore, NULL);
			g_free (tmp);
		}
		else
			str = g_strconcat ("fore:", sdata->fore, NULL);
	}
	if (sdata->back_use_default == FALSE)
	{
		tmp = str;
		if (tmp)
		{
			str = g_strconcat (tmp, ",back:", sdata->back, NULL);
			g_free (tmp);
		}
		else
			str = g_strconcat ("back:", sdata->back, NULL);
	}
	if (str == NULL)
		str = g_strdup ("");
	return str;
}

static void
style_data_set_font (StyleData * sdata, const gchar *font)
{
	PangoFontDescription *desc;
	const gchar *font_family;
	
	g_return_if_fail (sdata);
	
	desc = pango_font_description_from_string (font);
	font_family = pango_font_description_get_family(desc);
	string_assign (&sdata->font, font_family);
	
	/* Change to lower case */
	if (sdata->font)
	{
		gchar *s = sdata->font;
		while(*s)
		{
			*s = tolower(*s);
			s++;
		}
	}
	pango_font_description_free (desc);
}

static void
style_data_set_font_size_from_pango (StyleData * sdata, const gchar *font)
{
	PangoFontDescription *desc;
	
	g_return_if_fail (sdata);
	
	desc = pango_font_description_from_string (font);
	sdata->size = pango_font_description_get_size (desc) / PANGO_SCALE;
	pango_font_description_free (desc);
}

static void
style_data_set_fore (StyleData * sdata, const gchar *fore)
{
	g_return_if_fail (sdata);	
	string_assign (&sdata->fore, fore);
}

static void
style_data_set_back (StyleData * sdata, const gchar *back)
{
	g_return_if_fail (sdata);	
	string_assign (&sdata->back, back);
}

static void
style_data_set_item (StyleData * sdata, const gchar *item)
{
	g_return_if_fail (sdata);
	string_assign (&sdata->item, item);
}

static StyleData *
style_data_new (void)
{
	StyleData *sdata;
	sdata = g_new0 (StyleData, 1);
	
	sdata->font = g_strdup ("");
	sdata->size = 12;
	sdata->font_use_default = TRUE;
	sdata->attrib_use_default = TRUE;
	sdata->fore_use_default = TRUE;
	sdata->back_use_default = TRUE;
	style_data_set_fore (sdata, "#000000");	/* Black */
	style_data_set_back (sdata, "#FFFFFF");	/* White */
	return sdata;
}

static void
style_data_destroy (StyleData * sdata)
{
	if (!sdata)
		return;
	if (sdata->item)
		g_free (sdata->item);
	if (sdata->font)
		g_free (sdata->font);
	if (sdata->fore)
		g_free (sdata->fore);
	if (sdata->back)
		g_free (sdata->back);
	g_free (sdata);
}

static StyleData *
style_data_new_parse (gchar * style_string)
{
	gchar *val, *opt;
	StyleData *style_data;

	style_data = style_data_new ();
	if (!style_data)
		return NULL;

	val = g_strdup (style_string);
	opt = val;
	while (opt)
	{
		gchar *cpComma, *colon;

		cpComma = strchr (opt, ',');
		if (cpComma)
			*cpComma = '\0';
		colon = strchr (opt, ':');
		if (colon)
			*colon++ = '\0';
		if (0 == strcmp (opt, "italics"))
		{
			style_data->italics = TRUE;
			style_data->attrib_use_default = FALSE;
		}
		if (0 == strcmp (opt, "notitalics"))
		{
			style_data->italics = FALSE;
			style_data->attrib_use_default = FALSE;
		}
		if (0 == strcmp (opt, "bold"))
		{
			style_data->bold = TRUE;
			style_data->attrib_use_default = FALSE;
		}
		if (0 == strcmp (opt, "notbold"))
		{
			style_data->bold = FALSE;
			style_data->attrib_use_default = FALSE;
		}
		if (0 == strcmp (opt, "font"))
		{
			style_data_set_font (style_data, colon);
			style_data->font_use_default = FALSE;
		}
		if (0 == strcmp (opt, "fore"))
		{
			style_data_set_fore (style_data, colon);
			style_data->fore_use_default = FALSE;
		}
		if (0 == strcmp (opt, "back"))
		{
			style_data_set_back (style_data, colon);
			style_data->back_use_default = FALSE;
		}
		if (0 == strcmp (opt, "size"))
		{
			style_data->size = atoi (colon);
			style_data->font_use_default = FALSE;
		}
		if (0 == strcmp (opt, "eolfilled"))
			style_data->eolfilled = TRUE;
		if (0 == strcmp (opt, "noteolfilled"))
			style_data->eolfilled = FALSE;
		if (0 == strcmp (opt, "underlined"))
		{
			style_data->underlined = TRUE;
			style_data->attrib_use_default = FALSE;
		}
		if (0 == strcmp (opt, "notunderlined"))
		{
			style_data->underlined = FALSE;
			style_data->fore_use_default = FALSE;
		}
		if (cpComma)
			opt = cpComma + 1;
		else
			opt = 0;
	}
	if (val)
		g_free (val);
	return style_data;
}

static void
on_use_default_font_toggled (GtkToggleButton * tb, gpointer data)
{
	StyleEditor *p;
	gchar* font_name;
	gboolean state;

	g_return_if_fail (data);
	p = data;

	gtk_widget_set_sensitive (p->priv->font_picker, TRUE);
	state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (tb));
	if (state)
	{
		font_name = g_strdup_printf ("%s %d",
									 p->priv->default_style->font,
									 p->priv->default_style->size);
	}
	else
	{
		if (p->priv->current_style->font &&
			strlen (p->priv->current_style->font))
		{
			font_name = g_strdup_printf ("%s %d",
										 p->priv->current_style->font,
										 p->priv->current_style->size);
		}
		else
		{
			font_name = g_strdup_printf ("%s %d",
										 p->priv->default_style->font,
										 p->priv->default_style->size);
		}
	}
	gtk_widget_set_sensitive (p->priv->font_picker, !state);
	gnome_font_picker_set_font_name (GNOME_FONT_PICKER (p->priv->font_picker),
		font_name);
	g_free(font_name);
}

static void
on_use_default_attrib_toggled (GtkToggleButton * tb, gpointer data)
{
	StyleEditor *p;

	g_return_if_fail (data);
	p = data;
	gtk_widget_set_sensitive (p->priv->font_bold_check, TRUE);
	gtk_widget_set_sensitive (p->priv->font_italics_check, TRUE);
	gtk_widget_set_sensitive (p->priv->font_underlined_check, TRUE);
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (tb)))
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (p->priv->font_bold_check),
					      p->priv->default_style->bold);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (p->priv->font_italics_check),
					      p->priv->default_style->italics);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (p->priv->
					       font_underlined_check),
					      p->priv->default_style->underlined);
		gtk_widget_set_sensitive (p->priv->font_bold_check, FALSE);
		gtk_widget_set_sensitive (p->priv->font_italics_check,
					  FALSE);
		gtk_widget_set_sensitive (p->priv->font_underlined_check,
					  FALSE);
	}
	else
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (p->priv->font_bold_check),
					      p->priv->current_style->bold);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (p->priv->font_italics_check),
					      p->priv->current_style->italics);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (p->priv->
					       font_underlined_check),
					      p->priv->current_style->underlined);
	}
}

static void
on_use_default_fore_toggled (GtkToggleButton * tb, gpointer data)
{
	StyleEditor *p;
	guint8 r, g, b;

	g_return_if_fail (data);
	p = data;

	gtk_widget_set_sensitive (p->priv->fore_colorpicker, TRUE);
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (tb)))
	{
		anjuta_util_color_from_string (p->priv->default_style->fore, &r, &g, &b);
		gnome_color_picker_set_i8 (GNOME_COLOR_PICKER
					   (p->priv->fore_colorpicker), r,
					   g, b, 0);
		gtk_widget_set_sensitive (p->priv->fore_colorpicker, FALSE);
	}
	else
	{
		anjuta_util_color_from_string (p->priv->current_style->fore, &r, &g, &b);
		gnome_color_picker_set_i8 (GNOME_COLOR_PICKER
					   (p->priv->fore_colorpicker), r,
					   g, b, 0);
	}
}

static void
on_use_default_back_toggled (GtkToggleButton * tb, gpointer data)
{
	StyleEditor *p;
	guint8 r, g, b;

	g_return_if_fail (data);
	p = data;
	gtk_widget_set_sensitive (p->priv->back_colorpicker, TRUE);
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (tb)))
	{
		anjuta_util_color_from_string (p->priv->default_style->back, &r, &g, &b);
		gnome_color_picker_set_i8 (GNOME_COLOR_PICKER
					   (p->priv->back_colorpicker), r,
					   g, b, 0);
		gtk_widget_set_sensitive (p->priv->back_colorpicker, FALSE);
	}
	else
	{
		anjuta_util_color_from_string (p->priv->current_style->back, &r, &g, &b);
		gnome_color_picker_set_i8 (GNOME_COLOR_PICKER
					   (p->priv->back_colorpicker), r,
					   g, b, 0);
	}
}

static void
on_hilite_style_entry_changed (GtkEditable * editable, gpointer user_data)
{
	StyleEditor *p;
	const gchar *style_item;

	g_return_if_fail (user_data);
	p = user_data;

	style_item = gtk_entry_get_text (GTK_ENTRY (editable));
	if (!style_item || strlen (style_item) <= 0)
		return;
	if (p->priv->current_style)
	{
		guint8 r, g, b, a;
		gchar *str;
		const gchar *font;

		font = gnome_font_picker_get_font_name (GNOME_FONT_PICKER
												(p->priv->font_picker));
		if (font)
		{
			style_data_set_font (p->priv->current_style, font);
			style_data_set_font_size_from_pango (p->priv->current_style, font);
		}
		else
		{
			style_data_set_font (p->priv->current_style,
								 p->priv->default_style->font);
			p->priv->current_style->size = p->priv->default_style->size;
		}
		
		p->priv->current_style->bold =
			gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
						      (p->priv->font_bold_check));
		p->priv->current_style->italics =
			gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
						      (p->priv->font_italics_check));
		p->priv->current_style->underlined =
			gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
						      (p->priv->font_underlined_check));
		p->priv->current_style->eolfilled =
			(strcmp (style_item, hilite_style[1]) == 0);

		gnome_color_picker_get_i8 (GNOME_COLOR_PICKER
					   (p->priv->fore_colorpicker), &r,
					   &g, &b, &a);
		str = anjuta_util_string_from_color (r, g, b);
		style_data_set_fore (p->priv->current_style, str);
		g_free (str);

		gnome_color_picker_get_i8 (GNOME_COLOR_PICKER
					   (p->priv->back_colorpicker), &r, &g, &b, &a);
		str = anjuta_util_string_from_color (r, g, b);
		style_data_set_back (p->priv->current_style, str);
		g_free (str);

		p->priv->current_style->font_use_default =
			gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
						      (p->priv->font_use_default_check));
		p->priv->current_style->attrib_use_default =
			gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
						      (p->priv->font_attrib_use_default_check));
		p->priv->current_style->fore_use_default =
			gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
						      (p->priv->fore_color_use_default_check));
		p->priv->current_style->back_use_default =
			gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
						      (p->priv->back_color_use_default_check));
	}
	p->priv->current_style =
		g_object_get_data (G_OBJECT (p->priv->dialog),
				     style_item);
	
	g_return_if_fail (p->priv->current_style);
	
	/* We need to first toggle then set active to work properly */
	gtk_toggle_button_toggled (GTK_TOGGLE_BUTTON
				   (p->priv->font_use_default_check));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (p->priv->font_use_default_check),
				      p->priv->current_style->font_use_default);

	gtk_toggle_button_toggled (GTK_TOGGLE_BUTTON
				   (p->priv->font_attrib_use_default_check));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (p->priv->font_attrib_use_default_check),
				      p->priv->current_style->attrib_use_default);

	gtk_toggle_button_toggled (GTK_TOGGLE_BUTTON
				   (p->priv->fore_color_use_default_check));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (p->priv->fore_color_use_default_check),
				      p->priv->current_style->fore_use_default);

	gtk_toggle_button_toggled (GTK_TOGGLE_BUTTON
				   (p->priv->back_color_use_default_check));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (p->priv->back_color_use_default_check),
				      p->priv->current_style->back_use_default);
}

static void
sync_from_props (StyleEditor *se)
{
	gint i;
	gchar *str;
	guint8 r, g, b;
	
	g_return_if_fail (se);
	/* Never hurts to use g_object_*_data as temp hash buffer */
	for (i = 0;; i += 2)
	{
		StyleData *sdata;

		if (hilite_style[i] == NULL)
			break;
		str = prop_get_expanded (se->props, hilite_style[i + 1]);
		sdata = style_data_new_parse (str);
		if (str)
			g_free (str);

		style_data_set_item (sdata, hilite_style[i]);
		g_object_set_data_full (G_OBJECT (se->priv->dialog),
					  hilite_style[i], sdata,
					  (GtkDestroyNotify)style_data_destroy);
	}
	se->priv->default_style =
		gtk_object_get_data (GTK_OBJECT (se->priv->dialog),
				     hilite_style[0]);
	se->priv->current_style = NULL;

	on_hilite_style_entry_changed (GTK_EDITABLE (GTK_COMBO
					(se->priv->hilite_item_combo)->entry), se);

	str = prop_get (se->props, CARET_FORE_COLOR);
	if(str)
	{
		anjuta_util_color_from_string (str, &r, &g, &b);
		gnome_color_picker_set_i8 (GNOME_COLOR_PICKER
								   (se->priv->caret_fore_color), r, g, b, 0);
		g_free (str);
	}
	else
	{
		gnome_color_picker_set_i8 (GNOME_COLOR_PICKER
								   (se->priv->caret_fore_color), 0, 0, 0, 0);
	}

	str = prop_get (se->props, CALLTIP_BACK_COLOR);
	if(str)
	{
		anjuta_util_color_from_string (str, &r, &g, &b);
		gnome_color_picker_set_i8 (GNOME_COLOR_PICKER
								   (se->priv->calltip_back_color), r, g, b, 0);
		g_free (str);
	}
	else
	{
		anjuta_util_color_from_string ("#E6D6B6", &r, &g, &b);
		gnome_color_picker_set_i8 (GNOME_COLOR_PICKER
								   (se->priv->calltip_back_color), r, g, b, 0);
	}
	
	str = prop_get (se->props, SELECTION_FORE_COLOR);
	if(str)
	{
		anjuta_util_color_from_string (str, &r, &g, &b);
		gnome_color_picker_set_i8 (GNOME_COLOR_PICKER
								   (se->priv->selection_fore_color), r, g, b, 0);
		g_free (str);
	}
	else
	{
		anjuta_util_color_from_string ("#FFFFFF", &r, &g, &b);
		gnome_color_picker_set_i8 (GNOME_COLOR_PICKER
								   (se->priv->selection_fore_color), r, g, b, 0);
	}
	
	str = prop_get (se->props, SELECTION_BACK_COLOR);
	if(str)
	{
		anjuta_util_color_from_string (str, &r, &g, &b);
		gnome_color_picker_set_i8 (GNOME_COLOR_PICKER
								   (se->priv->selection_back_color), r, g, b, 0);
		g_free (str);
	}
	else
	{
		gnome_color_picker_set_i8 (GNOME_COLOR_PICKER
								   (se->priv->selection_back_color), 0, 0, 0, 0);
	}
}

static void
sync_to_props (StyleEditor *se)
{
	gint i;
	gchar *str;
	guint8 r, g, b, a;

	g_return_if_fail (se);
	/* Sync the current item */	
	on_hilite_style_entry_changed (GTK_EDITABLE (GTK_COMBO
					(se->priv->hilite_item_combo)->entry), se);
	
	/* Transfer to props */
	for (i = 0;; i += 2)
	{
		StyleData *sdata;

		if (hilite_style[i] == NULL)
			break;
		sdata =
			gtk_object_get_data (GTK_OBJECT (se->priv->dialog),
								 hilite_style[i]);
		str = style_data_get_string (sdata);
		if (str)
		{
			prop_set_with_key (se->props, hilite_style[i + 1], str);
			g_free (str);
		}
	}
	gnome_color_picker_get_i8 (GNOME_COLOR_PICKER
				   (se->priv->caret_fore_color), &r,
				   &g, &b, &a);
	str = anjuta_util_string_from_color (r, g, b);
	if(str)
	{
		prop_set_with_key (se->props, CARET_FORE_COLOR, str);
		g_free (str);
	}
	gnome_color_picker_get_i8 (GNOME_COLOR_PICKER
							   (se->priv->calltip_back_color), &r,
								&g, &b, &a);
	str = anjuta_util_string_from_color (r, g, b);
	if(str)
	{
		prop_set_with_key (se->props, CALLTIP_BACK_COLOR, str);
		g_free (str);
	}
	gnome_color_picker_get_i8 (GNOME_COLOR_PICKER
							   (se->priv->selection_fore_color), &r,
							    &g, &b, &a);
	str = anjuta_util_string_from_color (r, g, b);
	if(str)
	{
		prop_set_with_key (se->props, SELECTION_FORE_COLOR, str);
		g_free (str);
	}
	gnome_color_picker_get_i8 (GNOME_COLOR_PICKER
							   (se->priv->selection_back_color), &r,
								&g, &b, &a);
	str = anjuta_util_string_from_color (r, g, b);
	if(str)
	{
		prop_set_with_key (se->props, SELECTION_BACK_COLOR, str);
		g_free (str);
	}
}

static void
on_response (GtkWidget *dialog, gint res, StyleEditor *se)
{
	g_return_if_fail (se);
	
	switch (res)
	{
	case GTK_RESPONSE_APPLY:
		sync_to_props (se);
		anjuta_apply_styles ();
		return;
	case GTK_RESPONSE_OK:
		sync_to_props (se);
		anjuta_apply_styles ();
	case GTK_RESPONSE_CANCEL:
		style_editor_hide (se);
		return;
	}
}

static void
on_delete_event (GtkWidget *dialog, GdkEvent *event, StyleEditor *se)
{
	g_return_if_fail (se);
	style_editor_hide (se);
}

static void
create_style_editor_gui (StyleEditor * se)
{
	GladeXML *gxml;
	GList *list = NULL;
	gint i;

	g_return_if_fail (se);
	g_return_if_fail (se->priv->dialog == NULL);
	
	gxml = glade_xml_new (GLADE_FILE_ANJUTA, "style_editor_dialog", NULL);
	se->priv->dialog = glade_xml_get_widget (gxml, "style_editor_dialog");
	gtk_widget_show (se->priv->dialog);
	se->priv->font_picker = glade_xml_get_widget (gxml, "font");
	se->priv->hilite_item_combo = glade_xml_get_widget (gxml, "combo");
	se->priv->font_bold_check = glade_xml_get_widget (gxml, "bold");
	se->priv->font_italics_check = glade_xml_get_widget (gxml, "italic");
	se->priv->font_underlined_check = glade_xml_get_widget (gxml, "underlined");
	se->priv->fore_colorpicker = glade_xml_get_widget (gxml, "fore_color");
	se->priv->back_colorpicker = glade_xml_get_widget (gxml, "back_color");
	se->priv->font_use_default_check = glade_xml_get_widget (gxml, "font_default");
	se->priv->font_attrib_use_default_check = glade_xml_get_widget (gxml, "attributes_default");
	se->priv->fore_color_use_default_check = glade_xml_get_widget (gxml, "fore_default");
	se->priv->back_color_use_default_check = glade_xml_get_widget (gxml, "back_default");
	se->priv->caret_fore_color = glade_xml_get_widget (gxml, "caret");
	se->priv->calltip_back_color = glade_xml_get_widget (gxml, "calltip");
	se->priv->selection_fore_color = glade_xml_get_widget (gxml, "selection_fore");
	se->priv->selection_back_color = glade_xml_get_widget (gxml, "selection_back");
	
	for (i = 0;; i += 2)
	{
		if (hilite_style[i] == NULL)
			break;
		list = g_list_append (list, hilite_style[i]);
	}
	gtk_combo_set_popdown_strings (GTK_COMBO (se->priv->hilite_item_combo), list);
	g_list_free (list);
	
	gtk_window_set_transient_for (GTK_WINDOW (se->priv->dialog),
								  GTK_WINDOW (app->widgets.window));
	
	g_signal_connect (G_OBJECT (GTK_COMBO(se->priv->hilite_item_combo)->entry),
					  "changed", G_CALLBACK (on_hilite_style_entry_changed),
					  se);
	g_signal_connect (G_OBJECT (se->priv->font_use_default_check),
					  "toggled", G_CALLBACK (on_use_default_font_toggled),
					  se);
	g_signal_connect (G_OBJECT (se->priv->font_attrib_use_default_check),
					  "toggled", G_CALLBACK (on_use_default_attrib_toggled),
					  se);
	g_signal_connect (G_OBJECT (se->priv->fore_color_use_default_check),
					  "toggled", G_CALLBACK (on_use_default_fore_toggled),
					  se);
	g_signal_connect (G_OBJECT (se->priv->back_color_use_default_check),
					  "toggled", G_CALLBACK (on_use_default_back_toggled),
					  se);
	g_signal_connect (G_OBJECT (se->priv->dialog),
					  "delete_event", G_CALLBACK (on_delete_event),
					  se);
	g_signal_connect (G_OBJECT (se->priv->dialog),
					  "response", G_CALLBACK (on_response),
					  se);
	g_object_unref (gxml);
}

StyleEditor *
style_editor_new (PropsID props_global, PropsID props_local,
				  PropsID props_session, PropsID props)
{
	StyleEditor *se;
	se = g_new0 (StyleEditor, 1);
	se->priv = g_new0 (StyleEditorPriv, 1);
	se->props_global = props_global;
	se->props_local = props_local;
	se->props_session = props_session;
	se->props = props;
	se->priv->dialog = NULL;
	return se;
}

void style_editor_destroy (StyleEditor *se)
{
	g_return_if_fail (se);
	if (se->priv->dialog)
		gtk_widget_destroy (se->priv->dialog);
	g_free (se->priv);
	g_free (se);
}

void style_editor_show (StyleEditor *se)
{
	g_return_if_fail (se);
	if (se->priv->dialog)
		return;
	create_style_editor_gui (se);
	sync_from_props (se);
}

void style_editor_hide (StyleEditor *se)
{
	g_return_if_fail (se);
	g_return_if_fail (se->priv->dialog);
	gtk_widget_destroy (se->priv->dialog);
	se->priv->dialog = NULL;
}

void
style_editor_save_yourself (StyleEditor *se, FILE *fp)
{
	gint i;
	gchar *str;
	
	for (i = 0;; i += 2)
	{
		if (hilite_style[i] == NULL)
			break;
		str = prop_get (se->props, hilite_style[i + 1]);
		if (str)
		{
			fprintf (fp, "%s=%s\n", hilite_style[i + 1], str);
			g_free (str);
		}
		/* else
			fprintf (fp, "%s=\n", hilite_style[i + 1]); */
	}
	str = prop_get (se->props, CARET_FORE_COLOR);
	if(str)
	{
		fprintf (fp, "%s=%s\n", CARET_FORE_COLOR, str);
		g_free (str);
	}

	str = prop_get (se->props, CALLTIP_BACK_COLOR);
	if(str)
	{
		fprintf (fp, "%s=%s\n", CALLTIP_BACK_COLOR, str);
		g_free (str);
	}
	
	str = prop_get (se->props, SELECTION_FORE_COLOR);
	if(str)
	{
		fprintf (fp, "%s=%s\n", SELECTION_FORE_COLOR, str);
		g_free (str);
	}
	
	str = prop_get (se->props, SELECTION_BACK_COLOR);
	if(str)
	{
		fprintf (fp, "%s=%s\n", SELECTION_BACK_COLOR, str);
		g_free (str);
	}
}
