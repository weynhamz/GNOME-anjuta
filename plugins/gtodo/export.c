#include <gtk/gtk.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include <string.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <gdk/gdkkeysyms.h>
#include "main.h"




static
void file_open(GtkWidget *button, GtkEntry *entry)
{
	GtkWidget *selection = NULL;
	selection = gtk_file_selection_new(_("Export to"));
	gtk_file_selection_set_filename(GTK_FILE_SELECTION(selection), gtk_entry_get_text(entry));
	switch(gtk_dialog_run(GTK_DIALOG(selection)))
	{
		case GTK_RESPONSE_OK:
			gtk_entry_set_text(entry, gtk_file_selection_get_filename(GTK_FILE_SELECTION(selection)));
		default:
			gtk_widget_destroy(selection);
	}
}



void export_backup_xml(void)
{
	GtkWidget *dialog;
	GtkWidget *entry;
	GtkWidget *label; 
	GtkWidget *hbox;
	GtkWidget *browse;
	char *temp;
	GError *error = NULL;
	GFile *file = NULL;

	/* setup the dialog */
	dialog = gtk_dialog_new_with_buttons("Export Task List",
			GTK_WINDOW(mw.window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, 
			GTK_STOCK_CANCEL,
			GTK_RESPONSE_REJECT,
			GTK_STOCK_OK,
			GTK_RESPONSE_ACCEPT,			
			NULL);
	/* the widgets in the dialog */
	hbox  = gtk_hbox_new(FALSE, 6);
	label = gtk_label_new("Save Location:");
	entry = gtk_entry_new();
	temp = g_strdup_printf("%s/backup.tasks", g_getenv("HOME"));
	gtk_entry_set_text(GTK_ENTRY(entry), temp);
	g_free(temp);

	browse = gtk_button_new_from_stock(GTK_STOCK_SAVE);
	/* pack the log */
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), browse, FALSE, TRUE, 0);
        gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), hbox);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 12);

	g_signal_connect(G_OBJECT(browse), "clicked", G_CALLBACK(file_open), entry);
	gtk_widget_show_all(dialog);
	switch(gtk_dialog_run(GTK_DIALOG(dialog)))
	{
		case GTK_RESPONSE_ACCEPT:
			g_print("saving to: %s\n", gtk_entry_get_text(GTK_ENTRY(entry)));
			file = g_file_parse_name ((char *)gtk_entry_get_text(GTK_ENTRY(entry)));
			if(gtodo_client_save_xml_to_file(cl, file, &error))
			{
			g_print("Other error\n");
			}
			if(error != NULL)
			{
			g_print("Error: %s\n", error->message);
			}

			g_object_unref (file);
		default:
			gtk_widget_destroy(dialog);


	}

}



static
void cust_cb_clicked(GtkToggleButton *but, GtkWidget *entry)
{
	gtk_widget_set_sensitive(entry, gtk_toggle_button_get_active(but));
}

static
void emb_cb_clicked(GtkToggleButton *but, GtkWidget *entry)
{
	gtk_widget_set_sensitive(entry, !gtk_toggle_button_get_active(but));
}


