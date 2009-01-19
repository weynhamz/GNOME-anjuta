/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8; coding: utf-8 -*- */
/* gbf-mkfile-project.c
 *
 * Copyright (C) 2005  Eric Greveson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Eric Greveson
 * Based on the Autotools GBF backend (libgbf-am) by
 *   JP Rosevear
 *   Dave Camp
 *   Naba Kumar
 *   Gustavo Giráldez
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <libgnome/gnome-macros.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libanjuta/gbf-project.h>
#include <libanjuta/anjuta-utils.h>
#include "gbf-mkfile-project.h"
#include "gbf-mkfile-config.h"
#include "gbf-mkfile-properties.h"

#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
#define GBF_DEBUG(x) x
#else
#define GBF_DEBUG(x)
#endif


#define UNIMPLEMENTED  G_STMT_START { g_warning (G_STRLOC": unimplemented"); } G_STMT_END

/* Constant strings for parsing perl script error output */
#define ERROR_PREFIX      "ERROR("
#define WARNING_PREFIX    "WARNING("
#define MESSAGE_DELIMITER ": "


/* ----- Queue data types ----- */

/* FIXME: extend and make most operations asynchronous */
typedef enum {
	BUILD
} GbfMkfileProjectOpType;

typedef struct {
	GbfMkfileProject       *project;
	GbfMkfileProjectOpType  type;
	gchar 		   *build_id;
} GbfMkfileProjectOp;


/* ----- Change sets data types ----- */

typedef struct _GbfMkfileChange GbfMkfileChange;

typedef enum {
	GBF_MKFILE_CHANGE_ADDED,
	GBF_MKFILE_CHANGE_REMOVED
} GbfMkfileChangeType;

struct _GbfMkfileChange {
	GbfMkfileChangeType  change;
	GbfMkfileNodeType    type;
	gchar           *id;
};


/* ----- XML output parser data types ----- */

typedef struct _GbfMkfileProjectParseData GbfMkfileProjectParseData;

struct _GbfMkfileProjectParseData {
	GbfMkfileProject       *project;

	/* For tracking state */
	GNode                  *current_node;
	gint                    depth;          /* group depth only */
	GbfMkfileConfigMapping *config;
	gchar                  *param_key;
	gboolean                full_report;

	/* parser state */
	enum {
		PARSE_INITIAL,
		PARSE_DONE,
		
		PARSE_PROJECT,
		PARSE_GROUP,
		PARSE_TARGET,
		PARSE_SOURCE,
		PARSE_DEPENDENCY,
		PARSE_CONFIG,
		PARSE_PARAM,
		PARSE_ITEM,
		PARSE_PARAM_DONE,
		
		PARSE_ERROR
	} state, save_state;

	/* list of GbfAmChange with changed project elements */
	gboolean            compute_change_set;
	GSList             *change_set;

	/* hash table to keep track of possibly removed nodes */
	GHashTable         *nodes;
};


/* ----- Script spawning data types and constants ----- */

#define GBF_MKFILE_PARSE       SCRIPTS_DIR "/gbf-mkfile-parse"

/* timeout in milliseconds */
#define SCRIPT_TIMEOUT     30000

/* buffer size tuning parameters */
#define READ_BUFFER_SIZE   32768
#define READ_BUFFER_DELTA  16384

typedef struct _GbfMkfileSpawnData GbfMkfileSpawnData;
typedef struct _GbfMkfileChannel   GbfMkfileChannel;

struct _GbfMkfileChannel {
	GIOChannel *channel;
	gchar      *buffer;
	gsize       size;      /* allocated buffer size */
	gsize       length;    /* real buffer length/index into buffer */
	guint       tag;       /* main loop source tag for this channel's watch */
};
	
struct _GbfMkfileSpawnData {
	GMainLoop   *main_loop;
	
	/* child PID to kill it on timeout */
	gint         child_pid;
	
	/* buffers and related channels */
	GbfMkfileChannel input;
	GbfMkfileChannel output;
	GbfMkfileChannel error;
	gint         open_channels;
};


/* ----- Standard GObject types and variables ----- */

enum {
	PROP_0,
	PROP_PROJECT_DIR
};

static GbfProject *parent_class;


/* ----------------------------------------------------------------------
   Private prototypes
   ---------------------------------------------------------------------- */

static gboolean        uri_is_equal                 (const gchar       *uri1,
						     const gchar       *uri2);
static gboolean        uri_is_parent                (const gchar       *parent_uri,
						     const gchar       *child_uri);
static gboolean        uri_is_local_path            (const gchar       *uri);
static gchar          *uri_normalize                (const gchar       *path_or_uri,
						     const gchar       *base_uri);

static gboolean        xml_write_add_source         (GbfMkfileProject      *project,
						     xmlDocPtr          doc,
						     GNode             *g_node,
						     const gchar       *uri);
static gboolean        xml_write_remove_source      (GbfMkfileProject      *project,
						     xmlDocPtr          doc,
						     GNode             *g_node);
static gboolean        xml_write_add_target         (GbfMkfileProject      *project,
						     xmlDocPtr          doc,
						     GNode             *g_node,
						     const gchar       *name,
						     const gchar       *type);
static gboolean        xml_write_add_group          (GbfMkfileProject      *project,
						     xmlDocPtr          doc,
						     GNode             *g_node,
						     const gchar       *new_group);
static xmlDocPtr       xml_new_change_doc           (GbfMkfileProject      *project);

static GbfMkfileChange *change_new                   (GbfMkfileChangeType    ch,
						     GbfMkfileNode         *node);
static void            change_free                  (GbfMkfileChange       *change);
static GbfMkfileChange *change_set_find              (GSList            *change_set,
						     GbfMkfileChangeType    ch,
						     GbfMkfileNodeType      type);
static void            change_set_destroy           (GSList            *change_set);

static void            error_set                    (GError            **error,
						     gint               code,
						     const gchar       *message);
static GError         *parse_errors                 (GbfMkfileProject      *project,
						     const gchar       *error_buffer);
static gboolean        parse_output_xml             (GbfMkfileProject      *project,
						     gchar             *xml,
						     gint               length,
						     GSList           **change_set);

static void            spawn_shutdown               (GbfMkfileSpawnData    *data);
static void            spawn_data_destroy           (GbfMkfileSpawnData    *data);
static gboolean        spawn_write_child            (GIOChannel        *ioc,
						     GIOCondition       condition,
						     gpointer           user_data);
static gboolean        spawn_read_output            (GIOChannel        *ioc,
						     GIOCondition       condition,
						     gpointer           user_data);
static gboolean        spawn_read_error             (GIOChannel        *ioc,
						     GIOCondition       condition,
						     gpointer           user_data);
static gboolean        spawn_kill_child             (GbfMkfileSpawnData    *data);
static GbfMkfileSpawnData *spawn_script                 (gchar             **argv,
						     gint               timeout,
						     gchar             *input,
						     gint               input_size,
						     GIOFunc            input_cb,
						     GIOFunc            output_cb,
						     GIOFunc            error_cb);

static gboolean        project_reload               (GbfMkfileProject      *project,
						     GError           **err);
static gboolean        project_update               (GbfMkfileProject      *project,
						     xmlDocPtr          doc,
						     GSList           **change_set,
						     GError           **err);

static void            gbf_mkfile_node_free             (GbfMkfileNode         *node);
static GNode          *project_node_new             (GbfMkfileNodeType      type);
static void            project_node_destroy         (GbfMkfileProject      *project,
						     GNode             *g_node);
static void            project_data_destroy         (GbfMkfileProject      *project);
static void            project_data_init            (GbfMkfileProject      *project);

static void            gbf_mkfile_project_class_init    (GbfMkfileProjectClass *klass);
static void            gbf_mkfile_project_instance_init (GbfMkfileProject      *project);
static void            gbf_mkfile_project_dispose       (GObject           *object);
static void            gbf_mkfile_project_get_property  (GObject           *object,
						     guint              prop_id,
						     GValue            *value,
						     GParamSpec        *pspec);

/*
 * URI and path manipulation functions -----------------------------
 */

static gboolean 
uri_is_equal (const gchar *uri1,
	      const gchar *uri2)
{
	gboolean retval = FALSE;
	GFile *file1, *file2;

	file1 = g_file_new_for_commandline_arg (uri1);
	file2 = g_file_new_for_commandline_arg (uri2);
	retval = g_file_equal (file1, file2);
	g_object_unref (file1);
	g_object_unref (file2);

	return retval;
}

static gboolean 
uri_is_parent (const gchar *parent_uri,
	       const gchar *child_uri)
{
	gboolean retval = FALSE;
	GFile *parent, *child;

	parent = g_file_new_for_commandline_arg (parent_uri);
	child = g_file_new_for_commandline_arg (child_uri);
	retval = g_file_equal (parent, child);
	g_object_unref (parent);
	g_object_unref (child);

	return retval;
}

/*
 * This is very similar to the function that decides in 
 * g_file_new_for_commandline_arg
 */
static gboolean
uri_is_local_path (const gchar *uri)
{
	const gchar *p;
	
	for (p = uri;
	     g_ascii_isalnum (*p) || *p == '+' || *p == '-' || *p == '.';
	     p++)
		;

	if (*p == ':')
		return FALSE;
	else
		return TRUE;
}

/*
 * This is basically g_file_get_uri (g_file_new_for_commandline_arg (uri)) with
 * an option to give a basedir while g_file_new_for_commandline_arg always
 * uses the current dir.
 */
static gchar *
uri_normalize (const gchar *path_or_uri, const gchar *base_uri)
{
	gchar *normalized_uri = NULL;

	if (base_uri != NULL && uri_is_local_path (path_or_uri))
	{
		GFile *base;
		GFile *resolved;

		base = g_file_new_for_uri (base_uri);
		resolved = g_file_resolve_relative_path (base, path_or_uri);

		normalized_uri = g_file_get_uri (resolved);
		g_object_unref (base);
		g_object_unref (resolved);
	}
	else
	{
		GFile *file;

		file = g_file_new_for_commandline_arg (path_or_uri);
		normalized_uri = g_file_get_uri (file);
		g_object_unref (file);
	}

	GBF_DEBUG (g_debug (G_STRLOC" path_or_uri: %s, base_uri: %s, normalized: %s\n",
		path_or_uri, base_uri, normalized_uri));
	return normalized_uri;
}

/**
 * uri_get_chrooted_path:
 * @root_uri: the root uri (or NULL)
 * @uri: the uri which must be inside @root_uri for which the
 * root-changed path is wanted
 *
 * E.g.: uri_get_chrooted_path ("file:///foo/bar", "file:///foo/bar/baz/winkle") -> "/baz/winkle"
 *
 * Return value: a newly allocated chrooted path
 **/
static gchar *
uri_get_chrooted_path (const gchar *root_uri, const gchar *uri)
{
	GFile *root, *file;
	gchar *path = NULL;

	if (root_uri == NULL)
	{
		path = anjuta_util_get_local_path_from_uri (uri);
	}
	else
	{

		root = g_file_new_for_uri (root_uri);
		file = g_file_new_for_uri (uri);

		path = g_file_get_relative_path (root, file);

		g_object_unref (root);
		g_object_unref (file);
	}

	GBF_DEBUG (g_debug (G_STRLOC">>>>>>>>>>>>>>>>>> root_uri: %s, uri: %s, path: %s\n",
		root_uri, uri, path));
	return path;
}

/*
 * Project modification functions -----------------------------
 */

static xmlNodePtr 
xml_new_source_node (GbfMkfileProject *project,
		     xmlDocPtr     doc,
		     const gchar  *uri)
{
	xmlNodePtr source;
	gchar *filename;
	
	source = xmlNewDocNode (doc, NULL, BAD_CAST("source"), NULL);
	filename = uri_get_chrooted_path (project->project_root_uri, uri);
	xmlSetProp (source, BAD_CAST("uri"), BAD_CAST(filename));
	g_free (filename);
	
	return source;
}

static xmlNodePtr 
xml_write_location_recursive (GbfMkfileProject *project,
			      xmlDocPtr     doc,
			      xmlNodePtr    cur,
			      GNode        *g_node)
{
	xmlNodePtr result, xml_node, last_node;
	gboolean finished = FALSE;

	result = NULL;
	last_node = NULL;
	xml_node = NULL;
	while (g_node && !finished) {
		GbfMkfileNode *node = GBF_MKFILE_NODE (g_node);
		switch (node->type) {
			case GBF_MKFILE_NODE_GROUP:
				xml_node = xmlNewDocNode (doc, NULL, BAD_CAST("group"), NULL);
				xmlSetProp (xml_node, BAD_CAST("id"), BAD_CAST(node->id));
				finished = TRUE;
				break;
			case GBF_MKFILE_NODE_TARGET:
				xml_node = xmlNewDocNode (doc, NULL, BAD_CAST("target"), NULL);
				/* strip the group id from the target
				   id, since the script identifies
				   targets only within the group */
				xmlSetProp (xml_node, BAD_CAST("id"),
					    BAD_CAST(node->id +
						     strlen (GBF_MKFILE_NODE (g_node->parent)->id)));
				break;
			case GBF_MKFILE_NODE_SOURCE:
				xml_node = xml_new_source_node (project, doc, node->uri);
				break;
			default:
				g_assert_not_reached ();
				break;
		}

		/* set returned node */
		if (result == NULL)
			result = xml_node;

		/* link the previously created node to the new node */
		if (last_node != NULL)
			xmlAddChild (xml_node, last_node);

		last_node = xml_node;
		g_node = g_node->parent;
	}
	/* link the last created node to the current node */
	xmlAddChild (cur, last_node);
	
	return result;
}

