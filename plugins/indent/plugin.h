
#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <indent-util.h>
#include <indent-dialog.h>
#include <glib/gstring.h>

extern GType anjuta_indent_plugin_get_type (GTypeModule *module);
#define ANJUTA_TYPE_PLUGIN_INDENT         (anjuta_indent_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_INDENT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_INDENT, IndentPlugin))
#define ANJUTA_PLUGIN_INDENT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_INDENT, IndentPluginClass))
#define ANJUTA_IS_PLUGIN_INDENT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_INDENT))
#define ANJUTA_IS_PLUGIN_INDENT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_INDENT))
#define ANJUTA_PLUGIN_INDENT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_INDENT, IndentPluginClass))

typedef struct _IndentPlugin IndentPlugin;
typedef struct _IndentPluginClass IndentPluginClass;

struct _IndentPlugin{
	AnjutaPlugin parent;
	
	IndentData* idt;
	gint uiid;
	
	guint editor_watch_id;
	IAnjutaEditor* current_editor;
	GString* indent_output;
};

struct _IndentPluginClass{
	AnjutaPluginClass parent_class;
};
