/***************************************************************************
 *            memory.c
 *
 *  Sat May 10 07:45:38 2003
 *  Copyright  2003  Jean-Noel Guiheneuf
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
#include "config.h"
#endif


#include <gnome.h>
#include <glade/glade.h>

/* TODO #include "anjuta.h" */
#include "debugger.h"
#include "memory.h"


#define ADR_ENTRY                "adr_entry"
#define BUTTON_INSPECT           "button_inspect"
#define BUTTON_QUIT              "button_quit"
#define MEMORY_LABEL             "memory_label"
#define ADR_TEXTVIEW             "adr_textview"
#define DATA_TEXTVIEW            "data_textview"
#define ASCII_TEXTVIEW           "ascii_textview"
#define EVENTBOX_UP              "eventbox_up"
#define EVENTBOX_DOWN            "eventbox_down"

#define MEM_NB_LINES             16
#define MAX_CAR_ADR_ENTRY        8

void init_memory (MemApp *memapp);

void init_name_memory (MemApp *memapp);

void init_widget_memory (MemApp *memapp);

void init_event_memory (MemApp *memapp);

void on_adr_entry_insert_text (GtkEditable *editable, const gchar *text,
                              gint length, gint *pos, gpointer user_data);

static gboolean inspect_memory (gchar *adr, MemApp * memapp);

static gboolean on_text_data_button_release_event (GtkWidget *widget,
												   GdkEventButton *event,
												   MemApp *memapp);

static void on_button_inspect_clicked (GtkButton *button, MemApp *memapp);	

static gboolean on_text1_key_press_event (GtkWidget *widget, GdkEventKey *event,
										  MemApp *memapp);

static gboolean on_eventbox_up_button_press_event (GtkWidget *widget,
												   GdkEventButton *event,
												   MemApp *memapp);

static gboolean on_eventbox_down_button_press_event (GtkWidget *widget,
													 GdkEventButton *event,
													 MemApp *memapp);

static void on_dialog_memory_destroy (GtkObject *object, MemApp *memapp);

static void on_button_quit_clicked (GtkButton *button, MemApp *memapp);


static char *glade_names[] =
{
	ADR_ENTRY, BUTTON_INSPECT, BUTTON_QUIT, 
	MEMORY_LABEL, ADR_TEXTVIEW, DATA_TEXTVIEW,
	ASCII_TEXTVIEW, EVENTBOX_UP, EVENTBOX_DOWN, NULL
};

#define MEMORY_DIALOG "dialog_memory"
#define GLADE_FILE "glade/anjuta.glade"

gboolean timeout = TRUE;

GtkWidget*
memory_info_new (guchar *ptr)
{
	int i;
	MemApp *memapp;
	
	memapp = g_new0 (MemApp, 1);
	memapp->adr = ptr;
	
/* TODO 	if (NULL == (memapp->xml = glade_xml_new (GLADE_FILE_ANJUTA, MEMORY_DIALOG, NULL))) */
	{
/* TODO
		anjuta_error (_("Unable to build user interface for Memory\n"));
*/
		g_free (memapp);
		memapp = NULL;
		return NULL;
	}

	memapp->dialog = glade_xml_get_widget (memapp->xml, MEMORY_DIALOG);
	
	for (i=0; NULL != glade_names[i]; ++i)
		gtk_widget_ref (glade_xml_get_widget (memapp->xml, glade_names[i]));

	init_name_memory (memapp);
	init_widget_memory (memapp);
	init_event_memory (memapp);
	init_memory (memapp);
	
	memapp->new_window = FALSE;
	if (ptr)
		inspect_memory (ptr, memapp);
	
	gtk_widget_grab_focus (memapp->adr_entry);
	gtk_widget_grab_default (memapp->button_inspect);
	
	glade_xml_signal_autoconnect (memapp->xml);

	return memapp->dialog;
}

