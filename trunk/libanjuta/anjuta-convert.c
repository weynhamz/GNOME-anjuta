/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-convert.c
 * This file is part of gedit
 *
 * Copyright (C) 2003 - Paolo Maggi 
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
 * Modified by the gedit Team, 2003. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#include <string.h>
#include <stdio.h>

#include <glib/gi18n.h>

#include "anjuta-convert.h"

GQuark 
anjuta_convert_error_quark (void)
{
	static GQuark quark;
	if (!quark)
		quark = g_quark_from_static_string ("anjuta_convert_error");

	return quark;
}

static gchar *
anjuta_convert_to_utf8_from_charset (const gchar  *content,
				    gsize         len,
				    const gchar  *charset,
				    gsize        *new_len,
				    GError 	**error)
{
	gchar *utf8_content = NULL;
	GError *conv_error = NULL;
	gchar* converted_contents = NULL;
	gsize bytes_read;
	
	g_return_val_if_fail (content != NULL, NULL);
	g_return_val_if_fail (len > 0, NULL);
	g_return_val_if_fail (charset != NULL, NULL);
	
	if (strcmp (charset, "UTF-8") == 0)
	{
		if (g_utf8_validate (content, len, NULL))
		{
			if (new_len != NULL)
				*new_len = len;
					
			return g_strndup (content, len);
		}
		else	
			g_set_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
				     "The file you are trying to open contains an invalid byte sequence.");
		
			return NULL;	
	}
	
	converted_contents = g_convert (content, 
					len, 
					"UTF-8",
					charset, 
					&bytes_read, 
					new_len,
					&conv_error); 
		
	/* There is no way we can avoid to run 	g_utf8_validate on the converted text.
	 *
	 * <paolo> hmmm... but in that case g_convert should fail 
	 * <owen> paolo: g_convert() doesn't necessarily have the same definition
         * <owen> GLib just uses the system's iconv
         * <owen> paolo: I think we've explained what's going on. 
         * I have to define it as NOTABUG since g_convert() isn't going to 
         * start post-processing or checking what iconv() does and 
         * changing g_utf8_valdidate() wouldn't be API compatible even if I 
         * thought it was right
	 */			
	if ((conv_error != NULL) || 
	    !g_utf8_validate (converted_contents, *new_len, NULL) ||
	    (bytes_read != len))
	{
				
		if (converted_contents != NULL)
			g_free (converted_contents);

		if (conv_error != NULL)
			g_propagate_error (error, conv_error);		
		else
		{	
			g_set_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
				     "The file you are trying to open contains an invalid byte sequence.");
		}	
	} 
	else 
	{
		g_return_val_if_fail (converted_contents != NULL, NULL); 

		utf8_content = converted_contents;
	}

	return utf8_content;
}

gchar *
anjuta_convert_to_utf8 (const gchar          *content,
		       gsize                 len,
		       const AnjutaEncoding **encoding,
		       gsize                *new_len,
		       GError              **error)
{
	g_return_val_if_fail (content != NULL, NULL);
	g_return_val_if_fail (encoding != NULL, NULL);

	if (len < 0)
		len = strlen (content);

	if (*encoding != NULL)
	{
		const gchar* charset;
		
		charset = anjuta_encoding_get_charset (*encoding);

		g_return_val_if_fail (charset != NULL, NULL);

		return anjuta_convert_to_utf8_from_charset (content, 
							   len,
							   charset,
							   new_len,
							   error);
	}
	else
	{
		/* Automatically detect the encoding used */
	GSList *encodings;
	GSList *start;
	gchar *ret = NULL;
	if (g_utf8_validate (content, len, NULL))
		{
			if (new_len != NULL)
				*new_len = len;
			
			return g_strndup (content, len);
		}
		else
		{
			g_set_error (error, ANJUTA_CONVERT_ERROR, 
				     ANJUTA_CONVERT_ERROR_AUTO_DETECTION_FAILED,
			 	     "anjuta was not able to automatically determine "
				     "the encoding of the file you want to open.");
			return NULL;
		}

		start = encodings;

		while (encodings != NULL) 
		{
			const AnjutaEncoding *enc;
			const gchar *charset;
			gchar *utf8_content;

			enc = (const AnjutaEncoding *)encodings->data;

			charset = anjuta_encoding_get_charset (enc);
			g_return_val_if_fail (charset != NULL, NULL);

			utf8_content = anjuta_convert_to_utf8_from_charset (content, 
									   len, 
									   charset, 
									   new_len,
									   NULL);

			if (utf8_content != NULL) 
			{
				*encoding = enc;
				ret = utf8_content;

				break;
			}

			encodings = g_slist_next (encodings);
		}

		if (ret == NULL)
		{
			g_set_error (error, ANJUTA_CONVERT_ERROR,
				     ANJUTA_CONVERT_ERROR_AUTO_DETECTION_FAILED,
			 	     "anjuta was not able to automatically determine "
				     "the encoding of the file you want to open.");
		}

		g_slist_free (start);

		return ret;
	}

	g_return_val_if_reached (NULL);
}

gchar *
anjuta_convert_from_utf8 (const gchar          *content, 
		         gsize                 len,
		         const AnjutaEncoding  *encoding,
			 gsize                *new_len,
			 GError 	     **error)
{
	GError *conv_error         = NULL;
	gchar  *converted_contents = NULL;
	gsize   bytes_written = 0;
	
	g_return_val_if_fail (content != NULL, NULL);
	g_return_val_if_fail (g_utf8_validate (content, len, NULL), NULL);
	g_return_val_if_fail (encoding != NULL, NULL);

	if (len < 0)
		len = strlen (content);

	if (encoding == anjuta_encoding_get_utf8 ())
		return g_strndup (content, len);

	converted_contents = g_convert (content, 
					len, 
					anjuta_encoding_get_charset (encoding), 
					"UTF-8",
					NULL, 
					&bytes_written,
					&conv_error); 

	if (conv_error != NULL)
	{
		if (converted_contents != NULL)
		{
			g_free (converted_contents);
			converted_contents = NULL;
		}

		g_propagate_error (error, conv_error);
	}
	else
	{
		if (new_len != NULL)
			*new_len = bytes_written;
	}

	return converted_contents;
}

