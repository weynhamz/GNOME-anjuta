INCLUDES = \
  $(ANJUTA_CFLAGS) \
  -DEGG_COMPILATION \
  -DG_DISABLE_DEPRECATED \
  -DGDK_DISABLE_DEPRECATED \
  -DGTK_DISABLE_DEPRECATED \
  -I$(top_builddir)/libegg/util

lib_LTLIBRARIES = libanjuta-egg-treeview.la

# libegg_treeview_a_LIBADD = $(ANJUTA_LIBS)
libanjuta_egg_treeview_la_SOURCES = \
  eggaccelerators.c	\
  eggcellrendererkeys.c \
  eggcellrendererpopup.c \
  egg-cell-renderer-text.c \
  eggtreemodelfilter.c \
  eggtreemodelunion.c \
  eggtreemultidnd.c \
  eggtreeviewstate.c

noinst_HEADERS = \
  eggaccelerators.h	\
  eggcellrendererkeys.h \
  eggcellrendererpopup.h \
  egg-cell-renderer-text.h \
  eggtreemodelfilter.h \
  eggtreemultidnd.h \
  eggtreeviewstate.h


noinst_PROGRAMS = test-tree-utils test-multi-drag test-union

test_tree_utils_SOURCES = test-tree-utils.c
test_tree_utils_LDADD = libanjuta-egg-treeview.la $(ANJUTA_LIBS)
test_tree_utils_DEPENDENCIES = libanjuta-egg-treeview.la

test_multi_drag_SOURCES = test-multi-drag.c
test_multi_drag_LDADD = libanjuta-egg-treeview.la $(ANJUTA_LIBS)
test_multi_drag_DEPENDENCIES = libanjuta-egg-treeview.la

test_union_SOURCES = test-union.c
test_union_LDADD = libanjuta-egg-treeview.la $(ANJUTA_LIBS)
test_union_DEPENDENCIES = libanjuta-egg-treeview.la
