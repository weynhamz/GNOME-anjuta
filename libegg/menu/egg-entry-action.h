#ifndef EGG_ENTRY_ACTION_H
#define EGG_ENTRY_ACTION_H

#include <gtk/gtk.h>
#include <libegg/menu/egg-action.h>

#define EGG_TYPE_ENTRY_ACTION            (egg_entry_action_get_type ())
#define EGG_ENTRY_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EGG_TYPE_ENTRY_ACTION, EggEntryAction))
#define EGG_ENTRY_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), EGG_TYPE_ENTRY_ACTION, EggEntryActionClass))
#define EGG_IS_ENTRY_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EGG_TYPE_ENTRY_ACTION))
#define EGG_IS_ENTRY_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), EGG_TYPE_ENTRY_ACTION))
#define EGG_ENTRY_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), EGG_TYPE_ENTRY_ACTION, EggEntryActionClass))

typedef struct _EggEntryAction      EggEntryAction;
typedef struct _EggEntryActionClass EggEntryActionClass;

struct _EggEntryAction
{
  EggAction parent;

  gchar* text;
  gint   width;
};

struct _EggEntryActionClass
{
  EggActionClass parent_class;

  /* Signals */
  void (* changed)   (EggEntryAction *action);
  void (* focus_in)  (EggEntryAction *action);
  void (* focus_out) (EggEntryAction *action);
};

GType    egg_entry_action_get_type   (void);

void     egg_entry_action_changed (EggEntryAction *action);
void     egg_entry_action_set_text (EggEntryAction *action,
				    const gchar *text);
const gchar * egg_entry_action_get_text (EggEntryAction *action);

#endif