static gboolean 
xml_write_add_source (GbfMkfileProject *project,
		      xmlDocPtr     doc,
		      GNode        *g_node,
		      const gchar  *uri)
{
	xmlNodePtr cur, source;
	
	cur = xmlNewDocNode (doc, NULL, BAD_CAST("add"), NULL);
	xmlSetProp (cur, BAD_CAST("type"), BAD_CAST("source"));
	xmlAddChild (doc->children, cur);

	cur = xml_write_location_recursive (project, doc, cur, g_node);
	source = xml_new_source_node (project, doc, uri);
	xmlAddChild (cur, source);
	
	return (cur != NULL);
}

static gboolean
xml_write_remove_source (GbfMkfileProject *project,
			 xmlDocPtr     doc,
			 GNode        *g_node)
{
	xmlNodePtr cur;

	cur = xmlNewDocNode (doc, NULL, BAD_CAST("remove"), NULL);
	xmlSetProp (cur, BAD_CAST("type"), BAD_CAST("source"));
	xmlAddChild (doc->children, cur);
	
	cur = xml_write_location_recursive (project, doc, cur, g_node);
	
	return (cur != NULL);
}

static gboolean 
xml_write_add_target (GbfMkfileProject *project,
		      xmlDocPtr     doc,
		      GNode        *g_node,
		      const gchar  *name,
		      const gchar  *type)
{
	xmlNodePtr cur, target;
	
	g_assert (GBF_MKFILE_NODE (g_node)->type == GBF_MKFILE_NODE_GROUP);

	cur = xmlNewDocNode (doc, NULL, BAD_CAST("add"), NULL);
	xmlSetProp (cur, BAD_CAST("type"), BAD_CAST("target"));
	xmlAddChild (doc->children, cur);

	cur = xml_write_location_recursive (project, doc, cur, g_node);
	
	target = xmlNewDocNode (doc, NULL, BAD_CAST("target"), NULL);
	xmlSetProp (target, BAD_CAST("id"), BAD_CAST(name));
	xmlSetProp (target, BAD_CAST("type"), BAD_CAST(type));
	xmlAddChild (cur, target);

	return TRUE;
}

typedef struct {
	GbfMkfileConfigMapping *old_config;
	xmlDocPtr doc;
	xmlNodePtr curr_xml_node;
} GbfXmlWriteData;

static void
xml_write_set_item_config_cb (const gchar *param, GbfMkfileConfigValue *value,
			      gpointer user_data)
{
	xmlNodePtr param_node;
	GbfXmlWriteData *data = (GbfXmlWriteData*)user_data;
	
	if (value->type == GBF_MKFILE_TYPE_STRING) {
		GbfMkfileConfigValue *old_value;
		const gchar *old_str = "", *new_str = "";
		
		new_str = gbf_mkfile_config_value_get_string (value);
		if (new_str == NULL)
			new_str = "";
		
		old_value = gbf_mkfile_config_mapping_lookup (data->old_config, param);
		if (old_value) {
			old_str = gbf_mkfile_config_value_get_string (old_value);
			if (old_str == NULL)
				old_str = "";
		}
		if (strcmp (new_str, old_str) != 0)
		{
			param_node = xmlNewDocNode (data->doc, NULL,
						    BAD_CAST("item"), NULL);
			xmlSetProp (param_node, BAD_CAST("name"), BAD_CAST(param));
			xmlSetProp (param_node, BAD_CAST("value"), BAD_CAST(new_str));
			xmlAddChild (data->curr_xml_node, param_node);
		}
	}
}

static void
xml_write_set_param_config_cb (const gchar *param, GbfMkfileConfigValue *value,
			       gpointer user_data)
{
	xmlNodePtr param_node;
	GbfXmlWriteData *data = (GbfXmlWriteData*)user_data;
	
	if (value->type == GBF_MKFILE_TYPE_STRING) {
		GbfMkfileConfigValue *old_value;
		const gchar *old_str = "", *new_str = "";
		
		new_str = gbf_mkfile_config_value_get_string (value);
		if (new_str == NULL)
			new_str = "";
		
		old_value = gbf_mkfile_config_mapping_lookup (data->old_config, param);
		if (old_value) {
			old_str = gbf_mkfile_config_value_get_string (old_value);
			if (old_str == NULL)
				old_str = "";
		}
		if (strcmp (new_str, old_str) != 0)
		{
			param_node = xmlNewDocNode (data->doc, NULL,
						    BAD_CAST("param"), NULL);
			xmlSetProp (param_node, BAD_CAST("name"), BAD_CAST(param));
			xmlSetProp (param_node, BAD_CAST("value"), BAD_CAST(new_str));
			xmlAddChild (data->curr_xml_node, param_node);
		}
	} else if (value->type == GBF_MKFILE_TYPE_LIST) {
		param_node = xmlNewDocNode (data->doc, NULL,
					    BAD_CAST("param"), NULL);
		xmlSetProp (param_node, BAD_CAST("name"), BAD_CAST(param));
		/* FIXME */
	} else if (value->type == GBF_MKFILE_TYPE_MAPPING) {
		
		GbfXmlWriteData write_data;
		GbfMkfileConfigValue *old_value;
		GbfMkfileConfigMapping *new_mapping, *old_mapping;
		
		new_mapping = gbf_mkfile_config_value_get_mapping (value);
		old_value = gbf_mkfile_config_mapping_lookup (data->old_config, param);
		old_mapping = gbf_mkfile_config_value_get_mapping (old_value);
		
		param_node = xmlNewDocNode (data->doc, NULL, BAD_CAST("param"), NULL);
		
		xmlSetProp (param_node, BAD_CAST("name"), BAD_CAST(param));
		
		write_data.doc = data->doc;
		write_data.curr_xml_node = param_node;
		write_data.old_config = old_mapping;
		
		gbf_mkfile_config_mapping_foreach (new_mapping,
					       xml_write_set_item_config_cb,
					       &write_data);
		if (param_node->children)
			xmlAddChild (data->curr_xml_node, param_node);
		else
			xmlFreeNode (param_node);
	} else {
		g_warning ("Should not be here");
	}
}

static gboolean 
xml_write_set_config (GbfMkfileProject       *project,
		      xmlDocPtr           doc,
		      GNode              *g_node,
		      GbfMkfileConfigMapping *new_config)
{
	xmlNodePtr cur, config;
	GbfXmlWriteData user_data;
	
	cur = xmlNewDocNode (doc, NULL, BAD_CAST("set"), NULL);
	xmlSetProp (cur, BAD_CAST("type"), BAD_CAST("config"));
	xmlAddChild (doc->children, cur);

	if (g_node)
		cur = xml_write_location_recursive (project, doc, cur, g_node);
	
	config = xmlNewDocNode (doc, NULL, BAD_CAST("config"), NULL);
	xmlAddChild (cur, config);
	
	user_data.doc = doc;
	user_data.curr_xml_node = config;
	user_data.old_config = GBF_MKFILE_NODE (g_node)->config;
	
	gbf_mkfile_config_mapping_foreach (new_config,
				       xml_write_set_param_config_cb,
				       &user_data);
	if (config->children)
		return TRUE;
	else
		return FALSE;
}

static gboolean
xml_write_remove_target (GbfMkfileProject *project,
			 xmlDocPtr     doc,
			 GNode        *g_node)
{
	xmlNodePtr cur;

	cur = xmlNewDocNode (doc, NULL, BAD_CAST("remove"), NULL);
	xmlSetProp (cur, BAD_CAST("type"), BAD_CAST("target"));
	xmlAddChild (doc->children, cur);
	
	cur = xml_write_location_recursive (project, doc, cur, g_node);
	
	return (cur != NULL);
}

static gboolean 
xml_write_add_group (GbfMkfileProject *project,
		     xmlDocPtr     doc,
		     GNode        *g_node,
		     const gchar  *new_group)
{
	xmlNodePtr cur, group;
	gchar *new_id;
	
	g_assert (GBF_MKFILE_NODE (g_node)->type == GBF_MKFILE_NODE_GROUP);

	cur = xmlNewDocNode (doc, NULL, BAD_CAST("add"), NULL);
	xmlSetProp (cur, BAD_CAST("type"), BAD_CAST("group"));
	xmlAddChild (doc->children, cur);

	/* calculate new id needed by the script */
	new_id = g_strdup_printf ("%s%s/", GBF_MKFILE_NODE (g_node)->id, new_group);
	
	group = xmlNewDocNode (doc, NULL, BAD_CAST("group"), NULL);
	xmlSetProp (group, BAD_CAST("id"), BAD_CAST(new_id));
	xmlAddChild (cur, group);
	g_free (new_id);
	
	return TRUE;
}

static gboolean
xml_write_remove_group (GbfMkfileProject *project,
			 xmlDocPtr     doc,
			 GNode        *g_node)
{
	xmlNodePtr cur;

	cur = xmlNewDocNode (doc, NULL, BAD_CAST("remove"), NULL);
	xmlSetProp (cur, BAD_CAST("type"), BAD_CAST("group"));
	xmlAddChild (doc->children, cur);
	
	cur = xml_write_location_recursive (project, doc, cur, g_node);
	
	return (cur != NULL);
}

static xmlDocPtr
xml_new_change_doc (GbfMkfileProject *project)
{
	xmlDocPtr doc;
	
	doc = xmlNewDoc (BAD_CAST("1.0"));
	if (doc != NULL) {
		gchar *root_path;
		root_path = anjuta_util_get_local_path_from_uri (project->project_root_uri);
		doc->children = xmlNewDocNode (doc, NULL, BAD_CAST("project"), NULL);
		xmlSetProp (doc->children, BAD_CAST("root"), BAD_CAST(root_path));
		g_free (root_path);
	}

	return doc;
}


/*
 * File monitoring support --------------------------------
 * FIXME: review these
 */

static void
monitor_cb (GFileMonitor *monitor,
			GFile *file,
			GFile *other_file,
			GFileMonitorEvent event_type,
			gpointer data)
{
	GbfMkfileProject *project = GBF_MKFILE_PROJECT (data);

	g_return_if_fail (project != NULL && GBF_IS_MKFILE_PROJECT (project));

	switch (event_type) {
		case G_FILE_MONITOR_EVENT_CHANGED:
		case G_FILE_MONITOR_EVENT_DELETED:
			/* monitor will be removed here... is this safe? */
			GBF_DEBUG (g_message ("File changed"));
			project_reload (project, NULL);
			g_signal_emit_by_name (G_OBJECT (project), "project-updated");
			break;
		default:
			break;
	}
}

static void
monitor_add (GbfMkfileProject *project, const gchar *uri)
{
	GFileMonitor *monitor = NULL;

	g_return_if_fail (project != NULL);
	g_return_if_fail (project->monitors != NULL);

	if (!uri)
		return;
	
	monitor = g_hash_table_lookup (project->monitors, uri);
	if (!monitor) {
		gboolean exists;
		GFile *file;

		/* FIXME clarify if uri is uri, path or both */
		file = g_file_new_for_commandline_arg (uri);
		exists = g_file_query_exists (file, NULL);
		
		if (exists) {
			monitor = g_file_monitor_file (file, 
										   G_FILE_MONITOR_NONE,
										   NULL,
										   NULL);
			if (monitor != NULL)
			{
				g_signal_connect (G_OBJECT (monitor),
								  "changed",
								  G_CALLBACK (monitor_cb),
								  project);
				g_hash_table_insert (project->monitors,
								     g_strdup (uri),
								     monitor);
			}
		}
	}
}

static void
monitors_remove (GbfMkfileProject *project)
{
	g_return_if_fail (project != NULL);

	if (project->monitors)
		g_hash_table_destroy (project->monitors);
	project->monitors = NULL;
}

static void
group_hash_foreach_monitor (gpointer key,
			    gpointer value,
			    gpointer user_data)
{
	GNode *group_node = value;
	GbfMkfileProject *project = user_data;

	monitor_add (project, GBF_MKFILE_NODE (group_node)->uri);
}

static void
monitors_setup (GbfMkfileProject *project)
{
	g_return_if_fail (project != NULL);

	monitors_remove (project);
	
	/* setup monitors hash */
	project->monitors = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
						   (GDestroyNotify) g_file_monitor_cancel);

	monitor_add (project, project->project_file);
	g_hash_table_foreach (project->groups, group_hash_foreach_monitor, project);
}


/*
 * Change sets functions --------------------------------
 */

static GbfMkfileChange *
change_new (GbfMkfileChangeType ch, GbfMkfileNode *node)
{
	GbfMkfileChange *change;

	change = g_new0 (GbfMkfileChange, 1);
	change->change = ch;
	change->type = node->type;
	change->id = g_strdup (node->id);

	return change;
}

