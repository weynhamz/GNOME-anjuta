/***************************************************************************
 *            memory.c
 *
 *  Sun Jun 23 12:56:43 2002
 *  Copyright  2002  Jean-Noel Guiheneuf
 *  jnoel@saudionline.com.sa
 ****************************************************************************/
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif


#include <signal.h>
#include <setjmp.h>

#include <gnome.h>

#include "utilities.h"
#include "memory.h"


#define MEM_NB_LINES 16
#define MAX_ADR 8



static gint hexa_to_decimal (gchar c);

static guchar *adr_to_decimal (gchar * hexa);

static void remove_space (gchar * string);

static gboolean is_hexa (gchar c);

static gchar convert_hexa (gchar c);

static gchar *convert_adr_hexa (glong adr);

static gchar *convert_hexa_byte (gchar c);

static guchar convert_ascii_print (guchar c);

static void on_sigsegv_emit (gint sig);

static void mem (MemApp * memapp, guchar * ptr);

static void entry_filter (GtkEditable * editable, const gchar * text,
			  gint length, gint * pos, gpointer data);

static void mem_clear_gtktext (GtkText * text);

static void mem_clear_gtktext_all (MemApp * memapp);

static gboolean on_text_data_button_release_event (GtkWidget * widget,
						   GdkEventButton * event,
						   MemApp * memapp);

static void on_quit_button_clicked (GtkButton * button, MemApp * memapp);

static void on_inspect_button_clicked (GtkButton * button, MemApp * memapp);

static void on_quit_button_clicked (GtkButton * button, MemApp * memapp);

static void mem_move_up (MemApp * memapp);

static void mem_move_page_up (MemApp * memapp);

static void mem_move_down (MemApp * memapp);

static void mem_move_page_down (MemApp * memapp);

static gboolean on_eventbox1_button_press_event (GtkWidget * widget,
						 GdkEventButton * event,
						 MemApp * memapp);
static gboolean on_eventbox2_button_press_event (GtkWidget * widget,
						 GdkEventButton * event,
						 MemApp * memapp);
static gboolean on_text1_key_press_event (GtkWidget * widget,
					  GdkEventKey * event,
					  MemApp * memapp);
static void on_window_destroy (GtkObject * object, MemApp * memapp);



jmp_buf env;



