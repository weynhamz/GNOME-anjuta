# Targets for handing ui-to-GConf schema conversion for prefs keys

prefs_ui_schemas = $(prefs_ui_files:.ui=.gschema.xml)

# gsettings_SCHEMAS is a list of all the schemas you want to install
gsettings_SCHEMAS = $(prefs_name).gschema.xml

$(prefs_name).gschema.xml: $(prefs_ui_schemas)
	mv -f $< $@

%.gschema.xml: %.ui
	$(AM_V_GEN)$(top_srcdir)/scripts/builder2schema.pl $< $(prefs_name) $(prefs_keyfile:%=$(srcdir)/%) > $@

# include the appropriate makefile rules for schema handling
@GSETTINGS_RULES@

CLEANFILES = $(prefs_ui_schemas) $(gsettings_SCHEMAS)
