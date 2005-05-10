
SUBDIRS = util treeviewutils menu recent-files

gladefiledir = $(prefix)/@NO_PREFIX_PACKAGE_DATA_DIR@/glade
gladefile_DATA = layout.glade

INCLUDES = \
	$(GNOME_VFS_MODULE_CFLAGS) \
	$(GNOME_UI_CFLAGS) \
	$(GCONF_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	-DEGG_COMPILATION \
	-DEGG_GLADEDIR=\"$(gladefiledir)\" \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libegg \
	-I$(top_srcdir)/libegg/menu \
	-I$(top_srcdir)/libegg/treeviewutils \
	-I$(top_srcdir)/libegg/util \
	-I$(top_srcdir)/libegg/recent-files

lib_LTLIBRARIES = libanjuta-egg.la

BUILT_FILES = eggmarshalers.c

CLEANFILES = eggmarshalers.c

eggmarshalers.c: eggmarshalers.list
	cd $(srcdir) \
	&& $(GLIB_GENMARSHAL) --prefix=_egg_marshal eggmarshalers.list --body > xgen-emc \
	&& cp xgen-emc eggmarshalers.c \
	&& rm -f xgen-emc xgen-emc~

libanjuta_egg_la_SOURCES = \
	eggmarshalers.c \
	egg-entry-action.c \
	egg-combo-action.c \
	egg-recent-action.c \
	eggaccelerators.c \
	eggcellrendererkeys.c \
	eggcellrendererpopup.c \
	egg-cell-renderer-text.c \
	eggtreemodelfilter.c \
	eggtreemodelunion.c \
	eggtreemultidnd.c \
	eggtreeviewstate.c \
	egg-recent-files-module.c \
	egg-recent-model.c \
	egg-recent-view-gtk.c \
	egg-recent-item.c \
	egg-recent-util.c \
	egg-recent-view.c \
	eggcomboselect.c \
	gtkcellview.h \
	egg-submenu-action.c 

libanjuta_egg_la_LIBADD = \
	$(GNOME_VFS_MODULE_LIBS) \
	$(GNOME_UI_LIBS) \
	$(GCONF_LIBS)

noinst_PROGRAMS = \
	test-tree-utils \
	test-multi-drag \
	test-union \
	test-actions

test_tree_utils_SOURCES = test-tree-utils.c
test_tree_utils_LDADD = libanjuta-egg.la
test_tree_utils_DEPENDENCIES = libanjuta-egg.la

test_multi_drag_SOURCES = test-multi-drag.c
test_multi_drag_LDADD = libanjuta-egg.la
test_multi_drag_DEPENDENCIES = libanjuta-egg.la

test_union_SOURCES = test-union.c
test_union_LDADD = libanjuta-egg.la
test_union_DEPENDENCIES = libanjuta-egg.la

test_actions_SOURCES = test-actions.c
test_actions_LDADD = libanjuta-egg.la
test_actions_DEPENDENCIES = libanjuta-egg.la

EXTRA_DIST = \
	eggmarshalers.list \
	layout.glade
