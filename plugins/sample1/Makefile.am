#
# Sample Makefile for a gIDE plugin.
#
# Where can we find the gIDE include files.
#

ANJUTA_SRC_DIR = $(top_srcdir)/src

#
# Where to install the plugin
#
plugindir = $(prefix)/@NO_PREFIX_PACKAGE_PLUGIN_DIR@

# NOTE :
# The naming convention is very intentional
# We are forced to use the prefix 'lib' by automake and libtool
#    There is probably a way to avoid it but it is not worth to effort
#    to find out.
# The 'anjuta_' prfix is a safety measure to avoid conflicts where the
#    plugin 'libpython.so' needs to link with the real 'libpython.so'

plugin_LTLIBRARIES = libanjuta_sample1.la

libanjuta_sample1_la_SOURCES = plugin-sample1.c

INCLUDES = 						\
	-I$(ANJUTA_SRC_DIR)				\
	-I$(top_srcdir)/widgets		\
	-I$(top_srcdir)/tagmanager/include      \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DVERSION=\""$(VERSION)"\"             		\
	-I$(includedir)					\
	$(ANJUTA_CFLAGS)