static void
change_free (GbfMkfileChange *change)
{
	if (change) {
		g_free (change->id);
		g_free (change);
	}
}

static GbfMkfileChange *
change_set_find (GSList *change_set, GbfMkfileChangeType ch, GbfMkfileNodeType type)
{
	GSList *iter = change_set;

	while (iter) {
		GbfMkfileChange *change = iter->data;
		if (change->change == ch && change->type == type)
			return change;
		iter = g_slist_next (iter);
	}

	return NULL;
}

static void
change_set_destroy (GSList *change_set)
{
	GSList *l;
	for (l = change_set; l; l = g_slist_next (l))
		change_free (l->data);
	g_slist_free (change_set);
}

#ifdef ENABLE_DEBUG
static void
change_set_debug_print (GSList *change_set)
{
	GSList *iter = change_set;

	g_print ("Change set:\n");
	while (iter) {
		GbfMkfileChange *change = iter->data;

		switch (change->change) {
			case GBF_MKFILE_CHANGE_ADDED:
				g_print ("added   ");
				break;
			case GBF_MKFILE_CHANGE_REMOVED:
				g_print ("removed ");
				break;
			default:
				g_assert_not_reached ();
				break;
		}
		switch (change->type) {
			case GBF_MKFILE_NODE_GROUP:
				g_print ("group  ");
				break;
			case GBF_MKFILE_NODE_TARGET:
				g_print ("target ");
				break;
			case GBF_MKFILE_NODE_SOURCE:
				g_print ("source ");
				break;
			default:
				g_assert_not_reached ();
				break;
		}
		g_print ("%s\n", change->id);

		iter = g_slist_next (iter);
	}
}
#endif

/*
 * Perl script output parser ------------------------------
 */

