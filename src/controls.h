/*
    controls.h
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
#ifndef _CONTROLS_H_
#define _CONTROLS_H_
#define __NO_CONTROLS

#ifdef __NO_CONTROLS

#define main_toolbar_update()
#define debug_toolbar_update()
#define extended_toolbar_update() 
#define format_toolbar_update()
#define browser_toolbar_update()

#define update_main_menubar()
#define update_led_animator()

#else

void main_toolbar_update(void);
void debug_toolbar_update(void);
void extended_toolbar_update(void); 
void format_toolbar_update(void);
void browser_toolbar_update(void);

void update_main_menubar(void);
void update_led_animator(void);

#endif

#endif
