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
#include "fileselection.h"
#include "resources.h"
#include "controls.h"
#include "utilities.h"
#include "properties.h"
#include "source.h"
#include "glade_iface.h"
#include "compatibility_0.h"
#include "defaults.h"
#include "debugger.h"
#include "find_replace.h"
#include "executer.h"

#include "an_file_view.h"
#include "an_symbol_view.h"
#include "pixmaps.h"

static void project_reload_session_files(ProjectDBase * p);
static void project_dbase_save_session (ProjectDBase * p);
static void project_dbase_reload_session (ProjectDBase * p);
static void project_dbase_save_session_files (ProjectDBase * p);
static void project_dbase_save_markers( ProjectDBase * p, TextEditor *te, const gint nItem );
static void project_dbase_make_default_filetype_list(ProjectDBase * p);
static void session_load_node_expansion_states (ProjectDBase *p);

static const gchar szShowLocalsItem[] = { "ShowLocals" };

gchar *project_type_map[]=
{
	"GENERIC",
	"GTK",
	"GNOME",
	"BONOBO",
	"glademm (gtkmm 1.2)",
	"glademm (gnomemm 1.2)",
	"LIBGLADE",
	"wxWINDOWS",
	"GTK 2.0",
	"gtkmm 2.0",
	"GNOME 2.0",
	"LIBGLADE2",
	"gnomemm 2.0",
	"XLib",
	"XLib Dock App",
	"Qt",
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
	"include",
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

/*
These are the preferences which will be saved along
with the project. These preferences will override those
set by the user before loading the project.

In .prj file, we prefix with "preferences."
those are the pref names in preferences props
*/
static const gchar* editor_prefs[] =
{
	INDENT_AUTOMATIC,
	USE_TABS,
	INDENT_OPENING,
	INDENT_CLOSING,
	TAB_SIZE,
	INDENT_SIZE,
	AUTOFORMAT_STYLE,
	AUTOFORMAT_CUSTOM_STYLE,
	AUTOFORMAT_DISABLE,
	NULL
};

static void
gtree_insert_files (GtkTreeView *treeview, GtkTreeIter *parent,
					PrjModule mod,  gchar *dir_prefix, GList *items)
{
	GList *node;
	
	node = g_list_sort (items, compare_string_func);
	while (node)
	{
		gchar *full_fname;
		ProjectFileData *pfd;
		GtkTreeStore *store;
		GtkTreeIter iter;
		GdkPixbuf *pixbuf;

		if (node->data == NULL)
			continue;
		full_fname = g_strconcat (dir_prefix, "/", node->data, NULL);
		pixbuf = gdl_icons_get_uri_icon(app->icon_set, full_fname);
		pfd = project_file_data_new (mod, node->data, full_fname);
		g_free (full_fname);
		
		store = GTK_TREE_STORE (gtk_tree_view_get_model (treeview));
		gtk_tree_store_append (store, &iter, parent);
		gtk_tree_store_set (store, &iter,
							PROJECT_PIX_COLUMN, pixbuf,
							PROJECT_NAME_COLUMN, node->data,
							PROJECT_DATA_COLUMN, pfd, -1);
		node = g_list_next (node);
	}
}

ProjectFileData *
project_file_data_new (PrjModule mod,
					   gchar* fname, gchar * full_fname)
{
	ProjectFileData *pfd;

	pfd = g_malloc0 (sizeof (ProjectFileData));
	if (!pfd)
		return NULL;

	pfd->module = mod;
	pfd->filename = NULL;
	pfd->full_filename = NULL;
	if (fname)
		pfd->filename = g_strdup (fname);
	if (full_fname)
		pfd->full_filename = g_strdup (full_fname);
	return pfd;
}

void
project_file_data_destroy (ProjectFileData * pfd)
{
	g_return_if_fail (pfd != NULL);
	if (pfd->filename)
		g_free (pfd->filename);
	if (pfd->full_filename)
		g_free (pfd->full_filename);
	g_free (pfd);
}

static void
project_preferences_changed (AnjutaPreferences *pr, ProjectDBase *p)
{
	if (p->project_is_open)
		p->is_saved = FALSE;
}

ProjectDBase *
project_dbase_new (PropsID pr_props)
{
	ProjectDBase *p;
	gint i;
	
	/* Must declare static, because it will be used forever */
	static FileSelData fsd1 = { N_("Open Project"), NULL,
		on_open_prjfilesel_ok_clicked,
		NULL,
		NULL
	};

	/* Must declare static, because it will be used forever */
	static FileSelData fsd2 = { N_("Add to Project"), NULL,
		on_add_prjfilesel_ok_clicked,
		NULL,
		NULL
	};

	p = g_malloc0(sizeof (ProjectDBase));
	if (!p) return NULL;
	p->size = sizeof(ProjectDBase);
	
	/* Initialization */
	/* Cannot assign p to static fsd.data, so doing it now */
	fsd1.data = p;
	fsd2.data = p;
	
	p->tm_project = NULL;
	p->project_is_open = FALSE;
	p->is_showing = TRUE;
	p->is_docked = TRUE;
	p->win_pos_x = 100;
	p->win_pos_y = 80;
	p->win_width = 200;
	p->win_height = 400;
	p->top_proj_dir = NULL;
	p->current_file_data = NULL;
	p->clean_before_build = FALSE;

	create_project_dbase_gui (p);
	gtk_window_set_title (GTK_WINDOW (p->widgets.window),
			      _("Project: None"));
	p->fileselection_open = create_fileselection_gui (&fsd1);
	p->fileselection_add_file = create_fileselection_gui (&fsd2);
	p->sel_module = MODULE_SOURCE;
	p->props = prop_set_new ();
	prop_set_parent (p->props, pr_props);
	p->project_config = project_config_new (p->props);
	
	g_signal_connect (G_OBJECT (app->preferences), "changed",
					  G_CALLBACK (project_preferences_changed), p);
	return p;
}

void
project_dbase_destroy (ProjectDBase * p)
{
	g_return_if_fail (p != NULL);
	project_dbase_clear (p);
	
	project_config_destroy (p->project_config);
	
	gtk_widget_unref (p->widgets.window);
	sv_clear();
	fv_clear();
	gtk_widget_unref (p->widgets.client_area);
	gtk_widget_unref (p->widgets.client);
	gtk_widget_unref (p->widgets.scrolledwindow);
	gtk_widget_unref (p->widgets.treeview);
	gtk_widget_unref (p->widgets.menu);
	gtk_widget_unref (p->widgets.menu_import);
	gtk_widget_unref (p->widgets.menu_view);
	gtk_widget_unref (p->widgets.menu_edit);
	gtk_widget_unref (p->widgets.menu_remove);
	gtk_widget_unref (p->widgets.menu_configure);
	gtk_widget_unref (p->widgets.menu_info);
	gtk_widget_unref (p->widgets.menu_docked);
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
	GtkTreeStore *store;
	
	if (!p)
		return;
	store = GTK_TREE_STORE (gtk_tree_view_get_model (
							GTK_TREE_VIEW (p->widgets.treeview)));
	gtk_tree_store_clear (store);
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
	gtk_widget_set_sensitive(app->widgets.menubar.file.recent_projects, TRUE);
	p->is_saved = TRUE;
	extended_toolbar_update ();
	anjuta_update_app_status (FALSE, NULL);
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
		gtk_widget_grab_focus (p->widgets.treeview);
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

static void 
project_dbase_make_default_filetype_list(ProjectDBase * p)
{
    GList *ftypes=NULL;
	GList *combolist=NULL;

	p->fileselection_open = fileselection_clearfiletypes(p->fileselection_open);  
	ftypes = fileselection_addtype_f(ftypes, _("Anjuta project files"), "prj", NULL);
	ftypes = fileselection_addtype(ftypes, _("All files"), NULL);
	p->fileselection_open = fileselection_storetypes(p->fileselection_open, ftypes);
	combolist = fileselection_getcombolist(p->fileselection_open, ftypes);	
	fileselection_set_combolist(p->fileselection_open, combolist);
}

void
project_dbase_open_project (ProjectDBase * p)
{
	gchar *all_projects_dir;
	
	project_dbase_make_default_filetype_list(p);
	all_projects_dir =
		anjuta_preferences_get (ANJUTA_PREFERENCES (app->preferences),
								PROJECTS_DIRECTORY);
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
		anjuta_error (_("Unable to load Project: %s"), p->proj_filename);
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
				   ("Anjuta version %s or later is required to open this Project.\n"
					"Please upgrade to the latest version of Anjuta (Help for more information)."), ver);
		g_free (ver);
		return FALSE;
	}
	if (project_config_load(p->project_config, p->props)== FALSE)
	{
		anjuta_error (_("Unable to load Project: %s"), p->proj_filename);
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
	AnjutaPreferences *pr = ANJUTA_PREFERENCES (app->preferences);
	gboolean build_sv = anjuta_preferences_get_int (pr, BUILD_SYMBOL_BROWSER);
	gboolean build_fv = anjuta_preferences_get_int (pr, BUILD_FILE_BROWSER);
	gboolean auto_update = anjuta_preferences_get_int (pr, AUTOMATIC_TAGS_UPDATE);

	g_return_if_fail( NULL != p );
	debugger_reload_session_breakpoints(p);	
	project_reload_session_files(p);
	fv_session_load (p);
	if (auto_update)
		project_dbase_update_tags_image(p, TRUE);
	else
	{
		sv_populate(build_sv);
		fv_populate(build_fv);
	}
	session_load_node_expansion_states (p);
	find_replace_load_session( app->find_replace, p );
	executer_load_session( app->executer, p );
	find_in_files_load_session( app->find_in_files, p );
}

gboolean
project_dbase_load_project (ProjectDBase * p, const gchar *project_file,
							gboolean show_project)
{
	gchar* filename;
	gboolean ret;
	
	if (project_file)
		filename = g_strdup (project_file);
	else
		filename = fileselection_get_filename (p->fileselection_open);
	
	anjuta_set_busy ();
	ret = project_dbase_load_project_file(p, filename);
	if (ret)
		project_dbase_load_project_finish(p, show_project);		
	g_free(filename);
	anjuta_set_active ();
	return ret;
}

static gboolean
save_project_preference_property (AnjutaPreferences *pr, const gchar *key,
								  gpointer data)
{
	gchar *str;
	FILE *fp = data;
	
	str = prop_get (pr->props, key);
	if (str)
	{
		fprintf (fp, "preferences.%s=%s\n", key, str);
		g_free (str);
	}
	return TRUE;
}

gboolean
project_dbase_save_project (ProjectDBase * p)
{
	gchar* str, *str_prop;
	FILE *fp;
	gint i;

	str = NULL;
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
	if (!str) str = g_strdup ("Unknown Project");
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
	if (fprintf (fp, "project.source.target=%s\n", str) < 1)
		goto error_show;
	g_free (str); str = NULL;

	str = prop_get (p->props, "project.has.gettext");
	if (!str) str = g_strdup ("1");
	if (fprintf (fp, "project.has.gettext=%s\n", str) < 1)
		goto error_show;
	g_free (str); str = NULL;

	str = prop_get (p->props, "project.gui.command");
	if (!str) str = g_strdup ("");
	if (fprintf (fp, "project.gui.command=%s\n", str) < 1)
		goto error_show;
	g_free (str); str = NULL;

	str = prop_get (p->props, "project.programming.language");
	if (!str) str = g_strdup (programming_language_map[PROJECT_PROGRAMMING_LANGUAGE_C]);
	if (fprintf (fp, "project.programming.language=%s\n", str) < 1)
		goto error_show;
	g_free (str); str = NULL;

	str = prop_get (p->props, "project.excluded.modules");
	if (!str) str = g_strdup ("intl");
	if (fprintf (fp, "project.excluded.modules=%s\n\n", str) < 1)
		goto error_show;
	g_free (str); str = NULL;
	
	str = prop_get (p->props, "project.config.extra.modules.before");
	if (str)
	{
		fprintf (fp, "project.config.extra.modules.before=%s\n", str);
		g_free (str);
		str = NULL;
	}
	else
		fprintf (fp, "project.config.extra.modules.before=\n");

	str = prop_get (p->props, "project.config.extra.modules.after");
	if (str)
	{
		fprintf (fp, "project.config.extra.modules.after=%s\n", str);
		g_free (str);
		str = NULL;
	}
	else
		fprintf (fp, "project.config.extra.modules.after=\n");
	
	if (project_config_save (p->project_config, fp)== FALSE)
		goto error_show;
        
	str = prop_get (p->props, "project.menu.entry");
	if (!str) str = prop_get (p->props, "project.name");
	if (!str) str = g_strdup ("Unknown Project");
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
	if (fprintf (fp, "project.configure.options=%s\n", str) < 1)
		goto error_show;
	g_free (str); str = NULL;

	/* Yes, from the preferences */
	str = anjuta_preferences_get (ANJUTA_PREFERENCES (app->preferences),
								  "anjuta.program.arguments");
	if (!str) str = g_strdup ("");
	if (fprintf (fp, "anjuta.program.arguments=%s\n", str) < 1)
		goto error_show;
	g_free (str); str = NULL;

	/* Save the editor preferences if present */
#ifdef DEBUG
	printf("Saving editor preferences in the project file\n");
#endif
	anjuta_preferences_foreach (ANJUTA_PREFERENCES (app->preferences),
								ANJUTA_PREFERENCES_FILTER_PROJECT,
								save_project_preference_property, fp);
	fprintf(fp, "\n");

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
	if (p->tm_project)
		tm_project_save(TM_PROJECT(p->tm_project));
	p->is_saved = TRUE;
	fclose (fp);
	source_write_build_files (p);
	anjuta_status (_("Project saved successfully"));
	anjuta_set_active ();
	return TRUE;

error_show:
	if (str) g_free (str);
	anjuta_system_error (errno, _("Unable to save the Project."));
	fclose (fp);
	anjuta_status (_("Unable to save the Project."));
	anjuta_set_active ();
	p->is_saved = FALSE;
	return FALSE;
}

gboolean
project_dbase_save_yourself (ProjectDBase * p, FILE * stream)
{
	int clean_before_build;
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
	clean_before_build = p->clean_before_build ? 1 : 0;
	fprintf (stream, "project.clean_before_build=%d\n", clean_before_build);
	return TRUE;
}

gboolean
project_dbase_load_yourself (ProjectDBase * p, PropsID props)
{
	gboolean dock_flag;
	int clean_before_build;

	g_return_val_if_fail (p != NULL, FALSE);

	if (project_config_load_yourself (p->project_config, props)==FALSE)
		return FALSE;

	dock_flag = prop_get_int (props, "project.is.docked", 0);
	p->win_pos_x = prop_get_int (props, "project.win.pos.x", 100);
	p->win_pos_y = prop_get_int (props, "project.win.pos.y", 80);
	p->win_width = prop_get_int (props, "project.win.width", 200);
	p->win_height = prop_get_int (props, "project.win.height", 400);
	clean_before_build = prop_get_int (props, "project.clean_before_build", 0);
	p->clean_before_build = clean_before_build == 1 ? TRUE : FALSE;
	if (dock_flag)
		project_dbase_dock (p);
	else
		project_dbase_undock (p);
	return TRUE;
}

void project_dbase_sync_tags_image(ProjectDBase *p)
{
	AnjutaPreferences *pr = ANJUTA_PREFERENCES (app->preferences);
	gchar* src_dir, *inc_dir;
	gboolean build_sv = anjuta_preferences_get_int (pr, BUILD_SYMBOL_BROWSER);
	gboolean build_fv = anjuta_preferences_get_int (pr, BUILD_FILE_BROWSER);

	g_return_if_fail (p != NULL);

	if (p->project_is_open == FALSE)
		return;

	if (p->top_proj_dir && !p->tm_project)
		p->tm_project = tm_project_new(p->top_proj_dir, NULL, NULL, TRUE);

	src_dir = project_dbase_get_module_dir (p, MODULE_SOURCE);
	inc_dir = project_dbase_get_module_dir (p, MODULE_INCLUDE);

	if (src_dir)
	{
		gchar* src_path, *inc_path = NULL, *tmp;
		GList *src_files, *inc_files, *node;
		TextEditor *te;

		src_path = g_strconcat (src_dir, "/", NULL);
		if (inc_dir)
			inc_path = g_strconcat (inc_dir, "/", NULL);
		src_files = glist_from_data (p->props, "module.source.files");
		inc_files = glist_from_data (p->props, "module.include.files");
#ifdef DEBUG
		g_message("%d source files, %d include files"
		  , g_list_length(src_files), g_list_length(inc_files));
#endif
		glist_strings_prefix (src_files, src_path);
		if (inc_files && inc_path)
		{
			glist_strings_prefix (inc_files, inc_path);
			src_files = g_list_concat(src_files, inc_files);
		}
		for (node = src_files; node; node = g_list_next(node))
		{
			tmp = tm_get_real_path((const gchar *) node->data);
			g_free(node->data);
			node->data = tmp;
		}
		/* Since the open text editors hold a pointer to the
		source file, set it to NULL since the pointer might have
		been free()d by the syncing process
		*/
		for (node = app->text_editor_list; node; node = g_list_next(node))
		{
			te = (TextEditor *) node->data;
			te->tm_file = NULL;
		}
		tm_project_sync(TM_PROJECT(p->tm_project), src_files);
		/* Set the pointers back again */
		for (node = app->text_editor_list; node; node = g_list_next(node))
		{
			te = (TextEditor *) node->data;
			te->tm_file = tm_workspace_find_object(TM_WORK_OBJECT(
			  app->tm_workspace), te->full_filename, TRUE);
		}
		glist_strings_free (src_files);
		g_free (src_path);
		g_free (src_dir);
		if (inc_path)
			g_free (inc_path);
		if (inc_dir)
			g_free (inc_dir);
	}
	sv_populate(build_sv);
	fv_populate(build_fv);
}

void
project_dbase_update_tags_image(ProjectDBase* p, gboolean rebuild)
{
	gchar* src_dir;
	AnjutaPreferences *pr = ANJUTA_PREFERENCES (app->preferences);
	gboolean build_sv = anjuta_preferences_get_int (pr, BUILD_SYMBOL_BROWSER);
	gboolean build_fv = anjuta_preferences_get_int (pr, BUILD_FILE_BROWSER);

	g_return_if_fail (p != NULL);

	if (p->project_is_open == FALSE)
		return;

	if (p->tm_project)
	{
		if (((NULL == TM_PROJECT(p->tm_project)->file_list) ||
			(0 == TM_PROJECT(p->tm_project)->file_list->len) || rebuild)
			&& (p->top_proj_dir))
			tm_project_autoscan(TM_PROJECT(p->tm_project));
		else
			tm_project_update(p->tm_project, FALSE, TRUE, TRUE);
	}
	else if (p->top_proj_dir)
		p->tm_project = tm_project_new(p->top_proj_dir, NULL, NULL, TRUE);

	sv_populate(build_sv);
	fv_populate(build_fv);

	src_dir = project_dbase_get_module_dir (p, MODULE_SOURCE);

	if (src_dir)
	{
		gchar* src_path;
		GList *src_files;

		src_path = g_strconcat (src_dir, "/", NULL);
		src_files = glist_from_data (p->props, "module.source.files");
		glist_strings_prefix (src_files, src_path);
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
	GList	*editors = NULL;
	GList	*match;
	gint	i;
	TextEditor* te;

	g_return_if_fail (p != NULL);
	g_return_if_fail (p->project_is_open == TRUE);
	
	/* Save session.... */
	session_clear_section (p, SECSTR (SECTION_FILELIST));
	/* 
	TTimo - save using the tabs order, and push the docked files afterwards
	(the docked files are not remembered as docked status anyway)
	*/
	node = app->text_editor_list;
	while (node)
	{
		te = node->data;
		if(te && te->full_filename)
			editors = g_list_append(editors, te);
		node = node->next;
	}
	i = 0;
	while(( te = anjuta_get_notebook_text_editor(i)))
	{
		GList *match = g_list_find(editors, te);
		if (match)
		{
			te = (TextEditor *)match->data;
			session_save_string_n( p, SECSTR(SECTION_FILELIST), nIndex, te->full_filename );
			session_save_long_n( p, SECSTR(SECTION_FILENUMBER), nIndex, te->current_line );
			project_dbase_save_markers( p, te, nIndex );
			editors = g_list_remove_link(editors, match);
			g_list_free(match);
			nIndex++;			
		} else
			g_warning("Did not find notebook tab %d in text_editor_list\n", i);
		i++;
	}
	/* remaining items (if any) where docked */
	match = editors;
	while(match)
	{
		te = (TextEditor *)match->data;
		session_save_string_n (p, SECSTR(SECTION_FILELIST), nIndex, te->full_filename );
		session_save_long_n (p, SECSTR(SECTION_FILENUMBER), nIndex, te->current_line );
		project_dbase_save_markers (p, te, nIndex);
		nIndex++;
		match = g_list_next(match);
	}
	g_list_free(editors);
}

static void
project_dbase_save_markers (ProjectDBase * p, TextEditor *te, const gint nItem)
{
	gint	nLineNo = -1;
	gint	nIndex = 0;
	gint	nMarks;
	gchar	*szSaveStr;

	g_return_if_fail (p != NULL);
	g_return_if_fail (te != NULL);
	nMarks = text_editor_get_num_bookmarks (te);
	szSaveStr = g_malloc (nMarks*20+2);
	if ((NULL != szSaveStr) && nMarks)
	{
		gchar *sz = szSaveStr;
		strcpy (sz, "");
		while ((nLineNo = text_editor_get_bookmark_line (te, nLineNo)) >= 0)
		{
			sz += sprintf (sz, "%d,", nLineNo);
			nIndex ++;
			if (nIndex > nMarks)
				break;
		}
		session_save_string_n (p, SECSTR (SECTION_FILEMARKERS), nItem, szSaveStr);
	} else
		session_save_string_n (p, SECSTR (SECTION_FILEMARKERS), nItem, "");
	g_free (szSaveStr);
}

static void
mapping_function (GtkTreeView *treeview, GtkTreePath *path, gpointer data)
{
	gchar *str;
	GList *map = * ((GList **) data);
	
	str = gtk_tree_path_to_string (path);
	map = g_list_append (map, str);
	* ((GList **) data) = map;
};

/* Maps the expanded nodes for saving */
static GList *
tree_view_get_expansion_states (GtkTreeView *treeview)
{
	GList *map = NULL;
	gtk_tree_view_map_expanded_rows (treeview, mapping_function, &map);
	return map;
}

void
tree_view_set_expansion_states (GtkTreeView *treeview,
								GList *expansion_states)
{
	/* Restore expanded nodes */	
	if (expansion_states)
	{
		GtkTreePath *path;
		GtkTreeModel *model;
		GList *node;
		node = expansion_states;
		
		model = gtk_tree_view_get_model (treeview);
		while (node)
		{
			path = gtk_tree_path_new_from_string (node->data);
			gtk_tree_view_expand_row (treeview, path, FALSE);
			gtk_tree_path_free (path);
			node = g_list_next (node);
		}
	}
}

static void
session_save_node_expansion_states (ProjectDBase *p)
{
	GList *expansion_states = NULL;
	GList *node;
	gint   nIndex = 0;

	/* Project tree */
	expansion_states =
		tree_view_get_expansion_states (GTK_TREE_VIEW (p->widgets.treeview));
	session_clear_section (p, SECSTR (SECTION_PROJECT_TREE));
	node = expansion_states;
	while (node)
	{
		session_save_string_n (p, SECSTR (SECTION_PROJECT_TREE),
							   nIndex, node->data);
		nIndex++;
		node = node->next;
	}
	glist_strings_free (expansion_states);
	expansion_states = NULL;
	
	/* Symbol tree */
	nIndex = 0;
	expansion_states = sv_get_node_expansion_states ();
	session_clear_section (p, SECSTR (SECTION_SYMBOL_TREE));
	node = expansion_states;
	while (node)
	{
		session_save_string_n (p, SECSTR (SECTION_SYMBOL_TREE),
							   nIndex, node->data);
		nIndex++;
		node = node->next;
	}
	glist_strings_free (expansion_states);
	expansion_states = NULL;
	
	/* File tree */
	nIndex = 0;
	expansion_states = fv_get_node_expansion_states ();
	session_clear_section (p, SECSTR (SECTION_FILE_TREE));
	node = expansion_states;
	while (node)
	{
		session_save_string_n (p, SECSTR (SECTION_FILE_TREE),
							   nIndex, node->data);
		nIndex++;
		node = node->next;
	}
	glist_strings_free (expansion_states);
	expansion_states = NULL;
}

static void
session_load_node_expansion_states (ProjectDBase *p)
{
	GList *expansion_states = NULL;
	GList *node;
	gpointer config_iterator;

	g_return_if_fail (p != NULL);

	/* Project tree */
	config_iterator = session_get_iterator (p, SECSTR (SECTION_PROJECT_TREE));
	if (config_iterator !=  NULL)
	{
		gchar *szItem, *szNode;
		while ((config_iterator = gnome_config_iterator_next (config_iterator,
															  &szItem,
															  &szNode)))
		{
			if (szNode)
				expansion_states = g_list_prepend (expansion_states, szNode);
#ifdef DEBUG
			g_message ("Project Tree: %s", szNode);
#endif
			g_free (szItem);
			szItem = NULL;
			szNode = NULL;
		}
	}
	tree_view_set_expansion_states (GTK_TREE_VIEW (p->widgets.treeview),
									expansion_states);
	glist_strings_free (expansion_states);
	expansion_states = NULL;
	
	/* Symbol tree */
	config_iterator = session_get_iterator (p, SECSTR (SECTION_SYMBOL_TREE));
	if (config_iterator !=  NULL)
	{
		gchar *szItem, *szNode;
		while ((config_iterator = gnome_config_iterator_next (config_iterator,
															  &szItem,
															  &szNode)))
		{
			if (szNode)
				expansion_states = g_list_prepend (expansion_states, szNode);
#ifdef DEBUG
			g_message ("Symbol Tree: %s", szNode);
#endif
			g_free (szItem);
			szItem = NULL;
			szNode = NULL;
		}
	}
	sv_set_node_expansion_states (expansion_states);
	glist_strings_free (expansion_states);
	expansion_states = NULL;
	
	/* File tree */
	config_iterator = session_get_iterator (p, SECSTR (SECTION_FILE_TREE));
	if (config_iterator !=  NULL)
	{
		gchar *szItem, *szNode;
		while ((config_iterator = gnome_config_iterator_next (config_iterator,
															  &szItem,
															  &szNode)))
		{
			if (szNode)
				expansion_states = g_list_prepend (expansion_states, szNode);
#ifdef DEBUG
			g_message ("File Tree: %s", szNode);
#endif
			g_free (szItem);
			szItem = NULL;
			szNode = NULL;
		}
	}
	fv_set_node_expansion_states (expansion_states);
	glist_strings_free (expansion_states);
	expansion_states = NULL;
}

static void
project_dbase_save_session (ProjectDBase * p)
{
	debugger_save_session_breakpoints (p);
	project_dbase_save_session_files (p);
	fv_session_save (p);
	session_save_node_expansion_states (p);
	find_replace_save_session (app->find_replace, p);
	executer_save_session (app->executer, p);
	find_in_files_save_session (app->find_in_files, p);
	session_sync();
}

void
project_dbase_close_project (ProjectDBase * p)
{
	GList *node;

	g_return_if_fail (p != NULL);
	g_return_if_fail (p->project_is_open == TRUE);
	
	if (p->is_saved == FALSE)
	{
		GtkWidget *dialog;
		gint but;
		
		dialog = create_project_confirm_dlg (p->widgets.window);
		but = gtk_dialog_run (GTK_DIALOG (dialog));
		switch (but)
		{
			case GTK_RESPONSE_YES:
				project_dbase_save_project (p);
				break;
			case GTK_RESPONSE_NO:
				p->is_saved = TRUE;
				break;
			default:
				gtk_widget_destroy (dialog);
				return;
		}
		gtk_widget_destroy (dialog);
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
	if (p->tm_project)
	{
		tm_project_free(p->tm_project);
		p->tm_project = NULL;
	}
	project_dbase_hide (p);
	project_dbase_update_menu (p);	
	project_dbase_clean_left (p);
	sv_clear();
	fv_clear();
	p->project_is_open = FALSE;
    gtk_widget_set_sensitive(app->widgets.menubar.file.recent_projects, TRUE);
}

void
project_dbase_update_docked_status(void)
{
	gboolean status = app->project_dbase->project_is_open & app->project_dbase->is_docked;
	GTK_CHECK_MENU_ITEM(app->widgets.menubar.project.dock_undock)->active = status;
	GTK_CHECK_MENU_ITEM(app->project_dbase->widgets.menu_docked)->active = status;
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
		}
		else
			p->is_docked = TRUE;
		project_dbase_update_docked_status();
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
		}
		else
			p->is_docked = FALSE;
		project_dbase_update_docked_status();
	}
}

