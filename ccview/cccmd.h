/*  CcviewProject widget
 *
 *  Original code by Ron Jones <ronjones@xnet.com>
 *  Complete rewrite by Naba Kumar <kh_naba@123india.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __CCCMD_H__
#define __CCCMD_H__

#include <gnome.h>
#include <gtk/gtkitemfactory.h>

class class_pane;
class file_pane;

class popup_menu
{
protected:
    GtkItemFactory *item_factory;
    GtkAccelGroup *accel_group;

public:
    popup_menu();
    void popup(guint x,guint y,guint32 ti);
};

class class_popup: public popup_menu
{
    class_pane *owner;
    static GtkItemFactoryEntry menus[];

public:
    class_popup(class_pane *in);
    static void class_def(gpointer data,guint action,GtkWidget *widget);
    static void func_decl(gpointer data,guint action,GtkWidget *widget);
    static void func_def(gpointer data,guint action,GtkWidget *widget);
    static void add_func(gpointer data,guint action,GtkWidget *widget);
	static void properties(gpointer data,guint action,GtkWidget *widget);
};

class file_popup: public popup_menu
{
    file_pane *owner;
    static GtkItemFactoryEntry menus[];
	
public:
    file_popup(file_pane *owner);
    static void shell_here(gpointer data,guint action,GtkWidget *widget);
    static void open(gpointer data,guint action,GtkWidget *widget);
	static void properties(gpointer data,guint action,GtkWidget *widget);
};

#endif // __CCCMND_H__ 
