
#ifndef __anjuta_marshal_MARSHAL_H__
#define __anjuta_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* VOID:STRING,POINTER (./anjuta-marshal.list:1) */
extern void anjuta_marshal_VOID__STRING_POINTER (GClosure     *closure,
                                                 GValue       *return_value,
                                                 guint         n_param_values,
                                                 const GValue *param_values,
                                                 gpointer      invocation_hint,
                                                 gpointer      marshal_data);

/* VOID:STRING (./anjuta-marshal.list:2) */
#define anjuta_marshal_VOID__STRING	g_cclosure_marshal_VOID__STRING

/* VOID:STRING,BOXED (./anjuta-marshal.list:3) */
extern void anjuta_marshal_VOID__STRING_BOXED (GClosure     *closure,
                                               GValue       *return_value,
                                               guint         n_param_values,
                                               const GValue *param_values,
                                               gpointer      invocation_hint,
                                               gpointer      marshal_data);

G_END_DECLS

#endif /* __anjuta_marshal_MARSHAL_H__ */

