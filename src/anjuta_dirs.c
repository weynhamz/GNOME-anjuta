/*
    anjuta_dirs.c
    Copyright (C) 2000  Kh. Naba Kumar Singh

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/stat.h>
#include <gnome.h>
#include "anjuta_dirs.h"
#include "utilities.h"
#include "resources.h"

AnjutaDirs*
anjuta_dirs_new()
{
	AnjutaDirs *ad;
	ad = g_malloc (sizeof (AnjutaDirs));
	if (ad)
	{
		ad->tmp = g_strdup (g_get_tmp_dir ());
		ad->data = anjuta_res_get_data_dir ();
		ad->pixmaps = anjuta_res_get_pixmap_dir ();
		ad->help = anjuta_res_get_help_dir ();
		ad->doc = anjuta_res_get_doc_dir ();
		ad->home = g_strdup (g_get_home_dir ());
		ad->settings = g_strconcat (ad->home, "/.anjuta" PREF_SUFFIX, NULL);
		ad->first_time = FALSE;

		if (file_is_directory (ad->settings) == FALSE)
		{
			ad->first_time = TRUE;
			if (mkdir (ad->settings, 0755) != 0)
			{
				g_warning (_("Unable to create settings directory.\n"));
			}
		}
	}
	return ad;
}

void
anjuta_dirs_destroy( AnjutaDirs *ad)
{
   if(ad)
   {
      if(ad->data) g_free(ad->data);
      if(ad->pixmaps) g_free(ad->pixmaps);
      if(ad->doc) g_free(ad->doc);
      if(ad->help) g_free(ad->help);
      if(ad->tmp) g_free(ad->tmp);
      if(ad->home) g_free(ad->home);
      if(ad->settings) g_free(ad->settings);
      g_free (ad);
   }
}