GtkWidget *
create_info_memory (guchar * ptr)
{
	GtkWidget *vbox1;
	GtkWidget *vbox2;
	GtkWidget *hbox1;
	GtkWidget *hseparator1;
	GtkWidget *hbox2;
	GtkWidget *button1;
	GtkWidget *button2;
	GtkWidget *eventbox1;
	GtkWidget *eventbox2;
	GtkWidget *arrow1;
	GtkWidget *arrow2;
	GtkStyle *default_style;
	GdkFont *default_font;
	GtkWidget *label1;
	gint car_width;
	gint car_height;
	gint default_car_width;
	MemApp *mem_app;

	mem_app = (MemApp *) g_malloc0 (sizeof (MemApp));
	mem_app->adr = ptr;

	mem_app->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_object_set_data (GTK_OBJECT (mem_app->window), "Memory",
			     mem_app->window);
	gtk_window_set_title (GTK_WINDOW (mem_app->window), _("Memory"));
	gtk_window_set_policy (GTK_WINDOW (mem_app->window), FALSE, FALSE,
			       TRUE);
	mem_app->fixed_font = get_fixed_font ();
	car_width = gdk_char_width (mem_app->fixed_font, '_');
	car_height = gdk_char_height (mem_app->fixed_font, '|');

	default_style = gtk_widget_get_style (GTK_WIDGET (mem_app->window));
	default_font = default_style->font;
	default_car_width = gdk_char_width (default_font, '_');

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox1);
	gtk_container_add (GTK_CONTAINER (mem_app->window), vbox1);

	hbox1 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox1);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox1, TRUE, TRUE, 0);

	vbox2 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox2);
	gtk_box_pack_start (GTK_BOX (hbox1), vbox2, FALSE, TRUE, 0);

	eventbox1 = gtk_event_box_new ();
	gtk_widget_show (eventbox1);
	gtk_box_pack_start (GTK_BOX (vbox2), eventbox1, FALSE, TRUE, 0);
	arrow1 = gtk_arrow_new (GTK_ARROW_UP, GTK_SHADOW_OUT);
	gtk_widget_show (arrow1);
	gtk_container_add (GTK_CONTAINER (eventbox1), arrow1);

	eventbox2 = gtk_event_box_new ();
	gtk_widget_show (eventbox2);
	gtk_box_pack_end (GTK_BOX (vbox2), eventbox2, FALSE, TRUE, 0);
	arrow2 = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_OUT);
	gtk_widget_show (arrow2);
	gtk_container_add (GTK_CONTAINER (eventbox2), arrow2);

	mem_app->text_adr = gtk_text_new (NULL, NULL);
	gtk_widget_show (mem_app->text_adr);
	gtk_widget_set_usize (GTK_WIDGET (mem_app->text_adr),
			      car_width * (MAX_ADR + 4),
			      (car_height + 8) * MEM_NB_LINES);
	gtk_box_pack_start (GTK_BOX (hbox1), mem_app->text_adr, TRUE, TRUE,
			    0);

	mem_app->text_data = gtk_text_new (NULL, NULL);
	gtk_widget_show (mem_app->text_data);
	gtk_widget_set_usize (GTK_WIDGET (mem_app->text_data), car_width * 52,
			      (car_height + 8) * MEM_NB_LINES);
	gtk_box_pack_start (GTK_BOX (hbox1), mem_app->text_data, TRUE, TRUE,
			    0);

	mem_app->text_ascii = gtk_text_new (NULL, NULL);
	gtk_widget_show (mem_app->text_ascii);
	gtk_widget_set_usize (GTK_WIDGET (mem_app->text_ascii),
			      car_width * 19,
			      (car_height + 8) * MEM_NB_LINES);
	gtk_box_pack_start (GTK_BOX (hbox1), mem_app->text_ascii, TRUE, TRUE,
			    0);

	mem_app->mem_label = gtk_label_new ("");
	gtk_widget_show (mem_app->mem_label);
	// gtk_widget_set_usize (GTK_WIDGET (mem_app->mem_label),
	//		      default_car_width * 30, 0);
	gtk_box_pack_start (GTK_BOX (vbox1), mem_app->mem_label, TRUE, TRUE,
			    0);

	hseparator1 = gtk_hseparator_new ();
	gtk_widget_show (hseparator1);
	gtk_box_pack_start (GTK_BOX (vbox1), hseparator1, TRUE, TRUE, 5);

	hbox2 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox2);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox2, TRUE, TRUE, 5);

	button1 = gtk_button_new_with_label (_("Inspect"));
	GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);
	gtk_widget_show (button1);
	gtk_box_pack_start (GTK_BOX (hbox2), button1, TRUE, TRUE, 5);

	label1 = gtk_label_new (_("Hexa address"));
	gtk_widget_show (label1);
	gtk_box_pack_start (GTK_BOX (hbox2), label1, FALSE, FALSE, 0);

	mem_app->adr_entry = gtk_entry_new ();
	gtk_widget_show (mem_app->adr_entry);
	gtk_widget_set_usize (GTK_WIDGET (mem_app->adr_entry),
			      default_car_width * (MAX_ADR + 2), 0);
	gtk_box_pack_start (GTK_BOX (hbox2), mem_app->adr_entry, FALSE, FALSE,
			    0);

	button2 = gtk_button_new_with_label (_("Quit"));
	gtk_widget_show (button2);
	gtk_box_pack_start (GTK_BOX (hbox2), button2, TRUE, TRUE, 5);


	gtk_signal_connect (GTK_OBJECT (button1), "clicked",
			    GTK_SIGNAL_FUNC (on_inspect_button_clicked),
			    mem_app);
	gtk_signal_connect (GTK_OBJECT (button2), "clicked",
			    GTK_SIGNAL_FUNC (on_quit_button_clicked),
			    mem_app);
	gtk_signal_connect (GTK_OBJECT (mem_app->adr_entry), "insert_text",
			    GTK_SIGNAL_FUNC (entry_filter), NULL);
	gtk_signal_connect (GTK_OBJECT (mem_app->text_data),
			    "button_release_event",
			    GTK_SIGNAL_FUNC
			    (on_text_data_button_release_event), mem_app);
	gtk_signal_connect (GTK_OBJECT (mem_app->text_data),
			    "key_press_event",
			    GTK_SIGNAL_FUNC (on_text1_key_press_event),
			    mem_app);
	gtk_signal_connect (GTK_OBJECT (eventbox1), "button_press_event",
			    GTK_SIGNAL_FUNC (on_eventbox1_button_press_event),
			    mem_app);
	gtk_signal_connect (GTK_OBJECT (eventbox2), "button_press_event",
			    GTK_SIGNAL_FUNC (on_eventbox2_button_press_event),
			    mem_app);
	gtk_signal_connect (GTK_OBJECT (mem_app->window), "destroy",
			    GTK_SIGNAL_FUNC (on_window_destroy), mem_app);

	if (ptr != NULL)
		mem (mem_app, ptr);

	gtk_widget_grab_focus (mem_app->adr_entry);
	gtk_widget_grab_default (button1);

	return mem_app->window;
}


