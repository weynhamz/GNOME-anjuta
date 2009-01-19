#include <gtk/gtk.h>
#include "main.h"


static GtkWidget* gtodo_export_gui_create_extra_widget (void);

enum
{
	FILE_TYPE_XML,
	FILE_TYPE_PLAIN,
	FILE_TYPE_HTML,
	FILE_TYPE_COUNT
};

typedef struct _ExportExtraWidget
{
	GtkWidget *file_type;
	GtkWidget *emb_cb;
	GtkWidget *cust_cb;
	GtkWidget *cust_browser;
	GtkWidget *cb_curcat;
} ExportExtraWidget;

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


void export_gui()
{
	GtkWidget *dialog;
	char *tmp_string;
	char **param_string= NULL;
	ExportExtraWidget *eex = NULL;
	GError *error = NULL;

	dialog = gtk_file_chooser_dialog_new (_("Export task list"),
			NULL,
			GTK_FILE_CHOOSER_ACTION_SAVE,
			GTK_STOCK_CANCEL,
			GTK_RESPONSE_REJECT,
			GTK_STOCK_OK,
			GTK_RESPONSE_ACCEPT,
			NULL);

	tmp_string = g_build_filename (g_get_home_dir(), "output.html", NULL);
	gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), tmp_string);
	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (dialog), FALSE);
	gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (dialog), 
			gtodo_export_gui_create_extra_widget ());
	g_free(tmp_string);

	gtk_widget_show_all(dialog);
	switch(gtk_dialog_run(GTK_DIALOG(dialog)))
	{
		case GTK_RESPONSE_ACCEPT:
			gtk_widget_hide (dialog);
			break;
		default:
			gtk_widget_destroy(dialog);
			return;
	}

	eex = g_object_get_data (G_OBJECT (gtk_file_chooser_get_extra_widget (GTK_FILE_CHOOSER (dialog))),
			"eex");

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(eex->emb_cb)))
	{
		param_string = g_realloc(param_string, 3*sizeof(gchar *));
		param_string[0] = g_strdup("css");
		param_string[1] = g_strdup_printf("\"embed\"");
		param_string[2] = NULL;
	}
	else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(eex->cust_cb)))
	{
		param_string = g_realloc(param_string, 3*sizeof(gchar *));
		param_string[0] = g_strdup("css");
		param_string[1] = g_strdup_printf("\"%s\"", gtk_entry_get_text(GTK_ENTRY(eex->cust_browser)));
		param_string[2] = NULL;                                   
	}

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(eex->cb_curcat)))
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

	switch (gtk_combo_box_get_active (GTK_COMBO_BOX (eex->file_type)))
	{
		case FILE_TYPE_HTML:
			gtodo_client_export (cl, gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog)), 
				DATADIR"/gtodo/gtodo.xsl", param_string, &error);
			break;
		case FILE_TYPE_PLAIN:
			gtodo_client_export (cl, gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog)), 
				DATADIR"/gtodo/gtodo-plain.xsl", param_string, &error);
			break;
		case FILE_TYPE_XML:
			gtodo_client_save_xml_to_file (cl, gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog)),
					&error);
			break;
		default:
			break;
	}

	g_strfreev (param_string);

	g_free (eex);

	/* clean up some junk*/
	gtk_widget_destroy(dialog);
}

static void
on_file_type_changed_cb (GtkComboBox *box, GtkWidget* xml_export_options)
{
	gtk_widget_set_sensitive (xml_export_options, 
			gtk_combo_box_get_active (box) == 2);
}

GtkWidget*
gtodo_export_gui_create_extra_widget ()
{
	GtkWidget *extra_widget;
	GtkWidget *box;
	GtkWidget *hbox;
	GtkWidget *w;
	ExportExtraWidget *eex = g_new0 (ExportExtraWidget, 1);


	extra_widget = gtk_vbox_new (FALSE, 0);
	
	eex->file_type = gtk_combo_box_new_text ();
	gtk_combo_box_insert_text (GTK_COMBO_BOX (eex->file_type), FILE_TYPE_XML, _("XML"));
	gtk_combo_box_insert_text (GTK_COMBO_BOX (eex->file_type), FILE_TYPE_PLAIN, _("Plain Text"));
	gtk_combo_box_insert_text (GTK_COMBO_BOX (eex->file_type), FILE_TYPE_HTML, _("HTML"));
	gtk_combo_box_set_active (GTK_COMBO_BOX (eex->file_type), FILE_TYPE_HTML);


	gtk_box_pack_start (GTK_BOX (extra_widget), eex->file_type, FALSE, FALSE, 0);

	/* add the embed css style sheet tb */
	eex->cb_curcat = gtk_check_button_new_with_label(_("Export current category only"));
	gtk_box_pack_start(GTK_BOX(extra_widget), eex->cb_curcat, FALSE, TRUE, 0);           	

	w = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (w), _("<b>HTML export options:</b>"));
	gtk_misc_set_alignment (GTK_MISC (w), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (extra_widget), w, FALSE, FALSE, 0);

	w = gtk_alignment_new (0, 0, 1, 1);
	gtk_alignment_set_padding (GTK_ALIGNMENT (w), 0, 0, 12, 0);
	gtk_box_pack_start (GTK_BOX (extra_widget), w, FALSE, FALSE, 0);

	box = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (w), box);
	eex->emb_cb = gtk_check_button_new_with_label(_("Embed default (CSS) style sheet"));
	gtk_box_pack_start(GTK_BOX(box), eex->emb_cb, FALSE, TRUE, 0);
	/* add the custom stylesheet stuff */
	hbox         = gtk_hbox_new(FALSE, 6);
	eex->cust_cb      = gtk_check_button_new_with_label(_("Custom (CSS) style sheet"));
	eex->cust_browser = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(eex->cust_browser),"gtodo.css");
	gtk_widget_set_sensitive(eex->cust_browser, FALSE);
	/* add them to the hor. box */
	gtk_box_pack_start(GTK_BOX(hbox), eex->cust_cb, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), eex->cust_browser, FALSE, TRUE, 0);             	
	/* add to vbox */   
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(eex->cust_cb), "toggled",G_CALLBACK(cust_cb_clicked), eex->cust_browser);
	g_signal_connect(G_OBJECT(eex->emb_cb), "toggled",G_CALLBACK(emb_cb_clicked), hbox);



	g_object_set_data (G_OBJECT (extra_widget), "eex", eex);

	g_signal_connect (G_OBJECT (eex->file_type), "changed",
			G_CALLBACK (on_file_type_changed_cb), box);
	return extra_widget;
}

