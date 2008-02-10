# Targets for handing glade-to-GConf schema conversion for prefs keys

prefs_glade_schemasdir = @GCONF_SCHEMA_FILE_DIR@
prefs_glade_schemas_DATA = $(prefs_glade_files:.glade=.schemas)

%.schemas: %.glade
	$(top_srcdir)/scripts/glade2schema.pl $(srcdir)/$(?) > $(@)

if GCONF_SCHEMAS_INSTALL
install-data-local:
	        for p in $(prefs_glade_schemas) ; do \
	            GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $$p ; \
	        done
		@killall -1 gconfd-2 || true

uninstall-local:
	        for p in $(prefs_glade_schemas) ; do \
	            GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-uninstall-rule $$p ; \
	        done
		@killall -1 gconfd-2 || true

else
install-data-local:

uninstall-local:
endif

CLEANFILES = prefs_glade_schemas
