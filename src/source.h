/*
    source.c
    Copyright (C) 2000  Kh. Naba Kumar Singh

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

#ifndef __SOURCE_H__
#define __SOURCE_H__

#include "project_dbase.h"

gboolean
source_write_generic_main_c (ProjectDBase *data);

gboolean
source_write_wxwin_main_c (ProjectDBase *data);

gboolean
source_write_libglade_main_c (ProjectDBase *data);

gboolean
source_write_build_files (ProjectDBase * data);

#endif	/* __SOURCE_H__ */

