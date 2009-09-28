/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    Copyright (C) 2009 Maxim Ermilov   <zaspire@rambler.ru>

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

#ifndef _CODE_COMPLETION_H_
#define _CODE_COMPLETION_H_

#include "plugin.h"

GList* code_completion_get_list (JSLang *plugin, const gchar *tmp_file, const gchar *var_name, gint depth_level);
gchar* code_completion_get_str (IAnjutaEditor *editor, gboolean last_dot);
gboolean code_completion_is_symbol_func (JSLang *plugin, const gchar *var_name);
gchar* code_completion_get_func_tooltip (JSLang *plugin, const gchar *var_name);

gchar* file_completion (IAnjutaEditor *editor, gint *cur_depth);
GList* filter_list (GList *list, gchar *prefix);
#endif
