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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _STYLE_EDITOR_H_
#define _STYLE_EDITOR_H_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "properties.h"

typedef struct _StyleEditor StyleEditor;
typedef struct _StyleEditorPriv StyleEditorPriv;

struct _StyleEditor
{
	PropsID props_global;
	PropsID props_local;
	PropsID props_session;
	PropsID props;
	StyleEditorPriv *priv;
};

StyleEditor *
style_editor_new (PropsID props_global, PropsID prop_local,
				  PropsID prop_session, PropsID props);

void style_editor_destroy (StyleEditor *se);

void style_editor_show (StyleEditor *se);

void style_editor_hide (StyleEditor *se);

void style_editor_save_yourself (StyleEditor *se, FILE *stream);

#endif
