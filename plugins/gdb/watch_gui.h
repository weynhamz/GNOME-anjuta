#ifndef _WATCH_GUI_H_
#define _WATCH_GUI_H_

#include "watch.h"

void create_expr_watch_gui (ExprWatch *ew);
GtkWidget* create_watch_add_dialog (ExprWatch *ew);
GtkWidget* create_watch_change_dialog (ExprWatch *ew);
GtkWidget* create_eval_dialog (GtkWindow *parent, ExprWatch *ew);

#endif
