
INCLUDES = \
  $(ANJUTA_CFLAGS) \
  -DEGG_COMPILATION \
  -DGTK_DISABLE_DEPRECATED \
  -DGDK_DISABLE_DEPRECATED \
  -DG_DISABLE_DEPRECATED \
  -I$(top_builddir)/libegg/util \
  -Wmissing-prototypes

lib_LTLIBRARIES = libanjuta-egg-toolbar.la

# libegg_toolbar_a_LIBADD = \
#	$(ANJUTA_LIBS) \
#	$(top_builddir)/libegg/util/libeggutil.a

libanjuta_egg_toolbar_la_SOURCES = \
  eggtoolbar.c \
  eggtoolitem.c \
  eggseparatortoolitem.c \
  eggradiotoolbutton.c \
  eggtoolbutton.c \
  eggtoggletoolbutton.c

noinst_HEADERS = \
  eggtoolbar.h \
  eggtoolitem.h	 \
  eggseparatortoolitem.h \
  eggradiotoolbutton.h \
  eggtoolbutton.h \
  eggtoggletoolbutton.h

noinst_PROGRAMS = gtkexample test-drag #test-toolbar

gtkexample_SOURCES = gtkexample.c

gtkexample_LDADD = $(ANJUTA_LIBS) \
  $(top_builddir)/libegg/toolbar/libanjuta-egg-toolbar.la \
  $(top_builddir)/libegg/util/libanjuta-eggutil.la

#test_toolbar_SOURCES = test-toolbar.c prop-editor.c

#test_toolbar_LDADD = $(ANJUTA_LIBS) \
#  $(top_builddir)/libegg/toolbar/libegg-toolbar.a \
#  $(top_builddir)/libegg/util/libeggutil.a

test_drag_SOURCES = test-drag.c

test_drag_LDADD = $(ANJUTA_LIBS) \
  $(top_builddir)/libegg/toolbar/libanjuta-egg-toolbar.la \
  $(top_builddir)/libegg/util/libanjuta-eggutil.la