gboolean
project_dbase_is_file_in_module (ProjectDBase * p, PrjModule module,
				 gchar * file)
{
	gchar *tmp;
	gchar *real_fn;
	gchar *mod;
	gchar *mdir;
	GList *files, *node;

	if (!p || !file)
		return FALSE;
	if (p->project_is_open == FALSE)
		return FALSE;

	tmp = g_strconcat ("module.", module_map[module], ".name", NULL);
	mod = prop_get (p->props, tmp);
	g_free (tmp);
	mdir = g_strconcat (p->top_proj_dir, "/", mod, NULL);
	g_free (mod);
	if (!is_file_in_dir(file, mdir))
	{
		g_free(mdir);
		return FALSE;
	}

	files = project_dbase_get_module_files(p, module);
	for (node = files; node; node = g_list_next(node))
	{
		tmp = g_strconcat(mdir, "/", node->data, NULL);
		real_fn = tm_get_real_path(tmp);
		g_free(tmp);
		if (0 == strcmp(file, real_fn))
		{
			glist_strings_free (files);
			g_free(real_fn);
			g_free(mdir);
			return TRUE;
		}
		g_free(real_fn);
	}
	glist_strings_free (files);
	g_free(mdir);
	return FALSE;
}

void
project_dbase_show_info (ProjectDBase * p)
{
	gchar *str[14];
	gint i;
	GList *list;
	ProjectType* type;
	
	g_return_if_fail (p != NULL);
	g_return_if_fail (p->project_is_open == TRUE);

	str[0] = project_dbase_get_proj_name(p);
	str[1] = prop_get (p->props, "project.version");
	str[2] = prop_get (p->props, "project.author");
	type = project_dbase_get_project_type(p); 
	if (type->glade_support)
		str[3] = g_strdup (_("Yes"));
	else
		str[3] = g_strdup (_("No"));
	
	project_type_free (type);
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
project_dbase_edit_gui (ProjectDBase *p)
{
	gchar *edit_command;
	gchar *filename, *prj_name;
	gboolean ret;

	if (p->project_is_open == FALSE)
		return FALSE;
	
	
	edit_command = prop_get_expanded (p->props, "project.gui.command");

	if (edit_command  && strlen (edit_command) > 0)
	{
#ifdef DEBUG
		g_message ("GUI editing command: %s", edit_command);
#endif
		gnome_execute_shell (p->top_proj_dir, edit_command);
		g_free (edit_command);
		return TRUE;
	}
	prj_name = project_dbase_get_proj_name (p);
	filename = g_strdup_printf ("%s/%s.glade",
				    p->top_proj_dir,
				    prj_name);
	g_free (prj_name);

	if (file_is_regular (filename) == FALSE)
	{
		anjuta_error (
			_("A .glade file does not "
			"exist in the top level Project directory. "
			"If you do not use glade for GUI editing, "
			" please specify a custom command for it in "
			"[Project]->[Project Configuration]->[GUI editor command]"));
		ret = FALSE;
	}
	else
		ret = glade_iface_start_glade_editing (filename);

	g_free (filename);

	return ret;
}

gboolean
project_dbase_generate_source_code (ProjectDBase *p)
{
	gchar *filename, *prj_name;
	gboolean ret;
	ProjectType* type;
	
	if (p->project_is_open == FALSE)
		return FALSE;

	type = project_dbase_get_project_type(p);
	
	/* wxWindows has a special 'main program' equivalent,
	 * so use an extra function for writing its main.cc */
	if(type->id == PROJECT_TYPE_WXWIN)
	{
		project_type_free (type);
		return source_write_wxwin_main_c(p);
	}
	if(type->id == PROJECT_TYPE_XWIN)
	{
		project_type_free (type);
		return source_write_xwin_main_c(p);        
	}
	if(type->id == PROJECT_TYPE_XWINDOCKAPP)
	{
		project_type_free (type);
		return source_write_xwindockapp_main_c(p);        
	}

	
	if (!type->glade_support
		|| project_dbase_get_target_type(p) != PROJECT_TARGET_TYPE_EXECUTABLE)
	{
		project_type_free (type);
		return source_write_generic_main_c (p);
	}

	prj_name = project_dbase_get_proj_name (p);
	filename = g_strdup_printf ("%s/%s.glade",
				    p->top_proj_dir,
				    prj_name);
	g_free (prj_name);

	if (file_is_regular (filename) == FALSE)
	{
		anjuta_error (
			_("A .glade file does not\n"
			"exist in the top level Project directory."));
		ret = source_write_generic_main_c (p);
	}
	else
	{
		if (type->id == PROJECT_TYPE_LIBGLADE)
			ret = source_write_libglade_main_c (p);
		else if (type->id == PROJECT_TYPE_LIBGLADE2)
			ret = source_write_libglade2_main_c (p);
		else if ((ret = glade_iface_generate_source_code (filename)) == FALSE)
			ret = source_write_generic_main_c (p);
	}
	g_free (filename);
	project_type_free (type);
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
	gchar *real_target;

	if (!p)
		return NULL;
	if (p->project_is_open == FALSE)
		return NULL;
	target = prop_get (p->props, "project.source.target");
	src_dir = project_dbase_get_module_dir (p, MODULE_SOURCE);
	str = g_strconcat (src_dir, "/", target, NULL);
	real_target = tm_get_real_path(str);
	g_free (target);
	g_free (src_dir);
	g_free(str);
	return real_target;
}

gchar *
project_dbase_get_version (ProjectDBase * p)
{
	if (!p) return NULL;
	if (p->project_is_open == FALSE)
		return NULL;
	return prop_get (p->props, "project.version");
}

/* project type. Must be freed after use!!!*/
ProjectType*
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
			return project_type_load (i);
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
	
	g_return_val_if_fail (p != NULL, PROJECT_TARGET_TYPE_END_MARK);
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
	gchar *mod_name, *dir = NULL;

	g_return_val_if_fail (p != NULL, NULL);
	g_return_val_if_fail (module < MODULE_END_MARK, NULL);

	if (p->project_is_open == FALSE)
		return NULL;
	if (NULL != (mod_name = project_dbase_get_module_name (p, module)))
	{
	dir = g_strconcat (p->top_proj_dir, "/", mod_name, NULL);
		g_free(mod_name);
	}
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
	if(str[0] == '\0')
	{
		g_free (str);
		return TRUE;
	}
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

static gboolean
restore_preference_property (AnjutaPreferences *pr,
							 const gchar *key, gpointer data)
{
	gchar *str, *str_prop;
	ProjectDBase *p = data;

	str = prop_get (pr->props_session, key);
	if (str) {
		prop_set_with_key(pr->props, key, str);
		g_free(str);
	} else {
		prop_set_with_key(pr->props, key, "");
	}
	return TRUE;
}

void
project_dbase_clean_left (ProjectDBase * p)
{
	/* Clear project related preferences that have been set. */
#ifdef DEBUG
	g_message ("Clearing project preferences\n");
#endif
	anjuta_preferences_foreach (ANJUTA_PREFERENCES (app->preferences),
								ANJUTA_PREFERENCES_FILTER_PROJECT,
								restore_preference_property, p);
	project_dbase_clear (p);
	project_config_clear (p->project_config);

	compiler_options_load (app->compiler_options,
						   ANJUTA_PREFERENCES (app->preferences)->props);
	src_paths_load (app->src_paths,
				    ANJUTA_PREFERENCES (app->preferences)->props);
	anjuta_apply_styles ();
}

/*
 * Private functions: Do not use
 */
/*
	Here we handle juggling the user interface to hide/show the project pane.

	Note that the current *fully-populated* Anjuta interface consists of a vbox 
	which in turn contains a vpane.  The vpane contains the message pane at
	the bottom, and an hpane at the top.  The hpane contains the project pane
	on the left, and a notebook on the right.  The notebook contains multiple
	Scintilla pages.  Note that this is the fully-populated interface, as it is
	initially created.  Since the project and message panes can be hidden or
	detached, things are not always this way!  

	Actually, the above description applies to the client area of the main window,
	and ignores the menus and toolbars, etc.
	
	Previously, juggling the interface was done by a bunch of gtk_container_add()
	and gtk_container_remove() calls.  Unfortunately, this lead to a bug in
	selection handling (i.e. copy/paste) for the Scintilla pages.  To fix this,
	I've switched to using gtk_widget_reparent(), and all is well.  By the way,
	the bug was due to the internal gtk/gdk selection handling code which looks
	at the gdk window for the selection owner.  When a widget is removed from a
	container, its gdk window is destroyed, even if the widget is not.  This means
	that gtk/gdk gets confused when a widget that owns the selection has its gdk
	window changed.  And yes, using gtk_widget_reparent() preserves the gdk window.
	
	Note that the same arguments apply to the code in message-manager-dock.c.
*/

void
project_dbase_detach (ProjectDBase * p)
{
	gtk_widget_reparent (app->project_dbase->widgets.client,
						 app->project_dbase->widgets.client_area);
	gtk_widget_reparent (app->widgets.notebook, app->widgets.client_area);

	if (app->widgets.the_client == app->widgets.vpaned)
	{
		gtk_container_remove (GTK_CONTAINER (app->widgets.vpaned),
							  app->widgets.hpaned);
		gtk_widget_reparent (app->widgets.notebook, app->widgets.vpaned);
	}
	else
	{
		gtk_container_remove (GTK_CONTAINER (app->widgets.client_area),
							  app->widgets.hpaned);
		app->widgets.the_client = app->widgets.notebook;
	}
	app->widgets.hpaned_client = app->widgets.notebook;
}


void
project_dbase_attach (ProjectDBase * p)
{
	gtk_widget_reparent (app->project_dbase->widgets.client,
		app->widgets.project_dbase_win_container);
	gtk_widget_reparent (app->widgets.notebook,app->widgets.client_area);

	if (app->widgets.the_client == app->widgets.vpaned)
	{
		gtk_container_add (GTK_CONTAINER (app->widgets.vpaned), app->widgets.hpaned);
	}
	else
	{
		gtk_container_add (GTK_CONTAINER(app->widgets.client_area), app->widgets.hpaned);

		app->widgets.the_client = app->widgets.hpaned;
	}
	
	
	gtk_widget_reparent (app->widgets.notebook,app->widgets.hpaned);
	
	app->widgets.hpaned_client = app->widgets.hpaned;
	
	gtk_widget_show (app->widgets.the_client);
}

void
project_dbase_update_menu (ProjectDBase * p)
{
	GtkWidget *submenu;
	gint max_recent_prjs;

	max_recent_prjs =
		anjuta_preferences_get_int (ANJUTA_PREFERENCES (app->preferences),
				     				MAXIMUM_RECENT_PROJECTS);
	if (p->project_is_open)
	{
		app->recent_projects =
			glist_path_dedup(update_string_list (app->recent_projects,
												 p->proj_filename,
												 max_recent_prjs));
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
	gint i;

	GtkTreeStore *store;
	GtkTreeIter iter, parent, sub_parent;
	GdkPixbuf *pixbuf;
	ProjectFileData *pfd;
	GList *saved_map = NULL;

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

	saved_map =
		tree_view_get_expansion_states (GTK_TREE_VIEW (p->widgets.treeview));
	
	/* Add the root in the ctree */
	project_dbase_clear_ctree(p);
	
	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (p->widgets.treeview)));
	pixbuf = anjuta_res_get_pixbuf (ANJUTA_PIXMAP_CLOSED_FOLDER);
	pfd = project_file_data_new (MODULE_SOURCE, NULL, NULL);
	gtk_tree_store_append (store, &parent, NULL);
	gtk_tree_store_set (store, &parent, PROJECT_PIX_COLUMN, pixbuf,
						PROJECT_NAME_COLUMN, tmp3, PROJECT_DATA_COLUMN, pfd, -1);
	g_free (tmp3);
	
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
		{
			g_free(tmp1);
			continue;
		}

		pixbuf = anjuta_res_get_pixbuf (ANJUTA_PIXMAP_CLOSED_FOLDER);
		pfd = project_file_data_new (i, NULL, NULL);
		if (strcmp(tmp1, ".") == 0) {
			tmp3 = g_strdup (module_map[i]);
			prefix = g_strdup(p->top_proj_dir);
		} else {
			tmp3 = g_strconcat (module_map[i], " - ", tmp1, NULL);
			prefix = g_strconcat (p->top_proj_dir, "/", tmp1, NULL);
		}
		gtk_tree_store_append (store, &sub_parent, &parent);
		gtk_tree_store_set (store, &sub_parent,
							PROJECT_PIX_COLUMN, pixbuf,
							PROJECT_NAME_COLUMN, tmp3,
							PROJECT_DATA_COLUMN, pfd, -1);
		g_free (tmp3);
		g_free (tmp1);
		
		gtree_insert_files (GTK_TREE_VIEW (p->widgets.treeview),
							&sub_parent, i, prefix, list);
		g_free (prefix);
		glist_strings_free (list);
	}
	/* Expand first node */
	{
		GtkTreePath *path;
		
		path = gtk_tree_model_get_path (GTK_TREE_MODEL (store),
										&sub_parent);
		gtk_tree_view_expand_row (GTK_TREE_VIEW (p->widgets.treeview),
								  path, FALSE);
		gtk_tree_path_free (path);
	}
	tree_view_set_expansion_states (GTK_TREE_VIEW (p->widgets.treeview),
									saved_map);
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
	
	gtk_widget_set_sensitive (app->widgets.menubar.cvs.update_project, p);
	gtk_widget_set_sensitive (app->widgets.menubar.cvs.commit_project, p);
	gtk_widget_set_sensitive (app->widgets.menubar.cvs.import_project, p);
	gtk_widget_set_sensitive (app->widgets.menubar.cvs.status_project, p);
	gtk_widget_set_sensitive (app->widgets.menubar.cvs.log_project, p);
	gtk_widget_set_sensitive (app->widgets.menubar.cvs.diff_project, p);
}


