#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include <glade/glade.h>

#include "indent-util.h"
#include "indent-dialog.h"


#define PARAMETERS_ENTRY "indent_parameters_entry"
#define PREVIEW_BUTTON "indent_preview_button"
#define UPDATE_BUTTON "indent_update_button"
#define DELETE_BUTTON "indent_delete_button"
#define NEW_BUTTON "indent_new_button"
#define QUIT_BUTTON "indent_quit_button"
#define STYLE_COMBOBOX "indent_style_combobox"
#define PREVIEW_TEXTVIEW "indent_preview_textview"
#define DIALOG "indent_dialog"
#define STYLE_ENTRY "indent_style_entry"


void indent_widget_signal_connect(gchar *name_widget, gchar *signal, 
                                  gpointer func,IndentData *idt);
gchar *indent_spinbutton_get_value(gchar *name_widget, IndentData *idt);
void indent_init_items(gpointer key, gpointer data, IndentData *idt);
void indent_connect_items(gpointer key, gpointer data, IndentData *idt);
void indent_init_dialog(IndentData *idt);
void indent_init_connect(IndentData *idt);
gchar *indent_entry_get_chars(IndentData *idt);
void indent_entry_set_chars(gchar *text, IndentData *idt);
void indent_block_widget(gchar *name_widget, gpointer func, gboolean block, 
                         IndentData *idt);
void on_indent_checkbutton_toggled(GtkToggleButton *button, IndentData *idt);
void on_indent_spinbutton_value_changed(GtkSpinButton *button, IndentData *idt);
void on_indent_parameters_entry_changed(GtkEditable *edit, IndentData *idt);
void on_indent_preview_button_clicked(GtkButton *button, IndentData *idt);
void on_indent_update_button_clicked(GtkButton *button, IndentData *idt);
void on_indent_new_button_clicked(GtkButton *button, IndentData *idt);
void on_indent_delete_button_clicked(GtkButton *button, IndentData *idt);
void on_indent_style_combobox_changed(GtkComboBox *combo, IndentData *idt);
void on_indent_quit_button_clicked(GtkButton *button, IndentData *idt);
void indent_set_style_combo(gint index, IndentData *idt);
void indent_exit(GtkWidget *widget, gpointer user_data);
void indent_display_buffer(gchar *buffer, IndentData *idt);



/*****************************************************************************/
void
pref_style_combo_changed(GtkComboBox *combo, IndentData *idt)
{
	gchar *style_name;
	gchar *options;
	
	style_name = gtk_combo_box_get_active_text(combo);
	options = indent_find_style(style_name, idt);
	if (options)
		gtk_entry_set_text(GTK_ENTRY(idt->pref_indent_options), options);
	indent_save_active_style(style_name, options, idt);
}

void
pref_set_style_combo(IndentData *idt)
{
	GList *list;
	IndentStyle *ist;

	list = idt->style_list;
	while (list)
	{
		ist = list->data;
		gtk_combo_box_append_text(GTK_COMBO_BOX(idt->pref_indent_combo), ist->name);
		list = g_list_next(list);
	}	
	gtk_combo_box_set_active(GTK_COMBO_BOX(idt->pref_indent_combo), idt->style_active);
}

/*****************************************************************************/

void
indent_widget_signal_connect(gchar *name_widget, gchar *signal, gpointer func,
                             IndentData *idt)
{
	GtkWidget *widget;
	
	widget = glade_xml_get_widget(idt->xml, name_widget);
	g_signal_connect(widget, signal, G_CALLBACK (func), idt);
}

void
indent_toggle_button_set_active(gchar *name_widget, gboolean active, IndentData *idt)
{
	GtkWidget *widget;
	
	widget = glade_xml_get_widget(idt->xml, name_widget);
	gtk_toggle_button_set_active((GtkToggleButton *) widget, active);
}

void
indent_widget_set_sensitive(gchar *name_widget, gboolean sensitive, IndentData *idt)
{
	GtkWidget *widget;
	
	widget = glade_xml_get_widget(idt->xml, name_widget);
	gtk_widget_set_sensitive(widget, sensitive);
}

