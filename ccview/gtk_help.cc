/*  CcviewProject widget
 *
 *  Original code by Ron Jones <ronjones@xnet.com>
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

#include "gtk_help.h"
#include <stack>

void make_image_label(GtkContainer *cont,char **imdat,gchar *txt)
{
    GtkWidget *hbox,*image,*label;
    hbox=gtk_hbox_new(FALSE,3);
    gtk_container_add(cont,hbox);
    if(imdat != NULL)
    {
        image=gnome_pixmap_new_from_xpm_d(imdat);
        gtk_box_pack_start(GTK_BOX(hbox),image,FALSE,FALSE,0);
        gtk_widget_show(image);
    }
    label=gtk_label_new(txt);
    gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,0);
    gtk_widget_show(label);
    gtk_widget_show(hbox);
}

gchar *bin_get_contained_image_text(GtkBin *bin)
{
    GtkWidget *box;
    gchar *res;
    box=GTK_WIDGET(bin->child);
    GList *list=gtk_container_children(GTK_CONTAINER(box));
    GList *ptr=list;
    if(GTK_IS_LABEL(list->data)==FALSE)
    {
        ptr=list->next;
    }
    gtk_label_get(GTK_LABEL(ptr->data),&res);
    g_list_free(list);
    return res;
}

gchar * bin_get_contained_text(GtkBin *bin)
{
    GtkWidget *child;
    gchar *res;
    child=GTK_WIDGET(bin->child);
    gtk_label_get(GTK_LABEL(child),&res);
    return res;
}

string tree_get_expanded_name(GtkTreeItem *item,const char *nstart)
{
    string res=nstart;
    stack<string> strs;
    GtkWidget *itemptr=GTK_WIDGET(item);
    GtkWidget *tree;
    tree=GTK_WIDGET(item)->parent;
    while(!GTK_IS_ROOT_TREE(GTK_TREE(tree)))
    {
        strs.push(bin_get_contained_image_text(GTK_BIN(itemptr)));
        itemptr=GTK_TREE(tree)->tree_owner;
        tree=itemptr->parent;
    }
    while(!strs.empty())
    {
        if(res[res.size()-1]!= '/')
            res.append(1,'/');
        res.append(strs.top());
        strs.pop();
    }
    return res;
}
        
    
    
    
    

void tree_save_expanded(GtkTree *tree,set<string> &eset)
{
    GList *children;
    gchar *lab;
    children=gtk_container_children(GTK_CONTAINER(tree));
    while(children != NULL)
    {
        if(GTK_TREE_ITEM(children->data)->expanded)
        {
            lab=bin_get_contained_image_text(GTK_BIN(children->data));
            eset.insert(lab);
        }
        children=g_list_remove_link(children,children);
    }
}
void tree_restore_selection(GtkTree *tree,set<string> &eset,
                            const string &sel_parent,const string &sel_item)
{
    GList *children;
    GList *funcs;
    GList *ptr;
    gchar *lab;
    GtkWidget *subtree;
    children=gtk_container_children(GTK_CONTAINER(tree));
    while(children != NULL)
    {
        lab=bin_get_contained_image_text(GTK_BIN(children->data));
        if(eset.count(lab) > 0)
        {
            gtk_tree_item_expand(GTK_TREE_ITEM(children->data));
            if(sel_parent==lab)
            {
                subtree=GTK_TREE_ITEM_SUBTREE(GTK_TREE_ITEM(children->data));
                if(subtree)
                {
                    funcs=gtk_container_children(GTK_CONTAINER(subtree));
                    ptr=funcs;
                    while(ptr != NULL)
                    {
                        lab=bin_get_contained_image_text(GTK_BIN(ptr->data));
                        if(sel_item==lab)
                        {
                            gtk_tree_item_select(GTK_TREE_ITEM(ptr->data));
                            break;
                        }
                        ptr=ptr->next;
                    }
                    g_list_free(funcs);
                }
            }
        }
        children=g_list_remove_link(children,children);
    }
}
                        
                    
                
            
