/*
 * project_dbase.c
 * Copyright (C) 2000  Kh. Naba Kumar Singh
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>

#include <gnome.h>
#include "anjuta.h"
#include "project_dbase.h"
#include "mainmenu_callbacks.h"
#include "utilities.h"
#include "messagebox.h"
#include "fileselection.h"
#include "resources.h"
#include "controls.h"
#include "utilities.h"
#include "properties.h"
#include "source.h"
#include "glade_iface.h"
#include "compatibility_0.h"
#include "defaults.h"
#include "ccview.h"
#include "glades.h"
#include "CORBA-Server.h"
#include "debugger.h"
#include "find_replace.h"
#include "executer.h"

/* Including small pixmaps at compile time */
/* So that these at least work when gnome pixmaps are not found */
#include "../pixmaps/bfoldo.xpm"
#include "../pixmaps/bfoldc.xpm"
#include "../pixmaps/file_file.xpm"
#include "../pixmaps/file_c.xpm"
#include "../pixmaps/file_cpp.xpm"
#include "../pixmaps/file_h.xpm"
#include "../pixmaps/file_pix.xpm"
#include "../pixmaps/file_icon.xpm"
#include "../pixmaps/file_html.xpm"
#include "../pixmaps/file_i18n.xpm"

// To debug it
#define	SHOW_LOCALS_DEFAULT 	TRUE

static void
project_reload_session_files(ProjectDBase * p);
static void
project_dbase_save_session (ProjectDBase * p);
static void
project_dbase_reload_session (ProjectDBase * p);
static void
project_dbase_save_session_files (ProjectDBase * p);
static void
project_dbase_save_markers( ProjectDBase * p, TextEditor *te, const gint nItem );

static const gchar szShowLocalsItem[] = { "ShowLocals" };

static GdkPixmap *opened_folder_pix,
	*closed_folder_pix,
	*file_h_pix, *file_c_pix, *file_cpp_pix, *file_pix_pix,
	*file_file_pix, *file_icon_pix, *file_html_pix,
	*file_i18n_pix;

static GdkBitmap *opened_folder_mask,
	*closed_folder_mask,
	*file_h_mask, *file_c_mask, *file_cpp_mask, *file_pix_mask,
	*file_file_mask, *file_icon_mask, *file_html_mask,
	*file_i18n_mask;

gchar *project_type_map[]=
{
	"GENERIC",
	"GTK",
	"GNOME",
	"BONOBO",
	"GTK--",
	"GNOME--",
	NULL
};

gchar *project_target_type_map[]=
{
	"EXECUTABLE",
	"STATIC_LIB",
	"DYNAMIC_LIB",
	NULL
};

gchar *module_map[]=
{
	"incude",
	"source",
	"pixmap",
	"data",
	"help",
	"doc",
	"po",
	NULL
};

gchar *programming_language_map[]=
{
	"C",
	"C++",
	"C_C++",
	NULL
};

static gchar* default_module_name[]=
{
	"include", "src", "pixmaps", "data", "help", "doc", "po", NULL
};

static gchar* default_module_type[]=
{
	"",  "", "", "", "", "", "", NULL
};

void
get_pixmask_on_file_ext (FileExtType t, GdkPixmap ** p_c,
			 GdkBitmap ** m_c, GdkPixmap ** p_o,
			 GdkBitmap ** m_o);

int select_only_file (const struct dirent *e);

static void
get_pixmask_on_file_type (FileExtType t, GtkWidget * parent,
			  GdkPixmap ** p_c, GdkBitmap ** m_c,
			  GdkPixmap ** p_o, GdkBitmap ** m_o)
{
	switch (t)
	{
	case FILE_TYPE_DIR:
		*p_c = closed_folder_pix;
		*m_c = closed_folder_mask;
		*p_o = opened_folder_pix;
		*m_o = opened_folder_mask;
		return;

	case FILE_TYPE_C:
		*p_c = file_c_pix;
		*m_c = file_c_mask;
		*p_o = file_c_pix;
		*m_o = file_c_mask;
		return;

	case FILE_TYPE_CPP:
		*p_c = file_cpp_pix;
		*m_c = file_cpp_mask;
		*p_o = file_cpp_pix;
		*m_o = file_cpp_mask;
		return;

	case FILE_TYPE_HEADER:
		*p_c = file_h_pix;
		*m_c = file_h_mask;
		*p_o = file_h_pix;
		*m_o = file_h_mask;
		return;

	case FILE_TYPE_IMAGE:
		*p_c = file_pix_pix;
		*m_c = file_pix_mask;
		*p_o = file_pix_pix;
		*m_o = file_pix_mask;
		return;

	case FILE_TYPE_ICON:
		*p_c = file_icon_pix;
		*m_c = file_icon_mask;
		*p_o = file_icon_pix;
		*m_o = file_icon_mask;
		return;
	
	case FILE_TYPE_HTML:
		*p_c = file_html_pix;
		*m_c = file_html_mask;
		*p_o = file_html_pix;
		*m_o = file_html_mask;
		return;

	case FILE_TYPE_PO:
		*p_c = file_i18n_pix;
		*m_c = file_i18n_mask;
		*p_o = file_i18n_pix;
		*m_o = file_i18n_mask;
		return;

	default:
		*p_c = file_file_pix;
		*m_c = file_file_mask;
		*p_o = file_file_pix;
		*m_o = file_file_mask;
		return;
	}
}

static void
gtree_insert_files (GtkWidget * ctree, GtkCTreeNode * parent,
		    PrjModule mod,  gchar * dir_prefix, GList * items)
{
	GList *node;
	gchar *dum_array[1];

	GdkPixmap *pix_c;
	GdkPixmap *pix_o;
	GdkBitmap *mask_c;
	GdkBitmap *mask_o;


	node = g_list_sort (items, compare_string_func);
	while (node)
	{
		FileExtType ftype;
		gchar *full_fname;
		ProjectFileData *pfd;
		GtkCTreeNode *cnode;

		if (node->data == NULL)
			continue;
		ftype = get_file_ext_type (node->data);
		get_pixmask_on_file_type (ftype,
					  app->project_dbase->widgets.window,
					  &pix_c, &mask_c, &pix_o, &mask_o);
		dum_array[0] = node->data;
		if (ftype == FILE_TYPE_HEADER)
			cnode = gtk_ctree_insert_node (GTK_CTREE (ctree),
						       parent, NULL,
						       dum_array, 5, pix_c,
						       mask_c, pix_o, mask_o,
						       TRUE, FALSE);
		else
			cnode =
				gtk_ctree_insert_node (GTK_CTREE (ctree),
						       parent, NULL,
						       dum_array, 5, pix_c,
						       mask_c, pix_o, mask_o,
						       TRUE, FALSE);
		full_fname = g_strconcat (dir_prefix, "/", node->data, NULL);
		pfd =
			project_file_data_new (parent, mod, full_fname);
		g_free (full_fname);
		gtk_ctree_node_set_row_data_full (GTK_CTREE (ctree),
						  GTK_CTREE_NODE (cnode), pfd,
						  (GtkDestroyNotify)
						  project_file_data_destroy);
		node = g_list_next (node);
	}
}

ProjectFileData *
project_file_data_new (GtkCTreeNode * parent, PrjModule mod, gchar * full_fname)
{
	ProjectFileData *pfd;

	pfd = g_malloc (sizeof (ProjectFileData));
	if (!pfd)
		return NULL;

	pfd->module = mod;
	pfd->parent_node = parent;
	pfd->filename = NULL;
	if (full_fname)
		pfd->filename = g_strdup (full_fname);
	return pfd;
}

void
project_file_data_destroy (ProjectFileData * pfd)
{
	g_return_if_fail (pfd != NULL);
	if (pfd->filename)
		g_free (pfd->filename);
	g_free (pfd);
}