void
indent_spinbutton_set_value(gchar *name_widget, gchar *num, IndentData *idt)
{
	GtkWidget *widget;

	widget = glade_xml_get_widget(idt->xml, name_widget);
	gtk_spin_button_set_value((GtkSpinButton*) widget, g_strtod(num, NULL));
}

gchar *
indent_spinbutton_get_value(gchar *name_widget, IndentData *idt)
{
	GtkWidget *widget;
	gint value;

	widget = glade_xml_get_widget(idt->xml, name_widget);
	value = gtk_spin_button_get_value_as_int((GtkSpinButton*) widget);
	return g_strdup_printf("%d", value); 
}

void
indent_init_items(gpointer key, gpointer data, IndentData *idt)
{
	OptionData *ptrdata;

	ptrdata = data;
	indent_toggle_button_set_active(ptrdata->checkbutton, FALSE, idt);
	if (ptrdata->spinbutton)
		indent_widget_set_sensitive(ptrdata->spinbutton, FALSE, idt);
}

void
indent_connect_items(gpointer key, gpointer data, IndentData *idt)
{
	OptionData *ptrdata;

	ptrdata = data;
	indent_widget_signal_connect(ptrdata->checkbutton,  "toggled",
	                 GTK_SIGNAL_FUNC(on_indent_checkbutton_toggled), 
	                 idt);
	
	if (ptrdata->spinbutton)
		indent_widget_signal_connect(ptrdata->spinbutton, "value_changed",
	                 GTK_SIGNAL_FUNC(on_indent_spinbutton_value_changed), 
	                 idt);
}

void
indent_init_dialog(IndentData *idt)
{
	g_hash_table_foreach(idt->option_hash, (GHFunc) indent_init_items, idt);
}

void
indent_init_connect(IndentData *idt)
{
	g_hash_table_foreach(idt->option_hash, (GHFunc) indent_connect_items, idt);
	indent_widget_signal_connect(PARAMETERS_ENTRY, "changed",
	                 G_CALLBACK (on_indent_parameters_entry_changed), 
	                 idt);
	indent_widget_signal_connect(PREVIEW_BUTTON, "clicked",
	                 G_CALLBACK (on_indent_preview_button_clicked), 
	                 idt);
	indent_widget_signal_connect(UPDATE_BUTTON, "clicked",
	                 G_CALLBACK (on_indent_update_button_clicked), 
	                 idt);
	indent_widget_signal_connect(DELETE_BUTTON, "clicked",
	                 G_CALLBACK (on_indent_delete_button_clicked), 
	                 idt);
	indent_widget_signal_connect(NEW_BUTTON, "clicked",
	                 G_CALLBACK (on_indent_new_button_clicked), 
	                 idt);
	indent_widget_signal_connect(QUIT_BUTTON, "clicked",
	                 G_CALLBACK (on_indent_quit_button_clicked), 
	                 idt);
	indent_widget_signal_connect(STYLE_COMBOBOX, "changed",
	                 G_CALLBACK (on_indent_style_combobox_changed), 
	                 idt);
}

gchar *
indent_entry_get_chars(IndentData *idt)
{
	GtkWidget *widget;

	widget = glade_xml_get_widget(idt->xml, PARAMETERS_ENTRY);
	return gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
}

void
indent_entry_set_chars(gchar *text, IndentData *idt)
{
	GtkWidget *widget;

	widget = glade_xml_get_widget(idt->xml, PARAMETERS_ENTRY);
	gtk_entry_set_text(GTK_ENTRY(widget), text);
}

void
indent_block_widget(gchar *name_widget, gpointer func, gboolean block, IndentData *idt)
{
	GtkWidget *widget;

	widget = glade_xml_get_widget(idt->xml, name_widget);
	if (block)
		g_signal_handlers_block_by_func (GTK_OBJECT (widget), func, NULL);
	else
		g_signal_handlers_unblock_by_func (GTK_OBJECT (widget), func, NULL);
}