void
project_dbase_add_file_to_module (ProjectDBase * p, PrjModule module,
				  gchar * filename)
{
	gchar *mod_files, *file_list, *new_file_list, *comp_dir;
	gchar *relative_fn;
	
	AnjutaPreferences *pr = ANJUTA_PREFERENCES (app->preferences);
	gboolean build_sv = anjuta_preferences_get_int(pr, BUILD_SYMBOL_BROWSER);
	gboolean build_fv = anjuta_preferences_get_int(pr, BUILD_FILE_BROWSER);

	g_return_if_fail (p != NULL);
	g_return_if_fail (p->sel_module < MODULE_END_MARK);

	if (NULL == (comp_dir = project_dbase_get_module_dir (p, module)))
	{
		g_warning("Unable to get component directory!");
		return;
	}
	relative_fn = get_relative_file_name(comp_dir, filename);
	if (!relative_fn)
	{
		anjuta_error(_("Unable to get relative file name for %s\n in %s"), filename, comp_dir);
		g_free(comp_dir);
		return;
	}
	g_free(comp_dir);

	if (NULL == (mod_files = g_strconcat ("module.", module_map[module], ".files", NULL)))
	{
		g_warning("mod_files for %s is NULL", module_map[module]);
		g_free(relative_fn);
		return;
	}
	file_list = prop_get (p->props, mod_files);
	if (!file_list)
		file_list = g_strdup ("");
	new_file_list = g_strconcat (file_list, " ", relative_fn, NULL);
	prop_set_with_key (p->props, mod_files, new_file_list);
	g_free (new_file_list);
	g_free (file_list);
	g_free (mod_files);
	g_free(relative_fn);
	if ((MODULE_INCLUDE == module) || (MODULE_SOURCE == module))
	{
		GList *tmp;
		TextEditor *te;

		tm_project_add_file(TM_PROJECT(p->tm_project), filename, TRUE);
		for (tmp = app->text_editor_list; tmp; tmp = g_list_next(tmp))
		{
			te = (TextEditor *) tmp->data;
			if (te && !te->tm_file && (0 == strcmp(te->full_filename, filename)))
				te->tm_file = tm_workspace_find_object(TM_WORK_OBJECT(app->tm_workspace)
				  , filename, FALSE);
		}
	}
	project_dbase_update_tree (p);
	sv_populate(build_sv);
	fv_populate(build_fv);
	p->is_saved = FALSE;
}

