/*
    glades.h
	Corba Server Implementation for Anjuta
    Copyright (C) 2001  LB

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

/* Interface for the gladen component */
#ifndef	GLADES_H
#define	GLADES_H


gboolean
gladen_start(void);
gboolean
gladen_load_project( const gchar *szFileName );
gboolean 
gladen_add_main_components(void);
gboolean
gladen_write_source( const gchar *szGladeFileName );



#endif	/*GLADES_H*/
