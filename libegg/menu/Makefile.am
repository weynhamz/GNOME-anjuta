
INCLUDES = \
  $(ANJUTA_CFLAGS) \
  -DEGG_COMPILATION \
  -DGDK_DISABLE_DEPRECATED \
  -DG_DISABLE_DEPRECATED


noinst_LIBRARIES = libegg-menu.a

# libegg_menu_a_LIBADD = ../toolbar/libegg-toolbar.a $(ANJUTA_LIBS) 
libegg_menu_a_SOURCES = \
  egg-action.c \
  egg-action-group.c \
  egg-toggle-action.c \
  egg-radio-action.c \
  egg-entry-action.c \
  egg-combo-action.c \
  egg-markup.c \
  egg-menu-merge.c \
  egg-accel-dialog.c

noinst_HEADERS = \
  egg-menu.h \
  egg-action.h \
  egg-action-group.h \
  egg-toggle-action.h \
  egg-radio-action.h \
  egg-entry-action.h \
  egg-combo-action.h \
  egg-markup.h \
  egg-menu-merge.h \
  egg-accel-dialog.h

noinst_PROGRAMS = test-actions test-merge

test_actions_SOURCES = test-actions.c
test_actions_LDFLAGS = libegg-menu.a \
	$(top_builddir)/libegg/toolbar/libegg-toolbar.a \
	$(top_builddir)/libegg/util/libeggutil.a \
	$(ANJUTA_LIBS)
test_actions_DEPENDENCIES = libegg-menu.a \
	$(top_builddir)/libegg/toolbar/libegg-toolbar.a \
	$(top_builddir)/libegg/util/libeggutil.a

test_merge_SOURCES = test-merge.c
test_merge_LDFLAGS = libegg-menu.a \
	$(top_builddir)/libegg/toolbar/libegg-toolbar.a \
	$(top_builddir)/libegg/util/libeggutil.a \
	$(ANJUTA_LIBS)
test_merge_DEPENDENCIES = libegg-menu.a \
	$(top_builddir)/libegg/toolbar/libegg-toolbar.a \
	$(top_builddir)/libegg/util/libeggutil.a
