/* 
    launcher.h
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
#ifndef _LAUNCHER_H_
#define _LAUNCHER_H_

#include <gnome.h>

void launcher_init(void);

pid_t launcher_get_child_pid(void);

void launcher_reset(void);

void launcher_signal(int);

gboolean launcher_is_busy(void);

gboolean launcher_execute(gchar* command_str,
                               void (*stdout_func)(gchar*),
                               void (*stderr_func)(gchar*),
                               void (*command_terminated)(gint status, time_t time));

void launcher_send_stdin(gchar* input_str);

#endif
