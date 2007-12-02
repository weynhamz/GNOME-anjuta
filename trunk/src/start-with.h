/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * start-with.h
 * Copyright (C) 2003  Naba Kumar  <naba@gnome.org>
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free 
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59 
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA 
 */
#ifndef _START_WITH_H_
#define _START_WITH_H_

void start_with_dialog_show (GtkWindow *parent, AnjutaPreferences *pref,
							 gboolean force);
void start_with_dialog_save_yourself (AnjutaPreferences *pref, FILE *stream);

#endif
