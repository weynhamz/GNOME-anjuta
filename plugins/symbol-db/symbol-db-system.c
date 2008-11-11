/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta_trunk
 * Copyright (C) Massimo Cora' 2008 <maxcvs@email.it>
 * 
 * anjuta_trunk is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * anjuta_trunk is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with anjuta_trunk.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */


#include "symbol-db-system.h"
#include "plugin.h"

#include <glib.h>
#include <glib/gstdio.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-launcher.h>
#include <libanjuta/interfaces/ianjuta-language.h>
#include <libgnomevfs/gnome-vfs.h>
#include <string.h>

struct _SymbolDBSystemPriv
{
	AnjutaLauncher  *single_package_scan_launcher;
	IAnjutaLanguage *lang_manager;
	SymbolDBEngine  *sdbe_globals;
	
	GQueue *sscan_queue;
	GQueue *engine_queue;
}; 

typedef struct _SingleScanData {
	SymbolDBSystem *sdbs;
	gchar *package_name;
	gchar *contents;
	gboolean engine_scan;

	PackageParseableCallback parseable_cb;
	gpointer parseable_data;
	
} SingleScanData;

typedef struct _EngineScanData {
	SymbolDBSystem *sdbs;
	gchar *package_name;	
	GList *cflags;
	gboolean special_abort_scan;
	GPtrArray *files_to_scan_array;		
	GPtrArray *languages_array;			
	
} EngineScanData;

enum
{
	SCAN_PACKAGE_START,
	SCAN_PACKAGE_END,
	SINGLE_FILE_SCAN_END,
	LAST_SIGNAL
};

static unsigned int signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (SymbolDBSystem, sdb_system, G_TYPE_OBJECT);

/* forward decl */
static void
on_pkg_config_exit (AnjutaLauncher * launcher, int child_pid,
				   int exit_status, gulong time_taken_in_seconds,
				   gpointer user_data);

static void
on_engine_package_scan_end (SymbolDBEngine *dbe, gpointer user_data);

static void
destroy_single_scan_data (SingleScanData *ss_data)
{
	g_return_if_fail (ss_data != NULL);
	
	g_free (ss_data->package_name);
	g_free (ss_data->contents);
	
	g_free (ss_data);
}

static void
destroy_engine_scan_data (EngineScanData *es_data)
{
	if (es_data->cflags)
	{
		g_list_foreach (es_data->cflags, (GFunc)g_free, NULL);
		g_list_free (es_data->cflags);
	}
	
	g_free (es_data->package_name);
	
	if (es_data->special_abort_scan == TRUE)
	{
		g_ptr_array_foreach (es_data->files_to_scan_array, (GFunc)g_free, NULL);
		g_ptr_array_free (es_data->files_to_scan_array, TRUE);
			
		g_ptr_array_foreach (es_data->languages_array, (GFunc)g_free, NULL);
		g_ptr_array_free (es_data->languages_array, TRUE);
	}
	g_free (es_data);
}

static void
on_engine_package_single_file_scan_end (SymbolDBEngine *dbe, gpointer user_data)
{
	SymbolDBSystem *sdbs;
	
	sdbs = SYMBOL_DB_SYSTEM (user_data);
	
	g_signal_emit (sdbs, signals[SINGLE_FILE_SCAN_END], 0);
}

static void
sdb_system_init (SymbolDBSystem *object)
{
	SymbolDBSystem *sdbs;

	sdbs = SYMBOL_DB_SYSTEM (object);
	sdbs->priv = g_new0 (SymbolDBSystemPriv, 1);

	/* create launcher for single global package scan */
	sdbs->priv->single_package_scan_launcher = anjuta_launcher_new ();
	anjuta_launcher_set_check_passwd_prompt (sdbs->priv->single_package_scan_launcher, 
											 FALSE);
	
	/* single scan launcher's queue */
	sdbs->priv->sscan_queue = g_queue_new ();		
	sdbs->priv->engine_queue = g_queue_new ();
	
}

