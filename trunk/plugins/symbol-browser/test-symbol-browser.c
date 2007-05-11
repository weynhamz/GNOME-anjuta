
#include "an_symbol_view.h"

int
main (int argc, char *argv[]) {
	/*/
	GtkWidget *sv;
	GtkWidget *sw;
	GtkWidget *win;
	
	gtk_init (&argc, &argv);
	
	win = gtk_window_new (GTK_WINDOW_TOP_LEVEL);
	gtk_window_set_default_size (GTK_WINDOW (win), 150, 500);
	
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (win), sw);
	
	sv = anjuta_symbol_view_new ();
	anjuta_symbol_view_open ("../../");
	gtk_container_add (GTK_CONTAINER (sw), sv);
	gtk_widget_show_all (win);
	
	gtk_main();
	/*/
}
