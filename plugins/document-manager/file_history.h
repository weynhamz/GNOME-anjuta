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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
  

#ifndef _FILE_HISTORY_H
#define _FILE_HISTORY_H

#include <glib.h>

#include "anjuta-docman.h"

typedef struct _AnHistFile AnHistFile;

struct _AnHistFile
{
	gchar *file;
	glong line;
};

AnHistFile *an_hist_file_new(const char *name, glong line);
void an_hist_file_free(AnHistFile *h_file);

void an_file_history_reset(void);
void an_file_history_push(const char *filename, glong line);
void an_file_history_back(AnjutaDocman *docman);
void an_file_history_forward(AnjutaDocman *docman);
void an_file_history_dump(void);
void an_file_history_free(void);

#endif /* _FILE_HISTORY_H */
