# Targets for handing glade-to-GConf schema conversion for prefs keys

prefs_glade_schemas = $(prefs_glade_files:.glade=.schemas)

build-schema-files:
	for p in $(prefs_glade_files); do \
		$(top_srcdir)/scripts/glade2schema.pl $$p > `basename $$p .glade`.schemas; \
	done
	
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
