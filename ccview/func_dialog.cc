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

#include "func_dialog.h"

func_dlg::func_dlg()
{
    GtkWidget *label;
    GtkWidget *alignment;
    GtkWidget *hbox;
    win=gnome_dialog_new(_("Add Function"),
                         GNOME_STOCK_BUTTON_OK,GNOME_STOCK_BUTTON_CANCEL,NULL);
    gtk_window_set_position(GTK_WINDOW(win),GTK_WIN_POS_MOUSE);
    label=gtk_label_new(_("Function declaration:"));
    alignment=gtk_alignment_new(0.0,0.5,0.0,0.0);
    gtk_container_add(GTK_CONTAINER(alignment),label);
    gtk_label_set_justify(GTK_LABEL(label),GTK_JUSTIFY_LEFT);
    gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(win)->vbox),alignment,FALSE,FALSE,0);
    fentry=gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(win)->vbox),fentry,FALSE,FALSE,0);
    hbox=gtk_hbox_new(FALSE,0);
    bstatic=gtk_check_button_new_with_label("static");
    gtk_box_pack_start(GTK_BOX(hbox),bstatic,FALSE,FALSE,0);    
    bconst=gtk_check_button_new_with_label("const");
    gtk_box_pack_start(GTK_BOX(hbox),bconst,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(win)->vbox),hbox,FALSE,FALSE,0);
    hbox=gtk_hbox_new(FALSE,0);
    bpublic=gtk_radio_button_new_with_label(NULL,"public");
    gtk_box_pack_start(GTK_BOX(hbox),bpublic,FALSE,FALSE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bpublic),TRUE);
    bprotected=gtk_radio_button_new_with_label(
        gtk_radio_button_group(GTK_RADIO_BUTTON(bpublic)),"protected");
    gtk_box_pack_start(GTK_BOX(hbox),bprotected,FALSE,FALSE,0);
    bprivate=gtk_radio_button_new_with_label(
        gtk_radio_button_group(GTK_RADIO_BUTTON(bprotected)),"private");
    gtk_box_pack_start(GTK_BOX(hbox),bprivate,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(win)->vbox),hbox,FALSE,FALSE,0);
    gtk_widget_show_all(GNOME_DIALOG(win)->vbox);
    
}
bool func_dlg::gomodal()
{
    int gval=gnome_dialog_run(GNOME_DIALOG(win));
    bool res=false;
    if(gval==0)
    {
        func=gtk_entry_get_text(GTK_ENTRY(fentry));
        qstatic=GTK_TOGGLE_BUTTON(bstatic)->active;
        qconst=GTK_TOGGLE_BUTTON(bconst)->active;
        if(GTK_TOGGLE_BUTTON(bpublic)->active)
        {
            ftype=fpublic;
        }
        else if(GTK_TOGGLE_BUTTON(bprotected)->active)
        {
            ftype=fprotected;
        }
        else
        {
            ftype=fprivate;
        }
        res=true;
    }
    gnome_dialog_close(GNOME_DIALOG(win));
    return res;
}
string func_dlg::getfuncdecl(const char *class_name) const
{
    string res;
    if(isstatic())
    {
        res="static "+func;
    }
    else
    {
        res=func;
    }
    if(isconst())
    {
        res += " const";
    }
    res += ";";
    return res;
}
string func_dlg::getfuncdef(const char *class_name) const
{
    string res;
    string::size_type sppos;
    string::size_type parenpos;
    sppos=func.find(' ');
    parenpos=func.find('(');
    if(parenpos != string::npos && sppos != string::npos &&  sppos < parenpos)
    {
        res.assign(func,0,sppos+1);
        res += class_name;
        res += "::";
        res += func.substr(sppos+1);
    }
    else
    {
        res=class_name;
        res += "::";
        res += func;
    }
    if(isconst())
    {
        res += " const";
    }
    return res;
}
