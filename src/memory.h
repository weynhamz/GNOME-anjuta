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

typedef struct _MemApp MemApp;

struct _MemApp
{
	GtkWidget *window;
  GtkWidget *text_adr;
	GtkWidget *text_data;
	GtkWidget *text_ascii;
	GtkWidget *adr_entry;
	GtkWidget *mem_label;
	GdkFont *fixed_font;
	guchar *adr;
  guchar *start_adr;
  gboolean new_window;
};


GtkWidget* create_info_memory (guchar *ptr);

gboolean address_is_accessible(guchar *adr, MemApp *memapp);

guchar *adr_to_decimal (gchar *hexa);

#ifdef __cplusplus
}
#endif

#endif /* _MEMORY_H */



