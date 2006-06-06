
#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/interfaces/ianjuta-editor.h>

typedef struct _CppJavaPlugin CppJavaPlugin;
typedef struct _CppJavaPluginClass CppJavaPluginClass;

struct _CppJavaPlugin {
	AnjutaPlugin parent;
	gint editor_watch_id;
	GObject *current_editor;
	gboolean support_installed;
	const gchar *current_language;
};

struct _CppJavaPluginClass {
	AnjutaPluginClass parent_class;
};
