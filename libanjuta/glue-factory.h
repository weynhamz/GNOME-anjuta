#ifndef __GLUE_FACTORY_H__
#define __GLUE_FACTORY_H__

#include <glib-object.h>

#define GLUE_TYPE_FACTORY            (glue_factory_get_type ())
#define GLUE_FACTORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GLUE_TYPE_FACTORY, GlueFactory))
#define GLUE_FACTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GLUE_TYPE_FACTORY, GlueFactoryClass))
#define GLUE_IS_FACTORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GLUE_TYPE_FACTORY))
#define GLUE_IS_FACTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), GLUE_TYPE_FACTORY))
#define GLUE_FACTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GLUE_TYPE_FACTORY, GlueFactoryClass))

typedef struct _GlueFactory      GlueFactory;
typedef struct _GlueFactoryClass GlueFactoryClass;

GType        glue_factory_get_type        (void);
GlueFactory *glue_factory_new             (void);
gboolean     glue_factory_add_path        (GlueFactory  *factory,
					   const char   *path);
GType        glue_factory_get_object_type (GlueFactory  *factory,
					   const gchar  *component_name,
					   const gchar  *type_name);

GObject     *glue_factory_create_object   (GlueFactory  *factory,
					   const gchar  *component_name,
					   const gchar  *type_name,
					   ...);
#endif /* __GLUE_FACTORY_H__ */
