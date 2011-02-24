/*
 * python-indentation.h
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

#ifndef PYTHON_INDENTATION_H
#define PYTHON_INDENTATION_H

#include "plugin.h"

#include <libanjuta/interfaces/ianjuta-iterable.h>
#include <libanjuta/interfaces/ianjuta-editor.h>

void python_indent_init (PythonPlugin* lang_plugin);

void python_indent (PythonPlugin* lang_plugin, 
                    IAnjutaEditor* editor, 
                    IAnjutaIterable* insert_pos,
                    gchar ch);

void python_indent_auto (PythonPlugin* lang_plugin,
                         IAnjutaIterable* start,
                         IAnjutaIterable* end);

#endif