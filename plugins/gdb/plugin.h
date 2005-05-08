#ifndef GDB_PLUGIN_H
#define GDB_PLUGIN_H

#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/anjuta-launcher.h>
#include <libanjuta/interfaces/ianjuta-message-view.h>

#include "debugger.h"
#include "watch.h"
#include "locals.h"
#include "stack_trace.h"
#include "registers.h"
#include "sharedlib.h"
#include "signals.h"
#include "breakpoints.h"

/* TODO: remove UI from here */
#define PREFS_GLADE PACKAGE_DATA_DIR"/glade/anjuta-gdb.glade"

G_BEGIN_DECLS

typedef struct _GdbPlugin GdbPlugin;
typedef struct _GdbPluginClass GdbPluginClass;

struct _GdbPlugin
{
	AnjutaPlugin parent;
	
	gint uiid;
	
	gint merge_id;
	GtkActionGroup *action_group;
	guint editor_watch_id;
	guint project_watch_id;
	
	GObject *current_editor;
	gchar *project_root_uri;
	IAnjutaMessageView *mesg_view;
	
	/* Debugger */
	Debugger *debugger;
	
	/* Debugger components */
	ExprWatch *watch;
	Locals *locals;
	StackTrace *stack;
	CpuRegisters *registers;
	Sharedlibs *sharedlibs;
	Signals *signals;
	BreakpointsDBase *breakpoints;
};

struct _GdbPluginClass
{
	AnjutaPluginClass parent_class;
};

void gdb_plugin_update_ui (GdbPlugin *plugin);

G_END_DECLS

#endif
