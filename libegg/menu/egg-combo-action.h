#ifndef EGG_COMBO_ACTION_H
#define EGG_COMBO_ACTION_H

#include <gtk/gtk.h>
#include <libegg/menu/egg-entry-action.h>

#define EGG_TYPE_COMBO_ACTION            (egg_combo_action_get_type ())
#define EGG_COMBO_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EGG_TYPE_COMBO_ACTION, EggComboAction))
#define EGG_COMBO_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), EGG_TYPE_COMBO_ACTION, EggComboActionClass))
#define EGG_IS_COMBO_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EGG_TYPE_COMBO_ACTION))
#define EGG_IS_COMBO_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), EGG_TYPE_COMBO_ACTION))
#define EGG_COMBO_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), EGG_TYPE_COMBO_ACTION, EggComboActionClass))

typedef struct _EggComboAction      EggComboAction;
typedef struct _EggComboActionClass EggComboActionClass;

struct _EggComboAction
{
  EggEntryAction parent;
};

struct _EggComboActionClass
{
  EggEntryActionClass parent_class;
};

GType    egg_combo_action_get_type   (void);
void     egg_combo_action_set_popdown_string (EggComboAction *action,
					      GList *popdown_strings);

#endif
