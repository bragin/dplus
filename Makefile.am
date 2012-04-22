SUBDIRS = lout dw dlib dpip src doc
if BUILD_DPI
SUBDIRS += dpid dpi
endif
if BUILD_TEST
SUBDIRS += test
endif

EXTRA_DIST = Doxyfile dillorc d_size.h
if BUILD_DPI
EXTRA_DIST += install-dpi-local
endif

sysconf_DATA = dillorc
