## Copyright (c) 2001
## The gtkmm development team.

sublib_name = libanjutamm
sublib_namespace = Anjuta
sublib_parentdir = libanjutamm
files_defs = anjuta.defs anjuta_enums.defs

include ../build_shared/Makefile_gensrc.am_fragment

anjuta_enums.defs:
	perl ../enums.pl ../../libanjuta/*.h > anjuta_enums.defs

anjuta.defs:
	../h2def.py ../../libanjuta/*.h > anjuta.defs