static gint
hexa_to_decimal (gchar c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	else
		return 0;
}


static guchar *
adr_to_decimal (gchar * hexa)
{
	gchar *ptr;
	glong dec;

	ptr = hexa;
	dec = 0;
	while (*ptr)
	{
		dec = dec * 16 + hexa_to_decimal (*ptr++);
	}
	return (guchar *) dec;
}


static void
remove_space (gchar * string)
{
	gchar *ptr, *str;

	ptr = str = string;
	while (*str)
	{
		if (*str == ' ' || *str == '\n')
			str++;
		else
			*(ptr++) = *(str++);
	}
	*ptr = '\0';
}


static gboolean
is_hexa (gchar c)
{
	if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') ||
	    (c >= 'a' && c <= 'f'))
		return TRUE;
	else
		return FALSE;
}

static gchar
convert_hexa (gchar c)
{
	if (c < 10)
		return c + '0';
	else
		return c + 'A' - 10;
}

static gchar *
convert_adr_hexa (glong adr)
{
	static gchar buffer[9];
	static glong r;
	guchar c;
	gint i = 0;

	while (adr > 15 && i < MAX_ADR)
	{
		c = adr % 16;
		r = adr / 16;
		buffer[i++] = convert_hexa (c);
		adr = r;
	}
	buffer[i++] = convert_hexa (r);
	buffer[i] = '\0';
	g_strreverse (buffer);

	return buffer;
}

static gchar *
convert_hexa_byte (gchar c)
{
	static gchar byte[3];

	byte[0] = convert_hexa (c >> 4 & 0x0F);
	byte[1] = convert_hexa (c & 0x0F);
	byte[3] = '\0';
	return byte;
}

static guchar
convert_ascii_print (guchar c)
{
	if ((c > 31) && (c < 127))
		return c;
	else
		return 183;

}


static void
on_sigsegv_emit (gint sig)
{
	longjmp (env, 1);
}


static void
mem (MemApp * memapp, guchar * ptr)
{
	gchar c;
	gint i, j, k;
	gchar *data;
	gchar *adr;
	gchar ascii[20];
	gint seg;
	gint adjust;
	guchar *ptr2;
	GdkColor red = { 16, -1, 0, 0 };
	GdkColor blue = { 16, 0, 0, -1 };

	signal (SIGSEGV, on_sigsegv_emit);
	seg = 0;

	for (i = 0; i < MEM_NB_LINES; i++)
	{
		data = g_strdup (" ");;

		adjust = (glong) ptr % 16;
		if ((i == 0) && (adjust != 0))
		{
			for (k = 0; k < adjust; k++)
			{
				data = g_strconcat (data, "   ", NULL);
				ascii[k] = ' ';
			}
			ptr2 = ptr - adjust;
		}
		else
			ptr2 = ptr;
		ascii[adjust] = ' ';
		for (j = adjust; j < 16; j++)
		{
			if (setjmp (env) == 0)
			{
				c = *ptr++;
				data = g_strconcat (data,
						    convert_hexa_byte (c),
						    NULL);
				data = g_strconcat (data, " ", NULL);
				ascii[j + 1] = convert_ascii_print (c);
			}
			else
			{
				seg = 1;
				break;
			}
		}
		if (i < MEM_NB_LINES - 1)
		{
			data = g_strconcat (data, "\n", NULL);
			ascii[j] = '\n';
			ascii[j + 1] = '\0';
			adr = g_strconcat (" ",
					   convert_adr_hexa ((glong) ptr2),
					   "\n", NULL);
		}
		else
		{
			ascii[j] = '\0';
			adr = g_strconcat (" ",
					   convert_adr_hexa ((glong) ptr2));
		}
		if (seg == 0)
		{
			gtk_text_insert (GTK_TEXT (memapp->text_adr),
					 memapp->fixed_font, &red, NULL, adr,
					 -1); //MAX_ADR + 2);
			gtk_text_insert (GTK_TEXT (memapp->text_data),
					 memapp->fixed_font, NULL, NULL, data,
					 //51);
					 -1);
			gtk_text_insert (GTK_TEXT (memapp->text_ascii),
					 memapp->fixed_font, &blue, NULL,
					 ascii, -1); //20);
			g_free (data);
		}
		else
		{
			if (j > 0)
			{
				gtk_text_insert (GTK_TEXT (memapp->text_data),
						 memapp->fixed_font, NULL,
						 NULL, data, 51);
				gtk_text_insert (GTK_TEXT
						 (memapp->text_ascii),
						 memapp->fixed_font, &blue,
						 NULL, ascii, 20);
			}
			g_free (data);
			break;
		}
	}

	signal (SIGSEGV, SIG_DFL);
}