static
void export_xslt()
{
	GtkWidget *dialog;
	xmlDocPtr res;
	xsltStylesheetPtr cur;
	GtkWidget *label, *hbox;
	GtkWidget *loc_entry, *loc_browser;
	GtkWidget *emb_cb, *cust_cb, *cust_browser, *cb_curcat;;
	GtkWidget *box, *but;
	GFile* file;
	char *tmp_string;
	xmlChar *string;
	int length;
	char **param_string= NULL;

	/* setup the dialog */
	dialog = gtk_dialog_new_with_buttons("Export to html",
			GTK_WINDOW(mw.window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, NULL);
	/*			GTK_STOCK_CANCEL,
				GTK_RESPONSE_REJECT,
				GTK_STOCK_OK,
				GTK_RESPONSE_ACCEPT,			
				NULL);
				*/	but = gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT);		
	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_OK, GTK_RESPONSE_ACCEPT);
	//	gtk_widget_add_accelerator(but, "clicked", NULL, GDK_Escape,0 , GTK_ACCEL_VISIBLE); 
	box = gtk_vbox_new(FALSE, 6);
	/* I can't use the vbox of the dialog, somehow it wont set a border that way */
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), box);
	gtk_container_set_border_width(GTK_CONTAINER(box), 12);

	/* the location */
	hbox = gtk_hbox_new(FALSE, 6);
	label       = gtk_label_new("Save location:");
	loc_entry   = gtk_entry_new();
	loc_browser = gtk_button_new_from_stock(GTK_STOCK_OPEN);
	/* add them to the hor. box */
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), loc_entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), loc_browser, FALSE, TRUE, 0);
	/* add to vbox */
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);
	/* signals */
	g_signal_connect(G_OBJECT(loc_browser), "clicked",G_CALLBACK(file_open), loc_entry);
	tmp_string = g_strdup_printf("%s/output.html", g_getenv("HOME"));
	gtk_entry_set_text(GTK_ENTRY(loc_entry), tmp_string);
	g_free(tmp_string);

	/* add the embed css style sheet tb */
	emb_cb = gtk_check_button_new_with_label("Embed default (CSS) style sheet");
	gtk_box_pack_start(GTK_BOX(box), emb_cb, FALSE, TRUE, 0);
	/* add the custom stylesheet stuff */
	hbox         = gtk_hbox_new(FALSE, 6);
	cust_cb      = gtk_check_button_new_with_label("Custom (CSS) style sheet");
	cust_browser = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(cust_browser),"gtodo.css");
	gtk_widget_set_sensitive(cust_browser, FALSE);
	/* add them to the hor. box */
	gtk_box_pack_start(GTK_BOX(hbox), cust_cb, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), cust_browser, FALSE, TRUE, 0);             	
	/* add to vbox */   
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(cust_cb), "toggled",G_CALLBACK(cust_cb_clicked), cust_browser);
	g_signal_connect(G_OBJECT(emb_cb), "toggled",G_CALLBACK(emb_cb_clicked), hbox);


	/* add the embed css style sheet tb */
	cb_curcat = gtk_check_button_new_with_label("Export current category only");
	gtk_box_pack_start(GTK_BOX(box), cb_curcat, FALSE, TRUE, 0);           	

	gtk_widget_show_all(dialog);	
	switch(gtk_dialog_run(GTK_DIALOG(dialog)))
	{
		case GTK_RESPONSE_ACCEPT:
			break;
		default:
			gtk_widget_destroy(dialog);
			return;
	}


	cur= xsltParseStylesheetFile((xmlChar *)DATADIR"/gtodo/gtodo.xsl");
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(emb_cb)))
	{
		param_string = g_realloc(param_string, 3*sizeof(gchar *));
		param_string[0] = g_strdup("css");
		param_string[1] = g_strdup_printf("\"embed\"");
		param_string[2] = NULL;
	}
	else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cust_cb)))
	{
		param_string = g_realloc(param_string, 3*sizeof(gchar *));
		param_string[0] = g_strdup("css");
		param_string[1] = g_strdup_printf("\"%s\"", gtk_entry_get_text(GTK_ENTRY(cust_browser)));
		param_string[2] = NULL;                                   
	}

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb_curcat)))
	{
		int i= 0, cat = 0;
		for(i=0;param_string[i] != NULL;i++);
		cat = gtk_option_menu_get_history (GTK_OPTION_MENU (mw.option));

		if(cat != 0)
		{
			param_string = g_realloc(param_string, (i+3)*sizeof(gchar *));
			param_string[i]= g_strdup("category");			
			param_string[i+1] = g_strdup_printf("\"%s\"",mw.mitems[cat-2]->date);
			param_string[i+2] = NULL;
		}
	}	

	if(param_string == NULL)
	{
		res = xsltApplyStylesheet(cur, cl->gtodo_doc, NULL);
	}
	else
	{
		int i;
		res = xsltApplyStylesheet(cur, cl->gtodo_doc, (const char **)param_string);
		/* free info */
		for(i=0; param_string[i] != NULL; i++)
		{
			g_free(param_string[i]);
		}
		g_free(param_string);
	}
	xsltSaveResultToString(&string,&length , res, cur);
	
	file = g_file_parse_name (gtk_entry_get_text (GTK_ENTRY (loc_entry)));
	g_file_replace_contents (file, (char *)string, length, NULL, FALSE,
			G_FILE_CREATE_NONE, NULL, NULL, NULL);

	/* clean up some junk*/
	xmlFree(string);
	xsltFreeStylesheet(cur);
	xmlFreeDoc(res);
	xsltCleanupGlobals();
	gtk_widget_destroy(dialog);
}

void export_gui(void)
{
	export_xslt();
}