void
init_memory (MemApp *memapp)
{
	gchar *address = "";
	gchar *data = "";
	gchar *ascii = "";
	gint line, i;
	
	for (line = 0 ; line < MEM_NB_LINES; line++)
	{
		for (i=0; i<MAX_CAR_ADR_ENTRY; i++)
			address = g_strconcat (address, "0", NULL);
		address = g_strconcat (address, "\n", NULL);
		for (i = 0; i < 16; i++)
		{
			data = g_strconcat (data, "00 ", NULL);
			ascii = g_strconcat (ascii, ".", NULL);	
		}
		data = g_strconcat (data, "\n", NULL);
		ascii = g_strconcat (ascii, "\n", NULL);
	}
	gtk_text_buffer_set_text (memapp->adr_buffer, address, -1);
	gtk_text_buffer_set_text (memapp->data_buffer, data, -1);
	gtk_text_buffer_set_text (memapp->ascii_buffer, ascii, -1);
	
	g_free (data);
	g_free (address);
	g_free (ascii);
}

void
init_name_memory (MemApp *memapp)
{
	memapp->adr_entry = glade_xml_get_widget (memapp->xml, ADR_ENTRY);
	memapp->button_inspect = glade_xml_get_widget (memapp->xml, BUTTON_INSPECT);
	memapp->button_quit = glade_xml_get_widget (memapp->xml, BUTTON_QUIT);
	memapp->memory_label = glade_xml_get_widget (memapp->xml, MEMORY_LABEL);
	memapp->adr_textview = glade_xml_get_widget (memapp->xml, ADR_TEXTVIEW);
	memapp->data_textview = glade_xml_get_widget (memapp->xml, DATA_TEXTVIEW);
	memapp->ascii_textview = glade_xml_get_widget (memapp->xml, ASCII_TEXTVIEW);
	memapp->eventbox_up = glade_xml_get_widget (memapp->xml, EVENTBOX_UP);
	memapp->eventbox_down = glade_xml_get_widget (memapp->xml, EVENTBOX_DOWN);
}

void
init_widget_memory (MemApp *memapp)
{
	PangoFontDescription *font_desc;
	GdkColor red = {16, -1, 0, 0};
	GdkColor blue = {16, 0, 0, -1};
	
	font_desc = pango_font_description_from_string ("Fixed 11");
	
	gtk_widget_modify_font (memapp->adr_entry, font_desc);
	gtk_entry_set_width_chars (GTK_ENTRY (memapp->adr_entry),
							   MAX_CAR_ADR_ENTRY);
	
	gtk_widget_modify_font (memapp->adr_textview, font_desc);
	gtk_widget_modify_text (memapp->adr_textview, GTK_STATE_NORMAL, &red);
	gtk_widget_modify_font (memapp->data_textview, font_desc);
	gtk_widget_modify_font (memapp->ascii_textview, font_desc);
	gtk_widget_modify_text (memapp->ascii_textview, GTK_STATE_NORMAL, &blue);
	
	pango_font_description_free (font_desc);
	
	memapp->adr_buffer =
		gtk_text_view_get_buffer (GTK_TEXT_VIEW (memapp->adr_textview));
	memapp->data_buffer =
		gtk_text_view_get_buffer (GTK_TEXT_VIEW (memapp->data_textview));
	memapp->ascii_buffer =
		gtk_text_view_get_buffer (GTK_TEXT_VIEW (memapp->ascii_textview));
	
	gtk_text_buffer_create_tag (memapp->data_buffer, "data_select", 
								"foreground", "white",
								"background", "blue", NULL);
}