ProjectDBase *
project_dbase_new (PropsID pr_props)
{
	ProjectDBase *p;
	gint i;
	
	/* Must declare static, because it will be used forever */
	static FileSelData fsd1 = { N_("Open Project"), NULL,
		on_open_prjfilesel_ok_clicked,
		on_open_prjfilesel_cancel_clicked,
		NULL
	};

	/* Must declare static, because it will be used forever */
	static FileSelData fsd2 = { N_("Add To Project"), NULL,
		on_add_prjfilesel_ok_clicked,
		on_add_prjfilesel_cancel_clicked,
		NULL
	};

	p = g_malloc (sizeof (ProjectDBase));
	if (!p) return NULL;

	/* Cannot assign p to static fsd.data, so doing it now */
	fsd1.data = p;
	fsd2.data = p;
	create_project_dbase_gui (p);
	gtk_window_set_title (GTK_WINDOW (p->widgets.window),
			      _("Project: None"));
	p->fileselection_open = create_fileselection_gui (&fsd1);
	p->fileselection_add_file = create_fileselection_gui (&fsd2);
	p->project_config = project_config_new ();

	p->widgets.root_node = NULL;
	p->widgets.current_node = NULL;
	for(i=0; i<MODULE_END_MARK; i++)
	{
		p->widgets.module_node[i] = NULL;
	}
	p->m_prj_ShowLocal = SHOW_LOCALS_DEFAULT ;
	p->project_is_open = FALSE;
	p->is_showing = TRUE;
	p->is_docked = TRUE;
	p->win_pos_x = 100;
	p->win_pos_y = 80;
	p->win_width = 200;
	p->win_height = 400;
	p->top_proj_dir = NULL;
	p->current_file_data = NULL;
	p->sel_module = MODULE_SOURCE;
	p->props = prop_set_new ();
	prop_set_parent (p->props, pr_props);

	closed_folder_pix =
		gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gtk_widget_get_colormap(p->widgets.window),
			&closed_folder_mask, NULL, bfoldc_xpm);
	opened_folder_pix =
		gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gtk_widget_get_colormap(p->widgets.window),
			&opened_folder_mask, NULL, bfoldo_xpm);
	file_h_pix =
		gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gtk_widget_get_colormap(p->widgets.window),
			&file_h_mask, NULL, file_h_xpm);
	file_c_pix =
		gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gtk_widget_get_colormap(p->widgets.window),
			&file_c_mask, NULL, file_c_xpm);
	file_cpp_pix =
		gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gtk_widget_get_colormap(p->widgets.window),
			&file_cpp_mask, NULL, file_cpp_xpm);
	file_pix_pix =
		gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gtk_widget_get_colormap(p->widgets.window),
			&file_pix_mask, NULL, file_pix_xpm);
	file_file_pix =
		gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gtk_widget_get_colormap(p->widgets.window),
			&file_file_mask, NULL, file_file_xpm);
	file_icon_pix =
		gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gtk_widget_get_colormap(p->widgets.window),
			&file_icon_mask, NULL, file_icon_xpm);
	file_html_pix =
		gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gtk_widget_get_colormap(p->widgets.window),
			&file_html_mask, NULL, file_html_xpm);
	file_i18n_pix =
		gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gtk_widget_get_colormap(p->widgets.window),
			&file_i18n_mask, NULL, file_i18n_xpm);
	return p;
}

void
project_dbase_destroy (ProjectDBase * p)
{
	g_return_if_fail (p != NULL);
	project_dbase_clear (p);
	
	project_config_destroy (p->project_config);
	
	gtk_widget_unref (p->widgets.window);
	gtk_widget_unref (p->widgets.ccview);
	gtk_widget_unref (p->widgets.client_area);
	gtk_widget_unref (p->widgets.client);
	gtk_widget_unref (p->widgets.scrolledwindow);
	gtk_widget_unref (p->widgets.ctree);
	gtk_widget_unref (p->widgets.menu);
	gtk_widget_unref (p->widgets.menu_import);
	gtk_widget_unref (p->widgets.menu_view);
	gtk_widget_unref (p->widgets.menu_edit);
	gtk_widget_unref (p->widgets.menu_remove);
	gtk_widget_unref (p->widgets.menu_configure);
	gtk_widget_unref (p->widgets.menu_info);
	if (p->widgets.window)
		gtk_widget_destroy (p->widgets.window);

	if (p->fileselection_open)
		gtk_widget_destroy (p->fileselection_open);
	if (p->fileselection_add_file)
		gtk_widget_destroy (p->fileselection_add_file);
	g_free (p);
}


static void
project_dbase_clear_ctree (ProjectDBase * p)
{
	gint i;

	if (!p) return;
	if (!p->widgets.root_node) return;

	for (i=0; i<MODULE_END_MARK; i++)
	{
		gchar *filename, *key, buff[256];
		GdkPixmap *pixc, *pixo;
		GdkBitmap *maskc, *masko;
		gint8 space;
		gboolean is_leaf, expanded;
		
		if (p->widgets.module_node[i] == NULL) continue;
		gtk_ctree_get_node_info (GTK_CTREE (p->widgets.ctree),
					 p->widgets.module_node[i],
					 &filename, &space,
					 &pixc, &maskc, &pixo, &masko, &is_leaf,
					 &expanded);
		key = g_strconcat ("module.", module_map[i], ".expanded", NULL);
		sprintf (buff, "%d", expanded);
		prop_set_with_key (p->props, key, buff);
		g_free (key);
		p->widgets.module_node[i] = NULL;
	}
	gtk_ctree_remove_node (GTK_CTREE (p->widgets.ctree),
			       p->widgets.root_node);
	p->widgets.root_node = NULL;
	session_sync();
}

void
project_dbase_clear (ProjectDBase * p)
{
	if (!p) return;
		
	project_dbase_clear_ctree (p);
	string_assign (&p->top_proj_dir, NULL);
	string_assign (&p->proj_filename, NULL);
	prop_clear (p->props);
	gtk_window_set_title (GTK_WINDOW (p->widgets.window),
			      _("Project: None"));
	p->project_is_open = FALSE;
	p->is_saved = TRUE;
	p->m_prj_ShowLocal	= SHOW_LOCALS_DEFAULT ;
	extended_toolbar_update ();
}

void
project_dbase_show (ProjectDBase * p)
{
	if (p)
	{
		if (p->is_showing)
		{
			if (p->is_docked == FALSE)
				gdk_window_raise (p->widgets.window->window);
			return;
		}
		if (p->is_docked)
		{
			project_dbase_attach (p);
		}
		else		/* Is not docked */
		{
			gtk_widget_set_uposition (p->widgets.window,
						  p->win_pos_x, p->win_pos_y);
			gtk_window_set_default_size (GTK_WINDOW
						     (p->widgets.window),
						     p->win_width,
						     p->win_height);
			gtk_widget_show (p->widgets.window);
		}
		p->is_showing = TRUE;
		gtk_widget_grab_focus (p->widgets.ctree);
	}
}

void
project_dbase_hide (ProjectDBase * p)
{
	if (p)
	{
		if (p->is_showing == FALSE)
			return;
		if (p->is_docked == TRUE)
		{
			project_dbase_detach (p);
		}
		else		/* Is not docked */
		{
			gdk_window_get_root_origin (p->widgets.window->window,
						    &p->win_pos_x,
						    &p->win_pos_y);
			gdk_window_get_size (p->widgets.window->window,
					     &p->win_width, &p->win_height);
			gtk_widget_hide (p->widgets.window);
		}
		p->is_showing = FALSE;
	}
}

void
project_dbase_open_project (ProjectDBase * p)
{
	gchar *all_projects_dir;

	all_projects_dir =
		preferences_get (app->preferences, PROJECTS_DIRECTORY);
	chdir (all_projects_dir);
	
	fileselection_set_dir (p->fileselection_open, all_projects_dir);
	gtk_widget_show (p->fileselection_open);
	g_free (all_projects_dir);
}


