#ifndef __ANJUTA_GLUE_FACTORY_H__
#define __ANJUTA_GLUE_FACTORY_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define ANJUTA_GLUE_TYPE_FACTORY            (anjuta_glue_factory_get_type ())
#define ANJUTA_GLUE_FACTORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_GLUE_TYPE_FACTORY, AnjutaGlueFactory))
#define ANJUTA_GLUE_FACTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_GLUE_TYPE_FACTORY, AnjutaGlueFactoryClass))
#define ANJUTA_GLUE_IS_FACTORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_GLUE_TYPE_FACTORY))
#define ANJUTA_GLUE_IS_FACTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), ANJUTA_GLUE_TYPE_FACTORY))
#define ANJUTA_GLUE_FACTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), ANJUTA_GLUE_TYPE_FACTORY, AnjutaGlueFactoryClass))

typedef struct
{
  const gchar *path;
  GHashTable *loaded_plugins;
} PathEntry;

typedef struct _AnjutaGlueFactory      AnjutaGlueFactory;
typedef struct _AnjutaGlueFactoryClass AnjutaGlueFactoryClass;

GType        anjuta_glue_factory_get_type        (void);
AnjutaGlueFactory *anjuta_glue_factory_new             (void);
gboolean     anjuta_glue_factory_add_path        (AnjutaGlueFactory  *factory,
					   const char   *path);
GType        anjuta_glue_factory_get_object_type (AnjutaGlueFactory  *factory,
					   const gchar  *component_name,
					   const gchar  *type_name);

GObject     *anjuta_glue_factory_create_object   (AnjutaGlueFactory  *factory,
					   const gchar  *component_name,
					   const gchar  *type_name,
					   ...);
GList*			anjuta_glue_factory_get_path (AnjutaGlueFactory *factory);
					   
G_END_DECLS
#endif /* __ANJUTA_GLUE_FACTORY_H__ */
