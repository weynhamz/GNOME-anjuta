
SUBDIRS = util treeviewutils toolbar menu

INCLUDES = \
	$(ANJUTA_CFLAGS) \
	-DEGG_COMPILATION \
	-DGDK_DISABLE_DEPRECATED \
	-DG_DISABLE_DEPRECATED \
	-I$(top_builddir)/libegg \
	-I$(top_builddir)/libegg/menu \
	-I$(top_builddir)/libegg/toolbar \
	-I$(top_builddir)/libegg/treeviewutils \
	-I$(top_builddir)/libegg/util

lib_LTLIBRARIES = libanjuta-egg.la

BUILT_SOURCES = eggmarshalers.c

eggmarshalers.c: eggmarshalers.list
	cd $(srcdir) \
	&& $(GLIB_GENMARSHAL) --prefix=_egg_marshal eggmarshalers.list --body > xgen-emc \
	&& cp xgen-emc eggmarshalers.c \
	&& rm -f xgen-emc xgen-emc~

libanjuta_egg_la_SOURCES = \
	eggmarshalers.c \
	egg-action.c \
	egg-action-group.c \
	egg-toggle-action.c \
	egg-radio-action.c \
	egg-entry-action.c \
	egg-combo-action.c \
	egg-markup.c \
	egg-menu-merge.c \
	egg-accel-dialog.c \
	eggtoolbar.c \
	eggtoolitem.c \
	eggseparatortoolitem.c \
	eggradiotoolbutton.c \
	eggtoolbutton.c \
	eggtoggletoolbutton.c \
	eggaccelerators.c     \
	eggcellrendererkeys.c \
	eggcellrendererpopup.c \
	egg-cell-renderer-text.c \
	eggtreemodelfilter.c \
	eggtreemodelunion.c \
	eggtreemultidnd.c \
	eggtreeviewstate.c

noinst_PROGRAMS = \
	test-actions \
	test-merge \
	gtkexample \
	test-drag \
	test-tree-utils \
	test-multi-drag \
	test-union

#	test-toolbar

test_actions_SOURCES = test-actions.c
test_actions_LDFLAGS = libanjuta-egg.la $(ANJUTA_LIBS)
test_actions_DEPENDENCIES = libanjuta-egg.la

test_merge_SOURCES = test-merge.c
test_merge_LDFLAGS = libanjuta-egg.la $(ANJUTA_LIBS)
test_merge_DEPENDENCIES = libanjuta-egg.la

gtkexample_SOURCES = gtkexample.c
gtkexample_LDADD = libanjuta-egg.la $(ANJUTA_LIBS)

#test_toolbar_SOURCES = test-toolbar.c prop-editor.c
#test_toolbar_LDADD = libanjuta-egg.la $(ANJUTA_LIBS)

test_drag_SOURCES = test-drag.c
test_drag_LDADD = libanjuta-egg.la $(ANJUTA_LIBS)

test_tree_utils_SOURCES = test-tree-utils.c
test_tree_utils_LDADD = libanjuta-egg.la $(ANJUTA_LIBS)
test_tree_utils_DEPENDENCIES = libanjuta-egg.la

test_multi_drag_SOURCES = test-multi-drag.c
test_multi_drag_LDADD = libanjuta-egg.la $(ANJUTA_LIBS)
test_multi_drag_DEPENDENCIES = libanjuta-egg.la

test_union_SOURCES = test-union.c
test_union_LDADD = libanjuta-egg.la $(ANJUTA_LIBS)
test_union_DEPENDENCIES = libanjuta-egg.la


EXTRA_DIST = \
        eggmarshalers.list