/* Loads the default project */
static void
load_default_project (ProjectDBase* p)
{
	gchar* temp, *key;
	gint i;

	/* Read Default project, so that system remains sane */
	temp = malloc (strlen (default_project)+100);
	sprintf (temp, default_project, VERSION, COMPATIBILITY_LEVEL);
	prop_read_from_memory (p->props, temp, strlen (temp), "");
	g_free (temp);
	
	prop_set_with_key (p->props, "props.file.type", "project");
	prop_set_with_key (p->props, "anjuta.version", VERSION);
	temp = g_strdup_printf("%d", COMPATIBILITY_LEVEL);
	prop_set_with_key (p->props, "anjuta.compatibility.level", temp);
	g_free (temp);
	prop_set_with_key (p->props, "project.menu.group", "Applications");
	prop_set_with_key (p->props, "project.menu.comment", "No comment");
	prop_set_with_key (p->props, "project.menu.need.terminal", "1");

	/* Set all module names to their defaults */
	for (i=0; i<MODULE_END_MARK; i++)
	{
		key = g_strconcat ("module.", module_map[i], ".name", NULL);
		prop_set_with_key (p->props, key, default_module_name[i]);
		g_free (key);
	}
	/* Set all module types to their defaults */
	for (i=0; i<MODULE_END_MARK; i++)
	{
		key = g_strconcat ("module.", module_map[i], ".type", NULL);
		prop_set_with_key (p->props, key, default_module_type[i]);
		g_free (key);
	}
}

/* Reads using buffer prj_buff from location loc */
static gboolean
load_from_buffer (ProjectDBase* p, gchar* prj_buff, gint loc)
{
	gchar* temp;
	gint tmp;

	load_default_project(p);

	/* Read actual project */
	prop_read_from_memory (p->props, &prj_buff[loc], strlen (&prj_buff[loc]), "");

	/* Settings that should remain unchanged for all projects */
	prop_set_with_key (p->props, "module.po.name", "po");
	prop_set_with_key (p->props, "module.macro.name", "macros");
	
	temp = prop_get (p->props, "props.file.type");
	if (!temp) /* Avoiding seg-fault */
		temp = g_strdup (" ");
	if (strcmp (temp, "project") != 0)
	{
		anjuta_error (_("Couldn't load project: %s"), p->proj_filename);
		g_free (temp);
		return FALSE;
	}
	g_free (temp);
	tmp = prop_get_int (p->props, "anjuta.compatibility.level", 0);
	if (tmp < COMPATIBILITY_LEVEL)
	{
		gchar *ver;
		ver = prop_get (p->props, "anjuta.version");
		anjuta_error(_
					 ("Error: You need Anjuta version %s or later to open this project.\n"
					  "Please upgrade Anjuta to the latest version (Help for more information).\n"
					  "For the time being, I am too old to load it.:-("), ver);
		g_free (ver);
		return FALSE;
	}
	if (project_config_load(p->project_config, p->props)== FALSE)
	{
		anjuta_error (_("Couldn't load project: %s"), p->proj_filename);
		return FALSE;
	}
	return TRUE;
}

#if 0
static void
project_reload_session_files_v(ProjectDBase * p)
{
	gint	nItems, i ;
	gchar** argvp ;
	
	g_return_if_fail( p != NULL );

	session_get_strings( p, SECSTR(SECTION_FILELIST), &nItems, &argvp );
	for( i = 0 ; i < nItems ; i ++ )
	{
		if( argvp[i] )
		{
			anjuta_goto_file_line (argvp[i], -1);
		}
		g_free( argvp[i] );
	}
}
#endif

static void
project_reload_session_files(ProjectDBase * p)
{
	gpointer	config_iterator;
	g_return_if_fail( p != NULL );

	config_iterator = session_get_iterator( p, SECSTR(SECTION_FILELIST) );
	if ( config_iterator !=  NULL )
	{
		gchar * szItem, *szFile;
		while ((config_iterator = gnome_config_iterator_next( config_iterator,
									&szItem, &szFile )))
		{
			// shouldn't happen, but I'm paranoid
			if( ( NULL != szFile ) && ( NULL != szItem ) )
			{
				gint nItem = atoi(szItem);
				/* Read the line number */
				glong	gline = session_get_long_n( p, SECSTR(SECTION_FILENUMBER), nItem, -1 );
				gchar	*szMarkers = session_get_string_n( p, SECSTR(SECTION_FILEMARKERS), nItem, "" );
				TextEditor * te = anjuta_goto_file_line ( szFile, gline );
				/*printf( "%d:%s %d id:%ld (%s)\n", nItem, szFile, (int)gline, (long)te->editor_id, szMarkers );*/
				if( ( NULL != szMarkers ) && ( NULL != te ) )
				{
					/* a simple data parsing */
					gchar	*szData = szMarkers ;
					gchar	*szEnd ;
					while( (szEnd = strchr( szData, ',' ) ) != NULL )
					{
						gint	nLine = atoi( szData );
						if( nLine >= 0 )
						{
							aneditor_command (te->editor_id, ANE_BOOKMARK_TOGGLE_LINE, nLine, 0);
							/*printf( "To Editor %ld %d\n", (long)te->editor_id, nLine );*/
						}
						szData = szEnd + 1;
					}
				}
				g_free( szMarkers );
			}
			g_free( szItem );
			g_free( szFile );
		}
	}
}

static void
project_dbase_reload_session (ProjectDBase * p)
{
	g_return_if_fail( NULL != p );
	debugger_reload_session_breakpoints(p);	
	project_reload_session_files(p);

	find_replace_load_session( app->find_replace, p );
	executer_load_session( app->executer, p );
	find_in_files_load_session( app->find_in_files, p );
	p->m_prj_ShowLocal = session_get_bool( p, SECSTR(SECTION_PROJECTDBASE), szShowLocalsItem, SHOW_LOCALS_DEFAULT );
	
	/* Updates the menu */
	gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM (&app->widgets.menubar.view.show_hide_locals), p->m_prj_ShowLocal );
}


gboolean
project_dbase_load_project (ProjectDBase * p, gboolean show_project)
{
	gchar *prj_buff, buff[512], *str;
	gint level, read_size, pos;
	gboolean error_shown, syserr;
	FILE* fp;

	anjuta_set_busy ();
	prj_buff = NULL;
	fp = NULL;
	error_shown = FALSE;
	syserr = FALSE;
	pos = 0;
	
	if (p->project_is_open == TRUE)
		project_dbase_clean_left (p);

	p->project_is_open = TRUE;
	p->proj_filename = fileselection_get_filename (p->fileselection_open);

	/* Doing some check before actual loading */
	fp = fopen (p->proj_filename, "r");
	if (fp == NULL)
	{
		syserr = TRUE;
		goto go_error;
	}
	
	if (fscanf (fp, "# Anjuta Version %s \n", buff) < 1)
	{
		syserr = TRUE;
		goto go_error;
	}
	if (fscanf (fp, "Compatibility Level: %d\n", &level) < 1)
	{
		syserr = TRUE;
		goto go_error;
	}
	if (level < COMPATIBILITY_LEVEL)
	{
		switch (level)
		{
		case 0:
			load_default_project (p);
			compatibility_0_load_project (p, fp);
			fclose (fp);
			goto done;
		default:
			anjuta_error ("Unknown compatibility level of the project");
			error_shown = TRUE;
			goto go_error;
		}
	}
	if (level > COMPATIBILITY_LEVEL)
	{
		anjuta_error (_
		 ("You need Anjuta version %s or later to "
		"open this project.\n"
		"Please upgrade Anjuta to the latest version "
		"(Help for more information).\n"
		"For the time being, I am too old to load it. :-("), buff);
		error_shown = TRUE;
		goto go_error;
	}

	pos = ftell (fp);
	fclose (fp);
	fp = NULL;
	/* Checks end here */

	prj_buff = get_file_as_buffer (p->proj_filename);
	if (prj_buff == NULL)
	{
		syserr = TRUE;
		goto go_error;
	}
	if (pos > strlen(prj_buff))
	{
		syserr = TRUE;
		goto go_error;
	}
	read_size = project_config_load_scripts (p->project_config, &prj_buff[pos]);
	if (read_size < 0)
		goto go_error;
	if (load_from_buffer(p, prj_buff, pos+read_size) == FALSE)
	{
		error_shown = TRUE;
		goto go_error;
	}
	g_free (prj_buff);
	prj_buff = NULL;

done:
	/* It is necessary to transfer this variable */
	/* from prj props to preferences props */
	str = prop_get (p->props, "anjuta.program.parameters");
	if (str)
		prop_set_with_key (app->preferences->props,
			"anjuta.program.parameters", str);
	else
		prop_set_with_key (app->preferences->props,
			"anjuta.program.parameters", "");
	string_free (str);		
	p->is_saved = TRUE;
	p->top_proj_dir = g_dirname (p->proj_filename);
	compiler_options_load (app->compiler_options, p->props);
	src_paths_load (app->src_paths, p->props);
	ccview_project_set_directory (CCVIEW_PROJECT(p->widgets.ccview),
		p->top_proj_dir);
	/* Project loading completed */

	/* Now Project setup */
	project_dbase_update_tree (p);
	extended_toolbar_update ();
	tags_manager_load (app->tags_manager);
	project_dbase_update_tags_image(p);
	anjuta_status (_("Project loaded successfully."));
	anjuta_set_active ();
	if (show_project)
		project_dbase_show (p);
	project_dbase_reload_session(p);
	if( IsGladen() )
		project_dbase_summon_glade ( p );
	return TRUE;

go_error:
	prop_clear (p->props);
	if (!error_shown) /* If error is not yet shown */
	{
		if (syserr)
		{
			anjuta_system_error (errno, _("Error in loading Project: %s"), p->proj_filename);
		}
		else
		{
			anjuta_error (_("Error in loading Project: %s"), p->proj_filename);
		}
	}
	string_assign (&p->proj_filename, NULL);
	p->proj_filename = NULL;
	p->project_is_open = FALSE;
	if (prj_buff) g_free (prj_buff);
	if (fp) fclose (fp);
	anjuta_set_active ();
	return FALSE;
}

