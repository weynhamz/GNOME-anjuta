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

#include <config.h>
#include "cccmd.h"
#include "ccview_main.h"

GtkItemFactoryEntry class_popup::menus[]=
{
    {N_("/_Class Definition"),NULL,GTK_SIGNAL_FUNC(class_popup::class_def),0,NULL},
    {N_("/_Func Definition"),NULL,GTK_SIGNAL_FUNC(class_popup::func_def),0,NULL},
    {N_("/Func _Declaration"),NULL,GTK_SIGNAL_FUNC(class_popup::func_decl),0,NULL},
    {N_("/_Add function"),NULL,GTK_SIGNAL_FUNC(class_popup::add_func),0,NULL},
    {N_("/_Properties"),NULL,GTK_SIGNAL_FUNC(class_popup::properties),0,NULL}
};

GtkItemFactoryEntry file_popup::menus[]=
{
    {N_("/_Shell here"),NULL,GTK_SIGNAL_FUNC(file_popup::shell_here),0,NULL},
    {N_("/_Open"),NULL,GTK_SIGNAL_FUNC(file_popup::open),0,NULL},
    {N_("/_Properties"),NULL,GTK_SIGNAL_FUNC(file_popup::properties),0,NULL}
};
    
popup_menu::popup_menu()
{
    accel_group=gtk_accel_group_new();
    item_factory=gtk_item_factory_new(GTK_TYPE_MENU,"<main>",accel_group);
}

void popup_menu::popup(guint x,guint y,guint32 ti)
{
    gtk_item_factory_popup(item_factory,x,y,3,ti);
}  

file_popup::file_popup(file_pane *in)
{
    owner=in;
    int nitems=sizeof(menus)/sizeof(menus[0]);
    gtk_item_factory_create_items(item_factory,nitems,menus,this);
}

void file_popup::shell_here(gpointer data,guint action,GtkWidget *widget)
{
    ((file_popup *)data)->owner->popup_shell_here();
}

void file_popup::open(gpointer data,guint action,GtkWidget *widget)
{
    ((file_popup *)data)->owner->popup_open();
}

void file_popup::properties(gpointer data,guint action,GtkWidget *widget)
{
    ((file_popup *)data)->owner->popup_properties();
}


class_popup::class_popup(class_pane *in)
{
    owner=in;
    int nitems=sizeof(menus)/sizeof(menus[0]);
    gtk_item_factory_create_items(item_factory,nitems,menus,this);
}

void class_popup::class_def(gpointer data,guint action,GtkWidget *widget)
{
    ((class_popup *)data)->owner->popup_class_def();
}

void class_popup::add_func(gpointer data,guint action,GtkWidget *widget)
{
    ((class_popup *)data)->owner->popup_addfunc();
}

void class_popup::func_def(gpointer data,guint action,GtkWidget *widget)
{
    ((class_popup *)data)->owner->popup_func_def();
}

void class_popup::func_decl(gpointer data,guint action,GtkWidget *widget)
{
    ((class_popup *)data)->owner->popup_func_decl();
}

void class_popup::properties(gpointer data,guint action,GtkWidget *widget)
{
    ((class_popup *)data)->owner->popup_properties();
}