gboolean
address_is_accessible (guchar * adr)
{
	guchar c;

	signal (SIGSEGV, on_sigsegv_emit);
	if (setjmp (env) == 0)
	{
		c = *adr;
		/*  To detect segmentation fault ???  */
		c = convert_hexa (c);
		signal (SIGSEGV, SIG_DFL);
		return TRUE;
	}
	else
	{
		signal (SIGSEGV, SIG_DFL);
		return FALSE;
	}
}


static void
entry_filter (GtkEditable * editable, const gchar * text, gint length,
	      gint * pos, gpointer user_data)
{
	gint i;

	if (length == 1)
	{
		if (!is_hexa (*text))
		{
			gdk_beep ();
			gtk_signal_emit_stop_by_name (GTK_OBJECT (editable),
						      "insert_text");
		}
		return;
	}

	for (i = 0; i < length; i++)
	{
		if (!is_hexa (text[i]))
		{
			gdk_beep ();
			gtk_signal_emit_stop_by_name (GTK_OBJECT (editable),
						      "insert_text");
			return;
		}
	}
}



static void
mem_clear_gtktext (GtkText * text)
{
	gint length;

	gtk_text_set_point (text, 0);
	length = gtk_text_get_length (text);
	gtk_text_forward_delete (text, length);
}

static void
mem_clear_gtktext_all (MemApp * memapp)
{
	mem_clear_gtktext (GTK_TEXT (memapp->text_adr));
	mem_clear_gtktext (GTK_TEXT (memapp->text_data));
	mem_clear_gtktext (GTK_TEXT (memapp->text_ascii));

}


static gboolean
on_text_data_button_release_event (GtkWidget * widget,
				   GdkEventButton * event, MemApp * memapp)
{
	gint start, end, tmp;
	gint l;
	gchar *buffer;

	gtk_label_set_text (GTK_LABEL (memapp->mem_label), "");
	start = (GTK_EDITABLE (widget)->selection_start_pos);
	end = (GTK_EDITABLE (widget)->selection_end_pos);
	if (start > end)
	{
		tmp = start;
		start = end;
		end = tmp;
	}

	if (end != start)
	{
		if (start != 0)
			start--;
		if (end != MEM_NB_LINES * 50 - 1)	// NB_LIGNES * 50 -1
			end++;
		buffer = gtk_editable_get_chars (GTK_EDITABLE (widget), start,
						 end);
		if (buffer[0] == ' ')
			start++;
		else if (buffer[1] == ' ')
			start += 2;
		l = strlen (buffer);
		if (buffer[l - 1] == ' ')
			end--;

		else if (buffer[l - 2] == ' ')
			end -= 2;
		if (start > end)
			start = end;
		buffer = gtk_editable_get_chars (GTK_EDITABLE (widget), start,
						 end);
		gtk_editable_select_region (GTK_EDITABLE (widget), start,
					    end);
		remove_space (buffer);
		if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
		{
			for (l = 0; l < strlen (buffer); l += 2)
			{
				tmp = buffer[l];
				buffer[l] = buffer[l + 1];
				buffer[l + 1] = tmp;
			}
			g_strreverse (buffer);
		}
		gtk_entry_set_text (GTK_ENTRY (memapp->adr_entry), buffer);
		g_free (buffer);
	}
	else
		gtk_entry_set_text (GTK_ENTRY (memapp->adr_entry), "");

	return FALSE;
}