void
on_indent_checkbutton_toggled(GtkToggleButton *button, IndentData *idt)
{
	const gchar *checkname;
	gboolean num;
	CheckData *ptrcheck;
	gchar *line;
	gchar *option;
	
	if (idt->checkbutton_blocked) return; 
	
	checkname = gtk_widget_get_name (GTK_WIDGET(button));
	if ( (ptrcheck = g_hash_table_lookup(idt->check_hash, checkname)) == NULL)
		return;
	num = ptrcheck->spinbutton ? TRUE : FALSE;
	line = indent_entry_get_chars(idt);
	line = indent_delete_option(line, ptrcheck->option, num);
	if (gtk_toggle_button_get_active(button))
	{
		option = g_strconcat("-", ptrcheck->option, NULL);
		if (ptrcheck->spinbutton)
		{
			indent_widget_set_sensitive(ptrcheck->spinbutton, TRUE, idt);
			option = g_strconcat(option, 
			            indent_spinbutton_get_value(ptrcheck->spinbutton, idt), 
			            NULL);	
		}
		line = indent_insert_option(line, option);
		g_free(option);
	}
	else
	{
		if (ptrcheck->spinbutton)
			indent_widget_set_sensitive(ptrcheck->spinbutton, FALSE, idt);
		if (ptrcheck->not_option)
		{
			option = g_strconcat("-n", ptrcheck->option, NULL);
			line = indent_insert_option(line, option);
			g_free(option);
		}
	}
	indent_block_widget(PARAMETERS_ENTRY, 
	                    GTK_SIGNAL_FUNC (on_indent_parameters_entry_changed),
                        TRUE, idt);
	indent_entry_set_chars(line, idt);
	indent_block_widget(PARAMETERS_ENTRY, 
	                    GTK_SIGNAL_FUNC (on_indent_parameters_entry_changed),
                        FALSE, idt);
	g_free(line);
}

void
on_indent_spinbutton_value_changed(GtkSpinButton *button, IndentData *idt)
{
	const gchar *spinname;
	gchar *line;
	gchar *option;
	
	if (idt->checkbutton_blocked) return;
	
	spinname = gtk_widget_get_name (GTK_WIDGET(button));
	if ( (option = g_hash_table_lookup(idt->spin_hash, spinname)) == NULL)
		return;
	line = indent_entry_get_chars(idt);
	line = indent_delete_option(line, option, TRUE);
	option = g_strconcat("-", option, 
	                     indent_spinbutton_get_value((gchar *)spinname, idt), NULL);	
	line = indent_insert_option(line, option);
	g_free(option);
	indent_block_widget(PARAMETERS_ENTRY, 
	                    GTK_SIGNAL_FUNC (on_indent_parameters_entry_changed),
                        TRUE, idt);
	indent_entry_set_chars(line, idt);
	indent_block_widget(PARAMETERS_ENTRY, 
	                    GTK_SIGNAL_FUNC (on_indent_parameters_entry_changed),
                        FALSE, idt);
	g_free(line);
}

void
on_indent_parameters_entry_changed(GtkEditable *edit, IndentData *idt)
{
	idt->checkbutton_blocked = TRUE; 
	indent_init_dialog(idt);
	indent_anal_line_option(gtk_editable_get_chars(edit, 0, -1), idt);	
	idt->checkbutton_blocked = FALSE; 
}

void
indent_display_buffer(gchar *buffer, IndentData *idt)
{
	GtkWidget *widget;
	GtkTextBuffer *text_buffer = NULL;
	GtkTextTag *tag;
	GtkTextIter start, end ;
	
	widget = glade_xml_get_widget(idt->xml, PREVIEW_TEXTVIEW);

	text_buffer = gtk_text_buffer_new(NULL);
	tag = gtk_text_buffer_create_tag(text_buffer, "police", 
	                                 "left_margin", 5, 
	                                 "font", "Courier", NULL);
	gtk_text_buffer_set_text(text_buffer, buffer, -1);
  	gtk_text_buffer_get_iter_at_offset(text_buffer, &start, 0);
  	gtk_text_buffer_get_iter_at_offset(text_buffer, &end, -1);
	gtk_text_buffer_apply_tag(text_buffer, tag, &start, &end);
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(widget), text_buffer);	
}