void
project_dbase_import_file_real (ProjectDBase* p, PrjModule selMod, gchar* filename)
{
	gchar *comp_dir;
	GList *list, *mod_files;
	gchar full_fn[PATH_MAX];
	gchar *real_fn;
	gchar *real_modfile;

	selMod = p->sel_module;
	comp_dir = project_dbase_get_module_dir (p, selMod);
	mod_files = project_dbase_get_module_files (p, selMod);
	real_fn = tm_get_real_path(filename);
	list = mod_files;
	while (list)
	{
		g_snprintf(full_fn, PATH_MAX, "%s/%s", comp_dir, (gchar *) list->data);
		real_modfile = tm_get_real_path(full_fn);
		if (0 == strcmp(real_modfile, real_fn))
		{
			/*
			 * file has already been added. So skip with a message 
			 */
			gchar *message = g_strconcat(real_modfile,
										 _(" already exists in the project"),
										 NULL); 
			anjuta_information (message);
			g_free(message);
			g_free (comp_dir);
			g_free(real_modfile);
			g_free(real_fn);
			glist_strings_free (mod_files);
			return;
		}
		g_free(real_modfile);
		list = g_list_next (list);
	}
	glist_strings_free (mod_files);
	/*
	 * File has not been added. So add it 
	 */
	if (!is_file_in_dir(filename, comp_dir))
	{
		gchar* fn;
		/*
		 * File does not exist in the corroeponding dir. So, import it. 
		 */
		fn =
			g_strconcat (comp_dir, "/",
				     extract_filename (filename), NULL);
		force_create_dir (comp_dir);
		if (!copy_file (filename, fn, TRUE))
		{
			g_free (comp_dir);
			g_free (fn);
			g_free(real_fn);
			anjuta_information (_
					("Error while copying the file inside the module."));
			return;
		}
		g_free(real_fn);
		real_fn = tm_get_real_path(fn);
		g_free(fn);
	}
	project_dbase_add_file_to_module (p, selMod, real_fn);
	g_free (comp_dir);
	g_free(real_fn);
	return;
}

