/***************************************************************************
 *            memory.h
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

#ifndef _MEMORY_H
#define _MEMORY_H

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _MemApp
{
	GladeXML *xml;
	GtkWidget *dialog;
	GtkWidget *adr_entry;
	GtkWidget *button_inspect;
	GtkWidget *button_quit;
	GtkWidget *memory_label;
	GtkWidget *adr_textview;
	GtkWidget *data_textview;
	GtkWidget *ascii_textview;
	GtkTextBuffer *adr_buffer;
	GtkTextBuffer *data_buffer;
	GtkTextBuffer *ascii_buffer;
	GtkWidget *eventbox_up;
	GtkWidget *eventbox_down;
	gchar *adr;
	gchar *start_adr;
	gboolean new_window;
} MemApp;

GtkWidget* memory_info_new (guchar *ptr);
gchar *memory_info_address_to_decimal (gchar *hex_address);

#ifdef __cplusplus
}
#endif

#endif /* _MEMORY_H */
