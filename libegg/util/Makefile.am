
BUILT_SOURCES = eggmarshalers.h

eggmarshalers.h: $(srcdir)/../eggmarshalers.list
	cd $(srcdir) \
	&& $(GLIB_GENMARSHAL) --prefix=_egg_marshal $(srcdir)/../eggmarshalers.list --header > xgen-emh \
	&& cp xgen-emh eggmarshalers.h \
	&& rm -f xgen-emh xgen-emh~

noinst_HEADERS = \
	eggmarshalers.h \
	eggintl.h \
	egg-macros.h
