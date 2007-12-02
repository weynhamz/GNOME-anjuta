/*
 * anjuta-encodings.c
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <glib/gi18n.h>

#include "anjuta-encodings.h"


struct _AnjutaEncoding
{
	gint   index;
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
  ANJUTA_ENCODING_EUC_JP_MS,
  ANJUTA_ENCODING_CP932,
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
  ANJUTA_ENCODING_KOI8__R,
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

  ANJUTA_ENCODING_LAST,

  ANJUTA_ENCODING_UTF_8,
  ANJUTA_ENCODING_UNKNOWN
  
} AnjutaEncodingIndex;

static AnjutaEncoding utf8_encoding = 
	{ ANJUTA_ENCODING_UTF_8,
	  "UTF-8", 
	  N_("Unicode") 
	};

/* initialized in anjuta_encoding_lazy_init() */
static AnjutaEncoding unknown_encoding = 
	{ ANJUTA_ENCODING_UNKNOWN,
	  NULL, 
	  NULL 
	};

	
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
  { ANJUTA_ENCODING_EUC_JP_MS,
    "EUC-JP-MS", N_("Japanese") },
  { ANJUTA_ENCODING_CP932,
    "CP932", N_("Japanese") },

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
  { ANJUTA_ENCODING_KOI8__R,
    "KOI8-R", N_("Cyrillic") },
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

static void
anjuta_encoding_lazy_init (void)
{
	static gboolean initialized = FALSE;
	gint i;
	const gchar *locale_charset;

	if (initialized)
		return;

	i = 0;
	while (i < ANJUTA_ENCODING_LAST)
	{
		g_return_if_fail (encodings[i].index == i);

		/* Translate the names */
		encodings[i].name = _(encodings[i].name);

		++i;
    	}

	utf8_encoding.name = _(utf8_encoding.name);

	if (g_get_charset (&locale_charset) == FALSE)
	{
		unknown_encoding.charset = g_strdup (locale_charset);
	}

	initialized = TRUE;
}

const AnjutaEncoding *
anjuta_encoding_get_from_charset (const gchar *charset)
{
	gint i;

	g_return_val_if_fail (charset != NULL, NULL);

	anjuta_encoding_lazy_init ();

	if (charset == NULL)
		return NULL;

	if (g_ascii_strcasecmp (charset, "UTF-8") == 0)
		return anjuta_encoding_get_utf8 ();

	i = 0; 
	while (i < ANJUTA_ENCODING_LAST)
	{
		if (g_ascii_strcasecmp (charset, encodings[i].charset) == 0)
			return &encodings[i];
      
		++i;
	}

	if (unknown_encoding.charset != NULL)
	{
		if (g_ascii_strcasecmp (charset, unknown_encoding.charset) == 0)
			return &unknown_encoding;
	}

	return NULL;
}

const AnjutaEncoding *
anjuta_encoding_get_from_index (gint index)
{
	g_return_val_if_fail (index >= 0, NULL);

	if (index >= ANJUTA_ENCODING_LAST)
		return NULL;

	anjuta_encoding_lazy_init ();

	return &encodings [index];
}

const AnjutaEncoding *
anjuta_encoding_get_utf8 (void)
{
	anjuta_encoding_lazy_init ();

	return &utf8_encoding;
}

const AnjutaEncoding *
anjuta_encoding_get_current (void)
{
	static gboolean initialized = FALSE;
	static const AnjutaEncoding *locale_encoding = NULL;

	const gchar *locale_charset;

	anjuta_encoding_lazy_init ();

	if (initialized != FALSE)
		return locale_encoding;

	if (g_get_charset (&locale_charset) == FALSE) 
	{
		g_return_val_if_fail (locale_charset != NULL, &utf8_encoding);
		
		locale_encoding = anjuta_encoding_get_from_charset (locale_charset);
	}
	else
	{
		locale_encoding = &utf8_encoding;
	}
	
	if (locale_encoding == NULL)
	{
		locale_encoding = &unknown_encoding;
	}

	g_return_val_if_fail (locale_encoding != NULL, NULL);

	initialized = TRUE;

	return locale_encoding;
}

gchar *
anjuta_encoding_to_string (const AnjutaEncoding* enc)
{
	g_return_val_if_fail (enc != NULL, NULL);
	
	anjuta_encoding_lazy_init ();

	g_return_val_if_fail (enc->charset != NULL, NULL);

	if (enc->name != NULL)
	    	return g_strdup_printf ("%s (%s)", enc->name, enc->charset);
	else
	{
		if (g_ascii_strcasecmp (enc->charset, "ANSI_X3.4-1968") == 0)
			return g_strdup_printf ("US-ASCII (%s)", enc->charset);
		else
			return g_strdup (enc->charset);
	}
}

const gchar *
anjuta_encoding_get_charset (const AnjutaEncoding* enc)
{
	g_return_val_if_fail (enc != NULL, NULL);

	anjuta_encoding_lazy_init ();

	g_return_val_if_fail (enc->charset != NULL, NULL);

	return enc->charset;
}

const gchar *
anjuta_encoding_get_name (const AnjutaEncoding* enc)
{
	g_return_val_if_fail (enc != NULL, NULL);

	anjuta_encoding_lazy_init ();

	return (enc->name == NULL) ? _("Unknown") : enc->name;
}

/**
 * anjuta_encoding_get_type:
 * 
 * Retrieves the GType object which is associated with the
 * #AnjutaEncoding class.
 * 
 * Return value: the GType associated with #AnjutaEncoding.
 **/
GType 
anjuta_encoding_get_type (void)
{
	static GType our_type = 0;

	if (!our_type)
		our_type = g_boxed_type_register_static (
			"AnjutaEncoding",
			(GBoxedCopyFunc) anjuta_encoding_copy,
			(GBoxedFreeFunc) anjuta_encoding_free);

	return our_type;
} 

/**
 * anjuta_encoding_copy:
 * @enc: a #AnjutaEncoding.
 * 
 * Makes a copy of the given encoding.
 * This function is used by language bindings.
 *
 * Return value: a new #AnjutaEncoding.
 **/
AnjutaEncoding *
anjuta_encoding_copy (const AnjutaEncoding *enc)
{
	AnjutaEncoding *new_enc;

	g_return_val_if_fail (enc != NULL, NULL);
	
	new_enc = g_new0 (AnjutaEncoding, 1);
	*new_enc = *enc;

	return new_enc;
}


/**
 * anjuta_encoding_free:
 * @enc: a #AnjutaEncoding.
 * 
 * Frees the resources allocated by the given encoding.
 * This function is used by language bindings.
 **/
void 
anjuta_encoding_free (AnjutaEncoding *enc)
{
	g_return_if_fail (enc != NULL);
	
	g_free (enc);
}




