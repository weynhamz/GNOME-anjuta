# Targets for handing ui-to-GConf schema conversion for prefs keys

prefs_ui_schemasdir = @GCONF_SCHEMA_FILE_DIR@
prefs_ui_schemas = $(prefs_ui_files:.ui=.schemas)
prefs_ui_schemas_DATA = $(prefs_ui_schemas)

%.schemas: %.ui
	$(top_srcdir)/scripts/builder2schema.pl $< > $@

if GCONF_SCHEMAS_INSTALL
install-data-local: $(prefs_ui_schemas)
	        for p in $(prefs_ui_schemas) ; do \
	            GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $$p ; \
	        done
		@killall -1 gconfd-2 || true

uninstall-local: $(prefs_ui_schemas)
	        for p in $(prefs_ui_schemas) ; do \
	            GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-uninstall-rule $$p ; \
	        done
		@killall -1 gconfd-2 || true

else
install-data-local:

uninstall-local:
endif

CLEANFILES = $(prefs_ui_schemas)