gboolean
project_dbase_save_project (ProjectDBase * p)
{
	gchar* str;
	FILE *fp;
	gint i;

	str = NULL;
	tags_manager_save(app->tags_manager);
/*	if (p->is_saved) return TRUE;*/
	anjuta_set_busy ();
	if (p->project_is_open == FALSE)
	{
		anjuta_set_active ();
		return TRUE;
	}
	str = g_strdup_printf ("%s.bak", p->proj_filename);
	rename (p->proj_filename, str);
	g_free (str); str = NULL;

	fp = fopen (p->proj_filename, "w");
	if (fp == NULL)
		goto error_show;
	if (fprintf (fp, "# Anjuta Version %s \n", VERSION) < 1)
		goto error_show;
	if (fprintf (fp, "Compatibility Level: %d \n\n", COMPATIBILITY_LEVEL) < 1)
		goto error_show;

	if (project_config_save_scripts (p->project_config, fp)== FALSE)
		goto error_show;

	fprintf (fp, "props.file.type=project\n\n");
	if (fprintf (fp, "anjuta.version=%s\n", VERSION) < 1)
		goto error_show;
	if (fprintf (fp, "anjuta.compatibility.level=%d\n\n", COMPATIBILITY_LEVEL) < 1)
		goto error_show;

	str = prop_get (p->props, "project.name");
	if (!str) str = g_strdup ("Unkown_Project");
	if (fprintf (fp, "project.name=%s\n", str) < 1)
		goto error_show;
	g_free (str); str = NULL;

	str = prop_get (p->props, "project.type");
	if (!str) str = g_strdup ("GNOME");
	if (fprintf (fp, "project.type=%s\n", str) < 1)
		goto error_show;
	g_free (str); str = NULL;

	str = prop_get (p->props, "project.target.type");
	if (!str) str = g_strdup ("EXECUTABLE");
	if (fprintf (fp, "project.target.type=%s\n", str) < 1)
		goto error_show;
	g_free (str); str = NULL;

	str = prop_get (p->props, "project.version");
	if (!str) str = g_strdup ("0.1");
	if (fprintf (fp, "project.version=%s\n", str) < 1)
		goto error_show;
	g_free (str); str = NULL;

	str = prop_get (p->props, "project.author");
	if (!str) str = g_strdup ("Unknown author");
	if (fprintf (fp, "project.author=%s\n", str) < 1)
		goto error_show;
	g_free (str); str = NULL;

	str = prop_get (p->props, "project.source.target");
	if (!str) str = g_strdup ("dummytarget");
	if (fprintf (fp, "project.source.target=%s\n\n", str) < 1)
		goto error_show;
	g_free (str); str = NULL;

	str = prop_get (p->props, "project.has.gettext");
	if (!str) str = g_strdup ("1");
	if (fprintf (fp, "project.has.gettext=%s\n\n", str) < 1)
		goto error_show;
	g_free (str); str = NULL;

	str = prop_get (p->props, "project.programming.language");
	if (!str) str = g_strdup (programming_language_map[PROJECT_PROGRAMMING_LANGUAGE_C]);
	if (fprintf (fp, "project.programming.language=%s\n\n", str) < 1)
		goto error_show;
	g_free (str); str = NULL;

	str = prop_get (p->props, "project.menu.entry");
	if (!str) str = prop_get (p->props, "project.name");
	if (!str) str = g_strdup ("Unkown project");
	if (fprintf (fp, "project.menu.entry=%s\n", str) < 1)
		goto error_show;
	g_free (str); str = NULL;

	str = prop_get (p->props, "project.menu.group");
	if (!str) str = g_strdup ("Applications");
	if (fprintf (fp, "project.menu.group=%s\n", str) < 1)
		goto error_show;
	g_free (str); str = NULL;

	str = prop_get (p->props, "project.menu.comment");
	if (!str) str = g_strdup ("No Comment");
	if (fprintf (fp, "project.menu.comment=%s\n", str) < 1)
		goto error_show;
	g_free (str); str = NULL;

	str = prop_get (p->props, "project.menu.icon");
	if (!str) str = g_strdup ("");
	if (fprintf (fp, "project.menu.icon=%s\n", str) < 1)
		goto error_show;
	g_free (str); str = NULL;

	str = prop_get (p->props, "project.menu.need.terminal");
	if (!str) str = g_strdup ("1");
	if (fprintf (fp, "project.menu.need.terminal=%s\n\n", str) < 1)
		goto error_show;
	g_free (str); str = NULL;

	str = prop_get (p->props, "project.configure.options");
	if (!str) str = g_strdup ("");
	if (fprintf (fp, "project.configure.options=%s\n\n", str) < 1)
		goto error_show;
	g_free (str); str = NULL;

	/* Yes, from the preferences */
	str = prop_get (app->preferences->props, "anjuta.program.arguments");
	if (!str) str = g_strdup ("");
	if (fprintf (fp, "anjuta.program.arguments=%s\n\n", str) < 1)
		goto error_show;
	g_free (str); str = NULL;

	if (project_config_save (p->project_config, fp)== FALSE)
		goto error_show;

	for(i=0; i<MODULE_END_MARK; i++)
	{
		gchar *key;
		GList *files, *node;
		gchar *filename;
		GdkPixmap *pixc, *pixo;
		GdkBitmap *maskc, *masko;
		gint8 space;
		gboolean is_leaf, expanded;

		if (i != MODULE_PO)
		{
			key = g_strconcat ("module.", module_map[i], ".name", NULL);
			str = prop_get (p->props, key);
			if (!str)
				str = g_strdup (default_module_name[i]);
			if (fprintf (fp, "%s=%s\n", key, str) < 1)
			{
				g_free (key);
				goto error_show;
			}
			g_free (str); str = NULL;
			g_free (key);
			
			key = g_strconcat ("module.", module_map[i], ".type", NULL);
			str = prop_get (p->props, key);
			if (!str)
				str = g_strdup (default_module_type[i]);
			if (fprintf (fp, "%s=%s\n", key, str) < 1)
			{
				g_free (key);
				goto error_show;
			}
			g_free (str); str = NULL;
			g_free (key);
		}
		
		key = g_strconcat ("module.", module_map[i], ".expanded", NULL);
		if (p->widgets.module_node[i])
			gtk_ctree_get_node_info (GTK_CTREE (p->widgets.ctree),
				 GTK_CTREE_NODE (p->widgets.module_node[i]),
				 &filename, &space,
				 &pixc, &maskc, &pixo, &masko, &is_leaf,
				 &expanded);
		else
			expanded = FALSE;
		if (fprintf (fp, "%s=%d\n", key, (int)expanded) < 1)
		{
			g_free (key);
			goto error_show;
		}
		g_free (key);
		
		key = g_strconcat ("module.", module_map[i], ".files", NULL);
		files = glist_from_data (p->props, key);
		if (fprintf (fp, "%s=", key) < 1)
		{
			g_free (key);
			goto error_show;
		}
		g_free (key);
		node = files;
		while (node)
		{
			if (node->data)
			{
				if (fprintf (fp, "\\\n\t%s", (gchar*)node->data) < 1)
				{
					glist_strings_free (files);
					goto error_show;
				}
			}
			node = g_list_next (node);
		}
		glist_strings_free (files);
		fprintf (fp, "\n\n");
	}
	if (compiler_options_save (app->compiler_options, fp) == FALSE)
		goto error_show;
	if (src_paths_save (app->src_paths, fp) == FALSE)
		goto error_show;
	tags_manager_save (app->tags_manager);
	ccview_project_save(CCVIEW_PROJECT(app->project_dbase->widgets.ccview));
	p->is_saved = TRUE;
	fclose (fp);
	source_write_build_files (p);
	anjuta_status (_("Project Saved Successfully"));
	anjuta_set_active ();
	return TRUE;

error_show:
	if (str) g_free (str);
	anjuta_system_error (errno, _("Unable to save the project."));
	fclose (fp);
	anjuta_status (_("Unable to save the project."));
	anjuta_set_active ();
	p->is_saved = FALSE;
	return FALSE;
}

