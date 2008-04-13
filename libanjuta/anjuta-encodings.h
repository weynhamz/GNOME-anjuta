/*
 * anjuta-encodings.h
 * This file is part of anjuta
 *
 * Copyright (C) 2002-2005 Paolo Maggi 
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
 * Modified by the anjuta Team, 2002-2005. See the AUTHORS file for a 
 * list of people on the anjuta Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id$
 */

#ifndef __ANJUTA_ENCODINGS_H__
#define __ANJUTA_ENCODINGS_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _AnjutaEncoding AnjutaEncoding;

#define ANJUTA_TYPE_ENCODING     (anjuta_encoding_get_type ())

GType              	 anjuta_encoding_get_type (void) G_GNUC_CONST;

AnjutaEncoding		*anjuta_encoding_copy		 (const AnjutaEncoding *enc);
void               	 anjuta_encoding_free		 (AnjutaEncoding       *enc);

const AnjutaEncoding	*anjuta_encoding_get_from_charset (const gchar         *charset);
const AnjutaEncoding	*anjuta_encoding_get_from_index	 (gint                 index);

gchar 			*anjuta_encoding_to_string	 (const AnjutaEncoding *enc);

const gchar		*anjuta_encoding_get_name	 (const AnjutaEncoding *enc);
const gchar		*anjuta_encoding_get_charset	 (const AnjutaEncoding *enc);

const AnjutaEncoding 	*anjuta_encoding_get_utf8	 (void);
const AnjutaEncoding 	*anjuta_encoding_get_current	 (void);

G_END_DECLS

#endif  /* __ANJUTA_ENCODINGS_H__ */

