#ifndef _SEARCH_PREFERENCES_H
#define _SEARCH_PREFERENCES_H

#ifdef __cplusplus
extern "C"
{
#endif
	
	
gboolean
search_preferences_save_yourself (FILE *stream);
	
void
on_setting_pref_add_clicked(GtkButton *button, gpointer user_data);

void
on_setting_pref_remove_clicked(GtkButton *button, gpointer user_data);

void
on_setting_pref_modify_clicked(GtkButton *button, gpointer user_data);

void
on_setting_pref_activate_clicked(GtkButton *button, gpointer user_data);

void
search_preferences_initialize_setting_treeview(GtkWidget *dialog);


#ifdef __cplusplus
}
#endif

#endif /* _SEARCH_PREFERENCES_H */