gboolean
project_dbase_save_yourself (ProjectDBase * p, FILE * stream)
{
	g_return_val_if_fail (p != NULL, FALSE);
	g_return_val_if_fail (stream != NULL, FALSE);

	if (project_config_save_yourself (p->project_config, stream)==FALSE)
		return FALSE;

	fprintf (stream, "project.is.docked=%d\n", p->is_docked);
	if (p->is_showing && !p->is_docked)
	{
		gdk_window_get_root_origin (p->widgets.window->window,
					    &p->win_pos_x, &p->win_pos_y);
		gdk_window_get_size (p->widgets.window->window, &p->win_width,
				     &p->win_height);
	}
	fprintf (stream, "project.win.pos.x=%d\n", p->win_pos_x);
	fprintf (stream, "project.win.pos.y=%d\n", p->win_pos_y);
	fprintf (stream, "project.win.width=%d\n", p->win_width);
	fprintf (stream, "project.win.height=%d\n", p->win_height);
	return TRUE;
}

gboolean
project_dbase_load_yourself (ProjectDBase * p, PropsID props)
{
	gboolean dock_flag;

	g_return_val_if_fail (p != NULL, FALSE);

	if (project_config_load_yourself (p->project_config, props)==FALSE)
		return FALSE;

	dock_flag = prop_get_int (props, "project.is.docked", 0);
	p->win_pos_x = prop_get_int (props, "project.win.pos.x", 100);
	p->win_pos_y = prop_get_int (props, "project.win.pos.y", 80);
	p->win_width = prop_get_int (props, "project.win.width", 200);
	p->win_height = prop_get_int (props, "project.win.height", 400);
	if (dock_flag)
		project_dbase_dock (p);
	else
		project_dbase_undock (p);
	return TRUE;
}

void
project_dbase_update_tags_image(ProjectDBase* p)
{
	GList *src_files;
	gchar* src_dir;
	if (p->project_is_open == FALSE)
		return;
	
	src_dir = project_dbase_get_module_dir (p, MODULE_SOURCE);
	if (src_dir)
	{
		gchar* src_path;
		
		src_path = g_strconcat (src_dir, "/", NULL);
		src_files = glist_from_data (p->props, "module.source.files");
		glist_strings_prefix (src_files, src_path);
		tags_manager_update_image (app->tags_manager, src_files);
		glist_strings_free (src_files);
		g_free (src_path);
		g_free (src_dir);
	}
}

static void
project_dbase_save_session_files (ProjectDBase * p)
{
	GList	*node;
	gint	nIndex = 0;

	g_return_if_fail (p != NULL);
	g_return_if_fail (p->project_is_open == TRUE);
	
	/* Save session.... */
	session_clear_section( p, SECSTR(SECTION_FILELIST) );
	node = app->text_editor_list;
	while (node)
	{
		TextEditor* te;
		te = node->data;
		if(te)
		{
			if ( te->full_filename )
			{
				session_save_string( p, SECSTR(SECTION_FILELIST), nIndex, te->full_filename );
				session_save_long_n( p, SECSTR(SECTION_FILENUMBER), nIndex, te->current_line );
				project_dbase_save_markers( p, te, nIndex );
				nIndex++;
			}
		}
		node = node->next;
	}
}


static void
project_dbase_save_markers( ProjectDBase * p, TextEditor *te, const gint nItem )
{
	gint	nLineNo = -1 ;
	gint	nIndex = 0 ;
	gint	nMarks;
	gchar	*szSaveStr;

	g_return_if_fail (p != NULL);
	g_return_if_fail (te != NULL);
	nMarks = text_editor_get_num_bookmarks(te);
	szSaveStr = g_malloc( nMarks*20+2);
	if( ( NULL != szSaveStr ) && nMarks )
	{
		gchar	*sz = szSaveStr ;
		strcpy( sz, "" );
		while( ( nLineNo = text_editor_get_bookmark_line( te, nLineNo ) ) >= 0 )
		{
			sz += sprintf( sz, "%d,", nLineNo );
			nIndex ++ ;
			if( nIndex > nMarks )
				break;
		}
		session_save_string( p, SECSTR(SECTION_FILEMARKERS), nItem, szSaveStr );
	} else
		session_save_string( p, SECSTR(SECTION_FILEMARKERS), nItem, "" );
	g_free( szSaveStr );
}

static void
project_dbase_save_session (ProjectDBase * p)
{
	debugger_save_session_breakpoints( p );
	project_dbase_save_session_files ( p );
	find_replace_save_session( app->find_replace, p );
	executer_save_session( app->executer, p );
	find_in_files_save_session( app->find_in_files, p );
	session_save_bool( p, SECSTR(SECTION_PROJECTDBASE), szShowLocalsItem, p->m_prj_ShowLocal );
	session_sync();
}


void
project_dbase_close_project (ProjectDBase * p)
{
	GList *node;

	g_return_if_fail (p != NULL);
	g_return_if_fail (p->project_is_open == TRUE);
	
	tags_manager_save (app->tags_manager);
	if (p->is_saved == FALSE)
	{
		gint but;
		
		but = gnome_dialog_run (GNOME_DIALOG (create_project_confirm_dlg()));
		switch (but)
		{
			case 0:
				project_dbase_save_project (p);
				break;
			case 1:
				p->is_saved = TRUE;
				break;
			default:
				return;
		}
	}

	/* Save session.... */
	project_dbase_save_session(p);
	/* Close all files that are part of the project */
	node = app->text_editor_list;
	while (node)
	{
		TextEditor* te;
		GList* next;
		te = node->data;
		next = node->next; // Save it now, as we may change it.
		if(te)
		{
			if (text_editor_is_saved (te) && te->full_filename)
			{
				if (strncmp(te->full_filename, p->top_proj_dir, strlen(p->top_proj_dir)) ==0)
				{
					/*g_print("Closing file %s\n", te->filename);*/
					anjuta_remove_text_editor(te);
				}
			}
		}
		node = next;
	}
	project_dbase_hide (p);
	project_dbase_update_menu (p);	
	project_dbase_clean_left (p);
	ccview_project_clear(CCVIEW_PROJECT(p->widgets.ccview));
	p->project_is_open = FALSE;
}

