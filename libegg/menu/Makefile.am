
INCLUDES = \
  $(ANJUTA_CFLAGS) \
  -DEGG_COMPILATION \
  -DGDK_DISABLE_DEPRECATED \
  -DG_DISABLE_DEPRECATED


lib_LTLIBRARIES = libanjuta-egg-menu.la

# libegg_menu_a_LIBADD = ../toolbar/libegg-toolbar.a $(ANJUTA_LIBS) 
libanjuta_egg_menu_la_SOURCES = \
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
test_actions_LDFLAGS = libanjuta-egg-menu.la \
	$(top_builddir)/libegg/toolbar/libanjuta-egg-toolbar.la \
	$(top_builddir)/libegg/util/libanjuta-eggutil.la \
	$(ANJUTA_LIBS)
test_actions_DEPENDENCIES = libanjuta-egg-menu.la \
	$(top_builddir)/libegg/toolbar/libanjuta-egg-toolbar.la \
	$(top_builddir)/libegg/util/libanjuta-eggutil.la

test_merge_SOURCES = test-merge.c
test_merge_LDFLAGS = libanjuta-egg-menu.la \
	$(top_builddir)/libegg/toolbar/libanjuta-egg-toolbar.la \
	$(top_builddir)/libegg/util/libanjuta-eggutil.la \
	$(ANJUTA_LIBS)
test_merge_DEPENDENCIES = libanjuta-egg-menu.la \
	$(top_builddir)/libegg/toolbar/libanjuta-egg-toolbar.la \
	$(top_builddir)/libegg/util/libanjuta-eggutil.la
