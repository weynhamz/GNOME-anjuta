/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __GTK_GOTO_LINE_H__
#define __GTK_GOTO_LINE_H__


//#include <libgnome/gnome-defs.h>
#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C"
{
#endif				/* __cplusplus */

#define TYPE_GOTOLINE            (gotoline_get_type ())

#define GOTOLINE(obj)            (GTK_CHECK_CAST ((obj), TYPE_GOTOLINE, GotoLine))
#define GOTOLINE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), TYPE_GOTOLINE, GotoLineClass))

#define IS_GOTOLINE(obj)         (GTK_CHECK_TYPE ((obj), TYPE_GOTOLINE))
#define IS_GOTOLINE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), TYPE_GOTOLINE))

  typedef struct _GotoLine GotoLine;
  typedef struct _GotoLineClass GotoLineClass;

  /** The Base of the gotoline dialog, one per gotoline dialog instance */
  struct _GotoLine
  {
    GtkDialog parent;
  };

  /** The Base Class of the gotoline dialog, only one in existance 
  \todo Add signal to allow line number to be retrieved when recording a macro */
  struct _GotoLineClass
  {
    GtkDialogClass parent_class;
  };

  /** GTK widget implementation function */
  guint gotoline_get_type (void);
  
  /** Create a new instance of this gotoline dialog */
  GtkWidget *gotoline_new (void);
  
  /** Sets the displayed linenumber */
  void gotoline_set_linenumber (guint newlinenum);

#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif				/* __GTK_GOTO_LINE_H__ */