#define PREFS_GLADE PACKAGE_DATA_DIR"/glade/anjuta-document-manager.glade"

GtkWidget *create_dialog(IndentData *idt)
{
	idt->xml = glade_xml_new (PREFS_GLADE, DIALOG, NULL);

	if (idt->xml == NULL)
	{
		g_warning("Unable to build user interface for Indent\n");
		return NULL;
	}
	
	glade_xml_signal_autoconnect (idt->xml);
	idt->dialog = glade_xml_get_widget (idt->xml, DIALOG);
		
	indent_init_dialog(idt);
	indent_init_connect(idt);
	indent_set_style_combo(idt->style_active, idt);
	
	g_signal_connect(GTK_OBJECT(idt->dialog), "delete_event",
                     (GtkSignalFunc)indent_exit,
                     NULL);
	g_signal_connect(GTK_OBJECT(idt->dialog), "destroy",
                     (GtkSignalFunc)indent_exit,
                     NULL);
	return idt->dialog;
}


void
indent_set_style_combo(gint index, IndentData *idt)
{
	GtkWidget *widget;
	GList *list;
	IndentStyle *ist;
	
	widget = glade_xml_get_widget(idt->xml, STYLE_COMBOBOX);

	list = idt->style_list;
	while (list)
	{
		ist = list->data;
		gtk_combo_box_append_text(GTK_COMBO_BOX(widget), ist->name);
		list = g_list_next(list);
	}	
	gtk_combo_box_set_active(GTK_COMBO_BOX(widget), index);
}

void
on_indent_style_combobox_changed(GtkComboBox *combo, IndentData *idt)
{	
	gchar *style_name;
	gchar *options;
	gchar *buffer = NULL;
	
	style_name = gtk_combo_box_get_active_text(combo);
	options = indent_find_style(style_name, idt);
	if (options)
	{
		indent_entry_set_chars(options, idt);
		if (indent_execute(options, idt) == 0)
		{
			buffer = indent_get_buffer();
			indent_display_buffer(buffer, idt);
			g_free(buffer);
		}
		else
		{
			GtkWidget *message = gtk_message_dialog_new(GTK_WINDOW(idt->dialog), 
		                       GTK_DIALOG_DESTROY_WITH_PARENT,
	                           GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
		                       _("indent parameter not known !"));
			gtk_dialog_run (GTK_DIALOG (message));
			gtk_widget_destroy (message);	
		}
	}
}

/*****************************************************************************/

void
on_indent_preview_button_clicked(GtkButton *button, IndentData *idt)
{	
	gchar *line_option;
	gchar *buffer = NULL;

	line_option = indent_entry_get_chars(idt);
	if (indent_execute(line_option, idt) == 0)
	{
		buffer = indent_get_buffer();
		indent_display_buffer(buffer, idt);
		g_free(buffer);
	}
	else
		{
			GtkWidget *message = gtk_message_dialog_new(GTK_WINDOW(idt->dialog), 
		                       GTK_DIALOG_DESTROY_WITH_PARENT,
	                           GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
		                       _("indent parameter not known !"));
			gtk_dialog_run (GTK_DIALOG (message));
			gtk_widget_destroy (message);	
		}
}

