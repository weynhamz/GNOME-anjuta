/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-modeline.h
 * Copyright (C) Sébastien Granjoux 2013 <seb.sfo@free.fr>
 * 
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _ANJUTA_MODELINE_H_
#define _ANJUTA_MODELINE_H_

#include <glib.h>

#include <libanjuta/interfaces/ianjuta-editor.h>

G_BEGIN_DECLS

gboolean anjuta_apply_modeline (IAnjutaEditor *editor);

G_END_DECLS

#endif
