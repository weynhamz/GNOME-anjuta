
INCLUDES = \
  $(ANJUTA_CFLAGS) \
  -DEGG_COMPILATION \
  -DGTK_DISABLE_DEPRECATED \
  -DGDK_DISABLE_DEPRECATED \
  -DG_DISABLE_DEPRECATED \
  -I$(top_builddir)/libegg/util \
  -Wmissing-prototypes

noinst_LIBRARIES = libegg-toolbar.a

# libegg_toolbar_a_LIBADD = \
#	$(ANJUTA_LIBS) \
#	$(top_builddir)/libegg/util/libeggutil.a

libegg_toolbar_a_SOURCES = \
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

noinst_PROGRAMS = test-toolbar gtkexample test-drag

gtkexample_SOURCES = gtkexample.c

gtkexample_LDADD = $(ANJUTA_LIBS) \
  $(top_builddir)/libegg/toolbar/libegg-toolbar.a

test_toolbar_SOURCES = test-toolbar.c prop-editor.c

test_toolbar_LDADD = $(ANJUTA_LIBS) \
  $(top_builddir)/libegg/toolbar/libegg-toolbar.a

test_drag_SOURCES = test-drag.c

test_drag_LDADD = $(ANJUTA_LIBS) \
  $(top_builddir)/libegg/toolbar/libegg-toolbar.a
