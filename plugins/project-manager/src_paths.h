/*
    src_paths.h
    Copyright (C) 2000  Naba Kumar <naba@gnome.org>

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
#ifndef _SRC_PATHS_H_
#define _SRC_PATHS_H_

typedef struct _SrcPaths SrcPaths;
typedef struct _SrcPathsPriv SrcPathsPriv;
	
struct _SrcPaths
{
	SrcPathsPriv *priv;
};

SrcPaths *src_paths_new (void);
void src_paths_destroy (SrcPaths *);
void src_paths_show (SrcPaths *);
void src_paths_hide (SrcPaths *);
GList *src_paths_get_paths (SrcPaths *);

gboolean src_paths_save (SrcPaths * co, FILE * s);
gboolean src_paths_load (SrcPaths * co, PropsID props);

#endif
