#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/anjuta-launcher.h>

/* TODO: remove UI from here */
#define PREFS_GLADE PACKAGE_DATA_DIR"/glade/anjuta-gdb.glade"

typedef struct _GdbPlugin GdbPlugin;
typedef struct _GdbPluginClass GdbPluginClass;

struct _GdbPlugin
{
	AnjutaPlugin parent;
	gint uiid;
	AnjutaLauncher *launcher;
};

struct _GdbPluginClass
{
	AnjutaPluginClass parent_class;
};

void append_message (const gchar* message);
void show_messages (void);
void clear_messages (void);

AnjutaLauncher * get_launcher (void);
