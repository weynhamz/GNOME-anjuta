/*
    style-editor.h
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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _STYLE_EDITOR_H_
#define _STYLE_EDITOR_H_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <libanjuta/anjuta-preferences.h>
#include "properties.h"

typedef struct _StyleEditor StyleEditor;
typedef struct _StyleEditorPriv StyleEditorPriv;

struct _StyleEditor
{
	/*
	PropsID props_global;
	PropsID props_local;
	PropsID props_session;
	*/
	PropsID props;
	StyleEditorPriv *priv;
	AnjutaPreferences *prefs;
};

StyleEditor *
style_editor_new (AnjutaPreferences *prefs);

void style_editor_destroy (StyleEditor *se);

void style_editor_show (StyleEditor *se);

void style_editor_hide (StyleEditor *se);

void style_editor_save (StyleEditor *se, FILE *stream);

#endif