static void
on_inspect_button_clicked (GtkButton * button, MemApp * memapp)
{
	gchar *buffer;
	GtkWidget *win_mem;

	buffer = g_strstrip (gtk_entry_get_text
			     (GTK_ENTRY (memapp->adr_entry)));
	remove_space (buffer);

/* If correct address  */
	if (address_is_accessible (adr_to_decimal (buffer)))
	{
		if (memapp->adr != NULL)
		{
			win_mem =
				create_info_memory ((guchar *)
						    adr_to_decimal (buffer));
			gtk_widget_show (win_mem);
		}
		else
		{
			memapp->adr = (guchar *) adr_to_decimal (buffer);
			mem (memapp, memapp->adr);
		}
	}
	else
		gtk_label_set_text (GTK_LABEL (memapp->mem_label),
				    _("Non accessible address !"));
}


static void
on_quit_button_clicked (GtkButton * button, MemApp * memapp)
{
	gtk_widget_destroy (memapp->window);
}


static void
mem_move_up (MemApp * memapp)
{
	gtk_label_set_text (GTK_LABEL (memapp->mem_label), "");
	if (address_is_accessible (memapp->adr - 16))
	{
		memapp->adr -= 16;
		mem_clear_gtktext_all (memapp);
		mem (memapp, memapp->adr);
	}
	else
		gtk_label_set_text (GTK_LABEL (memapp->mem_label),
				    _("Non accessible address !"));
}

static void
mem_move_page_up (MemApp * memapp)
{
	gtk_label_set_text (GTK_LABEL (memapp->mem_label), "");
	if (address_is_accessible (memapp->adr - (16 * MEM_NB_LINES / 2)))
	{
		memapp->adr -= 16 * MEM_NB_LINES / 2;
		mem_clear_gtktext_all (memapp);
		mem (memapp, memapp->adr);
	}
	else
		gtk_label_set_text (GTK_LABEL (memapp->mem_label),
				    _("Non accessible address !"));
}

static gboolean
on_eventbox1_button_press_event (GtkWidget * widget,
				 GdkEventButton * event, MemApp * memapp)
{
	mem_move_up (memapp);
	return FALSE;
}

static void
mem_move_down (MemApp * memapp)
{
	gtk_label_set_text (GTK_LABEL (memapp->mem_label), "");
	if (address_is_accessible (memapp->adr + 16))
	{
		memapp->adr += 16;
		mem_clear_gtktext_all (memapp);
		mem (memapp, memapp->adr);
	}
	else
		gtk_label_set_text (GTK_LABEL (memapp->mem_label),
				    _("Non accessible address !"));
}

static void
mem_move_page_down (MemApp * memapp)
{
	gtk_label_set_text (GTK_LABEL (memapp->mem_label), "");
	if (address_is_accessible (memapp->adr + (16 * MEM_NB_LINES / 2)))
	{
		memapp->adr += 16 * MEM_NB_LINES / 2;
		mem_clear_gtktext_all (memapp);
		mem (memapp, memapp->adr);
	}
	else
		gtk_label_set_text (GTK_LABEL (memapp->mem_label),
				    _("Non accessible address !"));
}

static gboolean
on_eventbox2_button_press_event (GtkWidget * widget,
				 GdkEventButton * event, MemApp * memapp)
{
	mem_move_down (memapp);
	return FALSE;
}

static gboolean
on_text1_key_press_event (GtkWidget * widget,
			  GdkEventKey * event, MemApp * memapp)
{
	switch (event->keyval & 0xff)
	{
	case 82:
		mem_move_up (memapp);
		break;
	case 84:
		mem_move_down (memapp);
		break;
	case 85:
		mem_move_page_up (memapp);
		break;
	case 86:
		mem_move_page_down (memapp);
		break;
	}
	return FALSE;
}

static void
on_window_destroy (GtkObject * object, MemApp * memapp)
{
	g_free (memapp);
	memapp = NULL;
}
