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
#include <iostream.h>
#include "ccview_main.h"

#define CCVIEW_METAFILE_NAME ".ccview-metafile.xml"

void CcviewMain::clear()
{
	getproj().clear();
    cpane.clear();
    fpane.clear();
}

void CcviewMain::scanning_ended(CcviewMain* cm)
{
	cm->reload();
	cout << "Message: Saving project" << endl;
	if (!cm->save_in_xml())
		cout << "Message: Savining project in xml failed." << endl;
	gtk_signal_emit (cm->object, ccview_signals[UPDATE_END], cm->getproj().getdirectory());
}

CcviewMain::CcviewMain():cpane(this),fpane(this)
{
    notebook = gtk_notebook_new();
	gtk_widget_show (notebook);
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook),GTK_POS_BOTTOM);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),cpane.get_window(),
                            gtk_label_new(_("Classes")));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),fpane.get_window(),
                             gtk_label_new(_("Files")));
	getproj().get_scanner().set_notify_end ((void(*)(void*))scanning_ended, this);
}

CcviewMain::~CcviewMain()
{
}

void CcviewMain::reload()
{
    g_message("In reloadtrees");
    cpane.reload();
    fpane.reload();
}

bool CcviewMain::save_in_xml()
{
	bool ret = true;
	string fullfilename = getproj().getdirectory();
	fullfilename += CCVIEW_METAFILE_NAME;
	ofstream str;
	str.open(fullfilename.c_str());
	if (!str) return false;
	str << "<?xml version=\"1.0\"?>\n";
	ret = proj.save_in_xml(str, 0);
	return ret;
}

bool CcviewMain::load_from_xml()
{
	xmlDoc* doc;
	bool ret = true;
	
	string fullfilename = getproj().getdirectory();
	if (fullfilename.length() <= 0) return false;
	cout << "Message: Loading from xml metafile" << endl;
	fullfilename += CCVIEW_METAFILE_NAME;
	doc = xmlParseFile (fullfilename.c_str());
	if (doc)
	{
		ret = getproj().load_from_xml(doc->root);
		xmlFreeDoc (doc);
		cout << "Message: Stripping undefined classes and functions" << endl;
		getproj().strip_undefined();
	}
	return true;
}
bool CcviewMain::update()
{
	cout << "Message: Updating directory." << endl;
	return getproj().update();
}

bool CcviewMain::setdirectory(const char* dir)
{
	bool ret;
	clear();
	getproj().setdirectory(dir);
	ret = load_from_xml();
	reload();
	return ret;
}
