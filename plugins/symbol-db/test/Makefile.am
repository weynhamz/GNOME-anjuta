
bin_PROGRAMS = \
	test-symbol-db 

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DANJUTA_DATA_DIR=\"$(anjuta_data_dir)\" \
	-DANJUTA_PLUGIN_DIR=\"$(anjuta_plugin_dir)\" \
	-DANJUTA_IMAGE_DIR=\"$(anjuta_image_dir)\" \
	-DANJUTA_GLADE_DIR=\"$(anjuta_glade_dir)\" \
	-DANJUTA_UI_DIR=\"$(anjuta_ui_dir)\" \
	-DPACKAGE_SRC_DIR=\"$(srcdir)\" \
	$(LIBANJUTA_CFLAGS) \
	$(PLUGIN_SYMBOL_DB_CFLAGS) \
	$(GDL_CFLAGS)


test_symbol_db_SOURCES = \
	main.c       \
	../readtags.c       \
	../readtags.h       \
	../symbol-db-engine.c       \
	../symbol-db-engine.h       \
	../symbol-db-engine-iterator.c       \
	../symbol-db-engine-iterator.h \
	../symbol-db-engine-iterator-node.c \
	../symbol-db-engine-iterator-node.h \
	../symbol-db-view.h \
	../symbol-db-view.c 
 

test_symbol_db_LDFLAGS = \
	$(LIBANJUTA_LIBS) \
	$(PLUGIN_SYMBOL_DB_LIBS)

## File created by the gnome-build tools