void
project_dbase_remove_file (ProjectDBase * p)
{
	gchar *key, *fn, *files, *pos;
	gchar *cmp_dir, *full_fn;
	gint i;
	TMWorkObject *source_file;
	PrjModule module;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter, parent;

	module = p->current_file_data->module;
	key = g_strconcat ("module.", module_map[module], ".files", NULL);
	files = prop_get (p->props, key);
	if (files == NULL)
	{
		g_free(key);
		return;
	}
	
	/* fn = extract_filename (p->current_file_data->filename); */
	if (NULL == (fn = p->current_file_data->filename))
	{
		g_free(files);
		g_free(key);
		return;
	}
	pos = strstr (files, fn);
	if (pos == NULL)
	{
		g_free (files);
		g_free(key);
		return;
	}
	for (i=0; i< strlen(fn); i++)
		*pos++ = ' ';
	if (NULL == (cmp_dir = project_dbase_get_module_dir (p, module)))
	{
		g_warning("Unable to get component directory!");
		g_free(files);
		g_free(key);
		return;
	}
	full_fn = g_strconcat(cmp_dir, "/", fn, NULL);
	source_file = tm_project_find_file(p->tm_project, full_fn, FALSE);
	if (source_file)
	{
		GList *node;
		TextEditor *te;
		tm_project_remove_object(TM_PROJECT(p->tm_project), source_file);
		for (node = app->text_editor_list; node; node = g_list_next(node))
		{
			te = (TextEditor *) node->data;
			if (te && (source_file == te->tm_file))
				te->tm_file = NULL;
		}
	}
 	else
 		g_warning("Unable to find %s in project", full_fn);
	g_free(cmp_dir);
	g_free(full_fn);
	prop_set_with_key (p->props, key, files);
	g_free(files);
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (p->widgets.treeview));
	if (gtk_tree_selection_get_selected (selection, &model, &iter))
	{
		gboolean has_parent;
		
		has_parent = gtk_tree_model_iter_parent (model, &parent, &iter);
		gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);
		p->is_saved = FALSE;
	
		/* Check if the module is empty */
		files = prop_get (p->props, key);
		g_free(key);
		if (files == NULL && has_parent)
			/* Module is empty so remove the module */
			gtk_tree_store_remove (GTK_TREE_STORE (model), &parent);
		g_free (files);
	}
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

