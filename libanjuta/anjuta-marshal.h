
#ifndef __anjuta_cclosure_marshal_MARSHAL_H__
#define __anjuta_cclosure_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* VOID:BOOLEAN (./anjuta-marshal.list:24) */
#define anjuta_cclosure_marshal_VOID__BOOLEAN	g_cclosure_marshal_VOID__BOOLEAN

/* VOID:INT,STRING (./anjuta-marshal.list:25) */
extern void anjuta_cclosure_marshal_VOID__INT_STRING (GClosure     *closure,
                                                      GValue       *return_value,
                                                      guint         n_param_values,
                                                      const GValue *param_values,
                                                      gpointer      invocation_hint,
                                                      gpointer      marshal_data);

/* VOID:INT,INT,ULONG (./anjuta-marshal.list:26) */
extern void anjuta_cclosure_marshal_VOID__INT_INT_ULONG (GClosure     *closure,
                                                         GValue       *return_value,
                                                         guint         n_param_values,
                                                         const GValue *param_values,
                                                         gpointer      invocation_hint,
                                                         gpointer      marshal_data);

/* VOID:STRING,POINTER (./anjuta-marshal.list:27) */
extern void anjuta_cclosure_marshal_VOID__STRING_POINTER (GClosure     *closure,
                                                          GValue       *return_value,
                                                          guint         n_param_values,
                                                          const GValue *param_values,
                                                          gpointer      invocation_hint,
                                                          gpointer      marshal_data);

/* VOID:STRING (./anjuta-marshal.list:28) */
#define anjuta_cclosure_marshal_VOID__STRING	g_cclosure_marshal_VOID__STRING

/* VOID:STRING,BOXED (./anjuta-marshal.list:29) */
extern void anjuta_cclosure_marshal_VOID__STRING_BOXED (GClosure     *closure,
                                                        GValue       *return_value,
                                                        guint         n_param_values,
                                                        const GValue *param_values,
                                                        gpointer      invocation_hint,
                                                        gpointer      marshal_data);

/* VOID:POINTER (./anjuta-marshal.list:30) */
#define anjuta_cclosure_marshal_VOID__POINTER	g_cclosure_marshal_VOID__POINTER

G_END_DECLS

#endif /* __anjuta_cclosure_marshal_MARSHAL_H__ */