void
project_dbase_dock (ProjectDBase * p)
{
	if (p)
	{
		if (p->is_docked)
			return;
		if (p->is_showing)
		{
			project_dbase_hide (p);
			p->is_docked = TRUE;
			project_dbase_show (p);
			return;
		}
		else
		{
			p->is_docked = TRUE;
			return;
		}
	}
}
void
project_dbase_undock (ProjectDBase * p)
{
	if (p)
	{
		if (!p->is_docked)
			return;
		if (p->is_showing)
		{
			project_dbase_hide (p);
			p->is_docked = FALSE;
			project_dbase_show (p);
			return;
		}
		else
		{
			p->is_docked = FALSE;
			return;
		}
	}
}


gboolean
project_dbase_is_file_in_module (ProjectDBase * p, PrjModule module,
				 gchar * file)
{
	gchar *fdir, *fname, *tmp;
	gchar *mod;
	gchar *mdir;
	GList *files, *node;

	if (!p || !file)
		return FALSE;
	if (p->project_is_open == FALSE)
		return FALSE;
	fdir = g_dirname (file);
	fname = extract_filename (file);
	if (!fdir)
		return FALSE;

	tmp = g_strconcat ("module.", module_map[module], ".name", NULL);
	mod = prop_get (p->props, tmp);
	g_free (tmp);
	mdir = g_strconcat (p->top_proj_dir, "/", mod, NULL);
	g_free (mod);
	if (strcmp (mdir, fdir) != 0)
	{
		g_free (mdir);
		g_free (fdir);
		return FALSE;
	}
	g_free (fdir);
	g_free (mdir);

	files = project_dbase_get_module_files (p, module);
	node = files;
	while (node)
	{
		if (strcmp (node->data, fname) == 0)
		{
			glist_strings_free (files);
			return TRUE;
		}
		node = g_list_next (node);
	}
	glist_strings_free (files);
	return FALSE;
}

void
project_dbase_show_info (ProjectDBase * p)
{
	gchar *str[14];
	gint i;
	GList *list;
	
	g_return_if_fail (p != NULL);
	g_return_if_fail (p->project_is_open == TRUE);

	str[0] = project_dbase_get_proj_name(p);
	str[1] = prop_get (p->props, "project.verson");
	str[2] = prop_get (p->props, "project.author");
	if (project_dbase_get_project_type(p) != PROJECT_TYPE_GENERIC)
		str[3] = g_strdup (_("Yes"));
	else
		str[3] = g_strdup (_("No"));
	
	/* For the time being */
	str[4] = g_strdup (_("Yes"));

	if (prop_get_int (p->props, "project.has.gettext", 1))
		str[5] = g_strdup (_("Yes"));
	else
		str[5] = g_strdup (_("No"));
	str[6] = prop_get (p->props, "project.source.target");
	
	list = project_dbase_get_module_files (p, MODULE_SOURCE);
	str[7] = g_strdup_printf ("%d", g_list_length (list));
	glist_strings_free (list);

	list = project_dbase_get_module_files (p, MODULE_HELP);
	str[8] = g_strdup_printf ("%d", g_list_length (list));
	glist_strings_free (list);

	list = project_dbase_get_module_files (p, MODULE_DATA);
	str[9] = g_strdup_printf ("%d", g_list_length (list));
	glist_strings_free (list);

	list = project_dbase_get_module_files (p, MODULE_PIXMAP);
	str[10] = g_strdup_printf ("%d", g_list_length (list));
	glist_strings_free (list);

	list = project_dbase_get_module_files (p, MODULE_DOC);
	str[11] = g_strdup_printf ("%d", g_list_length (list));
	glist_strings_free (list);

	list = project_dbase_get_module_files (p, MODULE_PO);
	str[12] = g_strdup_printf ("%d", g_list_length (list));
	glist_strings_free (list);

	str[13] = prop_get (p->props, "project.type");

	gtk_widget_show (create_project_dbase_info_gui (str));
	for (i = 0; i < 14; i++)
	g_free (str[i]);
	return;
}

gboolean
project_dbase_summon_glade (ProjectDBase *p)
{
	gchar *filename, *target;
	gboolean ret;

	if (p->project_is_open == FALSE)
		return FALSE;

	target =	prop_get (p->props, "project.source.target");
	g_strdelimit (target, "-", '_');

	filename = g_strdup_printf ("%s/%s.glade",
				    p->top_proj_dir,
				    target);
	if (file_is_regular (filename) == FALSE)
	{
		g_free (filename);
		anjuta_error (
			_("The Project glade file does not\n"
			"exist in the top level project directory."));
		return FALSE;
	}
	ret = glade_iface_start_glade_editing (filename);
	g_free (filename);
	return ret;
}


gboolean
project_dbase_generate_source_code (ProjectDBase *p)
{
	gchar *filename, *target;
	gboolean ret;

	if (p->project_is_open == FALSE)
		return FALSE;

	target =	prop_get (p->props, "project.source.target");
	g_strdelimit (target, "-", '_');
	if (project_dbase_get_project_type (p) == PROJECT_TYPE_GENERIC)
	{
		return source_write_generic_main_c (p);
	}
	
	filename = g_strdup_printf ("%s/%s.glade",
				    p->top_proj_dir,
				    target);
	g_free (target);
	if (file_is_regular (filename) == FALSE)
	{
		anjuta_error (
			_("The Project glade file does not\n"
			"exist in the top level project directory."));
		ret = source_write_generic_main_c (p);
	}
	else
	{
		if ((ret=glade_iface_generate_source_code (filename)) == FALSE)
			ret = source_write_generic_main_c (p);
	}
	g_free (filename);
	return ret;
}

GList *
project_dbase_get_module_files (ProjectDBase * p, PrjModule module)
{
	gchar *tmp;
	GList *files;

	if (!p || module >= MODULE_END_MARK)
		return NULL;
	if (p->project_is_open == FALSE)
		return NULL;

	tmp = g_strconcat ("module.", module_map[module], ".files", NULL);
	files = glist_from_data (p->props, tmp);
	g_free (tmp);

	return files;
}

gchar *
project_dbase_get_proj_name (ProjectDBase * p)
{
	if (!p)
		return NULL;
	if (p->project_is_open == FALSE)
		return NULL;
	return prop_get (p->props, "project.name");
}

gchar *
project_dbase_get_source_target (ProjectDBase * p)
{
	gchar *str, *src_dir, *target;

	if (!p)
		return NULL;
	if (p->project_is_open == FALSE)
		return NULL;
	target = prop_get (p->props, "project.source.target");
	src_dir = project_dbase_get_module_dir (p, MODULE_SOURCE);
	str = g_strconcat (src_dir, "/", target, NULL);
	g_free (target);
	g_free (src_dir);
	return str;
}

gchar *
project_dbase_get_version (ProjectDBase * p)
{
	if (!p) return NULL;
	if (p->project_is_open == FALSE)
		return NULL;
	return prop_get (p->props, "project.version");
}

/* project type. */
Project_Type*
project_dbase_get_project_type (ProjectDBase* p)
{
	gchar *str;
	gint i;
	
	if (p == NULL)
		return NULL;
	
	str = prop_get (p->props, "project.type");
	if (!str)
		return NULL;

	for (i=0; i<PROJECT_TYPE_END_MARK; i++)
	{
		gint ret;
		ret = strcasecmp (project_type_map[i], g_strstrip (str));
		if (ret == 0)
		{
			g_free (str);
			return load_project_type(i);
		}
	}
	return NULL;
}

