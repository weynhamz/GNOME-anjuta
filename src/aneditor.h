/*
    aneditor.h
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
#ifndef _ANEDITOR_H_
#define _ANEDITOR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <gtk/gtk.h>

#define   ANE_COMMAND_BASE              0
#define   ANE_UNDO                      ANE_COMMAND_BASE+1
#define   ANE_REDO                      ANE_COMMAND_BASE+2
#define   ANE_CUT                       ANE_COMMAND_BASE+3
#define   ANE_COPY                      ANE_COMMAND_BASE+4
#define   ANE_PASTE                     ANE_COMMAND_BASE+5
#define   ANE_CLEAR                     ANE_COMMAND_BASE+6
#define   ANE_SELECTALL                 ANE_COMMAND_BASE+7
#define   ANE_FIND                      ANE_COMMAND_BASE+8

/*
 * This is not an Aneditor command, so don't use it as command.
 * Scintilla does not define it. So, I did here because I need it.
 * For the time being, let's assume scintilla does not use this bit field.
 */
#define   ANEFIND_REVERSE_FLAG          0x40000000

#define   ANE_GETBLOCKSTARTLINE         ANE_COMMAND_BASE+9
#define   ANE_GETBLOCKENDLINE           ANE_COMMAND_BASE+10
#define   ANE_GETCURRENTWORD            ANE_COMMAND_BASE+11
          /* 12 => Reserved */
#define   ANE_MATCHBRACE                ANE_COMMAND_BASE+13
#define   ANE_SELECTTOBRACE             ANE_COMMAND_BASE+14
#define   ANE_SHOWCALLTIP               ANE_COMMAND_BASE+15
#define   ANE_COMPLETE                  ANE_COMMAND_BASE+16
#define   ANE_COMPLETEWORD              ANE_COMMAND_BASE+17
#define   ANE_SELECTBLOCK               ANE_COMMAND_BASE+18
#define   ANE_UPRCASE                   ANE_COMMAND_BASE+19
#define   ANE_LWRCASE                   ANE_COMMAND_BASE+20
#define   ANE_EXPAND                    ANE_COMMAND_BASE+21
#define   ANE_LINENUMBERMARGIN          ANE_COMMAND_BASE+22
#define   ANE_SELMARGIN                 ANE_COMMAND_BASE+23
#define   ANE_FOLDMARGIN                ANE_COMMAND_BASE+24
#define   ANE_VIEWEOL                   ANE_COMMAND_BASE+25
#define   ANE_EOL_CRLF                  ANE_COMMAND_BASE+26
#define   ANE_EOL_CR                    ANE_COMMAND_BASE+27
#define   ANE_EOL_LF                    ANE_COMMAND_BASE+28
#define   ANE_EOL_CONVERT               ANE_COMMAND_BASE+29
#define   ANE_WORDPARTLEFT              ANE_COMMAND_BASE+30
#define   ANE_WORDPARTLEFTEXTEND        ANE_COMMAND_BASE+31
#define   ANE_WORDPARTRIGHT             ANE_COMMAND_BASE+32
#define   ANE_WORDPARTRIGHTEXTEND       ANE_COMMAND_BASE+33
#define   ANE_VIEWSPACE                 ANE_COMMAND_BASE+34
#define   ANE_VIEWGUIDES                ANE_COMMAND_BASE+35
#define   ANE_BOOKMARK_TOGGLE           ANE_COMMAND_BASE+36
#define   ANE_BOOKMARK_FIRST            ANE_COMMAND_BASE+37
#define   ANE_BOOKMARK_PREV             ANE_COMMAND_BASE+38
#define   ANE_BOOKMARK_NEXT             ANE_COMMAND_BASE+39
#define   ANE_BOOKMARK_LAST             ANE_COMMAND_BASE+40
#define   ANE_BOOKMARK_CLEAR            ANE_COMMAND_BASE+41
#define   ANE_SETTABSIZE                ANE_COMMAND_BASE+42
#define   ANE_SETLANGUAGE               ANE_COMMAND_BASE+43
#define   ANE_SETHILITE                 ANE_COMMAND_BASE+44
#define   ANE_READPROPERTIES            ANE_COMMAND_BASE+45
#define   ANE_GOTOLINE                  ANE_COMMAND_BASE+46
#define   ANE_GOTOPOINT                 ANE_COMMAND_BASE+47
#define   ANE_SETZOOM                   ANE_COMMAND_BASE+48
#define   ANE_SETACCELGROUP             ANE_COMMAND_BASE+49
#define   ANE_GETTEXTRANGE              ANE_COMMAND_BASE+50
#define   ANE_TOGGLE_FOLD               ANE_COMMAND_BASE+51
#define   ANE_CLOSE_FOLDALL             ANE_COMMAND_BASE+52
#define   ANE_OPEN_FOLDALL              ANE_COMMAND_BASE+53
#define   ANE_INDENT_INCREASE           ANE_COMMAND_BASE+54
#define   ANE_INDENT_DECREASE           ANE_COMMAND_BASE+55
#define   ANE_INSERTTEXT                ANE_COMMAND_BASE+56
#define   ANE_GETBOOKMARK_POS           ANE_COMMAND_BASE+57
#define   ANE_BOOKMARK_TOGGLE_LINE      ANE_COMMAND_BASE+58
#define   ANE_GETLENGTH                 ANE_COMMAND_BASE+59
#define   ANE_GET_LINENO                ANE_COMMAND_BASE+60
#define   ANE_LINEWRAP                  ANE_COMMAND_BASE+61
#define   ANE_READONLY                  ANE_COMMAND_BASE+62

typedef guint AnEditorID;
typedef struct _FindParameters FindParameters;

AnEditorID
aneditor_new(gpointer p);

void
aneditor_destroy(AnEditorID id);

GtkWidget*
aneditor_get_widget(AnEditorID id);

glong
aneditor_command(AnEditorID id, gint command, glong wparam, glong wlaram);

#ifdef __cplusplus
}
#endif

#endif /* _ANEDITOR_H_ */
