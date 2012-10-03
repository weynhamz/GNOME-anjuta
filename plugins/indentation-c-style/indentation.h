/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * indent-c-indentation.h
 *
 * Copyright (C) 2011 - Johannes Schmid
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "plugin.h"

void
cpp_java_indentation_char_added (IAnjutaEditor *editor,
                                 IAnjutaIterable *insert_pos,
                                 gchar ch,
                                 IndentCPlugin *plugin);

void
cpp_java_indentation_changed (IAnjutaEditor *editor,
                              IAnjutaIterable *position,
                              gboolean added,
                              gint length,
                              gint lines,
                              const gchar *text,
                              IndentCPlugin* plugin);

void
cpp_auto_indentation (IAnjutaEditor *editor,
                      IndentCPlugin *plugin,
                      IAnjutaIterable *start,
                      IAnjutaIterable *end);