static void
sdb_system_finalize (GObject *object)
{
	SymbolDBSystem *sdbs;
	SymbolDBSystemPriv *priv;
	
	sdbs = SYMBOL_DB_SYSTEM (object);
	priv = sdbs->priv;
	if (priv->single_package_scan_launcher) 
	{
		anjuta_launcher_reset (priv->single_package_scan_launcher);
		g_object_unref (priv->single_package_scan_launcher);
		priv->single_package_scan_launcher = NULL;		
	}

	/* free also the queue */
	g_queue_foreach (priv->sscan_queue, (GFunc)g_free, NULL);
	g_queue_free (priv->sscan_queue);
	priv->sscan_queue = NULL;	
	
	/* FIXME: missing engine queue */
	/* disconnect signals */
	g_signal_handlers_disconnect_by_func (G_OBJECT (priv->sdbe_globals),
									  on_engine_package_single_file_scan_end,
									  sdbs);
	
	
	
	G_OBJECT_CLASS (sdb_system_parent_class)->finalize (object);
}

static void
sdb_system_class_init (SymbolDBSystemClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	signals[SCAN_PACKAGE_START]
		= g_signal_new ("scan-package-start",
						G_OBJECT_CLASS_TYPE (object_class),
						G_SIGNAL_RUN_FIRST,
						G_STRUCT_OFFSET (SymbolDBSystemClass, scan_package_start),
						NULL, NULL,
						g_cclosure_marshal_VOID__UINT_POINTER, G_TYPE_NONE, 
						2,
						G_TYPE_INT,
						G_TYPE_POINTER);

	signals[SCAN_PACKAGE_END]
		= g_signal_new ("scan-package-end",
						G_OBJECT_CLASS_TYPE (object_class),
						G_SIGNAL_RUN_FIRST,
						G_STRUCT_OFFSET (SymbolDBSystemClass, scan_package_end),
						NULL, NULL,
						g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 
						1,
						G_TYPE_STRING);
	
	signals[SINGLE_FILE_SCAN_END]
		= g_signal_new ("single-file-scan-end",
						G_OBJECT_CLASS_TYPE (object_class),
						G_SIGNAL_RUN_FIRST,
						G_STRUCT_OFFSET (SymbolDBSystemClass, single_file_scan_end),
						NULL, NULL,
						g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	
	object_class->finalize = sdb_system_finalize;
}

/**
 * @return GList of cflags (strings) in a format like /usr/include/my_foo_lib. 
 * @return NULL on error.
 */
static GList *
sdb_system_get_normalized_cflags (const gchar *chars)
{
	gchar **flags;
	gint i;
	GList *good_flags;
	const gchar *curr_flag;
	
	/* We should receive here something like 
	* '-I/usr/include/gimp-2.0 -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include'.
	* Split up the chars and take a decision if we like it or not.
	*/
	flags = g_strsplit (chars, " ", -1);

	i = 0;
	/* if, after the while loop, good_flags is != NULL that means that we found
	 * some good flags to include for a future scan 
	 */
	good_flags = NULL;
	while ((curr_flag = flags[i++]) != NULL)
	{
		/* '-I/usr/include/gimp-2.0' would be good, but '/usr/include/' wouldn't. */
		if (g_regex_match_simple ("\\.*/usr/include/\\w+", curr_flag, 0, 0) == TRUE)
		{
			/* FIXME the +2. It's to skip the -I */
			DEBUG_PRINT ("adding %s to good_flags", curr_flag +2);
			/* FIXME the +2. It's to skip the -I */
			good_flags = g_list_prepend (good_flags, g_strdup (curr_flag + 2));
		}
	}	

	g_strfreev (flags);
	return good_flags;
}


SymbolDBSystem *
symbol_db_system_new (SymbolDBPlugin *sdb_plugin,
					  const SymbolDBEngine *sdbe)
{
	SymbolDBSystem *sdbs;
	SymbolDBSystemPriv *priv;

	g_return_val_if_fail (sdbe != NULL, NULL);
	sdbs = g_object_new (SYMBOL_TYPE_DB_SYSTEM, NULL);
	
	priv = sdbs->priv;
	priv->sdbe_globals = (SymbolDBEngine*)sdbe;
	
	priv->lang_manager = anjuta_shell_get_interface (ANJUTA_PLUGIN(sdb_plugin)->shell, 
													IAnjutaLanguage, NULL);

	g_signal_connect (G_OBJECT (priv->sdbe_globals), "single-file-scan-end",
	  			G_CALLBACK (on_engine_package_single_file_scan_end), sdbs);
	
	return sdbs;
}

/**
 * Check on globals db if the project 'package_name' is present or not.
 */
gboolean
symbol_db_system_is_package_parsed (SymbolDBSystem *sdbs, 
								   const gchar * package_name)
{
	SymbolDBSystemPriv *priv;
		
	g_return_val_if_fail (sdbs != NULL, FALSE);
	g_return_val_if_fail (package_name != NULL, FALSE);
	
	priv = sdbs->priv;
	
	return symbol_db_engine_project_exists (priv->sdbe_globals, 
											package_name);
}

static void
on_pkg_config_output (AnjutaLauncher * launcher,
					AnjutaLauncherOutputType output_type,
					const gchar * chars, gpointer user_data)
{
	SymbolDBSystem *sdbs;
	SymbolDBSystemPriv *priv;
	SingleScanData *ss_data;

	if (output_type == ANJUTA_LAUNCHER_OUTPUT_STDERR)
	{
		/* no way. We don't like errors on stderr... */
		return;
	}
	
	ss_data = (SingleScanData *)user_data;
	sdbs = ss_data->sdbs;
	priv = sdbs->priv;

	if (ss_data->contents != NULL) 
	{
		gchar *to_be_freed;
		to_be_freed = ss_data->contents;
		
		/* concatenate the output to the relative package's object */
		ss_data->contents = g_strconcat (ss_data->contents, chars, NULL);
		g_free (to_be_freed);
	}
	else 
	{
		ss_data->contents = g_strdup (chars);
	}
}

static GList **
sdb_system_files_visit_dir (GList **files_list, const gchar* uri)
{
	
	GList *files_in_curr_dir = NULL;
	
	if (gnome_vfs_directory_list_load (&files_in_curr_dir, uri,
								   GNOME_VFS_FILE_INFO_GET_MIME_TYPE) == GNOME_VFS_OK) 
	{
		GList *node;
		node = files_in_curr_dir;
		do {
			GnomeVFSFileInfo* info;
						
			info = node->data;
			
			if (info->type == GNOME_VFS_FILE_TYPE_DIRECTORY) 
			{				
				if (strcmp (info->name, ".") == 0 ||
					strcmp (info->name, "..") == 0)
					continue;

				gchar *tmp = g_strdup_printf ("%s/%s", uri, info->name);
				
				/* recurse */
				files_list = sdb_system_files_visit_dir (files_list, tmp);
				
				g_free (tmp);
			}
			else 
			{
				gchar *local_path;
				gchar *tmp = g_strdup_printf ("%s/%s", 
									uri, info->name);
			
				local_path = gnome_vfs_get_local_path_from_uri (tmp);
				*files_list = g_list_prepend (*files_list, local_path);
				g_free (tmp);
			}
		} while ((node = node->next) != NULL);		
	}	 
	
	return files_list;
}

static void
prepare_files_to_be_scanned (SymbolDBSystem *sdbs,
							 GList *cflags, 
							 GPtrArray *OUT_files_to_scan_array, 
							 GPtrArray *OUT_languages_array)
{
	SymbolDBSystemPriv *priv;
	GList *node;
	
	priv = sdbs->priv;	
	node = cflags;	
	
	do {
		GList *files_tmp_list = NULL;
		gchar *uri;
		
		uri = gnome_vfs_get_uri_from_local_path (node->data);
		
		/* files_tmp_list needs to be freed */
		sdb_system_files_visit_dir (&files_tmp_list, uri);
		g_free (uri);
		
		if (files_tmp_list != NULL) 
		{			
			/* last loop here. With files_visit_dir we'll retrieve all files nodes
			 * under the passed directory 
			 */
			GList *tmp_node;
			tmp_node = files_tmp_list;
			do {
				const gchar* file_mime;
				IAnjutaLanguageId lang_id;
				const gchar* lang;
				file_mime = gnome_vfs_get_mime_type_for_name (tmp_node->data);
		
				lang_id = ianjuta_language_get_from_mime_type (priv->lang_manager, file_mime, 
													   NULL);
		
				/* No supported language... */
				if (!lang_id)
				{
					continue;
				}
			
				lang = ianjuta_language_get_name (priv->lang_manager, lang_id, NULL);				
		
				g_ptr_array_add (OUT_languages_array, g_strdup (lang));				
				g_ptr_array_add (OUT_files_to_scan_array, 
								 g_strdup (tmp_node->data));
			} while ((tmp_node = tmp_node->next) != NULL);		
			
			/* free the tmp files list */
			g_list_foreach (files_tmp_list, (GFunc)g_free, NULL);
			g_list_free (files_tmp_list);
		}
	} while ((node = node->next) != NULL);
}

static inline void
sdb_system_do_engine_scan (SymbolDBSystem *sdbs, EngineScanData *es_data)
{
	SymbolDBSystemPriv *priv;
	GPtrArray *files_to_scan_array; 
	GPtrArray *languages_array;
	
	priv = sdbs->priv;

	/* will be disconnected automatically when callback is called. */
	g_signal_connect (G_OBJECT (priv->sdbe_globals), "scan-end",
 			G_CALLBACK (on_engine_package_scan_end), es_data);

	if (es_data->special_abort_scan == FALSE)
	{
		files_to_scan_array = g_ptr_array_new ();
		languages_array = g_ptr_array_new();
		
		/* the above arrays will be populated with this function */
		prepare_files_to_be_scanned (sdbs, es_data->cflags, files_to_scan_array,
								 languages_array);

		symbol_db_engine_add_new_project (priv->sdbe_globals, NULL,
								  		es_data->package_name);
	}
	else 
	{
		files_to_scan_array = es_data->files_to_scan_array;
		languages_array = es_data->languages_array;
	}		
							
	
	/* notify the listeners about our intention of adding new files
	 * to the db 
	 */
	g_signal_emit (sdbs, signals[SCAN_PACKAGE_START], 0, 
				   files_to_scan_array->len,
				   es_data->package_name); 
			
	/* note the FALSE as last parameter: we don't want
	 * to re-scan an already present file. There's the possibility
	 * infact to have more references of the same files in different
	 * packages
	 */
	symbol_db_engine_add_new_files (priv->sdbe_globals,
							es_data->special_abort_scan == FALSE ? 
									es_data->package_name : NULL, 
							files_to_scan_array,
							languages_array,
							es_data->special_abort_scan == FALSE ? 
									FALSE : TRUE);
		
	/* hey, destroy_engine_scan_data () will take care of destroying these for us,
	 * in case we're on a special_abort_scan
	 */
	if (es_data->special_abort_scan == FALSE)
	{
		g_ptr_array_foreach (files_to_scan_array, (GFunc)g_free, NULL);
		g_ptr_array_free (files_to_scan_array, TRUE);
			
		g_ptr_array_foreach (languages_array, (GFunc)g_free, NULL);
		g_ptr_array_free (languages_array, TRUE);	
	}
}

static void
on_engine_package_scan_end (SymbolDBEngine *dbe, gpointer user_data)
{
	SymbolDBSystem *sdbs;
	SymbolDBSystemPriv *priv;
	EngineScanData *es_data;
	
	es_data = (EngineScanData *)user_data;
	sdbs = es_data->sdbs;
	priv = sdbs->priv;

	/* first of all disconnect the signals */
	g_signal_handlers_disconnect_by_func (dbe, on_engine_package_scan_end, 
										  user_data);

	/* notify listeners that we ended the scan of the package */
	g_signal_emit (sdbs, signals[SCAN_PACKAGE_END], 0, es_data->package_name); 
	
	/* remove the data from the queue */
	DEBUG_PRINT ("removing on_engine_package_scan_end %s", es_data->package_name);
	g_queue_remove (priv->engine_queue, es_data);
	destroy_engine_scan_data (es_data);	
	
	/* have we got something left in the queue? */
	if (g_queue_get_length (priv->engine_queue) > 0)
	{				
		/* peek the head */
		es_data = g_queue_peek_head (priv->engine_queue);
	
		sdb_system_do_engine_scan (sdbs, es_data);
	}
}

static inline void 
sdb_system_do_scan_package_1 (SymbolDBSystem *sdbs,							
							SingleScanData *ss_data)
{
	SymbolDBSystemPriv *priv;
	gchar *exe_string;
	priv = sdbs->priv;
	
	DEBUG_PRINT ("sdb_system_do_scan_package_1 SCANNING %s", 
				 ss_data->package_name);
	exe_string = g_strdup_printf ("pkg-config --cflags %s", 
								  ss_data->package_name);
	
	g_signal_connect (G_OBJECT (priv->single_package_scan_launcher), 
					  "child-exited", G_CALLBACK (on_pkg_config_exit), ss_data);	
	
	anjuta_launcher_execute (priv->single_package_scan_launcher,
							 	exe_string, on_pkg_config_output, 
							 	ss_data);	
	g_free (exe_string);	
}

/**
 * Scan the next package in queue, if exists.
 */
static void
sdb_system_do_scan_next_package (SymbolDBSystem *sdbs)
{
	SymbolDBSystemPriv *priv;
	priv = sdbs->priv;
	
	if (g_queue_get_length (priv->sscan_queue) > 0)
	{
		/* get the next one without storing it into queue */
		SingleScanData *ss_data = g_queue_peek_head (priv->sscan_queue);
		
		/* enjoy */
		sdb_system_do_scan_package_1 (sdbs, ss_data);
	}
}

/**
 * Scan a new package storing it in queue either for later retrieval or
 * for signaling the 'busy status'.
 */
static void
sdb_system_do_scan_new_package (SymbolDBSystem *sdbs,							
							SingleScanData *ss_data)
{
	SymbolDBSystemPriv *priv;	
	priv = sdbs->priv;
	
	if (g_queue_get_length (priv->sscan_queue) > 0)
	{
		/* there's something already working... being this function called in a 
		 * single-threaded fashion we can put the the next parameter on the queue 
		 */
		DEBUG_PRINT ("%s", "pushed on queue for later scanning");
		g_queue_push_tail (priv->sscan_queue, ss_data);
		return;
	}
	
	g_queue_push_tail (priv->sscan_queue, ss_data);
	sdb_system_do_scan_package_1 (sdbs, ss_data);
	return;
}

static void
on_pkg_config_exit (AnjutaLauncher * launcher, int child_pid,
				   int exit_status, gulong time_taken_in_seconds,
				   gpointer user_data)
{
	SymbolDBSystem *sdbs;
	SymbolDBSystemPriv *priv;
	SingleScanData *ss_data;
	GList *cflags = NULL;
	
	ss_data = (SingleScanData *)user_data;
	sdbs = ss_data->sdbs;
	priv = sdbs->priv;	
	
	/* first of all disconnect the signals */
	g_signal_handlers_disconnect_by_func (launcher, on_pkg_config_exit,
										  user_data);
	
	if (ss_data->contents != NULL && strlen (ss_data->contents) > 0)
	{		
		cflags = sdb_system_get_normalized_cflags (ss_data->contents);
	}	
	
	/* check our ss_data struct. If it has a != null callback then we should
	 * call it right now..
	 */
	if (ss_data->parseable_cb != NULL)
	{
		DEBUG_PRINT ("%s", "on_pkg_config_exit parseable activated");
		ss_data->parseable_cb (sdbs, cflags == NULL ? FALSE : TRUE, 
							   ss_data->parseable_data);
	}

	/* no callback to call. Just parse the package on */
	if (ss_data->engine_scan == TRUE && cflags != NULL)
	{
		EngineScanData *es_data;
		
		es_data = g_new0 (EngineScanData, 1);
		es_data->sdbs = sdbs;
		es_data->cflags = cflags;
		es_data->package_name = g_strdup (ss_data->package_name);
		es_data->special_abort_scan = FALSE;
			
		/* is the engine queue already full && working? */
		if (g_queue_get_length (priv->engine_queue) > 0) 
		{
			/* just push the tail waiting for a later processing [i.e. after
			 * a scan-end received 
			 */
			DEBUG_PRINT ("pushing on engine queue %s", es_data->package_name);
			g_queue_push_tail (priv->engine_queue, es_data);
		}
		else
		{
			/* push the tail to signal a 'working engine' */
			DEBUG_PRINT ("scanning with engine queue %s", es_data->package_name);
			g_queue_push_tail (priv->engine_queue, es_data);
			
			sdb_system_do_engine_scan (sdbs, es_data);
		}
	}
	
	/* destroys, after popping, the ss_data from the queue */
	g_queue_remove (priv->sscan_queue, ss_data);
	destroy_single_scan_data (ss_data);
	
	/* proceed with another scan */	
	sdb_system_do_scan_next_package (sdbs);	
}

gboolean
symbol_db_system_scan_package (SymbolDBSystem *sdbs,
							  const gchar * package_name)
{
	SingleScanData *ss_data;
	SymbolDBSystemPriv *priv;
	
	g_return_val_if_fail (sdbs != NULL, FALSE);
	g_return_val_if_fail (package_name != NULL, FALSE);

	priv = sdbs->priv;
	
	/* does is already exist on db? */
	if (symbol_db_system_is_package_parsed (sdbs, package_name) == TRUE)
	{
		DEBUG_PRINT ("symbol_db_system_scan_package (): no need to scan %s",
					 package_name);
		return FALSE;
	}
	else 
	{
		DEBUG_PRINT ("symbol_db_system_scan_package (): NEED to scan %s",
					 package_name);
	}
	
	/* create the object to store in the queue */
	ss_data = (SingleScanData*)g_new0 (SingleScanData, 1);
	
	/* we don't have chars now. Just fill with the package_name */
	ss_data->sdbs = sdbs;
	ss_data->package_name = g_strdup (package_name);
	ss_data->contents = NULL;
	ss_data->parseable_cb = NULL;
	ss_data->parseable_data = NULL;
	ss_data->engine_scan = TRUE;
	
	/* package is a new one. No worries about scan queue */
	sdb_system_do_scan_new_package (sdbs, ss_data);
	return TRUE;
}

void 
symbol_db_system_is_package_parseable (SymbolDBSystem *sdbs, 
								   const gchar * package_name,
								   PackageParseableCallback parseable_cb,
								   gpointer user_data)
{
	SingleScanData *ss_data;
	SymbolDBSystemPriv *priv;
	
	g_return_if_fail (sdbs != NULL);
	g_return_if_fail (package_name != NULL);

	priv = sdbs->priv;

	/* create the object to store in the queue */
	ss_data = (SingleScanData*)g_new0 (SingleScanData, 1);
	
	/* we don't have chars now. Just fill with the package_name */
	ss_data->sdbs = sdbs;
	ss_data->package_name = g_strdup (package_name);
	ss_data->contents = NULL;
	ss_data->parseable_cb = parseable_cb;
	ss_data->parseable_data = user_data;
	/* this is just an info single_scan_data */
	ss_data->engine_scan = FALSE;

	/* package is a new one. No worries about scan queue */
	sdb_system_do_scan_new_package (sdbs, ss_data);
}

void 
symbol_db_system_parse_aborted_package (SymbolDBSystem *sdbs, 
								 GPtrArray *files_to_scan_array,
								 GPtrArray *languages_array)
{
	SymbolDBSystemPriv *priv;
	EngineScanData *es_data;
	
	g_return_if_fail (sdbs != NULL);
	g_return_if_fail (files_to_scan_array != NULL);
	g_return_if_fail (languages_array != NULL);
	
	priv = sdbs->priv;
	
	/* create a special EngineScanData */
	es_data = g_new0 (EngineScanData, 1);
	es_data->sdbs = sdbs;
	es_data->cflags = NULL;
	es_data->package_name = g_strdup (_("Resuming glb scan."));
	es_data->special_abort_scan = TRUE;
	es_data->files_to_scan_array = files_to_scan_array;
	es_data->languages_array = languages_array;
		
		
	/* is the engine queue already full && working? */
	if (g_queue_get_length (priv->engine_queue) > 0) 
	{
		/* just push the tail waiting for a later processing [i.e. after
		 * a scan-end received 
		 */
		DEBUG_PRINT ("pushing on engine queue %s", es_data->package_name);
		g_queue_push_tail (priv->engine_queue, es_data);
	}
	else
	{
		/* push the tail to signal a 'working engine' */
		g_queue_push_tail (priv->engine_queue, es_data);
		
		sdb_system_do_engine_scan (sdbs, es_data);
	}
}

