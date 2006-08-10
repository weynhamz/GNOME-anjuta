#ifndef __GLUE_PLUGIN_H__
#define __GLUE_PLUGIN_H__

#include <glib-object.h>
#include <gmodule.h>

G_BEGIN_DECLS

#define GLUE_TYPE_PLUGIN            (glue_plugin_get_type ())
#define GLUE_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GLUE_TYPE_PLUGIN, GluePlugin))
#define GLUE_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GLUE_TYPE_PLUGIN, GluePluginClass))
#define GLUE_IS_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GLUE_TYPE_PLUGIN))
#define GLUE_IS_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), GLUE_TYPE_PLUGIN))
#define GLUE_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GLUE_TYPE_PLUGIN, GluePluginClass))

typedef struct _GluePlugin      GluePlugin;
typedef struct _GluePluginClass GluePluginClass;
typedef struct _GluePluginComponent GluePluginComponent;

typedef GType (* GluePluginComponentGetTypeFunc) (GluePlugin *plugin);

struct _GluePlugin
{
  GTypeModule parent;
  GModule *module;
  char *path;
};

struct _GluePluginClass
{
  GTypeModuleClass parent_class;
};

struct _GluePluginComponent
{
  const gchar *name;
  GluePluginComponentGetTypeFunc get_type_func;
};

GType       glue_plugin_get_type      (void);
GluePlugin *glue_plugin_new           (GModule *module);

#define GLUE_PLUGIN_MODULE(components) \
GLUE_REGISTER_COMPONENTS(components) \
GLUE_GET_COMPONENT_TYPE(components)

#define GLUE_REGISTER_COMPONENTS(components) 	\
G_MODULE_EXPORT void				\
glue_register_components (GluePlugin *plugin)	\
{						\
  int i;					\
						\
  for (i = 0; components[i].name != NULL; i++)	\
    {						\
      (* components[i].get_type_func) (plugin);	\
    }						\
}

#define GLUE_GET_COMPONENT_TYPE(components)	\
G_MODULE_EXPORT GType \
glue_get_component_type (GluePlugin *plugin, const char *name) \
{ \
  int i; \
\
  for (i = 0; components[i].name != NULL; i++) \
    { \
      if (strcmp (name, components[i].name) == 0) \
	return (* components[i].get_type_func) (plugin); \
    } \
\
  return G_TYPE_INVALID; \
} 

G_END_DECLS

#endif /* __GLUE_PLUGIN_H__ */