void
init_event_memory (MemApp *memapp)
{
	gtk_signal_connect (GTK_OBJECT (memapp->button_inspect), "clicked",
						GTK_SIGNAL_FUNC (on_button_inspect_clicked), memapp);
	gtk_signal_connect (GTK_OBJECT (memapp->button_quit), "clicked",
						GTK_SIGNAL_FUNC (on_button_quit_clicked), memapp);
	gtk_signal_connect (GTK_OBJECT (memapp->adr_entry), "insert_text",
						GTK_SIGNAL_FUNC (on_adr_entry_insert_text), NULL);
	gtk_signal_connect (GTK_OBJECT (memapp->dialog), "destroy",
						GTK_SIGNAL_FUNC (on_dialog_memory_destroy), memapp);
	gtk_signal_connect (GTK_OBJECT (memapp->dialog),
						"key_press_event",
						GTK_SIGNAL_FUNC (on_text1_key_press_event), memapp);
	gtk_signal_connect (GTK_OBJECT (memapp->data_textview),
						"button_release_event",
						GTK_SIGNAL_FUNC (on_text_data_button_release_event),
						memapp);
	gtk_signal_connect (GTK_OBJECT (memapp->eventbox_up), "button_press_event",
						GTK_SIGNAL_FUNC (on_eventbox_up_button_press_event),
						memapp);
	gtk_signal_connect (GTK_OBJECT (memapp->eventbox_down),
						"button_press_event",
						GTK_SIGNAL_FUNC (on_eventbox_down_button_press_event),
						memapp);
}


void
on_adr_entry_insert_text (GtkEditable *editable, const gchar *text,
						  gint length, gint *pos, gpointer user_data)
{
	gint i;

 	if (length == 1)
 	{
		if (!g_ascii_isxdigit (*text))
		{
			gdk_beep ();
			gtk_signal_emit_stop_by_name (GTK_OBJECT (editable), "insert_text");
		}
		return;
	}

	for (i = 0; i < length; i++)
	{
		if (!g_ascii_isxdigit (text[i]))
		{
			gdk_beep ();
			gtk_signal_emit_stop_by_name (GTK_OBJECT (editable), "insert_text");
			return;
		}
	}
}

gchar*
memory_info_address_to_decimal (gchar * hexa)
{
	gchar *ptr;
	gulong dec;
	
	ptr = hexa;
	dec = 0;
	while (*ptr)
	{
		dec = dec * 16 + g_ascii_xdigit_value (*ptr++);
	}
	return (guchar *) dec;
}



static gchar
convert_hexa (gchar c)
{
	if (c < 10)
		return c + '0';
	else
		return c + 'A' - 10;
}

