INCLUDES = \
	$(ANJUTA_CFLAGS)

noinst_LIBRARIES=libeggutil.a

eggmarshalers.h: eggmarshalers.list
	cd $(srcdir) \
	&& $(GLIB_GENMARSHAL) --prefix=_egg_marshal eggmarshalers.list --header > xgen-emh \
	&& cp xgen-emh eggmarshalers.h \
	&& rm -f xgen-emh xgen-emh~

eggmarshalers.c: eggmarshalers.list
	cd $(srcdir) \
	&& $(GLIB_GENMARSHAL) --prefix=_egg_marshal eggmarshalers.list --body > xgen-emc \
	&& cp xgen-emc eggmarshalers.c \
	&& rm -f xgen-emc xgen-emc~

egg-marshal.c: eggmarshalers.h eggmarshalers.c

libeggutil_a_SOURCES =	\
	egg-marshal.c

noinst_HEADERS = \
	eggmarshalers.h	\
	eggintl.h \
	egg-macros.h

EXTRA_DIST=			\
	eggmarshalers.list
