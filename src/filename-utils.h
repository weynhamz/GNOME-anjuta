/***************************************************************************
 *            filename-utils.h
 *	Contains functions useful for working with filenames and paths
 *
 *  Tue Mar  1 14:34:06 2005
 *  Copyright  2005  James Liggett
 *  Email jrliggett@cox.net
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
 
#ifndef _FILENAME_UTILS_H
#define _FILENAME_UTILS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <glib.h> /* for gchar */


/* gchar* quote_filename: Takes a given filename and puts it in quotes.
   Intended to be used for calling commands like gdb. Free the returned
   string when you are finished with it.   */
   
gchar* quote_filename(gchar* filename);

#ifdef __cplusplus
}
#endif
#endif /* _FILENAME_UTILS_H */