/* project language. */
gint
project_dbase_get_language (ProjectDBase* p)
{
	gchar *str;
	gint i;
	
	g_return_val_if_fail (p != NULL, PROJECT_PROGRAMMING_LANGUAGE_END_MARK);
	
	str = prop_get (p->props, "project.programming.language");
	if (!str)
		return PROJECT_PROGRAMMING_LANGUAGE_END_MARK;

	for (i=0; i<PROJECT_PROGRAMMING_LANGUAGE_END_MARK; i++)
	{
		gint ret;
		ret = strcasecmp (programming_language_map[i], g_strstrip (str));
		if (ret == 0)
		{
			g_free (str);
			return i;
		}
	}
	return PROJECT_PROGRAMMING_LANGUAGE_END_MARK;
}

/* Target type*/
gint
project_dbase_get_target_type (ProjectDBase* p)
{
	gchar *str;
	gint i;
	
	g_return_val_if_fail (p != NULL, PROJECT_TARGET_TYPE_END_MARK;);
	str = prop_get (p->props, "project.target.type");
	if (!str)
		return PROJECT_TARGET_TYPE_END_MARK;

	for (i=0; i<PROJECT_TARGET_TYPE_END_MARK; i++)
	{
		gint ret;
		
		ret = strcasecmp (project_target_type_map[i], g_strstrip (str));
		if (ret == 0)
		{
			g_free (str);
			return i;
		}
	}
	return PROJECT_TARGET_TYPE_END_MARK;
}

gchar *
project_dbase_get_module_type (ProjectDBase * p, PrjModule module)
{
	gchar *key, *type;
	if (!p || module >= MODULE_END_MARK)
		return NULL;
	if (p->project_is_open == FALSE)
		return NULL;

	key = g_strconcat ("module.", module_map[module], ".type", NULL);
	type = prop_get (p->props, key);
	g_free (key);
	return type;
}

gchar *
project_dbase_get_module_dir (ProjectDBase * p, PrjModule module)
{
	gchar *mod_name, *dir;

	g_return_val_if_fail (p != NULL, NULL);
	g_return_val_if_fail (module < MODULE_END_MARK, NULL);

	if (p->project_is_open == FALSE)
		return NULL;
	mod_name = project_dbase_get_module_name (p, module);
	dir = g_strconcat (p->top_proj_dir, "/", mod_name, NULL);
	return dir;
}

gchar *
project_dbase_get_module_name (ProjectDBase * p, PrjModule module)
{
	gchar *tmp, *name;
	if (!p || module >= MODULE_END_MARK)
		return NULL;
	if (p->project_is_open == FALSE)
		return NULL;

	tmp = g_strconcat ("module.", module_map[module], ".name", NULL);
	name = prop_get (p->props, tmp);
	g_free (tmp);
	return name;
}

gboolean
project_dbase_module_is_empty (ProjectDBase * p, PrjModule module)
{
	gchar *tmp, *str;
	if (!p || module >= MODULE_END_MARK)
		return TRUE;
	if (p->project_is_open == FALSE)
		return TRUE;

	tmp = g_strconcat ("module.", module_map[module], ".files", NULL);
	str = prop_get (p->props, tmp);
	g_free (tmp);
	if (!str)
		return TRUE;
	g_free (str);
	return FALSE;
}

GList*
project_dbase_scan_files_in_module(ProjectDBase* p, PrjModule module, gboolean with_makefiles)
{
	gchar *top_dir, *mod_dir, *dir, *key;
	GList *files;

	g_return_val_if_fail (p != NULL, NULL);
	g_return_val_if_fail (module < MODULE_END_MARK, NULL);

	files = NULL;

	/* Can not use p->top_proj_dir yet. */
	top_dir = g_dirname (p->proj_filename);

	/* Can not use project_dbase_get_module_dir() yet */
	key = g_strconcat ("module.", module_map[module], ".name", NULL);
	mod_dir = prop_get (p->props, key);
	if (top_dir)
		dir = g_strconcat (top_dir, "/", mod_dir, NULL);
	else
		dir = g_strdup (mod_dir);

	if (dir)
	{
		GList *node;
		chdir(dir);
		files = scan_files_in_dir (dir, select_only_file);
		chdir(top_dir);

		if (with_makefiles == FALSE)
		{
			node = files;
			while (node)
			{
				gpointer data;
				
				data = node->data;
				node = g_list_next (node);
				if (strcasecmp (data, "Makefile") == 0
					||strcasecmp (data, "Makefile.am") == 0
					||strcasecmp (data, "Makefile.in") == 0)
				{
					files = g_list_remove (files, data);
				}
			}
		}
	}
	g_free (key);
	g_free (dir);
	g_free (mod_dir);
	g_free (top_dir);
	return files;
}

void
project_dbase_clean_left (ProjectDBase * p)
{
	TextEditor *te;
	gint i;
	project_dbase_clear (p);
	project_config_clear (p->project_config);

	tags_manager_freeze (app->tags_manager);
	tags_manager_clear (app->tags_manager);
	for (i = 0; i < g_list_length (app->text_editor_list); i++)
	{
		te = g_list_nth_data (app->text_editor_list, i);
		if (te->full_filename)
			tags_manager_update (app->tags_manager,
					     te->full_filename);
	}
	tags_manager_thaw (app->tags_manager);
	compiler_options_load (app->compiler_options, app->preferences->props);
	src_paths_load (app->src_paths, app->preferences->props);
}

/*
 * Private functions: Do not use
 */
void
project_dbase_detach (ProjectDBase * p)
{
	gtk_container_remove (GTK_CONTAINER
			      (app->widgets.project_dbase_win_container),
			      app->project_dbase->widgets.client);
	gtk_container_add (GTK_CONTAINER
			   (app->project_dbase->widgets.client_area),
			   app->project_dbase->widgets.client);

	if (app->widgets.the_client == app->widgets.vpaned)
	{
		gtk_container_remove (GTK_CONTAINER (app->widgets.hpaned),
				      app->widgets.notebook);
		gtk_container_remove (GTK_CONTAINER (app->widgets.vpaned),
				      app->widgets.hpaned);
		gtk_container_add (GTK_CONTAINER (app->widgets.vpaned),
				   app->widgets.notebook);
		app->widgets.hpaned_client = app->widgets.notebook;
	}
	else
	{
		gtk_container_remove (GTK_CONTAINER (app->widgets.hpaned),
				      app->widgets.notebook);
		gtk_container_remove (GTK_CONTAINER
				      (app->widgets.client_area),
				      app->widgets.hpaned);
		gtk_container_add (GTK_CONTAINER (app->widgets.client_area),
				   app->widgets.notebook);
		app->widgets.hpaned_client = app->widgets.notebook;
		app->widgets.the_client = app->widgets.notebook;
	}
	gtk_widget_show (app->project_dbase->widgets.client);
	gtk_widget_show (app->widgets.notebook);
}

void
project_dbase_attach (ProjectDBase * p)
{
	gtk_container_remove (GTK_CONTAINER
			      (app->project_dbase->widgets.client_area),
			      app->project_dbase->widgets.client);
	gtk_container_add (GTK_CONTAINER
			   (app->widgets.project_dbase_win_container),
			   app->project_dbase->widgets.client);

	if (app->widgets.the_client == app->widgets.vpaned)
	{
		gtk_container_remove (GTK_CONTAINER (app->widgets.vpaned),
				      app->widgets.notebook);
		gtk_container_add (GTK_CONTAINER (app->widgets.hpaned),
				   app->widgets.notebook);
		gtk_container_add (GTK_CONTAINER (app->widgets.vpaned),
				   app->widgets.hpaned);
		app->widgets.hpaned_client = app->widgets.hpaned;
	}
	else
	{
		gtk_container_remove (GTK_CONTAINER
				      (app->widgets.client_area),
				      app->widgets.notebook);
		gtk_container_add (GTK_CONTAINER
				   (app->widgets.hpaned),
				   app->widgets.notebook);
		gtk_container_add (GTK_CONTAINER (app->widgets.client_area),
				   app->widgets.hpaned);
		app->widgets.hpaned_client = app->widgets.hpaned;
		app->widgets.the_client = app->widgets.hpaned;
	}
	gtk_widget_show (app->widgets.the_client);
}