#define PARSER_ASSERT(x)  G_STMT_START {						\
	if (!(x)) {									\
		GBF_DEBUG (g_warning ("Parser assertion at " G_STRLOC " failed: " #x));	\
		data->state = PARSE_ERROR; return;					\
	}										\
											\
	} G_STMT_END 
 
static void
sax_start_element (void *ctxt, const xmlChar *name, const xmlChar **attrs)
{
	GbfMkfileProjectParseData *data;
	GbfMkfileProject *project;
	GNode *g_node;
	GbfMkfileNode *node;
	
	data = ctxt;
	project = data->project;
	
	PARSER_ASSERT (data->state != PARSE_ERROR && data->state != PARSE_DONE);
	
	if (xmlStrEqual (name, BAD_CAST "project")) {
		const xmlChar *project_file = NULL;

		/* project node */
		PARSER_ASSERT (data->state == PARSE_INITIAL);
		data->state = PARSE_PROJECT;

		/* process attributes: lookup project file and check
		 * if we're getting the whole project or just changed
		 * groups */
		while (attrs && *attrs != NULL) {
			const xmlChar **val = attrs;
			
			val++;
			if (xmlStrEqual (*attrs, BAD_CAST "root")) {
				/* FIXME: check that the root is the
				 * same as the project's root */
			}
			else if (xmlStrEqual (*attrs, BAD_CAST "report"))
				data->full_report = (xmlStrEqual (*val, BAD_CAST "full") != 0);
			else if (xmlStrEqual (*attrs, BAD_CAST "source"))
				project_file = *val;
			
			attrs = ++val;
		}

		/* clear project if we are getting a full report */
		if (data->full_report) {
			/* FIXME: will this be correct in all cases? */
			data->compute_change_set = FALSE;
			project_data_init (project);
		}
		
		/* assign here, since we destroy the data in project_data_init() */
		if (project_file) {
			g_free (project->project_file);
			project->project_file = g_strdup ((gchar*)project_file);
		}
		
	} else if (xmlStrEqual (name, BAD_CAST "group")) {
		const xmlChar *group_name = NULL;
		const xmlChar *group_id = NULL;
		const xmlChar *group_source = NULL;
		
		/* group node */
		PARSER_ASSERT (data->state == PARSE_PROJECT ||
			       data->state == PARSE_GROUP);
		data->state = PARSE_GROUP;

		/* process attributes: lookup name, id and source file
		 * (i.e. Makefile) */
		while (attrs && *attrs != NULL) {
			const xmlChar **val = attrs;
			
			val++;
			if (xmlStrEqual (*attrs, BAD_CAST "name"))
				group_name = *val;
			else if (xmlStrEqual (*attrs, BAD_CAST "id"))
				group_id = *val;
			else if (xmlStrEqual (*attrs, BAD_CAST "source"))
				group_source = *val;
			
			attrs = ++val;
		}
		PARSER_ASSERT (group_name != NULL && group_id != NULL);

		/* lookup the node first, as it could already exist */
		g_node = g_hash_table_lookup (project->groups, group_id);
		if (g_node == NULL) {
			g_node = project_node_new (GBF_MKFILE_NODE_GROUP);
			node = GBF_MKFILE_NODE (g_node);
			
			node->id = g_strdup ((gchar*)group_id);
			
			/* set parent group */
			if (data->current_node != NULL) {
				g_node_prepend (data->current_node, g_node);
			} else {
				/* node is project root */
				g_assert (project->root_node == NULL);
				project->root_node = g_node;
			}

			if (data->compute_change_set)
				data->change_set = g_slist_prepend (
					data->change_set,
					change_new (GBF_MKFILE_CHANGE_ADDED, node));
			
			/* save the group in the hash for quick access */
			g_hash_table_insert (project->groups, g_strdup (node->id), g_node);

		} else {
			GNode *child;
			
			node = GBF_MKFILE_NODE (g_node);

			/* node found so remove it from the tracking hash */
			g_hash_table_remove (data->nodes, g_node);
			
			/* re-set data */
			g_free (node->name);
			g_free (node->uri);

			/* recreate the configuration */
			gbf_mkfile_config_mapping_destroy (node->config);
			
			/* track all child nodes */
			child = g_node_first_child (g_node);
			while (child != NULL) {
				/* only track groups if getting a full
				 * report, otherwise non-changed
				 * groups are later removed from the
				 * project because the script doesn't
				 * report them */
				if (data->full_report ||
				    GBF_MKFILE_NODE (child)->type != GBF_MKFILE_NODE_GROUP)
					g_hash_table_insert (data->nodes, child, child);
				child = g_node_next_sibling (child);
			}
		}
		node->name = g_strdup ((gchar*)group_name);
		node->uri = g_strdup ((gchar*)group_source);
		node->config = gbf_mkfile_config_mapping_new ();
		
		/* set working node */
		data->depth++;
		data->current_node = g_node;

	} else if (xmlStrEqual (name, BAD_CAST "target")) {
		const xmlChar *id = NULL;
		const xmlChar *target_name = NULL;
		const xmlChar *target_type = NULL;
		gchar *group_id;
		
		/* target node */
		PARSER_ASSERT (data->state == PARSE_GROUP);
		data->state = PARSE_TARGET;

		/* process attributes: lookup name, type and id */
		while (attrs && *attrs != NULL) {
			const xmlChar **val = attrs;
			
			val++;
			if (xmlStrEqual (*attrs, BAD_CAST "name"))
				target_name = *val;
			else if (xmlStrEqual (*attrs, BAD_CAST "type"))
				target_type = *val;
			else if (xmlStrEqual (*attrs, BAD_CAST "id"))
				id = *val;

			attrs = ++val;
		}
		PARSER_ASSERT (target_name != NULL && target_type != NULL);
		
		/* compute the id using the given by the script if possible */
		group_id = g_strdup_printf ("%s%s",
					    GBF_MKFILE_NODE (data->current_node)->id,
					    id != NULL ? id : target_name);
		
		/* lookup the node first, as it could already exist */
		g_node = g_hash_table_lookup (project->targets, group_id);
		if (g_node == NULL) {
			g_node = project_node_new (GBF_MKFILE_NODE_TARGET);
			node = GBF_MKFILE_NODE (g_node);
			
			node->id = group_id;

			/* set target's parent */
			g_node_prepend (data->current_node, g_node);
			
			if (data->compute_change_set)
				data->change_set = g_slist_prepend (
					data->change_set,
					change_new (GBF_MKFILE_CHANGE_ADDED, node));
			
			/* save the target in the hash for quick access */
			g_hash_table_insert (project->targets, g_strdup (node->id), g_node);

		} else {
			GNode *child;
			
			node = GBF_MKFILE_NODE (g_node);
			g_free (group_id);

			/* node found so remove it from the tracking hash */
			g_hash_table_remove (data->nodes, g_node);
			
			/* re-set data */
			g_free (node->name);
			g_free (node->detail);

			/* recreate the configuration */
			gbf_mkfile_config_mapping_destroy (node->config);
			
			/* track all child nodes */
			child = g_node_first_child (g_node);
			while (child != NULL) {
				g_hash_table_insert (data->nodes, child, child);
				child = g_node_next_sibling (child);
			}
		}
		node->name = g_strdup ((gchar*)target_name);
		node->detail = g_strdup ((gchar*)target_type);
		node->config = gbf_mkfile_config_mapping_new ();

		/* set working node */
		data->current_node = g_node;

	} else if (xmlStrEqual (name, BAD_CAST "source")) {
		const xmlChar *uri = NULL;
		gchar *source_uri, *source_id;
		
		/* source node */
		PARSER_ASSERT (data->state == PARSE_TARGET);
		data->state = PARSE_SOURCE;

		/* process attributes: lookup uri */
		while (attrs && *attrs != NULL) {
			const xmlChar **val = attrs;
			
			val++;
			if (xmlStrEqual (*attrs, BAD_CAST "uri"))
				uri = *val;

			attrs = ++val;
		}
		PARSER_ASSERT (uri != NULL);
		
		source_uri = uri_normalize ((gchar *)uri, project->project_root_uri);
		source_id = g_strdup_printf ("%s:%s",
					     GBF_MKFILE_NODE (data->current_node)->id,
					     source_uri);

		/* lookup the node first, as it could already exist */
		g_node = g_hash_table_lookup (project->sources, source_id);
		if (g_node == NULL) {
			g_node = project_node_new (GBF_MKFILE_NODE_SOURCE);
			node = GBF_MKFILE_NODE (g_node);
			
			node->id = source_id;
			
			/* set source's parent */
			g_node_prepend (data->current_node, g_node);
			
			if (data->compute_change_set)
				data->change_set = g_slist_prepend (
					data->change_set,
					change_new (GBF_MKFILE_CHANGE_ADDED, node));
			
			/* save the source in the hash for quick access */
			g_hash_table_insert (project->sources, g_strdup (node->id), g_node);

		} else {
			/* node found so remove it from the tracking hash */
			g_hash_table_remove (data->nodes, g_node);
			
			node = GBF_MKFILE_NODE (g_node);
			g_free (source_id);
			
			/* re-set data */
			g_free (node->uri);
		}
		node->uri = source_uri;
			
		/* set working node */
		data->current_node = g_node;
		
	} else if (xmlStrEqual (name, BAD_CAST "dependency")) {
		const xmlChar *uri = NULL;
		const xmlChar *target_dep = NULL;
		gchar *source_uri, *source_id;
		
		/* dependency node */
		PARSER_ASSERT (data->state == PARSE_TARGET);
		data->state = PARSE_DEPENDENCY;

		/* process attributes: lookup file and dependency target */
		while (attrs && *attrs != NULL) {
			const xmlChar **val = attrs;
			
			val++;
			if (xmlStrEqual (*attrs, BAD_CAST "file"))
				uri = *val;
			else if (xmlStrEqual (*attrs, BAD_CAST "target"))
				target_dep = *val;

			attrs = ++val;
		}
		PARSER_ASSERT (uri != NULL && target_dep != NULL);
		
		/* Compute the id */
		source_uri = g_build_filename (project->project_root_uri, uri, NULL);
		source_id = g_strdup_printf ("%s:%s",
					     GBF_MKFILE_NODE (data->current_node)->id,
					     source_uri);

		/* lookup the node first, as it could already exist */
		g_node = g_hash_table_lookup (project->sources, source_id);
		if (g_node == NULL) {
			g_node = project_node_new (GBF_MKFILE_NODE_SOURCE);
			node = GBF_MKFILE_NODE (g_node);
			
			node->id = source_id;
			
			/* set source's parent */
			g_node_prepend (data->current_node, g_node);
			
			if (data->compute_change_set)
				data->change_set = g_slist_prepend (
					data->change_set,
					change_new (GBF_MKFILE_CHANGE_ADDED, node));
			
			/* save the source in the hash for quick access */
			g_hash_table_insert (project->sources, g_strdup (node->id), g_node);

		} else {
			/* node found so remove it from the tracking hash */
			g_hash_table_remove (data->nodes, g_node);
			
			g_free (source_id);
			node = GBF_MKFILE_NODE (g_node);
			
			/* re-set data */
			g_free (node->uri);
			g_free (node->detail);
		}
		node->uri = source_uri;
		node->detail = g_strdup ((gchar*)target_dep);

		/* set working node */
		data->current_node = g_node;
		
	} else if (xmlStrEqual (name, BAD_CAST "config")) {
		/* config node */
		PARSER_ASSERT (data->state == PARSE_PROJECT ||
			       data->state == PARSE_GROUP ||
			       data->state == PARSE_TARGET);

		switch (data->state) {
		    case PARSE_PROJECT:
			    data->config = project->project_config;
			    break;
		    case PARSE_GROUP:
		    case PARSE_TARGET:
			    g_assert (data->current_node != NULL);
			    data->config = GBF_MKFILE_NODE (data->current_node)->config;
			    break;
		    default:
			    g_assert_not_reached ();
			    break;
		}

		data->save_state = data->state;
		data->state = PARSE_CONFIG;

	} else if (xmlStrEqual (name, BAD_CAST "param")) {
		const xmlChar *param_name = NULL;
		const xmlChar *param_value = NULL;
		
		/* config param node */
		PARSER_ASSERT (data->state == PARSE_CONFIG);

		/* process attributes: lookup parameter name and value (if scalar) */
		while (attrs && *attrs != NULL) {
			const xmlChar **val = attrs;
			
			val++;
			if (xmlStrEqual (*attrs, BAD_CAST "name"))
				param_name = *val;
			else if (xmlStrEqual (*attrs, BAD_CAST "value"))
				param_value = *val;
			
			attrs = ++val;
		}
		PARSER_ASSERT (param_name != NULL);

		if (param_value != NULL) {
			GbfMkfileConfigValue *param;

			/* save scalar string parameter */
			param = gbf_mkfile_config_value_new (GBF_MKFILE_TYPE_STRING);
			gbf_mkfile_config_value_set_string (param, (gchar*)param_value);

			/* try to save the parameter in the config mapping */
			if (!gbf_mkfile_config_mapping_insert (data->config,
							       (gchar*)param_name, param)) {
				gbf_mkfile_config_value_free (param);
			}

			/* we are finished with the parameter */
			data->state = PARSE_PARAM_DONE;

		} else {
			/* we expect a list or a hash of values */
			data->param_key = g_strdup ((gchar*)param_name);
			data->state = PARSE_PARAM;
		}
		
	} else if (xmlStrEqual (name, BAD_CAST "item")) {
		GbfMkfileConfigValue *param, *item;
		const xmlChar *item_name = NULL;
		const xmlChar *item_value = NULL;

		/* parameter item node */
		PARSER_ASSERT (data->state == PARSE_PARAM);
		g_assert (data->param_key != NULL);
		data->state = PARSE_ITEM;
		
		/* process attributes: lookup parameter name and value (if scalar) */
		while (attrs && *attrs != NULL) {
			const xmlChar **val = attrs;
			
			val++;
			if (xmlStrEqual (*attrs, BAD_CAST "name"))
				item_name = *val;
			else if (xmlStrEqual (*attrs, BAD_CAST "value"))
				item_value = *val;
			
			attrs = ++val;
		}
		PARSER_ASSERT (item_value != NULL);
		
		/* create item value */
		item = gbf_mkfile_config_value_new (GBF_MKFILE_TYPE_STRING);
		gbf_mkfile_config_value_set_string (item, (gchar*)item_value);
		
		/* get current configuration parameter */
		param = gbf_mkfile_config_mapping_lookup (data->config, data->param_key);
		
		/* if this is the first item, we must decide whether
		   it's a list or a mapping.  if it's not the first
		   one, we must check that the items are of the same
		   type */
		if (param == NULL) {
			if (item_name != NULL) {
				param = gbf_mkfile_config_value_new (GBF_MKFILE_TYPE_MAPPING);
			} else {
				param = gbf_mkfile_config_value_new (GBF_MKFILE_TYPE_LIST);
			}
			gbf_mkfile_config_mapping_insert (data->config,
						      data->param_key,
						      param);
		}
		
		switch (param->type) {
		    case GBF_MKFILE_TYPE_LIST:
			    param->list = g_slist_prepend (param->list, item);
			    break;
			    
		    case GBF_MKFILE_TYPE_MAPPING:
			    if (item_name == NULL ||
				!gbf_mkfile_config_mapping_insert (param->mapping,
								   (gchar*)item_name,
								   item)) {
				    gbf_mkfile_config_value_free (item);
			    }
			    break;
			    
		    default:
			    g_assert_not_reached ();
			    break;
		}
	}
}

static void
sax_end_element (void *ctx, const xmlChar *name)
{
	GbfMkfileProjectParseData *data = ctx;

	PARSER_ASSERT (data->state != PARSE_ERROR && data->state != PARSE_DONE);
	
	if (xmlStrEqual (name, BAD_CAST "project")) {
		PARSER_ASSERT (data->state == PARSE_PROJECT);
		g_assert (data->current_node == NULL);
		data->state = PARSE_DONE;
		
	} else if (xmlStrEqual (name, BAD_CAST "group")) {
		PARSER_ASSERT (data->state == PARSE_GROUP);
		g_assert (data->current_node != NULL);
		data->depth--;
		if (data->depth == 0) {
			data->current_node = NULL;
			data->state = PARSE_PROJECT;
		} else {
			data->current_node = data->current_node->parent;
		}
		
		/* FIXME: resolve all target inter-dependencies here */

	} else if (xmlStrEqual (name, BAD_CAST "target")) {
		PARSER_ASSERT (data->state == PARSE_TARGET);
		g_assert (data->current_node != NULL);
		data->current_node = data->current_node->parent;
		data->state = PARSE_GROUP;

	} else if (xmlStrEqual (name, BAD_CAST "source")) {
		PARSER_ASSERT (data->state == PARSE_SOURCE);
		g_assert (data->current_node != NULL);
		data->current_node = data->current_node->parent;
		data->state = PARSE_TARGET;

	} else if (xmlStrEqual (name, BAD_CAST "dependency")) {
		PARSER_ASSERT (data->state == PARSE_DEPENDENCY);
		g_assert (data->current_node != NULL);
		data->current_node = data->current_node->parent;
		data->state = PARSE_TARGET;

	} else if (xmlStrEqual (name, BAD_CAST "config")) {
		PARSER_ASSERT (data->state == PARSE_CONFIG);
		data->state = data->save_state;
		data->save_state = PARSE_INITIAL;
		data->config = NULL;
		
	} else if (xmlStrEqual (name, BAD_CAST "param")) {
		PARSER_ASSERT (data->state == PARSE_PARAM ||
			       data->state == PARSE_PARAM_DONE);
		data->state = PARSE_CONFIG;
		g_free (data->param_key);
		data->param_key = NULL;
		
	} else if (xmlStrEqual (name, BAD_CAST "item")) {
		PARSER_ASSERT (data->state == PARSE_ITEM);
		data->state = PARSE_PARAM;

	}
}

#undef PARSER_ASSERT

static void 
hash_foreach_add_removed (GNode        *g_node,
			  gpointer      value,
			  GSList      **change_set)
{
	*change_set = g_slist_prepend (*change_set,
				       change_new (GBF_MKFILE_CHANGE_REMOVED,
						   GBF_MKFILE_NODE (g_node)));
}

static void 
hash_foreach_destroy_node (GNode        *g_node,
			   gpointer      value,
			   GbfMkfileProject *project)
{
	project_node_destroy (project, g_node);
}

static gboolean 
parse_output_xml (GbfMkfileProject *project,
		  gchar        *xml_text,
		  gint          length,
		  GSList      **change_set)
{
	xmlSAXHandler handler;
	GbfMkfileProjectParseData data;
	int retval;
	
	memset (&handler, 0, sizeof (xmlSAXHandler));
	handler.startElement = sax_start_element;
	handler.endElement = sax_end_element;
	handler.initialized = 0;
	
	data.state = PARSE_INITIAL;
	data.save_state = PARSE_INITIAL;
	data.project = project;
	data.current_node = NULL;
	data.depth = 0;
	data.config = NULL;
	data.param_key = NULL;
	data.full_report = TRUE;

	data.change_set = NULL;
	data.nodes = g_hash_table_new (g_direct_hash, g_direct_equal);
	data.compute_change_set = (change_set != NULL);

	xmlSubstituteEntitiesDefault (TRUE);

	/* Parse document */
	retval = xmlSAXUserParseMemory (&handler, &data, xml_text, length);

	if (data.state != PARSE_DONE)
		retval = -1;

	/* construct the change set */
	if (retval >= 0 && data.compute_change_set) {
		g_hash_table_foreach (data.nodes,
				      (GHFunc) hash_foreach_add_removed,
				      &data.change_set);
		*change_set = data.change_set;
		data.change_set = NULL;
	}
	
	/* free up any remaining parsing data */
	change_set_destroy (data.change_set);
	if (data.nodes) {
		g_hash_table_foreach (data.nodes,
				      (GHFunc) hash_foreach_destroy_node,
				      project);
		g_hash_table_destroy (data.nodes);
	}
	g_free (data.param_key);
	
	return (retval >= 0);
}


/*
 * Perl script error parsing ------------------------
 */

/* FIXME: set correct error codes in all the places we call this function */
static void
error_set (GError **error, gint code, const gchar *message)
{
	if (error != NULL) {
		if (*error != NULL) {
			gchar *tmp;
			
			/* error already created, just change the code
			 * and prepend the string */
			(*error)->code = code;
			tmp = (*error)->message;
			(*error)->message = g_strconcat (message, "\n\n", tmp, NULL);
			g_free (tmp);
			
		} else {
			*error = g_error_new_literal (GBF_PROJECT_ERROR,
						      code, 
						      message);
		}
	}
}

static GError *
parse_errors (GbfMkfileProject *project,
	      const gchar  *error_buffer)
{
	const gchar *line_ptr, *next_line, *p;
	GError *err = NULL;
	GString *message;
	
	message = g_string_new (NULL);
	line_ptr = error_buffer;
	while (line_ptr) {
		/* FIXME: maybe use constants or enums for the error type */
		gint error_type = 0;
		gint error_code;
		gint line_length;
		
		next_line = g_strstr_len (line_ptr, strlen (line_ptr), "\n");
		/* skip newline */
		next_line = next_line ? next_line + 1 : NULL;
		line_length = next_line ? next_line - line_ptr : strlen (line_ptr);
		p = line_ptr;
		
		if (g_str_has_prefix (line_ptr, ERROR_PREFIX)) {
			/* get the error */
			p = line_ptr + strlen (ERROR_PREFIX);
			error_type = 1;
		}
#if 0
		/* FIXME: skip warnings for now */
		else if (g_str_has_prefix (line_ptr, WARNING_PREFIX)) {
			/* get the warning */
			p = line_ptr + strlen (WARNING_PREFIX);
			error_type = 2;
		}
#endif

		if (error_type != 0) {
			/* get the error/warning code */
			error_code = strtol (p, (char **) &p, 10);
			if (error_code != 0) {
				p = g_strstr_len (p, line_length, MESSAGE_DELIMITER);
				if (p != NULL) {
					gchar *msg;

					/* FIXME: think another
					 * solution for getting the
					 * messages, since this has
					 * problems with i18n as it's
					 * very difficult to get
					 * translated strings from the
					 * perl script */
					p += strlen (MESSAGE_DELIMITER);
					if (next_line != NULL)
						msg = g_strndup (p, next_line - p - 1);
					else
						msg = g_strdup (p);

					if (message->len > 0)
						g_string_append (message, "\n");
					g_string_append (message, msg);
					g_free (msg);
				}
			}
		}
		
		line_ptr = next_line;
	}

	if (message->len > 0) {
		err = g_error_new (GBF_PROJECT_ERROR,
				   GBF_PROJECT_ERROR_GENERAL_FAILURE,
				   "%s", message->str);
	}
	g_string_free (message, TRUE);
	
	return err;
}


/*
 * Process spawning ------------------------
 */

static void
shutdown_channel (GbfMkfileSpawnData *data, GbfMkfileChannel *channel)
{
	if (channel->channel) {
		g_io_channel_shutdown (channel->channel, TRUE, NULL);
		g_io_channel_unref (channel->channel);
		channel->channel = NULL;
	}
	if (channel->tag != 0) {
		GMainContext *context = NULL;
		GSource *source;
		if (data->main_loop != NULL)
			context = g_main_loop_get_context (data->main_loop);
		source = g_main_context_find_source_by_id (context, channel->tag);
		if (source != NULL)
			g_source_destroy (source);
		channel->tag = 0;
	}
}

static void
spawn_shutdown (GbfMkfileSpawnData *data)
{
	g_return_if_fail (data != NULL);
	
	if (data->child_pid) {
		GBF_DEBUG (g_message ("Killing child"));
		kill (data->child_pid, SIGKILL);
		data->child_pid = 0;
	}

	/* close channels and remove sources */
	shutdown_channel (data, &data->input);
	shutdown_channel (data, &data->output);
	shutdown_channel (data, &data->error);
	data->open_channels = 0;
	
	if (data->output.buffer) {
		/* shrink buffer and add terminator */
		data->output.buffer = g_realloc (data->output.buffer,
						 ++data->output.length);
		data->output.buffer [data->output.length - 1] = 0;
	}

	if (data->error.buffer) {
		/* shrink buffer and add terminator */
		data->error.buffer = g_realloc (data->error.buffer,
						++data->error.length);
		data->error.buffer [data->error.length - 1] = 0;
	}
	
	if (data->main_loop)
		g_main_loop_quit (data->main_loop);
}

static void
spawn_data_destroy (GbfMkfileSpawnData *data)
{
	g_return_if_fail (data != NULL);

	spawn_shutdown (data);

	if (data->input.buffer) {
		/* input buffer is provided by the user, so it's not freed here */
		data->input.buffer = NULL;
		data->input.size = 0;
		data->input.length = 0;
	}
	if (data->output.buffer) {
		g_free (data->output.buffer);
		data->output.buffer = NULL;
		data->output.size = 0;
		data->output.length = 0;
	}
	if (data->error.buffer) {
		g_free (data->error.buffer);
		data->error.buffer = NULL;
		data->error.size = 0;
		data->error.length = 0;
	}
	g_free (data);
}

static gboolean
spawn_write_child (GIOChannel *ioc, GIOCondition condition, gpointer user_data)
{
	GbfMkfileSpawnData *data = user_data;
	gboolean retval = FALSE;

	g_assert (data != NULL);
	g_assert (data->input.channel == ioc);
	
	if (condition & G_IO_OUT) {
		gsize bytes_written = 0;
		GIOStatus status;
		GError *error = NULL;

		/* try to write all data remaining */
		status = g_io_channel_write_chars (ioc,
						   data->input.buffer + data->input.length,
						   data->input.size - data->input.length,
						   &bytes_written, &error);
		data->input.length += bytes_written;

		switch (status) {
		    case G_IO_STATUS_NORMAL:			    
			    if (data->input.length < data->input.size) {
				    /* don't remove the source */
				    retval = TRUE;
			    }
			    break;

		    default:
			    if (error) {
				    g_warning ("Error while writing to stdin: %s",
					       error->message);
				    g_error_free (error);
			    }
			    break;
		}
	}

	if (!retval) {
		/* finished writing or some error ocurred */
		g_io_channel_shutdown (data->input.channel, TRUE, NULL);
		g_io_channel_unref (data->input.channel);
		data->input.channel = NULL;
		/* returning false will remove the source */
		data->input.tag = 0;

		data->open_channels--;
		if (data->open_channels == 0) {
			/* need to signal the end of the operation */
			if (data->main_loop)
				/* executing synchronously */
				g_main_loop_quit (data->main_loop);
			/* FIXME: what to do in the async case? */
		}
	}

	return retval;
}

static gboolean
read_channel (GbfMkfileChannel *channel, GIOCondition condition, GbfMkfileSpawnData *data)
{
	gboolean retval = FALSE;
	
	if (condition & (G_IO_IN | G_IO_PRI)) {
		gsize bytes_read = 0;
		GIOStatus status;
		GError *error = NULL;

		/* allocate buffer */
		if (channel->buffer == NULL) {
			channel->size = READ_BUFFER_SIZE;
			channel->buffer = g_malloc (channel->size);
			channel->length = 0;
		}

		status = g_io_channel_read_chars (channel->channel,
						  channel->buffer + channel->length,
						  channel->size - channel->length,
						  &bytes_read, &error);
		channel->length += bytes_read;

		switch (status) {
		    case G_IO_STATUS_NORMAL:
			    /* grow buffer if necessary */
			    if (channel->size - channel->length < READ_BUFFER_DELTA) {
				    channel->size += READ_BUFFER_DELTA;
				    channel->buffer = g_realloc (channel->buffer,
								 channel->size);
			    }
			    retval = TRUE;
			    break;
			    
		    case G_IO_STATUS_EOF:
			    /* will close the channel */
			    break;
			    
		    default:
			    if (error) {
				    g_warning ("Error while reading stderr: %s",
					       error->message);
				    g_error_free (error);
			    }
			    break;
		}
	}
	
	if (!retval) {
		/* eof was reached or some error ocurred */
		g_io_channel_shutdown (channel->channel, FALSE, NULL);
		g_io_channel_unref (channel->channel);
		channel->channel = NULL;
		/* returning false will remove the source */
		channel->tag = 0;

		data->open_channels--;
		if (data->open_channels == 0) {
			/* need to signal the end of the operation */
			if (data->main_loop)
				/* executing synchronously */
				g_main_loop_quit (data->main_loop);
			/* FIXME: what to do in the async case? */
		}
	}

	return retval;
}

static gboolean
spawn_read_output (GIOChannel *ioc, GIOCondition condition, gpointer user_data)
{
	GbfMkfileSpawnData *data = user_data;
	
	/* some checks first */
	g_assert (data != NULL);
	g_assert (ioc == data->output.channel);
	
	return read_channel (&data->output, condition, data);
}

static gboolean
spawn_read_error (GIOChannel *ioc, GIOCondition condition, gpointer user_data)
{
	GbfMkfileSpawnData *data = user_data;
	
	/* some checks first */
	g_assert (data != NULL);
	g_assert (ioc == data->error.channel);
	
	return read_channel (&data->error, condition, data);
}

static gboolean
spawn_kill_child (GbfMkfileSpawnData *data)
{
	/* we can't wait longer */
	GBF_DEBUG (g_message ("Timeout: sending SIGTERM to child process"));
	
	kill (data->child_pid, SIGTERM);
	
	if (data->main_loop)
		g_main_loop_quit (data->main_loop);

	return FALSE;
}

static guint 
context_io_add_watch (GMainContext *context,
		      GIOChannel   *channel,
		      GIOCondition  condition,
		      GSourceFunc   func,
		      gpointer      user_data)
{
	GSource *source;
	guint id;
  
	g_return_val_if_fail (channel != NULL, 0);

	source = g_io_create_watch (channel, condition);
	g_source_set_callback (source, func, user_data, NULL);
	id = g_source_attach (source, context);
	g_source_unref (source);
	
	return id;
}

static GbfMkfileSpawnData * 
spawn_script (gchar  **argv,
	      gint     timeout,
	      gchar   *input,
	      gint     input_size,
	      GIOFunc  input_cb,
	      GIOFunc  output_cb,
	      GIOFunc  error_cb)
{
	GbfMkfileSpawnData *data;
	gint child_in, child_out, child_err;
	GError *error = NULL;
	gboolean async;
	
	data = g_new0 (GbfMkfileSpawnData, 1);

	/* we consider timeout < 0 to mean asynchronous request */
	async = (timeout <= 0);

	/* setup default callbacks */
	if (input_cb == NULL) input_cb = spawn_write_child;
	if (output_cb == NULL) output_cb = spawn_read_output;
	if (error_cb == NULL) error_cb = spawn_read_error;
	
	/* set input buffer */
	if (input) {
		data->input.buffer = input;
		data->input.size = input_size;
		data->input.length = 0;  /* for input buffer length acts as an index */
	}

	GBF_DEBUG (g_message ("Spawning script"));
	
	if (!g_spawn_async_with_pipes (NULL,             /* working dir */
				       argv,
				       NULL,             /* environment */
				       0,                /* flags */
				       NULL, NULL,       /* child setup func */
				       &data->child_pid,
				       &child_in, &child_out, &child_err,
				       &error)) {
		g_warning ("Unable to fork: %s", error->message);
		g_error_free (error);
		g_free (data);
		return NULL;

	} else {
		GMainContext *context = NULL;
		
		if (!async) {
			/* we need a new context to do the i/o
			 * otherwise we could have re-entrancy
			 * problems since gtk events will be processed
			 * while we iterate the inner main loop */
			context = g_main_context_new ();
			data->main_loop = g_main_loop_new (context, FALSE);
		}

		fcntl (child_in, F_SETFL, O_NONBLOCK);
		fcntl (child_out, F_SETFL, O_NONBLOCK);
		fcntl (child_err, F_SETFL, O_NONBLOCK);

		data->open_channels = 3;
		if (input != NULL && input_size > 0) {
			data->input.channel = g_io_channel_unix_new (child_in);
			data->input.tag = context_io_add_watch (context,
								data->input.channel,
								G_IO_OUT | G_IO_ERR |
								G_IO_HUP | G_IO_NVAL,
								(GSourceFunc) input_cb, data);
		} else {
			/* we are not interested in stdin */
			close (child_in);
			data->open_channels--;
		}
		
		/* create watches for stdout and stderr */
		data->output.channel = g_io_channel_unix_new (child_out);
		data->output.tag = context_io_add_watch (context,
							 data->output.channel,
							 G_IO_ERR | G_IO_HUP |
							 G_IO_NVAL | G_IO_IN,
							 (GSourceFunc) output_cb, data);
		data->error.channel = g_io_channel_unix_new (child_err);
		data->error.tag = context_io_add_watch (context,
							data->error.channel,
							G_IO_ERR | G_IO_HUP |
							G_IO_NVAL | G_IO_IN,
							(GSourceFunc) error_cb, data);
		
		if (!async) {
			GSource *source;

			/* add the timeout */
			source = g_timeout_source_new (timeout);
			g_source_set_callback (source, (GSourceFunc) spawn_kill_child, data, NULL);
			g_source_attach (source, context);
			g_source_unref (source);
		
			g_main_loop_run (data->main_loop);

			/* continue iterations until all channels have been closed */
			while (data->open_channels > 0 && g_main_context_pending (context))
				g_main_context_iteration (context, FALSE);
			
			/* close channels and remove io watches */
			if (data->open_channels == 0)
				/* normal shutdown */
				data->child_pid = 0;
			spawn_shutdown (data);

			/* destroy main loop & context */
			g_main_loop_unref (data->main_loop);
			data->main_loop = NULL;
			g_main_context_unref (context);
		}

		return data;
	}
}

/*
 * Script execution control ----------------------------
 */

static gboolean
project_reload (GbfMkfileProject *project, GError **err) 
{
	GbfMkfileSpawnData *data;
	gchar *argv [5], *project_path;
	gboolean retval;
	gint i;
	
	project_path = anjuta_util_get_local_path_from_uri (project->project_root_uri);
		
	i = 0;
	argv [i++] = GBF_MKFILE_PARSE;
	GBF_DEBUG (argv [i++] = "-d");
	argv [i++] = "--get";
	argv [i++] = project_path;
	argv [i++] = NULL;
	g_assert (i <= G_N_ELEMENTS (argv));

	data = spawn_script (argv, SCRIPT_TIMEOUT,
			     NULL, 0,            /* input buffer */
			     NULL, NULL, NULL);  /* i/o callbacks */

	g_free (project_path);

	retval = FALSE;
	if (data != NULL) {
		if (data->error.length > 0 && err != NULL) {
			/* the buffer is zero terminated */
			*err = parse_errors (project, data->error.buffer);
		}
		
		if (data->output.length > 0) {
			retval = parse_output_xml (project,
						   data->output.buffer,
						   data->output.length,
						   NULL);
		} else {
			/* FIXME: generate some kind of error here */
			g_warning ("Child process returned no data");
		}

		spawn_data_destroy (data);
	}

	monitors_setup (project);
	
	return retval;
}

static gboolean 
project_update (GbfMkfileProject *project,
		xmlDocPtr     doc,
		GSList      **change_set,
		GError      **err) 
{
	GbfMkfileSpawnData *data;
	gchar *argv [5];
	gboolean retval;
	gint i;
	xmlChar *xml_doc;
	int xml_size;
	
	/* remove file monitors */
	monitors_remove (project);

	i = 0;
	argv [i++] = GBF_MKFILE_PARSE;
	GBF_DEBUG (argv [i++] = "-d");
	argv [i++] = "--set";
	argv [i++] = "-";
	argv [i++] = NULL;
	g_assert (i <= G_N_ELEMENTS (argv));

	/* dump the document to memory */
	xmlSubstituteEntitiesDefault (TRUE);
	xmlDocDumpMemory (doc, &xml_doc, &xml_size);

	/* execute the script */
	data = spawn_script (argv, SCRIPT_TIMEOUT,
			     (gchar*)xml_doc, xml_size,  /* input buffer */
			     NULL, NULL, NULL);  /* i/o callbacks */
	xmlFree (xml_doc);

	retval = FALSE;
	if (data != NULL) {
		if (data->error.length > 0 && err != NULL) {
			/* the buffer is zero terminated */
			*err = parse_errors (project, data->error.buffer);
		}
		
		if (data->output.length > 0) {
			/* process the xml output for changed groups */
			retval = parse_output_xml (project,
						   data->output.buffer,
						   data->output.length,
						   change_set);
			/* FIXME: emit this only if the project has indeed changed */
			g_signal_emit_by_name (G_OBJECT (project), "project-updated");
		}
		
		spawn_data_destroy (data);
	}

	monitors_setup (project);
	
	return retval;
}

/*
 * ---------------- Data structures managment
 */

static void
gbf_mkfile_node_free (GbfMkfileNode *node)
{
	if (node) {
		g_free (node->id);
		g_free (node->name);
		g_free (node->detail);
		g_free (node->uri);
		gbf_mkfile_config_mapping_destroy (node->config);

		g_free (node);
	}
}

static GNode *
project_node_new (GbfMkfileNodeType type)
{
	GbfMkfileNode *node;

	node = g_new0 (GbfMkfileNode, 1);
	node->type = type;

	return g_node_new (node);
}

static gboolean 
foreach_node_destroy (GNode    *g_node,
		      gpointer  data)
{
	GbfMkfileProject *project = data;
	
	switch (GBF_MKFILE_NODE (g_node)->type) {
		case GBF_MKFILE_NODE_GROUP:
			g_hash_table_remove (project->groups, GBF_MKFILE_NODE (g_node)->id);
			break;
		case GBF_MKFILE_NODE_TARGET:
			g_hash_table_remove (project->targets, GBF_MKFILE_NODE (g_node)->id);
			break;
		case GBF_MKFILE_NODE_SOURCE:
			g_hash_table_remove (project->sources, GBF_MKFILE_NODE (g_node)->id);
			break;
		default:
			g_assert_not_reached ();
			break;
	}
	gbf_mkfile_node_free (GBF_MKFILE_NODE (g_node));

	return FALSE;
}

static void
project_node_destroy (GbfMkfileProject *project, GNode *g_node)
{
	g_return_if_fail (project != NULL);
	g_return_if_fail (GBF_IS_MKFILE_PROJECT (project));
	
	if (g_node) {
		/* free each node's data first */
		g_node_traverse (g_node,
				 G_IN_ORDER, G_TRAVERSE_ALL, -1,
				 foreach_node_destroy, project);

		/* now destroy the tree itself */
		g_node_destroy (g_node);
	}
}

static void
project_data_destroy (GbfMkfileProject *project)
{
	g_return_if_fail (project != NULL);
	g_return_if_fail (GBF_IS_MKFILE_PROJECT (project));
	
	monitors_remove (project);
	
	/* project data */
	project_node_destroy (project, project->root_node);
	project->root_node = NULL;
	g_free (project->project_file);
	project->project_file = NULL;
	gbf_mkfile_config_mapping_destroy (project->project_config);
	project->project_config = NULL;
	
	/* shortcut hash tables */
	if (project->groups) g_hash_table_destroy (project->groups);
	if (project->targets) g_hash_table_destroy (project->targets);
	if (project->sources) g_hash_table_destroy (project->sources);
	project->groups = NULL;
	project->targets = NULL;
	project->sources = NULL;
}

static void
project_data_init (GbfMkfileProject *project)
{
	g_return_if_fail (project != NULL);
	g_return_if_fail (GBF_IS_MKFILE_PROJECT (project));
	
	/* free data if necessary */
	project_data_destroy (project);

	/* FIXME: initialize monitors here, since monitors' lifecycle
	 * are bound to source files from the project */
	
	/* project data */
	project->project_file = NULL;
	project->project_config = gbf_mkfile_config_mapping_new ();
	project->root_node = NULL;
	
	/* shortcut hash tables */
	project->groups = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	project->targets = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	project->sources = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
}

GbfMkfileConfigMapping *
gbf_mkfile_project_get_config (GbfMkfileProject *project, GError **error)
{
	g_return_val_if_fail (GBF_IS_MKFILE_PROJECT (project), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
	
	return gbf_mkfile_config_mapping_copy (project->project_config);
}

GbfMkfileConfigMapping *
gbf_mkfile_project_get_group_config (GbfMkfileProject *project, const gchar *group_id,
				 GError **error)
{
	GbfMkfileNode *node;
	GNode *g_node;
	
	g_return_val_if_fail (GBF_IS_MKFILE_PROJECT (project), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
	
	g_node = g_hash_table_lookup (project->groups, group_id);
	if (g_node == NULL) {
		error_set (error, GBF_PROJECT_ERROR_DOESNT_EXIST,
			   _("Group doesn't exist"));
		return NULL;
	}
	node = GBF_MKFILE_NODE (g_node);
	return gbf_mkfile_config_mapping_copy (node->config);
}

GbfMkfileConfigMapping *
gbf_mkfile_project_get_target_config (GbfMkfileProject *project, const gchar *target_id,
				  GError **error)
{
	GbfMkfileNode *node;
	GNode *g_node;
	
	g_return_val_if_fail (GBF_IS_MKFILE_PROJECT (project), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
	
	g_node = g_hash_table_lookup (project->targets, target_id);
	if (g_node == NULL) {
		error_set (error, GBF_PROJECT_ERROR_DOESNT_EXIST,
			   _("Target doesn't exist"));
		return NULL;
	}
	node = GBF_MKFILE_NODE (g_node);
	return gbf_mkfile_config_mapping_copy (node->config);
}

void
gbf_mkfile_project_set_config (GbfMkfileProject *project,
			   GbfMkfileConfigMapping *new_config, GError **error)
{
	xmlDocPtr doc;
	GSList *change_set = NULL;
	
	g_return_if_fail (GBF_IS_MKFILE_PROJECT (project));
	g_return_if_fail (new_config != NULL);
	g_return_if_fail (error == NULL || *error == NULL);
	
	/* Create the update xml */
	doc = xml_new_change_doc (project);
	
	if (!xml_write_set_config (project, doc, NULL, new_config)) {
		xmlFreeDoc (doc);
		return;
	}

	GBF_DEBUG ({
		xmlSetDocCompressMode (doc, 0);
		xmlSaveFile ("/tmp/set-config.xml", doc);
	});

	/* Update the project */
	if (!project_update (project, doc, &change_set, error)) {
		error_set (error, GBF_PROJECT_ERROR_PROJECT_MALFORMED,
			   _("Unable to update project"));
		xmlFreeDoc (doc);
		return;
	}
	xmlFreeDoc (doc);
	change_set_destroy (change_set);
}

void
gbf_mkfile_project_set_group_config (GbfMkfileProject *project, const gchar *group_id,
				 GbfMkfileConfigMapping *new_config, GError **error)
{
	GbfMkfileNode *node;
	xmlDocPtr doc;
	GNode *g_node;
	GSList *change_set = NULL;
	
	g_return_if_fail (GBF_IS_MKFILE_PROJECT (project));
	g_return_if_fail (new_config != NULL);
	g_return_if_fail (error == NULL || *error == NULL);
	
	g_node = g_hash_table_lookup (project->groups, group_id);
	if (g_node == NULL) {
		error_set (error, GBF_PROJECT_ERROR_DOESNT_EXIST,
			   _("Group doesn't exist"));
		return;
	}
	node = GBF_MKFILE_NODE (g_node);
	
	/* Create the update xml */
	doc = xml_new_change_doc (project);
	if (!xml_write_set_config (project, doc, g_node, new_config)) {
		xmlFreeDoc (doc);
		return;
	}

	GBF_DEBUG ({
		xmlSetDocCompressMode (doc, 0);
		xmlSaveFile ("/tmp/set-config.xml", doc);
	});
	
	/* Update the project */
	if (!project_update (project, doc, &change_set, error)) {
		error_set (error, GBF_PROJECT_ERROR_PROJECT_MALFORMED,
			   _("Unable to update project"));
		xmlFreeDoc (doc);
		return;
	}
	xmlFreeDoc (doc);
	change_set_destroy (change_set);
}

void
gbf_mkfile_project_set_target_config (GbfMkfileProject *project,
				  const gchar *target_id,
				  GbfMkfileConfigMapping *new_config,
				  GError **error)
{
	xmlDocPtr doc;
	GNode *g_node;
	GSList *change_set = NULL;
	
	g_return_if_fail (GBF_IS_MKFILE_PROJECT (project));
	g_return_if_fail (new_config != NULL);
	g_return_if_fail (error == NULL || *error == NULL);
	
	g_node = g_hash_table_lookup (project->targets, target_id);
	if (g_node == NULL) {
		error_set (error, GBF_PROJECT_ERROR_DOESNT_EXIST,
			   _("Target doesn't exist"));
	}
	
	/* Create the update xml */
	doc = xml_new_change_doc (project);
	if (!xml_write_set_config (project, doc, g_node, new_config)) {
		xmlFreeDoc (doc);
		return;
	}

	GBF_DEBUG ({
		xmlSetDocCompressMode (doc, 0);
		xmlSaveFile ("/tmp/set-config.xml", doc);
	});

	/* Update the project */
	if (!project_update (project, doc, &change_set, error)) {
		error_set (error, GBF_PROJECT_ERROR_PROJECT_MALFORMED,
			   _("Unable to update project"));
		xmlFreeDoc (doc);
		return;
	}
	xmlFreeDoc (doc);
	change_set_destroy (change_set);
}

/*
 * GbfProjectIface methods ------------------------------------------
 */

static void 
impl_load (GbfProject  *_project,
	   const gchar *uri,
	   GError     **error)
{
	GbfMkfileProject *project;
	GFile *file;
	GFileInfo *file_info;
	
	g_return_if_fail (GBF_IS_MKFILE_PROJECT (_project));

	project = GBF_MKFILE_PROJECT (_project);
	if (project->project_root_uri) {
		/* FIXME:
		 * - do we really want to allow object reutilization
		 * - cancel some pending operations in the queue?
		 */
		project_data_destroy (project);
		g_free (project->project_root_uri);
		project->project_root_uri = NULL;

		project_data_init (project);
	}

	/* allow this? */
	if (uri == NULL)
		return;
	
	file = g_file_new_for_commandline_arg (uri);
	/* check that the uri is in the filesystem */
	project->project_root_uri = g_file_get_uri (file);
	if (project->project_root_uri == NULL) {
		error_set (error, GBF_PROJECT_ERROR_DOESNT_EXIST,
			   _("Invalid or remote path (only local paths supported)"));
		return;
	}
	
	if (!g_file_query_exists (file, NULL))
	{
		error_set (error, GBF_PROJECT_ERROR_DOESNT_EXIST,
			   _("Project doesn't exist"));
		g_object_unref (file);
		g_free (project->project_root_uri);
		project->project_root_uri = NULL;
		return;
	}

	file_info = g_file_query_info (file,
			G_FILE_ATTRIBUTE_STANDARD_TYPE,
			G_FILE_QUERY_INFO_NONE,
			NULL, NULL);
	if (file_info == NULL || g_file_info_get_file_type (file_info) != G_FILE_TYPE_DIRECTORY)
	{
		error_set (error, GBF_PROJECT_ERROR_DOESNT_EXIST,
			   _("Project doesn't exist or invalid path"));
		if (file_info != NULL)
		{
			g_object_unref (file_info);
		}
		g_object_unref (file);
		g_free (project->project_root_uri);
		project->project_root_uri = NULL;
		return;
	}

	/* now try loading the project */
	if (!project_reload (project, error)) {
		error_set (error, GBF_PROJECT_ERROR_PROJECT_MALFORMED,
			   _("Malformed project"));
		g_free (project->project_root_uri);
		project->project_root_uri = NULL;
	}
	g_object_unref (file_info);
	g_object_unref (file);
}

static gboolean
file_exists (const gchar *path, const gchar *filename)
{
	gchar *full_path;
	gboolean retval;
	
	full_path = g_build_filename (path, filename, NULL);
	retval = g_file_test (full_path, G_FILE_TEST_EXISTS);
	g_free (full_path);

	return retval;
}

static gboolean 
impl_probe (GbfProject  *_project,
	    const gchar *path,
	    GError     **error)
{
	gchar *root_path;
	gboolean retval = FALSE;
	GFile *file;
	
	g_return_val_if_fail (GBF_IS_MKFILE_PROJECT (_project), FALSE);

	/* use _for_commandline_arg to resolve eventually relative path against
	 * current directory
	 */
	file = g_file_new_for_commandline_arg (path);
	root_path = g_file_get_path (file);
	g_object_unref (file);
	if (root_path != NULL && g_file_test (root_path, G_FILE_TEST_IS_DIR)) {
		retval = ((file_exists (root_path, "Makefile") ||
				   file_exists (root_path, "makefile")) &&
				   !(file_exists (root_path, "Makefile.am") || 
					 file_exists (root_path, "Makefile.in")));
		g_free (root_path);
	}

	return retval;
}

static void
impl_refresh (GbfProject *_project,
	      GError    **error)
{
	GbfMkfileProject *project;

	g_return_if_fail (GBF_IS_MKFILE_PROJECT (_project));

	project = GBF_MKFILE_PROJECT (_project);

	if (project_reload (project, error))
		g_signal_emit_by_name (G_OBJECT (project), "project-updated");
}

static GbfProjectCapabilities
impl_get_capabilities (GbfProject *_project, GError    **error)
{
	return GBF_PROJECT_CAN_ADD_NONE;
}

static GbfProjectGroup * 
impl_get_group (GbfProject  *_project,
		const gchar *id,
		GError     **error)
{
	GbfMkfileProject *project;
	GbfProjectGroup *group;
	GNode *g_node;
	GbfMkfileNode *node;
	
	g_return_val_if_fail (GBF_IS_MKFILE_PROJECT (_project), NULL);

	project = GBF_MKFILE_PROJECT (_project);
	g_node = g_hash_table_lookup (project->groups, id);
	if (g_node == NULL) {
		error_set (error, GBF_PROJECT_ERROR_DOESNT_EXIST,
			   _("Group doesn't exist"));
		return NULL;
	}
	node = GBF_MKFILE_NODE (g_node);

	group = g_new0 (GbfProjectGroup, 1);
	group->id = g_strdup (node->id);
	group->name = g_strdup (node->name);
	if (g_node->parent)
		group->parent_id = g_strdup (GBF_MKFILE_NODE (g_node->parent)->id);
	else
		group->parent_id = NULL;
	group->groups = NULL;
	group->targets = NULL;

	/* add subgroups and targets of the group */
	g_node = g_node_first_child (g_node);
	while (g_node) {
		node = GBF_MKFILE_NODE (g_node);
		switch (node->type) {
			case GBF_MKFILE_NODE_GROUP:
				group->groups = g_list_prepend (group->groups,
								g_strdup (node->id));
				break;
			case GBF_MKFILE_NODE_TARGET:
				group->targets = g_list_prepend (group->targets,
								 g_strdup (node->id));
				break;
			default:
				break;
		}
		g_node = g_node_next_sibling (g_node);
	}
	group->groups = g_list_reverse (group->groups);
	group->targets = g_list_reverse (group->targets);
	
	return group;
}

static void
foreach_group (gpointer key, gpointer value, gpointer data)
{
	GList **groups = data;

	*groups = g_list_prepend (*groups, g_strdup (key));
}


static GList *
impl_get_all_groups (GbfProject *_project,
		     GError    **error)
{
	GbfMkfileProject *project;
	GList *groups = NULL;

	g_return_val_if_fail (GBF_IS_MKFILE_PROJECT (_project), NULL);

	project = GBF_MKFILE_PROJECT (_project);
	g_hash_table_foreach (project->groups, foreach_group, &groups);

	return groups;
}

static GtkWidget *
impl_configure_new_group (GbfProject *_project,
			  GError    **error)
{
	UNIMPLEMENTED;

	return NULL;
}

static GtkWidget * 
impl_configure_group (GbfProject  *_project,
		      const gchar *id,
		      GError     **error)
{
	GtkWidget *wid = NULL;
	GError *err = NULL;
	
	g_return_val_if_fail (GBF_IS_PROJECT (_project), NULL);
	g_return_val_if_fail (id != NULL, NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);
	
	wid = gbf_mkfile_properties_get_group_widget (GBF_MKFILE_PROJECT (_project),
						  id, &err);
	if (err) {
		g_propagate_error (error, err);
	}
	return wid;
}

static gchar * 
impl_add_group (GbfProject  *_project,
		const gchar *parent_id,
		const gchar *name,
		GError     **error)
{
	GbfMkfileProject *project;
	GNode *g_node, *iter_node;
	xmlDocPtr doc;
	GSList *change_set = NULL;
	GbfMkfileChange *change;
	gchar *retval;
	
	g_return_val_if_fail (GBF_IS_MKFILE_PROJECT (_project), NULL);

	project = GBF_MKFILE_PROJECT (_project);
	
	/* find the parent group */
	g_node = g_hash_table_lookup (project->groups, parent_id);
	if (g_node == NULL) {
		error_set (error, GBF_PROJECT_ERROR_DOESNT_EXIST,
			   _("Parent group doesn't exist"));
		return NULL;
	}

	/* check that the new group doesn't already exist */
	iter_node = g_node_first_child (g_node);
	while (iter_node) {
		GbfMkfileNode *node = GBF_MKFILE_NODE (iter_node);
		if (node->type == GBF_MKFILE_NODE_GROUP &&
		    !strcmp (node->name, name)) {
			error_set (error, GBF_PROJECT_ERROR_DOESNT_EXIST,
				   _("Group already exists"));
			return NULL;
		}
		iter_node = g_node_next_sibling (iter_node);
	}
			
	/* Create the update xml */
	doc = xml_new_change_doc (project);
	if (!xml_write_add_group (project, doc, g_node, name)) {
		error_set (error, GBF_PROJECT_ERROR_DOESNT_EXIST,
			   _("Group couldn't be created"));
		xmlFreeDoc (doc);
		return NULL;
	}

	GBF_DEBUG ({
		xmlSetDocCompressMode (doc, 0);
		xmlSaveFile ("/tmp/add-group.xml", doc);
	});

	/* Update the project */
	if (!project_update (project, doc, &change_set, error)) {
		error_set (error, GBF_PROJECT_ERROR_PROJECT_MALFORMED,
			   _("Unable to update project"));
		xmlFreeDoc (doc);
		return NULL;
	}
	xmlFreeDoc (doc);

	/* get newly created group id */
	retval = NULL;
	GBF_DEBUG (change_set_debug_print (change_set));
	change = change_set_find (change_set, GBF_MKFILE_CHANGE_ADDED, GBF_MKFILE_NODE_GROUP);
	if (change) {
		retval = g_strdup (change->id);
	} else {
		error_set (error, GBF_PROJECT_ERROR_DOESNT_EXIST,
			   _("Group couldn't be created"));
	}
	change_set_destroy (change_set);

	return retval;
}

static void 
impl_remove_group (GbfProject  *_project,
		   const gchar *id,
		   GError     **error)
{
	GbfMkfileProject *project;
	GNode *g_node;
	xmlDocPtr doc;
	GSList *change_set = NULL;
	
	g_return_if_fail (GBF_IS_MKFILE_PROJECT (_project));

	project = GBF_MKFILE_PROJECT (_project);
	
	/* Find the target */
	g_node = g_hash_table_lookup (project->groups, id);
	if (g_node == NULL) {
		error_set (error, GBF_PROJECT_ERROR_DOESNT_EXIST,
			   _("Group doesn't exist"));
		return;
	}

	/* Create the update xml */
	doc = xml_new_change_doc (project);
	if (!xml_write_remove_group (project, doc, g_node)) {
		error_set (error, GBF_PROJECT_ERROR_DOESNT_EXIST,
			   _("Group coudn't be removed"));
		xmlFreeDoc (doc);
		return;
	}

	GBF_DEBUG ({
		xmlSetDocCompressMode (doc, 0);
		xmlSaveFile ("/tmp/remove-group.xml", doc);
	});

	/* Update the project */
	if (!project_update (project, doc, &change_set, error)) {
		error_set (error, GBF_PROJECT_ERROR_PROJECT_MALFORMED,
			   _("Unable to update project"));
	}
	xmlFreeDoc (doc);
	change_set_destroy (change_set);
}

static GbfProjectTarget * 
impl_get_target (GbfProject  *_project,
		 const gchar *id,
		 GError     **error)
{
	GbfMkfileProject *project;
	GbfProjectTarget *target;
	GNode *g_node;
	GbfMkfileNode *node;

	g_return_val_if_fail (GBF_IS_MKFILE_PROJECT (_project), NULL);

	project = GBF_MKFILE_PROJECT (_project);
	g_node = g_hash_table_lookup (project->targets, id);
	if (g_node == NULL) {
		error_set (error, GBF_PROJECT_ERROR_DOESNT_EXIST,
			   _("Target doesn't exist"));
		return NULL;
	}
	node = GBF_MKFILE_NODE (g_node);

	target = g_new0 (GbfProjectTarget, 1);
	target->id = g_strdup (node->id);
	target->name = g_strdup (node->name);
	target->type = g_strdup (node->detail);
	target->group_id = g_strdup (GBF_MKFILE_NODE (g_node->parent)->id);
	target->sources = NULL;

	/* add sources to the target */
	g_node = g_node_first_child (g_node);
	while (g_node) {
		node = GBF_MKFILE_NODE (g_node);
		switch (node->type) {
			case GBF_MKFILE_NODE_SOURCE:
				target->sources = g_list_prepend (target->sources,
								  g_strdup (node->id));
				break;
			default:
				break;
		}
		g_node = g_node_next_sibling (g_node);
	}
	target->sources = g_list_reverse (target->sources);

	return target;
}

static void
foreach_target (gpointer key, gpointer value, gpointer data)
{
	GList **targets = data;

	*targets = g_list_prepend (*targets, g_strdup (key));
}

static GList *
impl_get_all_targets (GbfProject *_project,
		      GError    **error)
{
	GbfMkfileProject *project;
	GList *targets = NULL;

	g_return_val_if_fail (GBF_IS_MKFILE_PROJECT (_project), NULL);

	project = GBF_MKFILE_PROJECT (_project);
	g_hash_table_foreach (project->targets, foreach_target, &targets);

	return targets;
}

static GtkWidget *
impl_configure_new_target (GbfProject *_project,
			   GError    **error)
{
	UNIMPLEMENTED;

	return NULL;
}

static GtkWidget * 
impl_configure_target (GbfProject  *_project,
		       const gchar *id,
		       GError     **error)
{
	GtkWidget *wid = NULL;
	GError *err = NULL;
	
	g_return_val_if_fail (GBF_IS_PROJECT (_project), NULL);
	g_return_val_if_fail (id != NULL, NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);
	
	wid = gbf_mkfile_properties_get_target_widget (GBF_MKFILE_PROJECT (_project),
						   id, &err);
	if (err) {
		g_propagate_error (error, err);
	}
	return wid;
}

static char * 
impl_add_target (GbfProject  *_project,
		 const gchar *group_id,
		 const gchar *name,
		 const gchar *type,
		 GError     **error)
{
	GbfMkfileProject *project;
	GNode *g_node, *iter_node;
	xmlDocPtr doc;
	GSList *change_set = NULL;
	GbfMkfileChange *change;
	gchar *retval;

	g_return_val_if_fail (GBF_IS_MKFILE_PROJECT (_project), NULL);

	project = GBF_MKFILE_PROJECT (_project);
	
	/* find the group */
	g_node = g_hash_table_lookup (project->groups, group_id);
	if (g_node == NULL) {
		error_set (error, GBF_PROJECT_ERROR_DOESNT_EXIST,
			   _("Group doesn't exist"));
		return NULL;
	}

	/* FIXME: maybe pre-check the target type? */

	/* check that the target doesn't already exist */
	iter_node = g_node_first_child (g_node);
	while (iter_node) {
		GbfMkfileNode *node = GBF_MKFILE_NODE (iter_node);
		if (node->type == GBF_MKFILE_NODE_TARGET &&
		    !strcmp (node->name, name)) {
			error_set (error, GBF_PROJECT_ERROR_DOESNT_EXIST,
				   _("Target already exists"));
			return NULL;
		}
		iter_node = g_node_next_sibling (iter_node);
	}
			
	/* Create the update xml */
	doc = xml_new_change_doc (project);
	if (!xml_write_add_target (project, doc, g_node, name, type)) {
		error_set (error, GBF_PROJECT_ERROR_DOESNT_EXIST,
			   _("Target couldn't be created"));
		xmlFreeDoc (doc);
		return NULL;
	}

	GBF_DEBUG ({
		xmlSetDocCompressMode (doc, 0);
		xmlSaveFile ("/tmp/add-target.xml", doc);
	});

	/* Update the project */
	if (!project_update (project, doc, &change_set, error)) {
		error_set (error, GBF_PROJECT_ERROR_PROJECT_MALFORMED,
			   _("Unable to update project"));
		xmlFreeDoc (doc);
		return NULL;
	}
	xmlFreeDoc (doc);
	
	/* get newly created target id */
	retval = NULL;
	GBF_DEBUG (change_set_debug_print (change_set));
	change = change_set_find (change_set, GBF_MKFILE_CHANGE_ADDED, GBF_MKFILE_NODE_TARGET);
	if (change) {
		retval = g_strdup (change->id);
	} else {
		error_set (error, GBF_PROJECT_ERROR_DOESNT_EXIST,
			   _("Target couldn't be created"));
	}
	change_set_destroy (change_set);

	return retval;
}

static void 
impl_remove_target (GbfProject  *_project,
		    const gchar *id,
		    GError     **error)
{
	GbfMkfileProject *project;
	GNode *g_node;
	xmlDocPtr doc;
	GSList *change_set = NULL;
	
	g_return_if_fail (GBF_IS_MKFILE_PROJECT (_project));

	project = GBF_MKFILE_PROJECT (_project);
	
	/* Find the target */
	g_node = g_hash_table_lookup (project->targets, id);
	if (g_node == NULL) {
		error_set (error, GBF_PROJECT_ERROR_DOESNT_EXIST,
			   _("Target doesn't exist"));
		return;
	}

	/* Create the update xml */
	doc = xml_new_change_doc (project);
	if (!xml_write_remove_target (project, doc, g_node)) {
		error_set (error, GBF_PROJECT_ERROR_DOESNT_EXIST,
			   _("Target coudn't be removed"));
		xmlFreeDoc (doc);
		return;
	}

	GBF_DEBUG ({
		xmlSetDocCompressMode (doc, 0);
		xmlSaveFile ("/tmp/remove-target.xml", doc);
	});

	/* Update the project */
	if (!project_update (project, doc, &change_set, error)) {
		error_set (error, GBF_PROJECT_ERROR_PROJECT_MALFORMED,
			   _("Unable to update project"));
	}
	xmlFreeDoc (doc);
	change_set_destroy (change_set);
}

static const gchar * 
impl_name_for_type (GbfProject  *_project,
		    const gchar *type)
{
	if (!strcmp (type, "static_lib")) {
		return _("Static Library");
	} else if (!strcmp (type, "shared_lib")) {
		return _("Shared Library");
	} else if (!strcmp (type, "man")) {
		return _("Man Documentation");
	} else if (!strcmp (type, "data")) {
		return _("Miscellaneous Data");
	} else if (!strcmp (type, "program")) {
		return _("Program");
	} else if (!strcmp (type, "script")) {
		return _("Script");
	} else if (!strcmp (type, "info")) {
		return _("Info Documentation");
	} else {
		return _("Unknown");
	}
}

static const gchar * 
impl_mimetype_for_type (GbfProject  *_project,
			const gchar *type)
{
	if (!strcmp (type, "static_lib")) {
		return "application/x-archive";
	} else if (!strcmp (type, "shared_lib")) {
		return "application/x-sharedlib";
	} else if (!strcmp (type, "man")) {
		return "text/x-troff-man";
	} else if (!strcmp (type, "data")) {
		return "application/octet-stream";
	} else if (!strcmp (type, "program")) {
		return "application/x-executable";
	} else if (!strcmp (type, "script")) {
		return "text/x-shellscript";
	} else if (!strcmp (type, "info")) {
		return "application/x-tex-info";
	} else {
		return "text/plain";
	}
}

static gchar **
impl_get_types (GbfProject *_project)
{
	return g_strsplit ("program:shared_lib:static_lib:"
			   "man:data:script:info", ":", 0);
}

static GbfProjectTargetSource * 
impl_get_source (GbfProject  *_project,
		 const gchar *id,
		 GError     **error)
{
	GbfMkfileProject *project;
	GbfProjectTargetSource *source;
	GNode *g_node;
	GbfMkfileNode *node;

	g_return_val_if_fail (GBF_IS_MKFILE_PROJECT (_project), NULL);

	project = GBF_MKFILE_PROJECT (_project);
	g_node = g_hash_table_lookup (project->sources, id);
	if (g_node == NULL) {
		error_set (error, GBF_PROJECT_ERROR_DOESNT_EXIST,
			   _("Source doesn't exist"));
		return NULL;
	}
	node = GBF_MKFILE_NODE (g_node);

	source = g_new0 (GbfProjectTargetSource, 1);
	source->id = g_strdup (node->id);
	source->source_uri = g_strdup (node->uri);
	source->target_id = g_strdup (GBF_MKFILE_NODE (g_node->parent)->id);

	return source;
}

static void
foreach_source (gpointer key, gpointer value, gpointer data)
{
	GList **sources = data;

	*sources = g_list_prepend (*sources, g_strdup (key));
}

static GList *
impl_get_all_sources (GbfProject *_project,
		      GError    **error)
{
	GbfMkfileProject *project;
	GList *sources = NULL;

	g_return_val_if_fail (GBF_IS_MKFILE_PROJECT (_project), NULL);

	project = GBF_MKFILE_PROJECT (_project);
	g_hash_table_foreach (project->sources, foreach_source, &sources);

	return sources;
}

static GtkWidget *
impl_configure_new_source (GbfProject *_project,
			   GError    **error)
{
	UNIMPLEMENTED;

	return NULL;
}

static GtkWidget * 
impl_configure_source (GbfProject  *_project,
		       const gchar *id,
		       GError     **error)
{
	UNIMPLEMENTED;

	return NULL;
}

/**
 * impl_add_source:
 * @project: 
 * @target_id: the target ID to where to add the source
 * @uri: an uri to the file, which can be absolute or relative to the target's group
 * @error: 
 * 
 * Add source implementation.  The uri must have the project root as its parent.
 * 
 * Return value: 
 **/
static gchar * 
impl_add_source (GbfProject  *_project,
		 const gchar *target_id,
		 const gchar *uri,
		 GError     **error)
{
	GbfMkfileProject *project;
	GNode *g_node, *iter_node;
	xmlDocPtr doc;
	gboolean abort_action = FALSE;
	gchar *full_uri = NULL;
	gchar *group_uri;
	GSList *change_set = NULL;
	GbfMkfileChange *change;
	gchar *retval;
	
	g_return_val_if_fail (GBF_IS_MKFILE_PROJECT (_project), NULL);

	project = GBF_MKFILE_PROJECT (_project);
	
	/* check target */
	g_node = g_hash_table_lookup (project->targets, target_id);
	if (g_node == NULL) {
		error_set (error, GBF_PROJECT_ERROR_DOESNT_EXIST,
			   _("Target doesn't exist"));
		return NULL;
	}

	/* if the uri is relative, resolve it against the target's
	 * group directory; we need to compute the group's uri
	 * first */
	group_uri = uri_normalize (g_path_skip_root (GBF_MKFILE_NODE (g_node->parent)->id),
				   project->project_root_uri);
	full_uri = uri_normalize (uri, group_uri);
	g_free (group_uri);
	
	/* Check that the source uri is inside the project root */
	if (!uri_is_parent (project->project_root_uri, full_uri)) {
		error_set (error, GBF_PROJECT_ERROR_DOESNT_EXIST,
			   _("Source file must be inside the project directory"));
		abort_action = TRUE;
	}
	
	/* check for source duplicates */
	iter_node = g_node_first_child (g_node);
	while (!abort_action && iter_node) {
		GbfMkfileNode *node = GBF_MKFILE_NODE (iter_node);
		
		if (node->type == GBF_MKFILE_NODE_SOURCE &&
		    uri_is_equal (full_uri, node->uri)) {
			error_set (error, GBF_PROJECT_ERROR_DOESNT_EXIST,
				   _("Source is already in target"));
			abort_action = TRUE;
		}
		iter_node = g_node_next_sibling (iter_node);
	}

	/* have there been any errors? */
	if (abort_action) {
		g_free (full_uri);
		return NULL;
	}
	
	/* Create the update xml */
	doc = xml_new_change_doc (project);

	if (!xml_write_add_source (project, doc, g_node, full_uri)) {
		error_set (error, GBF_PROJECT_ERROR_DOESNT_EXIST,
			   _("Source couldn't be added"));
		abort_action = TRUE;
	}

	g_free (full_uri);
	if (abort_action) {
		xmlFreeDoc (doc);
		return NULL;
	}
	
	GBF_DEBUG ({
		xmlSetDocCompressMode (doc, 0);
		xmlSaveFile ("/tmp/add-source.xml", doc);
	});

	/* Update the project */
	if (!project_update (project, doc, &change_set, error)) {
		error_set (error, GBF_PROJECT_ERROR_PROJECT_MALFORMED,
			   _("Unable to update project"));
		xmlFreeDoc (doc);
		return NULL;
	}
	xmlFreeDoc (doc);

	/* get newly created source id */
	retval = NULL;
	GBF_DEBUG (change_set_debug_print (change_set));
	change = change_set_find (change_set, GBF_MKFILE_CHANGE_ADDED, GBF_MKFILE_NODE_SOURCE);
	if (change) {
		retval = g_strdup (change->id);
	} else {
		error_set (error, GBF_PROJECT_ERROR_DOESNT_EXIST,
			   _("Source couldn't be added"));
	}
	change_set_destroy (change_set);

	return retval;
}

static void 
impl_remove_source (GbfProject  *_project,
		    const gchar *id,
		    GError     **error)
{
	GbfMkfileProject *project;
	GNode *g_node;
	xmlDocPtr doc;
	
	g_return_if_fail (GBF_IS_MKFILE_PROJECT (_project));

	project = GBF_MKFILE_PROJECT (_project);
	
	/* Find the source */
	g_node = g_hash_table_lookup (project->sources, id);
	if (g_node == NULL) {
		error_set (error, GBF_PROJECT_ERROR_DOESNT_EXIST,
			   _("Source doesn't exist"));
		return;
	}

	/* Create the update xml */
	doc = xml_new_change_doc (project);
	if (!xml_write_remove_source (project, doc, g_node)) {
		error_set (error, GBF_PROJECT_ERROR_DOESNT_EXIST,
			   _("Source coudn't be removed"));
		xmlFreeDoc (doc);
		return;
	}

	GBF_DEBUG ({
		xmlSetDocCompressMode (doc, 0);
		xmlSaveFile ("/tmp/remove-source.xml", doc);
	});

	/* Update the project */
	/* FIXME: should get and process the change set to verify that
	 * the source has been removed? */
	if (!project_update (project, doc, NULL, error)) {
		error_set (error, GBF_PROJECT_ERROR_PROJECT_MALFORMED,
			   _("Unable to update project"));
	}
	xmlFreeDoc (doc);
}

static GtkWidget *
impl_configure (GbfProject *_project, GError **error)
{
	GtkWidget *wid = NULL;
	GError *err = NULL;
	
	g_return_val_if_fail (GBF_IS_PROJECT (_project), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);
	
	wid = gbf_mkfile_properties_get_widget (GBF_MKFILE_PROJECT (_project), &err);
	if (err) {
		g_propagate_error (error, err);
	}
	return wid;
}

static GList *
impl_get_config_modules   (GbfProject *project, GError **error)
{
	return NULL;
}

static GList *
impl_get_config_packages  (GbfProject *project,
			   const gchar* module,
			   GError **error)
{
	return NULL;
}

static void
gbf_mkfile_project_class_init (GbfMkfileProjectClass *klass)
{
	GObjectClass *object_class;
	GbfProjectClass *project_class;

	object_class = G_OBJECT_CLASS (klass);
	project_class = GBF_PROJECT_CLASS (klass);
	parent_class = g_type_class_peek_parent (klass);

	object_class->dispose = gbf_mkfile_project_dispose;
	object_class->get_property = gbf_mkfile_project_get_property;

	project_class->load = impl_load;
	project_class->probe = impl_probe;
	project_class->refresh = impl_refresh;
	project_class->get_capabilities = impl_get_capabilities;

	project_class->add_group = impl_add_group;
	project_class->remove_group = impl_remove_group;
	project_class->get_group = impl_get_group;
	project_class->get_all_groups = impl_get_all_groups;
	project_class->configure_group = impl_configure_group;
	project_class->configure_new_group = impl_configure_new_group;

	project_class->add_target = impl_add_target;
	project_class->remove_target = impl_remove_target;
	project_class->get_target = impl_get_target;
	project_class->get_all_targets = impl_get_all_targets;
	project_class->configure_target = impl_configure_target;
	project_class->configure_new_target = impl_configure_new_target;

	project_class->add_source = impl_add_source;
	project_class->remove_source = impl_remove_source;
	project_class->get_source = impl_get_source;
	project_class->get_all_sources = impl_get_all_sources;
	project_class->configure_source = impl_configure_source;
	project_class->configure_new_source = impl_configure_new_source;

	project_class->configure = impl_configure;
	project_class->name_for_type = impl_name_for_type;
	project_class->mimetype_for_type = impl_mimetype_for_type;
	project_class->get_types = impl_get_types;
	
	project_class->get_config_modules = impl_get_config_modules;
	project_class->get_config_packages = impl_get_config_packages;
	
	/* default signal handlers */
	project_class->project_updated = NULL;

	/* FIXME: shouldn't we use '_' instead of '-' ? */
	g_object_class_install_property 
		(object_class, PROP_PROJECT_DIR,
		 g_param_spec_string ("project-dir", 
				      _("Project directory"),
				      _("Project directory"),
				      NULL,
				      G_PARAM_READABLE));
}

static void
gbf_mkfile_project_instance_init (GbfMkfileProject *project)
{
	/* initialize data & monitors */
	project->project_root_uri = NULL;
	project_data_init (project);

	/* setup queue */
	project->queue_ops = g_queue_new ();
	project->queue_handler_tag = 0;
	
	/* initialize build callbacks */
	project->callbacks = NULL;

	/* FIXME: those path should be configurable */
	project->make_command = g_strdup ("/usr/bin/make");
	project->configure_command = g_strdup ("./configure");
	project->autogen_command = g_strdup ("./autogen.sh");
	project->install_prefix = g_strdup ("/gnome");
}

static void
gbf_mkfile_project_dispose (GObject *object)
{
	GbfMkfileProject *project;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GBF_IS_MKFILE_PROJECT (object));

	project = GBF_MKFILE_PROJECT (object);

	/* project data & monitors */
	project_data_destroy (project);
	g_free (project->project_root_uri);
	project->project_root_uri = NULL;

	g_free (project->make_command);
	g_free (project->configure_command);
	g_free (project->autogen_command);
	g_free (project->install_prefix);
	
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (object));
}

static void
gbf_mkfile_project_get_property (GObject    *object,
			     guint       prop_id,
			     GValue     *value,
			     GParamSpec *pspec)
{
	GbfMkfileProject *project = GBF_MKFILE_PROJECT (object);

	switch (prop_id) {
		case PROP_PROJECT_DIR:
			g_value_set_string (value, project->project_root_uri);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

GbfProject *
gbf_mkfile_project_new (void)
{
	return GBF_PROJECT (g_object_new (GBF_TYPE_MKFILE_PROJECT, NULL));
}

GBF_BACKEND_BOILERPLATE (GbfMkfileProject, gbf_mkfile_project);