void
on_indent_new_button_clicked(GtkButton *button, IndentData *idt)
{	
 	gchar *style_name;
	GtkWidget *widget;
	GtkWidget *message;
	gint index;
	
	widget = glade_xml_get_widget(idt->xml, STYLE_ENTRY);
	style_name = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
	style_name = g_strstrip(style_name);
	if  (strlen(style_name) < 1)
		return;
	widget = glade_xml_get_widget(idt->xml, STYLE_COMBOBOX);
	if (indent_add_style(style_name, idt))
	{
		gtk_combo_box_append_text(GTK_COMBO_BOX(widget), style_name);
		gtk_combo_box_append_text(GTK_COMBO_BOX(idt->pref_indent_combo), style_name);

		index = indent_find_index(style_name, idt);
		gtk_combo_box_set_active(GTK_COMBO_BOX(widget), index);
		gtk_combo_box_set_active(GTK_COMBO_BOX(idt->pref_indent_combo), index);
		
		widget = glade_xml_get_widget(idt->xml, STYLE_ENTRY);
		gtk_editable_delete_text(GTK_EDITABLE(widget), 0, -1);
		indent_save_all_style(idt);
	}
	else 
	{
		message = gtk_message_dialog_new(GTK_WINDOW(idt->dialog), 
		                       GTK_DIALOG_DESTROY_WITH_PARENT,
	                           GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
		                       _("A Style has already this name !"));
		gtk_dialog_run (GTK_DIALOG (message));
		gtk_widget_destroy (message);
	}                        
}

void
on_indent_update_button_clicked(GtkButton *button, IndentData *idt)
{	
	gchar *style_name;
	gchar *options;
	GtkWidget *widget;
	GtkWidget *message;
	
	widget = glade_xml_get_widget(idt->xml, STYLE_COMBOBOX);
	style_name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(widget));
	widget = glade_xml_get_widget(idt->xml, PARAMETERS_ENTRY);
	options = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
	
	if (indent_update_style(style_name, options, idt))
	{
		indent_save_style(style_name, options, idt);
		gtk_entry_set_text(GTK_ENTRY(idt->pref_indent_options), options);
	}
	else
	{
		message = gtk_message_dialog_new(GTK_WINDOW(idt->dialog), 
		                       GTK_DIALOG_DESTROY_WITH_PARENT,
	                           GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
		                       _("This Style is not modifiable !"));
		gtk_dialog_run (GTK_DIALOG (message));
		gtk_widget_destroy (message);
	}
}

void
on_indent_delete_button_clicked(GtkButton *button, IndentData *idt)
{	
	GtkWidget *widget;
	GtkWidget *message;
	gint index;
	gchar *style_name;
	
	widget = glade_xml_get_widget(idt->xml, STYLE_COMBOBOX);
	style_name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(widget));
	if (indent_remove_style(style_name, idt))
	{
		index = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
		gtk_combo_box_remove_text(GTK_COMBO_BOX(widget), index);
		gtk_combo_box_remove_text(GTK_COMBO_BOX(idt->pref_indent_combo), index);

		gtk_combo_box_set_active(GTK_COMBO_BOX(widget), index - 1);
		gtk_combo_box_set_active(GTK_COMBO_BOX(idt->pref_indent_combo), index - 1);
		indent_save_all_style(idt);
	}
	else
	{
		message = gtk_message_dialog_new(GTK_WINDOW(idt->dialog), 
		                       GTK_DIALOG_DESTROY_WITH_PARENT,
	                           GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
		                       _("This Style is not modifiable !"));
		gtk_dialog_run (GTK_DIALOG (message));
		gtk_widget_destroy (message);
	}
}

/*****************************************************************************/

void
on_indent_quit_button_clicked(GtkButton *button, IndentData *idt)
{	
	GtkWidget *widget;
	gchar *style_name, *options;
	gint index;
	
	widget = glade_xml_get_widget(idt->xml, STYLE_COMBOBOX);
	style_name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(widget));
	options = indent_find_style(style_name, idt);
	indent_save_active_style(style_name, options, idt);
	
	index = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
	gtk_combo_box_set_active(GTK_COMBO_BOX(idt->pref_indent_combo), index);

	gtk_widget_hide(idt->dialog);
}



void
indent_exit(GtkWidget *widget, gpointer user_data)
{
	gtk_widget_hide(widget);
}
