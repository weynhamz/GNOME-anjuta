/* Header for IGprofViewManager interface */

#ifndef _IGPPROF_VIEW_MANAGER_H
#define _IGPPROF_VIEW_MANAGER_H

#include <glib.h>
#include <glib-object.h>

#include "gprof-view.h"

G_BEGIN_DECLS

#define IGPROF_VIEW_MANAGER_IFACE_TYPE             (igprof_view_manager_iface_get_type ())
#define IGPROF_VIEW_MANAGER_IFACE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), IGPROF_VIEW_MANAGER_IFACE_TYPE, IGProfViewManagerIface))
#define IGPROF_VIEW_MANAGER_IFACE_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), IGPROF_VIEW_MANAGER_IFACE_TYPE, IGProfViewManagerIfaceClass))
#define IGPROF_S_IVIEW_MANAGER_IFACE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IGPROF_VIEW_MANAGER_IFACE_TYPE))
#define IS_IGPROF_VIEW_MANAGER_IFACE_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), IGPROF_VIEW_MANAGER_IFACE_TYPE))
#define IGPROF_VIEW_MANAGER_IFACE_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), IVIEW_MANAGER_IFACE_TYPE, IGProfViewManagerIfaceClass))


typedef struct _IGProfViewManagerIface IGprofViewManagerIface;
typedef struct _IGProfViewManagerIfaceClass IViewManagerIfaceClass;

struct _IViewManagerIfaceClass
{
	GTypeInterface g_iface;
	
	void (*merge) (IGprofViewManagerIface *obj, GProfView *view);
	void (*update) (IGprofViewManagerIface *obj, GProfView *view);
};

GType igprof_view_manager_iface_get_type (void);

void igprof_view_manager_iface_merge (IGprofViewManagerIface *obj, 
									  GProfView *view);
void igprof_view_manager_iface_update (IGprofViewManagerIface *obj, 
									   GProfView *view);

G_END_DECLS

#endif
