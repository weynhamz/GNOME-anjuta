/*
 * anjuta-tabber-test.c
 *
 * Copyright (C) 2011 - Johannes Schmid
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */


#include <libanjuta/anjuta-tabber.h>

int main (int argc, char** argv)
{
	GtkWidget* window;
	GtkWidget* tabber;
	GtkWidget* notebook;
	GtkWidget* box;
	
	gtk_init(&argc, &argv);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), "tabber-test");

	box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
	notebook = gtk_notebook_new ();
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK(notebook), FALSE);
	tabber = anjuta_tabber_new (GTK_NOTEBOOK(notebook));
	gtk_box_pack_start (GTK_BOX(box), tabber, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX(box), notebook, TRUE, TRUE, 0);
	
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
	                          gtk_label_new ("First"),
	                          gtk_label_new ("First page"));
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
	                          gtk_label_new ("Second"),
	                          gtk_label_new ("Second page"));
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
	                          gtk_label_new ("Third"),
	                          gtk_label_new ("Third page"));

	anjuta_tabber_add_tab (ANJUTA_TABBER (tabber), 
	                       gtk_label_new ("First"));
	anjuta_tabber_add_tab (ANJUTA_TABBER (tabber), 
	                       gtk_label_new ("Second"));
	anjuta_tabber_add_tab (ANJUTA_TABBER (tabber), 
	                       gtk_label_new ("Third"));
	
	gtk_container_add (GTK_CONTAINER (window), box);
	gtk_widget_show_all (window);

	g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
	gtk_main ();

	return 0;
}