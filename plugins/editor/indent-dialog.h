#ifndef _INDENT_DIALOG_H_
#define _INDENT_DIALOG_H_


void pref_set_style_combo(IndentData *idt);
void pref_style_combo_changed(GtkComboBox *combo, IndentData *idt);

void indent_toggle_button_set_active(gchar *name_widget, gboolean active, IndentData *idt);
void indent_widget_set_sensitive(gchar *name_widget, gboolean sensitive, IndentData *idt);
void indent_spinbutton_set_value(gchar *name_widget, gchar *num, IndentData *idt);

GtkWidget *create_dialog(IndentData *idt);

#endif
