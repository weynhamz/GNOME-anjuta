
dock_headers = \
	egg-dock-object.h \
	egg-dock-master.h \
	egg-dock.h \
	egg-dock-item.h \
	egg-dock-item-grip.h \
	egg-dock-notebook.h \
	egg-dock-paned.h \
	egg-dock-tablabel.h \
	egg-dock-layout.h \
	egg-dock-placeholder.h

noinst_HEADERS = \
	$(dock_headers) \
	libeggdocktypebuiltins.h

BUILT_SOURCES = \
	libeggdocktypebuiltins.h

CLEANFILES = \
	libeggdocktypebuiltins.h

libeggdocktypebuiltins.h: $(dock_headers)
	@GLIB_MKENUMS@ \
			--fhead "#ifndef __LIBEGGDOCKTYPEBUILTINS_H__\n" \
			--fhead "#define __LIBEGGDOCKTYPEBUILTINS_H__ 1\n\n" \
			--fhead "#include <dock/egg-dock.h>\n\n" \
			--fhead "G_BEGIN_DECLS\n\n" \
			--ftail "G_END_DECLS\n\n" \
			--ftail "#endif /* __LIBEGGDOCKTYPEBUILTINS_H__ */\n" \
			--fprod "\n/* --- @filename@ --- */" \
			--eprod "#define EGG_TYPE_@ENUMSHORT@ @enum_name@_get_type()\n" \
			--eprod "GType @enum_name@_get_type (void);\n" \
		$(dock_headers) > xgen-gth \
	&& cp xgen-gth $(@F) \
	&& rm -f xgen-gth