static gboolean
load_preferences_property (AnjutaPreferences *pr,
						   const gchar *key, gpointer data)
{
	ProjectDBase *p;
	gchar *str_prop, *str;
	
	p = data;
	g_return_val_if_fail (p, TRUE);
	str_prop = g_strdup_printf ("preferences.%s", key);			
	str = prop_get (p->props, str_prop);
	g_free(str_prop);
	if (str)
	{
		prop_set_with_key (pr->props, key, str);
		g_free(str);
	}
	return TRUE;
}

gboolean
project_dbase_load_project_file (ProjectDBase * p, gchar * filename)
{
	gchar *prj_buff, buff[512], *str;
	gint level, read_size, pos, i;
	gboolean error_shown, syserr, prefs_changed;
	FILE* fp;
	
	prj_buff = NULL;
	fp = NULL;
	error_shown = FALSE;
	syserr = FALSE;
	pos = 0;
	
	if (p->project_is_open == TRUE)
		project_dbase_clean_left (p);

	p->project_is_open = TRUE;
	gtk_widget_set_sensitive(app->widgets.menubar.file.recent_projects, FALSE);
	p->proj_filename = g_strdup(filename);
	
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
			anjuta_error ("Unknown compatibility level of the Project");
			error_shown = TRUE;
			goto go_error;
		}
	}
	if (level > COMPATIBILITY_LEVEL)
	{
		anjuta_error (_("Anjuta version %s or later is required"
					  " to open this Project.\n"
					  "Please upgrade to the latest version of Anjuta"
					  " (Help for more information)."), buff);
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
		anjuta_preferences_set (ANJUTA_PREFERENCES (app->preferences),
							   "anjuta.program.parameters", str);
	else
		anjuta_preferences_set (ANJUTA_PREFERENCES (app->preferences),
							   "anjuta.program.parameters", "");
	g_free (str);	
	
	/* some preferences may be stored in project file */
#ifdef DEBUG
	g_message ("Loading editor preferences from project");
#endif
	/* Save the current preferences in session database */
	anjuta_preferences_sync_to_session (ANJUTA_PREFERENCES (app->preferences));
	
	/* Transfer preferences from project to preferences database */
	anjuta_preferences_foreach (ANJUTA_PREFERENCES (app->preferences),
								ANJUTA_PREFERENCES_FILTER_PROJECT,
								load_preferences_property, p);
	/* Update the preferences */
	anjuta_apply_styles ();
	
	p->is_saved = TRUE;
	p->top_proj_dir = tm_get_real_path(p->proj_filename);
	*(strrchr(p->top_proj_dir, '/')) = '\0';
	prop_set_with_key (p->props, "top.proj.dir", p->top_proj_dir);
	p->has_cvs = is_cvs_active_for_dir(p->top_proj_dir);

	compiler_options_load (app->compiler_options, p->props);
	src_paths_load (app->src_paths, p->props);
	/* Project loading completed */
	return TRUE;
	
go_error:
	if (!error_shown) /* If error is not yet shown */
	{
		if (syserr)
			anjuta_system_error (errno, _("Error in loading Project: %s"),
								 p->proj_filename);
		else
			anjuta_error (_("Error in loading Project: %s"), p->proj_filename);
	}
	prop_clear (p->props);
	string_assign (&p->proj_filename, NULL);
	p->proj_filename = NULL;
	gtk_widget_set_sensitive(app->widgets.menubar.file.recent_projects, TRUE);
	p->project_is_open = FALSE;
	if (prj_buff) g_free (prj_buff);
	if (fp) fclose (fp);
	return FALSE;
}

gboolean
project_dbase_load_project_finish (ProjectDBase * p, gboolean show_project)
{
	/* Now Project setup */
	project_dbase_update_tree (p);
	if (show_project)
		project_dbase_show (p);
	extended_toolbar_update ();
	anjuta_update_app_status(FALSE, NULL);
	anjuta_status (_("Project loaded successfully."));
	anjuta_set_active ();
	project_dbase_update_docked_status();
	project_dbase_reload_session(p);

	return TRUE;
}
