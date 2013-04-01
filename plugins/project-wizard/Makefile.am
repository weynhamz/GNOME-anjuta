
SUBDIRS = templates

# Plugin glade file
wizard_gladedir = $(anjuta_glade_dir)
dist_wizard_glade_DATA = anjuta-project-wizard.ui \
	applogo.png \
	appwizard.png

# Plugin UI file
wizard_uidir = $(anjuta_ui_dir)
dist_wizard_ui_DATA =

# Plugin pixmaps
wizard_pixmapsdir = $(anjuta_image_dir)
dist_wizard_pixmaps_DATA = \
	anjuta-project-wizard-plugin.svg \
	anjuta-project-wizard-plugin-48.png

# Plugin Icon file
plugin_in_files = anjuta-project-wizard.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

wizard_plugindir = $(anjuta_plugin_dir)
dist_wizard_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

plugindir = $(anjuta_plugin_dir)
plugin_LTLIBRARIES = libanjuta-project-wizard.la

AM_CPPFLAGS = \
	$(WARN_CFLAGS) \
	$(DEPRECATED_FLAGS) \
	$(LIBANJUTA_CFLAGS) \
	-DANJUTA_BINDIR=\"$(bindir)\" \
	-DANJUTA_LIBDIR=\"$(libdir)\" \
	-DG_LOG_DOMAIN=\"libanjuta-project-wizard\"

libanjuta_project_wizard_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

libanjuta_project_wizard_la_LIBADD = \
	$(LIBANJUTA_LIBS)

libanjuta_project_wizard_la_SOURCES= \
	plugin.c \
	plugin.h \
	druid.c \
	druid.h \
	header.c \
	header.h \
	property.c \
	property.h \
	file.c \
	file.h \
	action.c \
	action.h \
	parser.c \
	parser.h \
	install.c \
	install.h \
	tar.c \
	tar.h

EXTRA_DIST = \
	$(plugin_in_files)

## templates_dir = $(datadir)/anjuta/projects
## 
## install-data-local:
##	mkdir -p $(DESTDIR)$(templates_dir)
##	$(INSTALL_DATA) $(srcdir)/templates $(DESTDIR)$(templates_dir)
##
## uninstall-local:
##	@rm -fv $(*:%=$(templates_dir)/%)
##	-if test -d $(templates_dir); then rmdir $(templates_dir); fi

-include $(top_srcdir)/git.mk