static gchar*
convert_adr_hexa (gulong adr)
{
	static gchar buffer[9];
	static gulong r;
	guchar c;
	gint i = 0;
	
	while (adr > 15 && i < MAX_CAR_ADR_ENTRY)
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

static gchar*
convert_hexa_byte (gchar c)
{
	static gchar byte[3];
	
	byte[0] = convert_hexa (c >> 4 & 0x0F);
	byte[1] = convert_hexa (c & 0x0F);
	byte[3] = '\0';
	return byte;
}

static gchar*
convert_ascii_print (gchar c)
{
	if (g_ascii_isprint (c))
		return (g_strdup_printf("%c", c));
	else
		return ".";
}

extern void
debugger_memory_cbs (GList* list, MemApp *memapp)
{
	gchar *address = "";
	gchar *data = "";
	gchar *ascii = "";
	gulong adr;
	gchar *ptr;
	gint line, i;
	gint car;
	GtkWidget *win_mem;

	if (memapp->new_window)
	{
		win_mem = memory_info_new (memapp->adr);
		gtk_widget_show (win_mem);
	}
	else
	{
		memapp->new_window = TRUE;
		memapp->start_adr = memapp->adr;
		adr = (gulong) memapp->adr & -16;
		line = (gulong) memapp->adr & 0xF;

		memapp->adr = (gchar*)adr;

		for (i = 0; i < line; i++)
		{
			data = g_strconcat (data, "   ", NULL);
			ascii = g_strconcat (ascii, " ", NULL);
		}

		while (list)
		{
			ptr = (gchar*) list->data;
			while (*ptr != ':')
				ptr++;
			ptr++;
			while (*ptr)
			{
				car = atoi(++ptr);
				data = g_strconcat (data, convert_hexa_byte (car), " ", NULL);
				ascii = g_strconcat (ascii, convert_ascii_print (car), NULL);
				while(*ptr && *ptr != '\t')
					ptr++;
				line++;
				if (line == 16)
				{
					address =  g_strconcat (address, convert_adr_hexa (adr),
											"\n", NULL);
					data = g_strconcat (data, "\n", NULL);	
					ascii = g_strconcat (ascii, "\n", NULL);
					line = 0;
					adr += 16;
				}
			}
			list = g_list_next (list);
		}
		gtk_text_buffer_set_text (memapp->adr_buffer, address, -1);
		gtk_text_buffer_set_text (memapp->data_buffer, data, -1);
		gtk_text_buffer_set_text (memapp->ascii_buffer, ascii, -1);

		g_free (data);
		g_free (address);
		g_free (ascii);
	}
}

static gboolean
memory_timeout (MemApp * memapp)
{	
	if (debugger_is_ready())
		/*  Accessible address  */
		gtk_label_set_text (GTK_LABEL (memapp->memory_label), 
						_("Enter a Hexa address or Select one in the data"));
  else
	{
		/*  Non Accessible address  */
		memapp->adr = memapp->start_adr;
		debugger_set_ready (TRUE);
		gtk_label_set_text (GTK_LABEL (memapp->memory_label),
							_("Non accessible address !"));
	}
	timeout = TRUE;
	
	return FALSE;
}

static gboolean
inspect_memory (gchar *adr, MemApp * memapp)
{
	gchar *cmd;
	gchar *address;
	gchar *nb_car;
	
	address =g_strdup_printf ("%ld", (gulong) adr);
	nb_car = g_strdup_printf ("%d", (gint) (MEM_NB_LINES * 16 - ((gulong)adr & 0xF)) );
	cmd = g_strconcat ("x", "/", nb_car, "bd ", address, " ", NULL);
	memapp->adr = adr;
	
	debugger_put_cmd_in_queqe (cmd, 0, (void*) debugger_memory_cbs, memapp);
	debugger_execute_cmd_in_queqe();
	
	g_free (cmd);
	g_free (address);
	g_free (nb_car);
	
	/* No answer from gdb after 500ms ==> memory non accessible */
	g_timeout_add (500, (void*) memory_timeout, memapp);
	
	return FALSE;
}

static void
remove_space_nl (gchar * string)
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
dummy (GtkWidget *widget, GdkEventButton *event, MemApp *memapp)
{
	return FALSE;
}

static void
select_new_data (MemApp *memapp, GtkTextIter *start, GtkTextIter *end)
{
	GtkTextIter buffer_start, buffer_end;
	
	gtk_text_buffer_get_bounds (memapp->data_buffer, &buffer_start, &buffer_end);
	gtk_text_buffer_remove_tag_by_name (memapp->data_buffer, "data_select",
										&buffer_start, &buffer_end);

	gtk_text_buffer_apply_tag_by_name (memapp->data_buffer, "data_select",
									   start, end);
	gtk_signal_emit_by_name (GTK_OBJECT (memapp->data_textview),
							 "button_press_event",
							 GTK_SIGNAL_FUNC (dummy), memapp);
}

static gboolean
on_text_data_button_release_event (GtkWidget *widget,
								   GdkEventButton *event,
								   MemApp *memapp)
{
	GtkTextIter start;
	GtkTextIter end;
	gchar *select;
	gint offset;
	gint len, nb_lines, nb_digits;
	gint tmp, l;
	
	if (gtk_text_buffer_get_selection_bounds (memapp->data_buffer,
											  &start, &end))
	{
		offset = gtk_text_iter_get_line_offset(&start) % 3;
		if (offset  == 1)
			gtk_text_iter_backward_char(&start);
		else if (offset == 2)
			gtk_text_iter_forward_char(&start);
		
		offset = gtk_text_iter_get_line_offset(&end) % 3;
		if (offset  == 0)
			gtk_text_iter_backward_char(&end);
		else if (offset == 1)
			gtk_text_iter_forward_char(&end);
		
		len = gtk_text_iter_get_offset(&end) - gtk_text_iter_get_offset (&start);
		nb_lines = gtk_text_iter_get_line (&end) - gtk_text_iter_get_line (&start);
		nb_digits = (len + 1 - nb_lines) / 3;
		if (nb_digits > (MAX_CAR_ADR_ENTRY / 2))
		{
			gint x = (gtk_text_iter_get_line_offset(&start) > 36) ? 1 : 0;
			gtk_text_iter_backward_chars (&end,
					(nb_digits - (MAX_CAR_ADR_ENTRY / 2)) * 3 + nb_lines -x);
		}
		
		select = gtk_text_buffer_get_text (memapp->data_buffer, &start,
										   &end, TRUE);
		remove_space_nl (select);
		
		if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
		{
			for (l = 0; l < strlen (select); l += 2)
			{
				tmp = select[l];
				select[l] = select[l + 1];
				select[l + 1] = tmp;
			}
			g_strreverse (select);
		}
		
		select_new_data (memapp, &start, &end);
	
		gtk_entry_set_text (GTK_ENTRY (memapp->adr_entry), select);
		gtk_widget_grab_focus (memapp->adr_entry);
		
		g_free (select);
	}

	return FALSE;
}

static void
memory_inspect (MemApp *memapp)
{
	gchar *string;

	string = g_strstrip (g_strdup (gtk_entry_get_text (GTK_ENTRY (memapp->adr_entry))));
	inspect_memory (memory_info_address_to_decimal (string), memapp );
	g_free (string);
}

static void
on_button_inspect_clicked (GtkButton *button, MemApp *memapp)
{
	memory_inspect (memapp);
}


static void
mem_move_up (MemApp *memapp)
{
	if (timeout)
	{
		memapp->adr -= 16;
		memapp->new_window = FALSE;
		inspect_memory (memapp->adr, memapp );
		timeout = FALSE;
	}
}

static void
mem_move_page_up (MemApp *memapp)
{
	if (timeout)
	{
		memapp->adr -= 16 * MEM_NB_LINES / 2;
		memapp->new_window = FALSE;
		inspect_memory (memapp->adr, memapp );
		timeout = FALSE;
	}
}

static void
mem_move_down (MemApp *memapp)
{
	if (timeout)
	{
		memapp->adr += 16;
		memapp->new_window = FALSE;
		inspect_memory (memapp->adr, memapp );
		timeout = FALSE;
	}
}

static void
mem_move_page_down (MemApp *memapp)
{
	if (timeout)
	{
		memapp->adr += 16 * MEM_NB_LINES / 2;
		memapp->new_window = FALSE;
		inspect_memory (memapp->adr, memapp );
		timeout = FALSE;
	}
}

static gboolean
on_text1_key_press_event (GtkWidget *widget, GdkEventKey *event,
						  MemApp *memapp)
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
	case 13:
		memory_inspect (memapp);
		break;
	}
	return FALSE;
}

static gboolean
on_eventbox_up_button_press_event (GtkWidget *widget,
								   GdkEventButton *event,
								   MemApp *memapp)
{
	mem_move_up (memapp);
	return FALSE;
}

static gboolean
on_eventbox_down_button_press_event (GtkWidget *widget,
									 GdkEventButton *event,
									 MemApp *memapp)
{
	mem_move_down (memapp);
	return FALSE;
}

static void
on_dialog_memory_destroy (GtkObject *object, MemApp *memapp)
{
	g_free (memapp);
	memapp = NULL;
}

static void
on_button_quit_clicked (GtkButton *button, MemApp *memapp)
{
	gtk_widget_destroy (memapp->dialog);
}