void
project_dbase_update_menu (ProjectDBase * p)
{
	GtkWidget *submenu;
	gint max_recent_prjs;

	max_recent_prjs =
		preferences_get_int (app->preferences,
				     MAXIMUM_RECENT_PROJECTS);
	if (p->project_is_open)
	{
		app->recent_projects =
			update_string_list (app->recent_projects,
					    p->proj_filename,
					    max_recent_prjs);
		submenu =
			create_submenu (_("Recent Projects "),
					app->recent_projects,
					GTK_SIGNAL_FUNC
					(on_recent_projects_menu_item_activate));
		gtk_menu_item_set_submenu (GTK_MENU_ITEM
					   (app->widgets.menubar.
					    file.recent_projects), submenu);
	}
}

void
project_dbase_update_tree (ProjectDBase * p)
{
	gchar *tmp1, *tmp2, *tmp3, *title;
	gchar *dum_array[1];
	gint i;

	GtkCTreeNode *parent, *sub_parent;
	GdkPixmap *pix_c;
	GdkPixmap *pix_o;
	GdkBitmap *mask_c;
	GdkBitmap *mask_o;
	ProjectFileData *pfd;

	/* Read the basic information */
	tmp1 = prop_get (p->props, "project.name");
	tmp2 = prop_get (p->props, "project.version");
	if (tmp1 && tmp2)
		tmp3 = g_strconcat (tmp1, "-", tmp2, NULL);
	else
		tmp3 = NULL;
	if (tmp1)
		g_free (tmp1);
	if (tmp2)
		g_free (tmp2);
	if (!tmp3)
		return;

	/* Set project title */
	title = g_strconcat (_("Project: "), tmp3, NULL);
	gtk_window_set_title (GTK_WINDOW (p->widgets.window), title);
	g_free (title);

	/* Add the root in the ctree */
	gtk_clist_freeze (GTK_CLIST (p->widgets.ctree));
	project_dbase_clear_ctree(p);
	get_pixmask_on_file_type (FILE_TYPE_DIR, p->widgets.window, &pix_c,
				  &mask_c, &pix_o, &mask_o);
	dum_array[0] = tmp3;
	parent = gtk_ctree_insert_node (GTK_CTREE (p->widgets.ctree),
					NULL, NULL, dum_array, 5, pix_c,
					mask_c, pix_o, mask_o, FALSE, TRUE);
	pfd =
		project_file_data_new (NULL, MODULE_SOURCE, NULL);
	gtk_ctree_node_set_row_data_full (GTK_CTREE (p->widgets.ctree),
					  GTK_CTREE_NODE (parent), pfd,
					  (GtkDestroyNotify)
					  project_file_data_destroy);

	g_free (tmp3);
	p->widgets.root_node = parent;
	
	/* Read the all the modules of the project */
	for (i=0; i<MODULE_END_MARK; i++)
	{
		gchar *key, *prefix;
		GList *list;
		
		key = g_strconcat ("module.", module_map[i], ".name", NULL);
		tmp1 = prop_get (p->props, key);
		g_free (key);
		if (!tmp1)
			continue;
		
		key = g_strconcat ("module.", module_map[i], ".files", NULL);
		list = glist_from_data (p->props, key);
		g_free (key);
		if (!list)
			continue;
		
		get_pixmask_on_file_type (FILE_TYPE_DIR,
					  p->widgets.window, &pix_c,
					  &mask_c, &pix_o, &mask_o);
		dum_array[0] = g_strconcat (module_map[i], " [", tmp1, "]", NULL);
		key = g_strconcat ("module.", module_map[i], ".expanded", NULL);
		sub_parent =
			gtk_ctree_insert_node (GTK_CTREE
					       (p->widgets.ctree),
					       parent, NULL,
					       dum_array, 5, pix_c,
					       mask_c, pix_o, mask_o,
					       FALSE, prop_get_int (p->props, key, 1));
		g_free (key);
		g_free (dum_array[0]);
		p->widgets.module_node[i] = sub_parent;
		pfd = project_file_data_new (parent, i, NULL);
		gtk_ctree_node_set_row_data_full (GTK_CTREE
						  (p->widgets.ctree),
						  GTK_CTREE_NODE
						  (sub_parent), pfd,
						  (GtkDestroyNotify)
						  project_file_data_destroy);
		prefix =
			g_strconcat (p->top_proj_dir, "/", tmp1,
				     NULL);
		gtree_insert_files (p->widgets.ctree, sub_parent, i, prefix, list);
		g_free (prefix);
		glist_strings_free (list);
		g_free (tmp1);
	}
	gtk_clist_thaw (GTK_CLIST (p->widgets.ctree));
}

void
project_dbase_update_controls (ProjectDBase * pd)
{
	gboolean p = pd->project_is_open;
	gtk_widget_set_sensitive (pd->widgets.menu_import, p);
	gtk_widget_set_sensitive (pd->widgets.menu_view, p);
	gtk_widget_set_sensitive (pd->widgets.menu_edit, p);
	gtk_widget_set_sensitive (pd->widgets.menu_remove, p);
	gtk_widget_set_sensitive (pd->widgets.menu_configure, p);
	gtk_widget_set_sensitive (pd->widgets.menu_info, p);
}


void
project_dbase_add_file_to_module (ProjectDBase * p, PrjModule module,
				  gchar * filename)
{
	gchar *mod_files, *file_list, *new_file_list;

	g_return_if_fail (p != NULL);
	g_return_if_fail (p->sel_module < MODULE_END_MARK);

	mod_files =
		g_strconcat ("module.", module_map[module], ".files", NULL);
	g_return_if_fail (mod_files != NULL);
	file_list = prop_get (p->props, mod_files);
	if (!file_list)
		file_list = g_strdup ("");
	new_file_list =
		g_strconcat (file_list, " ", extract_filename (filename),
			     NULL);
	prop_set_with_key (p->props, mod_files, new_file_list);
	g_free (new_file_list);
	g_free (file_list);
	g_free (mod_files);
	project_dbase_update_tree (p);
	p->is_saved = FALSE;
}

void
project_dbase_remove_file (ProjectDBase * p)
{
	gchar *key, *fn, *files, *pos;
	gint i;
	PrjModule module;
	
	module = p->current_file_data->module;
	key = g_strconcat ("module.", module_map[module], ".files", NULL);
	files = prop_get (p->props, key);
	if (files == NULL)
	{
		g_free (key);
		return;
	}
	fn = extract_filename (p->current_file_data->filename);
	
	pos = strstr (files, fn);
	if (pos == NULL)
	{
		g_free (files);
		g_free (key);
		return;
	}
	for (i=0; i< strlen(fn); i++)
	{
		*pos++ = ' ';
	}
	prop_set_with_key (p->props, key, files);
	gtk_ctree_remove_node (GTK_CTREE (p->widgets.ctree),
		p->widgets.current_node);
	p->is_saved = FALSE;

	/* Check if the module is empty */
	files = prop_get (p->props, key);
	if (files == NULL)
	{
		/* Module is empty so remove the module */
		gtk_ctree_remove_node (GTK_CTREE (p->widgets.ctree),
			p->widgets.module_node[module]);
		return;
	}
	g_free (key);
	g_free (files);
}

gchar*
project_dbase_get_dir (ProjectDBase * p)
{
	g_return_val_if_fail( (NULL != p), NULL );
	return p->top_proj_dir ;
}

gchar*
project_dbase_get_name (ProjectDBase * p)
{
	g_return_val_if_fail( (NULL != p), NULL );
	return prop_get (p->props, "project.name");
}

void
project_dbase_set_show_locals( ProjectDBase * p,  const gboolean bActive )
{
	/* Null can be a valid entry */
	if( NULL != p )
	{
		p->m_prj_ShowLocal = bActive ;
	}
}
